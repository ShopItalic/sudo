"""Public factory package controls for the independent signed-OTA verifier."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/firmware'))
from verify_signed_application_package import FACTORY, verify


class SignedPackageTests(unittest.TestCase):
    def test_factory_and_corruption_controls(self):
        package = FACTORY / 'BCL603S2P_6.0.3.3Z62.zip'
        binary = FACTORY / 'application.bin'
        self.assertTrue(verify(package, binary, '6.0.3.3Z62')['signatureVerified'])
        with zipfile.ZipFile(package) as source:
            original = {n: source.read(n) for n in source.namelist()}
        application = json.loads(original['manifest.json'])['manifest']['application']
        with tempfile.TemporaryDirectory(prefix='dfu-verifier-tests-') as folder:
            out = Path(folder)
            for name in ('signature', 'image', 'extra_component', 'identity'):
                entries = original.copy()
                expected = binary
                version = '6.0.3.3Z62'
                if name == 'signature':
                    data = bytearray(entries[application['dat_file']])
                    data[-1] ^= 1
                    entries[application['dat_file']] = data
                elif name == 'image':
                    data = bytearray(entries[application['bin_file']])
                    data[128] ^= 1
                    entries[application['bin_file']] = data
                    expected = out / 'modified.bin'
                    expected.write_bytes(data)
                elif name == 'extra_component':
                    entries['bootloader.bin'] = b'Unexpected extra image'
                else:
                    version = '6.0.3.3P02'
                changed = out / (name + '.zip')
                with zipfile.ZipFile(changed, 'w') as archive:
                    for entry, data in entries.items():
                        archive.writestr(entry, data)
                with self.subTest(name=name), self.assertRaises(ValueError):
                    verify(changed, expected, version)


if __name__ == '__main__':
    unittest.main()
