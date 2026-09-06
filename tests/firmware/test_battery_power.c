#ifndef SUDO_VOICE_ONLY
#define SUDO_VOICE_ONLY 1
#endif
#ifndef HANDWARE_1_23_2
#define HANDWARE_1_23_2 1
#endif

#include "q_device.h"
#include "bc_linear_motor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned checks;
static unsigned failures;

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

static struct q_device adc_device = {"vbat_adc"};
static int mock_open_result;
static int mock_close_result;
static int mock_read_results[3];
static uint16_t mock_samples[3];
static unsigned mock_open_calls;
static unsigned mock_close_calls;
static unsigned mock_read_calls;
static unsigned mock_power_on_calls;
static unsigned mock_power_off_calls;
static unsigned mock_delay_calls;
static unsigned mock_power_manage_calls;
static unsigned mock_callback_calls;
static unsigned mock_critical_depth;
static int mock_charge_status;
static bc_linear_motor_activity_t mock_motor_activity;
static int mock_activity_start_read;
static int mock_activity_stop_read;
uint32_t test_ticks;

void bc_linear_motor_activity_get(bc_linear_motor_activity_t *activity)
{
    if (activity != NULL)
        *activity = mock_motor_activity;
}

q_device_t *q_device_find(const char *name)
{
    CHECK(name != NULL);
    CHECK(strcmp(name, "vbat_adc") == 0);
    return &adc_device;
}

void q_device_assert(q_device_t *device)
{
    CHECK(device == &adc_device);
}

int q_device_open(q_device_t *device)
{
    CHECK(device == &adc_device);
    ++mock_open_calls;
    return mock_open_result;
}

int q_device_close(q_device_t *device)
{
    CHECK(device == &adc_device);
    ++mock_close_calls;
    return mock_close_result;
}

int q_device_read(q_device_t *device, int position, const void *buffer,
                  int size)
{
    uint16_t *sample = (uint16_t *)buffer;
    unsigned index = mock_read_calls;

    CHECK(device == &adc_device);
    CHECK(position == 0);
    CHECK(size == 1);
    CHECK(buffer != NULL);
    ++mock_read_calls;
    if ((int)index == mock_activity_start_read)
    {
        ++mock_motor_activity.generation;
        mock_motor_activity.active = true;
    }
    if ((int)index == mock_activity_stop_read)
    {
        ++mock_motor_activity.generation;
        mock_motor_activity.active = false;
        mock_motor_activity.has_finished = true;
        mock_motor_activity.last_finished_tick = test_ticks;
    }
    if (index >= 3U)
        return RESULT_READ_ERR;
    if (mock_read_results[index] == RESULT_OK)
        *sample = mock_samples[index];
    return mock_read_results[index];
}

void bc_delay_ms(uint32_t milliseconds)
{
    CHECK(milliseconds == 10U);
    ++mock_delay_calls;
}

void bc_ldo_bat_power_on(void)
{
    ++mock_power_on_calls;
}

void bc_ldo_bat_power_off(void)
{
    ++mock_power_off_calls;
}

void test_task_enter(void)
{
    ++mock_critical_depth;
}

void test_task_exit(void)
{
    CHECK(mock_critical_depth > 0U);
    --mock_critical_depth;
}

void power_manage(void)
{
    ++mock_power_manage_calls;
}

static void test_hardware_error_callback(void)
{
    ++mock_callback_calls;
}

/* Include the production implementation so every assertion below exercises
 * its real cleanup, ownership and percentage-error paths. */
#include "../../firmware/bc_ros/bc_module/pmic/bc_power.c"

enum pmic_charge_status bc_pmic_get_charge_status(void)
{
    return (enum pmic_charge_status)mock_charge_status;
}

static void reset_mocks(void)
{
    unsigned i;

    mock_open_result = RESULT_OK;
    mock_close_result = RESULT_OK;
    mock_open_calls = 0U;
    mock_close_calls = 0U;
    mock_read_calls = 0U;
    mock_power_on_calls = 0U;
    mock_power_off_calls = 0U;
    mock_delay_calls = 0U;
    mock_power_manage_calls = 0U;
    mock_critical_depth = 0U;
    mock_charge_status = 0;
    mock_motor_activity.generation = 0U;
    mock_motor_activity.active_since_tick = 0U;
    mock_motor_activity.last_finished_tick = 0U;
    mock_motor_activity.active = false;
    mock_motor_activity.has_finished = false;
    mock_activity_start_read = -1;
    mock_activity_stop_read = -1;
    test_ticks = 0U;
    for (i = 0U; i < 3U; ++i)
    {
        mock_read_results[i] = RESULT_OK;
        mock_samples[i] = 0U;
    }
}

static void test_checked_adc_success_and_bounds(void)
{
    reset_mocks();
    mock_samples[0] = 100U;
    mock_samples[1] = 200U;
    mock_samples[2] = 300U;
    CHECK(bc_power_get_adc_value() == 200U);
    CHECK(mock_open_calls == 1U);
    CHECK(mock_read_calls == 3U);
    CHECK(mock_close_calls == 1U);
    CHECK(mock_power_on_calls == 1U);
    CHECK(mock_power_off_calls == 1U);
    CHECK(mock_critical_depth == 0U);

    reset_mocks();
    mock_samples[0] = 4095U;
    mock_samples[1] = 4095U;
    mock_samples[2] = 4095U;
    CHECK(bc_power_get_adc_value() == 4095U);

    reset_mocks();
    mock_samples[1] = 4096U;
    CHECK(bc_power_get_adc_value() == UINT16_MAX);
    CHECK(mock_read_calls == 2U);
    CHECK(mock_close_calls == 1U);
    CHECK(mock_power_on_calls == 1U);
    CHECK(mock_power_off_calls == 1U);
}

static void test_checked_adc_failures_release_owned_resources(void)
{
    reset_mocks();
    mock_open_result = RESULT_OPEN_ERR;
    CHECK(bc_power_get_adc_value() == UINT16_MAX);
    CHECK(mock_open_calls == 1U);
    CHECK(mock_close_calls == 0U);
    CHECK(mock_power_on_calls == 1U);
    CHECK(mock_power_off_calls == 1U);

    reset_mocks();
    mock_read_results[1] = RESULT_READ_ERR;
    mock_samples[0] = 123U;
    CHECK(bc_power_get_adc_value() == UINT16_MAX);
    CHECK(mock_read_calls == 2U);
    CHECK(mock_close_calls == 1U);
    CHECK(mock_power_on_calls == 1U);
    CHECK(mock_power_off_calls == 1U);

    reset_mocks();
    mock_close_result = RESULT_READ_ERR;
    CHECK(bc_power_get_adc_value() == UINT16_MAX);
    CHECK(mock_close_calls == 1U);
    CHECK(mock_power_off_calls == 1U);

    /* A failed transaction must release the nonblocking ownership token. */
    reset_mocks();
    mock_samples[0] = 10U;
    mock_samples[1] = 20U;
    mock_samples[2] = 30U;
    CHECK(bc_power_get_adc_value() == 20U);
    CHECK(mock_power_on_calls == 1U);
    CHECK(mock_power_off_calls == 1U);
}

static void test_contention_does_not_touch_power(void)
{
    reset_mocks();
    vbat_adc_transaction_active = true;
    CHECK(bc_power_get_adc_value() == UINT16_MAX);
    CHECK(!bc_power_check_vbat());
    CHECK(mock_open_calls == 0U);
    CHECK(mock_close_calls == 0U);
    CHECK(mock_power_on_calls == 0U);
    CHECK(mock_power_off_calls == 0U);
    vbat_adc_transaction_active = false;
}

static void test_motor_activity_exclusion_and_generation(void)
{
    reset_mocks();
    mock_motor_activity.generation = 1U;
    mock_motor_activity.active = true;
    CHECK(bc_power_get_adc_value() == UINT16_MAX);
    CHECK(mock_open_calls == 0U && mock_power_on_calls == 0U);

    reset_mocks();
    mock_motor_activity.generation = 2U;
    mock_motor_activity.has_finished = true;
    mock_motor_activity.last_finished_tick = 1000U;
    test_ticks = 1249U;
    CHECK(bc_power_get_adc_value() == UINT16_MAX);
    CHECK(mock_open_calls == 0U && mock_power_on_calls == 0U);

    reset_mocks();
    mock_motor_activity.generation = 3U;
    mock_motor_activity.has_finished = true;
    mock_motor_activity.last_finished_tick = 1000U;
    test_ticks = 1250U;
    mock_samples[0] = 100U;
    mock_samples[1] = 200U;
    mock_samples[2] = 300U;
    CHECK(bc_power_get_adc_value() == 200U);
    CHECK(mock_open_calls == 1U && mock_power_on_calls == 1U);

    reset_mocks();
    mock_motor_activity.generation = 4U;
    mock_motor_activity.has_finished = true;
    mock_motor_activity.last_finished_tick = UINT32_MAX - 100U;
    test_ticks = 148U;
    CHECK(bc_power_get_adc_value() == UINT16_MAX);
    test_ticks = 149U;
    mock_samples[0] = 100U;
    mock_samples[1] = 200U;
    mock_samples[2] = 300U;
    CHECK(bc_power_get_adc_value() == 200U);

    reset_mocks();
    mock_activity_start_read = 1;
    mock_activity_stop_read = 2;
    mock_samples[0] = 100U;
    mock_samples[1] = 200U;
    mock_samples[2] = 300U;
    CHECK(bc_power_get_adc_value() == UINT16_MAX);
    CHECK(mock_open_calls == 1U && mock_close_calls == 1U &&
          mock_power_on_calls == 1U && mock_power_off_calls == 1U);

    /* The rejected transaction still releases ownership for the next read. */
    reset_mocks();
    mock_samples[0] = 10U;
    mock_samples[1] = 20U;
    mock_samples[2] = 30U;
    CHECK(bc_power_get_adc_value() == 20U);
}

static void test_check_vbat_cleanup_and_callback(void)
{
    union {
        const void *object;
        void (*function)(void);
    } callback;

    reset_mocks();
    callback.function = test_hardware_error_callback;
    CHECK(bc_power_adc_hardware_error_register_callback(
              callback.object));

    mock_samples[0] = 99U;
    CHECK(!bc_power_check_vbat());
    CHECK(mock_open_calls == 1U);
    CHECK(mock_read_calls == 1U);
    CHECK(mock_close_calls == 1U);
    CHECK(mock_power_on_calls == 1U);
    CHECK(mock_power_off_calls == 1U);
    CHECK(mock_callback_calls == 1U);

    reset_mocks();
    mock_read_results[0] = RESULT_READ_ERR;
    CHECK(!bc_power_check_vbat());
    CHECK(mock_close_calls == 1U);
    CHECK(mock_power_off_calls == 1U);
    CHECK(mock_callback_calls == 1U);

    reset_mocks();
    mock_open_result = RESULT_OPEN_ERR;
    CHECK(!bc_power_check_vbat());
    CHECK(mock_close_calls == 0U);
    CHECK(mock_power_off_calls == 1U);

    reset_mocks();
    mock_samples[0] = 100U;
    CHECK(bc_power_check_vbat());
    CHECK(mock_close_calls == 1U);
    CHECK(mock_power_off_calls == 1U);
}

static void test_percentage_failure_is_unknown(void)
{
    reset_mocks();
    mock_open_result = RESULT_OPEN_ERR;
    CHECK(bc_power_get_vbat_percen() == BC_POWER_PERCENT_UNKNOWN);
    CHECK(mock_power_manage_calls == 0U);
}

int main(void)
{
    bc_power_vbat_adc_find();
    test_checked_adc_success_and_bounds();
    test_checked_adc_failures_release_owned_resources();
    test_contention_does_not_touch_power();
    test_motor_activity_exclusion_and_generation();
    test_check_vbat_cleanup_and_callback();
    test_percentage_failure_is_unknown();

    if (failures != 0U)
    {
        fprintf(stderr, "%u/%u checks failed\n", failures, checks);
        return 1;
    }

    printf("battery power: %u checks passed\n", checks);
    return 0;
}
