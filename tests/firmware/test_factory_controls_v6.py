from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest
import test_factory_ptt_v5 as factory
import prepare_factory_ptt_v6 as recipe

class FactoryControlsV6(unittest.TestCase):
    def test_persistence_and_actual_recording_led_paths(self):
        source = recipe.patch_recording_light(factory.vendor(recipe.LED))
        blocks = re.findall(r'bc_device_led_motor_mode_info info = \{0\};.*?bc_ic_led_rgb_set\(g, r, b, 1\);', source, re.S)
        blocks = [block for block in blocks if 'info.recording_color' in block]
        self.assertEqual(len(blocks), 5)
        helper = factory.function(source, 'static void bc_ic_led_color_value_get(uint8_t color, uint8_t *g, uint8_t *r, uint8_t *b)')
        fixture = """
static uint8_t rgb_g, rgb_r, rgb_b, fixture_color;
typedef struct { uint8_t recording_color; } bc_device_led_motor_mode_info;
static void bc_device_info_led_motor_mode_info_get(bc_device_led_motor_mode_info *info) { info->recording_color=fixture_color; }
static void bc_ic_led_rgb_set(uint8_t g,uint8_t r,uint8_t b,unsigned on) { assert(on==1); rgb_g=g; rgb_r=r; rgb_b=b; }
""" + helper + '\n'
        fixture += 'static void recording_led_fixture(unsigned path, uint8_t color) { fixture_color=color; switch(path) {\n'
        for index, block in enumerate(blocks):
            fixture += 'case '+str(index)+': { '+block+' break; }\n'
        fixture += '} }\n'
        with tempfile.TemporaryDirectory() as folder:
            out=Path(folder)
            (out/'factory_recording_led.inc').write_text(fixture, encoding='latin1')
            command=[os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror','-Wno-invalid-utf8','-O1','-g',
                '-fsanitize=address,undefined','-I'+str(out),'-Itests/firmware/factory_ptt_v5','-Itests/firmware/factory_controls',
                '-Itests/firmware/factory_ptt_v2','-Ifirmware/factory_ptt_v6',
                'firmware/factory_ptt_v6/app_factory_ptt.c','tests/firmware/test_factory_controls_v6.c','-o',str(out/'controls')]
            subprocess.run(command,cwd=recipe.ROOT,check=True)
            subprocess.run([str(out/'controls')],check=True)
            (out/'factory_recording_led.inc').write_text(fixture.replace('if (app_factory_controls_recording_light())', 'if (1)'), encoding='latin1')
            subprocess.run(command,cwd=recipe.ROOT,check=True)
            negative=subprocess.run([str(out/'controls')],capture_output=True,text=True)
            self.assertNotEqual(negative.returncode,0)
            self.assertIn('FAIL ',negative.stderr)
            print('PASS negative control: unguarded recording LED is detected')

    def test_p05_motor_and_recording_led_patch_scope(self):
        for name in ('app_factory_motor.h',):
            self.assertEqual((recipe.OVERLAY/name).read_bytes(),(recipe.ROOT/'firmware/factory_ptt_v5'/name).read_bytes())
        original=factory.vendor(recipe.LED)
        expected='#include "app_factory_controls.h"\n'+original.replace(
            'bc_ic_led_color_value_get(info.recording_color, &g, &r, &b);',
            'if (app_factory_controls_recording_light())\n            bc_ic_led_color_value_get(info.recording_color, &g, &r, &b);')
        self.assertEqual(recipe.patch_recording_light(original),expected)

if __name__=='__main__': unittest.main()
