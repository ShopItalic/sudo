#include "bc_touch_report.h"

#include <stdio.h>

static unsigned checks;
static unsigned failures;
static unsigned callback_count;
static bc_touch_report_t callback_report;

static void check_condition(bool condition, const char *expression,
                            unsigned line)
{
    ++checks;
    if (!condition)
    {
        ++failures;
        fprintf(stderr, "FAIL line %u: %s\n", line, expression);
    }
}

#define CHECK(condition) check_condition((condition), #condition, __LINE__)

static void observe_report(const bc_touch_report_t *report)
{
    ++callback_count;
    if (report != 0)
        callback_report = *report;
}

static void set_coordinates(uint8_t status[BC_TOUCH_REPORT_STATUS_LENGTH],
                            uint16_t x, uint16_t y)
{
    status[4] = (uint8_t)(x & 0xFFU);
    status[5] = (uint8_t)(x >> 8);
    status[6] = (uint8_t)(y & 0xFFU);
    status[7] = (uint8_t)(y >> 8);
}

static void test_valid_statuses(void)
{
    uint8_t status[BC_TOUCH_REPORT_STATUS_LENGTH] = {0};
    bc_touch_report_t report = {0};

    set_coordinates(status, 123U, 456U);
    CHECK(bc_touch_report_decode(status, sizeof(status), true, &report));
    CHECK(report.valid);
    CHECK(report.contact);
    CHECK(!report.hold);
    CHECK(!report.double_tap);

    status[0] = BC_TOUCH_REPORT_GESTURE_HOLD;
    CHECK(bc_touch_report_decode(status, sizeof(status), true, &report));
    CHECK(report.valid);
    CHECK(report.contact);
    CHECK(report.hold);

    status[0] = BC_TOUCH_REPORT_GESTURE_DOUBLE_TAP;
    CHECK(bc_touch_report_decode(status, sizeof(status), true, &report));
    CHECK(report.valid);
    CHECK(report.double_tap);
    CHECK(!report.hold);
}

static void test_release_and_hold_without_contact(void)
{
    uint8_t status[BC_TOUCH_REPORT_STATUS_LENGTH] = {0};
    bc_touch_report_t report;

    set_coordinates(status, 0xFFFFU, 0xFFFFU);
    CHECK(bc_touch_report_decode(status, sizeof(status), true, &report));
    CHECK(report.valid);
    CHECK(!report.contact);
    CHECK(!report.hold);
    CHECK(!report.double_tap);

    status[0] = BC_TOUCH_REPORT_GESTURE_DOUBLE_TAP;
    CHECK(bc_touch_report_decode(status, sizeof(status), true, &report));
    CHECK(report.valid);
    CHECK(!report.contact);
    CHECK(report.double_tap);

    set_coordinates(status, 0xFFFFU, 1U);
    CHECK(bc_touch_report_decode(status, sizeof(status), true, &report));
    CHECK(report.valid);
    CHECK(!report.contact);
    CHECK(!report.hold);

    status[0] = BC_TOUCH_REPORT_GESTURE_HOLD;
    CHECK(bc_touch_report_decode(status, sizeof(status), true, &report));
    CHECK(report.valid);
    CHECK(!report.contact);
    CHECK(!report.hold);
}

static void test_invalid_readings(void)
{
    uint8_t status[BC_TOUCH_REPORT_STATUS_LENGTH] = {0};
    bc_touch_report_t report;

    set_coordinates(status, 1U, 2U);
    status[0] = BC_TOUCH_REPORT_GESTURE_HOLD |
                BC_TOUCH_REPORT_GESTURE_DOUBLE_TAP;
    status[2] = BC_TOUCH_REPORT_INFO_RESET;
    CHECK(!bc_touch_report_decode(status, sizeof(status), true, &report));
    CHECK(!report.valid);
    CHECK(!report.contact);
    CHECK(!report.hold);
    CHECK(!report.double_tap);
    CHECK(report.error_flags == 0U);
    CHECK(report.reset_flags == BC_TOUCH_REPORT_INFO_RESET);

    status[2] = BC_TOUCH_REPORT_INFO_ATI_ERROR |
                BC_TOUCH_REPORT_INFO_ATI_ACTIVE |
                BC_TOUCH_REPORT_INFO_ALP_ATI_ERROR;
    CHECK(!bc_touch_report_decode(status, sizeof(status), true, &report));
    CHECK(!report.valid);
    CHECK(report.error_flags == (BC_TOUCH_REPORT_INFO_ATI_ERROR |
                                 BC_TOUCH_REPORT_INFO_ATI_ACTIVE |
                                 BC_TOUCH_REPORT_INFO_ALP_ATI_ERROR));
    CHECK(report.reset_flags == 0U);

    status[2] = 0U;
    CHECK(!bc_touch_report_decode(status, sizeof(status), false, &report));
    CHECK(!report.valid);
    CHECK(!report.contact);
    CHECK(!report.hold);
    CHECK(!report.double_tap);

    CHECK(!bc_touch_report_decode(status, sizeof(status) - 1U, true, &report));
    CHECK(!report.valid);
    CHECK(!bc_touch_report_decode(0, sizeof(status), true, &report));
    CHECK(!bc_touch_report_decode(status, sizeof(status), true, 0));
}

static void test_callback_registration(void)
{
    uint8_t status[BC_TOUCH_REPORT_STATUS_LENGTH] = {0};
    bc_touch_report_t report = {0};

    CHECK(bc_touch_report_register_callback(0));
    CHECK(!bc_touch_report_consumer_installed());
    callback_count = 0U;
    bc_touch_report_notify(&report);
    CHECK(callback_count == 0U);

    CHECK(bc_touch_report_register_callback(observe_report));
    CHECK(bc_touch_report_consumer_installed());
    set_coordinates(status, 8U, 9U);
    CHECK(bc_touch_report_decode(status, sizeof(status), true, &report));
    bc_touch_report_notify(&report);
    CHECK(callback_count == 1U);
    CHECK(callback_report.valid);
    CHECK(callback_report.contact);

    status[0] = 0U;
    set_coordinates(status, 0xFFFFU, 0xFFFFU);
    CHECK(bc_touch_report_decode(status, sizeof(status), true, &report));
    bc_touch_report_notify(&report);
    CHECK(callback_count == 2U);
    CHECK(callback_report.valid);
    CHECK(!callback_report.contact);
    CHECK(!callback_report.hold);

    status[0] = BC_TOUCH_REPORT_GESTURE_HOLD;
    CHECK(!bc_touch_report_decode(status, sizeof(status), false, &report));
    bc_touch_report_notify(&report);
    CHECK(callback_count == 3U);
    CHECK(!callback_report.valid);
    CHECK(!callback_report.hold);

    CHECK(bc_touch_report_register_callback(0));
    CHECK(!bc_touch_report_consumer_installed());
    bc_touch_report_notify(&report);
    CHECK(callback_count == 3U);
}

int main(void)
{
    test_valid_statuses();
    test_release_and_hold_without_contact();
    test_invalid_readings();
    test_callback_registration();

    if (failures != 0U)
    {
        fprintf(stderr, "%u/%u checks failed\n", failures, checks);
        return 1;
    }

    printf("%u checks passed\n", checks);
    return 0;
}
