#!/usr/bin/env python3
"""Run exact ArmCC retirement code with modeled register IO; never hardware."""
import argparse
import json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_PC, UC_ARM_REG_LR
from qualify_armcc5_runtime import Artifact, Probe, require
from factory_retire_diagnostics import verify


class RegisterIO:
    """Model only the board IO boundary, using the pinned LSM6DSO register map."""
    def __init__(self, probe, bank=0, fault=0, wrong_id=False, drop=False):
        self.probe = probe
        self.registers = [bytearray((3 * 17 + r * 7 + b) & 255 for r in range(256)) for b in range(3)]
        self.registers[0][0x0f] = 0 if wrong_id else 0x6c
        self.registers[0][1] = (0, 0x80, 0x40)[bank]
        self.registers[0][0x10] = 0x40
        self.bank, self.fault, self.drop = bank, fault, drop
        self.calls = self.opens = self.closes = 0
        self.opened = False
        self.functions = {probe.artifact.sym("bc_g_sensor_i2c_" + name) & ~1: name
                          for name in ("read", "write", "open", "close")}
        self.hook = probe.cpu.hook_add(UC_HOOK_CODE, self.intercept)

    def intercept(self, cpu, address, size, data):
        name = self.functions.get(address)
        if name is None: return
        result = 0
        if name == "open":
            require(not self.opened, "Nested sensor bus open")
            self.opened = True; self.opens += 1
        elif name == "close":
            require(self.opened, "Closing an unopened sensor bus")
            self.opened = False; self.closes += 1
        else:
            slave, reg, buffer, length = [cpu.reg_read(r) for r in (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3)]
            require(self.opened and slave == 0xd4 and 0 < length < 256 and reg + length <= 256, "Invalid standard-1232 IO")
            self.calls += 1
            if self.calls != self.fault:
                result = 1
                if name == "read":
                    cpu.mem_write(buffer, bytes(self.registers[0 if reg+i == 1 else self.bank][reg+i] for i in range(length)))
                else:
                    for i, value in enumerate(cpu.mem_read(buffer, length)):
                        if self.bank == 0 and reg+i in (0x10, 0x11):
                            require(value & 0xf0 == 0, "Shutdown enabled an ODR")
                        if self.drop and self.bank == 0 and reg+i == 0x10: continue
                        self.registers[0 if reg+i == 1 else self.bank][reg+i] = value
                        if reg+i == 1: self.bank = 1 if value & 0x80 else 2 if value & 0x40 else 0
        cpu.reg_write(UC_ARM_REG_R0, result)
        cpu.reg_write(UC_ARM_REG_PC, cpu.reg_read(UC_ARM_REG_LR))

    def finish(self):
        self.probe.cpu.hook_del(self.hook)
        require(not self.opened and self.opens == self.closes == 1, "Sensor bus ownership leaked")


def qualify(source):
    stem = source / "firmware/BCL603S2X/app/project/mdk5/Objects/app"
    artifact = Artifact(stem.with_suffix(".axf"), stem.with_suffix(".bin"))
    retirement = verify(artifact)
    runs = []
    successful_calls = 0
    def run(bank=0, fault=0, wrong_id=False, drop=False):
        probe = Probe(artifact)
        io = RegisterIO(probe, bank, fault, wrong_id, drop)
        result = probe.call("app_factory_motion_off", limit=200000)
        io.finish()
        status = probe.read_u32("app_factory_motion_status")
        expected = 3 if wrong_id else 2 if fault or drop else 1
        require(status == expected, "False motion-shutdown status: " + str((bank, fault, wrong_id, drop, status)))
        if expected == 1:
            require(io.bank == 0 and not (io.registers[0][0x10] & 0xf0) and
                    not (io.registers[0][0x11] & 0xf0), "Sensor still sampling")
        runs.append({"bank": bank, "fault": fault, "wrongID": wrong_id, "dropWrite": drop,
                     "status": status, "ioCalls": io.calls, "stackBytes": result["stack_bytes"]})
        return io.calls
    for bank in range(3): successful_calls = max(successful_calls, run(bank=bank))
    for fault in range(1, successful_calls+1): run(fault=fault)
    run(wrong_id=True); run(drop=True)
    return {"status": "pass-exact-armcc-retirement-cpu-probes", **artifact.identity, **retirement,
            "scenarios": len(runs), "maxObservedShutdownStackBytes": max(r["stackBytes"] for r in runs),
            "runs": runs, "flashed": False, "physicallyQualified": False,
            "limits": ["Actual linked ARM shutdown and ST code; board IO is modeled.",
                       "No scheduler, physical bus, elapsed-time, battery or recovery proof."]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists(): parser.error("Use fresh evidence")
    result = qualify(args.source.resolve())
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k != "runs"}, indent=2))
