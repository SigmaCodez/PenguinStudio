#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""One-time reviewed setup. Always requires normal administrator authorization."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT_SETUP = r'''
import hashlib, json, os, pathlib, pwd, shutil, stat, subprocess, sys, tempfile
assert os.geteuid() == 0
sources = json.loads(sys.argv[1])
assert pwd.getpwnam('ivan').pw_uid == 1000
paths = [pathlib.Path('/usr/local/sbin/penguin-local-update'),
         pathlib.Path('/etc/penguin-local-update.json'),
         pathlib.Path('/etc/sudoers.d/penguin-local-update')]
if any(p.exists() or p.is_symlink() for p in paths):
    raise SystemExit('An existing authorization must be reviewed separately; nothing was replaced.')
env = {'PATH':'/usr/sbin:/usr/bin:/sbin:/bin', 'HOME':'/root', 'LC_ALL':'C'}
created = []
with tempfile.TemporaryDirectory(prefix='penguin-authorization-', dir='/var/tmp') as directory:
    staged = []
    for name in ('installer.py', 'policy.json', 'sudoers'):
        source, expected = sources[name]
        content = pathlib.Path(source).read_bytes()
        if len(content) > 16 * 1024 * 1024 or hashlib.sha256(content).hexdigest() != expected:
            raise SystemExit('Prepared authorization changed; refusing setup.')
        item = pathlib.Path(directory) / name
        item.write_bytes(content)
        item.chmod(0o600)
        staged.append(item)
    policy = json.loads(staged[1].read_text())
    assert policy['uid'] == 1000 and policy['package'] == 'penguin-studio'
    assert policy['incoming'] == '/home/ivan/.cache/penguin-studio-updates/candidate.deb'
    assert staged[2].read_text() == 'ivan ALL=(root) NOPASSWD: /usr/local/sbin/penguin-local-update ""\n'
    subprocess.run(['/usr/sbin/visudo', '-cf', str(staged[2])], env=env, check=True)
    state = pathlib.Path('/var/lib/penguin-local-update')
    if not state.exists():
        state.mkdir(mode=0o700)
    state_info = state.lstat()
    assert stat.S_ISDIR(state_info.st_mode) and state_info.st_uid == 0 and not state_info.st_mode & 0o077
    try:
        for source, destination, mode in zip(staged, paths, (0o755, 0o600, 0o440)):
            parent = destination.parent.stat()
            assert parent.st_uid == 0 and not parent.st_mode & 0o022
            # Exclusive creation prevents overwriting a concurrent setup.
            with destination.open('xb') as target:
                created.append(destination)
                target.write(source.read_bytes())
                os.fchmod(target.fileno(), mode)
                os.fsync(target.fileno())
        subprocess.run(['/usr/sbin/visudo', '-c'], env=env, check=True)
    except BaseException:
        for destination in reversed(created):
            destination.unlink()
        raise
print('Restricted Penguin Studio GPU-worker update authorization installed. No password was stored.')
'''


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('bundle', type=Path, help='Reviewed installer.py, policy.json and sudoers directory')
    p.add_argument('--install', action='store_true', help='Request normal Ubuntu administrator authorization')
    args = p.parse_args()
    bundle = args.bundle.resolve()
    sources = {name: [str(bundle / name), hashlib.sha256((bundle / name).read_bytes()).hexdigest()]
               for name in ('installer.py', 'policy.json', 'sudoers')}
    compile(ROOT_SETUP, '<root-setup>', 'exec')
    if not args.install:
        print('Prepared hashes; no system changes. Use --install only after reviewing the authorization scope.')
        print(json.dumps(sources, indent=2))
        return
    subprocess.run(['pkexec', '/usr/bin/python3', '-I', '-c', ROOT_SETUP, json.dumps(sources)], check=True)


if __name__ == '__main__':
    main()
