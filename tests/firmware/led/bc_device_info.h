#ifndef SUDO_LED_TEST_BC_DEVICE_INFO_H
#define SUDO_LED_TEST_BC_DEVICE_INFO_H

#include <stdint.h>

typedef struct
{
    uint8_t ble_connect_color;
    uint8_t ble_disconnect_color;
    uint8_t recording_color;
    uint8_t motor_start_mode;
    uint8_t motor_stop_mode;
} bc_device_led_motor_mode_info;

void bc_device_info_led_motor_mode_info_get(
    bc_device_led_motor_mode_info *info);

#endif
