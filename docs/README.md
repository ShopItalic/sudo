# Documentation

Start with the **[S04 RC1 download, flashing and test guide](how-to/test-s04-release-candidate.md)**.
The **[current RC release](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S04-rc.1)**
contains unsigned application/debug files and a supplier review bundle.

## Candidate engineering

- [Supplier change log: factory baseline to current source](reference/supplier-firmware-change-log.md).

Current main firmware and primary RC are **S04 / S04 RC1**, merged through [PR #3](https://github.com/ShopItalic/sudo/pull/3):
PTT until release, three independently mapped inputs, adjustable hold activation,
persisted light/haptic controls and conservative cleanup. Fresh defaults are
one-second hold to PTT, double tap off and triple tap to memo toggle.

S04 passes 16,902 C checks, six archive-normalizer tests and the 225-source GNU
ARM build, including GitHub CI. It remains an unsigned engineering prerelease, without physical qualification. S04 client integration is outside this firmware-only change.
Published S03 RC1 and historical S01 RC1 retain their pinned source and binaries.

- [S03 changes and qualification plan](reference/ring-s03-reliability.md).
- [Build, validation evidence and Caption comparison](reference/ring-firmware-candidate.md).
- [Recording/PTT requirements and physical acceptance matrix](reference/ring-recording-and-ptt.md).
- [BLE wire contract](reference/ring-voice-protocol.md).
- [Source import, baseline and build workflow](firmware.md).
- [Engineering follow-ups](backlog.md).
- [Published S03 app candidate](https://github.com/ShopItalic/app/pull/12).

The [historical S01 RC1 source tag](https://github.com/ShopItalic/sudo/tree/v6.0.3.3S01-rc.1)
preserves the matching documentation for those downloads.

## Hardware and original files

- [Component stack and hardware reference](reference/sudo-ring-hardware.md).
- [Component BOM](reference/sudo-ring/bom.csv) and [quoted cost BOM](reference/sudo-ring/quoted-bom.csv).
- [Workbook extracts](reference/sudo-ring/source-extracts.json) and [source manifest](reference/sudo-ring/sources.json).
- [Factory firmware extraction, memory map and signature evidence](reference/ring-firmware.md).

S04 RC1 is `6.0.3.3S04`; historical S03 RC1 is `6.0.3.3S03`; historical S01 RC1 is `6.0.3.3S01`; the factory
distribution is `6.0.3.3Z62`. All target standard `603V1.23.2`. Sudo candidates
are unsigned engineering applications,
not a signed OTA release or evidence of physical qualification.
