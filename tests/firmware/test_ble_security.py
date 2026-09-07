#!/usr/bin/env python3
"""Exercise the production GAP/GATTS switch slice with SoftDevice call spies.

Only the two adjacent event cases are extracted; this is event dispatch
coverage, not an over-the-air pairing or Peer Manager integration test.
"""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/firmware'))
from extract_recording_sources import function_bytes

source = Path(os.environ.get('BLE_SECURITY_SOURCE', str(ROOT / 'firmware/bc_ros/bc_module/ble/src/bc_ble.c')))
handler = function_bytes(source.read_bytes(), 'ble_evt_handler')
start = handler.index(b'case BLE_GAP_EVT_SEC_PARAMS_REQUEST:')
end = handler.index(b'case BLE_GATTC_EVT_TIMEOUT:', start)
cases = handler[start:end].decode('utf-8', errors='replace')
prelude = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define BLE_GAP_EVT_SEC_PARAMS_REQUEST 1
#define BLE_GATTS_EVT_SYS_ATTR_MISSING 2
#define BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP 5
#define APP_ERROR_CHECK(code) assert((code) == 0)
typedef struct { unsigned device_hid_type; } bc_device_hid_info;
static bc_device_hid_info hid;
static uint16_t m_conn_handle = 17;
#ifdef HANDWARE_1_23_4
static bool m_conn_rejected;
#endif
static unsigned replies, attributes;
static bc_device_hid_info *bc_device_info_get_hid_info(void) { return &hid; }
static unsigned sd_ble_gap_sec_params_reply(uint16_t handle, unsigned status, void *p, void *k)
{
    assert(handle == 17 && status == BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP && !p && !k);
    ++replies; return 0;
}
static unsigned sd_ble_gatts_sys_attr_set(uint16_t handle, void *p, unsigned size, unsigned flags)
{
    assert(handle == 17 && !p && !size && !flags);
    ++attributes; return 0;
}
static void dispatch(unsigned event)
{
    unsigned err_code;
    switch(event) {
'''
tail = r'''
    }
}
int main(void)
{
    for (unsigned type = 0; type <= 2; ++type) {
        hid.device_hid_type = type;
        replies = attributes = 0;
        dispatch(BLE_GAP_EVT_SEC_PARAMS_REQUEST);
        assert(replies == (type == 1 ? 0U : 1U));
        assert(attributes == 0); /* A security request must not reset CCCDs. */
        dispatch(BLE_GATTS_EVT_SYS_ATTR_MISSING);
        assert(attributes == 1);
        assert(replies == (type == 1 ? 0U : 1U));
    }
#ifdef HANDWARE_1_23_4
    m_conn_rejected = true;
    replies = attributes = 0;
    dispatch(BLE_GAP_EVT_SEC_PARAMS_REQUEST);
    dispatch(BLE_GATTS_EVT_SYS_ATTR_MISSING);
    assert(!replies && !attributes);
#endif
    return 0;
}
'''
out = ROOT / 'build/firmware/tests'
out.mkdir(parents=True, exist_ok=True)
c = out / 'ble_security_events.c'
c.write_text(prelude + cases + tail)
for variant in ('standard', 'rejected_connection'):
    exe = out / ('test_ble_security_' + variant)
    command = [os.environ.get('CC', 'cc'), '-std=c99', '-Wall', '-Wextra', '-Werror',
               '-fsanitize=address,undefined', '-g']
    if variant == 'rejected_connection': command.append('-DHANDWARE_1_23_4')
    subprocess.run(command + [str(c), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS: BLE security event routing (HID delegation, non-HID rejection, system attributes, rejected connection)')
