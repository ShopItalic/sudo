#include "bc_touch_tuning.h"
#include "IQS7211E.h"
#include "IQS7211E_init_1232.h"

#include <stdio.h>
#include <string.h>

static unsigned checks;
static unsigned failures;

static void check_condition(bool condition, const char *expression,
                            unsigned line)
{
    ++checks;
    if (!condition) {
        ++failures;
        fprintf(stderr, "FAIL line %u: %s\n", line, expression);
    }
}

#define CHECK(condition) check_condition((condition), #condition, __LINE__)

unsigned test_touch_tuning_critical_depth;
unsigned test_touch_tuning_critical_enters;
unsigned test_touch_tuning_critical_exits;

void test_touch_tuning_enter(void)
{
    ++test_touch_tuning_critical_enters;
    ++test_touch_tuning_critical_depth;
}

void test_touch_tuning_exit(void)
{
    ++test_touch_tuning_critical_exits;
    CHECK(test_touch_tuning_critical_depth != 0U);
    if (test_touch_tuning_critical_depth != 0U)
        --test_touch_tuning_critical_depth;
}

typedef struct {
    uint8_t touch_thresholds[2];
    uint8_t gesture_mask[2];
    uint8_t operation_reg[16];
    uint8_t operation_kind[16];
    uint8_t operation_count;
    unsigned write_calls;
    unsigned read_calls;
    unsigned fail_write_call;
    unsigned fail_read_call;
    uint8_t mismatch_reg;
    bool inject_request;
    uint8_t inject_request_phase;
    bool injected_request;
    bool inject_reset;
    bool injected_reset;
    unsigned callback_critical_violations;
} tuning_fixture;

typedef struct {
    bool opened;
    uint8_t status[BC_TOUCH_REPORT_STATUS_LENGTH];
    uint8_t touch_thresholds[2];
    uint8_t gesture_mask[2];
    uint8_t write_reg[128];
    uint8_t write_length[128];
    uint8_t write_data[128][22];
    unsigned write_count;
    unsigned open_count;
    unsigned close_count;
} iqs_fixture;

static iqs_fixture iqs_bus;

int touch_i2c_open(void)
{
    ++iqs_bus.open_count;
    iqs_bus.opened = true;
    return 0;
}

int touch_i2c_close(void)
{
    ++iqs_bus.close_count;
    iqs_bus.opened = false;
    return 0;
}

bool touch_i2c_write(uint8_t reg, uint8_t *data, uint8_t length)
{
    if (!iqs_bus.opened || length > sizeof(iqs_bus.write_data[0]))
        return false;
    if (iqs_bus.write_count < sizeof(iqs_bus.write_reg)) {
        iqs_bus.write_reg[iqs_bus.write_count] = reg;
        iqs_bus.write_length[iqs_bus.write_count] = length;
        if (length != 0U)
            memcpy(iqs_bus.write_data[iqs_bus.write_count], data, length);
        ++iqs_bus.write_count;
    }

    if (reg == BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG && length == 2U) {
        memcpy(iqs_bus.touch_thresholds, data, 2U);
    } else if (reg == 0x36U && length >= 6U) {
        iqs_bus.touch_thresholds[0] = data[4];
        iqs_bus.touch_thresholds[1] = data[5];
    } else if (reg == BC_TOUCH_TUNING_GESTURE_ENABLE_REG && length >= 2U) {
        iqs_bus.gesture_mask[0] = data[0];
        iqs_bus.gesture_mask[1] = data[1];
    }
    return true;
}

bool touch_i2c_read(uint8_t reg, uint8_t *data, uint8_t length)
{
    if (!iqs_bus.opened)
        return false;
    if (reg == 0x0EU && length == sizeof(iqs_bus.status)) {
        memcpy(data, iqs_bus.status, length);
        return true;
    }
    if (reg == BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG && length == 2U) {
        memcpy(data, iqs_bus.touch_thresholds, 2U);
        return true;
    }
    if (reg == BC_TOUCH_TUNING_GESTURE_ENABLE_REG && length == 2U) {
        memcpy(data, iqs_bus.gesture_mask, 2U);
        return true;
    }
    memset(data, 0, length);
    return true;
}

uint8_t touch_io_irq_status(void)
{
    return 0U;
}

static void fixture_reset(tuning_fixture *fixture)
{
    memset(fixture, 0, sizeof(*fixture));
    fixture->touch_thresholds[0] = BC_TOUCH_TUNING_DEFAULT_SET;
    fixture->touch_thresholds[1] = BC_TOUCH_TUNING_DEFAULT_CLEAR;
    fixture->gesture_mask[0] = (uint8_t)(BC_TOUCH_TUNING_DEFAULT_GESTURE_MASK &
                                         0xFFU);
    fixture->gesture_mask[1] = (uint8_t)(BC_TOUCH_TUNING_DEFAULT_GESTURE_MASK >>
                                         8);
}

static bool fixture_write(void *ctx, uint8_t reg, uint8_t *data,
                          uint8_t length)
{
    tuning_fixture *fixture = (tuning_fixture *)ctx;

    if (test_touch_tuning_critical_depth != 0U)
        ++fixture->callback_critical_violations;
    if (fixture->operation_count < sizeof(fixture->operation_reg)) {
        fixture->operation_reg[fixture->operation_count] = reg;
        fixture->operation_kind[fixture->operation_count] = 1U;
        ++fixture->operation_count;
    }
    ++fixture->write_calls;
    if (length != 2U)
        return false;

    if (fixture->inject_request && fixture->inject_request_phase != 2U &&
        !fixture->injected_request &&
        reg == BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG) {
        fixture->injected_request = true;
        CHECK(bc_touch_tuning_request(64U, 60U, false));
    }
    if (fixture->inject_reset && !fixture->injected_reset &&
        reg == BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG) {
        fixture->injected_reset = true;
        bc_touch_tuning_sensor_reset();
    }
    if (fixture->fail_write_call != 0U &&
        fixture->write_calls == fixture->fail_write_call)
        return false;

    if (reg == BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG)
        memcpy(fixture->touch_thresholds, data, 2U);
    else if (reg == BC_TOUCH_TUNING_GESTURE_ENABLE_REG)
        memcpy(fixture->gesture_mask, data, 2U);
    else
        return false;
    return true;
}

static bool fixture_read(void *ctx, uint8_t reg, uint8_t *data,
                         uint8_t length)
{
    tuning_fixture *fixture = (tuning_fixture *)ctx;

    if (test_touch_tuning_critical_depth != 0U)
        ++fixture->callback_critical_violations;
    if (fixture->operation_count < sizeof(fixture->operation_reg)) {
        fixture->operation_reg[fixture->operation_count] = reg;
        fixture->operation_kind[fixture->operation_count] = 2U;
        ++fixture->operation_count;
    }
    ++fixture->read_calls;
    if (length != 2U ||
        (fixture->fail_read_call != 0U &&
         fixture->read_calls == fixture->fail_read_call))
        return false;

    if (fixture->inject_request && fixture->inject_request_phase == 2U &&
        !fixture->injected_request && reg == BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG) {
        fixture->injected_request = true;
        CHECK(bc_touch_tuning_request(64U, 60U, false));
    }

    if (reg == BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG)
        memcpy(data, fixture->touch_thresholds, 2U);
    else if (reg == BC_TOUCH_TUNING_GESTURE_ENABLE_REG)
        memcpy(data, fixture->gesture_mask, 2U);
    else
        return false;

    if (fixture->mismatch_reg == reg)
        data[0] ^= 1U;
    return true;
}

static bc_touch_tuning_snapshot snapshot(void)
{
    bc_touch_tuning_snapshot result;

    memset(&result, 0, sizeof(result));
    CHECK(bc_touch_tuning_snapshot_get(&result));
    return result;
}

static void apply_safe(tuning_fixture *fixture)
{
    bc_touch_tuning_on_sample(true, false, false, fixture_write,
                              fixture_read, fixture);
}

static void test_defaults_and_profile_mask(void)
{
    bc_touch_tuning_snapshot current;

    CHECK(GESTURE_ENABLE_0 == 0x0AU);
    CHECK(GESTURE_ENABLE_1 == 0x00U);
    CHECK(BC_TOUCH_TUNING_DEFAULT_GESTURE_MASK == 0x000AU);

    bc_touch_tuning_init((uint16_t)GESTURE_ENABLE_0 |
                         ((uint16_t)GESTURE_ENABLE_1 << 8));
    current = snapshot();
    CHECK(current.touch_set == 54U);
    CHECK(current.touch_clear == 52U);
    CHECK(current.memo_enabled);
    CHECK(current.status == BC_TOUCH_TUNING_APPLIED);
    CHECK(current.generation != 0U);
}

static void test_request_boundaries(void)
{
    bc_touch_tuning_snapshot before;
    bc_touch_tuning_snapshot after;

    before = snapshot();
    CHECK(!bc_touch_tuning_request(31U, 30U, true));
    CHECK(!bc_touch_tuning_request(81U, 79U, true));
    CHECK(!bc_touch_tuning_request(54U, 29U, true));
    CHECK(!bc_touch_tuning_request(54U, 54U, true));
    CHECK(!bc_touch_tuning_request(32U, 31U, true));
    after = snapshot();
    CHECK(after.touch_set == before.touch_set);
    CHECK(after.touch_clear == before.touch_clear);
    CHECK(after.memo_enabled == before.memo_enabled);
    CHECK(after.status == before.status);

    CHECK(bc_touch_tuning_request(32U, 30U, true));
    CHECK(bc_touch_tuning_request(80U, 78U, true));
    CHECK(bc_touch_tuning_request(54U, 52U, true));
}

static void test_release_only_apply_and_mask(void)
{
    tuning_fixture fixture;
    bc_touch_tuning_snapshot current;

    fixture_reset(&fixture);
    CHECK(bc_touch_tuning_request(60U, 57U, true));
    bc_touch_tuning_on_sample(true, true, false, fixture_write, fixture_read,
                              &fixture);
    CHECK(fixture.operation_count == 0U);
    current = snapshot();
    CHECK(current.status == BC_TOUCH_TUNING_PENDING);

    bc_touch_tuning_on_sample(false, false, false, fixture_write, fixture_read,
                              &fixture);
    CHECK(fixture.operation_count == 0U);
    apply_safe(&fixture);
    CHECK(fixture.operation_count == 4U);
    CHECK(fixture.operation_reg[0] == BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG);
    CHECK(fixture.operation_kind[0] == 1U);
    CHECK(fixture.operation_reg[1] == BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG);
    CHECK(fixture.operation_kind[1] == 2U);
    CHECK(fixture.operation_reg[2] == BC_TOUCH_TUNING_GESTURE_ENABLE_REG);
    CHECK(fixture.operation_kind[2] == 1U);
    CHECK(fixture.operation_reg[3] == BC_TOUCH_TUNING_GESTURE_ENABLE_REG);
    CHECK(fixture.operation_kind[3] == 2U);
    CHECK(fixture.touch_thresholds[0] == 60U);
    CHECK(fixture.touch_thresholds[1] == 57U);
    CHECK(fixture.gesture_mask[0] == 0x0AU);
    CHECK(fixture.gesture_mask[1] == 0x00U);
    current = snapshot();
    CHECK(current.status == BC_TOUCH_TUNING_APPLIED);
    CHECK(fixture.callback_critical_violations == 0U);

    fixture_reset(&fixture);
    CHECK(bc_touch_tuning_request(60U, 57U, false));
    apply_safe(&fixture);
    CHECK(fixture.gesture_mask[0] == BC_TOUCH_TUNING_GESTURE_HOLD);
    CHECK(fixture.gesture_mask[1] == 0U);
    CHECK(snapshot().status == BC_TOUCH_TUNING_APPLIED);
}

static void test_io_failures_retry(void)
{
    tuning_fixture fixture;
    bc_touch_tuning_snapshot current;

    fixture_reset(&fixture);
    CHECK(bc_touch_tuning_request(65U, 62U, true));
    fixture.fail_write_call = 1U;
    apply_safe(&fixture);
    current = snapshot();
    CHECK(current.status == BC_TOUCH_TUNING_IO_ERROR);
    CHECK(fixture.read_calls == 0U);

    fixture_reset(&fixture);
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_APPLIED);

    fixture_reset(&fixture);
    CHECK(bc_touch_tuning_request(66U, 63U, true));
    fixture.fail_read_call = 1U;
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_IO_ERROR);
    fixture_reset(&fixture);
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_APPLIED);

    fixture_reset(&fixture);
    CHECK(bc_touch_tuning_request(67U, 64U, true));
    fixture.mismatch_reg = BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG;
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_IO_ERROR);
    fixture_reset(&fixture);
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_APPLIED);

    fixture_reset(&fixture);
    CHECK(bc_touch_tuning_request(68U, 65U, true));
    fixture.mismatch_reg = BC_TOUCH_TUNING_GESTURE_ENABLE_REG;
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_IO_ERROR);
    fixture_reset(&fixture);
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_APPLIED);

    fixture_reset(&fixture);
    CHECK(bc_touch_tuning_request(69U, 66U, true));
    fixture.fail_write_call = 2U;
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_IO_ERROR);
    fixture_reset(&fixture);
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_APPLIED);

    fixture_reset(&fixture);
    CHECK(bc_touch_tuning_request(70U, 67U, true));
    fixture.fail_read_call = 2U;
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_IO_ERROR);
    fixture_reset(&fixture);
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_APPLIED);
}

static void test_reset_and_newer_request_during_io(void)
{
    tuning_fixture fixture;
    bc_touch_tuning_snapshot current;

    fixture_reset(&fixture);
    CHECK(bc_touch_tuning_request(71U, 68U, true));
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_APPLIED);

    bc_touch_tuning_sensor_reset();
    current = snapshot();
    CHECK(current.status == BC_TOUCH_TUNING_PENDING);
    fixture_reset(&fixture);
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_APPLIED);

    CHECK(bc_touch_tuning_request(72U, 69U, true));
    fixture_reset(&fixture);
    fixture.inject_request = true;
    apply_safe(&fixture);
    current = snapshot();
    CHECK(current.status == BC_TOUCH_TUNING_PENDING);
    CHECK(current.touch_set == 64U);
    CHECK(current.touch_clear == 60U);
    CHECK(!current.memo_enabled);
    CHECK(fixture.injected_request);

    fixture_reset(&fixture);
    apply_safe(&fixture);
    current = snapshot();
    CHECK(current.status == BC_TOUCH_TUNING_APPLIED);
    CHECK(current.touch_set == 64U);
    CHECK(current.touch_clear == 60U);
    CHECK(!current.memo_enabled);
    CHECK(fixture.touch_thresholds[0] == 64U);
    CHECK(fixture.touch_thresholds[1] == 60U);
    CHECK(fixture.gesture_mask[0] == BC_TOUCH_TUNING_GESTURE_HOLD);
    CHECK(fixture.gesture_mask[1] == 0U);

    CHECK(bc_touch_tuning_request(75U, 72U, true));
    fixture_reset(&fixture);
    fixture.inject_request = true;
    fixture.inject_request_phase = 2U;
    apply_safe(&fixture);
    current = snapshot();
    CHECK(current.status == BC_TOUCH_TUNING_PENDING);
    CHECK(current.touch_set == 64U);
    CHECK(current.touch_clear == 60U);
    CHECK(!current.memo_enabled);
    CHECK(fixture.injected_request);
    fixture_reset(&fixture);
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_APPLIED);

    CHECK(bc_touch_tuning_request(73U, 70U, true));
    fixture_reset(&fixture);
    fixture.inject_reset = true;
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_PENDING);
    fixture_reset(&fixture);
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_APPLIED);
}

static void test_invalid_and_null_samples(void)
{
    tuning_fixture fixture;

    fixture_reset(&fixture);
    CHECK(bc_touch_tuning_request(74U, 71U, true));
    bc_touch_tuning_on_sample(true, false, true, fixture_write, fixture_read,
                              &fixture);
    CHECK(fixture.operation_count == 0U);
    CHECK(snapshot().status == BC_TOUCH_TUNING_PENDING);
    bc_touch_tuning_on_sample(true, false, false, 0, fixture_read, &fixture);
    bc_touch_tuning_on_sample(true, false, false, fixture_write, 0, &fixture);
    bc_touch_tuning_on_sample(true, true, false, fixture_write, fixture_read,
                              &fixture);
    bc_touch_tuning_on_sample(false, false, false, fixture_write, fixture_read,
                              &fixture);
    CHECK(fixture.operation_count == 0U);
    apply_safe(&fixture);
    CHECK(snapshot().status == BC_TOUCH_TUNING_APPLIED);
}

static void iqs_set_coordinates(uint16_t x, uint16_t y)
{
    iqs_bus.status[4] = (uint8_t)(x & 0xFFU);
    iqs_bus.status[5] = (uint8_t)(x >> 8);
    iqs_bus.status[6] = (uint8_t)(y & 0xFFU);
    iqs_bus.status[7] = (uint8_t)(y >> 8);
}

static unsigned iqs_dynamic_write_count(void)
{
    unsigned i;
    unsigned count = 0U;

    for (i = 0U; i < iqs_bus.write_count; ++i) {
        if ((iqs_bus.write_reg[i] == BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG ||
             iqs_bus.write_reg[i] == BC_TOUCH_TUNING_GESTURE_ENABLE_REG) &&
            iqs_bus.write_length[i] == 2U)
            ++count;
    }
    return count;
}

static void test_iqs_event_window_integration(void)
{
    unsigned dynamic_before;

    memset(&iqs_bus, 0, sizeof(iqs_bus));
    iqs_set_coordinates(0xFFFFU, 0xFFFFU);
    CHECK(touch_i2c_open() == 0);
    IQS7211E_Init();
    CHECK(touch_i2c_close() == 0);
    CHECK(iqs_bus.opened == false);
    CHECK(iqs_bus.touch_thresholds[0] == TRACKPAD_TOUCH_SET_THRESHOLD);
    CHECK(iqs_bus.touch_thresholds[1] == TRACKPAD_TOUCH_CLEAR_THRESHOLD);
    CHECK(iqs_bus.gesture_mask[0] == 0x0AU);
    CHECK(iqs_bus.gesture_mask[1] == 0x00U);

    CHECK(bc_touch_tuning_request(75U, 72U, false));
    dynamic_before = iqs_dynamic_write_count();
    iqs_set_coordinates(1U, 2U);
    iqs_bus.status[2] = 0U;
    Process_IQS7211E_Events();
    CHECK(iqs_dynamic_write_count() == dynamic_before);
    CHECK(iqs_bus.open_count == iqs_bus.close_count);

    iqs_bus.status[2] = BC_TOUCH_REPORT_INFO_ATI_ERROR;
    iqs_set_coordinates(0xFFFFU, 0xFFFFU);
    Process_IQS7211E_Events();
    CHECK(iqs_dynamic_write_count() == dynamic_before);
    CHECK(bc_touch_tuning_status_get() == BC_TOUCH_TUNING_PENDING);

    iqs_bus.status[2] = 0U;
    Process_IQS7211E_Events();
    CHECK(iqs_dynamic_write_count() == dynamic_before + 2U);
    CHECK(iqs_bus.touch_thresholds[0] == 75U);
    CHECK(iqs_bus.touch_thresholds[1] == 72U);
    CHECK(iqs_bus.gesture_mask[0] == BC_TOUCH_TUNING_GESTURE_HOLD);
    CHECK(iqs_bus.gesture_mask[1] == 0U);
    CHECK(bc_touch_tuning_status_get() == BC_TOUCH_TUNING_APPLIED);

    CHECK(bc_touch_tuning_request(76U, 73U, true));
    iqs_bus.status[2] = BC_TOUCH_REPORT_INFO_RESET;
    iqs_set_coordinates(0xFFFFU, 0xFFFFU);
    dynamic_before = iqs_dynamic_write_count();
    Process_IQS7211E_Events();
    CHECK(iqs_dynamic_write_count() == dynamic_before);
    CHECK(bc_touch_tuning_status_get() == BC_TOUCH_TUNING_PENDING);

    iqs_bus.status[2] = 0U;
    Process_IQS7211E_Events();
    CHECK(iqs_dynamic_write_count() == dynamic_before + 2U);
    CHECK(iqs_bus.touch_thresholds[0] == 76U);
    CHECK(iqs_bus.touch_thresholds[1] == 73U);
    CHECK(iqs_bus.gesture_mask[0] == 0x0AU);
    CHECK(iqs_bus.gesture_mask[1] == 0x00U);
    CHECK(iqs_bus.opened == false);
}

int main(void)
{
    test_defaults_and_profile_mask();
    test_request_boundaries();
    test_release_only_apply_and_mask();
    test_io_failures_retry();
    test_reset_and_newer_request_during_io();
    test_invalid_and_null_samples();
    test_iqs_event_window_integration();

    CHECK(test_touch_tuning_critical_depth == 0U);
    CHECK(test_touch_tuning_critical_enters ==
          test_touch_tuning_critical_exits);
    if (failures != 0U) {
        fprintf(stderr, "%u/%u checks failed\n", failures, checks);
        return 1;
    }
    printf("%u checks passed\n", checks);
    return 0;
}
