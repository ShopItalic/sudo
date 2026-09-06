#ifndef BC_BATTERY_FILTER_H
#define BC_BATTERY_FILTER_H

#include <stdbool.h>
#include <stdint.h>

/* Battery percentages are a protocol byte.  Values above 100 are reserved
 * for status/error values and are never admitted to the sample window. */
#define BC_BATTERY_PERCENT_UNKNOWN ((uint8_t)255U)
#define BC_BATTERY_FILTER_WINDOW 10U

/* This filter has no RTOS or platform dependency.  Its owner supplies the
 * critical section around update() when it is shared by more than one task. */
typedef struct {
    uint8_t samples[BC_BATTERY_FILTER_WINDOW];
    uint8_t count;
    uint8_t next;
    uint8_t value;
    bool initialized;
    bool charging;
    bool charging_initialized;
} bc_battery_filter;

void bc_battery_filter_init(bc_battery_filter *filter);
void bc_battery_filter_reset(bc_battery_filter *filter, bool charging);

/*
 * Add one sample and return the filtered result.  Invalid samples (>100)
 * return BC_BATTERY_PERCENT_UNKNOWN and leave valid history unchanged.  A
 * transition between charging and discharging starts a fresh epoch.  Valid
 * samples remain in the trimmed window, allowing a recovered estimate to
 * rise or fall without smoothing invalid data into a result.
 */
uint8_t bc_battery_filter_update(bc_battery_filter *filter, uint8_t sample,
                                 bool charging);

#endif
