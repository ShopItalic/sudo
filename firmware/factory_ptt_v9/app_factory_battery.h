#ifndef APP_FACTORY_BATTERY_H
#define APP_FACTORY_BATTERY_H

#include <stdbool.h>
#include <stdint.h>

#define BC_POWER_ADC_ERROR UINT16_MAX
#define BC_POWER_PERCENT_UNKNOWN ((uint8_t)255U)

/* Task-context snapshot of every factory motor entry/exit, including the
 * pre-PWM supply settling interval. No driver calls or waits in this getter. */
typedef struct {
    uint32_t generation;
    uint32_t active_since_tick;
    uint32_t last_finished_tick;
    bool active;
    bool has_finished;
} bc_linear_motor_activity_t;

void bc_linear_motor_activity_get(bc_linear_motor_activity_t *activity);

/* PMIC observer: invalidate history even when a charger transition does not
 * request a percentage (for example, the factory charge-complete branch). */
void app_factory_battery_charge_changed(void);

/* Owns a complete PMIC/ADC/filter transaction. Returns 255 if busy, invalid,
 * or if charge state changes during acquisition. Valid history can recover. */
uint8_t app_factory_battery_percent(void);

#endif
