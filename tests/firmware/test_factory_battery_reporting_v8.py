"""Compare compact notifications with exact factory behavior before adoption."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/firmware'))
import factory_battery_reporting_v8 as recipe


def vendor(path):
    return subprocess.check_output(['git', 'show', '102bfd2:' + path], cwd=ROOT).decode('latin1').replace('\r\n', '\n')


def function(source, signature):
    start = source.index(signature + '\n{')
    return source[start:source.index('\n}', start) + 2]


def packet_struct(source, name):
    start = source.index('struct ' + name + '\n{')
    return source[start:source.index('\n};', start) + 3]


class BatteryReportingV8(unittest.TestCase):
    def test_only_two_notification_functions_change(self):
        original = vendor(recipe.PACKAGE)
        compact = recipe.patch_package(original)
        for signature in recipe.SIGNATURES:
            original = original.replace(function(original, signature), '')
            compact = compact.replace(function(compact, signature), '')
        self.assertEqual(original, compact)

    def test_exhaustive_wire_and_side_effect_equivalence(self):
        original = vendor(recipe.PACKAGE)
        compact = recipe.patch_package(original)
        with tempfile.TemporaryDirectory(prefix='p08-battery-report-') as folder:
            out = Path(folder)
            types = packet_struct(vendor('firmware/bc_ros/bc_application/app_cmd_handler.h'), 'app_cmd_package')
            types += '\n' + packet_struct(vendor('firmware/bc_ros/bc_module/ble/inc/bc_ble_modu_interface.h'), 'bc_ble_data_package')
            (out / 'factory_packet_types.inc').write_text(types, encoding='latin1')
            for name, source in [('original', original), ('compact', compact)]:
                code = '\n'.join(function(source, signature) for signature in recipe.SIGNATURES)
                code = code.replace('app_package_precent_status_up', name + '_status')
                code = code.replace('app_package_precent_up', name + '_percent')
                (out / (name + '_notifications.inc')).write_text(code, encoding='latin1')
            command = [os.environ.get('CC', 'cc'), '-std=c99', '-O1', '-g',
                       '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                       '-I' + str(out), str(ROOT / 'tests/firmware/test_factory_battery_reporting_v8.c')]
            for factory_use in (False, True):
                binary = out / ('factory' if factory_use else 'normal')
                subprocess.run(command + (['-DFACTORY_USE=1'] if factory_use else []) +
                               ['-o', str(binary)], check=True, cwd=ROOT)
                result = subprocess.run([str(binary)], text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                print(('FACTORY_USE: ' if factory_use else 'normal: ') + result.stdout.strip())

            # The comparison must notice a changed command byte.
            p = out / 'compact_notifications.inc'
            p.write_text(p.read_text().replace('0x12', '0x13'))
            negative = out / 'negative'
            subprocess.run(command + ['-o', str(negative)], check=True, cwd=ROOT)
            result = subprocess.run([str(negative)], text=True, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('original/compact payload or side-effect mismatch', result.stderr)


if __name__ == '__main__':
    unittest.main()
