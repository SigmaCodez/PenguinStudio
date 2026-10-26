# SPDX-License-Identifier: GPL-2.0-or-later
import hashlib
from pathlib import Path
import subprocess
import tempfile
import unittest
import installer


class PolicyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / 'root'
        (self.root / 'DEBIAN').mkdir(parents=True)
        self.worker = self.root / installer.WORKER
        self.worker.parent.mkdir(parents=True)
        self.worker.write_text('baseline test data')
        self.worker.chmod(0o755)
        self.readme = self.root / 'usr/share/doc/penguin-studio/README'
        self.readme.parent.mkdir(parents=True)
        self.readme.write_text('Pinned test documentation')
        self.control = self.root / 'DEBIAN/control'
        self.control.write_text('Package: penguin-studio\nVersion: 0.7.1-1+ubuntu26.04.local1\n'
            'Architecture: amd64\nMaintainer: Test <test@example.invalid>\n'
            'Description: Test fixture only\nDepends: libc6\nInstalled-Size: 1\n')
        (self.root / 'DEBIAN/triggers').write_text('activate-noawait ldconfig\n')
        baseline = self.package()
        control = installer.members(baseline, True)
        fields = installer.fields(control['control']['text'])
        fields.pop('Version')
        fields.pop('Installed-Size')
        self.policy = {'version_prefix': '0.7.1-1+ubuntu26.04', 'control': control,
                       'fields': fields, 'payload': installer.members(baseline)}

    def package(self, checksums=True):
        if checksums:
            entries = []
            for f in sorted((self.root / 'usr').rglob('*')):
                if f.is_file() and not f.is_symlink():
                    entries.append(hashlib.md5(f.read_bytes(), usedforsecurity=False).hexdigest() + '  ' +
                                   f.relative_to(self.root).as_posix())
            (self.root / 'DEBIAN/md5sums').write_text('\n'.join(entries) + '\n')
        output = Path(self.temp.name) / 'fixture.deb'
        subprocess.run(['/usr/bin/dpkg-deb', '--root-owner-group', '-Znone', '--build', str(self.root), str(output)],
                       check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        return output

    def refused(self):
        with self.assertRaises(ValueError):
            installer.verify(self.package(), self.policy)

    def test_worker_update_accepted(self):
        self.worker.write_text('updated test data')
        self.assertEqual(installer.verify(self.package(), self.policy), '0.7.1-1+ubuntu26.04.local1')

    def test_unapproved_file_change(self):
        self.readme.write_text('changed')
        self.refused()

    def test_added_path(self):
        (self.readme.parent / 'new-file').write_text('new')
        self.refused()

    def test_removed_path(self):
        self.readme.unlink()
        self.refused()

    def test_maintainer_script(self):
        (self.root / 'DEBIAN/postinst').write_text('# Test data; never executed\n')
        (self.root / 'DEBIAN/postinst').chmod(0o755)
        self.refused()

    def test_dependency_change(self):
        self.control.write_text(self.control.read_text().replace('Depends: libc6', 'Depends: libc6, zlib1g'))
        self.refused()

    def test_worker_symlink(self):
        self.worker.unlink()
        self.worker.symlink_to('other-file')
        self.refused()

    def test_permission_change(self):
        self.readme.chmod(0o755)
        self.refused()

    def test_public_version_change(self):
        self.control.write_text(self.control.read_text().replace('0.7.1-', '0.7.2-'))
        self.refused()

    def test_checksum_mismatch(self):
        self.worker.write_text('new bytes')
        with self.assertRaises(ValueError):
            installer.verify(self.package(checksums=False), self.policy)


if __name__ == '__main__':
    unittest.main()
