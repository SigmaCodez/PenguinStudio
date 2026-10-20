#!/usr/bin/env python3
"""Select releases from version changes, matching tags, or explicit dispatches."""
import os
from pathlib import Path
import re
import subprocess


def release_requested(version, event, ref, previous, requested, selection):
    if not re.fullmatch(r'(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)', version):
        raise ValueError('packaging/version.txt must contain a numeric major.minor.patch version')
    if event == 'workflow_dispatch' and requested:
        if ref != 'refs/heads/main' or selection != 'all':
            raise ValueError('Publishing requires main and the complete all package selection')
        return True
    if event == 'push' and ref.startswith('refs/tags/penguin-v'):
        if ref != f'refs/tags/penguin-v{version}':
            raise ValueError('Release tag does not match packaging/version.txt')
        return True
    if event == 'push' and ref == 'refs/heads/main' and previous != version:
        if previous and tuple(map(int, version.split('.'))) <= tuple(map(int, previous.split('.'))):
            raise ValueError('Release versions must increase')
        return True
    return False


def main():
    version = Path('packaging/version.txt').read_text().strip()
    event, ref = os.environ['GITHUB_EVENT_NAME'], os.environ['GITHUB_REF']
    previous = version
    before = os.environ.get('BEFORE_SHA', '')
    if event == 'push' and ref == 'refs/heads/main':
        if before and set(before) != {'0'}:
            previous = subprocess.check_output(
                ['git', 'show', f'{before}:packaging/version.txt'], text=True).strip()
        else:
            previous = ''
    publish = release_requested(version, event, ref, previous,
                                os.environ.get('REQUEST_PUBLISH') == 'true',
                                os.environ.get('PACKAGE_SELECTION', 'all'))
    with open(os.environ['GITHUB_OUTPUT'], 'a') as output:
        output.write(f'version={version}\npublish={str(publish).lower()}\n')
    print(f'Penguin Studio {version}; publish={publish}')


if __name__ == '__main__':
    main()
