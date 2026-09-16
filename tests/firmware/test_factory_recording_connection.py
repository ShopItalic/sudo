"""Exercise actual generated BLE callbacks and PTT through connection changes."""
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/firmware"))
import factory_recording_connection as fix
import prepare_factory_ptt_v10_r4 as p10
import prepare_factory_ptt_v11_r3 as p11_adpcm
import prepare_factory_ptt_v11_r4 as p11_opus

APP = "firmware/bc_ros/bc_application/"


def function(source, signature):
    start = source.index(signature + "\n{")
    # Vendor functions sometimes indent their final brace. Ignore comments
    # and quoted literals when matching nested braces, preserving positions.
    masked = re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                    lambda m: " " * len(m.group()), source, flags=re.S)
    depth = 0
    for i in range(masked.index("{", start), len(masked)):
        if masked[i] == "{": depth += 1
        elif masked[i] == "}":
            depth -= 1
            if depth == 0: return source[start:i + 1]
    raise ValueError("Unbalanced function: " + signature)


def text(path):
    return path.read_bytes().decode("latin1").replace("\r\n", "\n")


class RecordingConnectionTests(unittest.TestCase):
    def run_transitions(self, source, directory):
        directory.mkdir()
        for name in ("app_factory_ptt.c", "app_factory_ptt.h"):
            shutil.copyfile(source / APP / name, directory / name)
        for name in ("app_factory_short.h", "bc_rtos.h", "app_pdm_handler.h",
                     "app_factory_controls.h", "app_cmd_handler.h", "app_package.h"):
            (directory / name).write_text("/* Host declarations supplied by fixture.h. */\n")
        pieces = [
            ("app_package.c", "void app_package_mic_recording_stop_isr(void)"),
            ("app_package.c", "void app_package_pdm_key_flag_clear(void)"),
            ("app_touch_button_handler.c", "void app_touch_pdm_key_flag_clear(void)"),
            ("app_ble_handler.c", "static void app_ble_connect_callback(void)"),
            ("app_ble_handler.c", "static void app_ble_disconnect_callback(void)"),
        ]
        callbacks = "\n\n".join(function(text(source / APP / path), signature)
                                for path, signature in pieces)
        (directory / "callbacks.inc").write_text(callbacks, encoding="latin1")
        parser = function(text(source / APP / "app_cmd_handler.c"),
                          "static uint8_t app_cmd_pdm(struct app_cmd_package * cmd_package)")
        first = parser.index("    case 0xF9:")
        end = parser.index("\n\t}", first)
        (directory / "command-cases.inc").write_text(parser[first:end], encoding="latin1")
        binary = directory / "test"
        fixture = ROOT / "tests/firmware/factory_recording_connection"
        command = [os.environ.get("CC", "clang"), "-std=c99", "-Wall", "-Wextra", "-Werror",
                   "-Wno-unused-variable", "-Wno-unused-function", "-Wno-invalid-utf8", "-O1", "-g",
                   "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                   "-include", str(fixture / "fixture.h"), "-I" + str(fixture), "-I" + str(directory),
                   str(directory / "app_factory_ptt.c"),
                   str(ROOT / "tests/firmware/test_factory_recording_connection.c"), "-o", str(binary)]
        subprocess.run(command, check=True)
        return subprocess.run([str(binary)], capture_output=True, text=True)

    def test_generated_candidates_and_released_negative_control(self):
        with tempfile.TemporaryDirectory(prefix="recording-connection-", dir=ROOT / "build") as folder:
            temp = Path(folder)
            cases = (("p10-r4", p10, {}, 4, None),
                     ("p11-r3", p11_adpcm, {}, 3, "charging-lights"),
                     ("p11-r4", p11_opus, {}, 4, "opus"),
                     ("p11-r4-adpcm-control", p11_opus, {"adpcm_control": True}, 4, "adpcm-control-not-for-release"))
            for name, recipe, options, revision, mode in cases:
                with self.subTest(candidate=name):
                    source = temp / name
                    parent, evidence = recipe.prepare_base(source)
                    prior_ble = text(source / fix.BLE)
                    prior_ptt = (source / APP / "app_factory_ptt.c").read_bytes()
                    if name == "p10-r4":
                        negative = self.run_transitions(source, temp / "released-parent-test")
                        self.assertNotEqual(negative.returncode, 0, "Released reconnect-stop bug was not detected")
                        self.assertIn("blue_connect == 1 && stops == 0 && working", negative.stderr)
                        print("PASS negative control: released P10-r2 fails reconnect-preserves-recording assertion")
                    result = recipe.apply_overlay(source, parent, evidence, **options)
                    self.assertEqual(result["packageRevision"], revision)
                    if mode is not None:
                        self.assertEqual(result["mode"], mode)
                        self.assertEqual(result["opusUnits"] > 0, mode == "opus")
                        self.assertTrue(result["chargingLights"])
                    self.assertTrue(result["reconnectStopRemoved"])
                    current_ble = text(source / fix.BLE)
                    self.assertNotIn(fix.STOP_ON_CONNECT, function(current_ble, fix.CONNECT))
                    self.assertEqual(function(current_ble, "static void app_ble_disconnect_callback(void)"),
                                     function(prior_ble, "static void app_ble_disconnect_callback(void)"))
                    self.assertEqual((source / APP / "app_factory_ptt.c").read_bytes(), prior_ptt)
                    entries = [c for c in result["changes"] if c["path"] == fix.BLE]
                    self.assertEqual(len(entries), 1)
                    self.assertEqual(entries[0]["sha256"], hashlib.sha256((source / fix.BLE).read_bytes()).hexdigest())
                    runtime = self.run_transitions(source, temp / (name + "-test"))
                    self.assertEqual(runtime.returncode, 0, runtime.stdout + runtime.stderr)
                    print(name + ": " + runtime.stdout.strip())

    def test_patch_requires_exact_connection_stop(self):
        source = fix.CONNECT + "\n{\n" + fix.STOP_ON_CONNECT + "\n}\n"
        for invalid in (source.replace(fix.STOP_ON_CONNECT, ""),
                        source.replace(fix.STOP_ON_CONNECT, fix.STOP_ON_CONNECT + "\n" + fix.STOP_ON_CONNECT)):
            with self.assertRaisesRegex(ValueError, "Expected one"):
                fix.patch_ble(invalid)


if __name__ == "__main__": unittest.main()
