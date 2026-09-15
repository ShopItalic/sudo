from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/firmware'))
import prepare_factory_ptt_v9 as recipe

def vendor(path):
    return subprocess.check_output(['git','show','102bfd2:'+path],cwd=ROOT).decode('latin1').replace('\r\n','\n')

class FactoryScrollV9(unittest.TestCase):
    def test_settings_migration_and_confirmed_opt_in(self):
        with tempfile.TemporaryDirectory(prefix='p09-scroll-settings-') as folder:
            binary=Path(folder)/'settings'
            subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror','-O1','-g',
                '-fsanitize=address,undefined','-Itests/firmware/factory_controls',
                '-Ifirmware/factory_ptt_v9','tests/firmware/test_factory_scroll_settings_v9.c',
                '-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)

    def test_transport_errors_lifecycle_and_direction_gates(self):
        with tempfile.TemporaryDirectory(prefix='p09-scroll-') as folder:
            binary=Path(folder)/'scroll'
            command=[os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror','-O1','-g',
                '-fsanitize=address,undefined','-Itests/firmware/factory_scroll_v9',
                '-Itests/firmware/factory_ptt_v2','-Ifirmware/factory_ptt_v9',
                'tests/firmware/test_factory_scroll_v9.c','-o',str(binary)]
            subprocess.run(command,cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)

    def test_transport_safety_regressions_are_detected(self):
        source=(recipe.OVERLAY/'app_factory_scroll.c').read_text()
        mutations={
            'worker not woken': ('if (wake) xTaskNotifyGive(wake);', 'if (wake) (void)wake;'),
            'horizontal sent vertically': ('case FACTORY_SWIPE_LEFT: report[4]', 'case FACTORY_SWIPE_LEFT: report[3]'),
            'reconnect replay': ('intent.epoch != epoch || !allowed(intent.direction)', '!allowed(intent.direction)'),
            'SVC called with interrupts masked': ('result = sd_ble_gap_conn_sec_get(connection, &sec);',
                'taskENTER_CRITICAL(); result = sd_ble_gap_conn_sec_get(connection, &sec); taskEXIT_CRITICAL();'),
            'BLE IRQ races with send': ('(void)sd_nvic_critical_region_enter(&nested);', 'nested = 0;'),
        }
        with tempfile.TemporaryDirectory(prefix='p09-scroll-negative-') as folder:
            out=Path(folder);binary=out/'negative'
            for name,(before,after) in mutations.items():
                with self.subTest(name=name):
                    self.assertEqual(source.count(before),1)
                    (out/'app_factory_scroll.c').write_text(source.replace(before,after))
                    subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror','-O1',
                        '-fsanitize=address,undefined','-I'+str(out),'-Itests/firmware/factory_scroll_v9',
                        '-Itests/firmware/factory_ptt_v2','-Ifirmware/factory_ptt_v9',
                        'tests/firmware/test_factory_scroll_v9.c','-o',str(binary)],cwd=ROOT,check=True)
                    result=subprocess.run([str(binary)],capture_output=True,text=True)
                    self.assertNotEqual(result.returncode,0)
                    self.assertRegex(result.stderr, r'FAIL |[Aa]ssertion')
            print('PASS negative controls: missing wakeup, wrong horizontal axis, reconnect replay, interrupt-masked SVC, BLE IRQ race')

    def test_descriptor_is_one_five_byte_mouse_report(self):
        source=(recipe.OVERLAY/'app_factory_scroll.c').read_text()
        descriptor=source.split('static uint8_t scroll_report_map[] = {',1)[1].split('};',1)[0]
        data=bytes(int(v,16) for v in re.findall(r'0x([a-fA-F0-9]+)',descriptor))
        globals_={}; usages=[]; fields=[]; collections=[]; i=0
        while i<len(data):
            prefix=data[i]; i+=1
            size=(0,1,2,4)[prefix&3]; value=int.from_bytes(data[i:i+size],'little'); i+=size
            type_=(prefix>>2)&3; tag=prefix>>4
            if type_==1: globals_[tag]=value
            elif type_==2 and tag==0: usages.append((globals_.get(0),value))
            elif type_==0:
                if tag==10: collections.append(tuple(usages))
                if tag==8: fields.append((globals_.copy(),tuple(usages),value))
                usages=[]
        self.assertIn(((1,2),),collections) # Generic Desktop Mouse
        self.assertEqual(sum(g[7]*g[9] for g,_,_ in fields),40)
        self.assertEqual({g[8] for g,_,_ in fields},{1})
        self.assertEqual(fields[-2][1],((1,0x30),(1,0x31),(1,0x38)))
        self.assertEqual(fields[-1][1],((12,0x238),))
        self.assertEqual([field[2] for field in fields[-2:]],[6,6]) # relative variable input

    def test_generated_factory_hooks_and_pm_ownership(self):
        touch=recipe.patch_touch(vendor(recipe.APP+'app_touch_button_handler.c'))
        for direction in ('UP','DOWN','LEFT','RIGHT'):
            self.assertEqual(touch.count('app_factory_scroll_input(FACTORY_SWIPE_'+direction+');'),1)
            self.assertNotIn('app_package_button_up(BUTTON_SWIPE_'+direction+'_PRESS);',touch)
        ble=recipe.PATCHES[recipe.BLE](vendor(recipe.BLE))
        self.assertEqual(ble.count('app_factory_controls_init(true);'),1)
        self.assertEqual(ble.count('app_factory_scroll_init();'),1)
        self.assertNotIn('hids_init(&m_conn_handle);',ble)
        pm=ble.split('static void peer_manager_init(void)')[1].split('\n}')[0]
        self.assertNotIn('APP_ERROR_CHECK',pm)
        sec=ble.split('case BLE_GAP_EVT_SEC_PARAMS_REQUEST:')[1].split('case BLE_GATTS_EVT_SYS_ATTR_MISSING:')[0]
        self.assertIn('break;',sec)
        worker=recipe.patch_ble(vendor(recipe.APP+'app_ble_handler.c'))
        self.assertEqual(worker.count('app_factory_scroll_service();'),worker.count('app_factory_ptt_worker_service();'))

    def test_factory_softdevice_dispatch_contract(self):
        sdk='firmware/BCL603S2X/app/'
        dispatch=vendor(sdk+'components/softdevice/common/nrf_sdh.c')
        irq=dispatch.split('void SD_EVT_IRQHandler(void)')[1].split('\n}')[0]
        self.assertIn('nrf_sdh_evts_poll();',irq)
        self.assertIn('#define NRF_SDH_DISPATCH_MODEL 0',vendor(sdk+'user/inc/sdk_config.h'))
        nvic=vendor(sdk+'components/softdevice/s140/headers/nrf_nvic.h')
        region=nvic.rsplit('__STATIC_INLINE uint32_t sd_nvic_critical_region_enter(',1)[1].split('\n}')[0]
        self.assertIn('NVIC->ICER[0] = __NRF_NVIC_APP_IRQS_0;',region)
        self.assertIn('__sd_nvic_irq_enable();',region)
        self.assertNotIn('BASEPRI',region)
        config=vendor(sdk+'user/inc/FreeRTOSConfig.h')
        self.assertIn('#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY _PRIO_APP_HIGH',config)
        platform=vendor(sdk+'components/libraries/util/app_util_platform.h')
        self.assertRegex(platform,r'#define _PRIO_APP_HIGH\s+2\b')
        port=vendor(sdk+'external/freertos/portable/CMSIS/nrf52/portmacro_cmsis.h')
        self.assertIn('__set_BASEPRI',port)

if __name__=='__main__': unittest.main()
