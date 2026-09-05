#include "bc_battery_filter.h"

#include <stddef.h>
#include <string.h>

static uint8_t bc_battery_filter_average(const bc_battery_filter *filter)
{
    uint32_t total = 0U;
    uint8_t minimum = 0U;
    uint8_t maximum = 0U;
    uint8_t index;
    uint8_t divisor;

    if (filter->count == 0U)
        return BC_BATTERY_PERCENT_UNKNOWN;

    minimum = filter->samples[0];
    maximum = minimum;
    for (index = 0U; index < filter->count; ++index)
    {
        const uint8_t value = filter->samples[index];

        total += value;
        if (value < minimum)
            minimum = value;
        if (value > maximum)
            maximum = value;
    }

    divisor = filter->count;
    if (filter->count >= 3U)
    {
        /* Remove exactly one occurrence of each extremum.  Keeping the
         * extrema as values, rather than sentinel indices, also handles
         * repeated zero-valued samples correctly. */
        total -= minimum;
        total -= maximum;
        divisor = (uint8_t)(filter->count - 2U);
    }

    return (uint8_t)(total / divisor);
}

void bc_battery_filter_init(bc_battery_filter *filter)
{
    if (filter == NULL)
        return;

    (void)memset(filter, 0, sizeof(*filter));
}

void bc_battery_filter_reset(bc_battery_filter *filter, bool charging)
{
    if (filter == NULL)
        return;

    (void)memset(filter->samples, 0, sizeof(filter->samples));
    filter->count = 0U;
    filter->next = 0U;
    filter->value = 0U;
    filter->initialized = false;
    filter->charging = charging;
    filter->charging_initialized = true;
}

uint8_t bc_battery_filter_update(bc_battery_filter *filter, uint8_t sample,
                                 bool charging)
{
    uint8_t candidate;

    if (filter == NULL)
        return BC_BATTERY_PERCENT_UNKNOWN;

    if (!filter->charging_initialized)
    {
        filter->charging = charging;
        filter->charging_initialized = true;
    }
    else if (filter->charging != charging)
    {
        bc_battery_filter_reset(filter, charging);
    }

    if (sample > 100U)
        return BC_BATTERY_PERCENT_UNKNOWN;

    filter->samples[filter->next] = sample;
    filter->next = (uint8_t)((filter->next + 1U) % BC_BATTERY_FILTER_WINDOW);
    if (filter->count < BC_BATTERY_FILTER_WINDOW)
        ++filter->count;

    candidate = bc_battery_filter_average(filter);
    if (filter->initialized)
    {
        if (filter->charging)
        {
            if (candidate < filter->value)
                candidate = filter->value;
        }
        else if (candidate > filter->value)
        {
            candidate = filter->value;
        }
    }

    filter->value = candidate;
    filter->initialized = true;
    return candidate;
}
