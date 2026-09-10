#!/usr/bin/env python3
"""Reproduce runtime defects in the preserved S05 CI binary, without hardware.

Requires unicorn==2.1.4 and pyelftools==0.33. Example:
  python reproduce_s05_runtime_failures.py --elf sudo_voice.elf \
      --bin sudo_voice.bin --output reproduction.json

This is a forensic reproducer for one hash-pinned, known-bad artifact, not a
qualification test for future firmware. A successful exit means the defects
were reproduced. No target instructions are patched, stubbed, or bypassed.
"""

import argparse
from collections import deque
import hashlib
import json
from pathlib import Path
import struct

from elftools.elf.elffile import ELFFile
from unicorn import (
    Uc, UcError, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS,
    UC_HOOK_CODE, UC_HOOK_MEM_WRITE,
)
from unicorn.arm_const import (
    UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_SP, UC_ARM_REG_LR,
    UC_ARM_REG_R2, UC_ARM_REG_R3,
    UC_ARM_REG_XPSR, UC_ARM_REG_IPSR, UC_ARM_REG_PC,
    UC_CPU_ARM_CORTEX_M4,
)

ELF_SHA256 = "b950481058842cf4335a4b8429d2ac4e1a278a832a95f393986ad42ed7b3b68d"
BIN_SHA256 = "e99d68e970476da98034e47c6f6a4872766f490be2fe7ea3ea55bf0c4eae3f3d"
BLE_HANDLER = 0x33304
RTC_CALLBACK = 0x2F908
MOTOR_TASK = 0x292C8
MOTOR_FIRST_LOG_RETURN = 0x29304
HARDWARE_CHECK_TASK = 0x28F18
HARDWARE_CHECK_FIRST_LOG_RETURN = 0x28F38
RETURN_SENTINEL = 0x100000
STACK_TOP = 0x20040000


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def load_artifact(elf_path, bin_path):
    require(hashlib.sha256(elf_path.read_bytes()).hexdigest() == ELF_SHA256,
            "ELF differs from the preserved S05 artifact; fixed addresses are unsafe")
    require(hashlib.sha256(bin_path.read_bytes()).hexdigest() == BIN_SHA256,
            "BIN differs from the preserved S05 artifact")
    with elf_path.open("rb") as stream:
        elf = ELFFile(stream)
        segments = [
            {"vaddr": s["p_vaddr"], "paddr": s["p_paddr"], "data": s.data()}
            for s in elf.iter_segments() if s["p_type"] == "PT_LOAD"
        ]
        all_symbols = list(elf.get_section_by_name(".symtab").iter_symbols())
        symbols = {s.name: s["st_value"] & ~1 for s in all_symbols
                   if s["st_info"]["type"] == "STT_FUNC"}
        vector = struct.unpack("<128I", elf.get_section_by_name(".isr_vector").data())
    for name, index in [("Reset_Handler", 1), ("SWI2_EGU2_IRQHandler", 38),
                        ("RTC2_IRQHandler", 52)]:
        require(vector[index] == symbols[name] | 1, f"Unexpected {name} vector")
    require(vector[0] == STACK_TOP, "Unexpected initial stack")
    flash = [s for s in segments if s["data"]]
    start = min(s["paddr"] for s in flash)
    end = max(s["paddr"] + len(s["data"]) for s in flash)
    reconstructed = bytearray(end - start)
    for s in flash:
        offset = s["paddr"] - start
        reconstructed[offset:offset + len(s["data"])] = s["data"]
    require(reconstructed == bin_path.read_bytes(), "ELF load bytes do not match BIN")
    return segments, symbols


def run(segments, symbols, name, entry, ipsr, warm_stdio=False,
        rtc_rollover=False, motor=False, arguments=None, input_bytes=None,
        task_descriptor=None, stop_after=None, stack_budget_descriptor=None,
        instruction_limit=50000):
    cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    cpu.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_M4)
    cpu.mem_map(0, 0x100000)
    cpu.mem_map(0x20000000, 0x40000)
    cpu.mem_map(0xE000E000, 0x2000)
    cpu.mem_map(0xE0001000, 0x1000)  # DWT counters; no elapsed-time model.
    # ELF VMA initialization models completed .data copy and zeroed BSS.
    for segment in segments:
        if segment["data"]:
            cpu.mem_write(segment["vaddr"], segment["data"])
    task_model = None
    if task_descriptor is not None:
        # Execute the real kernel's task creation and task-selection routines
        # so delay/list bookkeeping and the task's Newlib reent exist. The CPU
        # model does not deliver PendSV or advance time: the task's delay call
        # executes, but its wait interval/context switch are not simulated.
        require(ipsr == 0, "Task setup is only valid in thread context")
        descriptor = bytes(cpu.mem_read(task_descriptor, 64))
        words = struct.unpack_from("<H", descriptor, 40)[0]
        priority = struct.unpack_from("<I", descriptor, 48)[0]
        task_entry = struct.unpack_from("<I", descriptor, 56)[0]
        require(task_entry == entry | 1, "Task descriptor entry mismatch")
        cpu.mem_write(STACK_TOP - 32, struct.pack("<II", priority, 0x2003C000))
        for reg, value in ((UC_ARM_REG_R0, task_entry),
                           (UC_ARM_REG_R1, task_descriptor),
                           (UC_ARM_REG_R2, words), (UC_ARM_REG_R3, 0),
                           (UC_ARM_REG_SP, STACK_TOP - 32),
                           (UC_ARM_REG_LR, RETURN_SENTINEL | 1),
                           (UC_ARM_REG_XPSR, 0x01000000)):
            cpu.reg_write(reg, value)
        cpu.emu_start(symbols["xTaskCreate"] | 1, RETURN_SENTINEL, count=200000)
        require(cpu.reg_read(UC_ARM_REG_PC) == RETURN_SENTINEL and
                cpu.reg_read(UC_ARM_REG_R0) == 1, "Modeled task creation failed")
        cpu.reg_write(UC_ARM_REG_SP, STACK_TOP - 32)
        cpu.reg_write(UC_ARM_REG_LR, RETURN_SENTINEL | 1)
        cpu.emu_start(symbols["vTaskSwitchContext"] | 1, RETURN_SENTINEL, count=50000)
        require(cpu.reg_read(UC_ARM_REG_PC) == RETURN_SENTINEL,
                "Modeled task selection failed")
        task_model = {"descriptor": hex(task_descriptor),
                      "name": descriptor[:40].split(b"\0")[0].decode(),
                      "configured_stack_bytes": words * 4,
                      "priority": priority,
                      "kernel_task_and_reent_initialized": True,
                      "delay_time_and_pendsv_not_simulated": True}
    if warm_stdio:
        # Use the actual linked __sinit in task context; no libc shim.
        reent = struct.unpack("<I", bytes(cpu.mem_read(0x20005D38, 4)))[0]
        cpu.reg_write(UC_ARM_REG_R0, reent)
        cpu.reg_write(UC_ARM_REG_SP, STACK_TOP)
        cpu.reg_write(UC_ARM_REG_LR, RETURN_SENTINEL | 1)
        cpu.reg_write(UC_ARM_REG_XPSR, 0x01000000)
        cpu.emu_start(symbols["__sinit"] | 1, RETURN_SENTINEL, count=50000)
        require(cpu.reg_read(UC_ARM_REG_PC) == RETURN_SENTINEL,
                "Warm stdio initialization did not return")
    if rtc_rollover:
        cpu.mem_write(0x20007110, struct.pack("<H", 7))  # eighth 125 ms tick
    # BLE_GAP_EVT_CONNECTED = 0x10 in the project's Nordic headers.
    cpu.mem_write(0x2003C000, struct.pack("<HH", 0x10, 64) + bytes(60))
    if input_bytes is not None:
        require(len(input_bytes) <= 1024, "Modeled callback input is too large")
        cpu.mem_write(0x2003C000, input_bytes)
    args = arguments if arguments is not None else (
        0x2003C000 if entry == BLE_HANDLER else 0, 0)
    cpu.reg_write(UC_ARM_REG_R0, args[0])
    cpu.reg_write(UC_ARM_REG_R1, args[1])
    cpu.reg_write(UC_ARM_REG_SP, STACK_TOP)
    cpu.reg_write(UC_ARM_REG_LR, RETURN_SENTINEL | 1)
    cpu.reg_write(UC_ARM_REG_XPSR, 0x01000000 | ipsr)
    cpu.reg_write(UC_ARM_REG_IPSR, ipsr)
    interesting = {v: k for k, v in symbols.items()
                   if k.startswith("__retarget_lock_") or k in {
                       "bc_ble_new", "xTaskGetTickCount", "printf", "_vfprintf_r",
                       "__sinit", "__NVIC_SystemReset", "bsp_rtc_callback",
                       "bsp_rtc_get_date_time", "localtime", "localtime_r", "__tz_lock",
                       "app_linear_motor_handler_thread",
                       "app_hardware_check_handler_thread", "vTaskDelay",
                       "linear_motor_pwm_callback", "bc_g_sensor_int_callback",
                       "pm_evt_handler", "bc_ble_recv",
                   }}
    interesting[BLE_HANDLER] = "bc_ble.c:ble_evt_handler"
    result = {"name": name, "entry": hex(entry), "ipsr": ipsr,
              "warm_stdio": warm_stdio, "rtc_rollover": rtc_rollover,
              "trace": [], "reset_requested": False, "instructions": 0,
              "stack_bytes": 0, "stack_write_depth_bytes": 0}
    if task_model is not None:
        result["task_model"] = task_model
    recent = deque(maxlen=18)

    def code(uc, address, size, context):
        recent.append(hex(address))
        result["instructions"] += 1
        result["stack_bytes"] = max(result["stack_bytes"],
                                    STACK_TOP - uc.reg_read(UC_ARM_REG_SP))
        if address in interesting:
            result["trace"].append({"pc": hex(address), "function": interesting[address],
                                    "ipsr": uc.reg_read(UC_ARM_REG_IPSR)})
        if address == stop_after or (motor and address == MOTOR_FIRST_LOG_RETURN):
            result["first_log_returned"] = True
            uc.emu_stop()

    def write(uc, access, address, size, value, context):
        if STACK_TOP - 8192 <= address < STACK_TOP:
            result["stack_write_depth_bytes"] = max(
                result["stack_write_depth_bytes"], STACK_TOP - address)
        if address == 0xE000ED0C and value & 4:
            result.update(reset_requested=True, reset_value=hex(value),
                          reset_pc=hex(uc.reg_read(UC_ARM_REG_PC)),
                          last_instructions=list(recent))
            uc.emu_stop()

    cpu.hook_add(UC_HOOK_CODE, code)
    cpu.hook_add(UC_HOOK_MEM_WRITE, write)
    cpu.ctl_flush_tb()  # Include blocks translated before hooks during __sinit.
    try:
        cpu.emu_start(entry | 1, RETURN_SENTINEL, count=instruction_limit)
    except UcError as error:
        raise RuntimeError(f"{name}: emulation error at "
                           f"{cpu.reg_read(UC_ARM_REG_PC):#x}: {error}") from error
    result["returned"] = cpu.reg_read(UC_ARM_REG_PC) == RETURN_SENTINEL
    result["final_pc"] = hex(cpu.reg_read(UC_ARM_REG_PC))
    result["return_r0"] = cpu.reg_read(UC_ARM_REG_R0)
    if motor:
        # app_linear_motor_time_create loads the depth from this actual ELF
        # descriptor: the PC-relative literal at 0x2945c, then uint16 +0x28.
        descriptor = struct.unpack("<I", bytes(cpu.mem_read(0x2945C, 4)))[0]
        words = struct.unpack("<H", bytes(cpu.mem_read(descriptor + 0x28, 2)))[0]
        require(words == 128, "Unexpected motor stack descriptor")
        result["configured_stack_bytes"] = words * 4
        result["write_beyond_stack_bytes"] = max(
            0, result["stack_write_depth_bytes"] - result["configured_stack_bytes"])
    if task_model is not None:
        result["configured_stack_bytes"] = task_model["configured_stack_bytes"]
        result["write_beyond_stack_bytes"] = max(
            0, result["stack_write_depth_bytes"] - result["configured_stack_bytes"])
    if stack_budget_descriptor is not None:
        descriptor = bytes(cpu.mem_read(stack_budget_descriptor, 64))
        words = struct.unpack_from("<H", descriptor, 40)[0]
        require(words == 128, "Unexpected motion task stack descriptor")
        require(struct.unpack_from("<I", descriptor, 56)[0] ==
                symbols["app_g_sensor_storage_handler_thread"] | 1,
                "Unexpected motion task entry")
        result["configured_stack_bytes"] = words * 4
        result["write_beyond_stack_bytes"] = max(
            0, result["stack_write_depth_bytes"] - words * 4)
        result["measurement_scope"] = (
            "Timer-creation helper alone, excluding its motion-task/driver caller frames; "
            "successful allocation modeled, no I2C/peripheral initialization executed")
    return result


def task_descriptors(segments, symbols):
    """Read named bc_rtos_thread_struct records from initialized ELF RAM."""
    entries = {v | 1: k for k, v in symbols.items() if "thread" in k}
    result = []
    for segment in segments:
        if not 0x20000000 <= segment["vaddr"] < 0x20040000:
            continue
        data = segment["data"]
        for offset in range(0, len(data) - 63, 4):
            entry = struct.unpack_from("<I", data, offset + 56)[0]
            if entry not in entries:
                continue
            name = data[offset:offset + 40].split(b"\0")[0]
            words = struct.unpack_from("<H", data, offset + 40)[0]
            priority = struct.unpack_from("<I", data, offset + 48)[0]
            if not name or any(c < 32 or c > 126 for c in name):
                continue
            require(0 < words < 16384 and priority < 32, "Invalid task descriptor")
            result.append({"address": hex(segment["vaddr"] + offset),
                           "name": name.decode(), "entry": hex(entry & ~1),
                           "function": entries[entry], "stack_bytes": words * 4,
                           "priority": priority})
    require(len(result) == 9, "Unexpected set of linked vendor task descriptors")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--bin", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    segments, symbols = load_artifact(args.elf, args.bin)
    results = [
        run(segments, symbols, "BLE connection, cold stdio", BLE_HANDLER, 38),
        run(segments, symbols, "BLE connection, initialized stdio", BLE_HANDLER, 38,
            warm_stdio=True),
        run(segments, symbols, "RTC eighth tick", RTC_CALLBACK, 52, rtc_rollover=True),
        run(segments, symbols, "Task-context lock control", symbols["__retarget_lock_acquire_recursive"], 0),
        run(segments, symbols, "Motor task first log", MOTOR_TASK, 0,
            warm_stdio=True, motor=True),
        run(segments, symbols, "PWM0 stopped callback, already idle",
            symbols["linear_motor_pwm_callback"], 44, warm_stdio=True),
        run(segments, symbols, "Motion GPIO callback",
            symbols["bc_g_sensor_int_callback"], 22, warm_stdio=True),
        # pm_evt_id_t is a byte in this -fshort-enums artifact. The selected
        # informational event (id 1) requires no peer database or SoftDevice.
        run(segments, symbols, "Peer Manager security-start event",
            symbols["pm_evt_handler"], 38, warm_stdio=True,
            arguments=(0x2003C000, 0),
            input_bytes=struct.pack("<BBHH", 1, 0, 0, 0xFFFF) + bytes(58)),
        run(segments, symbols, "BLE receive, one byte",
            symbols["bc_ble_recv"], 38, warm_stdio=True,
            arguments=(0x2003C000, 1), input_bytes=b"\x01"),
        run(segments, symbols, "BLE receive, zero-length control",
            symbols["bc_ble_recv"], 38, warm_stdio=True,
            arguments=(0x2003C000, 0)),
        run(segments, symbols, "RTC first tick control", RTC_CALLBACK, 52),
        run(segments, symbols, "Hardware-check task first log",
            HARDWARE_CHECK_TASK, 0, warm_stdio=True,
            task_descriptor=0x2000495C,
            stop_after=HARDWARE_CHECK_FIRST_LOG_RETURN),
        run(segments, symbols, "Motion task timer-creation helper",
            symbols["gsensor_int_timer_create"], 0, warm_stdio=True,
            stop_after=0x2B7FE, stack_budget_descriptor=0x200048D8),
        run(segments, symbols, "Opus startup preparation and silent-frame self-test",
            symbols["app_sudo_capture_prepare"], 0, warm_stdio=True,
            instruction_limit=5000000),
    ]
    for result in results[:3]:
        require(result["reset_requested"] and result.get("reset_value") == "0x5fa0004",
                f"Expected reset not observed: {result['name']}")
        require(result.get("reset_pc") == "0x73524", "Unexpected reset callsite")
    require(not results[3]["reset_requested"] and results[3]["returned"],
            "Task-context control failed")
    require(results[4].get("first_log_returned") and not results[4]["reset_requested"]
            and results[4]["write_beyond_stack_bytes"] > 0,
            "Expected motor stack overrun not observed")
    for result in results[5:9]:
        require(result["reset_requested"] and result.get("reset_pc") == "0x73524",
                f"Expected callback reset not observed: {result['name']}")
    for result in results[9:11]:
        require(result["returned"] and not result["reset_requested"],
                f"Control did not return safely: {result['name']}")
    require(results[11].get("first_log_returned") and
            not results[11]["reset_requested"] and
            results[11]["write_beyond_stack_bytes"] > 0,
            "Expected hardware-check task stack overrun not observed")
    require(results[12].get("first_log_returned") and
            not results[12]["reset_requested"] and
            results[12]["write_beyond_stack_bytes"] > 0,
            "Expected motion-task helper stack overrun not observed")
    require(results[13]["returned"] and results[13]["return_r0"] == 1 and
            not results[13]["reset_requested"],
            "Opus startup control did not complete successfully")
    report = {
        "status": "known S05 runtime defects reproduced; firmware is not qualified",
        "source_commit": "8632de604b2bd79c103e43292e91eab4ba13aed1",
        "elf_sha256": ELF_SHA256, "bin_sha256": BIN_SHA256,
        "elf_load_bytes_match_bin": True,
        "model": "Cortex-M4 CPU only; initialized application data and zeroed BSS",
        "limits": ["No full boot, FreeRTOS scheduling, SoftDevice, radio, or peripherals",
                   "Selected callbacks entered with modeled exception context",
                   "Motor log uses initialized global stdio and no interrupt preemption",
                   "Hardware-check task uses actual kernel task/reent setup; delay time and PendSV are not simulated",
                   "Task stacks are relocated to measure depth, not adjacent-object corruption",
                   "PWM probe enters the duplicate/already-idle stopped path; no peripheral activity modeled",
                   "Motion helper depth excludes its task/driver caller frames and peripheral initialization",
                   "Opus preparation uses initially free RTOS heap and silent PCM; no full worker stack, radio load, timing or audio-quality qualification",
                   "No physical reset trace, flash readback, or adjacent-memory damage observed"],
        "vendor_task_descriptors": task_descriptors(segments, symbols),
        "additional_task_stacks": {
            "LOGGER": 1024, "sudo-voice": 12288, "IDLE": 512, "Tmr Svc": 1024,
            "provenance": "Direct xTaskCreate callsites and selected FreeRTOS configuration; not high-water measurements"
        },
        "results": results,
    }
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(report["status"])
    for r in results:
        print(f"  {r['name']}: reset={r['reset_requested']}, "
              f"stack={r['stack_bytes']} bytes, "
              f"overrun={r.get('write_beyond_stack_bytes', 0)} bytes")
    print(f"Evidence: {args.output}")


if __name__ == "__main__":
    main()
