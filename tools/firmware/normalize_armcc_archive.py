#!/usr/bin/env python3
"""Normalize ArmCC ELF objects inside an archive for GNU ld.

ArmCC 5 can emit an ELF symbol table with a local symbol after the global
symbol range advertised by ``sh_info``.  GNU ld rejects that ordering while
the Arm linker accepts it.  This tool makes a build-only copy of an archive
safe for GNU ld by stably moving all STB_LOCAL entries before non-local
entries and updating the symbol-index-bearing records that point at them.

The input archive is never written.  Archive headers, member order, the
archive symbol index, and every unchanged member byte are retained.  The
normalizer is deliberately conservative: section formats which may contain
symbol indexes but are not implemented here fail closed instead of being
guessed at.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import struct
import sys
from typing import Iterable


class NormalizeError(ValueError):
    """Raised when an ELF or archive is outside the supported safe subset."""


# ELF32 constants used by the object files in the supplier archive.
ELF_MAGIC = b"\x7fELF"
ELFCLASS32 = 1
ELFDATA2LSB = 1
EV_CURRENT = 1
ET_REL = 1
EM_ARM = 40

SHT_NULL = 0
SHT_PROGBITS = 1
SHT_SYMTAB = 2
SHT_STRTAB = 3
SHT_RELA = 4
SHT_HASH = 5
SHT_DYNAMIC = 6
SHT_NOTE = 7
SHT_NOBITS = 8
SHT_REL = 9
SHT_SHLIB = 10
SHT_DYNSYM = 11
SHT_INIT_ARRAY = 14
SHT_FINI_ARRAY = 15
SHT_PREINIT_ARRAY = 16
SHT_GROUP = 17
SHT_SYMTAB_SHNDX = 18

SHT_GNU_HASH = 0x6FFFFFF6
SHT_GNU_LIBLIST = 0x6FFFFFF7
SHT_CHECKSUM = 0x6FFFFFF8
SHT_GNU_VERDEF = 0x6FFFFFFD
SHT_GNU_VERNEED = 0x6FFFFFFE
SHT_GNU_VERSYM = 0x6FFFFFFF

SHT_ARM_EXIDX = 0x70000001
SHT_ARM_ATTRIBUTES = 0x70000003

STB_LOCAL = 0
SHN_UNDEF = 0
SHN_XINDEX = 0xFFFF

ELF_HEADER_SIZE = 52
SECTION_HEADER_SIZE = 40
SYMBOL_SIZE = 16
REL_SIZE = 8
RELA_SIZE = 12


def _u16(data: bytes | bytearray, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def _u32(data: bytes | bytearray, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def _put_u16(data: bytearray, offset: int, value: int) -> None:
    struct.pack_into("<H", data, offset, value)


def _put_u32(data: bytearray, offset: int, value: int) -> None:
    struct.pack_into("<I", data, offset, value)


def _section_header_offset(view: "ElfView", section_index: int) -> int:
    return view.section_table_offset + section_index * SECTION_HEADER_SIZE


def _cstring(table: bytes, offset: int, what: str) -> str:
    if offset >= len(table):
        raise NormalizeError(f"{what}: string offset {offset} is outside table")
    end = table.find(b"\0", offset)
    if end < 0:
        raise NormalizeError(f"{what}: unterminated string at offset {offset}")
    return table[offset:end].decode("latin-1")


@dataclass(frozen=True)
class Section:
    index: int
    name: str
    type: int
    flags: int
    address: int
    offset: int
    size: int
    link: int
    info: int
    addralign: int
    entsize: int


@dataclass
class SymbolTable:
    section: Section
    strtab: Section
    symbols: list[bytes]
    names: list[str]
    order: list[int]
    old_to_new: list[int]
    local_count: int


@dataclass
class ElfView:
    data: bytes
    section_table_offset: int
    sections: list[Section]
    symbols: dict[int, SymbolTable]

    def section_bytes(self, section: Section) -> bytes:
        if section.type == SHT_NOBITS:
            return b""
        return self.data[section.offset:section.offset + section.size]


@dataclass(frozen=True)
class ArchiveMember:
    header_offset: int
    payload_offset: int
    payload_size: int
    name: str


@dataclass(frozen=True)
class NormalizeReport:
    changed: bool
    symbol_tables: int
    reordered_symbol_tables: int
    relocation_sections: int
    group_sections: int


@dataclass(frozen=True)
class ArchiveReport:
    members: int
    elf_members: int
    changed_members: int
    reordered_symbol_tables: int
    relocation_sections: int
    group_sections: int


def _section_data_range(data: bytes, section: Section) -> tuple[int, int]:
    if section.type == SHT_NOBITS:
        if section.offset > len(data):
            raise NormalizeError(
                f"section {section.index} ({section.name}): NOBITS offset outside file"
            )
        return section.offset, section.offset
    end = section.offset + section.size
    if section.offset > len(data) or end > len(data):
        raise NormalizeError(
            f"section {section.index} ({section.name}): data range outside file"
        )
    return section.offset, end


def _check_section_type(section: Section) -> None:
    # These section types either contain symbol indexes in a format not
    # implemented below or carry dynamic-linking state which cannot safely be
    # repaired by a local symbol-table reorder.
    unsupported_symbol_index_types = {
        SHT_HASH: "SHT_HASH",
        SHT_DYNAMIC: "SHT_DYNAMIC",
        SHT_GNU_HASH: "SHT_GNU_HASH",
        SHT_GNU_LIBLIST: "SHT_GNU_LIBLIST",
        SHT_GNU_VERDEF: "SHT_GNU_VERDEF",
        SHT_GNU_VERNEED: "SHT_GNU_VERNEED",
        SHT_GNU_VERSYM: "SHT_GNU_VERSYM",
    }
    if section.type in unsupported_symbol_index_types:
        raise NormalizeError(
            f"section {section.index} ({section.name}): unsupported "
            f"symbol-index format {unsupported_symbol_index_types[section.type]}"
        )
    if section.type in (SHT_ARM_EXIDX, SHT_ARM_ATTRIBUTES):
        return
    if section.type >= 0x60000000:
        raise NormalizeError(
            f"section {section.index} ({section.name}): unsupported "
            f"OS/processor section type 0x{section.type:08x}"
        )
    supported_plain_types = {
        SHT_NULL,
        SHT_PROGBITS,
        SHT_SYMTAB,
        SHT_STRTAB,
        SHT_RELA,
        SHT_NOTE,
        SHT_NOBITS,
        SHT_REL,
        SHT_SHLIB,
        SHT_DYNSYM,
        SHT_INIT_ARRAY,
        SHT_FINI_ARRAY,
        SHT_PREINIT_ARRAY,
        SHT_GROUP,
        SHT_SYMTAB_SHNDX,
    }
    if section.type not in supported_plain_types:
        raise NormalizeError(
            f"section {section.index} ({section.name}): unsupported section "
            f"type {section.type}"
        )


def _parse_elf(data: bytes) -> ElfView:
    if len(data) < ELF_HEADER_SIZE or data[:4] != ELF_MAGIC:
        raise NormalizeError("not an ELF32 object")
    ident = data[:16]
    if ident[4] != ELFCLASS32:
        raise NormalizeError(f"unsupported ELF class {ident[4]}")
    if ident[5] != ELFDATA2LSB:
        raise NormalizeError(f"unsupported ELF data encoding {ident[5]}")
    if ident[6] != EV_CURRENT:
        raise NormalizeError(f"unsupported ELF ident version {ident[6]}")

    e_type = _u16(data, 16)
    e_machine = _u16(data, 18)
    e_version = _u32(data, 20)
    e_phoff = _u32(data, 28)
    e_shoff = _u32(data, 32)
    e_ehsize = _u16(data, 40)
    e_phentsize = _u16(data, 42)
    e_phnum = _u16(data, 44)
    e_shentsize = _u16(data, 46)
    e_shnum = _u16(data, 48)
    e_shstrndx = _u16(data, 50)
    if e_type != ET_REL or e_machine != EM_ARM:
        raise NormalizeError(
            f"expected ARM ET_REL, got type={e_type} machine={e_machine}"
        )
    if e_version != EV_CURRENT:
        raise NormalizeError(f"unsupported ELF version {e_version}")
    if e_ehsize != ELF_HEADER_SIZE:
        raise NormalizeError(f"unsupported ELF header size {e_ehsize}")
    if e_phnum != 0 or e_phoff != 0 or e_phentsize != 0:
        raise NormalizeError("program headers are unsupported for relocatable objects")
    if e_shentsize != SECTION_HEADER_SIZE:
        raise NormalizeError(f"unsupported section-header size {e_shentsize}")
    if e_shnum == 0 or e_shstrndx == 0xFFFF:
        raise NormalizeError("extended section numbering is unsupported")
    section_table_end = e_shoff + e_shnum * SECTION_HEADER_SIZE
    if e_shoff < ELF_HEADER_SIZE or section_table_end > len(data):
        raise NormalizeError("section-header table is outside the object")
    if e_shstrndx >= e_shnum:
        raise NormalizeError("section-header string-table index is out of range")

    raw_headers: list[tuple[int, ...]] = []
    for index in range(e_shnum):
        offset = e_shoff + index * SECTION_HEADER_SIZE
        raw_headers.append(struct.unpack_from("<IIIIIIIIII", data, offset))
    if raw_headers[0] != (0, SHT_NULL, 0, 0, 0, 0, 0, 0, 0, 0):
        raise NormalizeError("section zero is not the required null section")
    shstr_header = raw_headers[e_shstrndx]
    if shstr_header[1] != SHT_STRTAB:
        raise NormalizeError("section-header string-table is not SHT_STRTAB")
    shstr_start = shstr_header[4]
    shstr_end = shstr_start + shstr_header[5]
    if shstr_end > len(data):
        raise NormalizeError("section-header string table is outside the object")
    shstrtab = data[shstr_start:shstr_end]

    sections: list[Section] = []
    for index, header in enumerate(raw_headers):
        name_offset, section_type, flags, address, offset, size, link, info, align, entsize = header
        if link >= e_shnum:
            raise NormalizeError(
                f"section {index}: sh_link {link} is out of range"
            )
        name = _cstring(shstrtab, name_offset, f"section {index} name")
        section = Section(
            index=index,
            name=name,
            type=section_type,
            flags=flags,
            address=address,
            offset=offset,
            size=size,
            link=link,
            info=info,
            addralign=align,
            entsize=entsize,
        )
        _check_section_type(section)
        _section_data_range(data, section)
        sections.append(section)

    # A rewritten symbol table must not overlap another file-backed section.
    # Rejecting overlaps avoids silently changing bytes belonging to a second
    # section in malformed input.
    file_ranges = sorted(
        (section.offset, section.offset + section.size, section)
        for section in sections
        if section.type != SHT_NOBITS and section.size != 0
    )
    for (_, previous_end, previous), (next_start, _, current) in zip(
        file_ranges, file_ranges[1:]
    ):
        if next_start < previous_end:
            raise NormalizeError(
                f"sections {previous.index} ({previous.name}) and "
                f"{current.index} ({current.name}) overlap"
            )
    for section in sections:
        if section.type != SHT_NOBITS and section.offset + section.size > e_shoff:
            raise NormalizeError(
                f"section {section.index} ({section.name}) overlaps section headers"
            )

    view = ElfView(data=data, section_table_offset=e_shoff, sections=sections, symbols={})
    for section in sections:
        if section.type not in (SHT_SYMTAB, SHT_DYNSYM):
            continue
        if section.entsize != SYMBOL_SIZE:
            raise NormalizeError(
                f"section {section.index} ({section.name}): symbol entry size "
                f"{section.entsize}, expected {SYMBOL_SIZE}"
            )
        if section.size == 0 or section.size % SYMBOL_SIZE != 0:
            raise NormalizeError(
                f"section {section.index} ({section.name}): invalid symbol-table size"
            )
        strtab = sections[section.link]
        if strtab.type != SHT_STRTAB:
            raise NormalizeError(
                f"section {section.index} ({section.name}): sh_link does not name a string table"
            )
        strings = view.section_bytes(strtab)
        if not strings or strings[0] != 0:
            raise NormalizeError(
                f"section {section.index} ({section.name}): malformed string table"
            )
        count = section.size // SYMBOL_SIZE
        symbols = [
            view.section_bytes(section)[i * SYMBOL_SIZE:(i + 1) * SYMBOL_SIZE]
            for i in range(count)
        ]
        if section.info > count:
            raise NormalizeError(
                f"section {section.index} ({section.name}): sh_info {section.info} "
                f"exceeds symbol count {count}"
            )
        if _u32(symbols[0], 0) != 0 or _u32(symbols[0], 4) != 0 or \
                _u32(symbols[0], 8) != 0 or symbols[0][12] != 0 or \
                symbols[0][13] != 0 or _u16(symbols[0], 14) != SHN_UNDEF:
            raise NormalizeError(
                f"section {section.index} ({section.name}): symbol zero is malformed"
            )
        names = []
        for symbol_index, symbol in enumerate(symbols):
            names.append(_cstring(strings, _u32(symbol, 0),
                                  f"symbol {symbol_index} in {section.name}"))
            shndx = _u16(symbol, 14)
            if shndx == SHN_XINDEX:
                # The linked SHT_SYMTAB_SHNDX section is checked below.
                continue
            if shndx >= len(sections) and shndx < 0xFF00:
                raise NormalizeError(
                    f"symbol {symbol_index} in {section.name}: section index "
                    f"{shndx} is out of range"
                )
        if (symbols[0][12] >> 4) != STB_LOCAL:
            raise NormalizeError(
                f"section {section.index} ({section.name}): symbol zero is not local"
            )
        order = [i for i, symbol in enumerate(symbols)
                 if (symbol[12] >> 4) == STB_LOCAL]
        order.extend(i for i, symbol in enumerate(symbols)
                     if (symbol[12] >> 4) != STB_LOCAL)
        old_to_new = [0] * count
        for new_index, old_index in enumerate(order):
            old_to_new[old_index] = new_index
        view.symbols[section.index] = SymbolTable(
            section=section,
            strtab=strtab,
            symbols=symbols,
            names=names,
            order=order,
            old_to_new=old_to_new,
            local_count=len([i for i in order
                             if (symbols[i][12] >> 4) == STB_LOCAL]),
        )

    if not view.symbols:
        raise NormalizeError("object has no symbol table")
    return view


def _linked_table(view: ElfView, section: Section) -> SymbolTable:
    try:
        return view.symbols[section.link]
    except KeyError as exc:
        raise NormalizeError(
            f"section {section.index} ({section.name}) links to non-symbol section "
            f"{section.link}"
        ) from exc


def _normalize_relocations(out: bytearray, view: ElfView, section: Section,
                           table: SymbolTable) -> None:
    entry_size = REL_SIZE if section.type == SHT_REL else RELA_SIZE
    if section.entsize != entry_size:
        raise NormalizeError(
            f"section {section.index} ({section.name}): relocation entry size "
            f"{section.entsize}, expected {entry_size}"
        )
    if section.size % entry_size != 0:
        raise NormalizeError(
            f"section {section.index} ({section.name}): malformed relocation size"
        )
    # Relocation sh_info identifies the section being relocated, not a symbol.
    if section.info >= len(view.sections):
        raise NormalizeError(
            f"section {section.index} ({section.name}): target section is out of range"
        )
    for offset in range(section.offset, section.offset + section.size, entry_size):
        info_offset = offset + 4
        old_info = _u32(out, info_offset)
        old_symbol = old_info >> 8
        relocation_type = old_info & 0xFF
        if old_symbol >= len(table.symbols):
            raise NormalizeError(
                f"section {section.index} ({section.name}): relocation symbol "
                f"index {old_symbol} is out of range"
            )
        new_symbol = table.old_to_new[old_symbol]
        if new_symbol > 0x00FFFFFF:
            raise NormalizeError("relocation symbol index does not fit ELF32_R_INFO")
        _put_u32(out, info_offset, (new_symbol << 8) | relocation_type)


def _normalize_group(out: bytearray, view: ElfView, section: Section,
                     table: SymbolTable) -> None:
    if section.entsize not in (0, 4):
        raise NormalizeError(
            f"section {section.index} ({section.name}): unsupported group entry size "
            f"{section.entsize}"
        )
    if section.size < 4 or section.size % 4 != 0:
        raise NormalizeError(
            f"section {section.index} ({section.name}): malformed group contents"
        )
    if section.info >= len(table.symbols):
        raise NormalizeError(
            f"section {section.index} ({section.name}): signature symbol "
            f"index {section.info} is out of range"
        )
    for offset in range(section.offset + 4, section.offset + section.size, 4):
        member_section = _u32(out, offset)
        if member_section == 0 or member_section >= len(view.sections):
            raise NormalizeError(
                f"section {section.index} ({section.name}): group member section "
                f"index {member_section} is out of range"
            )
    header_offset = _section_header_offset(view, section.index)
    _put_u32(out, header_offset + 28, table.old_to_new[section.info])


def _normalize_symtab_shndx(out: bytearray, view: ElfView, section: Section,
                            table: SymbolTable) -> None:
    if section.entsize != 4 or section.size != len(table.symbols) * 4:
        raise NormalizeError(
            f"section {section.index} ({section.name}): malformed "
            "SHT_SYMTAB_SHNDX contents"
        )
    old_values = [
        _u32(out, section.offset + i * 4) for i in range(len(table.symbols))
    ]
    for new_index, old_index in enumerate(table.order):
        _put_u32(out, section.offset + new_index * 4, old_values[old_index])


def normalize_elf(data: bytes) -> tuple[bytes, NormalizeReport]:
    """Return a normalized ELF object and a report.

    The returned object has the same length as ``data``.  Every section type
    that can carry a symbol index is either updated explicitly or rejected by
    :class:`NormalizeError`.
    """

    view = _parse_elf(data)
    out = bytearray(data)
    reordered = 0
    relocation_sections = 0
    group_sections = 0

    shndx_links = {
        section.link
        for section in view.sections
        if section.type == SHT_SYMTAB_SHNDX
    }
    for table in view.symbols.values():
        for index, symbol in enumerate(table.symbols):
            if _u16(symbol, 14) == SHN_XINDEX and table.section.index not in shndx_links:
                raise NormalizeError(
                    f"symbol table {table.section.name}: SHN_XINDEX symbol "
                    "has no SHT_SYMTAB_SHNDX section"
                )
        new_bytes = b"".join(table.symbols[index] for index in table.order)
        if new_bytes != view.section_bytes(table.section):
            reordered += 1
            out[table.section.offset:table.section.offset + table.section.size] = new_bytes
        header_offset = _section_header_offset(view, table.section.index)
        if table.local_count != table.section.info:
            _put_u32(out, header_offset + 28, table.local_count)

    for section in view.sections:
        if section.type in (SHT_REL, SHT_RELA):
            _normalize_relocations(out, view, section, _linked_table(view, section))
            relocation_sections += 1
        elif section.type == SHT_GROUP:
            _normalize_group(out, view, section, _linked_table(view, section))
            group_sections += 1
        elif section.type == SHT_SYMTAB_SHNDX:
            _normalize_symtab_shndx(out, view, section, _linked_table(view, section))

    normalized = bytes(out)
    return normalized, NormalizeReport(
        changed=normalized != data,
        symbol_tables=len(view.symbols),
        reordered_symbol_tables=reordered,
        relocation_sections=relocation_sections,
        group_sections=group_sections,
    )


def inspect_elf(data: bytes) -> ElfView:
    """Parse an object for tests and diagnostics without changing it."""

    return _parse_elf(data)


def _archive_member_name(name_field: bytes, longnames: bytes | None) -> tuple[str, int]:
    field = name_field.decode("ascii", errors="replace")
    stripped = field.rstrip()
    if stripped.startswith("#1/"):
        try:
            length = int(stripped[3:])
        except ValueError as exc:
            raise NormalizeError(f"invalid BSD archive name field {field!r}") from exc
        return f"#1/{length}", length
    if stripped in ("/", "/SYM64/", "//"):
        return stripped, 0
    if stripped.startswith("/") and stripped[1:].isdigit():
        if longnames is None:
            raise NormalizeError("archive member uses long name without // table")
        name_offset = int(stripped[1:])
        if name_offset >= len(longnames):
            raise NormalizeError("archive long-name offset is outside // table")
        end = longnames.find(b"/\n", name_offset)
        if end < 0:
            end = longnames.find(b"\n", name_offset)
        if end < 0:
            raise NormalizeError("archive long-name entry is unterminated")
        return longnames[name_offset:end].decode("latin-1"), 0
    if stripped.endswith("/"):
        stripped = stripped[:-1]
    if not stripped:
        raise NormalizeError("archive member has an empty name")
    return stripped, 0


def _parse_archive(data: bytes) -> list[ArchiveMember]:
    if not data.startswith(b"!<arch>\n"):
        raise NormalizeError("not a standard ar archive")
    members: list[ArchiveMember] = []
    longnames: bytes | None = None
    cursor = 8
    while cursor < len(data):
        if cursor + 60 > len(data):
            raise NormalizeError("truncated archive member header")
        header_offset = cursor
        header = data[cursor:cursor + 60]
        if header[58:60] != b"`\n":
            raise NormalizeError(f"archive member at offset {cursor} has bad magic")
        size_field = header[48:58].decode("ascii", errors="replace").strip()
        if not size_field.isdigit():
            raise NormalizeError(f"archive member at offset {cursor} has bad size")
        size = int(size_field)
        data_start = cursor + 60
        data_end = data_start + size
        if data_end > len(data):
            raise NormalizeError("archive member extends beyond file")
        name, bsd_name_length = _archive_member_name(header[:16], longnames)
        payload_offset = data_start
        payload_size = size
        if name == "//":
            longnames = data[data_start:data_end]
        elif name.startswith("#1/"):
            if bsd_name_length > size:
                raise NormalizeError("BSD archive name exceeds member size")
            name_end = data_start + bsd_name_length
            name = data[data_start:name_end].decode("latin-1")
            payload_offset = name_end
            payload_size = size - bsd_name_length
        members.append(ArchiveMember(
            header_offset=header_offset,
            payload_offset=payload_offset,
            payload_size=payload_size,
            name=name,
        ))
        cursor = data_end + (size & 1)
    if cursor != len(data):
        raise NormalizeError("archive has trailing bytes")
    return members


def normalize_archive(data: bytes) -> tuple[bytes, ArchiveReport]:
    """Normalize every ELF object member in an archive without changing size."""

    members = _parse_archive(data)
    out = bytearray(data)
    elf_members = 0
    changed_members = 0
    reordered_symbol_tables = 0
    relocation_sections = 0
    group_sections = 0
    for member in members:
        payload = data[member.payload_offset:member.payload_offset + member.payload_size]
        if payload[:4] != ELF_MAGIC:
            if member.name.endswith((".o", ".obj")):
                raise NormalizeError(
                    f"archive member {member.name}: expected ELF32 ARM object"
                )
            continue
        elf_members += 1
        normalized, report = normalize_elf(payload)
        if len(normalized) != len(payload):
            raise NormalizeError(
                f"archive member {member.name}: normalizer changed member size"
            )
        if normalized != payload:
            changed_members += 1
            out[member.payload_offset:member.payload_offset + member.payload_size] = normalized
        reordered_symbol_tables += report.reordered_symbol_tables
        relocation_sections += report.relocation_sections
        group_sections += report.group_sections
    if elf_members == 0:
        raise NormalizeError("archive contains no ELF object members")
    return bytes(out), ArchiveReport(
        members=len(members),
        elf_members=elf_members,
        changed_members=changed_members,
        reordered_symbol_tables=reordered_symbol_tables,
        relocation_sections=relocation_sections,
        group_sections=group_sections,
    )


def normalize_archive_file(input_path: Path, output_path: Path) -> ArchiveReport:
    """Normalize ``input_path`` into a separate build-copy ``output_path``."""

    source = input_path.resolve()
    destination = output_path.resolve()
    if source == destination:
        raise NormalizeError("refusing to overwrite the input archive")
    data = source.read_bytes()
    normalized, report = normalize_archive(data)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(normalized)
    return report


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Normalize local-after-global ARM ELF symbols in an archive copy"
    )
    parser.add_argument("input_archive", type=Path)
    parser.add_argument("output_archive", type=Path)
    return parser


def main(argv: Iterable[str] | None = None) -> int:
    args = _build_parser().parse_args(argv)
    try:
        report = normalize_archive_file(args.input_archive, args.output_archive)
    except (OSError, NormalizeError) as exc:
        print(f"normalize_armcc_archive: error: {exc}", file=sys.stderr)
        return 2
    print(
        f"normalized {report.changed_members}/{report.elf_members} ELF members "
        f"({report.reordered_symbol_tables} symbol tables, "
        f"{report.relocation_sections} relocation sections, "
        f"{report.group_sections} groups) -> {args.output_archive}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
