"""Compile the actual P08 command and audio-worker changes with host seams."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/firmware'))
from prepare_factory_ptt_v8 import PATCHES, APP
from factory_local_recording_v8 import function_span


def vendor(name):
    return subprocess.check_output(['git', 'show', '102bfd2:' + APP + name], cwd=ROOT).decode('latin1').replace('\r\n', '\n')


def function(source, signature):
    a, b = function_span(source, signature)
    return source[a:b]


class LocalRecording(unittest.TestCase):
    def test_commands_reject_streaming_and_keep_local_capture(self):
        cmd = PATCHES[APP+'app_cmd_handler.c'](vendor('app_cmd_handler.c'))
        pdm = PATCHES[APP+'app_pdm_handler.c'](vendor('app_pdm_handler.c'))
        ptt = (ROOT / 'firmware/factory_ptt_v8/app_factory_ptt.c').read_text()
        code = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>
#define taskENTER_CRITICAL()
#define taskEXIT_CRITICAL()
struct app_cmd_package { uint8_t type, id, cmd, subcmd, data[32]; };
static unsigned replies, writes, starts, stops, handoffs;
static bool recording;
static bool on_worker(void) { return true; }
static struct { bool toggle_pending, memo_owned, owned; } P;
static void app_factory_ptt_cancel(void) { ++handoffs; }
static void app_factory_ptt_worker_service(void) { ++handoffs; }
static void app_package_send_enqueue(struct app_cmd_package *p, unsigned length)
{ assert(length == 5 && p->id == 42); ++replies; }
static bool app_pdm_recording_start(void) { ++starts; recording = true; return true; }
static bool app_pdm_recording_stop(void) { ++stops; recording = false; return true; }
static bool app_pdm_capture_recording_start(void) { return app_pdm_recording_start(); }
static bool app_pdm_capture_recording_stop(void) { return app_pdm_recording_stop(); }
static void app_touch_pdm_key_flag_clear(void) {}
static void bc_delay_ms(unsigned ms) { (void)ms; }
static void app_ppg_list_capture_audio_up_check(void) {}
#define PDM_DATA_SEND_SIZE 220
#define BC_QUEUE_TYPE_PDM_COLLECTION_DATA 0
#define PDM_MODE_OFFLINE 1
#define PDM_MODE_KEY_OFFLINE 2
#define PDM_MODE_IDIE 0
#define PDM_MODE_ONLINE 3
static struct { struct { uint8_t pdm_data_buff[226]; } pdm_package; } pdm_ble_package;
static int mode;
static jmp_buf worker_exit;
static bool received;
static int app_pdm_mode_get(void) { return mode; }
static bool bc_queue_dequeue(int queue, void *data)
{ (void)queue; (void)data; if (received) longjmp(worker_exit, 1); received = true; return true; }
static void app_ppg_file_write(const uint8_t *data, unsigned length)
{ assert(data == &pdm_ble_package.pdm_package.pdm_data_buff[6] && length == 220); ++writes; }
'''
        signatures = [
            'static uint8_t app_cmd_pdm(struct app_cmd_package * cmd_package)',
            'static uint8_t app_cmd_ipc_event(struct app_cmd_package * cmd_package)',
        ]
        code += '\n'.join(function(cmd, sig) for sig in signatures)
        code += '\n' + function(ptt, 'void app_factory_ptt_before_command(const uint8_t *d, unsigned n)')
        for sig in ('static void app_pdm_handler_thread(void *thread_handler)',
                    'void app_pdm_start(struct app_cmd_package * pack)',
                    'void app_pdm_stop(struct app_cmd_package * pack)',
                    'void app_pdm_touch_start(void)', 'void app_pdm_touch_stop(void)',
                    'bool app_pdm_switch_online_to_offline(void)'):
            code += '\n' + function(pdm, sig)
        code += r'''
int main(void) {
    struct app_cmd_package cmd = {0, 42, 0x71, 5, {1}};
    const uint8_t retired[] = {0, 1, 2, 3, 0xfd};
    unsigned i, value;
    app_cmd_pdm(&cmd);
    assert(recording && starts == 1 && stops == 0 && cmd.data[0] == 1);
    for (i = 0; i < sizeof(retired); ++i) for (value = 0; value <= 1; ++value) {
        unsigned before = replies;
        cmd.subcmd = retired[i]; cmd.data[0] = value;
        app_factory_ptt_before_command((uint8_t *)&cmd, 5);
        app_cmd_pdm(&cmd);
        assert(replies == before + 1 && cmd.data[0] == 0);
        assert(recording && starts == 1 && stops == 0 && handoffs == 0);
    }
    for (i = 1; i <= 2; ++i) {
        unsigned before = replies;
        cmd.cmd = 0xee; cmd.subcmd = i; cmd.data[0] = 1;
        app_factory_ptt_before_command((uint8_t *)&cmd, 5);
        app_cmd_ipc_event(&cmd);
        assert(replies == before + 1 && cmd.data[0] == 0 && recording && handoffs == 0);
    }
    app_pdm_start(&cmd); app_pdm_stop(&cmd); app_pdm_touch_start(); app_pdm_touch_stop();
    assert(!app_pdm_switch_online_to_offline() && recording && stops == 0);
    for (mode = PDM_MODE_OFFLINE; mode <= PDM_MODE_ONLINE; ++mode) {
        received = false;
        if (setjmp(worker_exit) == 0) app_pdm_handler_thread(0);
    }
    assert(writes == 2); /* Both local modes write; online mode has no sender. */
    cmd.cmd = 0x71; cmd.subcmd = 5; cmd.data[0] = 0;
    app_cmd_pdm(&cmd);
    assert(!recording && stops == 1 && cmd.data[0] == 1);
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='ring-local-only-') as folder:
            c = Path(folder)/'test.c'; exe = Path(folder)/'test'
            c.write_text(code, encoding='latin1')
            subprocess.run([os.environ.get('CC','cc'), '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-Wno-unused-parameter', '-Wno-return-type', '-Wno-invalid-utf8',
                            '-fsanitize=address,undefined', str(c), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

    def test_local_storage_and_recording_functions_are_preserved(self):
        before = vendor('app_pdm_handler.c')
        after = PATCHES[APP+'app_pdm_handler.c'](before)
        for sig in ('bool app_pdm_recording_start(void)', 'bool app_pdm_recording_stop(void)',
                    'bool app_pdm_capture_recording_start(void)', 'bool app_pdm_capture_recording_stop(void)'):
            self.assertEqual(function(before, sig), function(after, sig))
        self.assertNotIn('ble_calss.ble_send(', after)
        self.assertNotIn('app_pdm_mode_set(PDM_MODE_ONLINE)', after)

if __name__ == '__main__':
    unittest.main()
