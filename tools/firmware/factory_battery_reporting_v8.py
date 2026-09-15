"""Compact P08 battery notifications without changing their wire contract."""
from factory_battery_v8 import replace_function

PACKAGE = 'firmware/bc_ros/bc_application/app_package.c'
SIGNATURES = ('void app_package_precent_up(uint8_t data)',
              'void app_package_precent_status_up(uint16_t data)')


def patch_package(source):
    # Each buffer is private to this call, including concurrent/nested calls.
    # The factory sender consumes/copies it before returning, just as it did
    # with the two full-size automatic packet structures. Keep both guards,
    # the same direct ble_calss.ble_send call, and the idle-timer restart.
    percent = '''void app_package_precent_up(uint8_t data)
{
    if (!ble_calss.ble_connect_status()) return;
    if (!app_ble_notify_allowed()) return;
    uint8_t packet[5] = {0, 0x2A, 0x12, 0, data};
#if defined(FACTORY_USE)
    packet[3] = 0x02;
#endif
    ble_calss.ble_send(packet, sizeof(packet));
    app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);
}'''
    status = '''void app_package_precent_status_up(uint16_t data)
{
    if (!ble_calss.ble_connect_status()) return;
    if (!app_ble_notify_allowed()) return;
    /* Preserve the factory's little-endian uint16 payload explicitly. */
    uint8_t packet[6] = {0, 0x2A, 0x12, 0x01,
                         (uint8_t)data, (uint8_t)(data >> 8)};
    ble_calss.ble_send(packet, sizeof(packet));
    app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);
}'''
    source = replace_function(source, SIGNATURES[0], percent)
    return replace_function(source, SIGNATURES[1], status)
