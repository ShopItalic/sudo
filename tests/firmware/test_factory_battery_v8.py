"""Exercise the exact P08 battery patches against immutable factory source."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/firmware'))
import factory_battery_v8 as recipe

OVERLAY = ROOT / 'firmware/factory_ptt_v8'


def vendor(path):
    return subprocess.check_output(['git', 'show', '102bfd2:' + path], cwd=ROOT).decode('latin1').replace('\r\n', '\n')


def function(source, signature):
    start = source.index(signature + '\n{')
    return source[start:source.index('\n}', start) + 2]


def compile_run(out, sources, includes, extra=()):
    if '-DHARDWARE_1231_ENABLED=1' in extra:
        config = (ROOT / 'tests/firmware/battery_power/ring_config.h').read_text()
        (out / 'ring_config.h').write_text(config.replace(
            '#define HARDWARE_1231_ENABLED 0', '#define HARDWARE_1231_ENABLED 1'))
    command = [os.environ.get('CC', 'cc'), '-std=c99', '-Wall', '-Wextra', '-Werror',
               '-Wno-unused-parameter', '-Wno-unused-variable', '-Wno-unused-function',
               '-Wno-unused-but-set-variable', '-Wno-invalid-utf8', '-Wno-sign-compare',
               '-g', '-O1', '-fsanitize=address,undefined']
    command += ['-I' + str(p) for p in [out, OVERLAY, *includes]]
    command += list(extra) + list(map(str, sources)) + ['-o', str(out / 'test')]
    subprocess.run(command, cwd=ROOT, check=True)
    return subprocess.run([str(out / 'test')], text=True, capture_output=True)


class FactoryBatteryV8(unittest.TestCase):
    def assert_run(self, result):
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        print(result.stdout.strip())

    def test_filter_and_actual_pmic_integration(self):
        with tempfile.TemporaryDirectory(prefix='p08-battery-app-') as folder:
            out = Path(folder)
            source = recipe.patch_pmic(vendor(recipe.PMIC_APP))
            (out / 'factory_pmic.inc').write_text('\n'.join(function(source, sig) for sig in
                ['uint8_t getvpct_(void)', 'uint8_t getvpct(void)', 'static void app_pmic_handler(void)']), encoding='latin1')
            result = compile_run(out, [ROOT / 'tests/firmware/test_factory_battery_v8.c',
                OVERLAY / 'app_factory_battery.c', OVERLAY / 'bc_battery_filter.c'],
                [ROOT / 'tests/firmware/battery_power', ROOT / 'firmware/bc_ros/bc_module/pmic'],
                ['-DHARDWARE_1231_ENABLED=1', '-DHANDWARE_1_23_2'])
            self.assert_run(result)

            # The regression must detect a missing maximum subtraction.
            broken = (OVERLAY / 'bc_battery_filter.c').read_text().replace(
                'total -= maximum;', '/* negative control: maximum not removed */')
            (out / 'broken_filter.c').write_text(broken)
            negative = compile_run(out, [ROOT / 'tests/firmware/test_factory_battery_v8.c',
                OVERLAY / 'app_factory_battery.c', out / 'broken_filter.c'],
                [ROOT / 'tests/firmware/battery_power', ROOT / 'firmware/bc_ros/bc_module/pmic'],
                ['-DHARDWARE_1231_ENABLED=1', '-DHANDWARE_1_23_2'])
            self.assertNotEqual(negative.returncode, 0)
            self.assertIn('FAIL ', negative.stderr)

    def test_checked_adc_and_motor_exclusion(self):
        with tempfile.TemporaryDirectory(prefix='p08-battery-power-') as folder:
            out = Path(folder)
            (out / 'factory_power.c').write_text(recipe.patch_power(vendor(recipe.POWER)), encoding='latin1')
            source = (ROOT / 'tests/firmware/test_battery_power.c').read_text()
            source = source.replace('#ifndef SUDO_VOICE_ONLY\n#define SUDO_VOICE_ONLY 1\n#endif\n', '')
            source = source.replace('#include "bc_linear_motor.h"', '#include "app_factory_battery.h"')
            source = source.replace('../../firmware/bc_ros/bc_module/pmic/bc_power.c', 'factory_power.c')
            (out / 'power_test.c').write_text(source)
            self.assert_run(compile_run(out, [out / 'power_test.c'],
                [ROOT / 'tests/firmware/battery_power', ROOT / 'firmware/bc_ros/bc_module/pmic'],
                ['-DHARDWARE_1231_ENABLED=1', '-DHANDWARE_1_23_2']))

    def test_nordic_conversion_errors_and_unchanged_acquisition(self):
        with tempfile.TemporaryDirectory(prefix='p08-battery-adc-') as folder:
            out = Path(folder)
            (out / 'factory_adc.c').write_text(recipe.patch_adc(vendor(recipe.ADC)), encoding='latin1')
            source = (ROOT / 'tests/firmware/test_bsp_adc.c').read_text()
            source = source.replace('#ifndef SUDO_VOICE_ONLY\n#define SUDO_VOICE_ONLY 1\n#endif\n', '')
            source = source.replace('../../firmware/bc_ros/bc_driver/bsp/src/bsp_adc.c', 'factory_adc.c')
            source = source.replace('NRF_SAADC_ACQTIME_40US', 'NRF_SAADC_ACQTIME_10US')
            (out / 'adc_test.c').write_text(source)
            self.assert_run(compile_run(out, [out / 'adc_test.c'], [ROOT / 'tests/firmware/bsp_adc']))

    def test_voltage_calibration_and_curve_unchanged(self):
        original = vendor(recipe.POWER)
        changed = recipe.patch_power(original)
        expected = function(original, 'uint8_t bc_power_get_vbat_percen(void)').replace(
            'bat_adc_float = bc_power_get_adc_value();',
            'bat_adc_float = bc_power_get_adc_value();\n'
            '    if (bat_adc_float == BC_POWER_ADC_ERROR)\n'
            '        return BC_POWER_PERCENT_UNKNOWN;')
        self.assertEqual(function(changed, 'uint8_t bc_power_get_vbat_percen(void)'), expected)
        self.assertEqual(original.split('static const bat_vp_table_t g_bat_vp_table[]')[1].split('};')[0],
                         changed.split('static const bat_vp_table_t g_bat_vp_table[]')[1].split('};')[0])
        self.assertEqual((OVERLAY / 'bc_battery_filter.c').read_bytes(),
                         (ROOT / 'firmware/bc_ros/bc_module/pmic/bc_battery_filter.c').read_bytes())

    def test_actual_factory_motor_snapshot(self):
        import prepare_factory_ptt_v8 as p08
        with tempfile.TemporaryDirectory(prefix='p08-battery-motor-') as folder:
            out = Path(folder)
            (out / 'bc_linear_motor.c').write_text(p08.patch_motor(vendor(p08.MOTOR)), encoding='latin1')
            (out / 'bc_linear_motor.h').write_text(vendor(p08.MOTOR.replace('.c', '.h')), encoding='latin1')
            (out / 'bc_delay.h').write_text('#include <stdint.h>\nvoid bc_delay_ms(uint32_t ms);\n')
            source = (ROOT / 'tests/firmware/test_factory_motor_v5.c').read_text()
            source = '#include "app_factory_battery.h"\n' + source
            source = source.replace('    printf("PASS P05 motor:', '''    bc_linear_motor_activity_t activity;
    bc_linear_motor_activity_get(&activity);
    CHECK(!activity.active && activity.has_finished);
    uint32_t generation = activity.generation;
    fixture_ticks = 1234;
    bc_linear_motor_start(LINEAR_MOTOR_MIC_START);
    bc_linear_motor_activity_get(&activity);
    CHECK(activity.active && activity.active_since_tick == 1234);
    CHECK(activity.generation != generation);
    fixture_ticks = 1244;
    finish_irq();
    bc_linear_motor_activity_get(&activity);
    CHECK(activity.active); /* Supply remains on until the service owns stop. */
    bc_linear_motor_service();
    bc_linear_motor_activity_get(&activity);
    CHECK(!activity.active && activity.last_finished_tick == 1244);
    CHECK(activity.has_finished && !depth);
    printf("PASS P08 motor:''')
            (out / 'motor_test.c').write_text(source)
            self.assert_run(compile_run(out, [out / 'bc_linear_motor.c', out / 'motor_test.c'],
                [ROOT / 'tests/firmware/factory_ptt_v2', ROOT / 'tests/firmware/motor'],
                ['-DHANDWARE_1_23_1', '-DHANDWARE_1_23_2']))

    def test_finite_recording_cues_release_the_battery_guard(self):
        import prepare_factory_ptt_v8 as p08
        with tempfile.TemporaryDirectory(prefix='p08-battery-cue-') as folder:
            out = Path(folder)
            (out / 'bc_linear_motor.c').write_text(p08.patch_motor(vendor(p08.MOTOR)), encoding='latin1')
            (out / 'bc_linear_motor.h').write_text(vendor(p08.MOTOR.replace('.c', '.h')), encoding='latin1')
            app = p08.APP + 'app_linear_motor_handler.c'
            (out / 'app_linear_motor_handler.h').write_text(vendor(app.replace('.c', '.h')), encoding='latin1')
            source = p08.patch_vibrate(vendor(app))
            table = source[source.index('typedef struct\n{'):source.index('\n#if !(defined(HANDWARE_1_23_3)')]
            # Select the standard-board PWM implementation, not the IC branch.
            pwm = source[source.rindex('uint8_t app_vibrate_start'):]
            cue = '\n'.join(function(pwm, sig) for sig in (
                'uint8_t app_vibrate_start(vibrate_mode_t mode, uint8_t count)',
                'void app_vibrate_stop(void)'))
            (out / 'factory_cue.c').write_text(
                '#include "app_linear_motor_handler.h"\n#include "bc_linear_motor.h"\n'
                '#include "q_device.h"\n#include "bc_logger.h"\n' + table +
                '\nstatic vibrate_mode_t s_vibrate_mode = VIBRATE_MODE_MAX;\n' + cue, encoding='latin1')
            (out / 'factory_pwm.c').write_text(vendor('firmware/bc_ros/bc_driver/bsp/src/bsp_pwm0.c'), encoding='latin1')
            (out / 'factory_power.c').write_text(recipe.patch_power(vendor(recipe.POWER)), encoding='latin1')
            (out / 'q_device.h').write_text((ROOT / 'tests/firmware/motor/q_device.h').read_text() +
                '\n#include <assert.h>\n#include <stddef.h>\n#define APP_ERROR_CHECK(e) assert((e) == 0)\n'
                'int q_device_read(q_device_t *, int, const void *, int);\n')
            (out / 'bc_rtos.h').write_text((ROOT / 'tests/firmware/factory_ptt_v2/bc_rtos.h').read_text() +
                '\n#define bc_rtos_taskENTER_CRITICAL() fixture_enter()\n'
                '#define bc_rtos_taskEXIT_CRITICAL() fixture_leave()\n'
                '#define bc_rtos_task_get_tick_count() fixture_ticks\n')
            (out / 'bc_ldo_switch.h').write_text(
                'void bc_ldo_motor_power_on(void);\nvoid bc_ldo_motor_power_off(void);\n'
                'void bc_ldo_bat_power_on(void);\nvoid bc_ldo_bat_power_off(void);\n')
            (out / 'bc_delay.h').write_text('#include <stdint.h>\nvoid bc_delay_ms(uint32_t ms);\n')
            sources = [out / name for name in ('bc_linear_motor.c', 'factory_cue.c', 'factory_pwm.c', 'factory_power.c')]
            sources.append(ROOT / 'tests/firmware/test_factory_battery_motor_v8.c')
            includes = [ROOT / 'tests/firmware/motor', ROOT / 'tests/firmware/battery_power',
                        ROOT / 'firmware/bc_ros/bc_module/pmic']
            defines = ['-DHARDWARE_ARCH_TYPE_NORDIC=1', '-DHARDWARE_1231_ENABLED=1',
                       '-DHANDWARE_1_23_1', '-DHANDWARE_1_23_2']
            self.assert_run(compile_run(out, sources, includes, defines))

            # The exact historical mistake must fail the integrated path:
            # FINISHED is not STOPPED, so the motor remains active to the guard.
            old = (out / 'factory_cue.c').read_text(encoding='latin1').replace(
                'flags = PWM_FLAG_STOP;', 'flags = 0;')
            (out / 'factory_cue.c').write_text(old, encoding='latin1')
            negative = compile_run(out, sources, includes, defines)
            self.assertNotEqual(negative.returncode, 0)
            self.assertIn('ADC=65535 and battery=255 even 30 minutes after playback', negative.stderr)
            self.assertIn('!activity.active && activity.has_finished && !powered', negative.stderr)
            print('PASS negative control: historical flags=0 reproduces ADC=65535 / battery=255 after 30 minutes')


if __name__ == '__main__':
    unittest.main()
