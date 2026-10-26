#!/usr/bin/env python3
"""Regression checks for release triggers and incomplete/wrong-version artifacts."""
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'scripts/packaging' / f'{name}.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


version = load('release-version')
publish = load('publish-release')


class Releases(unittest.TestCase):
    def test_triggers(self):
        cases = [
            ('push', 'refs/heads/main', '0.1.0', False, 'all', True),
            ('push', 'refs/heads/main', '0.2.0', False, 'all', False),
            ('pull_request', 'refs/pull/1/merge', '0.1.0', False, 'all', False),
            ('push', 'refs/heads/penguin/test', '0.1.0', False, 'all', False),
            ('push', 'refs/tags/penguin-v0.2.0', '0.2.0', False, 'all', True),
            ('workflow_dispatch', 'refs/heads/main', '0.2.0', True, 'all', True),
            ('workflow_dispatch', 'refs/heads/main', '0.2.0', False, 'deb', False),
        ]
        for event, ref, previous, requested, selection, expected in cases:
            with self.subTest(event=event, ref=ref, requested=requested):
                self.assertEqual(version.release_requested('0.2.0', event, ref, previous, requested, selection), expected)

    def test_reject_unsafe_release_requests(self):
        for args in [
            ('0.2.0', 'workflow_dispatch', 'refs/heads/main', '0.2.0', True, 'deb'),
            ('0.2.0', 'workflow_dispatch', 'refs/heads/topic', '0.2.0', True, 'all'),
            ('0.2.0', 'push', 'refs/tags/penguin-v0.3.0', '0.2.0', False, 'all'),
            ('0.2.0', 'push', 'refs/heads/main', '0.3.0', False, 'all'),
            ('0.2.0;bad', 'push', 'refs/heads/main', '0.1.0', False, 'all'),
        ]:
            with self.subTest(args=args), self.assertRaises(ValueError):
                version.release_requested(*args)

    def test_upload_failure_keeps_release_unpublished(self):
        for fail_upload in (False, True):
            with self.subTest(fail_upload=fail_upload), tempfile.TemporaryDirectory() as directory:
                previous = Path.cwd()
                os.chdir(directory)
                try:
                    Path('packaging').mkdir()
                    Path('packaging/version.txt').write_text('0.2.0\n')
                    package = Path('fixture.deb')
                    package.write_bytes(b'fixture')
                    calls = []

                    def fake_gh(*args):
                        calls.append(args)
                        if args[:2] == ('release', 'upload') and fail_upload:
                            raise RuntimeError('simulated upload failure')
                        return '[]' if args[0] == 'api' else ''

                    environment = dict(RELEASE_VERSION='0.2.0', GITHUB_REPOSITORY='test/test',
                                       GITHUB_SHA='a' * 40, GITHUB_RUN_ID='1')
                    with patch.object(publish, 'gh', fake_gh), \
                         patch.object(publish, 'collect_packages', return_value={'fixture': package}), \
                         patch.dict(os.environ, environment), patch('sys.argv', ['publish', '.']):
                        if fail_upload:
                            with self.assertRaises(RuntimeError):
                                publish.main()
                        else:
                            publish.main()
                    edited = any(call[:2] == ('release', 'edit') for call in calls)
                    self.assertEqual(edited, not fail_upload)
                    manifest = json.loads(Path('release-assets/build-manifest.json').read_text())
                    self.assertEqual(manifest['source_commit'], 'a' * 40)
                    self.assertEqual(manifest['packages'][0]['bytes'], len(b'fixture'))
                    notes = Path('release-notes.md').read_text()
                    for name in ('workspace-en.png', 'camera-en.png', 'stream-details-en.png'):
                        self.assertIn(f'https://raw.githubusercontent.com/test/test/{"a" * 40}/docs/images/current/{name}', notes)
                finally:
                    os.chdir(previous)

    def test_gpu_addon_required_for_new_release(self):
        names = ['penguin-studio_0.6.3-1+ubuntu24.04_amd64.deb',
                 'penguin-studio_0.6.3-1+ubuntu26.04_amd64.deb',
                 'penguin-studio-0.6.3-1.x86_64.rpm',
                 'penguin-studio-0.6.3-x86_64.flatpak',
                 'penguin-studio_0.6.3_amd64.snap',
                 'penguin-studio-0.6.3-x86_64.AppImage',
                 'penguin-studio-0.6.3-aur.tar.gz']
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in names:
                (root / name).write_bytes(b'test artifact')
            with self.assertRaises(ValueError):
                publish.collect_packages(root, '0.6.3')
            (root / 'penguin-studio-camera-amd-gfx1201_0.6.3-1+ubuntu26.04_amd64.deb').write_bytes(b'runtime')
            self.assertEqual(len(publish.collect_packages(root, '0.6.3')), 8)

    def test_obs33_package_set(self):
        names = ['penguin-studio_0.7.0-1+ubuntu26.04_amd64.deb',
                 'penguin-studio-camera-amd-gfx1201_0.7.0-1+ubuntu26.04_amd64.deb',
                 'penguin-studio-0.7.0-1.x86_64.rpm',
                 'penguin-studio-0.7.0-x86_64.flatpak',
                 'penguin-studio_0.7.0_amd64.snap',
                 'penguin-studio-0.7.0-x86_64.AppImage',
                 'penguin-studio-0.7.0-aur.tar.gz']
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in names:
                (root / name).write_bytes(b'artifact')
            self.assertEqual(len(publish.collect_packages(root, '0.7.0')), 7)
            # Never accidentally publish an old-ABI DEB under the new release.
            (root / 'penguin-studio_0.7.0-1+ubuntu24.04_amd64.deb').write_bytes(b'wrong ABI')
            with self.assertRaises(ValueError):
                publish.collect_packages(root, '0.7.0')

    def test_ubuntu_2610_required_for_new_release(self):
        names = ['penguin-studio_0.7.2-1+ubuntu26.04_amd64.deb',
                 'penguin-studio-camera-amd-gfx1201_0.7.2-1+ubuntu26.04_amd64.deb',
                 'penguin-studio-0.7.2-1.x86_64.rpm',
                 'penguin-studio-0.7.2-x86_64.flatpak',
                 'penguin-studio_0.7.2_amd64.snap',
                 'penguin-studio-0.7.2-x86_64.AppImage',
                 'penguin-studio-0.7.2-aur.tar.gz']
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in names:
                (root / name).write_bytes(b'artifact')
            with self.assertRaises(ValueError):
                publish.collect_packages(root, '0.7.2')
            (root / 'penguin-studio_0.7.2-1+ubuntu26.10_amd64.deb').write_bytes(b'interim artifact')
            self.assertEqual(len(publish.collect_packages(root, '0.7.2')), 8)

    def test_complete_package_set(self):
        names = ['penguin-studio_0.2.0-6+ubuntu24.04_amd64.deb',
                 'penguin-studio_0.2.0-6+ubuntu26.04_amd64.deb',
                 'penguin-studio-0.2.0-1.x86_64.rpm',
                 'penguin-studio-0.2.0-x86_64.flatpak',
                 'penguin-studio_0.2.0_amd64.snap',
                 'penguin-studio-0.2.0-x86_64.AppImage',
                 'penguin-studio-0.2.0-aur.tar.gz']
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in names:
                (root / name).write_bytes(b'test artifact')
            self.assertEqual(len(publish.collect_packages(root, '0.2.0')), 7)
            with self.assertRaises(ValueError):
                publish.collect_packages(root, '0.3.0')
            (root / names[-1]).unlink()
            with self.assertRaises(ValueError):
                publish.collect_packages(root, '0.2.0')
            (root / names[-1]).touch()
            with self.assertRaises(ValueError):
                publish.collect_packages(root, '0.2.0')
            (root / names[-1]).write_bytes(b'test artifact')
            (root / 'duplicate').mkdir()
            (root / 'duplicate' / names[0]).write_bytes(b'test artifact')
            with self.assertRaises(ValueError):
                publish.collect_packages(root, '0.2.0')


if __name__ == '__main__':
    unittest.main()
