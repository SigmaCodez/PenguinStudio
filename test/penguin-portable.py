#!/usr/bin/env python3
"""Check portable entrypoint symlinks and AUR checksum/version binding."""
import hashlib
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class PortablePackages(unittest.TestCase):
    def test_apprun_resolves_launcher_and_preserves_arguments(self):
        with tempfile.TemporaryDirectory(prefix='penguin portable ') as directory:
            app = Path(directory) / 'AppDir'
            binary = app / 'usr/bin'
            binary.mkdir(parents=True)
            launcher = binary / 'penguin-studio'
            shutil.copyfile(ROOT / 'packaging/appimage/penguin-studio', launcher)
            launcher.chmod(0o755)
            obs = binary / 'obs'
            obs.write_text('#!/bin/sh\nprintf "%s\\n%s\\n" "$PYTHONHOME" "$1"\n')
            obs.chmod(0o755)
            (app / 'AppRun').symlink_to('usr/bin/penguin-studio')
            for entry in (launcher, app / 'AppRun'):
                lines = subprocess.check_output([str(entry), 'argument with spaces'], text=True).splitlines()
                self.assertEqual(Path(lines[0]).resolve(), app / 'usr')
                self.assertEqual(lines[1], 'argument with spaces')

    def test_recipe_pins_exact_image(self):
        version = (ROOT / 'packaging/version.txt').read_text().strip()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            image = root / f'penguin-studio-{version}-x86_64.AppImage'
            image.write_bytes(b'fixture for digest validation')
            script = ROOT / 'scripts/packaging/prepare-aur.py'
            subprocess.run(['python3', str(script), str(image), str(root / 'recipe')], check=True)
            recipe = (root / 'recipe/PKGBUILD').read_text()
            self.assertIn(hashlib.sha256(image.read_bytes()).hexdigest(), recipe)
            self.assertIn(f'pkgver={version}', recipe)
            self.assertNotIn('SKIP', recipe)
            subprocess.run(['bash', '-n', str(root / 'recipe/PKGBUILD')], check=True)
            image.write_bytes(b'')
            result = subprocess.run(['python3', str(script), str(image), str(root / 'empty')], capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse((root / 'empty').exists())


if __name__ == '__main__':
    unittest.main()
