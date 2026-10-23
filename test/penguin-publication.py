#!/usr/bin/env python3
"""Candidate captures may be pending; a publish must always reject them."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('publication', Path(__file__).resolve().parents[1] / 'scripts/check-publication.py')
publication = importlib.util.module_from_spec(spec)
spec.loader.exec_module(publication)

class PublicationTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        for name in ('packaging', 'docs/images/current'):
            (self.root / name).mkdir(parents=True)
        (self.root / 'packaging/version.txt').write_text('0.6.0\n')
        (self.root / 'README.md').write_text('Current release: [Penguin Studio 0.6.0](https://example.test)')
        (self.root / 'docs/PACKAGING.md').write_text('The current source version is **0.6.0**')
        for name in ('APPIMAGE-AUR', 'WORKSPACE', 'CAMERA-EFFECTS', 'PLATFORMS'):
            (self.root / f'docs/{name}.md').write_text('Current product documentation')
        self.manifest = {'version': '0.6.0', 'language': 'en-US', 'images': []}
        for name in ('workspace-en.png', 'camera-en.png', 'stream-details-en.png'):
            data = b'\x89PNG\r\n\x1a\n' + name.encode()
            (self.root / 'docs/images/current' / name).write_bytes(data)
            self.manifest['images'].append({'file': name, 'sha256': hashlib.sha256(data).hexdigest()})
        self.save()

    def save(self):
        (self.root / 'docs/images/current/manifest.json').write_text(json.dumps(self.manifest))

    def test_current(self):
        publication.validate(self.root)

    def test_pending_only_allowed_for_candidate(self):
        self.manifest.update(version='0.5.3', pending_version='0.6.0')
        self.save()
        with self.assertRaises(ValueError):
            publication.validate(self.root)
        publication.validate(self.root, candidate=True)
        self.manifest['pending_version'] = '0.7.0'
        self.save()
        with self.assertRaises(ValueError):
            publication.validate(self.root, candidate=True)

    def test_candidate_still_checks_images(self):
        (self.root / 'docs/images/current/camera-en.png').write_bytes(b'changed')
        with self.assertRaises(ValueError):
            publication.validate(self.root, candidate=True)

    def test_stale_text_and_wrong_language(self):
        self.manifest['language'] = 'de-DE'
        self.save()
        with self.assertRaises(ValueError):
            publication.validate(self.root, candidate=True)
        self.manifest['language'] = 'en-US'
        self.save()
        (self.root / 'docs/WORKSPACE.md').write_text('Penguin Studio 0.5.3')
        with self.assertRaises(ValueError):
            publication.validate(self.root, candidate=True)

if __name__ == '__main__':
    unittest.main()
