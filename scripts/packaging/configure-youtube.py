#!/usr/bin/env python3
"""Inject publisher desktop OAuth configuration into a package checkout.

Desktop OAuth credentials are extractable from installed binaries. Never supply
web/server credentials or user access/refresh tokens here.
"""
import argparse
import json
import os
from pathlib import Path
import re


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--required', action='store_true')
    parser.add_argument('--verify', type=Path, help='Verify compiled platform modules below this directory')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    target = root / 'plugins/penguin-platforms/youtube-app-config.hpp'
    if args.verify is not None:
        if not target.exists():
            if args.required:
                raise SystemExit('Publisher YouTube configuration is missing.')
            print('Custom-configuration development build; no publisher credentials to verify.')
            return
        defines = dict(re.findall(r'^#define (PENGUIN_YOUTUBE_CLIENT_\w+) (.+)$',
                                  target.read_text(), re.MULTILINE))
        values = [json.loads(defines[key]) for key in
                  ('PENGUIN_YOUTUBE_CLIENT_ID', 'PENGUIN_YOUTUBE_CLIENT_SECRET')]
        libraries = list(args.verify.rglob('penguin-platforms.so'))
        if not libraries:
            raise SystemExit('No compiled Penguin platform module found for verification.')
        for library in libraries:
            data = library.read_bytes()
            if not all(value and (value.encode('utf-16-le') in data or value.encode() in data)
                       for value in values):
                raise SystemExit('Compiled platform module is missing publisher YouTube configuration.')
        print('Publisher YouTube configuration verified in compiled platform modules.')
        return
    client = os.environ.get('PENGUIN_YOUTUBE_CLIENT_ID', '').strip()
    secret = os.environ.get('PENGUIN_YOUTUBE_CLIENT_SECRET', '').strip()
    if not client and not secret and not args.required:
        print('Publisher YouTube configuration unavailable; custom application setup remains available.')
        return
    if not re.fullmatch(r'[A-Za-z0-9_-]+\.apps\.googleusercontent\.com', client):
        raise SystemExit('A Google Desktop app client ID is required.')
    if not re.fullmatch(r'[A-Za-z0-9_-]+', secret):
        raise SystemExit('A Google Desktop app client secret is required.')
    target.write_text(
        '// Generated publisher Desktop OAuth configuration. Do not commit.\n'
        '#pragma once\n'
        f'#define PENGUIN_YOUTUBE_CLIENT_ID {json.dumps(client)}\n'
        f'#define PENGUIN_YOUTUBE_CLIENT_SECRET {json.dumps(secret)}\n',
        encoding='utf-8',
    )
    print('Publisher YouTube browser sign-in configured.')


if __name__ == '__main__':
    main()
