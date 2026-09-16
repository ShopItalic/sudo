"""Execute generated P11 LED/storage code from an authenticated parent."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/firmware"))
import prepare_factory_ptt_v11 as recipe
from factory_local_recording_v8 import function_span


def function(source, signature):
    a, b = function_span(source, signature)
    return source[a:b]


class P11OverlayTests(unittest.TestCase):
    revision = 2
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="p11-overlay-test-", dir=ROOT / "build")
        cls.directory = Path(cls.temp.name)
        cls.source = cls.directory / "candidate"
        parent, evidence = recipe.prepare_base(cls.source)
        cls.preparation = recipe.apply_overlay(cls.source, parent, evidence, revision=cls.revision)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def compile_run(self, test, includes):
        binary = self.directory / test
        options = [os.environ.get("CC", "clang"), "-std=c99", "-Wall", "-Wextra", "-Werror", "-O1", "-g",
                   "-Wno-unused-parameter", "-Wno-unused-function", "-Wno-invalid-utf8", "-fsanitize=address,undefined",
                   "-fno-sanitize-recover=all", "-I" + str(self.directory), "-I" + str(recipe.OVERLAY)]
        subprocess.run(options + [str(ROOT / "tests/firmware" / (test + ".c"))] + includes + ["-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)

    def test_actual_led_task_and_renderer(self):
        source = (self.source / recipe.charging.LED).read_text(encoding="latin1")
        definitions = source[source.index("enum bc_ic_led_flag"):source.index("static bc_rtos_event_struct")]
        signatures = ["static void bc_ic_led_color_value_get(uint8_t color, uint8_t *g, uint8_t *r, uint8_t *b)",
            "static void bc_ic_led_rgb_set(uint8_t rgb_g,uint8_t rgb_r,uint8_t rgb_b,uint8_t num)",
            "static void bc_ic_led_rgb_clear(void)", "static void bc_ic_led_handler_thread(void *thread_handler)",
            "void bc_ic_led_test_cmd(uint8_t g,uint8_t r,uint8_t b)", "void bc_ic_led_stop(void)", "void bc_id_led_clear(void)"]
        signatures += ["void bc_ic_led_mic_" + mode + "_" + state + "(void)"
                       for mode in ("offline_recording", "online_recording", "offline_recording_capture", "online_recording_capture")
                       for state in ("on", "off")]
        functions = [function(source, signature) for signature in signatures]
        (self.directory / "led.inc").write_text(definitions + "\n" + "\n".join(functions[:3]) +
            recipe.charging.HELPERS + "\n" + "\n".join(functions[3:]), encoding="latin1")
        self.compile_run("test_factory_led_p11", [])
        pmic = (self.source / recipe.charging.PMIC).read_text(encoding="latin1")
        self.assertIn("pmic_state == PMIC_CHARGED_ING ? P11_CHARGE_AMBER", pmic)
        self.assertIn("pmic_state == PMIC_CHARGED_OVER ? P11_CHARGE_FULL : P11_CHARGE_OFF", pmic)
        self.assertNotIn("timer_create", recipe.charging.HELPERS)
        self.assertNotIn("pvPortMalloc", recipe.charging.HELPERS)

    def test_actual_append_and_atomic_rollover(self):
        source = (self.source / recipe.audio.APP / "app_ppg_file_data_handler.c").read_text(encoding="latin1")
        signatures = ["static bool p10_claim_status(unsigned status,bool cleanup)",
            "bool p11_file_write(const uint8_t *data, unsigned length)", "bool p11_file_rollover(void)"]
        claims = "\n".join(line for line in source.splitlines()
                           if line.startswith(("static bool p10_claim_idle(void)", "static bool p10_claim_cleanup(void)", "static bool p10_claim_write(void)")))
        (self.directory / "storage.inc").write_text(function(source, signatures[0]) + "\n" + claims + "\n" +
            "\n".join(function(source, signature) for signature in signatures[1:]), encoding="latin1")
        self.compile_run("test_factory_storage_p11", [])


class P11ChargingOnlyTests(P11OverlayTests):
    revision = 1

    def test_actual_append_and_atomic_rollover(self):
        self.assertEqual(self.preparation["mode"], "charging-lights")
        self.assertEqual(self.preparation["opusUnits"], 0)
        self.assertFalse((self.source / recipe.audio.APP / "app_factory_audio.c").exists())
        source = (self.source / recipe.audio.APP / "app_pdm_handler.c").read_text(encoding="latin1")
        self.assertNotIn("p11_capture_", source)
        self.assertNotIn("P11_ADPCM_CONTROL", (self.source / self.preparation["project"]).read_text())


if __name__ == "__main__":
    unittest.main()
