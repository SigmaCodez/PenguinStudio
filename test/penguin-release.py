#!/usr/bin/env python3
"""Regression checks for release triggers and incomplete/wrong-version artifacts."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

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

    def test_complete_package_set(self):
        names = ['penguin-studio_0.2.0-6+ubuntu24.04_amd64.deb',
                 'penguin-studio_0.2.0-6+ubuntu26.04_amd64.deb',
                 'penguin-studio-0.2.0-1.x86_64.rpm',
                 'penguin-studio-0.2.0-x86_64.flatpak',
                 'penguin-studio_0.2.0_amd64.snap']
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in names:
                (root / name).write_bytes(b'test artifact')
            self.assertEqual(len(publish.collect_packages(root, '0.2.0')), 5)
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
