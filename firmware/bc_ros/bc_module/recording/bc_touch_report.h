#ifndef __BC_TOUCH_REPORT_H__
#define __BC_TOUCH_REPORT_H__

#include <stdbool.h>
#include <stdint.h>

#define BC_TOUCH_REPORT_STATUS_LENGTH             8U

#define BC_TOUCH_REPORT_GESTURE_SINGLE_TAP        0x01U
#define BC_TOUCH_REPORT_GESTURE_DOUBLE_TAP        0x02U
#define BC_TOUCH_REPORT_GESTURE_TRIPLE_TAP        0x04U
#define BC_TOUCH_REPORT_GESTURE_HOLD              0x08U

#define BC_TOUCH_REPORT_INFO_ATI_ERROR            0x08U
#define BC_TOUCH_REPORT_INFO_ATI_ACTIVE           0x10U
#define BC_TOUCH_REPORT_INFO_ALP_ATI_ERROR        0x20U
#define BC_TOUCH_REPORT_INFO_RESET               0x80U
#define BC_TOUCH_REPORT_INFO_INVALID_MASK         \
    (BC_TOUCH_REPORT_INFO_ATI_ERROR |             \
     BC_TOUCH_REPORT_INFO_ATI_ACTIVE |            \
     BC_TOUCH_REPORT_INFO_ALP_ATI_ERROR |         \
     BC_TOUCH_REPORT_INFO_RESET)

typedef struct
{
    bool valid;
    bool contact;
    bool hold;
    bool double_tap;
    bool triple_tap;
    uint8_t error_flags;
    uint8_t reset_flags;
} bc_touch_report_t;

typedef void (*bc_touch_report_callback_t)(const bc_touch_report_t *report);

/* Decode the IQS7211E status bytes read from register 0x0E. */
bool bc_touch_report_decode(const uint8_t *status,
                            uint8_t status_length,
                            bool read_ok,
                            bc_touch_report_t *report);

/* A NULL callback unregisters the SUDO report consumer. */
bool bc_touch_report_register_callback(bc_touch_report_callback_t callback);

bool bc_touch_report_consumer_installed(void);

/* Deliver both valid samples and explicit invalid/read-failure samples. */
void bc_touch_report_notify(const bc_touch_report_t *report);

#endif
