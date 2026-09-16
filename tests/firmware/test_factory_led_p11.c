#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "app_factory_charging_p11.h"
#define HANDWARE_1_23_2 1
#define BC_LOG_INFO(...)
#define bc_rtos_max_delay UINT32_MAX
static unsigned checks, depth, delay_action, pending, waits, outputs;
static bool powered, recording_light = true;
static uint32_t shown;
static jmp_buf done;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"LED line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
#define taskENTER_CRITICAL() (++depth)
#define taskEXIT_CRITICAL() do { CHECK(depth); --depth; } while(0)
typedef uint32_t bc_event_bits;
typedef struct { uint8_t ble_connect_color, ble_disconnect_color, recording_color; } bc_device_led_motor_mode_info;
struct rgb_struct { uint8_t rgb_g, rgb_r, rgb_b; };
static struct { void *event_handler; bool event_clear_on_exit, event_wait_for_all_bits; } event_struct;
static void bc_rtos_event_group_set_bits(void *handle, unsigned bits) { CHECK(handle); pending |= bits; }
static unsigned bc_rtos_event_group_wait_bits(void *handle, unsigned bits, bool clear, bool all, unsigned wait) {
    unsigned result = pending; (void)bits; (void)clear; (void)all; (void)wait; CHECK(handle && !depth);
    if (waits++) longjmp(done, 1);
    pending = 0; return result;
}
static bool app_factory_controls_recording_light(void) { return recording_light; }
static void bc_device_info_led_motor_mode_info_get(bc_device_led_motor_mode_info *info) {
    info->recording_color = 2; info->ble_connect_color = info->ble_disconnect_color = 1;
}
static void bc_ldo_rgb_power_on(void) { powered = true; }
static void bc_ldo_rgb_power_off(void) { powered = false; shown = 0; }
static void tx1812n5_rgb_init(void) {}
static void tx1812n5_RGB(const struct rgb_struct *rgb, unsigned count) {
    CHECK(count == 1); shown = rgb->rgb_g | rgb->rgb_r<<8 | rgb->rgb_b<<16;
    CHECK(!shown || powered); ++outputs;
}
static void bc_delay_ms(unsigned ms) {
    unsigned action = delay_action;
    CHECK(!depth && ms); delay_action = 0;
    if (action == 1) p11_charging_indicator_set(P11_CHARGE_OFF);
    if (action == 2) p11_charging_indicator_set(P11_CHARGE_FULL);
    if (action == 3) recording_light = false;
}
#include "led.inc"
static void service(void) {
    waits = 0;
    if (!setjmp(done)) bc_ic_led_handler_thread(NULL);
    CHECK(!depth);
}
int main(void) {
    unsigned state, bits;
    for (state = 0; state < 256; ++state) {
        CHECK(p11_led_color(state, false, 99, 0) == (state == 1 ? 5128U : state == 2 ? 20U : 0U));
        CHECK(p11_led_color(state, true, 99, 0) == 99);
        CHECK(p11_led_color(state, true, 0, 0) == 0);
        CHECK(p11_led_color(state, true, 99, 456) == 456);
    }
    p11_charging_indicator_set(P11_CHARGE_AMBER); CHECK(!pending && !outputs);
    event_struct.event_handler = (void *)1;
    service(); CHECK(shown == 5128 && powered);
    p11_charging_indicator_set(P11_CHARGE_FULL); service(); CHECK(shown == 20);
    p11_charging_indicator_set(99); service(); CHECK(shown == 0 && !powered);
    p11_charging_indicator_set(P11_CHARGE_AMBER);
    delay_action = 1; p11_restore_led_base(); CHECK(shown == 0 && !powered);
    p11_charging_indicator_set(P11_CHARGE_AMBER);
    delay_action = 2; p11_restore_led_base(); CHECK(shown == 20);
    for (bits = 0; bits < 16; ++bits) {
        p11_recording_bits = bits; p11_restore_led_base(); CHECK(shown == (bits ? 5120U : 20U));
        recording_light = false; p11_restore_led_base(); CHECK(shown == (bits ? 0U : 20U));
        recording_light = true;
    }
    p11_recording_bits = 1; delay_action = 3; p11_restore_led_base(); CHECK(!shown && !powered);
    p11_recording_bits = 0; recording_light = true;
    bc_ic_led_mic_offline_recording_off(); bc_ic_led_mic_offline_recording_on(); service(); CHECK(shown == 5120);
    bc_ic_led_mic_offline_recording_on(); bc_ic_led_mic_offline_recording_off(); service(); CHECK(shown == 20);
    bc_ic_led_test_cmd(0, 0, 20); service(); CHECK(shown == 20U<<16);
    p11_charging_indicator_set(P11_CHARGE_AMBER); service(); CHECK(shown == 20U<<16);
    bc_ic_led_stop(); service(); CHECK(shown == 5128);
    bc_ic_led_test_cmd(0, 10, 0); bc_id_led_clear(); service(); CHECK(shown == 5128);
    pending |= IC_LED_BLE_CONNECT_EVENT; delay_action = 1; service(); CHECK(!shown && !powered);
    p11_charging_indicator_set(P11_CHARGE_AMBER); pending |= IC_LED_BLE_DISCONNECT_EVENT;
    delay_action = 2; service(); CHECK(shown == 20);
    printf("P11 LED: %u checks, 0 failures\n", checks); return 0;
}
