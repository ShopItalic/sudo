"""Run both production recording entry points, sender and timed finalization."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/firmware'))
import prepare_factory_ptt_v9 as recipe
from factory_local_recording_v8 import function_span
class Capture(unittest.TestCase):
    def test_actual_recording_paths_and_timeout_retry(self):
        original=subprocess.check_output(['git','show','102bfd2:'+recipe.APP+'app_pdm_handler.c'],cwd=ROOT).decode('latin1').replace('\r\n','\n')
        source=recipe.PATCHES[recipe.APP+'app_pdm_handler.c'](original)
        with tempfile.TemporaryDirectory(prefix='p09-capture-') as folder:
            out=Path(folder)
            signatures=['static bool factory_pdm_enqueue(void *data)','static void app_pdm_handler_thread(void *thread_handler)',
                        'static bool app_pdm_open(void)','static void app_pdm_close(void)',
                        'bool app_pdm_recording_start(void)','bool app_pdm_capture_recording_start(void)',
                        'bool app_pdm_recording_stop(void)','bool app_pdm_capture_recording_stop(void)',
                        'void bc_pdm_stop(void)']
            funcs=[]
            for sig in signatures:
                a,b=function_span(source,sig);funcs.append(source[a:b])
            (out/'capture.inc').write_text('\n'.join(funcs),encoding='latin1')
            subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror',
                '-Wno-unused-parameter','-Wno-invalid-utf8','-O1','-g','-fsanitize=address,undefined',
                '-I'+str(out),'-Itests/firmware/factory_ptt_v2','-Ifirmware/factory_ptt_v9',
                'tests/firmware/test_factory_capture_v9.c','firmware/factory_ptt_v9/app_factory_short.c',
                '-o',str(out/'test')],cwd=ROOT,check=True)
            subprocess.run([str(out/'test')],check=True)
if __name__=='__main__': unittest.main()
