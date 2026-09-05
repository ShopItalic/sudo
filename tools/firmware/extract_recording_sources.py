#!/usr/bin/env python3
"""Extract the small set of vendor functions used by the host recording test.

The complete SDK translation units require the Keil/Nordic build environment.
This keeps the test tied to the exact function bytes in the selected source
files while giving the host harness only the producer/parser/dispatch slice it
can exercise with hardware stubs.
"""

from __future__ import annotations

import argparse
from pathlib import Path


FUNCTIONS = {
    "app_package": (
        "app_package_mic_recording_start",
        "app_package_mic_recording_stop",
        "app_package_mic_capture_recording_start",
        "app_package_mic_capture_recording_stop",
        "app_package_mic_recording_stop_isr",
        "app_package_pdm_switch_online_to_offline",
        "app_package_pdm_key_flag_clear",
    ),
    "app_cmd": ("app_cmd_pdm", "app_cmd_package_parse"),
}


def skip_quoted(source: bytes, index: int, quote: int) -> int:
    index += 1
    while index < len(source):
        if source[index] == 0x5C:  # backslash
            index += 2
        elif source[index] == quote:
            return index + 1
        else:
            index += 1
    return len(source)


def skip_comment(source: bytes, index: int) -> int:
    if source[index:index + 2] == b"//":
        newline = source.find(b"\n", index + 2)
        return len(source) if newline < 0 else newline + 1
    end = source.find(b"*/", index + 2)
    return len(source) if end < 0 else end + 2


def matching(source: bytes, opening: int, left: int, right: int) -> int:
    depth = 0
    index = opening
    while index < len(source):
        if source[index:index + 2] in (b"//", b"/*"):
            index = skip_comment(source, index)
            continue
        if source[index] in (0x22, 0x27):  # string or character literal
            index = skip_quoted(source, index, source[index])
            continue
        if source[index] == left:
            depth += 1
        elif source[index] == right:
            depth -= 1
            if depth == 0:
                return index
        index += 1
    raise ValueError("unbalanced source delimiters")


def function_bytes(source: bytes, name: str) -> bytes:
    marker = name.encode("ascii") + b"("
    offset = 0
    while True:
        occurrence = source.find(marker, offset)
        if occurrence < 0:
            break
        close_paren = matching(source, occurrence + len(name), 0x28, 0x29)
        index = close_paren + 1
        while index < len(source):
            if source[index] in b" \t\r\n":
                index += 1
            elif source[index:index + 2] in (b"//", b"/*"):
                index = skip_comment(source, index)
            else:
                break
        if index < len(source) and source[index] == 0x7B:  # opening brace
            close_brace = matching(source, index, 0x7B, 0x7D)
            line_start = source.rfind(b"\n", 0, occurrence) + 1
            return source[line_start:close_brace + 1] + b"\n"
        offset = occurrence + len(marker)
    raise ValueError(f"function not found: {name}")


def write_extraction(source_path: Path, names: tuple[str, ...], output_path: Path) -> None:
    source = source_path.read_bytes()
    extracted = b'#include "recording_test_shim.h"\n\n'
    for name in names:
        extracted += function_bytes(source, name) + b"\n"
    output_path.write_bytes(extracted)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--app-package-source", type=Path, required=True)
    parser.add_argument("--cmd-handler-source", type=Path, required=True)
    parser.add_argument("--app-package-output", type=Path, required=True)
    parser.add_argument("--cmd-handler-output", type=Path, required=True)
    args = parser.parse_args()
    write_extraction(args.app_package_source, FUNCTIONS["app_package"], args.app_package_output)
    write_extraction(args.cmd_handler_source, FUNCTIONS["app_cmd"], args.cmd_handler_output)


if __name__ == "__main__":
    main()
