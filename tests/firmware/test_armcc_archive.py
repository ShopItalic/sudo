#!/usr/bin/env python3
"""Tests for the build-only ArmCC archive symbol-table normalizer."""

from __future__ import annotations

import hashlib
import importlib
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "firmware"))
normalizer = importlib.import_module("normalize_armcc_archive")


def align(value: int, boundary: int) -> int:
    return (value + boundary - 1) & ~(boundary - 1)


def symbol(name: int, value: int, size: int, bind: int, sym_type: int,
           shndx: int) -> bytes:
    return struct.pack("<IIIBBH", name, value, size,
                       (bind << 4) | sym_type, 0, shndx)


def build_fixture_elf() -> bytes:
    """Build a small ARM ET_REL with local-after-global and all index paths."""

    section_names = (
        b"\0.text\0.data\0.group\0.rel.text\0.symtab\0.strtab\0"
        b".shstrtab\0.ARM.attributes\0.symtab_shndx\0"
    )
    section_name_offsets = {
        name: section_names.index(name.encode() + b"\0")
        for name in (
            ".text", ".data", ".group", ".rel.text", ".symtab",
            ".strtab", ".shstrtab", ".ARM.attributes", ".symtab_shndx",
        )
    }
    strings = b"\0global\0local\0weak\0xlocal\0"
    name_offsets = {
        "global": strings.index(b"global"),
        "local": strings.index(b"local"),
        "weak": strings.index(b"weak"),
        "xlocal": strings.index(b"xlocal"),
    }

    # The old table advertises only symbol zero as local even though symbol 2
    # and symbol 4 are local. Symbol 4 exercises SHN_XINDEX and its side table.
    symbols = b"".join((
        symbol(0, 0, 0, 0, 0, 0),
        symbol(name_offsets["global"], 1, 2, 1, 2, 1),
        symbol(name_offsets["local"], 0x55, 4, 0, 1, 2),
        symbol(name_offsets["weak"], 0, 0, 2, 0, 0),
        symbol(name_offsets["xlocal"], 0, 4, 0, 1, 0xFFFF),
    ))
    symtab_shndx = struct.pack("<IIIII", 0, 0, 0, 0, 0x12345678)
    group = struct.pack("<III", 1, 1, 2)
    rel = struct.pack("<II", 0, (1 << 8) | 2)
    rel += struct.pack("<II", 2, (2 << 8) | 3)
    contents = {
        ".text": b"\x10\x20\x30\x40",
        ".data": b"DATA",
        ".group": group,
        ".rel.text": rel,
        ".symtab": symbols,
        ".strtab": strings,
        ".shstrtab": section_names,
        ".ARM.attributes": b"ATTRIBUTES\0ARM\0",
        ".symtab_shndx": symtab_shndx,
    }
    section_specs = [
        ("", normalizer.SHT_NULL, 0, b"", 0, 0, 0, 0, 0),
        (".text", normalizer.SHT_PROGBITS, 0x6, contents[".text"], 0, 0, 4, 0, 0),
        (".data", normalizer.SHT_PROGBITS, 0x3, contents[".data"], 0, 0, 4, 0, 0),
        (".group", normalizer.SHT_GROUP, 0, contents[".group"], 5, 1, 4, 4, 0),
        (".rel.text", normalizer.SHT_REL, 0, contents[".rel.text"], 5, 1, 4, 8, 0),
        (".symtab", normalizer.SHT_SYMTAB, 0, contents[".symtab"], 6, 1, 4, 16, 0),
        (".strtab", normalizer.SHT_STRTAB, 0, contents[".strtab"], 0, 0, 1, 0, 0),
        (".shstrtab", normalizer.SHT_STRTAB, 0, contents[".shstrtab"], 0, 0, 1, 0, 0),
        (".ARM.attributes", normalizer.SHT_ARM_ATTRIBUTES, 0,
         contents[".ARM.attributes"], 0, 0, 1, 0, 0),
        (".symtab_shndx", normalizer.SHT_SYMTAB_SHNDX, 0, contents[".symtab_shndx"], 5, 0, 4, 4, 0),
    ]

    image = bytearray(b"\0" * normalizer.ELF_HEADER_SIZE)
    offsets: list[int] = []
    sizes: list[int] = []
    for name, _, _, body, _, _, section_align, _, _ in section_specs:
        if not body:
            offsets.append(0)
            sizes.append(0)
            continue
        cursor = align(len(image), max(section_align, 1))
        image.extend(b"\0" * (cursor - len(image)))
        offsets.append(cursor)
        image.extend(body)
        sizes.append(len(body))
    shoff = align(len(image), 4)
    image.extend(b"\0" * (shoff - len(image)))
    section_headers = []
    for index, (name, section_type, flags, _, link, info, section_align,
                entsize, _) in enumerate(section_specs):
        section_headers.append(struct.pack(
            "<IIIIIIIIII",
            0 if index == 0 else section_name_offsets[name],
            section_type,
            flags,
            0,
            offsets[index],
            sizes[index],
            link,
            info,
            section_align,
            entsize,
        ))
    image.extend(b"".join(section_headers))
    struct.pack_into(
        "<16sHHIIIIIHHHHHH",
        image,
        0,
        b"\x7fELF\x01\x01\x01\0" + b"\0" * 8,
        normalizer.ET_REL,
        normalizer.EM_ARM,
        1,
        0,
        0,
        shoff,
        0x05000000,
        normalizer.ELF_HEADER_SIZE,
        0,
        0,
        normalizer.SECTION_HEADER_SIZE,
        len(section_specs),
        7,
    )
    return bytes(image)


def build_archive(*objects: tuple[str, bytes]) -> bytes:
    """Build a deterministic ordinary ar archive for parser tests."""

    def member(name: str, body: bytes) -> bytes:
        name_field = (name + ("/" if not name.endswith("/") else "")).encode()
        if len(name_field) > 16:
            raise AssertionError("fixture member name is too long")
        header = (
            name_field.ljust(16, b" ") +
            b"0           " + b"0     " + b"0     " + b"100644  " +
            f"{len(body):<10}".encode() + b"`\n"
        )
        assert len(header) == 60
        return header + body + (b"\n" if len(body) & 1 else b"")

    return b"!<arch>\n" + b"".join(member(name, body) for name, body in objects)


def section(view: normalizer.ElfView, name: str) -> normalizer.Section:
    return next(item for item in view.sections if item.name == name)


def relocation_symbols(view: normalizer.ElfView, section_name: str) -> list[int]:
    rel_section = section(view, section_name)
    result = []
    for offset in range(rel_section.offset, rel_section.offset + rel_section.size, 8):
        result.append(normalizer._u32(view.data, offset + 4) >> 8)
    return result


def symbol_identity(table: normalizer.SymbolTable, index: int) -> tuple[object, ...]:
    entry = table.symbols[index]
    return (
        table.names[index],
        normalizer._u32(entry, 4),
        normalizer._u32(entry, 8),
        entry[12],
        entry[13],
        normalizer._u16(entry, 14),
    )


def assert_archive_member_invariants(
    testcase: unittest.TestCase,
    original: bytes,
    normalized: bytes,
) -> None:
    """Check byte-preserving sections and semantic index remapping."""

    old_members = normalizer._parse_archive(original)
    new_members = normalizer._parse_archive(normalized)
    testcase.assertEqual(len(old_members), len(new_members))
    mutable_types = {
        normalizer.SHT_SYMTAB,
        normalizer.SHT_DYNSYM,
        normalizer.SHT_REL,
        normalizer.SHT_RELA,
        normalizer.SHT_GROUP,
        normalizer.SHT_SYMTAB_SHNDX,
    }
    for old_member, new_member in zip(old_members, new_members):
        testcase.assertEqual(old_member.name, new_member.name)
        testcase.assertEqual(old_member.payload_size, new_member.payload_size)
        old_payload = original[
            old_member.payload_offset:old_member.payload_offset + old_member.payload_size
        ]
        new_payload = normalized[
            new_member.payload_offset:new_member.payload_offset + new_member.payload_size
        ]
        if old_payload[:4] != normalizer.ELF_MAGIC:
            testcase.assertEqual(old_payload, new_payload)
            continue
        old_view = normalizer.inspect_elf(old_payload)
        new_view = normalizer.inspect_elf(new_payload)
        testcase.assertEqual(len(old_view.sections), len(new_view.sections))
        testcase.assertEqual(old_payload[:normalizer.ELF_HEADER_SIZE],
                             new_payload[:normalizer.ELF_HEADER_SIZE])
        for old_section, new_section in zip(old_view.sections, new_view.sections):
            testcase.assertEqual(old_section.name, new_section.name)
            testcase.assertEqual(old_section.type, new_section.type)
            testcase.assertEqual(old_section.flags, new_section.flags)
            testcase.assertEqual(old_section.address, new_section.address)
            testcase.assertEqual(old_section.offset, new_section.offset)
            testcase.assertEqual(old_section.size, new_section.size)
            testcase.assertEqual(old_section.link, new_section.link)
            testcase.assertEqual(old_section.addralign, new_section.addralign)
            testcase.assertEqual(old_section.entsize, new_section.entsize)
            if old_section.type not in mutable_types:
                testcase.assertEqual(
                    old_view.section_bytes(old_section),
                    new_view.section_bytes(new_section),
                )
        for table_index, old_table in old_view.symbols.items():
            new_table = new_view.symbols[table_index]
            testcase.assertEqual(new_table.section.info, new_table.local_count)
            for old_index, new_index in enumerate(old_table.old_to_new):
                testcase.assertEqual(
                    symbol_identity(old_table, old_index),
                    symbol_identity(new_table, new_index),
                )
            for index, entry in enumerate(new_table.symbols):
                is_local = (entry[12] >> 4) == normalizer.STB_LOCAL
                testcase.assertEqual(is_local, index < new_table.local_count)
        for old_section in old_view.sections:
            if old_section.type not in (normalizer.SHT_REL, normalizer.SHT_RELA):
                continue
            new_section = new_view.sections[old_section.index]
            old_entry_size = 8 if old_section.type == normalizer.SHT_REL else 12
            for offset in range(0, old_section.size, old_entry_size):
                old_offset = normalizer._u32(old_payload,
                                             old_section.offset + offset)
                new_offset = normalizer._u32(new_payload,
                                             new_section.offset + offset)
                testcase.assertEqual(old_offset, new_offset)
                old_info = normalizer._u32(old_payload,
                                           old_section.offset + offset + 4)
                new_info = normalizer._u32(new_payload,
                                           new_section.offset + offset + 4)
                old_symbol = old_info >> 8
                new_symbol = new_info >> 8
                testcase.assertEqual(
                    new_symbol,
                    old_view.symbols[old_section.link].old_to_new[old_symbol],
                )
                testcase.assertEqual(old_info & 0xFF, new_info & 0xFF)
                if old_entry_size == 12:
                    testcase.assertEqual(
                        old_payload[old_section.offset + offset + 8:
                                    old_section.offset + offset + 12],
                        new_payload[new_section.offset + offset + 8:
                                    new_section.offset + offset + 12],
                    )
            testcase.assertEqual(old_section.info, new_section.info)
        for old_section in old_view.sections:
            if old_section.type == normalizer.SHT_GROUP:
                new_section = new_view.sections[old_section.index]
                table = old_view.symbols[old_section.link]
                testcase.assertEqual(
                    new_section.info, table.old_to_new[old_section.info]
                )
                testcase.assertEqual(
                    old_view.section_bytes(old_section),
                    new_view.section_bytes(new_section),
                )
            elif old_section.type == normalizer.SHT_SYMTAB_SHNDX:
                new_section = new_view.sections[old_section.index]
                table = old_view.symbols[old_section.link]
                old_values = [normalizer._u32(old_payload,
                                              old_section.offset + i * 4)
                              for i in range(len(table.symbols))]
                new_values = [normalizer._u32(new_payload,
                                              new_section.offset + i * 4)
                              for i in range(len(table.symbols))]
                testcase.assertEqual(new_values,
                                     [old_values[i] for i in table.order])


class ArmccArchiveTests(unittest.TestCase):
    def test_mapping_preserves_symbols_relocations_groups_and_payloads(self) -> None:
        original = build_fixture_elf()
        normalized, report = normalizer.normalize_elf(original)
        normalized_again, report_again = normalizer.normalize_elf(original)
        self.assertEqual(normalized, normalized_again)
        self.assertEqual(report, report_again)
        self.assertTrue(report.changed)
        self.assertEqual(report.reordered_symbol_tables, 1)
        self.assertEqual(report.relocation_sections, 1)
        self.assertEqual(report.group_sections, 1)

        old_view = normalizer.inspect_elf(original)
        new_view = normalizer.inspect_elf(normalized)
        old_table = old_view.symbols[5]
        new_table = new_view.symbols[5]
        self.assertEqual(old_table.order, [0, 2, 4, 1, 3])
        self.assertEqual(old_table.old_to_new, [0, 3, 1, 4, 2])
        self.assertEqual(new_table.section.info, 3)
        self.assertEqual(new_table.local_count, 3)
        self.assertEqual(
            [new_table.names[i] for i in range(new_table.local_count)],
            ["", "local", "xlocal"],
        )
        for old_index, new_index in enumerate(old_table.old_to_new):
            self.assertEqual(new_table.names[new_index], old_table.names[old_index])
            self.assertEqual(new_table.symbols[new_index], old_table.symbols[old_index])

        self.assertEqual(relocation_symbols(old_view, ".rel.text"), [1, 2])
        self.assertEqual(relocation_symbols(new_view, ".rel.text"), [3, 1])
        old_group = section(old_view, ".group")
        new_group = section(new_view, ".group")
        self.assertEqual(old_group.info, 1)
        self.assertEqual(new_group.info, old_table.old_to_new[old_group.info])
        self.assertEqual(
            old_view.section_bytes(old_group), new_view.section_bytes(new_group)
        )

        old_shndx = section(old_view, ".symtab_shndx")
        new_shndx = section(new_view, ".symtab_shndx")
        old_values = [normalizer._u32(old_view.data, old_shndx.offset + i * 4)
                      for i in range(5)]
        new_values = [normalizer._u32(new_view.data, new_shndx.offset + i * 4)
                      for i in range(5)]
        self.assertEqual(new_values, [old_values[i] for i in old_table.order])

        for name in (".text", ".data", ".ARM.attributes"):
            self.assertEqual(
                old_view.section_bytes(section(old_view, name)),
                new_view.section_bytes(section(new_view, name)),
            )

    def test_archive_copy_is_deterministic_and_never_changes_input(self) -> None:
        object_bytes = build_fixture_elf()
        archive = build_archive(("one.o", object_bytes), ("two.o", object_bytes))
        before_hash = hashlib.sha256(archive).digest()
        normalized, report = normalizer.normalize_archive(archive)
        self.assertEqual(hashlib.sha256(archive).digest(), before_hash)
        self.assertEqual(len(normalized), len(archive))
        self.assertEqual(report.members, 2)
        self.assertEqual(report.elf_members, 2)
        self.assertEqual(report.changed_members, 2)
        self.assertEqual(normalized, normalizer.normalize_archive(normalized)[0])

        with tempfile.TemporaryDirectory(prefix="armcc-archive-test-", dir=ROOT / "build") as temp:
            source = Path(temp) / "source.lib"
            destination = Path(temp) / "normalized.lib"
            source.write_bytes(archive)
            file_report = normalizer.normalize_archive_file(source, destination)
            self.assertEqual(file_report, report)
            self.assertEqual(source.read_bytes(), archive)
            self.assertEqual(destination.read_bytes(), normalized)

    def test_rejects_unsupported_symbol_index_formats(self) -> None:
        original = build_fixture_elf()
        view = normalizer.inspect_elf(original)
        malformed = bytearray(original)
        attributes = section(view, ".ARM.attributes")
        struct.pack_into(
            "<I", malformed,
            view.section_table_offset + attributes.index * normalizer.SECTION_HEADER_SIZE + 4,
            normalizer.SHT_GNU_HASH,
        )
        with self.assertRaisesRegex(normalizer.NormalizeError, "SHT_GNU_HASH"):
            normalizer.normalize_elf(bytes(malformed))

        malformed = bytearray(original)
        struct.pack_into(
            "<I", malformed,
            view.section_table_offset + attributes.index * normalizer.SECTION_HEADER_SIZE + 4,
            0x70000042,
        )
        with self.assertRaisesRegex(normalizer.NormalizeError, "OS/processor"):
            normalizer.normalize_elf(bytes(malformed))

    def test_rejects_out_of_range_relocation_symbol(self) -> None:
        original = build_fixture_elf()
        view = normalizer.inspect_elf(original)
        rel_section = section(view, ".rel.text")
        malformed = bytearray(original)
        struct.pack_into("<I", malformed, rel_section.offset + 4, (99 << 8) | 2)
        with self.assertRaisesRegex(normalizer.NormalizeError, "out of range"):
            normalizer.normalize_elf(bytes(malformed))

    def test_rejects_in_place_archive_overwrite(self) -> None:
        with tempfile.TemporaryDirectory(prefix="armcc-archive-test-", dir=ROOT / "build") as temp:
            source = Path(temp) / "source.lib"
            source.write_bytes(build_archive(("one.o", build_fixture_elf())))
            with self.assertRaisesRegex(normalizer.NormalizeError, "overwrite"):
                normalizer.normalize_archive_file(source, source)

    @unittest.skipUnless(
        (ROOT / "firmware" / "bc_ros" / "bc_algorithm" / "bc_algorithm.lib").exists(),
        "supplier archive is not present",
    )
    def test_supplier_archive_normalizes_and_gnu_ld_reads_whole_archive(self) -> None:
        archive_path = Path(os.environ.get(
            "ARMCC_ARCHIVE",
            ROOT / "firmware" / "bc_ros" / "bc_algorithm" / "bc_algorithm.lib",
        ))
        toolchain = Path(os.environ.get(
            "ARM_GNU_BIN",
            ROOT / ".local" / "toolchains" /
            "arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi" / "bin",
        ))
        ar = toolchain / "arm-none-eabi-ar"
        ld = toolchain / "arm-none-eabi-ld"
        readelf = toolchain / "arm-none-eabi-readelf"
        if not all(item.exists() for item in (ar, ld, readelf)):
            self.skipTest("official Arm GNU toolchain is not extracted")

        original_bytes = archive_path.read_bytes()
        before_hash = hashlib.sha256(original_bytes).hexdigest()
        with tempfile.TemporaryDirectory(prefix="armcc-archive-test-", dir=ROOT / "build") as temp:
            temp_path = Path(temp)
            normalized_path = temp_path / "bc_algorithm.normalized.lib"
            report = normalizer.normalize_archive_file(archive_path, normalized_path)
            normalized_bytes = normalized_path.read_bytes()
            self.assertEqual(
                hashlib.sha256(archive_path.read_bytes()).hexdigest(), before_hash
            )
            self.assertEqual(report.elf_members, 14)
            self.assertGreater(report.changed_members, 0)
            assert_archive_member_invariants(self, original_bytes, normalized_bytes)
            self.assertEqual(
                normalizer.normalize_archive(normalized_bytes)[1].changed_members,
                0,
            )

            extracted = temp_path / "objects"
            extracted.mkdir()
            subprocess.run(
                [str(ar), "x", str(normalized_path), "--output", str(extracted)],
                check=True,
                capture_output=True,
                text=True,
            )
            for object_path in sorted(extracted.glob("*.o")):
                result = subprocess.run(
                    [str(readelf), "-sW", str(object_path)],
                    check=True,
                    capture_output=True,
                    text=True,
                )
                self.assertNotIn("local symbol", result.stderr)

            linked = temp_path / "bc_algorithm.whole.o"
            result = subprocess.run(
                [str(ld), "-r", "--whole-archive", str(normalized_path),
                 "--no-whole-archive", "-o", str(linked)],
                capture_output=True,
                text=True,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(linked.exists())


if __name__ == "__main__":
    unittest.main()
