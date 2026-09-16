"""Exercise catalog revision identity and exact P11 package URL binding."""
import copy
import json
from pathlib import Path
import tempfile
import unittest
from test_release_manifest import check_manifest


def release(revision):
    version = "6.0.3.3P11"
    url = f"https://github.com/ShopItalic/sudo/releases/download/v{version}-r{revision}/BCL603S2P_{version}.zip"
    return {"schemaVersion": 1, "hardware": "603V1.23.2", "version": version,
            "packageRevision": revision, "status": "supplier-signed-ota", "otaAvailable": True,
            "toolchain": "Arm Compiler 5.06u7", "otaPackageURL": url,
            "otaPackage": {"url": url, "bytes": 200, "sha256": "a" * 64}}


class RevisionCatalogTests(unittest.TestCase):
    def check(self, releases, schema=2):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "catalog.json"
            path.write_text(json.dumps({"schemaVersion": schema, "releases": releases}))
            return check_manifest(path)

    def test_four_revisions_and_duplicate_rejection(self):
        entries = [release(i) for i in range(1, 5)]
        self.assertEqual(self.check(entries).count("ota-downloadable"), 4)
        with self.assertRaisesRegex(SystemExit, "duplicate"):
            self.check(entries, schema=1)
        with self.assertRaisesRegex(SystemExit, "duplicate"):
            self.check(entries + [entries[0]])
        legacy = copy.deepcopy(entries[0])
        legacy.update(version="6.0.3.3P10", otaAvailable=False, otaPackageURL=None, otaPackage=None)
        del legacy["packageRevision"]
        explicit = {**legacy, "packageRevision": 1}
        with self.assertRaisesRegex(SystemExit, "duplicate"):
            self.check([legacy, explicit])

    def test_unknown_or_mismatched_revisions_fail_closed(self):
        for revision in [None, 0, 5, True, "3"]:
            with self.subTest(revision=revision), self.assertRaises(SystemExit):
                self.check([{**release(1), "packageRevision": revision}])
        with self.assertRaisesRegex(SystemExit, "release tag"):
            self.check([{**release(2), "packageRevision": 4}])


if __name__ == "__main__":
    unittest.main()
