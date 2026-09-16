"""Isolated P10-r3/P11 retirement proof; never mutate existing build snapshots."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/firmware"))
import prepare_factory_ptt_v11 as recipe
import factory_disable_speed_test_p11 as retirement
import factory_retire_diagnostics as all_retirement
import prepare_factory_ptt_v10_r3 as p10
from factory_local_recording_v8 import function_span


class SpeedTestRetirement(unittest.TestCase):
    def test_all_release_modes(self):
        with tempfile.TemporaryDirectory(prefix="p11-speed-retirement-", dir=ROOT / "build") as directory:
            root = Path(directory)
            for revision, control in ((0, False), (1, False), (2, False), (2, True)):
                with self.subTest(revision=revision, adpcm_control=control):
                    source = root / (str(revision) + ("-control" if control else ""))
                    parent, evidence = recipe.prepare_base(source)
                    retired_vendor = (source / retirement.APP / retirement.UNIT).read_bytes()
                    sensor_vendor = (source / retirement.APP / "app_six_axis_sensor_handler.c").read_bytes()
                    # These core paths are verified by the shared overlay suites;
                    # retirement must preserve the immutable vendor files too.
                    result = p10.apply_overlay(source, parent, evidence) if revision == 0 else recipe.apply_overlay(source, parent, evidence, control, revision)
                    self.assertTrue(result["speedTestDisabled"] and result["motionDisabled"])
                    changes = result["changes"]
                    project = source / result["project"]
                    tree = ET.parse(project)
                    names = [entry.findtext("FileName") for entry in tree.findall(".//File")]
                    self.assertNotIn(retirement.UNIT, names)
                    self.assertNotIn("app_six_axis_sensor_handler.c", names)
                    self.assertEqual(names.count("app_factory_motion_off.c"), 1)
                    with self.assertRaises(ValueError): retirement.patch_project(tree)
                    self.assertEqual((source / retirement.APP / retirement.UNIT).read_bytes(), retired_vendor)
                    self.assertEqual((source / retirement.APP / "app_six_axis_sensor_handler.c").read_bytes(), sensor_vendor)
                    app = (source / retirement.APP / "app.c").read_text(encoding="latin1")
                    commands = (source / retirement.APP / "app_cmd_handler.c").read_text(encoding="latin1")
                    package = (source / retirement.APP / "app_package.c").read_text(encoding="latin1")
                    self.assertNotIn("app_ble_speed_time_create", app)
                    self.assertNotIn("app_g_sensor_time_create", app)
                    self.assertNotIn("app_six_axis_sensor_time_create", app)
                    self.assertIn("app_factory_motion_off();", app)
                    for name in ("app_ble_speed_test_start", "app_ble_speed_test_stop", "app_wifi_speed_test_start", "app_wifi_speed_test_stop"):
                        self.assertNotIn(name, commands)
                    self.assertNotIn("app_package_speed_test_up", package)
                    self.assertIn("app_test_ble_speed_callback,", commands)  # rejection remains routed
                    self.assertIn("case 0x00: /* P11: retired TCP traffic generator */\n      app_test_ble_speed_callback(cmd_package);", commands)
                    for relative in retirement.PATCHES:
                        entry = next(c for c in changes if c["path"] == relative)
                        self.assertEqual(entry["sha256"], hashlib.sha256((source / relative).read_bytes()).hexdigest())
                    self.assertEqual(len(changes), len({c["path"] for c in changes}))
                    start, end = function_span(commands, retirement.REJECT_SIGNATURE)
                    (root / "speed-reject.inc").write_text(commands[start:end])
                    binary = root / "reject-test"
                    subprocess.run([os.environ.get("CC", "clang"), "-std=c99", "-Wall", "-Wextra", "-Werror", "-O1", "-g",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-I" + str(root),
                        str(ROOT / "tests/firmware/test_factory_disable_speed_test_p11.c"), "-o", str(binary)], check=True)
                    subprocess.run([str(binary)], check=True)
                    sensor = source / "firmware/bc_ros/bc_device/lsm6dsow"
                    app_dir = source / retirement.APP
                    motion_binary = root / "motion-test"
                    subprocess.run([os.environ.get("CC", "clang"), "-std=c99", "-Wall", "-Wextra", "-Werror", "-O1", "-g",
                        "-U__weak", "-D__weak=__attribute__((weak))", "-Wno-unused-parameter", "-Wno-invalid-utf8", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                        "-I" + str(ROOT / "tests/firmware/factory_retirement"), "-I" + str(app_dir), "-I" + str(sensor),
                        str(ROOT / "tests/firmware/test_factory_motion_off.c"), str(app_dir / "app_factory_motion_off.c"),
                        str(sensor / "lsm6dso_reg.c"), "-o", str(motion_binary)], check=True)
                    subprocess.run([str(motion_binary)], check=True)

    def test_linked_gate_rejects_remaining_symbols(self):
        class Artifact:
            symbols = {}
        self.assertTrue(retirement.verify_retirement(Artifact())["speedTestDisabled"])
        for name in retirement.verify_retirement(Artifact())["retiredSymbolsAbsent"]:
            artifact = Artifact()
            artifact.symbols = {name: [(0x27000, 4)]}
            with self.assertRaises(ValueError): retirement.verify_retirement(artifact)

    def test_motion_linked_gate_and_startup_stack_fail_closed(self):
        class Artifact:
            symbols = {"app_factory_motion_off": [(0x27000, 4)]}
            stack_size = 8192
        proof = all_retirement.verify(Artifact())
        for name in proof["motionSymbolsAbsent"]:
            artifact = Artifact()
            artifact.symbols = {**artifact.symbols, name: [(0x27004, 4)]}
            with self.assertRaises(ValueError): all_retirement.verify(artifact)
        artifact = Artifact()
        artifact.symbols = {}
        with self.assertRaises(ValueError): all_retirement.verify(artifact)
        chains = {"main": 400, "app_factory_motion_off": 300,
                  "motion_read": 40, "motion_write": 40,
                  "bsp_i2c_read": 100, "nrf_gpio_pin_read": 8}
        report = "".join("<P><STRONG><a></a>" + name + "</STRONG> Max Depth = " + str(size)
                         for name, size in chains.items())
        self.assertEqual(all_retirement.startup_stack_budget(Artifact(), report)["remainingBytes"], 7388)
        artifact.stack_size = 512
        with self.assertRaises(ValueError): all_retirement.startup_stack_budget(artifact, report)
        with self.assertRaises(ValueError): all_retirement.startup_stack_budget(Artifact(), "")


if __name__ == "__main__": unittest.main()
