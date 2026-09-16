#ifndef APP_FACTORY_CHARGING_P11_H
#define APP_FACTORY_CHARGING_P11_H
#include <stdbool.h>
#include <stdint.h>
enum p11_charge_indicator { P11_CHARGE_OFF, P11_CHARGE_AMBER, P11_CHARGE_FULL };
/* Packed G,R,B (LED API order), not the device struct's R,G,B field order. */
static uint32_t p11_led_color(unsigned charge, bool recording, uint32_t recording_color, uint32_t manual)
{
    if (manual) return manual;
    if (recording) return recording_color; /* Includes recording-light off. */
    if (charge == P11_CHARGE_AMBER) return 8U | (20U << 8);
    if (charge == P11_CHARGE_FULL) return 20U;
    return 0; /* Unplugged or unknown: never guess from battery percent. */
}
void p11_charging_indicator_set(unsigned state);
#endif
