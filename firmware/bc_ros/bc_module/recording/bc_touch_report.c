#include "bc_touch_report.h"

static bc_touch_report_callback_t report_callback;

bool bc_touch_report_decode(const uint8_t *status,
                            uint8_t status_length,
                            bool read_ok,
                            bc_touch_report_t *report)
{
    uint8_t info_flags;
    uint16_t x;
    uint16_t y;

    if (report == 0)
        return false;

    report->valid = false;
    report->contact = false;
    report->hold = false;
    report->double_tap = false;
    report->error_flags = 0;
    report->reset_flags = 0;

    if (!read_ok || status == 0 || status_length < BC_TOUCH_REPORT_STATUS_LENGTH)
        return false;

    info_flags = status[2];
    report->error_flags = (uint8_t)(info_flags &
                                    (BC_TOUCH_REPORT_INFO_ATI_ERROR |
                                     BC_TOUCH_REPORT_INFO_ATI_ACTIVE |
                                     BC_TOUCH_REPORT_INFO_ALP_ATI_ERROR));
    report->reset_flags = (uint8_t)(info_flags & BC_TOUCH_REPORT_INFO_RESET);
    if ((info_flags & BC_TOUCH_REPORT_INFO_INVALID_MASK) != 0U)
        return false;

    x = (uint16_t)status[4] | ((uint16_t)status[5] << 8);
    y = (uint16_t)status[6] | ((uint16_t)status[7] << 8);

    report->valid = true;
    report->contact = (x != 0xFFFFU) && (y != 0xFFFFU);
    report->hold = ((status[0] & BC_TOUCH_REPORT_GESTURE_HOLD) != 0U) &&
                   report->contact;
    report->double_tap = (status[0] & BC_TOUCH_REPORT_GESTURE_DOUBLE_TAP) != 0U;
    return true;
}

bool bc_touch_report_register_callback(bc_touch_report_callback_t callback)
{
    report_callback = callback;
    return true;
}

bool bc_touch_report_consumer_installed(void)
{
    return report_callback != 0;
}

void bc_touch_report_notify(const bc_touch_report_t *report)
{
    if (report_callback != 0 && report != 0)
        report_callback(report);
}
