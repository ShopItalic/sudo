"""Test retirement against both authenticated P10-r3 and P11 generated trees."""
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
import factory_disable_swipes as retirement


class SwipeRetirement(unittest.TestCase):
    def test_both_generated_releases_and_preserved_contract(self):
        with tempfile.TemporaryDirectory(prefix="swipe-retirement-", dir=ROOT / "build") as folder:
            temp = Path(folder)
            fixture = (ROOT / "tests/firmware/test_factory_scroll_settings_v9.c").read_text().split("int main(void)", 1)[0]
            (temp / "controls-test.c").write_text(fixture + (ROOT / "tests/firmware/test_factory_disable_swipes.c").read_text())
            for name, recipe, revision in (("p10-r3", p10, None), ("p11-r1", p11, 1), ("p11-r2", p11, 2)):
                with self.subTest(release=name):
                    source = temp / name
                    parent, evidence = recipe.prepare_base(source)
                    controls = (source / retirement.CONTROLS).read_bytes()
                    unchanged = [
                        "firmware/bc_ros/bc_application/app_factory_scroll.c",
                        "firmware/bc_ros/bc_application/app_factory_ptt.c",
                        "firmware/bc_ros/bc_module/ble/src/bc_ble.c",
                        "firmware/bc_ros/bc_device/touch_button/IQS7211E/IQS7211E_init_1232.h",
                    ]
                    saved = {f: (source / f).read_bytes() for f in unchanged}
                    result = recipe.apply_overlay(source, parent, evidence, **({"revision": revision} if revision else {}))
                    self.assertTrue(result["swipesDisabled"])
                    self.assertNotEqual((source / retirement.CONTROLS).read_bytes(), controls)
                    for f, content in saved.items(): self.assertEqual((source / f).read_bytes(), content, f)
                    binary = temp / (name + "-test")
                    app = source / retirement.CONTROLS
                    subprocess.run([os.environ.get("CC", "clang"), "-std=c99", "-Wall", "-Wextra", "-Werror", "-O1", "-g",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-I" + str(app.parent),
                        "-I" + str(ROOT / "tests/firmware/factory_controls"),
                        str(temp / "controls-test.c"), "-o", str(binary)], check=True)
                    subprocess.run([str(binary)], check=True)


if __name__ == "__main__": unittest.main()
