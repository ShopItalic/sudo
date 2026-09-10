"""Check that baseline preparation cannot silently select a different build."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "baseline", ROOT / "tools/firmware/prepare_vendor_baseline.py")
baseline = importlib.util.module_from_spec(spec)
spec.loader.exec_module(baseline)


class BaselineTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.original = subprocess.check_output(
            ["git", "show", f"{baseline.BASELINE}:{baseline.PROJECT}"], cwd=ROOT)

    def test_only_packaging_hook_changes_in_original_target(self):
        generated, changes = baseline.build_only_project(self.original)
        original = next(t for t in ET.fromstring(self.original).find("Targets")
                        if t.findtext("TargetName") == "1.23.2")
        targets = ET.fromstring(generated).find("Targets")
        self.assertEqual(len(targets), 1)
        selected = targets[0]
        self.assertEqual(changes, [{"phase": "AfterMake", "hook": 1,
                                   "command": "..\\..\\..\\dfu\\ota_bat\\creat_1232_dfu.bat"}])
        self.assertEqual(selected.findtext("TargetName"), "1.23.2")
        self.assertTrue(all(e.text == "0" for e in selected.iter()
                            if e.tag.startswith("RunUserProg")))
        selected.find("TargetOption/TargetCommonOption/AfterMake/RunUserProg1").text = "1"
        self.assertEqual(ET.tostring(selected), ET.tostring(original))

    def test_wrong_compiler_is_rejected(self):
        changed = self.original.replace(b"5060960::V5.06", b"5060750::V5.06")
        with self.assertRaises(ValueError):
            baseline.build_only_project(changed)

    def test_similar_board_name_is_not_accepted(self):
        changed = self.original.replace(b"<TargetName>1.23.2</TargetName>",
                                        b"<TargetName>1.23.2_one_sec</TargetName>")
        with self.assertRaises(ValueError):
            baseline.build_only_project(changed)

    def test_output_cannot_overwrite_repository_or_existing_state(self):
        for destination in (ROOT, ROOT / "build", ROOT / "tools"):
            with self.subTest(destination=destination), self.assertRaises(ValueError):
                baseline.prepare(destination)
        (ROOT / "build").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
            sentinel = Path(directory) / "existing-build.txt"
            sentinel.write_text("preserve")
            with self.assertRaises(ValueError):
                baseline.prepare(Path(directory))
            self.assertEqual(sentinel.read_text(), "preserve")


if __name__ == "__main__":
    unittest.main()
