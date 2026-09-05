#!/usr/bin/env python3
"""Compile real candidate translation units using GCC; this is not a firmware link."""
import argparse
import json
from pathlib import Path
import subprocess
import xml.etree.ElementTree as E

ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser()
p.add_argument('--project',default='firmware/BCL603S2X/app/project/mdk5/sudo_voice.uvprojx')
p.add_argument('--newlib-include',default='.local/toolchains/newlib-4.5.0.20241231/newlib/libc/include')
p.add_argument('--output',default='build/firmware/arm-check')
p.add_argument('files',nargs='*')
a=p.parse_args()
project=ROOT/a.project
tree=E.parse(project)
target=tree.find('./Targets/Target')
if 'sudo_voice' not in project.name:
    target=next(t for t in tree.findall('./Targets/Target') if t.findtext('TargetName')=='1.23.2')
controls=target.find('.//TargetArmAds/Cads/VariousControls')
def resolve(s):
    return (project.parent/s.replace('\\','/')).resolve()
# Group/file include lists add supplier paths without changing SDK definitions.
paths=[]
for n in target.findall('.//IncludePath'):
    for x in (n.text or '').split(';'):
        if x and resolve(x) not in paths:
            paths.append(resolve(x))
app=project.parents[2]
paths=[app/'external/freertos/portable/GCC/nrf52',app/'components/toolchain/cmsis/include'] + paths
paths=[x for x in paths if '/RVDS/' not in str(x)]
newlib=(ROOT/a.newlib_include).resolve()
paths.append(newlib)
output=ROOT/a.output;output.mkdir(parents=True,exist_ok=True)
flags=['-mcpu=cortex-m4','-mthumb','-mfloat-abi=hard','-mfpu=fpv4-sp-d16','-std=gnu99','-O2','-ffunction-sections','-fdata-sections','-ffreestanding','-fno-strict-aliasing','-fstack-usage','-Werror=implicit-function-declaration','-D__MODULE__=__FILE__']
flags+=['-D'+x for x in controls.findtext('Define').replace(',',' ').split()]
flags+=['-I'+str(x) for x in paths]
files=a.files or ['bc_ble.c','bc_ble_gatt.c','bc_ble_info_service.c','bc_ble_modu_interface.c','bc_ble_hids_service.c','bc_queue.c','app_ble_handler.c','app_package.c','app_cmd_handler.c','app_ppg_file_data_handler.c','app.c','bc_ble_tx.c','bc_file_transfer.c']
entries={f.findtext('FileName'):resolve(f.findtext('FilePath')) for f in target.findall('./Groups/Group/Files/File')}
results=[]
for name in files:
    source=entries[name]
    command=['arm-none-eabi-gcc',*flags,'-c',str(source),'-o',str(output/(name+'.o'))]
    result=subprocess.run(command,capture_output=True)
    log=(result.stdout+result.stderr).decode('utf-8',errors='replace')
    (output/(name+'.log')).write_text(log)
    results.append({'file':name,'exit_code':result.returncode,'command':command,'log':str((output/(name+'.log')).relative_to(ROOT))})
    errors=[l for l in log.splitlines() if 'error:' in l]
    print(name, 'PASS' if result.returncode==0 else 'FAIL', '\n'.join(errors[:5]))
(output/'result.json').write_text(json.dumps(results,indent=2)+'\n')
raise SystemExit(any(r['exit_code'] for r in results))
