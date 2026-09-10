#!/usr/bin/env python3
"""CPU-only regressions for the restored production ArmCC 5 runtime.

Requires unicorn==2.1.4 and pyelftools==0.33. This executes
the linked image, including its real compressed-data scatter loader. It does
not emulate a Ring, SoftDevice, scheduling, elapsed time or physical I/O.
"""

import argparse
from collections import deque
import hashlib
import html
import importlib.metadata
import json
import re
from pathlib import Path
import struct

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UcError, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.arm_const import (
    UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3,
    UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_XPSR, UC_ARM_REG_IPSR,
    UC_ARM_REG_PC, UC_ARM_REG_C1_C0_2, UC_ARM_REG_FPEXC,
    UC_CPU_ARM_CORTEX_M4,
)

FLASH_START, FLASH_LIMIT = 0x27000, 0xE0000
RAM_START, RAM_LIMIT = 0x20004758, 0x20040000
RETURN = 0x100000
SCRATCH = 0x2003E000


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Artifact:
    def __init__(self, elf_path, bin_path):
        self.elf_path = elf_path
        self.identity = {"elf_sha256": digest(elf_path), "bin_sha256": digest(bin_path)}
        with elf_path.open("rb") as stream:
            elf = ELFFile(stream)
            require(elf["e_machine"] == "EM_ARM", "Expected ARM ELF")
            self.segments = [(s["p_paddr"], s.data()) for s in elf.iter_segments()
                             if s["p_type"] == "PT_LOAD" and s["p_filesz"]]
            self.symbols = {}
            self.functions = {}
            self.stack_size = None
            for s in elf.get_section_by_name(".symtab").iter_symbols():
                if s.name == "STACK" and s["st_info"]["type"] == "STT_SECTION":
                    self.stack_size = s["st_size"]
                if s["st_info"]["type"] in ("STT_FUNC", "STT_OBJECT", "STT_NOTYPE"):
                    self.symbols.setdefault(s.name, []).append((s["st_value"], s["st_size"]))
                    if s["st_info"]["type"] == "STT_FUNC":
                        self.functions.setdefault(s["st_value"] & ~1, []).append(s.name)
            ram_sections = [s for s in elf.iter_sections()
                            if s["sh_flags"] & 2 and s["sh_addr"] >= 0x20000000]
            self.ram_end = max(s["sh_addr"] + s["sh_size"] for s in ram_sections)
        start = min(a for a, _ in self.segments)
        end = max(a + len(b) for a, b in self.segments)
        require(start == FLASH_START and end <= FLASH_LIMIT, "Application overlaps reserved flash")
        load = bytearray(end - start)
        for addr, data in self.segments:
            load[addr - start:addr - start + len(data)] = data
        require(load == bin_path.read_bytes(), "ELF load bytes differ from BIN")
        require(self.ram_end <= RAM_LIMIT, "Linked RAM exceeds physical SRAM")
        self.stack_top = self.sym("__initial_sp")
        require(self.stack_top <= self.ram_end and self.stack_top % 8 == 0,
                "Invalid linked initial stack")
        require(self.stack_size == 8192, "Unexpected application interrupt-stack reservation")
        require(self.ram_end < SCRATCH, "Probe scratch overlaps image; choose a new validated scratch region")
        vectors = struct.unpack_from("<64I", load)
        require(vectors[0] == self.stack_top, "Initial SP differs from vector")
        for name, index in (("Reset_Handler", 1), ("SWI2_EGU2_IRQHandler", 38),
                            ("RTC2_IRQHandler", 52), ("PendSV_Handler", 14)):
            require(vectors[index] == self.sym(name) | 1, f"Wrong {name} vector")
        require("__scatterload" in self.symbols, "Missing original Arm scatter loader")
        require("__retarget_lock_acquire_recursive" not in self.symbols,
                "Unexpected GNU Newlib runtime")
        self.layout = {"flash_start": hex(start), "flash_end": hex(end),
                       "application_bytes": len(load), "ram_end": hex(self.ram_end),
                       "ram_spare_bytes": RAM_LIMIT - self.ram_end,
                       "initial_sp": hex(self.stack_top), "interrupt_stack_bytes": self.stack_size,
                       "elf_load_matches_bin": True}
        cpu = self.new_cpu()
        cpu.reg_write(UC_ARM_REG_SP, self.stack_top)
        cpu.reg_write(UC_ARM_REG_LR, RETURN | 1)
        cpu.reg_write(UC_ARM_REG_XPSR, 0x01000000)
        cpu.emu_start(self.sym("__main") | 1, self.sym("main") & ~1, count=1000000)
        require(cpu.reg_read(UC_ARM_REG_PC) == self.sym("main") & ~1,
                f"Real C startup did not reach main: {cpu.reg_read(UC_ARM_REG_PC):#x}")
        self.ram = bytes(cpu.mem_read(0x20000000, 0x40000))
        self.startup = {"entry": "__main", "stopped_before": "main",
                        "actual_scatter_loader_executed": True,
                        "initialized_ram_sha256": hashlib.sha256(self.ram).hexdigest(),
                        "limit": "SystemInit, SoftDevice and application initialization are not executed"}

    def sym(self, name):
        values = self.symbols.get(name, [])
        require(len(values) == 1, f"Missing or ambiguous symbol {name}: {values}")
        return values[0][0]

    def new_cpu(self, initialized=False):
        cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
        cpu.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_M4)
        cpu.mem_map(0, 0x100000)
        cpu.mem_map(0x20000000, 0x40000)
        cpu.mem_map(0xE000E000, 0x2000)
        cpu.mem_map(0xE0001000, 0x1000)  # DWT; no elapsed-time model.
        cpu.reg_write(UC_ARM_REG_C1_C0_2, 0xF00000)
        cpu.reg_write(UC_ARM_REG_FPEXC, 0x40000000)
        for address, data in self.segments:
            cpu.mem_write(address, data)
        if initialized:
            cpu.mem_write(0x20000000, self.ram)
        return cpu

    def descriptors(self):
        found = []
        for offset in range(RAM_START - 0x20000000, self.ram_end - 0x20000000 - 63, 4):
            entry = struct.unpack_from("<I", self.ram, offset + 56)[0]
            names = self.functions.get(entry & ~1, []) if entry & 1 else []
            if not any("thread" in n for n in names):
                continue
            name = self.ram[offset:offset + 40].split(b"\0")[0]
            words = struct.unpack_from("<H", self.ram, offset + 40)[0]
            priority = struct.unpack_from("<I", self.ram, offset + 48)[0]
            if not name or any(c < 32 or c > 126 for c in name):
                continue
            require(0 < words < 16384 and priority < 32, "Invalid task descriptor")
            found.append({"address": hex(0x20000000 + offset), "name": name.decode(),
                          "entry": hex(entry & ~1), "function": names[0],
                          "stack_bytes": words * 4, "priority": priority})
        require(len(found) == 9, f"Expected nine linked vendor task descriptors, got {len(found)}")
        return found

    def timer_descriptors(self):
        found = []
        # Only inspect declared ELF objects at record boundaries. Searching every
        # word can misidentify the tail of a name plus an adjacent callback table.
        sites = set()
        for values in self.symbols.values():
            for address, size in values:
                if RAM_START <= address < self.ram_end and 64 <= size <= 2048 and size % 64 == 0:
                    sites.update(range(address - 0x20000000, address - 0x20000000 + size, 64))
        for offset in sorted(sites):
            entry = struct.unpack_from("<I", self.ram, offset + 56)[0]
            names = self.functions.get(entry & ~1, []) if entry & 1 else []
            if not names or not any("timer_callback" in n or "timeout" in n for n in names):
                continue
            name = self.ram[offset + 4:offset + 44].split(b"\0")[0]
            reload, period = struct.unpack_from("<II", self.ram, offset + 44)
            if not name or any(c < 32 or c > 126 for c in name) or reload > 1 or period == 0:
                continue
            found.append({"address": hex(0x20000000 + offset), "name": name.decode(),
                          "function": names[0], "auto_reload": bool(reload), "period_ticks": period})
        require(found, "No actual timer descriptors found")
        return found

    def object_symbol(self, map_path, name, obj):
        """Resolve same-named local symbols by their actual linker object."""
        pattern = (r"^\s*" + re.escape(name) + r"\s+(0x[0-9a-fA-F]+)\s+"
                   r"(?:Thumb Code|Data)\s+\d+\s+" + re.escape(obj) + r"\(")
        matches = re.findall(pattern, map_path.read_text(errors="replace"), re.M)
        require(len(matches) == 1, f"Ambiguous map symbol {obj}:{name}: {matches}")
        result = int(matches[0], 16)
        require(any(v == result for v, _ in self.symbols.get(name, [])), "Map/ELF mismatch")
        return result


class Probe:
    def __init__(self, artifact):
        self.artifact = artifact
        self.cpu = artifact.new_cpu(initialized=True)
        self.created_tasks = []

    def write_u32(self, name, value):
        self.cpu.mem_write(self.artifact.sym(name), struct.pack("<I", value))

    def read_u32(self, name):
        return struct.unpack("<I", self.cpu.mem_read(self.artifact.sym(name), 4))[0]

    def create_task(self, descriptor):
        address = int(descriptor["address"], 16)
        result = self.call("xTaskCreate", args=(int(descriptor["entry"], 16) | 1,
                                                address, descriptor["stack_bytes"] // 4,
                                                address + 52, descriptor["priority"], address + 44))
        require(result["return_r0"] == 1, "Actual task allocation failed")
        return struct.unpack("<I", self.cpu.mem_read(address + 44, 4))[0]

    def call(self, name, entry=None, args=(), ipsr=0, limit=1000000,
             stop_on_entry=None, stop_after_call=None, allow_svc=False,
             expected_return=True, expect_reset=False):
        a, cpu = self.artifact, self.cpu
        entry = a.sym(name) if entry is None else entry
        sp = a.stack_top - 32
        for reg, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3),
                              (*args[:4], 0, 0, 0, 0)):
            cpu.reg_write(reg, value)
        for i, value in enumerate(args[4:]):
            cpu.mem_write(sp + i * 4, struct.pack("<I", value))
        cpu.reg_write(UC_ARM_REG_SP, sp)
        cpu.reg_write(UC_ARM_REG_LR, RETURN | 1)
        cpu.reg_write(UC_ARM_REG_XPSR, 0x01000000 | ipsr)
        cpu.reg_write(UC_ARM_REG_IPSR, ipsr)
        result = {"name": name, "entry": hex(entry & ~1), "ipsr": ipsr,
                  "instructions": 0, "stack_bytes": 0, "stack_write_depth_bytes": 0,
                  "reset_requested": False, "trace": [], "unsafe_isr_calls": []}
        interesting = {"printf", "vsnprintf", "bc_log_task_printf", "bc_log_task_hex",
                       "localtime", "xTaskGetTickCount", "xTaskGetTickCountFromISR",
                       "xTimerGenericCommand", "xQueueGenericSendFromISR", "q_device_close",
                       "xTaskResumeAll", "vTaskDelay", "vPortFree", "pvPortMalloc",
                       "xTaskResumeFromISR",
                       "vApplicationMallocFailedHook", "vApplicationStackOverflowHook"}
        forbidden = {"printf", "vsnprintf", "localtime", "xTaskGetTickCount",
                     "vTaskSuspendAll", "xTaskResumeAll", "pvPortMalloc", "vTaskDelay"}
        stop_address = a.sym(stop_on_entry) & ~1 if stop_on_entry else None
        return_from = a.sym(stop_after_call) & ~1 if stop_after_call else None
        after = None
        recent = deque(maxlen=18)

        def code(uc, address, size, _):
            nonlocal after
            recent.append((hex(address), a.functions.get(address, [])))
            result["instructions"] += 1
            if address == a.sym("xTaskCreate") & ~1:
                task_entry = uc.reg_read(UC_ARM_REG_R0) & ~1
                pointer = uc.reg_read(UC_ARM_REG_R1)
                task_name = bytes(uc.mem_read(pointer, 40)).split(b"\0")[0].decode(errors="replace")
                self.created_tasks.append({"name": task_name,
                                           "entry": hex(task_entry),
                                           "function": a.functions.get(task_entry, []),
                                           "stack_bytes": uc.reg_read(UC_ARM_REG_R2) * 4})
            if address == a.sym("pxPortInitialiseStack") & ~1 and self.created_tasks:
                task = self.created_tasks[-1]
                top = uc.reg_read(UC_ARM_REG_R0)
                # The actual kernel aligns (pxStack + words - 1) down to 8 bytes.
                # Every pvPortMalloc stack allocation on this port is 8-aligned.
                task["allocated_stack_base"] = hex(top + 8 - task["stack_bytes"])
            depth = sp - uc.reg_read(UC_ARM_REG_SP)
            result["stack_bytes"] = max(result["stack_bytes"], depth)
            for function in a.functions.get(address, []):
                if function in interesting or function.endswith("callback"):
                    result["trace"].append(function)
                if ipsr and function in forbidden:
                    result["unsafe_isr_calls"].append(function)
            if return_from == address:
                after = uc.reg_read(UC_ARM_REG_LR) & ~1
            if address == after or address == stop_address:
                result["stopped_at"] = "return from " + stop_after_call if address == after else stop_on_entry
                uc.emu_stop()
            if size == 2 and bytes(uc.mem_read(address, 2))[1] == 0xDF:
                result["svc"] = {"pc": hex(address), "number": bytes(uc.mem_read(address, 1))[0]}
                uc.emu_stop()

        def write(uc, access, address, size, value, _):
            if sp - 16384 <= address < sp:
                result["stack_write_depth_bytes"] = max(result["stack_write_depth_bytes"], sp - address)
            if address == 0xE000ED0C and value & 4:
                result["reset_requested"] = True
                uc.emu_stop()

        hooks = [cpu.hook_add(UC_HOOK_CODE, code), cpu.hook_add(UC_HOOK_MEM_WRITE, write)]
        cpu.ctl_flush_tb()
        try:
            cpu.emu_start(entry | 1, RETURN, count=limit)
        except UcError as error:
            raise RuntimeError(f"{name}: {error} at {cpu.reg_read(UC_ARM_REG_PC):#x}; recent={list(recent)}; trace={result['trace']}") from error
        finally:
            for hook in hooks:
                cpu.hook_del(hook)
        result.update(returned=cpu.reg_read(UC_ARM_REG_PC) == RETURN,
                      final_pc=hex(cpu.reg_read(UC_ARM_REG_PC)), return_r0=cpu.reg_read(UC_ARM_REG_R0))
        require(result["reset_requested"] == expect_reset, f"{name}: unexpected reset outcome")
        require(not result["unsafe_isr_calls"], f"{name}: unsafe ISR calls {result['unsafe_isr_calls']}")
        require("svc" not in result or allow_svc, f"{name}: unexpected SVC boundary")
        require(not expected_return or result["returned"], f"{name} did not return: {result}")
        return result


def regressions(a, map_path):
    results = []
    for name, ipsr, args in (("linear_motor_pwm_callback", 44, ()),
                             ("pm_evt_handler", 38, (SCRATCH,)),
                             ("bc_ble_recv", 38, (SCRATCH, 1)),
                             ("bc_ble_recv", 38, (SCRATCH, 0)),
                             ("bc_g_sensor_int_callback", 22, (0, 1))):
        p = Probe(a)
        p.cpu.mem_write(SCRATCH, struct.pack("<BBHH", 1, 0, 0, 0xFFFF) + bytes(58))
        results.append(p.call(name, ipsr=ipsr, args=args))
    for tick in (0, 7):
        p = Probe(a)
        count = a.object_symbol(map_path, "count", "bsp_rtc.o")
        p.cpu.mem_write(count, struct.pack("<H", tick))
        before = p.read_u32("unix_time")
        result = p.call("bsp_rtc_callback", ipsr=52)
        require(p.read_u32("unix_time") == before + (tick == 7), "RTC increment mismatch")
        result["initial_tick_count"] = tick
        results.append(result)
    p = Probe(a)
    rtc_task = next(d for d in a.descriptors() if d["function"] == "app_rtc_irq_handler_thread")
    p.create_task(rtc_task)
    p.cpu.mem_write(SCRATCH, struct.pack("<I", SCRATCH + 64) + bytes(20))
    p.cpu.mem_write(SCRATCH + 64, b"sys rtc\0")
    require(p.call("bsp_rtc_timer_register_callback", args=(SCRATCH, 0, a.sym("app_rtc_time_isr_callback")))["return_r0"] == 0,
            "Actual alarm callback registration failed")
    epochs = (0, 1, 86399, 86400, 1735660800, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFE, 0xFFFFFFFF)
    for epoch in epochs:
        p.write_u32("unix_time", epoch)
        # Compare the selected Arm library's clock-of-day conversion, including
        # dates beyond 2038, before exercising the integer interrupt path.
        p.call("bsp_rtc_get_date_time", args=(SCRATCH + 128,))
        sec, minute, hour = struct.unpack("<3i", p.cpu.mem_read(SCRATCH + 128, 12))
        sod = epoch % 86400
        require((sec, minute, hour) == (sod % 60, sod // 60 % 60, sod // 3600), "Arm calendar/integer clock mismatch")
        require(p.call("bsp_rtc_timer_config", args=(SCRATCH, SCRATCH + 128, 0))["return_r0"] == 0,
                "Actual alarm configuration failed")
        p.write_u32("unix_time", (epoch - 1) & 0xFFFFFFFF)
        p.cpu.mem_write(a.object_symbol(map_path, "count", "bsp_rtc.o"), struct.pack("<H", 7))
        result = p.call("bsp_rtc_callback", ipsr=52)
        require("app_rtc_time_isr_callback" in result["trace"] and
                "xTaskResumeFromISR" in result["trace"], "Configured alarm callback was not reached")
        require(p.read_u32("unix_time") == epoch, "Alarm tick changed the wrong timestamp")
        result.update(name="RTC alarm and Arm calendar equivalence", epoch=epoch,
                      limit="Actual alarm configuration/registration and a real task; scheduling is not modeled")
        results.append(result)
    p = Probe(a)
    p.cpu.mem_write(SCRATCH + 128, bytes(64))
    require(p.call("nrf_ble_qwr_init", args=(a.sym("m_qwr"), SCRATCH + 128))["return_r0"] == 0,
            "Actual QWR initialization failed")
    p.cpu.mem_write(SCRATCH, struct.pack("<HH", 0x10, 64) + bytes(60))
    result = p.call("BLE connection callback before application callback registration",
                    entry=a.object_symbol(map_path, "ble_evt_handler", "bc_ble.o"),
                    args=(SCRATCH, 0), ipsr=38)
    result["limit"] = "Actual QWR initialized; no application callbacks, SoftDevice or radio model"
    results.append(result)
    # Keep actual queue, task, callback registration and timer initialization.
    p.call("bc_queue_init")
    p.call("app_ble_handler_thread_create")
    result = p.call("BLE connection with registered application callback",
                    entry=a.object_symbol(map_path, "ble_evt_handler", "bc_ble.o"),
                    args=(SCRATCH, 0), ipsr=38)
    require("app_ble_connect_callback" in result["trace"] and
            "xTaskGetTickCountFromISR" in result["trace"], "BLE ISR callback path was not exercised")
    result["limit"] = "Actual QWR, task/queue/timer creation and callback registration; scheduling and radio are not modeled"
    results.append(result)
    results.append(p.call("bc_ble_recv", args=(SCRATCH, 1), ipsr=38))
    for name, args in (("bc_log_task_printf", (SCRATCH, 42)),
                       ("bc_log_task_hex", (SCRATCH, SCRATCH + 256, 64))):
        p = Probe(a)
        p.cpu.mem_write(SCRATCH, b"ARM task logger %u\r\n\0")
        results.append(p.call(name, args=args))
        results.append(p.call(name, args=args, ipsr=38))
    p = Probe(a)
    p.cpu.mem_write(SCRATCH, b"float %.2f %u\r\n\0")
    # AAPCS aligns this variadic double to r2:r3; the following integer is stacked.
    result = p.call("bc_log_task_printf", args=(SCRATCH, 0, 0, 0x400C0000, 17))
    require(bytes(p.cpu.mem_read(a.sym("log_buffer"), 32)).split(b"\0")[0] == b"float 3.50 17\r\n",
            "Arm Microlib variadic float/integer formatting mismatch")
    result["name"] = "Task logger floating-point formatting"
    results.append(result)
    p = Probe(a)
    results.append(p.call("gsensor_int_timer_create"))
    result = p.call("bc_g_sensor_int_callback", ipsr=22, args=(0, 1))
    result["timer_initialized"] = True
    result["limit"] = "Actual timer queue command executes; GPIO device and application callback remain unregistered"
    results.append(result)
    # Exercise the real queue-full path without fabricating a kernel return code.
    for _ in range(64):
        result = p.call("bc_g_sensor_int_callback", ipsr=22, args=(0, 1))
        if "q_device_close" not in result["trace"]:
            break
    require("xQueueGenericSendFromISR" in result["trace"] and "q_device_close" not in result["trace"],
            "Motion queue-full path was not reached")
    result["name"] = "Motion IRQ with full timer-command queue"
    result["gpio_close_skipped"] = True
    results.append(result)
    for function in ("app_linear_motor_handler_thread", "app_hardware_check_handler_thread"):
        descriptor = next(d for d in a.descriptors() if d["function"] == function)
        p = Probe(a)
        p.create_task(descriptor)
        p.call("vTaskSwitchContext")
        result = p.call(function, stop_after_call="bc_log_task_printf", expected_return=False)
        require(result.get("stopped_at") == "return from bc_log_task_printf", "Task did not finish its first log")
        result["configured_stack_bytes"] = descriptor["stack_bytes"]
        result["exception_and_port_reserve_bytes"] = 256
        require(result["stack_bytes"] + 256 <= descriptor["stack_bytes"], "Task log exceeds stack budget with reserve")
        result["limit"] = "Actual kernel task creation/selection; no elapsed delay or PendSV delivery; relocated measurement stack"
        results.append(result)
    p = Probe(a)
    result = p.call("app_sudo_capture_prepare", limit=5000000)
    require(result["return_r0"] == 1, "Opus preparation failed")
    results.append(result)
    p = Probe(a)
    require(p.call("nrf_log_init", args=(0,))["return_r0"] == 0, "Nordic logger initialization failed")
    p.call("nrf_log_default_backends_init")
    # Nordic packs format-string addresses as flash pointers. This input data
    # lives in unused mapped flash; no image code or image data is overwritten.
    format_address = 0xDF000
    require(int(a.layout["flash_end"], 16) < format_address, "Logger input overlaps image")
    p.cpu.mem_write(format_address, b"LOGGER %lu %08x %s\r\n\0")
    p.cpu.mem_write(SCRATCH, b"L" * 600 + b"\0")
    p.call("nrf_log_frontend_std_3", args=(3, format_address, 42, 0xFEEDBEEF, SCRATCH))
    result = p.call("logger_thread", stop_on_entry="vTaskSuspend", expected_return=False)
    require(result.get("stopped_at") == "vTaskSuspend", "Nordic logger did not finish its queued record")
    require(result["stack_bytes"] + 256 <= 1024, "Nordic logger exceeds stack budget with reserve")
    result["configured_stack_bytes"] = 1024
    result["limit"] = "Actual Nordic frontend and RTT backend; one 600-character modeled record, stopped before suspension"
    results.append(result)
    p = Probe(a)
    result = p.call("pvPortMalloc", args=(143360,))
    require(result["return_r0"] == 0 and p.read_u32("sudo_rtos_malloc_failed") == 1,
            "Actual failed allocation did not invoke diagnostic hook")
    result["name"] = "Allocation failure diagnostic"
    results.append(result)
    p = Probe(a)
    descriptor = next(d for d in a.descriptors() if d["function"] == "app_linear_motor_handler_thread")
    tcb = p.create_task(descriptor)
    p.call("vTaskSwitchContext")
    base = int(p.created_tasks[0]["allocated_stack_base"], 16)
    require(bytes(p.cpu.mem_read(base, 20)) == b"\xa5" * 20, "Unexpected stack guard region")
    p.cpu.mem_write(base, b"\0")  # Deliberate stack-corruption fault injection.
    result = p.call("vTaskSwitchContext", expected_return=False, expect_reset=True)
    require(p.read_u32("sudo_rtos_stack_overflow") == 1 and
            p.read_u32("sudo_rtos_overflow_task") == tcb, "Stack diagnostic did not record affected task")
    require("vApplicationStackOverflowHook" in result["trace"] and "bc_log_task_printf" not in result["trace"],
            "Stack fault hook path was not exercised without logging")
    result["name"] = "Kernel stack-guard fault injection"
    result["expected_reset"] = True
    result["limit"] = "Deliberately corrupts a real allocated task guard; does not model adjacent-object damage"
    results.append(result)
    return results


def heap_accounting(a):
    """Actual allocations, without starting the port or peripheral drivers."""
    p = Probe(a)
    for descriptor in a.descriptors():
        p.create_task(descriptor)
    p.cpu.mem_write(SCRATCH, b"LOGGER\0" + bytes(64))
    require(p.call("xTaskCreate", args=(a.sym("logger_thread"), SCRATCH, 256, 0, 1, SCRATCH + 128))["return_r0"] == 1,
            "LOGGER allocation failed")
    p.call("app_pdm_thread_create")  # Actual voice task and both command queues.
    p.call("bc_queue_init")
    for timer in a.timer_descriptors():
        address = int(timer["address"], 16)
        result = p.call("xTimerCreate", args=(address + 4, timer["period_ticks"], int(timer["auto_reload"]),
                                              0, a.sym(timer["function"])))
        require(result["return_r0"] != 0, "Timer allocation failed")
        p.cpu.mem_write(address, struct.pack("<I", result["return_r0"]))
    opus = p.call("app_sudo_capture_prepare", limit=5000000)
    require(opus["return_r0"] == 1, "Opus allocation failed with task/queue allocations present")
    # The actual scheduler creates IDLE and Tmr Svc using this binary's selected
    # configuration. Stop at the architecture port, before any hardware access.
    scheduler = p.call("vTaskStartScheduler", stop_on_entry="xPortStartScheduler", expected_return=False)
    require(scheduler.get("stopped_at") == "xPortStartScheduler", "Scheduler setup boundary not reached")
    require(p.read_u32("uxCurrentNumberOfTasks") == 13 and len(p.created_tasks) == 13,
            "Expected thirteen actual kernel tasks")
    require(p.read_u32("sudo_rtos_malloc_failed") == 0, "Allocation failure hook was triggered")
    free = p.read_u32("xFreeBytesRemaining")
    require(free >= 16384, "Insufficient remaining heap for excluded peripheral/storage allocations")
    return {"created_tasks": p.created_tasks, "task_count": 13,
            "free_heap_bytes_after_modeled_allocations": free,
            "minimum_free_heap_bytes_in_model": p.read_u32("xMinimumEverFreeBytesRemaining"),
            "opus_silent_frame_self_test": True, "malloc_failed_hook": False,
            "limit": "No scheduling. Includes thirteen tasks, BLE queues, voice command/touch queues, linked timer records and Opus state. Excludes driver/event-group/storage allocations and sustained workload fragmentation; this is not device heap low-water evidence."}


def stack_inventory(callgraph, report):
    nodes = {}
    for block in callgraph.read_text(errors="replace").split("<P><STRONG>")[1:]:
        name = re.search(r"</a>(.*?)</STRONG>", block)
        depth = re.search(r"Max Depth = (\d+)", block)
        own = re.search(r"Stack size (\d+)", block)
        if not name or not own:
            continue
        chain = block[block.find("Call Chain = "):block.find("</UL>")] if depth else name[1] + " (own frame only; indirect calls may be unknown)"
        nodes.setdefault(name[1], []).append({"depth": int(depth[1]) if depth else int(own[1]),
                                             "chain": html.unescape(re.sub(r"<[^>]+>", "", chain)).strip()})
    def node(function):
        values = nodes.get(function, [])
        require(len(values) == 1, f"Missing/ambiguous callgraph task {function}")
        return values[0]
    result = []
    timer_depth = max(node(t["function"])["depth"] for t in report["timer_descriptors"])
    for task in report["heap_accounting"]["created_tasks"]:
        require(len(task["function"]) == 1, "Ambiguous task entry")
        function = task["function"][0]
        info = node(function)
        measured = max((r["stack_bytes"] for r in report["results"] if r["name"] == function), default=0)
        screen = max(info["depth"], measured)
        note = "Direct linker paths and any listed CPU probe; indirect calls/cycles are not a proven bound"
        if function == "prvTimerTask":
            # Deliberately sum the entire direct kernel maximum and the largest
            # callback maximum; this overcounts shared frames, and avoids treating
            # an indirect timer callback as a zero-depth leaf.
            screen += timer_depth
            note = "Direct kernel maximum plus largest linked timer callback maximum; indirect driver calls remain a measurement boundary"
        require(screen + 256 <= task["stack_bytes"], f"Known {function} path does not fit with Cortex-M4F reserve")
        result.append({"function": function, "name": task["name"], "configured_bytes": task["stack_bytes"],
                       "linker_direct_depth_bytes": info["depth"], "largest_listed_cpu_probe_bytes": measured or None,
                       "known_path_screen_bytes": screen, "context_reserve_bytes": 256,
                       "remaining_after_screen_bytes": task["stack_bytes"] - screen - 256,
                       "direct_call_chain": info["chain"], "scope": note})
    return {"tasks": result, "interrupt_stack_bytes": report["layout"]["interrupt_stack_bytes"],
            "largest_linked_timer_callback_bytes": timer_depth,
            "largest_callback_probe_bytes": max(r["stack_bytes"] for r in report["results"] if r["ipsr"]),
            "reserve_basis": "256 bytes covers the 104-byte Cortex-M4F extended hardware frame plus 100-byte ARM port context save and alignment, with 48 bytes spare. Nested exceptions also consume the separate MSP stack.",
            "limit": "This checks known linked/model paths, not task high-water marks or worst-case bounds. Linker unknown function pointers, recursion, fatal logging and actual nested interrupts require recoverable-device measurements."}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--bin", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--map", type=Path, required=True)
    parser.add_argument("--callgraph", type=Path, required=True)
    args = parser.parse_args()
    artifact = Artifact(args.elf, args.bin)
    report = {**artifact.identity, "layout": artifact.layout, "startup": artifact.startup,
              "map_sha256": digest(args.map), "callgraph_sha256": digest(args.callgraph),
              "validator_sha256": digest(Path(__file__)),
              "dependencies": {name: importlib.metadata.version(name) for name in ("unicorn", "pyelftools")},
              "vendor_task_descriptors": artifact.descriptors(),
              "timer_descriptors": artifact.timer_descriptors(),
              "results": regressions(artifact, args.map),
              "heap_accounting": heap_accounting(artifact)}
    report["stack_inventory"] = stack_inventory(args.callgraph, report)
    report["model_limits"] = [
        "CPU-only Cortex-M4F model. No full boot, SoftDevice, radio, peripheral I/O, interrupt delivery, scheduling or elapsed-time model.",
        "IPSR is supplied at callback entry. Peripheral initialization and runtime NVIC priorities are not exercised.",
        "Real Arm compressed-data initialization executes before probes. No target instruction is patched or replaced with a host stub.",
        "Task stacks are relocated for depth measurement. The deliberate stack-guard corruption case expects the diagnostic reset.",
        "No physical stack/heap high-water, audio-quality, throughput, power, DFU or recovery evidence is produced."]
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: ArmCC runtime probes; " + str(len(report["results"])) + " scenarios")
    for result in report["results"]:
        print(f"  {result['name']}: IPSR={result['ipsr']}, stack={result['stack_bytes']} B, "
              f"returned={result['returned']}, reset={result['reset_requested']}")


if __name__ == "__main__":
    main()
