"""Exercise P06 with the actual IQS driver, recorder lifecycle and shared file guards."""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/firmware'))
from prepare_factory_ptt_v6 import IQS, APP, QUEUE, OVERLAY, PATCHES, patch_iqs, patch_touch, patch_ble, patch_queue


def vendor(path):
    return subprocess.check_output(['git', 'show', '102bfd2:' + path], cwd=ROOT).decode('latin1').replace('\r\n', '\n')


def function(source, signature):
    start = source.index(signature + '\n')
    brace = source.index('\n{', start)
    # Supplier function closing braces are at column zero. Avoid comment braces.
    end = source.index('\n}', start) + 2
    return signature + source[brace:end]


class FactoryPTTV6(unittest.TestCase):
    def test_generated_integration(self):
        with tempfile.TemporaryDirectory(prefix='factory-ptt-p06-') as folder:
            out = Path(folder)
            for path in OVERLAY.glob('*.[ch]'):
                shutil.copyfile(path, out / path.name)
            (out / 'IQS7211E.c').write_text(patch_iqs(vendor(IQS)), encoding='latin1')
            for name in ('IQS7211E.h', 'IQS7211E_init_1232.h'):
                (out / name).write_text(vendor(str(Path(IQS).with_name(name))), encoding='latin1')
            pdm = vendor(APP + 'app_pdm_handler.c')
            fs = vendor(APP + 'app_ppg_file_data_handler.c')
            actual = '\n'.join([
                function(fs, 'bool lk_app_ppg_file_open(enum ppg_file_type file_type)'),
                function(fs, 'bool app_ppg_file_upload(struct app_cmd_package * pack)'),
                function(pdm, 'bool app_pdm_recording_start(void)'),
                function(pdm, 'bool app_pdm_recording_stop(void)'),
            ])
            (out / 'factory_recorder.inc').write_text(actual, encoding='latin1')
            touch = patch_touch(vendor(APP + 'app_touch_button_handler.c'))
            branch = '    else if((event_bits & TOUCH_DOUBLE_TAP_EVENT) == TOUCH_DOUBLE_TAP_EVENT)'
            tail = '    else if((event_bits & TOUCH_TRIPLE_TAP_EVENT) == TOUCH_TRIPLE_TAP_EVENT)'
            body = touch.split(branch)[1].split(tail)[0]
            triple = touch.split(tail)[1].split('    else if((event_bits & TOUCH_SWIPE_LEFT_EVENT)')[0]
            (out / 'factory_touch.inc').write_text('static void dispatch_double_tap(void)' + body + '\nstatic void dispatch_triple_tap(void)' + triple, encoding='latin1')
            worker = function(patch_ble(vendor(APP + 'app_ble_handler.c')),
                              'static void app_ble_recv_handler_thread(void *thread_handler)')
            queue = function(patch_queue(vendor(QUEUE + 'bc_queue.c')),
                             'bool bc_queue_dequeue_timeout(bc_queue_type queue_type, void * const pvBuffer, uint32_t ticks)')
            (out / 'factory_worker.inc').write_text(queue + '\n' + worker, encoding='latin1')
            command = [os.environ.get('CC', 'cc'), '-std=c99', '-Wall', '-Wextra', '-Werror',
                       '-Wno-invalid-utf8', '-Wno-unused-variable', '-Wno-unused-function',
                       '-Wno-unused-but-set-variable', '-Wno-unused-parameter', '-Wno-return-type',
                       '-include', 'string.h', '-O1', '-g', '-fsanitize=address,undefined',
                       '-Dprintf=fixture_log', '-DHANDWARE_1_23_1', '-DHANDWARE_1_23_2',
                       '-I' + str(ROOT / 'tests/firmware/factory_ptt_v5'),
                       '-I' + str(ROOT / 'tests/firmware/factory_ptt_v2'),
                       '-I' + str(ROOT / 'tests/firmware/touch_tuning'), '-I' + str(out)]
            sources = [out / 'app_factory_ptt.c', out / 'IQS7211E.c', ROOT / 'tests/firmware/test_factory_ptt_v6.c']
            def run(name):
                subprocess.run(command + list(map(str, sources)) + ['-o', str(out / name)], check=True, cwd=ROOT)
                return subprocess.run([str(out / name)], capture_output=True, text=True)
            result = run('test')
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            print(result.stdout.strip())
            for filename, old, new in (
                ('app_factory_ptt.c', 'pdMS_TO_TICKS(app_factory_controls_hold_delay_ms() - 500U)', '0U'),
                ('app_factory_ptt.c', 'P.suppress_taps |= d[0] & 6U;', '(void)0;'),
                ('factory_touch.inc', 'app_package_button_up(BUTTON_THREE_PRESS);', '(void)0;'),
                ('IQS7211E.c', 'app_factory_ptt_report(&System_Data_buffer[8], true);', '(void)0;'),
                ('app_factory_ptt.c', '(uint32_t)(now - P.last_tick) >= PTT_LEASE_TICKS', 'false'),
                ('app_factory_ptt.c', 'if (!on_worker()) return;', 'if (false) return;'),
                ('factory_worker.inc', 'app_factory_ptt_before_command(ble_recv_msg.data,ble_recv_msg.data_length);', '(void)0;'),
                ('factory_touch.inc', 'app_factory_ptt_double_tap();', 'app_factory_ptt_cancel();'),
                ('factory_touch.inc', 'app_factory_ptt_double_tap();', 'app_factory_ptt_double_tap(); app_package_button_up(2);'),
            ):
                path = out / filename
                original = path.read_bytes()
                self.assertIn(old.encode(), original)
                path.write_bytes(original.replace(old.encode(), new.encode()))
                negative = run('negative')
                self.assertNotEqual(negative.returncode, 0, filename)
                self.assertIn('FAIL ', negative.stderr)
                path.write_bytes(original)
            print('PASS 9 negative controls: hold activation delay, delayed tap raw latch, default triple notification, disconnected sensor, disabled lease, wrong-task recorder access, missing command handoff, tap cancels hold, tap leaks host notification')

    def test_factory_delta(self):
        for path, patch in PATCHES.items():
            self.assertNotEqual(vendor(path), patch(vendor(path)), path)
        config = vendor(str(Path(IQS).with_name('IQS7211E_init_1232.h')))
        self.assertIn('GESTURE_ENABLE_0                         0x0F', config)
        self.assertIn('GESTURE_ENABLE_1                         0x0F', config)
        before = vendor(APP + 'app_touch_button_handler.c')
        after = patch_touch(before)
        branch = '    else if((event_bits & TOUCH_DOUBLE_TAP_EVENT) == TOUCH_DOUBLE_TAP_EVENT)\n    {'
        tail = '    else if((event_bits & TOUCH_TRIPLE_TAP_EVENT) == TOUCH_TRIPLE_TAP_EVENT)'
        body = after.split(branch)[1].split(tail)[0]
        self.assertIn('app_factory_ptt_double_tap();', body)
        self.assertNotIn('app_pdm_', body)
        self.assertNotIn('app_package_button_up', body)
        self.assertNotIn('app_touch_factory_double_tap', after)
        self.assertNotIn('BUTTON_DOUBLE_PRESS', after)
        for name in ('BUTTON_CLICK_PRESS', 'BUTTON_THREE_PRESS',
                     'BUTTON_LONG_PRESS', 'BUTTON_SWIPE_LEFT_PRESS', 'BUTTON_SWIPE_RIGHT_PRESS',
                     'BUTTON_SWIPE_UP_PRESS', 'BUTTON_SWIPE_DOWN_PRESS'):
            self.assertEqual(before.count(name), after.count(name), name)
        self.assertNotIn(APP + 'app_pdm_handler.c', PATCHES)
        self.assertNotIn(APP + 'app_ppg_file_data_handler.c', PATCHES)
        self.assertIn('.thread_stack_depth   = APP_TOUCH_EVENT_STACK_SIZE ,', after)
        worker = patch_ble(vendor(APP + 'app_ble_handler.c'))
        self.assertIn('bc_queue_dequeue_timeout(BC_QUEUE_TYPE_BLE_RECV,(void*)&ble_recv_msg,pdMS_TO_TICKS(50))', worker)
        self.assertIn('app_factory_ptt_before_command(ble_recv_msg.data,ble_recv_msg.data_length);', worker)


if __name__ == '__main__':
    unittest.main()
