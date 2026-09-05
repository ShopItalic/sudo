#include "app_pmic_handler.h"
#include "bc_battery_filter.h"
#include "bc_pmic.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static unsigned checks;
static unsigned failures;
static enum pmic_charge_status mock_status;
static const uint8_t *mock_samples;
static unsigned mock_sample_count;
static unsigned mock_sample_index;
static unsigned mock_status_calls;
static unsigned mock_percent_calls;
static unsigned critical_depth;
static unsigned critical_enters;
static unsigned critical_exits;
static unsigned status_calls_in_critical;
static bool reentrant_trigger;
static bool reentrant_triggered;
static bool reentrant_active;
static uint8_t reentrant_outer_sample;
static uint8_t reentrant_sample;
static uint8_t reentrant_result;
static unsigned status_trigger_on_call;
static enum pmic_charge_status reentrant_nested_status;
static enum pmic_charge_status reentrant_restore_status;
static unsigned reentrant_status_delta;
static unsigned reentrant_percent_delta;

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

void test_task_enter(void)
{
    ++critical_depth;
    ++critical_enters;
}

void test_task_exit(void)
{
    CHECK(critical_depth > 0U);
    --critical_depth;
    ++critical_exits;
}

enum pmic_charge_status bc_pmic_get_charge_status(void)
{
    unsigned status_calls_before_nested;
    unsigned percent_calls_before_nested;

    ++mock_status_calls;
    if (critical_depth != 0U)
        ++status_calls_in_critical;

    if (status_trigger_on_call != 0U &&
        mock_status_calls == status_trigger_on_call)
    {
        status_trigger_on_call = 0U;
        status_calls_before_nested = mock_status_calls;
        percent_calls_before_nested = mock_percent_calls;
        mock_status = reentrant_nested_status;
        reentrant_active = true;
        reentrant_result = getvpct();
        reentrant_active = false;
        mock_status = reentrant_restore_status;
        reentrant_status_delta = mock_status_calls - status_calls_before_nested;
        reentrant_percent_delta = mock_percent_calls - percent_calls_before_nested;
    }

    return mock_status;
}

uint8_t bc_pmic_get_vbat_percen(void)
{
    ++mock_percent_calls;

    if (reentrant_active)
        return reentrant_sample;

    if (reentrant_trigger && !reentrant_triggered)
    {
        unsigned status_calls_before_nested;
        unsigned percent_calls_before_nested;

        reentrant_triggered = true;
        mock_status = PMIC_CHARGED_ING;
        status_calls_before_nested = mock_status_calls;
        percent_calls_before_nested = mock_percent_calls;
        reentrant_active = true;
        reentrant_result = getvpct();
        reentrant_active = false;
        reentrant_status_delta = mock_status_calls - status_calls_before_nested;
        reentrant_percent_delta = mock_percent_calls - percent_calls_before_nested;
        return reentrant_outer_sample;
    }

    CHECK(mock_sample_index < mock_sample_count);
    if (mock_sample_index >= mock_sample_count)
        return BC_BATTERY_PERCENT_UNKNOWN;
    return mock_samples[mock_sample_index++];
}

static uint8_t next_percent(enum pmic_charge_status status, uint8_t sample)
{
    static uint8_t values[1];

    mock_status = status;
    values[0] = sample;
    mock_samples = values;
    mock_sample_count = 1U;
    mock_sample_index = 0U;
    return getvpct();
}

int main(void)
{
    CHECK(next_percent(PMIC_CHARGED_NOT, 0U) == 0U);
    CHECK(next_percent(PMIC_CHARGED_NOT, 100U) == 0U);
    CHECK(next_percent(PMIC_CHARGED_NOT, 80U) == 0U);

    /* Charging starts a new monotonic epoch and permits the real zero
     * percentage to remain initialized. */
    CHECK(next_percent(PMIC_CHARGED_ING, 20U) == 20U);
    CHECK(next_percent(PMIC_CHARGED_ING, 0U) == 20U);

    /* OVER is a distinct captured PMIC state, so it starts its own epoch. */
    CHECK(next_percent(PMIC_CHARGED_OVER, 40U) == 40U);
    CHECK(next_percent(PMIC_CHARGED_OVER, BC_BATTERY_PERCENT_UNKNOWN) ==
          BC_BATTERY_PERCENT_UNKNOWN);
    CHECK(next_percent(PMIC_CHARGED_NOT, 10U) == 10U);

    /* The outer caller starts in the discharging epoch.  Its ADC callback
     * changes charge state and re-enters getvpct().  Single-flight ownership
     * rejects the nested call before it touches PMIC or ADC; the outer sample
     * then spans the phase change and is discarded. */
    mock_status = PMIC_CHARGED_NOT;
    reentrant_outer_sample = 30U;
    reentrant_sample = 70U;
    reentrant_trigger = true;
    reentrant_triggered = false;
    reentrant_result = BC_BATTERY_PERCENT_UNKNOWN;
    CHECK(getvpct() == BC_BATTERY_PERCENT_UNKNOWN);
    CHECK(reentrant_result == BC_BATTERY_PERCENT_UNKNOWN);
    CHECK(reentrant_status_delta == 0U);
    CHECK(reentrant_percent_delta == 0U);
    CHECK(next_percent(PMIC_CHARGED_ING, 60U) == 60U);

    /* A nested caller at the second status snapshot is also rejected.  The
     * outer caller keeps the existing discharging history, even though the
     * rejected call briefly reports a different phase. */
    reentrant_trigger = false;
    mock_status = PMIC_CHARGED_NOT;
    CHECK(next_percent(PMIC_CHARGED_NOT, 80U) == 80U);
    reentrant_nested_status = PMIC_CHARGED_ING;
    reentrant_restore_status = PMIC_CHARGED_NOT;
    reentrant_sample = 20U;
    reentrant_result = BC_BATTERY_PERCENT_UNKNOWN;
    reentrant_status_delta = 1U;
    reentrant_percent_delta = 1U;
    status_trigger_on_call = mock_status_calls + 2U;
    CHECK(next_percent(PMIC_CHARGED_NOT, 90U) == 80U);
    CHECK(reentrant_result == BC_BATTERY_PERCENT_UNKNOWN);
    CHECK(reentrant_status_delta == 0U);
    CHECK(reentrant_percent_delta == 0U);
    CHECK(next_percent(PMIC_CHARGED_NOT, 95U) == 80U);

    CHECK(mock_status_calls == 26U);
    CHECK(mock_percent_calls == 13U);
    CHECK(critical_depth == 0U);
    CHECK(critical_enters == 28U);
    CHECK(critical_exits == 28U);
    CHECK(status_calls_in_critical == 0U);

    if (failures != 0U)
    {
        fprintf(stderr, "%u/%u checks failed\n", failures, checks);
        return 1;
    }

    printf("battery app: %u checks passed\n", checks);
    return 0;
}
