#!/usr/bin/python3 -I
# SPDX-License-Identifier: GPL-2.0-or-later
"""Root-owned, argument-free installer for an explicitly approved local payload.

The policy pins every file except the GPU worker. It is NOT a general-purpose
passwordless apt/dpkg wrapper. Policy expansion requires administrator review.
"""
import hashlib
import fcntl
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import subprocess
import sys
import tarfile
import tempfile

POLICY = Path('/etc/penguin-local-update.json')
STATE = Path('/var/lib/penguin-local-update')
ENV = {'PATH': '/usr/sbin:/usr/bin:/sbin:/bin', 'LC_ALL': 'C', 'HOME': '/root'}
WORKER = 'usr/lib/x86_64-linux-gnu/obs-modules/penguin-camera/penguin-gpu-worker'
MAX_PACKAGE = 800 * 1024 * 1024
MAX_EXPANDED = 1200 * 1024 * 1024


def require(condition, message):
    if not condition:
        raise ValueError(message)


def members(package, control=False, require_root=True):
    """Read through dpkg without extracting any untrusted filesystem entries."""
    cmd = ['/usr/bin/dpkg-deb', '--ctrl-tarfile' if control else '--fsys-tarfile', str(package)]
    result = {}
    total = 0
    with subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, env=ENV) as process:
        try:
            with tarfile.open(fileobj=process.stdout, mode='r|') as archive:
                for member in archive:
                    name = member.name.removeprefix('./').rstrip('/')
                    if not name or name == '.':
                        require(member.isdir(), 'Invalid archive root')
                        continue
                    parts = PurePosixPath(name).parts
                    require(not name.startswith('/') and '..' not in parts and str(PurePosixPath(name)) == name,
                            'Non-canonical archive path')
                    require(name not in result and len(result) < 30000, 'Duplicate or excessive archive entries')
                    require(not (member.mode & 0o7000), 'Special permission bits are forbidden')
                    require(member.uid == 0 and member.gid == 0 or control or not require_root, 'Payload ownership must be root:root')
                    require(not member.pax_headers, 'Extended archive attributes are not supported')
                    item = {'mode': member.mode & 0o777}
                    if member.isdir():
                        item['kind'] = 'directory'
                    elif member.issym():
                        item.update(kind='symlink', target=member.linkname)
                    elif member.isfile():
                        total += member.size
                        require(total <= MAX_EXPANDED and member.size <= MAX_EXPANDED, 'Payload too large')
                        item.update(kind='file', size=member.size)
                        digest = hashlib.sha256()
                        md5 = hashlib.md5(usedforsecurity=False)
                        data = bytearray() if control else None
                        with archive.extractfile(member) as stream:
                            for block in iter(lambda: stream.read(1024 * 1024), b''):
                                digest.update(block)
                                md5.update(block)
                                if data is not None:
                                    require(len(data) + len(block) <= 4 * 1024 * 1024, 'Control file too large')
                                    data.extend(block)
                        item['sha256'] = digest.hexdigest()
                        item['md5'] = md5.hexdigest()
                        if data is not None:
                            item['text'] = data.decode('utf-8')
                    else:
                        raise ValueError('Hardlinks and special file types are forbidden')
                    result[name] = item
        except BaseException:
            process.kill()
            process.wait()
            raise
        require(process.wait() == 0, 'Cannot read Debian archive')
    return result


def fields(text):
    result = {}
    key = None
    for line in text.splitlines():
        if line.startswith((' ', '\t')):
            require(key is not None, 'Invalid control continuation')
            result[key] += '\n' + line
        else:
            require(':' in line, 'Invalid control field')
            key, value = line.split(':', 1)
            require(key not in result, 'Duplicate control field')
            result[key] = value.strip()
    return result


def verify(package, policy):
    control = members(package, True)
    require(set(control) == set(policy['control']), 'Unexpected control files / maintainer scripts')
    for name, expected in policy['control'].items():
        if name not in ('control', 'md5sums'):
            require(control[name] == expected, 'Changed control metadata: ' + name)
        else:
            require(control[name]['kind'] == 'file' and control[name]['mode'] == expected['mode'],
                    'Invalid control file type or mode')
    metadata = fields(control['control']['text'])
    version = metadata.pop('Version', '')
    installed_size = metadata.pop('Installed-Size', '')
    require(installed_size.isdigit() and 0 < int(installed_size) <= MAX_EXPANDED // 1024, 'Invalid installed size')
    require(re.fullmatch(re.escape(policy['version_prefix']) + r'\.local[1-9][0-9]{0,5}', version),
            'Only approved local revisions are allowed')
    require(metadata == policy['fields'], 'Package identity, dependencies or metadata changed')
    payload = members(package)
    require(set(payload) == set(policy['payload']), 'Package paths changed')
    for name, expected in policy['payload'].items():
        actual = payload[name]
        if name == WORKER:
            require(actual['kind'] == 'file' and actual['mode'] == 0o755 and 0 < actual['size'] <= 64 * 1024 * 1024,
                    'Invalid GPU worker executable')
        else:
            require(actual == expected, 'Unapproved payload change: ' + name)
    # md5sums is data, not code; reject path injection and out-of-package paths.
    checksums = {}
    for line in control['md5sums']['text'].splitlines():
        require(re.fullmatch(r'[0-9a-f]{32}  [^\r\n]+', line), 'Invalid checksum entry')
        name = line[34:]
        require(name not in checksums and name in payload and payload[name]['kind'] == 'file',
                'Invalid checksum path')
        require(line[:32] == payload[name]['md5'], 'Incorrect payload checksum')
        checksums[name] = line[:32]
    require(set(checksums) == {name for name, item in payload.items() if item['kind'] == 'file'},
            'Incomplete checksum list')
    return version


def protected(path):
    info = path.lstat()
    require(info.st_uid == 0 and not info.st_mode & 0o022 and not stat.S_ISLNK(info.st_mode),
            'Installer configuration must be protected and root-owned')


def main():
    require(os.geteuid() == 0 and len(sys.argv) == 1, 'Run only the approved argument-free helper')
    protected(POLICY)
    protected(STATE)
    lock = (STATE / 'lock').open('a')
    fcntl.flock(lock.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
    policy = json.loads(POLICY.read_text())
    require(policy['package'] == 'penguin-studio', 'Unexpected policy')
    # A previously installed maintainer script must never run during upgrade.
    for script in ('preinst', 'postinst', 'prerm', 'postrm', 'config'):
        require(not Path('/var/lib/dpkg/info/penguin-studio.' + script).exists(),
                'Installed maintainer scripts require administrator review')
    busy = subprocess.run(['/usr/bin/pgrep', '-u', str(policy['uid']), '-x', 'obs'], env=ENV,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    require(busy.returncode == 1, 'Close Penguin Studio before updating')
    incoming = Path(policy['incoming'])
    fd = os.open(incoming, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    with os.fdopen(fd, 'rb') as source:
        info = os.fstat(source.fileno())
        require(stat.S_ISREG(info.st_mode) and info.st_uid == policy['uid'] and 0 < info.st_size <= MAX_PACKAGE,
                'Invalid local update candidate')
        with tempfile.TemporaryDirectory(prefix='install-', dir=STATE) as temp:
            package = Path(temp) / 'candidate.deb'
            # Validate the private copy, never a mutable user-controlled pathname.
            with package.open('xb') as target:
                remaining = MAX_PACKAGE + 1
                while remaining:
                    block = source.read(min(1024 * 1024, remaining))
                    if not block:
                        break
                    target.write(block)
                    remaining -= len(block)
                require(remaining > 0, 'Candidate grew beyond size limit')
            version = verify(package, policy)
            current = subprocess.check_output(['/usr/bin/dpkg-query', '-W', '-f=${Version}', 'penguin-studio'],
                                              env=ENV, text=True)
            require(current == policy['version_prefix'] or
                    re.fullmatch(re.escape(policy['version_prefix']) + r'\.local[1-9][0-9]{0,5}', current),
                    'Installed public version changed; review policy again')
            subprocess.run(['/usr/bin/dpkg', '--compare-versions', version, 'ge', current], env=ENV, check=True)
            # No dependency resolution, removals, shell evaluation or arbitrary arguments.
            subprocess.run(['/usr/bin/dpkg', '--install', str(package)], env=ENV, check=True)
            print('Installed approved Penguin Studio local revision ' + version)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print('Penguin Studio local update refused: ' + str(error), file=sys.stderr)
        sys.exit(1)
