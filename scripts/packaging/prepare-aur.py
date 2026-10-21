#!/usr/bin/env python3
"""Generate the binary AUR recipe from the exact AppImage built in this run."""
import hashlib
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[2]
version = (root / 'packaging/version.txt').read_text().strip()
if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', version):
    raise SystemExit('Invalid version')
image = Path(sys.argv[1])
if image.name != f'penguin-studio-{version}-x86_64.AppImage' or image.stat().st_size == 0:
    raise SystemExit('Wrong or empty AppImage')
with image.open('rb') as stream:
    digest = hashlib.file_digest(stream, 'sha256').hexdigest()
out = Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=False)
template = (root / 'packaging/aur/PKGBUILD.in').read_text()
(out / 'PKGBUILD').write_text(template.replace('@VERSION@', version).replace('@SHA256@', digest))
print(out / 'PKGBUILD')
