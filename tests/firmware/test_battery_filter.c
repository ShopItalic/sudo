#include "bc_battery_filter.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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

static void test_zero_is_initialized(void)
{
    bc_battery_filter filter;

    bc_battery_filter_init(&filter);
    CHECK(bc_battery_filter_update(&filter, 0U, false) == 0U);
    CHECK(filter.initialized);
    CHECK(filter.count == 1U);
    CHECK(bc_battery_filter_update(&filter, BC_BATTERY_PERCENT_UNKNOWN,
                                   false) == BC_BATTERY_PERCENT_UNKNOWN);
    CHECK(filter.initialized);
    CHECK(filter.count == 1U);
}

static void test_averages_and_single_extrema(void)
{
    bc_battery_filter filter;

    bc_battery_filter_init(&filter);
    CHECK(bc_battery_filter_update(&filter, 100U, false) == 100U);
    CHECK(bc_battery_filter_update(&filter, 90U, false) == 95U);
    /* 100, 90 and 80 -> remove one 100 and one 80 -> 90. */
    CHECK(bc_battery_filter_update(&filter, 80U, false) == 90U);

    bc_battery_filter_init(&filter);
    CHECK(bc_battery_filter_update(&filter, 0U, false) == 0U);
    CHECK(bc_battery_filter_update(&filter, 0U, false) == 0U);
    /* Repeated zero extrema must still remove exactly one minimum. */
    CHECK(bc_battery_filter_update(&filter, 50U, false) == 0U);

    bc_battery_filter_init(&filter);
    CHECK(bc_battery_filter_update(&filter, 40U, true) == 40U);
    CHECK(bc_battery_filter_update(&filter, 60U, true) == 50U);
}

static void test_invalid_samples_do_not_poison_history(void)
{
    bc_battery_filter filter;

    bc_battery_filter_init(&filter);
    CHECK(bc_battery_filter_update(&filter, 50U, false) == 50U);
    CHECK(bc_battery_filter_update(&filter, 101U, false) ==
          BC_BATTERY_PERCENT_UNKNOWN);
    CHECK(filter.count == 1U);
    CHECK(filter.samples[0] == 50U);
    CHECK(bc_battery_filter_update(&filter, 40U, false) == 45U);
    CHECK(bc_battery_filter_update(&filter, 255U, false) ==
          BC_BATTERY_PERCENT_UNKNOWN);
    CHECK(filter.count == 2U);
}

static void test_recovery_epochs_and_window(void)
{
    bc_battery_filter filter;
    uint8_t sample;

    bc_battery_filter_init(&filter);
    CHECK(bc_battery_filter_update(&filter, 80U, false) == 80U);
    CHECK(bc_battery_filter_update(&filter, 0U, false) == 40U);
    CHECK(bc_battery_filter_update(&filter, 100U, false) == 80U);
    CHECK(bc_battery_filter_update(&filter, 100U, false) == 90U);
    CHECK(bc_battery_filter_update(&filter, 100U, false) == 93U);
    CHECK(bc_battery_filter_update(&filter, 0U, true) == 0U);
    CHECK(bc_battery_filter_update(&filter, 100U, true) == 50U);
    CHECK(bc_battery_filter_update(&filter, 0U, true) == 0U);
    CHECK(bc_battery_filter_update(&filter, 100U, true) == 50U);
    CHECK(bc_battery_filter_update(&filter, 10U, false) == 10U);

    bc_battery_filter_init(&filter);
    for (sample = 1U; sample <= BC_BATTERY_FILTER_WINDOW; ++sample)
        CHECK(bc_battery_filter_update(&filter, sample, true) !=
              BC_BATTERY_PERCENT_UNKNOWN);
    CHECK(filter.count == BC_BATTERY_FILTER_WINDOW);
    CHECK(filter.next == 0U);
    CHECK(bc_battery_filter_update(&filter, 20U, true) !=
          BC_BATTERY_PERCENT_UNKNOWN);
    CHECK(filter.count == BC_BATTERY_FILTER_WINDOW);
    CHECK(filter.samples[0] == 20U);
}

static void test_null_filter(void)
{
    CHECK(bc_battery_filter_update(NULL, 10U, false) ==
          BC_BATTERY_PERCENT_UNKNOWN);
    bc_battery_filter_init(NULL);
    bc_battery_filter_reset(NULL, true);
}

int main(void)
{
    test_zero_is_initialized();
    test_averages_and_single_extrema();
    test_invalid_samples_do_not_poison_history();
    test_recovery_epochs_and_window();
    test_null_filter();

    if (failures != 0U)
    {
        fprintf(stderr, "%u/%u checks failed\n", failures, checks);
        return 1;
    }

    printf("battery filter: %u checks passed\n", checks);
    return 0;
}
