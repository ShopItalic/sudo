"""P09 scroll integration, applied only to the manifest-verified factory tree."""
from prepare_factory_ptt_v3 import once

ADDED = ('app_factory_scroll.c', 'app_factory_scroll.h')
GAP = 'firmware/bc_ros/bc_module/ble/src/bc_ble_gap.c'

def edit_function(source, signature, transform):
    start = source.index(signature + '\n{')
    end = source.index('\n}', start) + 2
    return source[:start] + transform(source[start:end]) + source[end:]

def patch_ble(source):
    source = '#include "app_factory_scroll.h"\n#include <stdbool.h>\nstatic bool factory_scroll_peer_ready;\n' + source
    # The old factory HID helper stays untouched and uninitialized. Its malformed
    # composite descriptor is replaced by one small, optional mouse service.
    source = once(source, '    if(hid_info->device_hid_type == 1)\n\t{\n\t\thids_init(&m_conn_handle);\n\t}',
                  '    app_factory_scroll_init(); /* P09 runtime capability; no provisioning mutation. */')
    source = once(source, 'app_factory_controls_init(hid_info->device_hid_type == 1);',
                  'app_factory_controls_init(true); /* Peer Manager is the sole FDS init owner. */')
    # Source is decoded as latin1 by the preparation machinery. Match the call's
    # enclosing conditional without depending on the vendor comment encoding.
    def init(body):
        if '\tif(hid_info->device_hid_type == 1)\n\t{' in body:
            a = body.index('\tif(hid_info->device_hid_type == 1)\n\t{')
            b = body.index('\n\t}', a) + len('\n\t}')
            if body[a:b].count('peer_manager_init();') != 1: raise ValueError('Unexpected PM init block')
            body = body[:a] + '\tpeer_manager_init();' + body[b:]
        return body
    source = edit_function(source, 'void bc_ble_init(void)', init)
    def pm(body):
        if body.count('APP_ERROR_CHECK(err_code);') != 3: raise ValueError('Unexpected PM setup')
        body = body.replace('APP_ERROR_CHECK(err_code);',
            'if (err_code != NRF_SUCCESS) { factory_scroll_last_error = err_code; return; }')
        return body[:-1] + '    factory_scroll_peer_ready = true;\n}'
    source = edit_function(source, 'static void peer_manager_init(void)', pm)
    a = source.index('case BLE_GAP_EVT_SEC_PARAMS_REQUEST:')
    b = source.index('case BLE_GATTC_EVT_TIMEOUT:', a)
    source = source[:a] + '''case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            /* PM owns pairing and encrypted system-attribute restoration.
             * Never fall through to SYS_ATTR_MISSING during pairing. */
            if (!factory_scroll_peer_ready) {
                factory_scroll_last_error = sd_ble_gap_sec_params_reply(m_conn_handle,
                    BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            }
            break;
        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            if (!factory_scroll_peer_ready) {
                factory_scroll_last_error = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            }
            break;
        ''' + source[b:]
    return source

def patch_gap(source):
    source = once(source, '\tif(hid_info->device_hid_type == 1)\n',
                  '\tif(true) /* P09 provides an optional HID mouse independently of provisioning. */\n')
    return once(source, 'sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_HID)',
                'sd_ble_gap_appearance_set(BLE_APPEARANCE_HID_MOUSE)')
