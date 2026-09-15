"""Keep P09's battery calculation, guards and compact reports aligned with P08."""
from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/firmware'))
import factory_battery_v8 as battery
import factory_battery_reporting_v8 as reporting
import prepare_factory_ptt_v8 as p08
import prepare_factory_ptt_v9 as p09
import verify_factory_ptt_v9 as verifier


def vendor(path):
    return subprocess.check_output(['git', 'show', '102bfd2:' + path], cwd=ROOT).decode('latin1').replace('\r\n', '\n')


class FactoryBatteryV9(unittest.TestCase):
    def test_generated_battery_sampling_and_notifications_match_p08(self):
        # Compare the production recipe entries, so forgetting to register a
        # shared patch cannot silently restore the factory implementation.
        for path in (*battery.PATCHES, reporting.PACKAGE, p08.MOTOR):
            with self.subTest(path=path):
                self.assertIn(path, p09.PATCHES)
                original = vendor(path)
                self.assertEqual(p09.PATCHES[path](original), p08.PATCHES[path](original))

    def test_battery_and_motor_sources_match_the_exercised_p08_code(self):
        # These exact P08 sources run through the sanitizer-backed battery
        # integration, invalid-read, recovery and motor-exclusion harnesses.
        for name in (*battery.ADDED, 'app_factory_motor.h'):
            with self.subTest(name=name):
                self.assertEqual((p09.OVERLAY / name).read_bytes(),
                                 (p08.OVERLAY / name).read_bytes())

    def test_compact_report_is_an_explicit_verified_source_change(self):
        self.assertIn(reporting.PACKAGE.removeprefix('firmware/'), verifier.ALLOWED)

    def test_timer_budget_rejects_the_old_large_notification_path(self):
        def report(callback):
            entries = (
                ('prvTimerTask', 16, 16),
                ('prvProcessTimerOrBlockTask', 24, 24),
                ('prvProcessReceivedCommands', 40, 40),
                ('prvSampleTimeNow', 8, 8),
                ('prvSwitchTimerLists', 24, 24),
                ('app_pmic_handler_timer_callback', 8, callback),
            )
            return ''.join(f'<P><STRONG><a></a>{name}</STRONG> Stack size {frame} bytes; Max Depth = {depth}'
                           for name, frame, depth in entries)

        compact = verifier.battery_timer_budget(report(408))
        self.assertEqual(compact['joinedKnownBytes'], 496)
        self.assertEqual(compact['remainingAfterReserveBytes'], 272)
        with self.assertRaisesRegex(ValueError, 'Battery timer exceeds reviewed budget'):
            verifier.battery_timer_budget(report(744))
        with self.assertRaisesRegex(ValueError, 'Missing battery/timer stack entry'):
            verifier.battery_timer_budget('')


if __name__ == '__main__':
    unittest.main()
