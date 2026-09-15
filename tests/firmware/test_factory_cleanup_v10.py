"""Execute P10 code with actual supplier LittleFS and SHA-256 under sanitizers."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/firmware'))
import prepare_factory_ptt_v10 as recipe
import factory_cleanup_v10 as integration

def vendor(path):
    return subprocess.check_output(['git','show','102bfd2:'+path],cwd=ROOT).decode('latin1').replace('\r\n','\n')

class CleanupP10(unittest.TestCase):
    def test_actual_storage_workers(self):
        source=recipe.PATCHES[recipe.APP+'app_ppg_file_data_handler.c'](vendor(recipe.APP+'app_ppg_file_data_handler.c'))
        header=vendor(recipe.APP+'app_ppg_file_data_handler.h')
        types=[]
        for body,name in ((header,'enum ppg_file_type'),(source,'enum app_file_up_mode'),
                          (source,'enum ppg_file_err'),(source,'enum ppg_fls_status'),
                          (source,'struct ppg_file_hard'),(source,'struct ppg_file_one_click_upload')):
            a=body.index(name+'\n{');b=body.index('\n};',a)+3;types.append(body[a:b])
        functions=[]
        for name in ('app_ppg_list_files_info_get','ppg_file_data_upload_handler_thread',
                     'ppg_file_resume_upload_handler_thread','ppg_file_one_click_upload_handler_thread'):
            import re
            signature=re.search(r'(?m)^static [^\n;]+ '+name+r'\([^\n;]*\) *(?=\n\{)',source)[0]
            a,b=integration.function_span(source,signature);functions.append(source[a:b])
        with tempfile.TemporaryDirectory(prefix='p10-workers-') as folder:
            out=Path(folder)
            (out/'worker_types.inc').write_text('\n'.join(types),encoding='latin1')
            (out/'worker_functions.inc').write_text('\n'.join(functions),encoding='latin1')
            subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror',
                '-Wno-unused-parameter','-Wno-unused-function','-Wno-unused-variable','-Wno-unused-but-set-variable',
                '-Wno-invalid-utf8','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                '-I'+str(out),'tests/firmware/test_factory_workers_v10.c','-o',str(out/'workers')],check=True,cwd=ROOT)
            subprocess.run([str(out/'workers')],check=True,cwd=ROOT)

    def test_inherited_recording_paths(self):
        # The same P09 start/stop/drain/rollover fixtures run against the final
        # P10 generated source, including its storage finalization integration.
        import test_factory_capture_v9 as capture
        import test_factory_short_v9 as short
        old_capture,old_short=capture.recipe,short.recipe
        try:
            recipe.patch_delete_file=recipe.PATCHES[recipe.APP+'app_ppg_file_data_handler.c']
            capture.recipe=short.recipe=recipe
            capture.Capture('test_actual_recording_paths_and_timeout_retry').test_actual_recording_paths_and_timeout_retry()
            short.FactoryShortV9('test_real_file_finalization').test_real_file_finalization()
        finally:
            capture.recipe,short.recipe=old_capture,old_short

    def test_real_filesystem(self):
        with tempfile.TemporaryDirectory(prefix='p10-cleanup-') as folder:
            out=Path(folder)
            for name in ('lfs.c','lfs.h','lfs_util.c','lfs_util.h'):
                (out/name).write_text(vendor('firmware/bc_ros/bc_module/file/LittleFS/'+name),encoding='latin1')
            for name in ('sha256.c','sha256.h'):
                content=vendor('firmware/BCL603S2X/app/components/libraries/sha256/'+name)
                if name=='sha256.c': content=integration.patch_sha(content)
                (out/name).write_text(content,encoding='latin1')
            (out/'cleanup_adapter.inc').write_text(integration.ADAPTER)
            (out/'sdk_errors.h').write_text('#pragma once\n#include <stdint.h>\ntypedef uint32_t ret_code_t;\n#define NRF_SUCCESS 0\n#define NRF_ERROR_NULL 1\n')
            (out/'sdk_common.h').write_text('#pragma once\n#include <string.h>\n#define VERIFY_PARAM_NOT_NULL(x) do { if (!(x)) return NRF_ERROR_NULL; } while(0)\n')
            args=[os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror','-O1','-g',
                  '-fsanitize=address,undefined','-fno-sanitize-recover=all',
                  '-DLFS_NO_DEBUG','-DLFS_NO_WARN','-DLFS_NO_ERROR',
                  '-I'+str(out),'-I'+str(recipe.OVERLAY),str(ROOT/'tests/firmware/test_factory_cleanup_v10.c'),
                  str(out/'lfs.c'),str(out/'lfs_util.c'),str(out/'sha256.c'),'-o',str(out/'cleanup')]
            subprocess.run(args,check=True,cwd=ROOT);subprocess.run([str(out/'cleanup')],check=True,cwd=ROOT)
            # Reintroduce the early-idle race: this exact production adapter must
            # fail the two-task interleaving, not merely satisfy textual checks.
            (out/'cleanup_adapter.inc').write_text(integration.ADAPTER.replace(
                '&& !p10_reservation_owner &&', '&&'))
            subprocess.run(args,check=True,cwd=ROOT)
            negative=subprocess.run([str(out/'cleanup')],cwd=ROOT,text=True,capture_output=True)
            self.assertNotEqual(negative.returncode,0)
            self.assertIn('!p10_claim_idle()',negative.stderr)
            # Sanitizer findings must be fatal. The unpatched supplier SHA word
            # assembly is a known negative control for high-bit audio bytes.
            (out/'cleanup_adapter.inc').write_text(integration.ADAPTER)
            (out/'sha256.c').write_text(vendor(integration.SHA),encoding='latin1')
            subprocess.run(args,check=True,cwd=ROOT)
            negative=subprocess.run([str(out/'cleanup')],cwd=ROOT,text=True,capture_output=True)
            self.assertNotEqual(negative.returncode,0)
            self.assertIn('runtime error: left shift',negative.stderr)
            print('PASS P10 negative controls: storage reservation race and fatal SHA sanitizer finding')

    def test_recipe(self):
        file=recipe.PATCHES[recipe.APP+'app_ppg_file_data_handler.c'](vendor(recipe.APP+'app_ppg_file_data_handler.c'))
        ble=recipe.PATCHES[recipe.APP+'app_ble_handler.c'](vendor(recipe.APP+'app_ble_handler.c'))
        self.assertIn('app_factory_cleanup_mount(&app_ppg_file_hardle.lfs_fls_ppg_handle)',file)
        self.assertIn('app_factory_cleanup_service();',ble)
        self.assertIn('4096U / sizeof(StackType_t)',ble)
        for name in ('app_ppg_file_ls','app_ppg_file_sys_size_get','lk_app_ppg_file_open','app_ppg_file_open',
                     'app_ppg_file_upload','app_file_active_upload','app_ppg_file_resume_upload','app_ppg_file_one_click_upload',
                     'app_ppg_list_capture_audio_up_check','app_ppg_file_time_init'):
            self.assertIn(name+'_p10_inner(',file)
        self.assertIn('P10 requires an exact durable receipt',file)
        self.assertIn('Mount failure is not permission to format',recipe.PATCHES['firmware/bc_ros/bc_module/file/LittleFS/lfs_port.c'](
            vendor('firmware/bc_ros/bc_module/file/LittleFS/lfs_port.c')))
        for previous in recipe.prior.OVERLAY.iterdir():
            if previous.is_file():
                self.assertEqual(previous.read_bytes(),(recipe.OVERLAY/previous.name).read_bytes(),
                                 'P10 must carry the current P09 overlay: '+previous.name)

if __name__=='__main__': unittest.main()
