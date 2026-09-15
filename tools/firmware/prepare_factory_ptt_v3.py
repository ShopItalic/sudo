#!/usr/bin/env python3
"""Reapply the P03 experiment to a new, manifest-verified factory Z62 extraction.

Does not edit the working vendor tree, sign, package, publish, or flash.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET
from prepare_vendor_baseline import prepare, ROOT

VERSION = '6.0.3.3P03'
APP = 'firmware/bc_ros/bc_application/'
IQS = 'firmware/bc_ros/bc_device/touch_button/IQS7211E/IQS7211E.c'
QUEUE = 'firmware/bc_ros/bc_module/queue/'
CONFIG = 'firmware/bc_ros/bc_config/ring_config.h'
OVERLAY = ROOT / 'firmware/factory_ptt_v3'


def once(data, old, new):
    if data.count(old) != 1:
        raise ValueError(f'Expected one factory anchor: {old[:100]!r}')
    return data.replace(old, new, 1)


def patch_iqs(data):
    data = once(data, '#include <stdint.h>',
                '#include <stdint.h>\n#include "app_factory_ptt.h"')
    anchor = 'BC_LOG_INFO("bc_touch_button_irq_process touch_i2c_open fail\\r\\n");'
    data = once(data, anchor, anchor + '\n                app_factory_ptt_cancel();\n'
                '                disable_irq(); chip_is_busy = 0; enable_irq();')
    pattern = r'IQS_I2C_Read_Data\(IQS7211E_ADDR,0x0E,&System_Data_buffer\[8\],8,STOP_TRUE\);[^\n]*'
    matches = list(re.finditer(pattern, data))
    if len(matches) != 1:
        raise ValueError('Factory status read anchor changed')
    replacement = '''if (touch_i2c_read(0x0E, &System_Data_buffer[8], 8)) {
                  app_factory_ptt_report(&System_Data_buffer[8], true);
              } else {
                  app_factory_ptt_report(NULL, false);
                  goto factory_ptt_iqs_cleanup;
              }'''
    data = data[:matches[0].start()] + replacement + data[matches[0].end():]
    anchor = '        IQS7211E_Stop_I2C_Comm_Window();'
    return once(data, anchor, 'factory_ptt_iqs_cleanup:\n' + anchor)


def patch_touch(data):
    data = once(data, '#include <string.h>',
                '#include <string.h>\n#include "app_factory_ptt.h"')
    begin = '    else if((event_bits & TOUCH_DOUBLE_TAP_EVENT) == TOUCH_DOUBLE_TAP_EVENT)\n    {'
    end = '    else if((event_bits & TOUCH_TRIPLE_TAP_EVENT) == TOUCH_TRIPLE_TAP_EVENT)'
    body = data.split(begin)[1].split(end)[0]
    if not body.endswith('    }\n'):
        raise ValueError('Double-tap body boundary changed')
    body = body[:-6]
    action, notification = body.split('     app_package_button_up(BUTTON_DOUBLE_PRESS);')
    if notification.strip():
        raise ValueError('Unexpected double-tap tail')
    data = once(data, begin + body + '    }\n',
                begin + '\n      app_factory_ptt_double_tap();\n    }\n')
    # Double tap is detected but has no action or host notification in P03.
    # Preserve other factory callbacks and touch stack. Consume a coalesced IRQ
    # before the old else-if gesture chain can discard it.
    data = once(data, 'event_struct.event_wait_for_all_bits,bc_rtos_max_delay);',
                'event_struct.event_wait_for_all_bits,bc_rtos_max_delay);\n'
                '    if ((event_bits & TOUCH_IRQ_EVENT) &&\n'
                '        !(event_bits & (TOUCH_INIT_EVENT | TOUCH_UNINIT_EVENT)))\n'
                '        bc_touch_button_irq_process();')
    data = once(data, '\n      bc_touch_button_irq_process();',
                '\n      /* Sensor IRQ already consumed before gesture dispatch. */')
    data = once(data, '      bc_touch_button_init();',
                '      app_factory_ptt_cancel();\n      bc_touch_button_init();')
    data = once(data, '      bc_touch_button_uninit();',
                '      app_factory_ptt_cancel();\n      bc_touch_button_uninit();')
    return data


def patch_ble(data):
    data = once(data, '#include "app_ble_handler.h"',
                '#include "app_ble_handler.h"\n#include "app_factory_ptt.h"')
    begin = 'static void app_ble_recv_handler_thread(void *thread_handler)\n{'
    start = data.index(begin)
    end = data.index('static uint16_t temp = 0;', start)
    body = data[start:end]
    body = once(body, '  app_ble_adv_light_start();',
                '  app_factory_ptt_worker_init();\n  app_ble_adv_light_start();')
    body = once(body, 'if(bc_queue_dequeue(BC_QUEUE_TYPE_BLE_RECV,(void*)&ble_recv_msg))',
                'app_factory_ptt_worker_service();\n'
                '\t\tif(bc_queue_dequeue_timeout(BC_QUEUE_TYPE_BLE_RECV,(void*)&ble_recv_msg,pdMS_TO_TICKS(50)))')
    body = once(body, 'app_cmd_package_parse(ble_recv_msg.data,ble_recv_msg.data_length);',
                'app_factory_ptt_before_command(ble_recv_msg.data,ble_recv_msg.data_length);\n'
                '\t\t\tapp_cmd_package_parse(ble_recv_msg.data,ble_recv_msg.data_length);\n'
                '\t\t\tapp_factory_ptt_worker_service();')
    body = once(body, '\t\t}\t\n    bc_rtos_delay(1000);',
                '\t\t\tapp_factory_ptt_worker_wait();\n\t\t}\t')
    return data[:start] + body + data[end:]


def patch_queue(data):
    signature = 'bool bc_queue_dequeue(bc_queue_type queue_type, void *  const pvBuffer)\n{'
    data = once(data, signature,
                'bool bc_queue_dequeue_timeout(bc_queue_type queue_type, void * const pvBuffer, uint32_t ticks)\n{')
    data = once(data, 'bc_base_type_t xReturn = bc_rtos_queue_receive(bc_queue[queue_type].queue_handler,pvBuffer);',
                'bc_base_type_t xReturn = xQueueReceive(bc_queue[queue_type].queue_handler,pvBuffer,ticks);')
    # Timeout is normal, not an error to log 20 times per second.
    data = once(data, '        BC_LOG_ERROR("queue dequeue fial, queue type:%d  queue_count:%d \\r\\n",queue_type,bc_queue[queue_type].queue_count);',
                '        /* Empty queue / timeout. */')
    data += '\nbool bc_queue_dequeue(bc_queue_type queue_type, void * const pvBuffer)\n{\n'
    data += '    return bc_queue_dequeue_timeout(queue_type, pvBuffer, portMAX_DELAY);\n}\n'
    return data


PATCHES = {
    IQS: patch_iqs,
    APP + 'app_touch_button_handler.c': patch_touch,
    APP + 'app_ble_handler.c': patch_ble,
    QUEUE + 'bc_queue.c': patch_queue,
    QUEUE + 'bc_queue.h': lambda s: once(s,
        'bool bc_queue_dequeue(bc_queue_type queue_type, void *  const pvBuffer);',
        'bool bc_queue_dequeue(bc_queue_type queue_type, void *  const pvBuffer);\n'
        'bool bc_queue_dequeue_timeout(bc_queue_type queue_type, void * const pvBuffer, uint32_t ticks);'),
    CONFIG: lambda s: once(s, '#define RING_1232_SOFTWARE_VERSION "6.0.3.3Z62"',
                          f'#define RING_1232_SOFTWARE_VERSION "{VERSION}"'),
}


def apply_overlay(destination, baseline):
    changes = []
    for relative, patch in PATCHES.items():
        path = destination / relative
        before = path.read_bytes()
        after = patch(before.decode('latin1').replace('\r\n', '\n')).replace('\n', '\r\n').encode('latin1')
        path.write_bytes(after)
        changes.append({'path': relative, 'beforeSha256': hashlib.sha256(before).hexdigest(),
                        'afterSha256': hashlib.sha256(after).hexdigest()})
    project = destination / baseline['project']
    tree = ET.parse(project)
    group = ET.SubElement(tree.find('./Targets/Target/Groups'), 'Group')
    ET.SubElement(group, 'GroupName').text = 'Factory PTT P03'
    files = ET.SubElement(group, 'Files')
    for name in ('app_factory_ptt.h', 'app_factory_ptt.c'):
        content = (OVERLAY / name).read_bytes()
        (destination / APP / name).write_bytes(content)
        changes.append({'path': APP + name, 'added': True, 'sha256': hashlib.sha256(content).hexdigest()})
        if name.endswith('.c'):
            node = ET.SubElement(files, 'File')
            ET.SubElement(node, 'FileName').text = name
            ET.SubElement(node, 'FileType').text = '1'
            ET.SubElement(node, 'FilePath').text = '..\\..\\..\\..\\bc_ros\\bc_application\\' + name
    project = project.with_name('factory_ptt_p03_build_only.uvprojx')
    tree.write(project, encoding='utf-8', xml_declaration=True)
    result = {'status': 'experimental-not-flashable', 'version': VERSION,
              'sourceCommit': baseline['sourceCommit'], 'verifiedOriginalFiles': baseline['verifiedOriginalFiles'],
              'project': str(project.relative_to(destination)),
              'projectSha256': hashlib.sha256(project.read_bytes()).hexdigest(), 'changes': changes,
              'recorderAndFilesystemUnmodified': True, 'factoryGestureConfigurationUnchanged': True,
              'doubleTapAction': 'disabled', 'doubleTapHostNotification': False,
              'bootloaderModified': False, 'signed': False, 'flashed': False, 'physicallyQualified': False}
    (destination / 'ptt-preparation.json').write_text(json.dumps(result, indent=2) + '\n')
    (destination / 'NON-FLASHABLE.txt').write_text('Unsigned P03 source experiment; app adoption and physical qualification pending.\n')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    destination = args.output.resolve()
    print(json.dumps(apply_overlay(destination, prepare(destination)), indent=2))
