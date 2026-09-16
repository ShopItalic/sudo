"""Steady charging base layer on the existing standard-board LED owner."""
from factory_audio_v11 import once, function

LED = "firmware/bc_ros/bc_module/led/bc_ic_led.c"
PMIC = "firmware/bc_ros/bc_application/app_pmic_handler.c"

HELPERS = '''
/* Desired base state survives coalesced notifications and cue delays.
 * Only the existing LED task writes the device, including manual/find cues. */
static volatile uint32_t p11_manual_rgb;
static volatile unsigned p11_charge_state, p11_recording_bits;
static void p11_wake_led(void)
{
    if (event_struct.event_handler)
        bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_P11_BASE_EVENT);
}
void p11_charging_indicator_set(unsigned state)
{
    bool changed;
    if (state > P11_CHARGE_FULL) state = P11_CHARGE_OFF;
    taskENTER_CRITICAL(); changed = p11_charge_state != state; p11_charge_state = state; taskEXIT_CRITICAL();
    if (changed) p11_wake_led();
}
static void p11_recording_desired(unsigned mask, bool on)
{
    taskENTER_CRITICAL();
    if (on) p11_recording_bits |= mask; else p11_recording_bits &= ~mask;
    taskEXIT_CRITICAL();
}
static void p11_manual_desired(uint8_t g, uint8_t r, uint8_t b)
{
    taskENTER_CRITICAL(); p11_manual_rgb = (uint32_t)g | (uint32_t)r<<8 | (uint32_t)b<<16; taskEXIT_CRITICAL();
    p11_wake_led();
}
static void p11_restore_led_base(void)
{
    bc_device_led_motor_mode_info info = {0};
    struct rgb_struct rgb = {0};
    uint8_t g = 0, r = 0, b = 0;
    uint32_t color, recording_color;
    bc_device_info_led_motor_mode_info_get(&info);
    if (app_factory_controls_recording_light()) bc_ic_led_color_value_get(info.recording_color, &g, &r, &b);
    recording_color = (uint32_t)g | (uint32_t)r<<8 | (uint32_t)b<<16;
    taskENTER_CRITICAL();
    color = p11_led_color(p11_charge_state, p11_recording_bits != 0, recording_color, p11_manual_rgb);
    if (!color) {
        tx1812n5_RGB(&rgb, 1); bc_ldo_rgb_power_off();
        taskEXIT_CRITICAL(); return;
    }
    taskEXIT_CRITICAL();
    bc_ldo_rgb_power_on(); bc_delay_ms(20);
    /* Re-read AFTER settling. Unplugging during that delay cannot relight
     * amber/green using an old snapshot; publication and output serialize. */
    taskENTER_CRITICAL();
    if (!app_factory_controls_recording_light()) recording_color = 0;
    color = p11_led_color(p11_charge_state, p11_recording_bits != 0, recording_color, p11_manual_rgb);
    rgb.rgb_g = (uint8_t)color; rgb.rgb_r = (uint8_t)(color >> 8); rgb.rgb_b = (uint8_t)(color >> 16);
    tx1812n5_RGB(&rgb, 1);
    if (!color) bc_ldo_rgb_power_off();
    taskEXIT_CRITICAL();
}
'''


def patch_led(source):
    source = '#include "app_factory_charging_p11.h"\n' + source
    source = once(source, "enum BC_IC_LED_EVENT\n{", "enum BC_IC_LED_EVENT\n{\n  IC_LED_P11_BASE_EVENT = (0x00000001 << 23),")
    marker = "static void bc_ic_led_handler_thread(void *thread_handler)\n{"
    source = once(source, marker, HELPERS + "\n" + marker)
    start = source.index(marker); end = source.index("\n}", start) + 2
    body = source[start:end]
    body = once(body, "  bc_ic_led_rgb_clear();\n  while(true)", "  bc_ic_led_rgb_clear();\n  p11_restore_led_base();\n  while(true)")
    body = once(body, "IC_LED_BLE_CONNECT_EVENT | IC_LED_BLE_DISCONNECT_EVENT |", "IC_LED_P11_BASE_EVENT | IC_LED_BLE_CONNECT_EVENT | IC_LED_BLE_DISCONNECT_EVENT |")
    end_loop = body.rindex("\n  }")
    body = body[:end_loop] + "\n    p11_restore_led_base(); /* Latest state after all transient cues. */" + body[end_loop:]
    source = source[:start] + body + source[end:]
    for name, mask in (("offline_recording", 1), ("online_recording", 2),
                       ("offline_recording_capture", 4), ("online_recording_capture", 8)):
        for direction, state in (("on", "true"), ("off", "false")):
            signature = "void bc_ic_led_mic_" + name + "_" + direction + "(void)\n{"
            source = once(source, signature, signature + "\n    p11_recording_desired(" + str(mask) + ", " + state + ");")
    source = function(source, "void bc_ic_led_set(uint8_t* rgb_data)",
        "    const struct rgb_struct *rgb = (const struct rgb_struct *)rgb_data;\n"
        "    if (rgb) p11_manual_desired(rgb->rgb_g, rgb->rgb_r, rgb->rgb_b);")
    source = function(source, "void bc_ic_led_test_cmd(uint8_t g,uint8_t r,uint8_t b)", "    p11_manual_desired(g, r, b);")
    for signature in ("void bc_ic_led_stop(void)", "void bc_id_led_clear(void)"):
        source = function(source, signature, "    p11_manual_desired(0, 0, 0);")
    return source


def patch_pmic(source):
    source = '#include "app_factory_charging_p11.h"\n' + source
    return once(source, "pmic_state = bc_pmic_get_charge_status();",
        "pmic_state = bc_pmic_get_charge_status();\n"
        "        p11_charging_indicator_set(pmic_state == PMIC_CHARGED_ING ? P11_CHARGE_AMBER :\n"
        "            pmic_state == PMIC_CHARGED_OVER ? P11_CHARGE_FULL : P11_CHARGE_OFF);")


PATCHES = {LED: patch_led, PMIC: patch_pmic}
