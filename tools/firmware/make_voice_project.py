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
        reason = 'supplier opus-1.5.2 not selected; Sudo Voice compiles the pinned opus-1.6.1 subset'
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
        if f.findtext('FileName') in ('app_pdm_handler.c',
                                      'app_ppg_file_data_handler.c'):
            why = 'replaced by single Sudo recording/archive worker'
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
# The supplier's excluded opus-1.5.2 include paths must not shadow the pinned
# 1.6.1 headers selected below.
controls.find('IncludePath').text = ';'.join(
    p for p in controls.find('IncludePath').text.split(';') if 'opus-1.5.2' not in p)
recording = E.SubElement(original.find('Groups'), 'Group')
E.SubElement(recording, 'GroupName').text = 'Sudo Recording'
recording_files = E.SubElement(recording, 'Files')
# Keep this tuple explicit so a supplier-side source added to the directory
# cannot silently enter the candidate target. These are the portable recording
# sources currently reviewed for Sudo, including the staged store/service API.
recording_sources = (
    'bc_audio_format.c',
    'bc_capture.c',
    'bc_opus_encoder.c',
    'bc_opus_stream.c',
    'bc_rec_store.c',
    'bc_recording.c',
    'bc_resampler.c',
    'bc_touch_report.c',
    'bc_touch_tuning.c',
    'bc_voice_gesture.c',
    'bc_voice_legacy_archive.c',
    'bc_voice_service.c',
    'bc_voice_wire.c',
)
for name in recording_sources:
    f = E.SubElement(recording_files, 'File')
    E.SubElement(f, 'FileName').text = name
    E.SubElement(f, 'FileType').text = '1'
    E.SubElement(f, 'FilePath').text = '..\\..\\..\\..\\bc_ros\\bc_module\\recording\\' + name
f = E.SubElement(recording_files, 'File')
E.SubElement(f, 'FileName').text = 'app_sudo_capture.c'
E.SubElement(f, 'FileType').text = '1'
E.SubElement(f, 'FilePath').text = '..\\..\\..\\..\\bc_ros\\bc_application\\app_sudo_capture.c'
f = E.SubElement(recording_files, 'File')
E.SubElement(f, 'FileName').text = 'app_sudo_voice.c'
E.SubElement(f, 'FileType').text = '1'
E.SubElement(f, 'FilePath').text = '..\\..\\..\\..\\bc_ros\\bc_application\\app_sudo_voice.c'
f = E.SubElement(recording_files, 'File')
E.SubElement(f, 'FileName').text = 'bc_battery_filter.c'
E.SubElement(f, 'FileType').text = '1'
E.SubElement(f, 'FilePath').text = '..\\..\\..\\..\\bc_ros\\bc_module\\pmic\\bc_battery_filter.c'
# Pinned upstream libopus 1.6.1: the portable fixed-point encoder/decoder
# subset verified by tools/firmware/import_opus.py. The source lists come from
# the upstream .mk files so a review can diff them against the release.
OPUS_ROOT = ROOT / 'firmware/bc_ros/bc_module/opus/opus-1.6.1'
def opus_list(mk, name):
    lines = (OPUS_ROOT / mk).read_text().splitlines()
    out, active = [], False
    for line in lines:
        if line.startswith(f'{name} ='):
            active = True
            continue
        if active:
            if not line.strip():
                break
            out.append(line.strip().rstrip('\\').strip())
    return out
opus_sources = []
for mk, name in (('opus_sources.mk', 'OPUS_SOURCES'), ('celt_sources.mk', 'CELT_SOURCES'),
                 ('silk_sources.mk', 'SILK_SOURCES'), ('silk_sources.mk', 'SILK_SOURCES_FIXED')):
    for source in opus_list(mk, name):
        if any(x in source for x in ('multistream', 'projection', 'mapping_matrix')):
            continue
        opus_sources.append(source)
opus_group = E.SubElement(original.find('Groups'), 'Group')
E.SubElement(opus_group, 'GroupName').text = 'Sudo Opus 1.6.1'
opus_files = E.SubElement(opus_group, 'Files')
for source in opus_sources:
    f = E.SubElement(opus_files, 'File')
    E.SubElement(f, 'FileName').text = source.rsplit('/', 1)[-1]
    E.SubElement(f, 'FileType').text = '1'
    E.SubElement(f, 'FilePath').text = '..\\..\\..\\..\\bc_ros\\bc_module\\opus\\opus-1.6.1\\' + source.replace('/', '\\')
opus_scratch = None
for line in (ROOT / 'firmware/bc_ros/bc_module/recording/bc_opus_profile.h').read_text().splitlines():
    if line.startswith('#define BC_OPUS_SCRATCH_BYTES '):
        opus_scratch = int(line.split()[2].rstrip('U'))
assert opus_scratch is not None
controls.find('IncludePath').text += ';..\\..\\..\\..\\bc_ros\\bc_module\\recording'
for include in ('include', 'celt', 'silk', 'silk\\fixed', 'src'):
    controls.find('IncludePath').text += ';..\\..\\..\\..\\bc_ros\\bc_module\\opus\\opus-1.6.1\\' + include
controls.find('IncludePath').text += ';..\\..\\..\\..\\bc_ros\\bc_module\\recording\\opus_support'
controls.find('Define').text += ' SUDO_VOICE_ONLY'
# Remove stale options needed only by the pruned Opus and different IMU/NFC sources.
defines = controls.find('Define').text.split()
controls.find('Define').text = ' '.join(x for x in defines if x not in {
    'OPUS_BUILD','VAR_ARRAYS','FIXED_POINT','DISABLE_FLOAT_API','REMOVE_FOR_MALLOC','ICM42688P','ST25R200','BLE_POWER_TESTx'})
# Sudo Opus profile: portable fixed-point C, no float API, worker-owned
# pseudostack sized by bc_opus_profile.h, Sudo allocation hook.
controls.find('Define').text += (' OPUS_BUILD FIXED_POINT DISABLE_FLOAT_API NONTHREADSAFE_PSEUDOSTACK'
                                 f' CUSTOM_SUPPORT GLOBAL_STACK_SIZE={opus_scratch}')
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
