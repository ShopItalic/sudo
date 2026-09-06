#!/usr/bin/env python3
"""Emit a small, machine-readable report for the GNU firmware candidate.

The build driver retains a detailed command report for local diagnosis.  CI
publishes this summary instead.  It deliberately inspects the final ELF,
link map, and source-backed startup inputs so a successful compiler exit is
not mistaken for a valid reset/vector layout.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
MAIN_SOURCE = ROOT / "firmware/BCL603S2X/app/user/src/main.c"
STARTUP_SOURCE = ROOT / "firmware/BCL603S2X/app/modules/nrfx/mdk/gcc_startup_nrf52840.S"
SYSTEM_SOURCE = ROOT / "firmware/BCL603S2X/app/modules/nrfx/mdk/system_nrf52840.c"
LINKER_SCRIPT = ROOT / "firmware/gnu/sudo_voice.ld"

WIDE_TOKENS = (
    "wchar",
    "wint",
    "wcrtomb",
    "wcs",
    "isw",
    "fwide",
    "fwprintf",
    "swprintf",
    "wprintf",
    "wmem",
    "wctype",
    "mbstowcs",
    "wcstombs",
    "wctomb",
    "mbrtowc",
    "mbtowc",
    "btowc",
    "towctrans",
    "wctrans",
)
ARCHIVE_MEMBER = re.compile(
    r"(?P<archive>\S*(?:libc(?:_nano)?|libm|libg(?:_nano)?|libnosys)\.a)"
    r"\((?P<member>[^)]+)\)"
)


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def relative(path: Path) -> str:
    try:
        return str(path.resolve().relative_to(ROOT))
    except ValueError:
        return str(path)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def artifact(path: Path) -> dict[str, Any]:
    result: dict[str, Any] = {"path": relative(path), "present": path.is_file()}
    if path.is_file():
        result.update({"bytes": path.stat().st_size, "sha256": sha256(path)})
    return result


def is_wide_name(name: str) -> bool:
    lowered = name.lower()
    return any(token in lowered for token in WIDE_TOKENS)


def parse_nm(path: Path) -> dict[str, dict[str, Any]]:
    symbols: dict[str, dict[str, Any]] = {}
    for line in read_text(path).splitlines():
        fields = line.split()
        if len(fields) < 3:
            continue
        address, symbol_type, name = fields[-3:]
        if not re.fullmatch(r"[0-9A-Fa-f]+", address) or len(symbol_type) != 1:
            continue
        symbols[name] = {"address": int(address, 16), "type": symbol_type}
    return symbols


def parse_map_wide(map_path: Path, symbols: dict[str, dict[str, Any]]) -> dict[str, Any]:
    """Find wide-character libc members and final symbols in the link map."""
    lines = read_text(map_path).splitlines()
    linked_symbols = sorted(
        name
        for name, info in symbols.items()
        if info["type"] not in {"N", "n", "U", "u"} and is_wide_name(name)
    )
    linked_symbol_set = set(linked_symbols)
    members: set[str] = set()
    map_symbols: set[str] = set()
    matching_lines = 0
    in_archive_inclusion = False
    for line in lines:
        if line.startswith("Archive member included"):
            in_archive_inclusion = True
            continue
        if in_archive_inclusion and line.strip() == "Discarded input sections":
            in_archive_inclusion = False
            continue
        matches = list(ARCHIVE_MEMBER.finditer(line))
        if not matches:
            continue
        libc_match = False
        for match in matches:
            archive = match.group("archive")
            if "libc" not in archive:
                continue
            libc_match = True
            member = match.group("member")
            if in_archive_inclusion and is_wide_name(member):
                members.add(member)
                matching_lines += 1
        if not libc_match:
            continue
        for token in re.findall(r"[A-Za-z_][A-Za-z0-9_]*", line):
            if token in linked_symbol_set:
                map_symbols.add(token)
    # The final symbol table is the authoritative answer to whether code made
    # it into the image; the map supplies the newlib archive/member provenance.
    return {
        "linked": bool(linked_symbols),
        "linked_symbols": linked_symbols,
        "map_referenced_symbols": sorted(map_symbols),
        "newlib_archive_members": sorted(members),
        "map_matching_line_count": matching_lines,
        "map": relative(map_path),
    }


def parse_elf_vector(path: Path) -> dict[str, Any]:
    """Read the ELF section table and the first two vector words."""
    data = path.read_bytes()
    if len(data) < 52 or data[:4] != b"\x7fELF":
        raise ValueError("ELF header is missing")
    if data[4] != 1 or data[5] != 1:
        raise ValueError("candidate must be ELF32 little-endian")
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", data, 0)
    _, elf_type, machine, _, entry, _, section_offset, _, _, _, _, section_size, section_count, string_index = header
    if elf_type != 2 or machine != 40:
        raise ValueError(f"unexpected ELF type/machine: {elf_type}/{machine}")
    if section_size < 40 or section_count == 0 or string_index >= section_count:
        raise ValueError("invalid ELF section table")

    def section(index: int) -> tuple[int, ...]:
        offset = section_offset + index * section_size
        if offset < 0 or offset + 40 > len(data):
            raise ValueError("ELF section table exceeds file")
        return struct.unpack_from("<IIIIIIIIII", data, offset)

    string_section = section(string_index)
    string_data = data[string_section[4] : string_section[4] + string_section[5]]

    def section_name(offset: int) -> str:
        if offset >= len(string_data):
            return ""
        end = string_data.find(b"\0", offset)
        return string_data[offset : (len(string_data) if end < 0 else end)].decode("ascii", "replace")

    vector: tuple[int, ...] | None = None
    for index in range(section_count):
        candidate = section(index)
        if section_name(candidate[0]) == ".isr_vector":
            vector = candidate
            break
    if vector is None or vector[5] < 8:
        raise ValueError(".isr_vector is missing or too small")
    vector_data = data[vector[4] : vector[4] + vector[5]]
    stack_word, reset_word = struct.unpack_from("<II", vector_data, 0)
    return {
        "entry_point": entry,
        "section_address": vector[3],
        "section_size": vector[5],
        "initial_stack_word": stack_word,
        "reset_vector_word": reset_word,
    }


def parse_elf_loads(path: Path) -> list[dict[str, int]]:
    """Read ELF32 PT_LOAD headers, including RAM data LMAs.

    The linker memory-usage diagnostic accounts for a segment's VMA.  A
    .data segment has a RAM VMA but a flash LMA, so the diagnostic omits its
    bytes from FLASH.  The program headers retain both addresses and are the
    authoritative source for the load image footprint.
    """
    data = path.read_bytes()
    if len(data) < 52 or data[:4] != b"\x7fELF":
        raise ValueError("ELF header is missing")
    if data[4] != 1 or data[5] != 1:
        raise ValueError("candidate must be ELF32 little-endian")
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", data, 0)
    _, elf_type, machine, _, _, program_offset, _, _, _, program_size, program_count, _, _, _ = header
    if elf_type != 2 or machine != 40:
        raise ValueError(f"unexpected ELF type/machine: {elf_type}/{machine}")
    if program_size < 32 or program_count == 0:
        raise ValueError("invalid ELF program header table")
    table_end = program_offset + program_size * program_count
    if program_offset < 0 or table_end > len(data):
        raise ValueError("ELF program header table exceeds file")

    loads: list[dict[str, int]] = []
    for index in range(program_count):
        offset = program_offset + index * program_size
        if offset + 32 > len(data):
            raise ValueError("ELF program header exceeds file")
        p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align = struct.unpack_from(
            "<IIIIIIII", data, offset
        )
        if p_type != 1:  # PT_LOAD
            continue
        if p_filesz > p_memsz:
            raise ValueError(f"PT_LOAD #{index} file size exceeds memory size")
        if p_offset + p_filesz > len(data):
            raise ValueError(f"PT_LOAD #{index} file range exceeds ELF")
        loads.append(
            {
                "index": index,
                "offset": p_offset,
                "vaddr": p_vaddr,
                "paddr": p_paddr,
                "filesz": p_filesz,
                "memsz": p_memsz,
                "flags": p_flags,
                "align": p_align,
            }
        )
    if not loads:
        raise ValueError("ELF has no PT_LOAD segments")
    return loads


def parse_address(value: Any, label: str) -> int:
    """Parse the hex addresses emitted by build_gnu.py."""
    if isinstance(value, bool):
        raise ValueError(f"{label} is not an address")
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        token = value.strip()
        try:
            return int(token, 16) if not token.lower().startswith("0x") else int(token, 0)
        except ValueError as exc:
            raise ValueError(f"{label} is not a hexadecimal address: {value!r}") from exc
    raise ValueError(f"{label} is not a hexadecimal address: {value!r}")


def merge_ranges(ranges: list[tuple[int, int]]) -> list[tuple[int, int]]:
    """Return a sorted union of half-open address ranges."""
    merged: list[list[int]] = []
    for start, end in sorted(ranges):
        if end < start:
            raise ValueError(f"invalid address range 0x{start:x}..0x{end:x}")
        if not merged or start > merged[-1][1]:
            merged.append([start, end])
        else:
            merged[-1][1] = max(merged[-1][1], end)
    return [(start, end) for start, end in merged]


def address_range(start: int, end: int) -> dict[str, Any]:
    return {
        "start": f"0x{start:08x}",
        "end": f"0x{end:08x}",
        "bytes": end - start,
    }


def percent(used: int, total: int) -> float:
    return round(100.0 * used / total, 2) if total else 0.0


def elf_memory_usage(
    elf_path: Path,
    bin_path: Path,
    memory: dict[str, Any],
    memory_symbols: dict[str, Any],
) -> dict[str, Any]:
    """Compute actual flash load and RAM reservations from ELF evidence."""
    try:
        flash_config = memory["FLASH"]
        ram_config = memory["RAM"]
        flash_origin = parse_address(flash_config["origin"], "FLASH origin")
        flash_end = parse_address(flash_config["end"], "FLASH end")
        flash_length = parse_address(flash_config["length"], "FLASH length")
        ram_origin = parse_address(ram_config["origin"], "RAM origin")
        ram_end = parse_address(ram_config["end"], "RAM end")
        ram_length = parse_address(ram_config["length"], "RAM length")
    except (KeyError, TypeError) as exc:
        raise ValueError(f"memory configuration is incomplete: {exc}") from exc
    if flash_end <= flash_origin or flash_end - flash_origin != flash_length:
        raise ValueError("FLASH origin/end/length disagree")
    if ram_end <= ram_origin or ram_end - ram_origin != ram_length:
        raise ValueError("RAM origin/end/length disagree")

    loads = parse_elf_loads(elf_path)
    flash_segments: list[dict[str, Any]] = []
    ram_segments: list[dict[str, Any]] = []
    data_segments: list[dict[str, Any]] = []
    flash_ranges: list[tuple[int, int]] = []
    ram_ranges: list[tuple[int, int]] = []

    for load in loads:
        paddr_start = load["paddr"]
        paddr_file_end = paddr_start + load["filesz"]
        vaddr_start = load["vaddr"]
        vaddr_mem_end = vaddr_start + load["memsz"]
        segment = {
            "program_header": load["index"],
            "vaddr": f"0x{vaddr_start:08x}",
            "paddr": f"0x{paddr_start:08x}",
            "file_offset": f"0x{load['offset']:x}",
            "file_bytes": load["filesz"],
            "memory_bytes": load["memsz"],
            "flags": load["flags"],
        }
        if load["filesz"]:
            if not (flash_origin <= paddr_start and paddr_file_end <= flash_end):
                raise ValueError(
                    f"PT_LOAD #{load['index']} file LMA 0x{paddr_start:x}..0x{paddr_file_end:x} "
                    "is outside configured FLASH"
                )
            flash_ranges.append((paddr_start, paddr_file_end))
            flash_segments.append(segment)
        if load["memsz"]:
            if ram_origin <= vaddr_start and vaddr_mem_end <= ram_end:
                ram_ranges.append((vaddr_start, vaddr_mem_end))
                ram_segments.append(segment)
                if load["filesz"] and flash_origin <= paddr_start and paddr_file_end <= flash_end:
                    data_segments.append(segment)
            elif not (flash_origin <= vaddr_start and vaddr_mem_end <= flash_end):
                raise ValueError(
                    f"PT_LOAD #{load['index']} VMA 0x{vaddr_start:x}..0x{vaddr_mem_end:x} "
                    "is outside configured FLASH/RAM"
                )

    merged_flash = merge_ranges(flash_ranges)
    merged_ram = merge_ranges(ram_ranges)
    if not merged_flash:
        raise ValueError("ELF has no file-backed PT_LOAD range in FLASH")
    if not merged_ram:
        raise ValueError("ELF has no memory-backed PT_LOAD range in RAM")
    if merged_flash[0][0] != flash_origin:
        raise ValueError(
            f"ELF load image starts at 0x{merged_flash[0][0]:x}, expected FLASH origin 0x{flash_origin:x}"
        )

    flash_start = merged_flash[0][0]
    flash_end_used = merged_flash[-1][1]
    flash_span = flash_end_used - flash_start
    flash_file_bytes = sum(end - start for start, end in flash_ranges)
    bin_bytes = bin_path.stat().st_size
    if bin_bytes != flash_span:
        raise ValueError(
            f"BIN size {bin_bytes} does not match ELF PT_LOAD flash span {flash_span}"
        )

    static_start = merged_ram[0][0]
    static_end = merged_ram[-1][1]
    static_bytes = sum(end - start for start, end in merged_ram)

    def symbol(name: str) -> int:
        if name not in memory_symbols:
            raise ValueError(f"required memory symbol is missing: {name}")
        return parse_address(memory_symbols[name], name)

    heap_base = symbol("__HeapBase")
    heap_limit = symbol("__HeapLimit")
    stack_limit = symbol("__StackLimit")
    stack_top = symbol("__StackTop")
    data_start = symbol("__data_start__")
    data_end = symbol("__data_end__")
    bss_start = symbol("__bss_start__")
    bss_end = symbol("__bss_end__")
    if not (ram_origin <= heap_base <= heap_limit <= stack_limit <= stack_top <= ram_end):
        raise ValueError("heap/MSP reservation symbols are not ordered inside RAM")
    if not (static_start >= ram_origin and static_end <= heap_base):
        raise ValueError("static PT_LOAD RAM range overlaps or exceeds the heap reservation")
    data_symbol_bytes = data_end - data_start
    bss_symbol_bytes = bss_end - bss_start
    if data_end < data_start or bss_end < bss_start:
        raise ValueError("data/BSS symbols are not ordered")
    data_load_bytes = sum(int(segment["file_bytes"]) for segment in data_segments)
    if data_load_bytes != data_symbol_bytes:
        raise ValueError(
            f"ELF data LMA bytes {data_load_bytes} do not match __data symbols {data_symbol_bytes}"
        )

    heap_bytes = heap_limit - heap_base
    msp_bytes = stack_top - stack_limit
    reserved_bytes = heap_bytes + msp_bytes
    static_plus_reserved = static_bytes + reserved_bytes
    if static_plus_reserved > ram_length:
        raise ValueError("static RAM plus heap/MSP reservations exceed RAM")

    return {
        "source": "ELF PT_LOAD p_paddr/file_size for flash; PT_LOAD p_vaddr/mem_size and linker symbols for RAM",
        "flash": {
            "origin": f"0x{flash_origin:08x}",
            "region_size_bytes": flash_length,
            "load_start": f"0x{flash_start:08x}",
            "load_end": f"0x{flash_end_used:08x}",
            "load_span_bytes": flash_span,
            "load_file_bytes": flash_file_bytes,
            "bin_bytes": bin_bytes,
            "bin_matches_load_span": bin_bytes == flash_span,
            "used_bytes": flash_span,
            "percent": percent(flash_span, flash_length),
            "data_lma_bytes": data_load_bytes,
            "segments": flash_segments,
        },
        "ram": {
            "origin": f"0x{ram_origin:08x}",
            "region_size_bytes": ram_length,
            "static_start": f"0x{static_start:08x}",
            "static_end": f"0x{static_end:08x}",
            "static_bytes": static_bytes,
            "static_percent": percent(static_bytes, ram_length),
            "used_bytes": static_bytes,
            "reserved_heap_bytes": heap_bytes,
            "reserved_msp_bytes": msp_bytes,
            "reserved_bytes": reserved_bytes,
            "static_plus_reserved_bytes": static_plus_reserved,
            "static_plus_reserved_percent": percent(static_plus_reserved, ram_length),
            "available_after_static_plus_reserved_bytes": ram_length - static_plus_reserved,
            "data_symbol_bytes": data_symbol_bytes,
            "bss_symbol_bytes": bss_symbol_bytes,
            "static_symbol_alignment_bytes": static_bytes - data_symbol_bytes - bss_symbol_bytes,
            "heap": address_range(heap_base, heap_limit),
            "msp": address_range(stack_limit, stack_top),
            "segments": ram_segments,
        },
    }


def validate_runtime_lock_audit(
    audit: Any,
    build_dir: Path,
) -> list[str]:
    """Validate the build driver's proof that real retarget locks are linked."""
    if not isinstance(audit, dict):
        return ["build report runtime_lock_audit is missing or malformed"]
    errors: list[str] = []
    if audit.get("status") != "passed":
        errors.append(f"runtime lock audit status={audit.get('status')!r}, expected 'passed'")
    for field in (
        "backend_object_present",
        "all_source_symbols_defined",
        "all_linked_required_symbols_defined",
    ):
        if audit.get(field) is not True:
            errors.append(f"runtime lock audit {field}={audit.get(field)!r}, expected True")
    if audit.get("noop_backend_linked") is not False:
        errors.append(
            f"runtime lock audit noop_backend_linked={audit.get('noop_backend_linked')!r}, expected False"
        )
    if audit.get("noop_backend_member") != "libc_a-lock.o":
        errors.append("runtime lock audit does not identify libc_a-lock.o as the rejected no-op backend")
    for field in ("required_symbols", "source_defined_symbols", "linked_symbols", "linked_required_symbols"):
        if not isinstance(audit.get(field), list) or not all(isinstance(item, str) for item in audit[field]):
            errors.append(f"runtime lock audit {field} is not a list of symbol names")
    required = set(audit.get("required_symbols", []))
    source_defined = set(audit.get("source_defined_symbols", []))
    linked = set(audit.get("linked_symbols", []))
    linked_required = set(audit.get("linked_required_symbols", []))
    if required - source_defined:
        errors.append("runtime lock source is missing: " + ", ".join(sorted(required - source_defined)))
    if linked_required - linked:
        errors.append("runtime lock audit lists unlinked symbols: " + ", ".join(sorted(linked_required - linked)))
    backend_source = audit.get("backend_source")
    if not isinstance(backend_source, str) or not backend_source:
        errors.append("runtime lock audit backend_source is missing")
    else:
        backend_path = Path(backend_source)
        if not backend_path.is_absolute():
            backend_path = build_dir / backend_path
        if not backend_path.is_file():
            errors.append(f"runtime lock backend object is missing: {backend_source}")
    if not isinstance(audit.get("noop_backend_map_lines"), list) or audit.get("noop_backend_map_lines"):
        errors.append("runtime lock audit found libc_a-lock.o map entries")
    return errors


def newlib_abi_evidence(
    report: dict[str, Any],
    build_dir: Path,
) -> tuple[dict[str, Any] | None, list[str]]:
    """Return target-side newlib ABI evidence when its source/object remain available.

    The task-count and RAM-cost fields are emitted by the build driver's
    startup inventory.  They are included alongside the compile probe only
    when that reproducible target probe is still present; otherwise callers
    can attribute the inventory in documentation without presenting it as a
    fresh measurement.
    """
    raw = report.get("newlib_abi")
    if raw is None:
        return None, []
    if not isinstance(raw, dict):
        return None, ["build report newlib_abi is malformed"]
    compile_info = raw.get("compile")
    if not isinstance(compile_info, dict):
        return None, ["build report newlib_abi compile evidence is malformed"]
    source_value = compile_info.get("source")
    object_value = compile_info.get("object")
    if not isinstance(source_value, str) or not isinstance(object_value, str):
        return None, ["build report newlib_abi probe paths are malformed"]
    source_path = Path(source_value)
    object_path = Path(object_value)
    if not source_path.is_absolute():
        source_path = build_dir / source_path
    if not object_path.is_absolute():
        object_path = build_dir / object_path
    if not source_path.is_file() or not object_path.is_file():
        # A published summary may outlive ignored probe files.  Do not claim
        # probe-backed measurements in that case; the docs can cite the
        # build driver's source inventory separately.
        return None, []
    errors: list[str] = []
    expected = {
        "status": "passed",
        "struct_reent_bytes": 512,
        "per_task_reent_bytes": 512,
        "wchar_bytes": 4,
        "configUSE_NEWLIB_REENTRANT": 1,
        "task_count_initial": 13,
        "initial_reent_ram_bytes": 6656,
    }
    for field, value in expected.items():
        if raw.get(field) != value:
            errors.append(f"newlib ABI evidence {field}={raw.get(field)!r}, expected {value!r}")
    if compile_info.get("returncode") != 0:
        errors.append(f"newlib ABI probe returncode={compile_info.get('returncode')!r}, expected 0")
    probe_text = read_text(source_path)
    for marker in (
        "sizeof(struct _reent) == 512",
        "sizeof(wchar_t) == 4",
        "configUSE_NEWLIB_REENTRANT != 1",
    ):
        if marker not in probe_text:
            errors.append(f"newlib ABI probe does not contain assertion: {marker}")
    if errors:
        return None, errors
    return {
        "status": raw["status"],
        "struct_reent_bytes": raw["struct_reent_bytes"],
        "per_task_reent_bytes": raw["per_task_reent_bytes"],
        "wchar_bytes": raw["wchar_bytes"],
        "configUSE_NEWLIB_REENTRANT": raw["configUSE_NEWLIB_REENTRANT"],
        "task_count_initial": raw["task_count_initial"],
        "initial_reent_ram_bytes": raw["initial_reent_ram_bytes"],
        "post_hardware_check_reent_ram_bytes": raw.get("post_hardware_check_reent_ram_bytes"),
        "probe": {
            "source": relative(source_path),
            "object": relative(object_path),
            "returncode": compile_info["returncode"],
        },
    }, []


def command_for_source(report: dict[str, Any], suffix: str) -> list[str]:
    for item in report.get("compile", []):
        if str(item.get("source", "")).endswith(suffix):
            command = item.get("command", [])
            return [str(value) for value in command]
    return []


def make_summary(
    build_dir: Path,
    toolchain_url: str | None = None,
    toolchain_sha256: str | None = None,
    toolchain_archive: Path | None = None,
) -> tuple[dict[str, Any], list[str]]:
    report_path = build_dir / "build-report.json"
    map_path = build_dir / "sudo_voice.map"
    elf_path = build_dir / "sudo_voice.elf"
    bin_path = build_dir / "sudo_voice.bin"
    errors: list[str] = []
    report: dict[str, Any] = {}
    if report_path.is_file():
        try:
            report = json.loads(read_text(report_path))
        except json.JSONDecodeError as exc:
            errors.append(f"build report is not valid JSON: {exc}")
        if not isinstance(report, dict):
            errors.append("build report JSON root is not an object")
            report = {}
    else:
        errors.append("build-report.json is missing")

    for key, expected in (
        ("status", "linked"),
        ("compile_failure_count", 0),
        ("undefined_symbol_count", 0),
    ):
        if report.get(key) != expected:
            errors.append(f"build report {key}={report.get(key)!r}, expected {expected!r}")
    if report.get("source_count") != report.get("compiled_object_count"):
        errors.append("not every selected source produced an object")
    if not map_path.is_file():
        errors.append("link map is missing")
    if not elf_path.is_file():
        errors.append("ELF is missing")
    if not bin_path.is_file():
        errors.append("BIN is missing")

    runtime_lock_audit = report.get("runtime_lock_audit")
    errors.extend(validate_runtime_lock_audit(runtime_lock_audit, build_dir))

    symbols: dict[str, dict[str, Any]] = {}
    symbols_path = build_dir / "symbols.txt"
    if symbols_path.is_file():
        symbols = parse_nm(symbols_path)
    else:
        errors.append("symbols.txt is missing")

    actual_memory: dict[str, Any] = {}
    if elf_path.is_file() and bin_path.is_file():
        try:
            actual_memory = elf_memory_usage(
                elf_path,
                bin_path,
                report.get("memory", {}),
                report.get("memory_symbols", {}),
            )
        except (IndexError, KeyError, OSError, TypeError, ValueError, struct.error) as exc:
            errors.append(f"cannot inspect ELF memory usage: {exc}")

    newlib_evidence, newlib_errors = newlib_abi_evidence(report, build_dir)
    errors.extend(newlib_errors)

    required_symbols = {"main", "Reset_Handler", "SystemInit", "__isr_vector", "__StackTop"}
    missing_symbols = sorted(required_symbols - symbols.keys())
    if missing_symbols:
        errors.append("required linked symbols missing: " + ", ".join(missing_symbols))

    main_text = read_text(MAIN_SOURCE) if MAIN_SOURCE.is_file() else ""
    startup_text = read_text(STARTUP_SOURCE) if STARTUP_SOURCE.is_file() else ""
    system_text = read_text(SYSTEM_SOURCE) if SYSTEM_SOURCE.is_file() else ""
    linker_text = read_text(LINKER_SCRIPT) if LINKER_SCRIPT.is_file() else ""
    main_signature = bool(re.search(r"\bint\s+main\s*\(\s*void\s*\)", main_text))
    startup_source_checks = {
        "vector_section": ".section .isr_vector" in startup_text,
        "vector_symbol": "__isr_vector:" in startup_text,
        "initial_stack_entry": bool(re.search(r"\.long\s+__StackTop", startup_text)),
        "reset_vector_entry": bool(re.search(r"\.long\s+Reset_Handler", startup_text)),
        "reset_handler": "Reset_Handler:" in startup_text,
        "system_init_call": "bl SystemInit" in startup_text,
        "configurable_start": "__START" in startup_text,
    }
    system_source_checks = {"system_init_definition": bool(re.search(r"\bvoid\s+SystemInit\s*\(\s*void\s*\)", system_text))}
    linker_checks = {
        "entry_reset_handler": "ENTRY(Reset_Handler)" in linker_text,
        "vector_kept": "KEEP(*(.isr_vector))" in linker_text,
        "flash_origin": "ORIGIN = 0x00027000" in linker_text,
        "flash_end": "LENGTH = 0x000B9000" in linker_text,
    }
    startup_command = command_for_source(report, "gcc_startup_nrf52840.S")
    startup_command_checks = {
        "uses_main_start": "-D__START=main" in startup_command,
        "clears_bss": "-D__STARTUP_CLEAR_BSS" in startup_command,
    }

    vector: dict[str, Any] = {}
    if elf_path.is_file():
        try:
            vector = parse_elf_vector(elf_path)
        except (IndexError, OSError, ValueError, struct.error) as exc:
            errors.append(f"cannot inspect ELF vector: {exc}")
    if vector:
        reset_address = symbols.get("Reset_Handler", {}).get("address")
        stack_address = symbols.get("__StackTop", {}).get("address")
        vector_address = symbols.get("__isr_vector", {}).get("address")
        vector["reset_symbol_address"] = reset_address
        vector["stack_symbol_address"] = stack_address
        vector["vector_symbol_address"] = vector_address
        vector["vector_symbol_matches_section"] = vector_address is not None and vector_address == vector["section_address"]
        vector["reset_matches_thumb_symbol"] = reset_address is not None and vector["reset_vector_word"] == reset_address | 1
        vector["initial_stack_matches_symbol"] = stack_address is not None and vector["initial_stack_word"] == stack_address
        vector["entry_matches_thumb_reset"] = reset_address is not None and vector["entry_point"] == reset_address | 1
        if vector["section_address"] != 0x27000:
            errors.append(f".isr_vector address is 0x{vector['section_address']:x}, expected 0x27000")
        for field in (
            "vector_symbol_matches_section",
            "reset_matches_thumb_symbol",
            "initial_stack_matches_symbol",
            "entry_matches_thumb_reset",
        ):
            if not vector[field]:
                errors.append(f"ELF vector check failed: {field}")

    startup_compatible = (
        main_signature
        and all(startup_source_checks.values())
        and all(system_source_checks.values())
        and all(linker_checks.values())
        and all(startup_command_checks.values())
        and bool(vector)
        and vector.get("vector_symbol_matches_section", False)
        and vector.get("reset_matches_thumb_symbol", False)
        and vector.get("initial_stack_matches_symbol", False)
        and vector.get("entry_matches_thumb_reset", False)
    )
    if not startup_compatible:
        errors.append("main/reset/system startup compatibility checks failed")

    toolchain_summary: dict[str, Any] = {
        "gcc": next(
            (
                line.strip()
                for line in str(report.get("tool_versions", {}).get("gcc", {}).get("output_tail", "")).splitlines()
                if line.strip()
            ),
            None,
        )
    }
    if toolchain_url:
        toolchain_summary["archive_url"] = toolchain_url
    if toolchain_sha256:
        toolchain_summary["archive_sha256"] = toolchain_sha256
    if toolchain_archive:
        if toolchain_archive.is_file():
            actual_sha256 = sha256(toolchain_archive)
            toolchain_summary["archive_sha256_actual"] = actual_sha256
            toolchain_summary["archive_verified"] = bool(toolchain_sha256) and actual_sha256 == toolchain_sha256
            if not toolchain_summary["archive_verified"]:
                errors.append("Arm GNU archive checksum did not match the pinned SHA-256")
        else:
            toolchain_summary["archive_verified"] = False
            errors.append("Arm GNU archive is missing from the verified cache path")

    summary: dict[str, Any] = {
        "schema": 1,
        "status": "pass" if not errors else "fail",
        "engineering_candidate": True,
        "hardware_qualified": False,
        "target": report.get("target"),
        "build_status": report.get("status"),
        "source_count": report.get("source_count"),
        "compiled_object_count": report.get("compiled_object_count"),
        "compile_failure_count": report.get("compile_failure_count"),
        "undefined_symbol_count": report.get("undefined_symbol_count"),
        "artifacts": {
            "elf": artifact(elf_path),
            "map": artifact(map_path),
            "bin": artifact(bin_path),
        },
        "memory": report.get("memory", {}),
        "memory_symbols": report.get("memory_symbols", {}),
        "memory_usage": actual_memory,
        "linker_printed_memory_usage": report.get("memory_usage", {}),
        "abi_warnings": report.get("abi_warnings", {}),
        "runtime_lock_audit": runtime_lock_audit,
        "wide_newlib": parse_map_wide(map_path, symbols) if map_path.is_file() else {"linked": False, "map": relative(map_path)},
        "startup_reset_system": {
            "main_signature": main_signature,
            "source_checks": startup_source_checks,
            "system_source_checks": system_source_checks,
            "linker_checks": linker_checks,
            "startup_command_checks": startup_command_checks,
            "vector": vector,
            "compatible": startup_compatible,
        },
        "toolchain": toolchain_summary,
    }
    if newlib_evidence is not None:
        summary["newlib_abi"] = newlib_evidence
    if errors:
        summary["errors"] = errors
    return summary, errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--toolchain-url")
    parser.add_argument("--toolchain-sha256")
    parser.add_argument("--toolchain-archive", type=Path)
    args = parser.parse_args()
    build_dir = args.build_dir if args.build_dir.is_absolute() else ROOT / args.build_dir
    output = args.output or build_dir / "build-summary.json"
    if not output.is_absolute():
        output = ROOT / output
    output.parent.mkdir(parents=True, exist_ok=True)
    archive = args.toolchain_archive
    if archive is not None and not archive.is_absolute():
        archive = ROOT / archive
    summary, errors = make_summary(build_dir, args.toolchain_url, args.toolchain_sha256, archive)
    output.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if errors:
        print(f"GNU summary failed: {len(errors)} validation error(s); see {output}", file=sys.stderr)
        return 1
    print(f"GNU summary: {output} (wide newlib linked: {summary['wide_newlib']['linked']})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
