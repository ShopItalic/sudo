#ifndef TEST_MOTOR_BC_DEVICE_INFO_H
#define TEST_MOTOR_BC_DEVICE_INFO_H
#include <stdint.h>
typedef struct
{
    uint32_t device_hid_touch_mode;
} bc_device_hid_info;
bc_device_hid_info *bc_device_info_get_hid_info(void);
#endif
