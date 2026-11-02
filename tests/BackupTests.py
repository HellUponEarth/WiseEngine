#!/usr/bin/env python3
"""Recovery integrity regressions. How to run: python tests/BackupTests.py"""
import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('backup', ROOT / 'cmake/BackupMilestone.py')
backup = importlib.util.module_from_spec(spec)
spec.loader.exec_module(backup)


class RecoveryTests(unittest.TestCase):
    def setUp(self):
        (ROOT / 'build').mkdir(exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=ROOT / 'build')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / 'engine'
        self.root.mkdir()
        for name in ('src/Core/test.cpp', 'thirdparty/fmt/.git/HEAD', 'thirdparty/_downloads/sdk.zip',
                     'build/ignored.obj', 'backups/old.zip', 'thirdparty/python-runtime/Scripts/python.exe'):
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('recovery fixture', encoding='utf-8')
        self.archive = Path(self.temp.name) / 'recovery.zip'

    def test_complete_inventory_and_restore(self):
        report = backup.create_backup(self.root, self.archive, '0.0.1', 'fixture')
        self.assertEqual(report['files'], 3)
        restored = Path(self.temp.name) / 'restored'
        with zipfile.ZipFile(self.archive) as package:
            package.extractall(restored)
        self.assertEqual((restored / 'src/Core/test.cpp').read_text(), 'recovery fixture')
        self.assertTrue((restored / 'thirdparty/fmt/.git/HEAD').exists())
        self.assertFalse((restored / 'build').exists())

    def test_corrupted_payload_is_rejected(self):
        backup.create_backup(self.root, self.archive, '0.0.1', 'fixture')
        tampered = Path(self.temp.name) / 'tampered.zip'
        with zipfile.ZipFile(self.archive) as source, zipfile.ZipFile(tampered, 'w') as target:
            for name in source.namelist():
                target.writestr(name, b'corrupt' if name == 'src/Core/test.cpp' else source.read(name))
        with self.assertRaisesRegex(ValueError, 'content mismatch'):
            backup.verify_backup(tampered)

    def test_source_links_are_rejected(self):
        link = self.root / 'src/Core/alias.cpp'
        try:
            link.symlink_to(self.root / 'src/Core/test.cpp')
        except OSError as error:
            self.skipTest(f'Host does not permit symbolic links: {error}')
        with self.assertRaisesRegex(RuntimeError, 'refuses links'):
            backup.create_backup(self.root, self.archive, '0.0.1', 'fixture')

    def test_path_escape_is_rejected(self):
        with zipfile.ZipFile(self.archive, 'w') as target:
            target.writestr('../escape.txt', 'unsafe')
        with self.assertRaisesRegex(ValueError, 'Unsafe backup path'):
            backup.verify_backup(self.archive)


if __name__ == '__main__':
    unittest.main()
