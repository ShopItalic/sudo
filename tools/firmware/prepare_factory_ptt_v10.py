#!/usr/bin/env python3
"""Prepare P10 automatic receipt cleanup from verified factory source. Never flash."""
import argparse
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET
import prepare_factory_ptt_v9 as prior
import factory_cleanup_v10 as cleanup
from prepare_vendor_baseline import prepare, ROOT

VERSION='6.0.3.3P10'
OVERLAY=ROOT/'firmware/factory_ptt_v10'
APP=prior.APP
PATCHES=dict(prior.PATCHES)
_file, _ble, _config = PATCHES[APP+'app_ppg_file_data_handler.c'], PATCHES[APP+'app_ble_handler.c'], PATCHES[prior.base.CONFIG]
PATCHES[APP+'app_ppg_file_data_handler.c']=lambda s: cleanup.patch_file(_file(s))
PATCHES[APP+'app_ble_handler.c']=lambda s: cleanup.patch_ble(_ble(s))
PATCHES[prior.base.CONFIG]=lambda s: _config(s).replace('6.0.3.3P09','6.0.3.3P10')
PATCHES[cleanup.SHA]=cleanup.patch_sha

def apply_overlay(destination,baseline):
    old=(prior.PATCHES,prior.OVERLAY,prior.VERSION)
    try:
        prior.PATCHES,prior.OVERLAY,prior.VERSION=PATCHES,OVERLAY,VERSION
        result=prior.apply_overlay(destination,baseline)
    finally:
        prior.PATCHES,prior.OVERLAY,prior.VERSION=old
    old_project=destination/result['project']
    tree=ET.parse(old_project); group=tree.find('./Targets/Target/Groups/Group[last()]')
    group.find('GroupName').text='Factory P10 receipt cleanup'
    for name in cleanup.ADDED:
        content=(OVERLAY/name).read_bytes(); (destination/APP/name).write_bytes(content)
        result['changes'].append({'path':APP+name,'added':True,'sha256':hashlib.sha256(content).hexdigest()})
        if name.endswith('.c'):
            f=ET.SubElement(group.find('Files'),'File')
            ET.SubElement(f,'FileName').text=name; ET.SubElement(f,'FileType').text='1'
            ET.SubElement(f,'FilePath').text='..\\..\\..\\..\\bc_ros\\bc_application\\'+name
    # Compile supplier SHA-256 with the unsigned word-assembly fix and its license.
    f=ET.SubElement(group.find('Files'),'File')
    ET.SubElement(f,'FileName').text='sha256.c'; ET.SubElement(f,'FileType').text='1'
    ET.SubElement(f,'FilePath').text='..\\..\\components\\libraries\\sha256\\sha256.c'
    inc=tree.find('./Targets/Target/TargetOption/TargetArmAds/Cads/VariousControls/IncludePath')
    inc.text=(inc.text or '')+';..\\..\\components\\libraries\\sha256'
    project=old_project.with_name('factory_ptt_p10_build_only.uvprojx')
    tree.write(project,encoding='utf-8',xml_declaration=True)
    if old_project.name!='factory_ptt_p09_build_only.uvprojx': raise ValueError('Unexpected intermediate project')
    old_project.unlink() # generated intermediate inside a fresh build tree only
    result.update(project=str(project.relative_to(destination)),projectSha256=hashlib.sha256(project.read_bytes()).hexdigest())
    result['deletion'].update(stage='persistent-receiver-receipt-cleanup',receiverReceiptProtocol=True,
        automaticCleanup=True,command='0x84/0x20-0x24',identity='128-bit persisted random file attribute',
        checksum='SHA-256',receiptAttribute='0xD0',sliceBytes=512,maxFailureAttempts=5,
        qualificationPending=['demonstrated recovery on the identified spare',
            'physical runtime, storage, power-loss and recording qualification'])
    result['p10']={'requestsAndRepliesMaxBytes':20,'bleWorkerStackBytes':4096,'legacyDeleteDisabled':True,
        'explicitFormatDisabled':True,'receiptPersistsBeforeAck':True,'mountFailureAutoformat':False}
    (destination/'ptt-preparation.json').write_text(json.dumps(result,indent=2)+'\n')
    (destination/'NON-FLASHABLE.txt').write_text('Unsigned P10 development image. Physical qualification pending.\n')
    return result

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();print(json.dumps(apply_overlay(args.output.resolve(),prepare(args.output.resolve())),indent=2))
