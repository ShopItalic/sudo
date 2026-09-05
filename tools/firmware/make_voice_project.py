#!/usr/bin/env python3
"""Derive a reviewable recording-only Keil target without rewriting vendor XML."""
import copy
import json
from pathlib import Path
import xml.etree.ElementTree as E

ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / 'firmware/BCL603S2X/app/project/mdk5'
source = E.parse(PROJECT / 'bc_ring_app.uvprojx')
root = copy.deepcopy(source.getroot())
targets = root.find('Targets')
assert targets is not None
original = next(t for t in targets if t.findtext('TargetName') == '1.23.2')
for t in list(targets):
    if t is not original:
        targets.remove(t)
original.find('TargetName').text = 'Sudo Voice 1.23.2'
# Keil resolves CMSIS/startup sources through RTE targetInfo names too.
# Preserve exactly the factory board's selections when renaming the target.
rte = root.find('RTE')
if rte is not None:
    for infos in rte.findall('.//targetInfos'):
        for info in list(infos):
            if info.get('name') == '1.23.2':
                info.set('name', 'Sudo Voice 1.23.2')
            else:
                infos.remove(info)
    for parent in reversed(list(rte.iter())):
        for child in list(parent):
            infos = child.find('targetInfos')
            if infos is not None and len(infos) == 0:
                parent.remove(child)

removed = []
for group in list(original.findall('./Groups/Group')):
    reason = None
    name = group.findtext('GroupName')
    if group.findtext('./GroupOption/CommonProperty/IncludeInBuild') == '0':
        reason = 'already disabled in supplier 1.23.2 target'
    elif name.startswith('opus/'):
        reason = 'Opus not selected; production recording uses ADPCM'
    if reason:
        removed.extend({'path':f.findtext('FilePath'),'reason':reason} for f in group.findall('./Files/File'))
        original.find('Groups').remove(group)
        continue
    files = group.find('Files')
    if files is None:
        continue
    for f in list(files):
        why = None
        if f.findtext('./FileOption/CommonProperty/IncludeInBuild') == '0':
            why = 'already disabled in supplier 1.23.2 target'
        if f.findtext('FileName') in ('app_ble_speed_handler.c', 'app_opus.c'):
            why = 'unused traffic generator or alternative audio encoder'
        if why:
            removed.append({'path':f.findtext('FilePath'),'reason':why})
            files.remove(f)
ble = next(g for g in original.findall('./Groups/Group') if g.findtext('GroupName') == 'BC_Ble').find('Files')
for name in ('bc_ble_tx.c','bc_file_transfer.c'):
    f = E.SubElement(ble, 'File')
    E.SubElement(f, 'FileName').text = name
    E.SubElement(f, 'FileType').text = '1'
    E.SubElement(f, 'FilePath').text = '..\\..\\..\\..\\bc_ros\\bc_module\\ble\\src\\'+name
controls = original.find('.//TargetArmAds/Cads/VariousControls')
controls.find('Define').text += ' SUDO_VOICE_ONLY'
# Remove stale options needed only by the pruned Opus and different IMU/NFC sources.
defines = controls.find('Define').text.split()
controls.find('Define').text = ' '.join(x for x in defines if x not in {
    'OPUS_BUILD','VAR_ARRAYS','FIXED_POINT','DISABLE_FLOAT_API','REMOVE_FOR_MALLOC','ICM42688P','ST25R200','BLE_POWER_TESTx'})
common = original.find('./TargetOption/TargetCommonOption')
common.find('OutputName').text = 'sudo_voice_candidate'
common.find('OutputDirectory').text = '..\\..\\..\\..\\..\\build\\firmware\\sudo_voice\\'
# Never invoke a supplier packaging/signing script as a build side effect.
for node in original.iter():
    if node.tag.startswith('RunUserProg'):
        node.text = '0'
# Fail generation if pruning disconnects a selected source or RTE startup.
for f in original.findall('./Groups/Group/Files/File'):
    assert (PROJECT / f.findtext('FilePath').replace('\\','/')).is_file(), f.findtext('FilePath')
refs = root.findall('./RTE//targetInfo')
assert refs and all(x.get('name') == 'Sudo Voice 1.23.2' for x in refs)
startup = root.findall('./RTE/files/file/instance')
assert len(startup) == 2
for instance in startup:
    assert (PROJECT / instance.text.replace('\\','/')).is_file(), instance.text
E.indent(root, space='  ')
output = PROJECT / 'sudo_voice.uvprojx'
E.ElementTree(root).write(output, encoding='utf-8', xml_declaration=True)
manifest = {
    'source_target':'1.23.2','target':'Sudo Voice 1.23.2',
    'source_project':'bc_ring_app.uvprojx','removed_entries':removed,
    'remaining_entries':sum(len(g.findall('./Files/File')) for g in original.findall('./Groups/Group')),
    'production_link_verified':False,
}
(ROOT/'firmware/voice-profile.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(f'Generated {output.relative_to(ROOT)}: {len(removed)} removed entries, {manifest["remaining_entries"]} remaining')
