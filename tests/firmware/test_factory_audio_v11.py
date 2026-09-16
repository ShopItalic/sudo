"""Rebuild and execute the exact bounded P11 codec with ASan/UBSan."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/firmware"))
import prepare_factory_ptt_v11 as recipe


class P11CodecTests(unittest.TestCase):
    def test_real_fixed_point_codec(self):
        with tempfile.TemporaryDirectory(prefix="p11-codec-") as tmp:
            directory = Path(tmp)
            recipe.prepare_codec(directory)
            compiler = os.environ.get("CC", "clang")
            opus = directory / recipe.OPUS
            recording = directory / recipe.RECORDING
            options = [compiler, "-std=gnu99", "-O2", "-g", "-fsanitize=address,undefined",
                       "-fno-sanitize=shift-base,shift-exponent,signed-integer-overflow"]
            options += ["-D" + define for define in recipe.CODEC_DEFINES.split()]
            options += ["-DP11_AUDIO_TEST"]
            options += ["-I" + str(opus / name) for name in ("include", "celt", "silk", "silk/fixed", "src")]
            options += ["-I" + str(recording), "-I" + str(recording / "opus_support"), "-I" + str(recipe.OVERLAY)]
            def compile_one(relative):
                output = directory / (relative.replace("/", "_") + ".o")
                subprocess.run(options + ["-c", str(directory / relative), "-o", str(output)], check=True, capture_output=True)
                return output
            with ThreadPoolExecutor(max_workers=4) as executor:
                objects = list(executor.map(compile_one, recipe.opus_units(directory)))
            sources = [recording / name for name in ("bc_resampler.c", "bc_opus_encoder.c")]
            sources += [recipe.OVERLAY / "app_factory_audio.c", ROOT / "tests/firmware/test_factory_audio_v11.c"]
            binary = directory / "test-audio"
            subprocess.run(options + [str(p) for p in sources + objects] + ["-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
            short = directory / "app_factory_short.c"
            short.write_text(recipe.keep_short.patch_capture(recipe.audio.patch_short(
                recipe.committed("firmware/factory_ptt_v9/app_factory_short.c").decode())))
            (directory / "app_factory_short.h").write_text(recipe.audio.patch_short_header(
                recipe.committed("firmware/factory_ptt_v9/app_factory_short.h").decode()))
            capture_options = options + ["-DHANDWARE_1_23_2", "-I" + str(directory),
                "-I" + str(ROOT / "tests/firmware/factory_p11"), "-I" + str(ROOT / "tests/firmware/sudo_capture")]
            capture_sources = sources[:-1] + [short, recording / "bc_capture.c", ROOT / "tests/firmware/test_factory_capture_p11.c"]
            capture_binary = directory / "test-capture"
            subprocess.run(capture_options + [str(p) for p in capture_sources + objects] + ["-o", str(capture_binary)], check=True)
            subprocess.run([str(capture_binary)], check=True)
            (directory / "adpcm_a.h").write_bytes(recipe.committed("firmware/bc_ros/bc_algorithm/adpcm_a.h"))
            header = directory / "app_factory_short.h"
            header.write_text(header.read_text().replace("FACTORY_SHORT_SAMPLES_PER_STEP 8000U",
                                                        "FACTORY_SHORT_SAMPLES_PER_STEP 4000U"))
            control_sources = [recipe.OVERLAY / "app_factory_audio_control.c", short,
                               recording / "bc_capture.c", ROOT / "tests/firmware/test_factory_capture_p11.c"]
            control_binary = directory / "test-adpcm-control"
            subprocess.run(capture_options + ["-DP11_ADPCM_CONTROL"] + [str(p) for p in control_sources] +
                           ["-o", str(control_binary)], check=True)
            subprocess.run([str(control_binary)], check=True)


if __name__ == "__main__":
    unittest.main()
