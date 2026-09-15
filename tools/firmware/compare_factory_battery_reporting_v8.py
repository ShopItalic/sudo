#!/usr/bin/env python3
"""Compare linked P08/P09 notification code on a CPU model, never physical I/O.

Requires the same unicorn==2.1.4 / pyelftools==0.33 environment as
qualify_armcc5_runtime.py. Radio, notification permission and idle-timer calls
are intercepted. Instruction counts exclude those unchanged external services;
they are not elapsed-time, scheduler, radio, power or whole-task stack evidence.
"""
import argparse
import hashlib
import importlib.metadata
import json
from pathlib import Path
import struct

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE
from unicorn.arm_const import (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_SP,
                              UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_XPSR,
                              UC_CPU_ARM_CORTEX_M4)

RETURN = 0x100000
FUNCTIONS = ('app_package_precent_up', 'app_package_precent_status_up')


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


class Image:
    def __init__(self, stem, expected_version='6.0.3.3P08'):
        binary = stem.with_suffix('.bin').read_bytes()
        elf_path = stem.with_suffix('.axf')
        self.identity = {'binSha256': hashlib.sha256(binary).hexdigest(),
                         'elfSha256': hashlib.sha256(elf_path.read_bytes()).hexdigest()}
        require(expected_version in ('6.0.3.3P08', '6.0.3.3P09'), 'Unsupported image version')
        require(binary.count(expected_version.encode() + b'\0') == 2,
                'Expected ' + expected_version + ' identity')
        self.identity['version'] = expected_version
        with elf_path.open('rb') as stream:
            elf = ELFFile(stream)
            require(elf['e_machine'] == 'EM_ARM' and elf.little_endian,
                    'Expected little-endian ARM')
            segments = [(s['p_paddr'], s.data()) for s in elf.iter_segments()
                        if s['p_type'] == 'PT_LOAD' and s['p_filesz']]
            symbols = {}
            stacks = []
            for symbol in elf.get_section_by_name('.symtab').iter_symbols():
                symbols.setdefault(symbol.name, []).append(symbol['st_value'])
                if symbol.name == 'STACK' and symbol['st_info']['type'] == 'STT_SECTION':
                    stacks.append((symbol['st_value'], symbol['st_size']))
            require(len(stacks) == 1, 'Missing/ambiguous linked stack section')
            self.stack_base = stacks[0][0]
            self.stack_top = self.stack_base + stacks[0][1]
        self.symbols = symbols
        require(0x20004758 <= self.stack_base < self.stack_top <= 0x20040000,
                'Invalid linked stack range')
        require(struct.unpack_from('<I', binary)[0] == self.stack_top,
                'Initial vector differs from stack section')
        load = bytearray(len(binary))
        for address, data in segments:
            require(0x27000 <= address < address + len(data) <= 0x27000 + len(binary) <= 0xE0000,
                    'Invalid load range')
            load[address - 0x27000:address - 0x27000 + len(data)] = data
        require(load == binary, 'ELF/BIN mismatch')
        self.cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
        self.cpu.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_M4)
        self.cpu.mem_map(0, 0x100000)
        self.cpu.mem_map(0x20000000, 0x40000)
        for address, data in segments:
            self.cpu.mem_write(address, data)
        self.reset_registers(0)
        # Initialize RAM using this exact image's compressed scatter loader.
        self.cpu.emu_start(self.sym('__main') | 1, self.sym('main') & ~1, count=1000000)
        require(self.cpu.reg_read(UC_ARM_REG_PC) == self.sym('main') & ~1,
                'C startup did not reach main')
        self.reset_registers(0)
        self.cpu.emu_start(self.sym('app_package_init') | 1, RETURN, count=10000)
        require(self.cpu.reg_read(UC_ARM_REG_PC) == RETURN, 'Packet initialization failed')
        self.ram = bytes(self.cpu.mem_read(0x20000000, 0x40000))

    def sym(self, name):
        values = self.symbols.get(name, [])
        require(len(values) == 1, 'Missing/ambiguous symbol: ' + name)
        return values[0]

    def reset_registers(self, value):
        self.cpu.reg_write(UC_ARM_REG_SP, self.stack_top - 32)
        self.cpu.reg_write(UC_ARM_REG_R0, value)
        self.cpu.reg_write(UC_ARM_REG_LR, RETURN | 1)
        self.cpu.reg_write(UC_ARM_REG_XPSR, 0x01000000)

    def notify(self, function, value, connected, allowed):
        cpu = self.cpu
        cpu.mem_write(0x20000000, self.ram)
        self.reset_registers(value)
        result = {'events': [], 'instructions': 0, 'stackBytes': 0}
        intercepts = {self.sym(name) & ~1: name for name in
                      ('bc_ble_connect_status', 'app_ble_notify_allowed',
                       'bc_ble_send', 'app_connect_idie_timer_start')}

        def execute(uc, address, size, context):
            sp = uc.reg_read(UC_ARM_REG_SP)
            require(self.stack_base <= sp <= self.stack_top - 32, 'Probe stack bounds')
            result['stackBytes'] = max(result['stackBytes'], self.stack_top - 32 - sp)
            name = intercepts.get(address)
            if name is None:
                result['instructions'] += 1
                return
            if name == 'bc_ble_connect_status':
                result['events'].append(['connected', connected])
                uc.reg_write(UC_ARM_REG_R0, int(connected))
            elif name == 'app_ble_notify_allowed':
                result['events'].append(['allowed', allowed])
                uc.reg_write(UC_ARM_REG_R0, int(allowed))
            elif name == 'bc_ble_send':
                length = uc.reg_read(UC_ARM_REG_R1)
                require(length in (5, 6), 'Unexpected notification length')
                data = bytes(uc.mem_read(uc.reg_read(UC_ARM_REG_R0), length))
                result['events'].append(['send', data.hex()])
            else:
                result['events'].append(['timer', uc.reg_read(UC_ARM_REG_R0)])
            uc.reg_write(UC_ARM_REG_PC, uc.reg_read(UC_ARM_REG_LR))

        hook = cpu.hook_add(UC_HOOK_CODE, execute)
        try:
            cpu.emu_start(self.sym(function) | 1, RETURN, count=10000)
        finally:
            cpu.hook_del(hook)
        require(cpu.reg_read(UC_ARM_REG_PC) == RETURN, 'Notification failed to return')
        require(cpu.reg_read(UC_ARM_REG_SP) == self.stack_top - 32, 'Unbalanced stack')
        return result


def compare(before, after, after_version='6.0.3.3P08'):
    original, compact = Image(before), Image(after, after_version)
    rows = []
    count = 0
    for kind, function in enumerate(FUNCTIONS):
        values = range(256) if kind == 0 else (0, 1, 100, 101, 102, 255, 256, 257,
                                               0x7FFF, 0x8000, 0xFF00, 0xFFFF)
        for connected, allowed in ((False, True), (True, False), (True, True)):
            observed = set()
            for value in values:
                a = original.notify(function, value, connected, allowed)
                b = compact.notify(function, value, connected, allowed)
                require(a['events'] == b['events'], f'Linked behavior changed: {function} {value}')
                require(b['instructions'] <= a['instructions'], 'Instruction count increased')
                require(b['stackBytes'] <= a['stackBytes'], 'Stack use increased')
                observed.add((a['instructions'], b['instructions'], a['stackBytes'], b['stackBytes']))
                count += 1
            rows.append({'function': function, 'connected': connected, 'allowed': allowed,
                         'costs': [dict(zip(('beforeInstructions', 'afterInstructions',
                                            'beforeStackBytes', 'afterStackBytes'), row))
                                   for row in sorted(observed)]})
    return {'status': 'pass', 'before': original.identity, 'after': compact.identity,
            'comparisonPairs': count, 'rows': rows, 'actualCStartupExecuted': True,
            'scope': 'Linked notification code only; radio, permission and timer services intercepted. '
                     'Instruction counts are not cycles, elapsed time or power. No scheduler, interrupts or physical I/O.',
            'physicallyQualified': False}


if __name__ == '__main__':
    for dependency, version in (('unicorn', '2.1.4'), ('pyelftools', '0.33')):
        require(importlib.metadata.version(dependency) == version, 'Unexpected ' + dependency + ' version')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before', type=Path, required=True, help='Prior linked image stem (without extension)')
    parser.add_argument('--after', type=Path, required=True, help='Compact linked image stem')
    parser.add_argument('--after-version', choices=('6.0.3.3P08', '6.0.3.3P09'),
                        default='6.0.3.3P08', help='Require this exact candidate image identity')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    require(not args.output.exists(), 'Preserve existing evidence; choose a new output')
    result = compare(args.before, args.after, args.after_version)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
