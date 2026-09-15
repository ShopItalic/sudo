from pathlib import Path
import os
import subprocess
import tempfile
import unittest
import test_factory_ptt_v5 as factory
import prepare_factory_ptt_v5 as recipe

ROOT=recipe.ROOT
class FactoryControlsV5(unittest.TestCase):
    def test_persistence_and_gestures(self):
        with tempfile.TemporaryDirectory() as folder:
            out=Path(folder)/'controls'
            subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror','-O1','-g',
                            '-fsanitize=address,undefined','-Itests/firmware/factory_ptt_v5','-Itests/firmware/factory_controls',
                            '-Itests/firmware/factory_ptt_v2','-Ifirmware/factory_ptt_v5',
                            'firmware/factory_ptt_v5/app_factory_ptt.c','tests/firmware/test_factory_controls_v5.c',
                            '-o',str(out)],cwd=ROOT,check=True)
            subprocess.run([str(out)],check=True)

    def test_actual_factory_motor(self):
        with tempfile.TemporaryDirectory() as folder:
            out=Path(folder)
            original=factory.vendor(recipe.MOTOR)
            (out/'bc_linear_motor.c').write_text(recipe.patch_motor(original),encoding='latin1')
            (out/'bc_linear_motor.h').write_text(factory.vendor(recipe.MOTOR.replace('.c','.h')),encoding='latin1')
            (out/'bc_delay.h').write_text('#include <stdint.h>\nvoid bc_delay_ms(uint32_t ms);\n')
            command=[os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror','-Wno-invalid-utf8','-O1','-g',
                     '-fsanitize=address,undefined','-I'+str(out),'-Itests/firmware/factory_ptt_v2','-Itests/firmware/motor',
                     '-Ifirmware/factory_ptt_v5','-I'+str(out), str(out/'bc_linear_motor.c'),
                     'tests/firmware/test_factory_motor_v5.c','-o',str(out/'motor')]
            subprocess.run(command,cwd=ROOT,check=True)
            subprocess.run([str(out/'motor')],check=True)

if __name__=='__main__': unittest.main()
