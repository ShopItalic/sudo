# Documentation

Start with the **[RC1 download, flashing and test guide](how-to/test-release-candidate.md)**.
The **[GitHub prerelease](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S01-rc.1)**
contains downloadable application/debug files and a supplier review bundle.

## Candidate engineering

The RC source is pinned on its tag; these links work from the default branch.

- [Build, validation evidence and Caption comparison](https://github.com/ShopItalic/sudo/blob/v6.0.3.3S01-rc.1/docs/reference/ring-firmware-candidate.md).
- [Recording/PTT requirements and physical acceptance matrix](https://github.com/ShopItalic/sudo/blob/v6.0.3.3S01-rc.1/docs/reference/ring-recording-and-ptt.md).
- [BLE wire contract](https://github.com/ShopItalic/sudo/blob/v6.0.3.3S01-rc.1/docs/reference/ring-voice-protocol.md).
- [Source import, baseline and build workflow](https://github.com/ShopItalic/sudo/blob/v6.0.3.3S01-rc.1/docs/firmware.md).
- [Engineering follow-ups](https://github.com/ShopItalic/sudo/blob/v6.0.3.3S01-rc.1/docs/backlog.md).
- [Matching app candidate](https://github.com/ShopItalic/app/pull/10).

## Hardware and original files

- [Component stack and hardware reference](reference/sudo-ring-hardware.md).
- [Component BOM](reference/sudo-ring/bom.csv) and [quoted cost BOM](reference/sudo-ring/quoted-bom.csv).
- [Workbook extracts](reference/sudo-ring/source-extracts.json) and [source manifest](reference/sudo-ring/sources.json).
- [Factory firmware extraction, memory map and signature evidence](reference/ring-firmware.md).

RC1 is `6.0.3.3S01`; the preserved factory distribution is `6.0.3.3Z62`.
Both target standard `603V1.23.2`. RC1 is an unsigned engineering application,
not a signed OTA release or evidence of physical qualification.
