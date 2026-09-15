"""P08 production delete boundary, real LittleFS faults and inherited isolation."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/firmware'))
import prepare_factory_ptt_v7 as prior
import prepare_factory_ptt_v8 as recipe
from factory_local_recording_v8 import function_span


def vendor(path):
    return subprocess.check_output(['git', 'show', '102bfd2:' + path], cwd=ROOT).decode('latin1').replace('\r\n', '\n')


def adapter():
    text = recipe.patch_delete_file(vendor(recipe.APP + 'app_ppg_file_data_handler.c'))
    return '/* P08: explicit commands only;' + text.split('/* P08: explicit commands only;')[1].split(
        '\nvoid app_ppg_list_capture_audio_up_check')[0]


class FactoryDeleteV8(unittest.TestCase):
    def test_scope(self):
        for path, patch in prior.PATCHES.items():
            if path == prior.base.CONFIG:
                self.assertEqual(recipe.PATCHES[path](vendor(path)).replace('P08', 'P07'), patch(vendor(path)))
            elif path == recipe.APP + 'app_linear_motor_handler.c':
                # The only P08 cue delta is the stop/loop flag correction;
                # the battery integration suite exercises its lifecycle.
                self.assertEqual(recipe.PATCHES[path](vendor(path)),
                                 recipe.battery.patch_vibrate_flags(patch(vendor(path))))
            else:
                self.assertEqual(recipe.PATCHES[path](vendor(path)), patch(vendor(path)), path)
        for path in prior.OVERLAY.glob('*.[ch]'):
            if path.name == 'app_factory_motor.h':
                # P08 battery adds a task-safe activity snapshot. Its actual
                # motor lifecycle and snapshot are exercised by the battery suite.
                continue
            if path.name == 'app_factory_ptt.c':
                # Only the command handoff changes: retired stream controls
                # cannot interrupt capture. The local-recording suite executes
                # that function; all gesture/recorder ownership stays identical.
                def without_command_handoff(text):
                    a, b = function_span(text,
                        'void app_factory_ptt_before_command(const uint8_t *d, unsigned n)')
                    return text[:a] + text[b:]
                self.assertEqual(without_command_handoff(path.read_text()),
                    without_command_handoff((recipe.OVERLAY / path.name).read_text()))
            else:
                self.assertEqual(path.read_bytes(), (recipe.OVERLAY / path.name).read_bytes())
        self.assertEqual(set(recipe.PATCHES) - set(prior.PATCHES), {
            recipe.APP + 'app_package.c',
            recipe.APP + 'app_cmd_handler.c', recipe.APP + 'app_ppg_file_data_handler.c',
            recipe.APP + 'app_pdm_handler.c',
            'firmware/bc_ros/bc_module/pmic/bc_power.c',
            'firmware/bc_ros/bc_application/app_pmic_handler.c',
            'firmware/bc_ros/bc_driver/bsp/src/bsp_adc.c'})
        # No new transfer-triggered, timed or persistent deletion path.
        original = vendor(recipe.APP + 'app_ppg_file_data_handler.c')
        patched = recipe.patch_delete_file(original)
        for signature in ('static void ppg_file_data_upload_handler_thread(void * p_context)',
                          'static void ppg_file_resume_upload_handler_thread(void * p_context)',
                          'static void ppg_file_one_click_upload_handler_thread(void * p_context)'):
            def body(text):
                start = text.index(signature + '\n{')
                return text[start:text.index('\n}', start) + 2]
            self.assertEqual(body(original), body(patched))

    def test_production_delete(self):
        with tempfile.TemporaryDirectory(prefix='factory-delete-p08-') as folder:
            out = Path(folder)
            lfs = 'firmware/bc_ros/bc_module/file/LittleFS/'
            for name in ('lfs.h', 'lfs.c', 'lfs_util.h', 'lfs_util.c'):
                (out / name).write_text(vendor(lfs + name), encoding='latin1')
            (out / 'factory_delete_adapter.inc').write_text(adapter(), encoding='latin1')
            command_source = recipe.patch_delete_command(vendor(recipe.APP + 'app_cmd_handler.c'))
            arm = command_source.split('\t\tcase 0x12:')[1].split('\t\tcase 0x13:')[0]
            (out / 'factory_delete_command.inc').write_text(
                'static void command_delete(struct app_cmd_package *cmd_package) { switch(cmd_package->subcmd) { case 0x12:' + arm + '} }\n', encoding='latin1')
            prefix = command_source.split('void app_cmd_package_parse(uint8_t *cmd_pack,uint16_t pack_length)\n{')[1].split(
                '\tswitch(cmd_package->cmd)')[0]
            with (out / 'factory_delete_command.inc').open('a', encoding='latin1') as stream:
                stream.write('static unsigned parser_dispatches;\nstatic void parse_fixture(uint8_t *cmd_pack,uint16_t pack_length) { ' +
                             prefix + '\n++parser_dispatches; command_delete(cmd_package); }\n')
            cc = [os.environ.get('CC', 'cc'), '-std=c99', '-Wall', '-Wextra', '-Werror',
                  '-Wno-invalid-utf8', '-Wno-unused-function', '-O1', '-g', '-fsanitize=address,undefined',
                  '-DLFS_NO_DEBUG', '-DLFS_NO_WARN', '-DLFS_NO_ERROR',
                  '-I' + str(out), '-I' + str(recipe.OVERLAY)]
            for mode in ('mock', 'littlefs'):
                sources = [str(recipe.OVERLAY / 'app_factory_delete.c'),
                           str(ROOT / 'tests/firmware' / ('test_factory_delete_' + mode + '_v8.c'))]
                if mode == 'littlefs':
                    sources += [str(out / 'lfs.c'), str(out / 'lfs_util.c')]
                binary = out / mode
                subprocess.run(cc + sources + ['-o', str(binary)], cwd=ROOT, check=True)
                subprocess.run([str(binary)], check=True)
            # Meaningful negative controls: each critical protection must be
            # detected by the fault/malformed/reentrancy harness.
            original = (recipe.OVERLAY / 'app_factory_delete.c').read_text()
            for old, new in (
                ('length > 250U', 'length > 500U'),
                ("data[0] == '.'", 'false'),
                ('if (data[i] != 0U) return false;', 'if (false) return false;'),
                ('error == LFS_ERR_NOENT) return FACTORY_DELETE_ALREADY_ABSENT',
                 'error == LFS_ERR_NOENT) return FACTORY_DELETE_STORAGE_ERROR'),
                ('if (workspace->info.type != LFS_TYPE_REG)', 'if (false)'),
                ('return error == LFS_ERR_OK ? FACTORY_DELETE_NOT_CONFIRMED :',
                 'return error == LFS_ERR_OK ? FACTORY_DELETE_REMOVED :'),
            ):
                self.assertIn(old, original)
                mutant = out / 'mutant.c'
                mutant.write_text(original.replace(old, new))
                binary = out / 'negative'
                subprocess.run(cc + [str(mutant), str(ROOT / 'tests/firmware/test_factory_delete_mock_v8.c'),
                                     '-o', str(binary)], check=True)
                result = subprocess.run([str(binary)], capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0, old)
                self.assertTrue('FAIL' in result.stderr or 'AddressSanitizer' in result.stderr,
                                (old, result.stderr))
            print('PASS 6 deletion negative controls')


if __name__ == '__main__':
    unittest.main()
