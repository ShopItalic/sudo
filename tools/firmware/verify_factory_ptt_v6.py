#!/usr/bin/env python3
"""Verify P06 factory PTT source isolation and ArmCC5 load bounds; never flash."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
ALLOWED = {
    'bc_ros/bc_config/ring_config.h',
    'bc_ros/bc_module/ble/src/bc_ble.c',
    'bc_ros/bc_module/led/bc_ic_led.c',
    'bc_ros/bc_module/motor/bc_linear_motor.c',
    'bc_ros/bc_application/app_ble_handler.c',
    'bc_ros/bc_application/app_linear_motor_handler.c',
    'bc_ros/bc_application/app_touch_button_handler.c',
    'bc_ros/bc_device/touch_button/IQS7211E/IQS7211E.c',
    'bc_ros/bc_module/queue/bc_queue.c',
    'bc_ros/bc_module/queue/bc_queue.h',
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def verify(source, elf_path, binary_path):
    from elftools.elf.elffile import ELFFile
    preparation = json.loads((source / 'ptt-preparation.json').read_text())
    manifest = json.loads(subprocess.check_output(
        ['git', 'show', '102bfd2:firmware/source-manifest.json'], cwd=ROOT))
    changed, missing, generated_comments = [], [], []
    overlays = {c['path'].removeprefix('firmware/'): c for c in preparation['changes']}
    for item in manifest['files']:
        path = source / 'firmware' / item['path']
        if not path.is_file():
            missing.append(item['path'])
        elif sha(path.read_bytes()) != item['sha256']:
            if item['path'].endswith('/RTE/_1.23.2/RTE_Components.h'):
                original = subprocess.check_output(['git', 'show', '102bfd2:firmware/' + item['path']], cwd=ROOT)
                current = path.read_bytes()
                original = original.replace(b'\r\n', b'\n').strip()
                current = current.replace(b'\r\n', b'\n').strip()
                # uVision rewrites its generated project-name/version comment.
                # The C/preprocessor body must match after newline normalization.
                if (original.startswith(b'/*') and current.startswith(b'/*') and
                    b'*/' in original and b'*/' in current and
                    original.split(b'*/', 1)[1] == current.split(b'*/', 1)[1]):
                    generated_comments.append(item['path'])
                    continue
            changed.append(item['path'])
            expected = overlays.get(item['path'], {}).get('afterSha256')
            if sha(path.read_bytes()) != expected:
                raise ValueError('Unexpected source change: ' + item['path'])
    if missing or set(changed) != ALLOWED:
        raise ValueError(f'Unexpected factory delta: changed={changed}, missing={missing}')
    for relative, item in overlays.items():
        if item.get('added') and sha((source / 'firmware' / relative).read_bytes()) != item['sha256']:
            raise ValueError('Added source changed after preparation: ' + relative)
        if item.get('added') and sha((ROOT / 'firmware/factory_ptt_v6' / Path(relative).name).read_bytes()) != item['sha256']:
            raise ValueError('Active overlay differs from compiled source: ' + relative)
    binary = binary_path.read_bytes()
    if binary.count(b'6.0.3.3P06\0') != 2 or b'6.0.3.3Z62\0' in binary:
        raise ValueError('Incorrect factory PTT build identity')
    initial_sp, reset = struct.unpack('<II', binary[:8])
    if not 0x20000000 < initial_sp <= 0x20040000 or not reset & 1:
        raise ValueError('Invalid ARM vectors')
    if not 0x27000 <= (reset & ~1) < 0x27000 + len(binary) <= 0xe0000:
        raise ValueError('Reset or application outside permitted flash')
    load = bytearray(len(binary))
    ram_end = 0
    with elf_path.open('rb') as stream:
        elf = ELFFile(stream)
        if elf['e_machine'] != 'EM_ARM':
            raise ValueError('Expected ARM ELF')
        for segment in elf.iter_segments():
            if segment['p_type'] != 'PT_LOAD':
                continue
            addr, data = segment['p_paddr'], segment.data()
            if data:
                if not 0x27000 <= addr < addr + len(data) <= 0x27000 + len(binary):
                    raise ValueError('ELF load segment outside application image')
                load[addr-0x27000:addr-0x27000+len(data)] = data
        # ArmCC scatter-loaded ELF uses one combined PT_LOAD whose memsz is
        # not a contiguous virtual range. RAM placement lives in SHF_ALLOC sections.
        for section in elf.iter_sections():
            if not section['sh_flags'] & 2:
                continue
            vaddr, size = section['sh_addr'], section['sh_size']
            if vaddr >= 0x20000000:
                if not 0x20004758 <= vaddr < vaddr + size <= 0x20040000:
                    raise ValueError('RAM segment exceeds physical or SoftDevice boundary')
                ram_end = max(ram_end, vaddr + size)
    if ram_end == 0 or initial_sp != ram_end:
        raise ValueError('RAM layout or initial stack could not be verified')
    if load != binary:
        raise ValueError('ELF load bytes differ from BIN')
    stack_report = elf_path.with_suffix('.htm').read_text()
    stacks = {}
    for name, budget in (('app_touch_event_handler_thread', 1024),
                         ('app_ble_recv_handler_thread', 2048),
                         ('bc_ic_led_handler_thread', 512)):
        match = re.search(r'</a>' + name + r'</STRONG>.*?Max Depth = (\d+)', stack_report, re.S)
        if not match:
            raise ValueError('Missing linked stack chain: ' + name)
        depth = int(match[1])
        stacks[name] = {'allocatedBytes': budget, 'staticCallChainBytes': depth,
                        'exceptionReserveBytes': 256, 'remainingBytes': budget-depth-256}
        if depth + 256 > budget:
            raise ValueError(f'{name} exceeds stack: {stacks[name]}')
    return {'status': 'pass-source-isolation-and-load-bounds',
            'verifiedFactoryFiles': len(manifest['files']), 'changedFactoryFiles': changed,
            'generatedCommentOnlyFiles': generated_comments,
            'version': preparation['version'], 'bytes': len(binary), 'sha256': sha(binary),
            'elfSha256': sha(elf_path.read_bytes()), 'flashEnd': hex(0x27000 + len(binary)),
            'ramEnd': hex(ram_end), 'initialSP': hex(initial_sp),
            'taskStacks': stacks,
            'stackLimit': 'Static linker analysis; dynamic stack and heap high-water require hardware',
            'bootloaderAndSensorConfigurationUnchanged': True,
            'signed': False, 'flashed': False, 'physicallyQualified': False, 'flashable': False}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--elf', type=Path, required=True)
    parser.add_argument('--bin', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('Evidence already exists; choose a new file')
    result = verify(args.source, args.elf, args.bin)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
