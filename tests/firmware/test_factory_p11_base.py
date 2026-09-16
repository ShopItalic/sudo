"""P11 must not silently inherit a different P10 revision or dirty recipe."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("p11_base", ROOT / "tools/firmware/factory_p11_base.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


class ParentLockTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="p11-parent-test-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / "firmware/audio.c"
        self.source.parent.mkdir()
        self.source.write_bytes(b"revision 2\n")
        self.project = self.root / "firmware/project.uvprojx"
        self.project.write_bytes(b"compiler 5.06u7\n")
        vendor_project = self.root / "firmware/vendor.uvprojx"
        vendor_project.write_bytes(b"vendor project\n")
        self.unchanged = self.root / "firmware/unchanged.c"
        self.unchanged.write_bytes(b"vendor source\n")
        manifest = json.dumps({"files": [{"path": "audio.c", "sha256": base.digest(b"original")},
            {"path": "unchanged.c", "sha256": base.digest(self.unchanged.read_bytes())}]}).encode()
        (self.root / "firmware/source-manifest.json").write_bytes(manifest)
        self.git = patch.object(base.subprocess, "check_output", return_value=manifest).start()
        self.addCleanup(patch.stopall)
        self.lock = json.loads(base.LOCK.read_text())
        self.lock.update(generatedChangedFiles=1, projectSha256=base.digest(self.project.read_bytes()))
        changed = {"firmware/audio.c": base.digest(self.source.read_bytes())}
        self.lock["generatedDeltaSha256"] = base.digest(json.dumps(changed, sort_keys=True, separators=(",", ":")).encode())
        self.result = {"version": self.lock["version"], "sourceCommit": self.lock["vendorCommit"],
            "project": "firmware/project.uvprojx", "changes": [
                {"path": "firmware/audio.c", "afterSha256": changed["firmware/audio.c"]}]}
        (self.root / "baseline-preparation.json").write_text(json.dumps({
            "target": self.lock["target"], "requiredCompiler": self.lock["requiredCompiler"],
            "project": "firmware/vendor.uvprojx", "projectSha256": base.digest(vendor_project.read_bytes())}))

    def verify(self):
        return base.validate_base(self.root, self.result, self.lock)

    def test_exact_parent(self):
        self.assertEqual(self.verify(), self.lock["generatedDeltaSha256"])

    def test_source_drift_rejected(self):
        self.source.write_bytes(b"later revision\n")
        with self.assertRaisesRegex(ValueError, "source mismatch"):
            self.verify()

    def test_declared_later_revision_rejected(self):
        self.source.write_bytes(b"later revision\n")
        self.result["changes"][0]["afterSha256"] = base.digest(self.source.read_bytes())
        with self.assertRaisesRegex(ValueError, "differs from released"):
            self.verify()

    def test_undeclared_change_rejected(self):
        self.unchanged.write_bytes(b"changed\n")
        with self.assertRaisesRegex(ValueError, "Unexpected parent file changes"):
            self.verify()

    def test_extra_source_rejected(self):
        (self.root / "firmware/extra.c").write_bytes(b"extra")
        with self.assertRaisesRegex(ValueError, "Unexpected parent file changes"):
            self.verify()

    def test_missing_source_rejected(self):
        self.unchanged.unlink()
        with self.assertRaisesRegex(ValueError, "Unexpected parent file changes"):
            self.verify()

    def test_project_drift_rejected(self):
        self.project.write_bytes(b"different toolchain")
        with self.assertRaisesRegex(ValueError, "project/compiler"):
            self.verify()

    def test_wrong_version_rejected(self):
        self.result["version"] = "6.0.3.3P09"
        with self.assertRaisesRegex(ValueError, "version/vendor"):
            self.verify()

    def test_wrong_vendor_rejected(self):
        self.result["sourceCommit"] = "0" * 40
        with self.assertRaisesRegex(ValueError, "version/vendor"):
            self.verify()

    def test_wrong_board_rejected(self):
        path = self.root / "baseline-preparation.json"
        metadata = json.loads(path.read_text())
        metadata["target"] = "1.23.4"
        path.write_text(json.dumps(metadata))
        with self.assertRaisesRegex(ValueError, "board/compiler"):
            self.verify()

    def test_wrong_compiler_rejected(self):
        path = self.root / "baseline-preparation.json"
        metadata = json.loads(path.read_text())
        metadata["requiredCompiler"] = "GNU"
        path.write_text(json.dumps(metadata))
        with self.assertRaisesRegex(ValueError, "board/compiler"):
            self.verify()

    def test_symlink_rejected(self):
        (self.root / "firmware/link.c").symlink_to(self.unchanged)
        with self.assertRaisesRegex(ValueError, "symlink"):
            self.verify()

    def test_duplicate_manifest_entry_rejected(self):
        self.result["changes"].append(copy.deepcopy(self.result["changes"][0]))
        with self.assertRaisesRegex(ValueError, "Duplicate"):
            self.verify()

    def test_path_escape_rejected(self):
        for path in ("../audio.c", "/tmp/audio.c", "firmware/../audio.c", "audio.c"):
            with self.subTest(path=path), self.assertRaisesRegex(ValueError, "Invalid"):
                base.firmware_path(path)

    def test_release_identity(self):
        lock = json.loads(base.LOCK.read_text())
        self.assertEqual(lock["release"], "v6.0.3.3P10-r2")
        self.assertEqual(lock["revision"], 2)
        self.assertEqual(lock["sourceCommit"], "e9840428b22502f762d5cc596f7c469a2ef71c1e")
        self.assertEqual(lock["packageSha256"], "030e5597bb03a417abb99e0900f48a5676e09c22c3ea7eb3f0099f1b4c8d0248")


if __name__ == "__main__":
    unittest.main()
