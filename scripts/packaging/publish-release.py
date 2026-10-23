#!/usr/bin/env python3
"""Publish a complete, checksummed Linux build without replacing public releases."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


def collect_packages(root, version):
    patterns = {
        'Ubuntu 24.04 (amd64)': rf'penguin-studio_{re.escape(version)}-\d+\+ubuntu24\.04_amd64\.deb',
        'Ubuntu 26.04 / 26.04.1 (amd64)': rf'penguin-studio_{re.escape(version)}-\d+\+ubuntu26\.04_amd64\.deb',
        'Fedora 44 (x86_64)': rf'penguin-studio-{re.escape(version)}-[^.]+(?:\.fc44)?\.x86_64\.rpm',
        'AppImage (x86_64, glibc 2.39+)': rf'penguin-studio-{re.escape(version)}-x86_64\.AppImage',
        'Arch Linux / AUR recipe (x86_64)': rf'penguin-studio-{re.escape(version)}-aur\.tar\.gz',
        'Flatpak (x86_64)': rf'penguin-studio-{re.escape(version)}-x86_64\.flatpak',
        'Snap core24 (amd64)': rf'penguin-studio_{re.escape(version)}_amd64\.snap',
    }
    packages = [p for p in root.rglob('*') if p.suffix in ('.deb', '.rpm', '.flatpak', '.snap', '.AppImage') or p.name.endswith('-aur.tar.gz')]
    selected = {}
    for target, pattern in patterns.items():
        matches = [p for p in packages if re.fullmatch(pattern, p.name)]
        if len(matches) != 1:
            raise ValueError(f'{target}: expected exactly one matching package, got {matches}')
        package = matches[0]
        if package.stat().st_size == 0:
            raise ValueError(f'Empty package: {package}')
        selected[target] = package
    if len(packages) != len(selected):
        raise ValueError('Unexpected packages in release artifacts')
    return selected


def gh(*args):
    return subprocess.check_output(['gh', *args], text=True).strip()


def main():
    version = os.environ['RELEASE_VERSION']
    if version != Path('packaging/version.txt').read_text().strip():
        raise ValueError('Release version differs from checkout')
    selected = collect_packages(Path(sys.argv[1]), version)
    repo, sha = os.environ['GITHUB_REPOSITORY'], os.environ['GITHUB_SHA']
    tag = f'penguin-v{version}'
    output = Path('release-assets')
    output.mkdir(exist_ok=False)
    assets = []
    for target, path in selected.items():
        shutil.copyfile(path, output / path.name)
        with path.open('rb') as stream:
            digest = hashlib.file_digest(stream, 'sha256').hexdigest()
        assets.append(dict(target=target, file=path.name, sha256=digest, bytes=path.stat().st_size))
    (output / 'SHA256SUMS').write_text(''.join(f"{a['sha256']}  {a['file']}\n" for a in assets))
    manifest = dict(version=version, source_commit=sha,
                    workflow_url=f"https://github.com/{repo}/actions/runs/{os.environ['GITHUB_RUN_ID']}",
                    packages=assets)
    (output / 'build-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    notes = Path('release-notes.md')
    notes.write_text(f'''Penguin Studio {version} — Linux development preview

Download the package matching your system from Assets below. All package builds
and their automated checks passed before publication. Full live-provider,
hardware and cross-desktop testing is still incomplete; see docs/VERIFICATION.md.

| Target | Download |
| --- | --- |
''' + ''.join(f"| {a['target']} | [{a['file']}](https://github.com/{repo}/releases/download/{tag}/{a['file']}) |\n" for a in assets) + f'''
Ubuntu DEBs are release-specific; do not install the 24.04 package on 26.04.
The RPM targets Fedora 44. Flatpak requires the Freedesktop 25.08 runtime
from Flathub. Snap requires snapd and local installation with --dangerous;
it is a development build, not a Snap Store publication.
AppImage targets x86_64 hosts with glibc 2.39 or newer. Make it executable and
launch it; `--appimage-extract-and-run` avoids a FUSE dependency. The AUR archive
contains a checksum-pinned `penguin-studio-bin` recipe, not a claim of publication
on aur.archlinux.org. Extract it, review PKGBUILD, then run `makepkg -si` on Arch.
AppImage and Arch installations currently use manual/package-manager updates;
the in-app updater offers release links instead of replacing those installations.

Verify downloaded files with `sha256sum --check SHA256SUMS` after downloading
all listed files, or compare the entry for the package you downloaded.
`build-manifest.json` records the exact source commit and package hashes.

Source: `{sha}`. [Build and checks]({manifest['workflow_url']}).
''')
    version_notes = Path('docs/releases') / f'{version}.md'
    if version_notes.is_file():
        notes.write_text(notes.read_text() + '\n' + version_notes.read_text())
    # The checks job verifies the reviewed English preview manifest for this
    # version. Pin image URLs to the built commit so older releases stay honest.
    previews = '\n### Current English interface\n\n'
    previews += f'Penguin Studio {version}, captured from the real application with local demo media.\n\n'
    for label, filename in [('Workspace', 'workspace-en.png'),
                            ('Camera appearance', 'camera-en.png'),
                            ('Stream details before sign-in', 'stream-details-en.png')]:
        previews += f'![{label}](https://raw.githubusercontent.com/{repo}/{sha}/docs/images/current/{filename})\n\n'
    notes.write_text(notes.read_text() + previews)
    # An existing tag must identify precisely the built source. Never move it.
    refs = json.loads(gh('api', f'repos/{repo}/git/matching-refs/tags/{tag}'))
    exact = [r for r in refs if r['ref'] == f'refs/tags/{tag}']
    if exact:
        obj = exact[0]['object']
        while obj['type'] == 'tag':
            obj = json.loads(gh('api', f"repos/{repo}/git/tags/{obj['sha']}"))['object']
        if obj['sha'] != sha:
            raise ValueError('Existing version tag points to different source; increase version')
    else:
        gh('api', f'repos/{repo}/git/refs', '-f', f'ref=refs/tags/{tag}', '-f', f'sha={sha}')
    # Query success is required: API/auth failures must not look like missing releases.
    pages = json.loads(gh('api', '--paginate', '--slurp', f'repos/{repo}/releases?per_page=100'))
    releases = [r for page in pages for r in page if r['tag_name'] == tag]
    if releases and not releases[0]['draft']:
        print(f'Release {tag} already published for this source; leaving it unchanged.')
        return
    if not releases:
        gh('release', 'create', tag, '--repo', repo, '--verify-tag', '--draft',
           '--title', f'Penguin Studio {version}', '--notes-file', str(notes))
    gh('release', 'upload', tag, '--repo', repo, '--clobber',
       *(str(p) for p in sorted(output.iterdir())))
    # Publish only after every upload succeeded, so incomplete sets remain drafts.
    gh('release', 'edit', tag, '--repo', repo, '--draft=false', '--prerelease',
       '--notes-file', str(notes))
    print(f'https://github.com/{repo}/releases/tag/{tag}')


if __name__ == '__main__':
    main()
