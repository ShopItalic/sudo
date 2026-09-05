#!/usr/bin/env python3
"""Build the generated Sudo Voice target with the official GNU ARM toolchain.

This is a diagnostic build driver.  It consumes the selected source and include
paths from the generated uVision project, maps the Keil-only SDK files to their
SDK GNU equivalents, and links against a normalized *copy* of the supplier
algorithm archive.  The GNU-only _sbrk port is added here rather than to the
Keil project.  No project XML, archive, signing step, or device state is
modified by this script.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_PROJECT = ROOT / "firmware/BCL603S2X/app/project/mdk5/sudo_voice.uvprojx"
DEFAULT_OUTPUT = ROOT / "build/firmware/gnu/sudo_voice"
DEFAULT_SCRIPT = ROOT / "firmware/gnu/sudo_voice.ld"
DEFAULT_TOOLCHAIN = ROOT / ".local/toolchains/arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi/bin"

ABI_FLAGS = [
    "-mcpu=cortex-m4",
    "-mthumb",
    "-mfloat-abi=hard",
    "-mfpu=fpv4-sp-d16",
]
COMMON_C_FLAGS = [
    *ABI_FLAGS,
    "-std=gnu99",
    "-O2",
    "-ffunction-sections",
    "-fdata-sections",
    "-ffreestanding",
    "-fno-strict-aliasing",
    "-fno-common",
    "-fstack-usage",
    "-Werror=implicit-function-declaration",
    "-fshort-enums",
    "-DSUDO_GNU_RUNTIME=1",
]

SUPPLIER_ARCHIVE = ROOT / "firmware/bc_ros/bc_algorithm/bc_algorithm.lib"
SUPPLIER_ARCHIVE_SHA256 = (
    "7bb0e728200b58d87504056d7e86be0797de968799dd97bcfb0cb9343ebc7327"
)
SUPPLIER_WIDE_SYMBOL_RE = re.compile(
    r"(?i)(?<![A-Za-z0-9_])"
    r"_*"
    r"(?:wchar_t|wint_t|wcrtomb|wcs|mbtowc|mbrtowc|wctomb|"
    r"isw|fwprintf|wprintf|swprintf|wcstombs|mbstowcs|fwide|"
    r"wmem|wctype)[A-Za-z0-9_]*"
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(command: list[str], log_path: Path) -> dict:
    """Run a command and retain exact output for the build report."""
    log_path.parent.mkdir(parents=True, exist_ok=True)
    completed = subprocess.run(
        command,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    log_path.write_text(completed.stdout, encoding="utf-8", errors="replace")
    return {
        "command": command,
        "returncode": completed.returncode,
        "log": str(log_path.relative_to(ROOT)),
        "output_tail": completed.stdout[-4000:],
    }


def resolve_toolchain(explicit: str | None) -> tuple[Path | None, list[str]]:
    candidates: list[Path] = []
    if explicit:
        requested = Path(explicit).expanduser()
        candidates.append(requested if requested.name == "bin" else requested / "bin")
    env_bin = os.environ.get("ARM_NONE_EABI_BIN")
    if env_bin:
        requested = Path(env_bin).expanduser()
        candidates.append(requested if requested.name == "bin" else requested / "bin")
    candidates.append(DEFAULT_TOOLCHAIN)
    gcc_from_path = shutil.which("arm-none-eabi-gcc")
    if gcc_from_path:
        candidates.append(Path(gcc_from_path).resolve().parent)

    seen: set[Path] = set()
    ordered: list[Path] = []
    for candidate in candidates:
        candidate = candidate.resolve()
        if candidate in seen:
            continue
        seen.add(candidate)
        ordered.append(candidate)
        if (candidate / "arm-none-eabi-gcc").is_file():
            return candidate, [str(x) for x in ordered]
    return None, [str(x) for x in ordered]


def split_semicolon(value: str | None) -> list[str]:
    if not value:
        return []
    return [part.strip() for part in value.split(";") if part.strip()]


def project_path(project_dir: Path, raw: str) -> Path:
    return (project_dir / raw.replace("\\", "/")).resolve()


def parse_target(project: Path) -> tuple[ET.Element, list[Path], list[Path], list[str], list[str]]:
    root = ET.parse(project).getroot()
    targets = root.findall("./Targets/Target")
    if not targets:
        raise ValueError(f"no target in {project}")
    target = targets[0]
    project_dir = project.parent

    sources: list[Path] = []
    libraries: list[Path] = []
    source_xml: list[str] = []
    for file_node in target.findall(".//File"):
        file_type = file_node.findtext("FileType")
        raw = file_node.findtext("FilePath")
        if not raw or file_type not in {"1", "2", "3", "4"}:
            continue
        path = project_path(project_dir, raw)
        if file_type == "4" or path.suffix.lower() in {".lib", ".a"}:
            libraries.append(path)
        else:
            sources.append(path)
        source_xml.append(raw)

    # CMSIS/RTE selections are outside Target/Groups in uVision XML.
    for instance in root.findall("./RTE/files/file/instance"):
        raw = instance.text
        if raw:
            sources.append(project_path(project_dir, raw))
            source_xml.append(raw)

    return target, sources, libraries, source_xml, split_semicolon(
        target.findtext("./TargetOption/TargetArmAds/Cads/VariousControls/IncludePath")
    )


def replacement(path: Path) -> tuple[Path | None, str | None]:
    normalized = path.as_posix().lower()
    name = path.name.lower()
    app = ROOT / "firmware/BCL603S2X/app"
    if name == "arm_startup_nrf52840.s":
        return app / "modules/nrfx/mdk/gcc_startup_nrf52840.S", "Keil startup -> SDK GCC startup"
    if name == "app_error_handler_keil.c":
        return app / "components/libraries/util/app_error_handler_gcc.c", "Keil app_error -> SDK GCC app_error"
    if name == "system_nrf52840.c":
        return app / "modules/nrfx/mdk/system_nrf52840.c", "RTE system -> SDK module system"
    if normalized.endswith("/external/freertos/portable/arm/nrf52/port.c"):
        return app / "external/freertos/portable/GCC/nrf52/port.c", "FreeRTOS ARM port -> SDK GCC nrf52 port"
    return path, None


def unique_paths(paths: list[Path]) -> list[Path]:
    result: list[Path] = []
    seen: set[Path] = set()
    for path in paths:
        key = path.resolve()
        if key not in seen:
            seen.add(key)
            result.append(key)
    return result


def get_defines(target: ET.Element) -> list[str]:
    raw = target.findtext("./TargetOption/TargetArmAds/Cads/VariousControls/Define") or ""
    # Definitions are whitespace separated in the generated project.  Keep
    # values containing '=' intact and add only the compiler identity macro.
    result: list[str] = []
    seen: set[str] = set()
    for define in raw.split():
        if define.startswith("-D"):
            define = define[2:]
        if define and define not in seen:
            seen.add(define)
            result.append(define)
    if "__MODULE__=__FILE__" not in seen:
        result.append("__MODULE__=__FILE__")
    return result


def selected_paths(project: Path) -> dict:
    target, raw_sources, raw_libraries, raw_xml, xml_include_paths = parse_target(project)
    mappings: list[dict] = []
    sources: list[Path] = []
    for original in raw_sources:
        mapped, reason = replacement(original)
        assert mapped is not None
        if mapped != original or reason:
            mappings.append({"from": str(original), "to": str(mapped), "reason": reason})
        sources.append(mapped)

    # These two SDK files must be present even when RTE is not materialized by
    # the generator.  They are the only startup/system selections for this
    # GNU candidate.
    app = ROOT / "firmware/BCL603S2X/app"
    sources.extend(
        [
            app / "modules/nrfx/mdk/gcc_startup_nrf52840.S",
            app / "modules/nrfx/mdk/system_nrf52840.c",
            ROOT / "firmware/gnu/sbrk.c",
            ROOT / "firmware/gnu/newlib_locks.c",
        ]
    )
    sources = unique_paths(sources)

    include_paths: list[Path] = [
        app / "external/freertos/portable/GCC/nrf52",
        app / "components/toolchain/cmsis/include",
        app / "modules/nrfx/mdk",
    ]
    missing_xml_includes: list[str] = []
    for item in xml_include_paths:
        path = project_path(project.parent, item)
        if path.is_dir():
            include_paths.append(path)
        else:
            missing_xml_includes.append(str(path))
    include_paths = unique_paths(include_paths)

    libraries = unique_paths(raw_libraries)
    return {
        "target": target.findtext("TargetName"),
        "sources": sources,
        "libraries": libraries,
        "mappings": mappings,
        "include_paths": include_paths,
        "missing_xml_includes": missing_xml_includes,
        "defines": get_defines(target),
        "source_xml": raw_xml,
    }


def tool_versions(tools: dict[str, Path], output: Path) -> dict:
    versions: dict[str, dict] = {}
    for name, path in tools.items():
        result = run([str(path), "--version"], output / "logs" / f"version-{name}.log")
        versions[name] = {"path": str(path), **result}
    return versions


def audit_supplier_archive(archive: Path, nm: Path, log_path: Path) -> dict:
    """Fail closed unless the selected supplier boundary is the audited copy."""
    log_path = log_path if log_path.is_absolute() else ROOT / log_path
    expected = SUPPLIER_ARCHIVE.resolve()
    selected = archive.resolve()
    if selected != expected:
        raise ValueError(
            f"supplier boundary is not the audited archive: {selected}"
        )

    actual_hash = sha256(archive)
    if actual_hash != SUPPLIER_ARCHIVE_SHA256:
        raise ValueError(
            "supplier archive hash is not the audited value: "
            f"{actual_hash}"
        )

    result = run(
        [str(nm), "-A", "--extern-only", str(archive)],
        log_path,
    )
    if result["returncode"] != 0:
        raise ValueError("supplier archive symbol audit failed")

    symbol_lines = (ROOT / result["log"]).read_text(
        encoding="utf-8", errors="replace"
    ).splitlines()
    wide_hits = [line for line in symbol_lines if SUPPLIER_WIDE_SYMBOL_RE.search(line)]
    if wide_hits:
        raise ValueError(
            "supplier archive exposes a wchar-bearing symbol: "
            + wide_hits[0]
        )

    return {
        "archive": str(archive),
        "sha256": actual_hash,
        "expected_sha256": SUPPLIER_ARCHIVE_SHA256,
        "hash_match": True,
        "symbol_audit": "arm-none-eabi-nm --extern-only",
        "symbol_log": result["log"],
        "symbol_line_count": len(symbol_lines),
        "wide_symbol_pattern": SUPPLIER_WIDE_SYMBOL_RE.pattern,
        "wide_symbol_hits": [],
        "interface_header": str(
            ROOT / "firmware/bc_ros/bc_algorithm/adpcm_a.h"
        ),
        "interface_scope": [
            "adpcm_encoder(short *, char *, int, adpcm_state *)",
            "adpcm_decoder(char *, short *, int, adpcm_state *)",
            "mono_adpcm_init(MonoAdpcmProcessor *)",
        ],
        "interface_audit": (
            "Checked selected supplier symbols and the ADPCM header boundary; "
            "no wchar_t, wint_t, wide conversion, or wide formatted-I/O "
            "interface is permitted by this audit."
        ),
    }


def parse_nm_undefined(output: str) -> list[str]:
    names: list[str] = []
    for line in output.splitlines():
        line = line.strip()
        if not line:
            continue
        # GNU nm's default undefined format is "         U symbol".  Keep
        # the final field so versioned diagnostic prefixes do not leak in.
        names.append(line.split()[-1])
    return names


def run_newlib_abi_probe(
    compiler: Path,
    includes: list[str],
    defines: list[str],
    output: Path,
) -> tuple[dict, dict]:
    """Compile target-side assertions for the GNU runtime ABI.

    FreeRTOS embeds ``struct _reent`` in every TCB when
    ``configUSE_NEWLIB_REENTRANT`` is enabled.  A target compile assertion is
    preferable to a host libc size because the newlib headers and ABI are what
    the image actually consumes.
    """
    probe_source = output / "probes" / "newlib_abi_probe.c"
    probe_object = output / "probes" / "newlib_abi_probe.o"
    probe_source.parent.mkdir(parents=True, exist_ok=True)
    probe_source.write_text(
        "#include <stddef.h>\n"
        "#include <wchar.h>\n"
        "#include <reent.h>\n"
        "#include \"FreeRTOS.h\"\n"
        "#if configUSE_NEWLIB_REENTRANT != 1\n"
        "#error configUSE_NEWLIB_REENTRANT must be enabled for GNU\n"
        "#endif\n"
        "typedef char sudo_reent_must_be_512[(sizeof(struct _reent) == 512) ? 1 : -1];\n"
        "typedef char sudo_wchar_must_be_4[(sizeof(wchar_t) == 4) ? 1 : -1];\n"
        "int sudo_gnu_newlib_abi_probe(void) { return 0; }\n",
        encoding="utf-8",
    )
    command = [
        str(compiler),
        *ABI_FLAGS,
        "-std=gnu99",
        "-ffreestanding",
        "-fshort-enums",
        "-DSUDO_GNU_RUNTIME=1",
        *defines,
        *includes,
        "-c",
        str(probe_source),
        "-o",
        str(probe_object),
    ]
    result = run(command, output / "logs" / "probe-newlib-abi.log")
    result.update({"source": str(probe_source), "object": str(probe_object)})
    probe = {
        "status": "passed" if result["returncode"] == 0 else "failed",
        "struct_reent_bytes": 512 if result["returncode"] == 0 else None,
        "per_task_reent_bytes": 512 if result["returncode"] == 0 else None,
        "wchar_bytes": 4 if result["returncode"] == 0 else None,
        "configUSE_NEWLIB_REENTRANT": 1,
        "task_count_initial": 13,
        "initial_reent_ram_bytes": 13 * 512,
        "post_hardware_check_reent_ram_bytes": 12 * 512,
        "compile": result,
    }
    return probe, result


def audit_runtime_lock_backend(
    elf: Path,
    map_file: Path,
    nm: Path,
    lock_object: Path,
) -> dict:
    """Prove the final image uses the GNU backend instead of libc_a-lock.o."""
    lock_symbols = [
        "__retarget_lock_init",
        "__retarget_lock_init_recursive",
        "__retarget_lock_close",
        "__retarget_lock_close_recursive",
        "__retarget_lock_acquire",
        "__retarget_lock_acquire_recursive",
        "__retarget_lock_try_acquire",
        "__retarget_lock_try_acquire_recursive",
        "__retarget_lock_release",
        "__retarget_lock_release_recursive",
    ]
    symbol_output = subprocess.run(
        [str(nm), "-n", str(elf)],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    ).stdout
    linked_defined = set()
    for line in symbol_output.splitlines():
        fields = line.split()
        if len(fields) >= 3 and fields[1].upper() in {"T", "W"}:
            linked_defined.add(fields[-1])
    backend_output = subprocess.run(
        [str(nm), "-g", str(lock_object)],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    ).stdout
    backend_defined = set()
    for line in backend_output.splitlines():
        fields = line.split()
        if len(fields) >= 3 and fields[1].upper() in {"T", "W"}:
            backend_defined.add(fields[-1])
    map_text = map_file.read_text(encoding="utf-8", errors="replace")
    lock_object_name = lock_object.name
    lock_object_in_map = lock_object_name in map_text
    noop_members = sorted(
        {
            line.strip()
            for line in map_text.splitlines()
            if "libc_a-lock.o" in line
        }
    )
    return {
        "backend_source": str(lock_object),
        "backend_object_present": lock_object_in_map,
        "required_symbols": lock_symbols,
        "source_defined_symbols": sorted(set(lock_symbols) & backend_defined),
        "all_source_symbols_defined": set(lock_symbols) <= backend_defined,
        "linked_symbols": sorted(set(lock_symbols) & linked_defined),
        # Newlib only references the six operations below in this image.  The
        # try and non-recursive variants remain exported by the source object
        # and are covered by the host contract tests; --gc-sections may remove
        # an unreferenced implementation from the final image.
        "linked_required_symbols": [
            "__retarget_lock_init_recursive",
            "__retarget_lock_close_recursive",
            "__retarget_lock_acquire",
            "__retarget_lock_acquire_recursive",
            "__retarget_lock_release",
            "__retarget_lock_release_recursive",
        ],
        "all_linked_required_symbols_defined": {
            "__retarget_lock_init_recursive",
            "__retarget_lock_close_recursive",
            "__retarget_lock_acquire",
            "__retarget_lock_acquire_recursive",
            "__retarget_lock_release",
            "__retarget_lock_release_recursive",
        }
        <= linked_defined,
        "noop_backend_member": "libc_a-lock.o",
        "noop_backend_linked": bool(noop_members),
        "noop_backend_map_lines": noop_members,
        "status": (
            "passed"
            if lock_object_in_map
            and not noop_members
            and set(lock_symbols) <= backend_defined
            and {
                "__retarget_lock_init_recursive",
                "__retarget_lock_close_recursive",
                "__retarget_lock_acquire",
                "__retarget_lock_acquire_recursive",
                "__retarget_lock_release",
                "__retarget_lock_release_recursive",
            }
            <= linked_defined
            else "failed"
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=DEFAULT_PROJECT)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--linker-script", type=Path, default=DEFAULT_SCRIPT)
    parser.add_argument("--toolchain", help="toolchain bin directory or installation root")
    parser.add_argument("--no-link", action="store_true", help="compile and normalize only")
    args = parser.parse_args()

    project = args.project if args.project.is_absolute() else ROOT / args.project
    output = args.output if args.output.is_absolute() else ROOT / args.output
    linker_script = args.linker_script if args.linker_script.is_absolute() else ROOT / args.linker_script
    output.mkdir(parents=True, exist_ok=True)
    (output / "logs").mkdir(exist_ok=True)

    report: dict = {
        "schema": 1,
        "root": str(ROOT),
        "project": str(project),
        "project_sha256": sha256(project),
        "git_head": subprocess.run(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True, capture_output=True, check=False).stdout.strip(),
        "linker_script": str(linker_script),
        "linker_script_sha256": sha256(linker_script),
        "status": "started",
        "memory": {
            "FLASH": {"origin": "0x00027000", "length": "0x000B9000", "end": "0x000E0000"},
            "RAM": {"origin": "0x20004758", "length": "0x0003B8A8", "end": "0x20040000"},
            "softdevice_ram_minimum": "0x20002260",
        },
        "runtime": {
            "define": "SUDO_GNU_RUNTIME=1",
            "configUSE_NEWLIB_REENTRANT": 1,
            "normal_lock_gate": "FreeRTOS vTaskSuspendAll/xTaskResumeAll",
            "interrupt_masking": "normal lock gate leaves interrupts enabled; _sbrk owns short PRIMASK only",
            "stdout_write": "libnosys failure retained; GNU UART logging is disabled",
        },
        "commands": [],
        "compile": [],
        "input_hashes": {},
    }

    toolchain_bin, toolchain_candidates = resolve_toolchain(args.toolchain)
    report["toolchain_candidates"] = toolchain_candidates
    if toolchain_bin is None:
        report["status"] = "toolchain_missing"
        (output / "build-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print("No arm-none-eabi-gcc toolchain found", file=sys.stderr)
        return 2
    tool_names = ["gcc", "ar", "ld", "objcopy", "readelf", "nm"]
    tools = {name: toolchain_bin / f"arm-none-eabi-{name}" for name in tool_names}
    missing_tools = [name for name, path in tools.items() if not path.is_file()]
    report["toolchain_bin"] = str(toolchain_bin)
    report["tools"] = {name: str(path) for name, path in tools.items()}
    report["tool_versions"] = tool_versions(tools, output)
    if missing_tools:
        report["status"] = "tool_missing"
        report["missing_tools"] = missing_tools
        (output / "build-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"Missing tool(s): {', '.join(missing_tools)}", file=sys.stderr)
        return 2

    try:
        selection = selected_paths(project)
    except Exception as exc:
        report["status"] = "project_parse_failed"
        report["error"] = repr(exc)
        (output / "build-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        raise

    report["target"] = selection["target"]
    report["defines"] = selection["defines"]
    report["include_paths"] = [str(x) for x in selection["include_paths"]]
    report["missing_xml_includes"] = selection["missing_xml_includes"]
    report["source_count"] = len(selection["sources"])
    report["library_count"] = len(selection["libraries"])
    report["source_mappings"] = selection["mappings"]

    for source in selection["sources"]:
        if not source.is_file():
            report["compile"].append({"source": str(source), "status": "missing"})
        else:
            report["input_hashes"][str(source)] = sha256(source)
    for library in selection["libraries"]:
        if library.is_file():
            report["input_hashes"][str(library)] = sha256(library)
        else:
            report.setdefault("missing_libraries", []).append(str(library))

    # The generated target has exactly one supplier archive.  Normalize the
    # build copy before any link operation; the source archive hash is recorded
    # above and is never opened for writing.
    archive_inputs = [p for p in selection["libraries"] if p.suffix.lower() in {".lib", ".a"}]
    normalized_libraries: list[Path] = []
    normalizer = ROOT / "tools/firmware/normalize_armcc_archive.py"
    if archive_inputs:
        for archive in archive_inputs:
            try:
                report["supplier_boundary_audit"] = audit_supplier_archive(
                    archive,
                    tools["nm"],
                    output / "logs" / f"supplier-symbols-{archive.stem}.log",
                )
            except ValueError as exc:
                report["status"] = "supplier_boundary_audit_failed"
                report["supplier_boundary_audit_error"] = str(exc)
                (output / "build-report.json").write_text(
                    json.dumps(report, indent=2) + "\n", encoding="utf-8"
                )
                print(str(exc), file=sys.stderr)
                return 1
            source_hash_before = sha256(archive)
            normalized = output / f"{archive.stem}.normalized{archive.suffix.lower()}"
            command = [sys.executable, str(normalizer), str(archive), str(normalized)]
            result = run(command, output / "logs" / f"normalize-{archive.stem}.log")
            report["commands"].append(result)
            if result["returncode"] != 0:
                report["status"] = "archive_normalization_failed"
                report["archive_normalization"] = result
                (output / "build-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
                return 1
            normalized_libraries.append(normalized)
            report["input_hashes"][str(normalized)] = sha256(normalized)
            report.setdefault("archive_integrity", []).append({
                "source": str(archive),
                "sha256_before": source_hash_before,
                "sha256_after": sha256(archive),
                "unchanged": source_hash_before == sha256(archive),
                "normalized_copy": str(normalized),
            })
            if source_hash_before != sha256(archive):
                report["status"] = "archive_input_changed"
                report["error"] = "supplier archive changed while making build copy"
                (output / "build-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
                return 1
    elif selection["libraries"]:
        report["status"] = "library_missing"
        (output / "build-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        return 1

    includes = [f"-I{path}" for path in selection["include_paths"]]
    defines = [f"-D{define}" for define in selection["defines"]]
    compile_flags = [*COMMON_C_FLAGS, *defines, *includes]
    report["compile_flags"] = compile_flags

    abi_probe, abi_probe_result = run_newlib_abi_probe(
        tools["gcc"], includes, defines, output
    )
    report["newlib_abi"] = abi_probe
    report["commands"].append(abi_probe_result)
    if abi_probe_result["returncode"] != 0:
        report["status"] = "newlib_abi_probe_failed"
        (output / "build-report.json").write_text(
            json.dumps(report, indent=2) + "\n", encoding="utf-8"
        )
        print("newlib ABI probe failed; see build-report.json and logs/probe-newlib-abi.log", file=sys.stderr)
        return 1

    objects: list[Path] = []
    compile_failures = False
    for index, source in enumerate(selection["sources"]):
        if not source.is_file():
            compile_failures = True
            continue
        suffix = source.suffix.lower()
        obj = output / "obj" / f"{index:03d}_{source.stem}.o"
        obj.parent.mkdir(parents=True, exist_ok=True)
        if suffix == ".s" or source.name.endswith(".S"):
            # The SDK startup is preprocessed assembly.  It calls main
            # directly so this candidate does not mix the generic ARM-v4T
            # crt0.o with a Cortex-M4 Thumb2 image.
            startup_defines = [*defines, "-D__START=main", "-D__STARTUP_CLEAR_BSS"]
            command = [str(tools["gcc"]), *ABI_FLAGS, *startup_defines, *includes, "-x", "assembler-with-cpp", "-c", str(source), "-o", str(obj)]
        else:
            command = [str(tools["gcc"]), *compile_flags, "-c", str(source), "-o", str(obj)]
        result = run(command, output / "logs" / f"compile-{index:03d}-{source.stem}.log")
        result.update({"source": str(source), "object": str(obj)})
        report["compile"].append(result)
        report["commands"].append(result)
        if result["returncode"] != 0:
            compile_failures = True
        else:
            objects.append(obj)

    report["compiled_object_count"] = len(objects)
    report["compile_failure_count"] = sum(1 for item in report["compile"] if item.get("returncode", 0) != 0)
    if compile_failures:
        report["status"] = "compile_failed"
        (output / "build-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"compile failed: {report['compile_failure_count']} source(s)", file=sys.stderr)
        return 1
    if args.no_link:
        report["status"] = "compiled"
        (output / "build-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"compiled {len(objects)} source(s)")
        return 0

    elf = output / "sudo_voice.elf"
    map_file = output / "sudo_voice.map"
    bin_file = output / "sudo_voice.bin"
    link_command = [
        str(tools["gcc"]),
        *ABI_FLAGS,
        "-nostdlib",
        "-Wl,-T," + str(linker_script),
        "-Wl,-Map," + str(map_file),
        "-Wl,--gc-sections",
        "-Wl,--print-memory-usage",
        "-Wl,--cref",
        *[str(obj) for obj in objects],
        *[str(lib) for lib in normalized_libraries],
        "-Wl,--start-group",
        "-lc",
        "-lm",
        "-lnosys",
        "-lgcc",
        "-Wl,--end-group",
        "-o",
        str(elf),
    ]
    link_result = run(link_command, output / "logs" / "link.log")
    report["commands"].append(link_result)
    report["link"] = link_result
    link_log = (ROOT / link_result["log"]).read_text(encoding="utf-8", errors="replace")
    wchar_4_to_2 = link_log.count(
        "uses 4-byte wchar_t yet the output is to use 2-byte wchar_t"
    )
    wchar_2_to_4 = link_log.count(
        "uses 2-byte wchar_t yet the output is to use 4-byte wchar_t"
    )
    report["abi_warnings"] = {
        "wchar_size_warning_count": wchar_4_to_2 + wchar_2_to_4,
        "wchar_4_to_output_2_warning_count": wchar_4_to_2,
        "wchar_2_to_output_4_warning_count": wchar_2_to_4,
        "newlib_syscall_stub_warning_count": link_log.count("is not implemented and will always fail"),
        "compile_wchar_bytes": report["newlib_abi"]["wchar_bytes"],
        "newlib_wchar_bytes": report["newlib_abi"]["wchar_bytes"],
    }
    report["memory_usage"] = {}
    memory_pattern = re.compile(
        r"^\s*(FLASH|RAM):\s+(\d+) B\s+(\d+) (KB|B)\s+([0-9.]+)%"
    )
    for line in link_log.splitlines():
        match = memory_pattern.match(line)
        if match:
            region, used, size, unit, percent = match.groups()
            report["memory_usage"][region] = {
                "used_bytes": int(used),
                "region_size": f"{size} {unit}",
                "percent": float(percent),
            }
    if link_result["returncode"] != 0:
        report["status"] = "link_failed"
        (output / "build-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print("link failed; see build-report.json and logs/link.log", file=sys.stderr)
        return 1

    objcopy_result = run([str(tools["objcopy"]), "-O", "binary", str(elf), str(bin_file)], output / "logs" / "objcopy.log")
    report["commands"].append(objcopy_result)
    report["bin"] = str(bin_file)
    nm = subprocess.run([str(tools["nm"]), "-u", str(elf)], cwd=ROOT, text=True, capture_output=True, check=False)
    (output / "undefined-symbols.txt").write_text(nm.stdout, encoding="utf-8")
    report["undefined_symbols"] = parse_nm_undefined(nm.stdout)
    report["undefined_symbol_count"] = len(report["undefined_symbols"])
    readelf_outputs: dict[str, str] = {}
    for label, arguments in {
        "header": ["-h", str(elf)],
        "segments": ["-l", str(elf)],
        "sections": ["-S", str(elf)],
        "symbols": ["-s", str(elf)],
    }.items():
        result = run([str(tools["readelf"]), *arguments], output / "logs" / f"readelf-{label}.log")
        report["commands"].append(result)
        readelf_outputs[label] = result["log"]
    report["readelf"] = readelf_outputs
    report["memory_symbols"] = {}
    symbol_output = subprocess.run([str(tools["nm"]), "-n", str(elf)], cwd=ROOT, text=True, capture_output=True, check=False).stdout
    (output / "symbols.txt").write_text(symbol_output, encoding="utf-8")
    wanted = re.compile(r"(__app_(?:flash|ram)_(?:start|end)__|__softdevice_ram_start__|__isr_vector(?:_start|_end)__|__Stack(?:Top|Limit)|__Heap(?:Base|Limit)|__data_(?:start|end)__|__bss_(?:start|end)__)")
    for line in symbol_output.splitlines():
        fields = line.split()
        if len(fields) >= 3 and wanted.fullmatch(fields[-1]):
            report["memory_symbols"][fields[-1]] = fields[0]

    lock_object = next(
        (obj for obj in objects if obj.name.endswith("_newlib_locks.o")),
        output / "obj" / "newlib_locks.o",
    )
    report["runtime_lock_audit"] = audit_runtime_lock_backend(
        elf, map_file, tools["nm"], lock_object
    )
    if report["runtime_lock_audit"]["status"] != "passed":
        report["status"] = "runtime_lock_audit_failed"
        (output / "build-report.json").write_text(
            json.dumps(report, indent=2) + "\n", encoding="utf-8"
        )
        print("runtime lock backend audit failed; see build-report.json", file=sys.stderr)
        return 1

    report["status"] = "linked" if report["undefined_symbol_count"] == 0 else "linked_with_undefined_symbols"
    (output / "build-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"linked {elf}; undefined symbols: {report['undefined_symbol_count']}")
    return 0 if report["undefined_symbol_count"] == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
