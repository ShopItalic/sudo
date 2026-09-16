"""Verify keep-all capture and staged P11 scope against each generated release."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/firmware"))
import prepare_factory_ptt_v10_r3 as p10
import prepare_factory_ptt_v11 as p11
from factory_local_recording_v8 import function_span


class KeepShortRecordings(unittest.TestCase):
    def test_each_generated_release(self):
        with tempfile.TemporaryDirectory(prefix="keep-short-", dir=ROOT / "build") as folder:
            base = Path(folder)
            for name, recipe, revision in (("p10-r3", p10, None), ("p11-r1", p11, 1), ("p11-r2", p11, 2)):
                with self.subTest(release=name):
                    source = base / name; parent, evidence = recipe.prepare_base(source)
                    app = source / "firmware/bc_ros/bc_application"
                    preserved = ["app_factory_cleanup.c", "app_factory_delete.c", "app_factory_ptt.c"]
                    before = {name: (app / name).read_bytes() for name in preserved}
                    pdm = (app / "app_pdm_handler.c").read_bytes()
                    result = recipe.apply_overlay(source, parent, evidence, **({"revision": revision} if revision else {}))
                    self.assertFalse(result["firmwareShortDeletion"])
                    for f, data in before.items(): self.assertEqual((app / f).read_bytes(), data, f)
                    if revision != 2: self.assertEqual((app / "app_pdm_handler.c").read_bytes(), pdm)
                    if revision == 1:
                        self.assertEqual(result["opusUnits"], 0)
                        self.assertEqual(result["mode"], "charging-lights")
                    source_file = (app / "app_ppg_file_data_handler.c").read_text(encoding="latin1")
                    functions = []
                    for signature in ("bool app_factory_capture_bind_file(void)", "bool app_factory_capture_finish_file(void)"):
                        start, end = function_span(source_file, signature); functions.append(source_file[start:end])
                    finish = "\n".join(functions)
                    self.assertNotIn("factory_delete_", finish)
                    self.assertNotIn("lfs_remove", finish)
                    self.assertNotIn("factory_capture_path", source_file)
                    (app / "retained_finish.inc").write_text(finish)
                    binary = base / (name + "-test")
                    subprocess.run([os.environ.get("CC", "clang"), "-std=c99", "-Wall", "-Wextra", "-Werror", "-O1", "-g",
                        "-fsanitize=address,undefined", "-I" + str(app),
                        "-I" + str(ROOT / "tests/firmware/factory_controls"),
                        "-I" + str(ROOT / "tests/firmware/factory_ptt_v2"),
                        str(ROOT / "tests/firmware/test_factory_keep_short_recordings.c"), "-o", str(binary)], check=True)
                    subprocess.run([str(binary)], check=True)


if __name__ == "__main__": unittest.main()
