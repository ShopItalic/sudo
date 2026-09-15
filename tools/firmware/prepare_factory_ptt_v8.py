#!/usr/bin/env python3
"""Apply P08 to a fresh manifest-verified Z62 extraction. Never flash."""
import argparse
import json
from pathlib import Path
import prepare_factory_ptt_v3 as base
import factory_battery_v8 as battery
import factory_local_recording_v8 as local_recording
from prepare_vendor_baseline import prepare, ROOT

VERSION = '6.0.3.3P08'
OVERLAY = ROOT / 'firmware/factory_ptt_v8'
APP, IQS, QUEUE = base.APP, base.IQS, base.QUEUE
BLE = 'firmware/bc_ros/bc_module/ble/src/bc_ble.c'
MOTOR = 'firmware/bc_ros/bc_module/motor/bc_linear_motor.c'
once = base.once
patch_iqs, patch_queue = base.patch_iqs, base.patch_queue

def patch_delete_command(s):
    s = local_recording.patch_commands(s)
    s = '#include "app_factory_delete.h"\n' + s
    s = once(s, 'if(app_ppg_file_delete((char*)cmd_package->data))',
             'if(cmd_package->length >= 4U &&\n'
             '               app_ppg_file_delete_request(cmd_package->data, cmd_package->length - 4U))')
    # The factory field is uint8_t. Reject truncation and undersized frames
    # before dispatch can treat stale buffer bytes as a deletion request.
    return once(s, '    cmd_package->length = pack_length;',
                '    if (!cmd_pack || pack_length < 4U || pack_length > 254U) return;\n'
                '    cmd_package->length = (uint8_t)pack_length;')

def patch_delete_file(s):
    s = '#include "app_factory_delete.h"\n' + s
    signature = 'bool app_ppg_file_delete(char *path)'
    start = s.index(signature + '\n{')
    end = s.index('\n}', start) + 2
    replacement = '''/* P08: explicit commands only; no deferred filename-based erasure. */
static factory_delete_workspace delete_workspace;
volatile factory_delete_result factory_delete_last_result = FACTORY_DELETE_INVALID;
volatile int factory_delete_last_storage_error;

bool app_ppg_file_delete_request(const uint8_t *data, unsigned length)
{
    factory_delete_result result = FACTORY_DELETE_INVALID;
    taskENTER_CRITICAL();
    if (app_ppg_file_hardle.fls_status != PPG_FLS_IDIE ||
        app_ppg_file_hardle.ppg_file_status) {
        taskEXIT_CRITICAL();
        factory_delete_last_result = FACTORY_DELETE_BUSY;
        return false;
    }
    app_ppg_file_hardle.fls_status = PPG_FLS_BUSY;
    taskEXIT_CRITICAL();

    if (factory_delete_parse_name(data, length, &delete_workspace)) {
        bc_spi_flash_device_open();
        result = factory_delete_remove(&app_ppg_file_hardle.lfs_fls_ppg_handle,
                                       &delete_workspace);
        bc_spi_flash_device_close();
    }
    factory_delete_last_storage_error = delete_workspace.storage_error;
    factory_delete_last_result = result;
    taskENTER_CRITICAL();
    app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;
    taskEXIT_CRITICAL();
    return result == FACTORY_DELETE_REMOVED ||
           result == FACTORY_DELETE_ALREADY_ABSENT;
}

bool app_ppg_file_delete(char *path)
{
    unsigned length = 0;
    if (!path) return false;
    /* Internal callers use factory FILE_NAME_LENG arrays. The wire path
     * above always uses its received length and never scans beyond it. */
    while (length <= FACTORY_DELETE_NAME_MAX && path[length]) ++length;
    if (length > FACTORY_DELETE_NAME_MAX) return false;
    return app_ppg_file_delete_request((const uint8_t *)path, length);
}'''
    return s[:start] + replacement + s[end:]

def patch_touch(s):
    s = base.patch_touch(s)
    s = once(s, '#include "app_factory_ptt.h"', '#include "app_factory_ptt.h"\n#include "app_factory_controls.h"')
    for direction in ('UP', 'DOWN', 'LEFT', 'RIGHT'):
        s = once(s, '      app_package_button_up(BUTTON_SWIPE_'+direction+'_PRESS);',
                 '      if (app_factory_controls_swipe_enabled(FACTORY_SWIPE_'+direction+'))\n'
                 '          app_package_button_up(BUTTON_SWIPE_'+direction+'_PRESS);')
    return once(s, '      app_package_button_up(BUTTON_THREE_PRESS);',
                '      if (!app_factory_ptt_triple_tap())\n'
                '          app_package_button_up(BUTTON_THREE_PRESS);')

def patch_ble(s):
    s = base.patch_ble(s)
    s = once(s, '#include "app_factory_ptt.h"', '#include "app_factory_ptt.h"\n#include "app_factory_controls.h"')
    s = s.replace('app_factory_ptt_worker_service();', 'app_factory_controls_service();\n            app_factory_ptt_worker_service();')
    s = once(s, 'app_factory_ptt_before_command(ble_recv_msg.data,ble_recv_msg.data_length);\n\t\t\tapp_cmd_package_parse(ble_recv_msg.data,ble_recv_msg.data_length);',
             'if (!app_factory_controls_command(ble_recv_msg.data,ble_recv_msg.data_length)) {\n'
             '                app_factory_ptt_before_command(ble_recv_msg.data,ble_recv_msg.data_length);\n'
             '                app_cmd_package_parse(ble_recv_msg.data,ble_recv_msg.data_length);\n            }')
    return s

def patch_init(s):
    s = '#include "app_factory_controls.h"\n' + s
    # PM owns the sole FDS init on HID hardware. Controls own it only when
    # PM is disabled, so a failed first init cannot be hidden by a retry.
    return once(s, '\tble_stack_init();', '\tble_stack_init();\n    app_factory_controls_init(hid_info->device_hid_type == 1);')

def replace_function(s, signature, body):
    start = s.index(signature + '\n{')
    end = s.index('\n}', start) + 2
    return s[:start] + signature + '\n{\n' + body + '\n}' + s[end:]


def patch_motor(s):
    s = '#include "app_factory_controls.h"\n#include "bc_rtos.h"\n' + s
    signature = 'static void linear_motor_pwm_start(struct pwm_config *linear_motor_config)'
    s = once(s, signature, '#include "app_factory_motor.h"\n\n' + signature)
    # Every caller holds the task critical section, including shared sequence
    # writes. There is no blocking stop wait, allocation, logging or RTOS delay.
    s = replace_function(s, signature, '''    q_device_close(linear_motor_pwm_dev);
    q_device_cfg(linear_motor_pwm_dev, linear_motor_config, NULL);
    q_device_open(linear_motor_pwm_dev);
    pwm_falsh.pwm_status = LINEAR_MOTOR_PWM_BUSY;
    pwm_falsh.pwm_mode = linear_motor_config->pwm_parameter_config.flags == 2 ? PWM_LOOP : PWM_STOP;
    q_device_ctrl(linear_motor_pwm_dev, PWM_CTRL_START, NULL);''')
    s = replace_function(s, 'static void linear_motor_pwm_callback(void)',
                         '    factory_motor_stopped = true; /* ISR: flags only. */')
    s = once(s, 'void bc_linear_motor_start(enum LINEAR_MOTOR_MODE mode)\n{\n  bc_ldo_motor_power_on();\n    bc_delay_ms(20);',
             'void bc_linear_motor_start(enum LINEAR_MOTOR_MODE mode)\n{\n'
             '    uint32_t generation;\n    bool accepted;\n'
             '    taskENTER_CRITICAL();\n    accepted = factory_motor_begin_locked(&generation);\n'
             '    taskEXIT_CRITICAL();\n    if (!accepted) return;\n'
             '    bc_delay_ms(20);\n    taskENTER_CRITICAL();\n'
             '    if (!factory_motor_current_locked(generation)) { taskEXIT_CRITICAL(); return; }')
    start = s.index('void bc_linear_motor_start(enum LINEAR_MOTOR_MODE mode)')
    end = s.index('\n}', start)
    s = s[:end] + '\n    taskEXIT_CRITICAL();' + s[end:]
    s = replace_function(s, 'void bc_linear_motor_pwm_out(void *linear_motor_config)', '''    struct pwm_config *cfg = (struct pwm_config *)linear_motor_config;
    uint32_t generation;
    if (!cfg || !cfg->pwm_parameter_config.p_common ||
        !cfg->pwm_parameter_config.length ||
        cfg->pwm_parameter_config.length > sizeof(linear_motor_pwm_seq_values)/sizeof(uint16_t)) return;
    taskENTER_CRITICAL();
    if (factory_motor_begin_locked(&generation)) {
        memcpy(linear_motor_pwm_seq_values, cfg->pwm_parameter_config.p_common,
               cfg->pwm_parameter_config.length * sizeof(uint16_t));
        linear_motor_pwm_config = *cfg;
        linear_motor_pwm_config.pwm_parameter_config.p_common = linear_motor_pwm_seq_values;
    } else { taskEXIT_CRITICAL(); return; }
    taskEXIT_CRITICAL();
    bc_delay_ms(20);
    taskENTER_CRITICAL();
    if (factory_motor_current_locked(generation))
        linear_motor_pwm_start(&linear_motor_pwm_config);
    taskEXIT_CRITICAL();''')
    for kind in ('strong', 'continuous'):
        s = replace_function(s, 'void bc_linear_motor_'+kind+'_vibration_start(void)',
                             '    uint32_t generation;\n    taskENTER_CRITICAL();\n'
                             '    if (factory_motor_begin_locked(&generation))\n'
                             '        linear_motor_pwm_start(&'+kind+'_vibration_pwm_config);\n'
                             '    taskEXIT_CRITICAL();')
    s = replace_function(s, 'void bc_linear_motor_stop(void)',
                         '    bc_linear_motor_silence();')
    # Configuration may be written by the BLE task while another task starts.
    for signature in (
        'void bc_linear_motor_pwm_idie_register_callback(void * register_callback)',
        'void bc_linear_motor_strong_vibration_pwm_config(uint16_t pwm_seq_values,uint8_t playback_count,uint16_t repeats)',
        'void bc_linear_motor_continuous_vibration_pwm_config(uint16_t pwm_seq_values,uint8_t playback_count,uint16_t repeats)'):
        a = s.index(signature + '\n{'); b = s.index('\n}', a)
        body = s[a+len(signature)+2:b]
        cancel = '\n    factory_motor_stop_locked();' if 'vibration_pwm_config(' in signature else ''
        s = replace_function(s, signature, '    taskENTER_CRITICAL();' + cancel + body + '\n    taskEXIT_CRITICAL();')
    return s

def patch_vibrate(s):
    # Keep temporary manual-pattern parameters private until the guarded
    # motor entry point copies them. Its settling delay owns a cancel token.
    s = once(s, 'static uint16_t s_vibrate_pwm_seq[4] = {0};', '')
    s = once(s, 'static struct pwm_config s_vibrate_pwm_config = {0};', '')
    signature = 'uint8_t app_vibrate_start(vibrate_mode_t mode, uint8_t count)'
    start = s.rindex(signature + '\n{')
    end = s.index('\n}', start) + 2
    body = s[start:end]
    body = body.replace(signature + '\n{', signature + '\n{\n    uint16_t s_vibrate_pwm_seq[4] = {0};\n    struct pwm_config s_vibrate_pwm_config = {0};', 1)
    body = once(body, '    bc_ldo_motor_power_on();\n    bc_delay_ms(20);', '    /* Settling and cancellation belong to bc_linear_motor_pwm_out. */')
    return s[:start] + body + s[end:]

LED = 'firmware/bc_ros/bc_module/led/bc_ic_led.c'

def patch_recording_light(s):
    # All five standard-board paths: start offline/online/capture, and restore
    # recording feedback after a connection/disconnection status cue.
    before = 'bc_ic_led_color_value_get(info.recording_color, &g, &r, &b);'
    if s.count(before) != 5:
        raise ValueError('Unexpected factory recording LED paths')
    s = '#include "app_factory_controls.h"\n' + s
    return s.replace(before, 'if (app_factory_controls_recording_light())\n            ' + before)

PATCHES = dict(base.PATCHES)
PATCHES.update(battery.PATCHES)
PATCHES.update({APP+'app_touch_button_handler.c': patch_touch,
                APP+'app_ble_handler.c': patch_ble, APP+'app_linear_motor_handler.c': patch_vibrate,
                BLE: patch_init, MOTOR: patch_motor, LED: patch_recording_light,
                APP+'app_cmd_handler.c': patch_delete_command,
                APP+'app_pdm_handler.c': local_recording.patch_pdm,
                APP+'app_ppg_file_data_handler.c': patch_delete_file,
                base.CONFIG: lambda s: once(s, '#define RING_1232_SOFTWARE_VERSION "6.0.3.3Z62"',
                                            '#define RING_1232_SOFTWARE_VERSION "'+VERSION+'"')})

def apply_overlay(destination, baseline):
    # Reuse only the preparation machinery, never a previous generated tree.
    # Explicit additions and metadata are part of this version's own recipe.
    import hashlib
    import xml.etree.ElementTree as ET
    for path in (destination/'firmware').rglob('*'):
        if path.is_file() and path.suffix in ('.c', '.h'):
            s = path.read_text(encoding='latin1')
            if '0x1001' in s and ('fds_record_write(' in s or 'fds_record_update(' in s):
                raise ValueError('Review FDS file ID collision: '+str(path))
    base.PATCHES, base.OVERLAY, base.VERSION = PATCHES, OVERLAY, VERSION
    result = base.apply_overlay(destination, baseline)
    old_project = destination/result['project']
    tree = ET.parse(old_project)
    group = tree.find('./Targets/Target/Groups/Group[last()]')
    group.find('GroupName').text = 'Factory PTT P08 controls'
    for name in ('app_factory_controls.c', 'app_factory_controls.h', 'app_factory_motor.h',
                 'app_factory_delete.c', 'app_factory_delete.h') + battery.ADDED:
        content = (OVERLAY/name).read_bytes(); (destination/APP/name).write_bytes(content)
        result['changes'].append({'path': APP+name, 'added': True, 'sha256': hashlib.sha256(content).hexdigest()})
        if name.endswith('.c'):
            node = ET.SubElement(group.find('Files'), 'File')
            ET.SubElement(node, 'FileName').text = name
            ET.SubElement(node, 'FileType').text = '1'
            ET.SubElement(node, 'FilePath').text = '..\\..\\..\\..\\bc_ros\\bc_application\\'+name
    project = old_project.with_name('factory_ptt_p08_build_only.uvprojx')
    tree.write(project, encoding='utf-8', xml_declaration=True)
    # The helper-created intermediate lacks the controls compilation unit.
    # Keep only the complete P08 build entry point, not a misleading P03 one.
    if old_project.name != 'factory_ptt_p03_build_only.uvprojx':
        raise ValueError('Unexpected generated intermediate project')
    old_project.unlink()
    result.update(project=str(project.relative_to(destination)), projectSha256=hashlib.sha256(project.read_bytes()).hexdigest(),
                  recorderAndFilesystemUnmodified=False,
                  localRecordingFunctionsUnmodified=True, savedFileUploadWorkerUnmodified=True,
                  liveAudioStreaming=False, littlefsLibraryUnmodified=True,
                  battery={'stage': 'filter-and-acquisition-error-repair',
                           'filter': 'ten-valid-sample-trimmed-mean-with-recovery',
                           'invalidPercent': 255, 'motorSettlingMs': 250,
                           'factoryAcquisitionUs': 10, 'factoryCorrectionMv': 200,
                           'factoryCurveUnchanged': True, 'physicallyCalibrated': False},
                  deletion={'stage': 'bounded-command-hardening', 'command': '0x36/0x12',
                            'receiverReceiptProtocol': False, 'automaticCleanup': False,
                            'alreadyAbsentIsSuccess': True, 'postDeleteAbsenceCheck': True,
                            'releaseBlockedBy': ['factory flash error propagation and mount-failure autoformat',
                                                 'receipt-free low-space reclamation',
                                                 'persistent recording identity and receipt',
                                                 'filesystem ownership audit', 'physical qualification']},
                  doubleTapAction='programmable; disabled by default', tripleTapAction='programmable; disabled by default',
                  settings={'command': '0x84', 'get': '0x10', 'set': '0x11', 'schema': 4, 'legacySchemasSupported': [1, 2, 3],
                            'swipesDefaultEnabled': ['up', 'down', 'left', 'right'],
                            'fdsFile': '0x1001', 'fdsKey': '0x0001', 'hapticsDefault': True, 'recordingLightDefault': True,
                            'holdDelayDefaultMs': 500, 'holdDelayMinMs': 500, 'holdDelayMaxMs': 5000, 'holdDelayStepMs': 500})
    (destination/'ptt-preparation.json').write_text(json.dumps(result, indent=2)+'\n')
    (destination/'NON-FLASHABLE.txt').write_text('Unsigned P08 experiment. Physical qualification pending.\n')
    return result

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(apply_overlay(args.output.resolve(), prepare(args.output.resolve())), indent=2))
