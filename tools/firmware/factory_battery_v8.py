"""Battery-only patches for the P08 factory recipe; no calibration claims."""
from prepare_factory_ptt_v3 import once

POWER = 'firmware/bc_ros/bc_module/pmic/bc_power.c'
PMIC_APP = 'firmware/bc_ros/bc_application/app_pmic_handler.c'
ADC = 'firmware/bc_ros/bc_driver/bsp/src/bsp_adc.c'
ADDED = ('app_factory_battery.c', 'app_factory_battery.h',
         'app_factory_battery_power.h', 'bc_battery_filter.c', 'bc_battery_filter.h')


def replace_function(source, signature, replacement):
    start = source.index(signature + '\n{')
    end = source.index('\n}', start) + 2
    return source[:start] + replacement + source[end:]


def patch_power(source):
    source = once(source, 'static uint8_t pre_bat_percent = 100;',
                  'static uint8_t pre_bat_percent = 100;\n'
                  '#include "app_factory_battery_power.h"')
    for signature in ('uint16_t bc_power_get_adc_value(void)',
                      'bool bc_power_check_vbat(void)'):
        source = replace_function(source, signature,
                                  '/* P08 checked implementation is in app_factory_battery_power.h. */')
    return once(source, 'bat_adc_float = bc_power_get_adc_value();',
                'bat_adc_float = bc_power_get_adc_value();\n'
                '    if (bat_adc_float == BC_POWER_ADC_ERROR)\n'
                '        return BC_POWER_PERCENT_UNKNOWN;')


def patch_pmic(source):
    source = '#include "app_factory_battery.h"\n' + source
    for signature in ('uint8_t getvpct_(void)', 'uint8_t getvpct(void)'):
        source = replace_function(source, signature,
                                  signature + '\n{\n    return app_factory_battery_percent();\n}')
    source = once(source, 'static STR_PRECENT precentval = {0};', '')
    source = once(source, 'static uint8_t pre_percent = 0;', '')
    # Invalid 255 already takes the existing >10 branch and clears the count.
    return once(source, '\t\tswitch(pmic_state)',
                '        if (pmic_state != pmic_state_check) app_factory_battery_charge_changed();\n'
                '        if (pmic_state != PMIC_CHARGED_NOT) pmic_percent_low_count = 0;\n'
                '\t\tswitch(pmic_state)')


def patch_adc(source):
    # Keep 10 us acquisition; report conversion failures without reading an
    # uninitialized stack value. Calibration changes need physical evidence.
    return once(source,
                '\t\t\tnrf_saadc_value_t nrf_saadc_value;\n'
                '\t\t\tnrfx_saadc_sample_convert(i, &nrf_saadc_value);',
                '\t\t\tnrf_saadc_value_t nrf_saadc_value = 0;\n'
                '\t\t\tint result = nrfx_saadc_sample_convert(i, &nrf_saadc_value);\n'
                '\t\t\tif (result != RESULT_OK) return result;')


def patch_vibrate_flags(source):
    # Zero means playback without STOP, not PWM_FLAG_STOP (1). Without the
    # STOPPED event the motor owner stays active and battery reads stay 255.
    source = once(source, 'flags = 0;  /* PWM_FLAG_STOP */',
                  'flags = PWM_FLAG_STOP;  /* Request STOPPED, then worker-owned power-off. */')
    return once(source, 'flags = 2;  /* PWM_FLAG_LOOP */',
                'flags = PWM_FLAG_LOOP;  /* Explicit stop still required. */')


PATCHES = {POWER: patch_power, PMIC_APP: patch_pmic, ADC: patch_adc}
