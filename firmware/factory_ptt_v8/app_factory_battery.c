#include "app_factory_battery.h"
#include "bc_battery_filter.h"
#include "bc_pmic.h"
#include "bc_rtos.h"

static bc_battery_filter filter;
static volatile bool busy;
static bool epoch_valid;
static enum pmic_charge_status epoch;
static uint32_t charge_generation;

void app_factory_battery_charge_changed(void)
{
    bc_rtos_taskENTER_CRITICAL();
    epoch_valid = false;
    ++charge_generation;
    bc_rtos_taskEXIT_CRITICAL();
}

static bool charge_valid(enum pmic_charge_status state)
{
    return state == PMIC_CHARGED_NOT || state == PMIC_CHARGED_ING ||
           state == PMIC_CHARGED_OVER;
}

uint8_t app_factory_battery_percent(void)
{
    enum pmic_charge_status before, after;
    uint8_t sample = BC_POWER_PERCENT_UNKNOWN;
    uint8_t result = BC_POWER_PERCENT_UNKNOWN;
    uint32_t generation;

    bc_rtos_taskENTER_CRITICAL();
    if (busy) {
        bc_rtos_taskEXIT_CRITICAL();
        return result;
    }
    busy = true;
    generation = charge_generation;
    bc_rtos_taskEXIT_CRITICAL();

    /* PMIC bus and ADC waits must remain outside the critical section. */
    before = bc_pmic_get_charge_status();
    if (charge_valid(before)) sample = bc_pmic_get_vbat_percen();
    after = bc_pmic_get_charge_status();

    bc_rtos_taskENTER_CRITICAL();
    if (generation != charge_generation ||
        !charge_valid(before) || !charge_valid(after)) {
        epoch_valid = false;
    } else {
        if (!epoch_valid || epoch != after) {
            bc_battery_filter_reset(&filter, after != PMIC_CHARGED_NOT);
            epoch = after;
            epoch_valid = true;
        }
        if (before == after)
            result = bc_battery_filter_update(&filter, sample,
                                               after != PMIC_CHARGED_NOT);
    }
    busy = false;
    bc_rtos_taskEXIT_CRITICAL();
    return result;
}
