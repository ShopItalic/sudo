from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/firmware'))
import prepare_factory_ptt_v9 as recipe
from factory_local_recording_v8 import function_span

def vendor(path):
    return subprocess.check_output(['git','show','102bfd2:'+path],cwd=ROOT).decode('latin1').replace('\r\n','\n')

def function(source, signature):
    a,b=function_span(source,signature)
    return source[a:b]

class FactoryShortV9(unittest.TestCase):
    def test_upload_body_preserved_after_checked_open(self):
        original=vendor(recipe.APP+'app_ppg_file_data_handler.c')
        patched=recipe.patch_delete_file(original)
        for signature in ('static void ppg_file_data_upload_handler_thread(void * p_context)',
                          'static void ppg_file_one_click_upload_handler_thread(void * p_context)'):
            marker='file_size = ppg_file_size(&app_ppg_file_hardle);'
            self.assertEqual(function(original,signature).split(marker,1)[1],
                             function(patched,signature).split(marker,1)[1])

    def test_real_file_finalization(self):
        with tempfile.TemporaryDirectory(prefix='p09-littlefs-') as folder:
            out=Path(folder)
            source=recipe.patch_delete_file(vendor(recipe.APP+'app_ppg_file_data_handler.c'))
            for name in ('lfs.c','lfs.h','lfs_util.c','lfs_util.h'):
                (out/name).write_text(vendor('firmware/bc_ros/bc_module/file/LittleFS/'+name),encoding='latin1')
            signatures=['static enum ppg_file_err factory_ppg_file_create(struct ppg_file_hard *file_hardle)',
                        'static enum ppg_file_err lk_ppg_file_open(struct ppg_file_hard *file_hardle) ',
                        'static enum ppg_file_err lk_ppg_file_close(struct ppg_file_hard *file_hardle) ',
                        'static enum ppg_file_err ppg_file_write(struct ppg_file_hard *file_hardle,uint8_t *write_buff,uint32_t write_length) ',
                        'void app_ppg_file_write(uint8_t *write_buff,uint32_t write_length) ',
                        'bool app_factory_capture_bind_file(void)', 'bool app_factory_capture_finish_file(void)']
            (out/'factory_short_file.inc').write_text('static char factory_capture_path[FACTORY_DELETE_PATH_SIZE];\n'+
                '\n'.join(function(source,s) for s in signatures),encoding='latin1')
            binary=out/'files'
            subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror',
                '-Wno-sign-compare','-Wno-invalid-utf8','-Wno-unused-function','-O1','-g','-fsanitize=address,undefined',
                '-DLFS_NO_DEBUG','-DLFS_NO_WARN','-DLFS_NO_ERROR',
                '-I'+str(out),'-Itests/firmware/factory_ptt_v2','-Ifirmware/factory_ptt_v9',
                'tests/firmware/test_factory_short_files_v9.c','firmware/factory_ptt_v9/app_factory_short.c',
                'firmware/factory_ptt_v9/app_factory_delete.c',str(out/'lfs.c'),str(out/'lfs_util.c'),'-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)

    def test_policy_and_settings(self):
        with tempfile.TemporaryDirectory(prefix='p09-policy-') as folder:
            binary=Path(folder)/'policy'
            command=[os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror','-O1','-g',
                     '-fsanitize=address,undefined','-include','stdlib.h',
                     '-Itests/firmware/factory_controls','-Itests/firmware/factory_ptt_v2',
                     '-Ifirmware/factory_ptt_v9','tests/firmware/test_factory_short_v9.c','-o',str(binary)]
            subprocess.run(command,cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)

if __name__=='__main__': unittest.main()
