#!/usr/bin/env python3
"""Exercise the DEB preflight against real dpkg/APT without installing anything."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
host = {}
for line in Path('/etc/os-release').read_text().splitlines():
    if '=' in line:
        key, value = line.split('=', 1)
        host[key] = value.strip('"')
if host['ID'] not in ('ubuntu', 'debian'):
    raise SystemExit('Run these package checks on Ubuntu or Debian.')
arch = subprocess.check_output(['dpkg', '--print-architecture'], text=True).strip()
version = '0.2.0-1+' + host['ID'] + host['VERSION_ID']
cases = [
    ('matching-release', version, arch, 'libc6', 0, 'checks passed'),
    ('legacy-unlabelled', '0.2.0', arch, 'libc6', 2, 'does not identify'),
    ('wrong-release', '0.2.0-1+ubuntu99.99', arch, 'libc6', 2, 'Package targets'),
    ('wrong-architecture', version, 'arm64' if arch != 'arm64' else 'amd64', 'libc6', 2, 'architecture does not match'),
    ('missing-dependency', version, arch, 'penguin-test-nonexistent-dependency', 100, 'not installable'),
]
with tempfile.TemporaryDirectory(prefix='penguin-deb-preflight-') as folder:
    for name, package_version, package_arch, depends, expected_code, expected_text in cases:
        stage = Path(folder) / name
        control = stage / 'DEBIAN'
        control.mkdir(parents=True)
        (control / 'control').write_text(
            f'Package: penguin-studio\nVersion: {package_version}\nArchitecture: {package_arch}\n'
            f'Maintainer: Penguin test <test@example.invalid>\nDepends: {depends}\n'
            'Description: Preflight-only fixture; do not install\n')
        deb = stage.with_suffix('.deb')
        subprocess.run(['dpkg-deb', '--root-owner-group', '--build', str(stage), str(deb)], check=True, capture_output=True)
        result = subprocess.run(['bash', str(root / 'scripts/packaging/install-deb.sh'), '--check', str(deb)],
                                env=dict(os.environ, LC_ALL='C'), text=True, capture_output=True)
        output = result.stdout + result.stderr
        assert result.returncode == expected_code, f'{name}: exit {result.returncode}: {output}'
        assert expected_text in output, f'{name}: {output}'
        print(f'PASS: {name}')
print('All 5 DEB preflight cases passed; no packages were installed.')
