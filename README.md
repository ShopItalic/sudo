# Italic Ring firmware

Production Ring engineering lives in `ShopItalic/sudo`. The hardware baseline
is the Fitwatch / Feiyang Bravechip **603V1.23.2** board.

The [component stack and BOM](docs/reference/sudo-ring-hardware.md), extracted
on September 5, 2026, covers
electronics, mechanics, size variants, charging case, supplier costs, firmware
evidence, and conflicting or missing specifications. The source data is also
available as the [component BOM](docs/reference/sudo-ring/bom.csv), [quoted
cost BOM](docs/reference/sudo-ring/quoted-bom.csv), [workbook extracts](docs/reference/sudo-ring/source-extracts.json),
and [source manifest](docs/reference/sudo-ring/sources.json).

The recovered [raw factory firmware extraction](docs/reference/ring-firmware.md)
includes [application.bin](artifacts/ring-firmware/603v1.23.2-6.0.3.3z62/application.bin)
at 184,132 bytes, the combined
`BCL603S2P_6.0.3.3Z62.hex`, the application-only OTA ZIP, and seven exact
memory regions. SHA-256, ZIP CRC, HEX consistency, DFU metadata, and ECDSA
signature checks passed. These are extracted factory artifacts, not a device
dump or an approved release. No ring was dumped or flashed.

The [Sudo Voice firmware candidate](docs/reference/ring-firmware-candidate.md)
preserves the reviewed factory source at commit `102bfd2` and adds a queued BLE
sender, correct file resume, transfer error handling, and a recording-focused
Keil target. Host fault-injection tests and ARM object checks pass. A production
link, signed update package, measured throughput, and hardware validation remain
pending. Start with that document for the changes and reproduction commands.
Its [Caption comparison](docs/reference/ring-firmware-candidate.md#comparison-with-caption)
identifies recording-safety, recovery and session features to adapt next,
including inherited Ring capture and storage issues outside the transfer patch.

The separate
[`botnetai/ring-firmware`](https://github.com/botnetai/ring-firmware) repository
is an older Nordic/Seeed test-unit prototype and is not the production source.

| Component | Researched baseline |
| --- | --- |
| MCU / SiP | Nordic nRF52840 in Bravechip BCL603S2P; BCL603M3 module |
| Recording flash | GigaDevice GD25WQ128HQIGR, 128 Mbit / 16 MiB raw |
| Touch | Azoteq IQS7211E |
| Microphone | ST MP23DB01HPTR digital PDM |
| Motion | ST LSM6DSO family; supplier identifies LSM6DSOW |
| Haptics | `0518` motor; firmware includes a linear-motor driver |
| Power | Firmware selects YHM2712; package and SiP integration remain unresolved |
| Battery / shell | Size-specific Grepow cells, ceramic exterior, resin liner |
| Charging case | BCL701MHV1.29.2 reference; confirm the supplied case matches |
| Current app audio interpretation | 8 kHz, mono ADPCM; verify against a physical recording |

Start with the [documentation index](docs/README.md),
[firmware import and verification](docs/firmware.md), and the
[open work](docs/backlog.md). Hardware documents are evidence, not an approved
manufacturing or purchasing release.

## Repository boundaries

- [ShopItalic/app](https://github.com/ShopItalic/app): iOS, signed-in web app,
  backend, and client BLE adapters.
- [ShopItalic/store](https://github.com/ShopItalic/store): public website and store.
- [ShopItalic/caption](https://github.com/ShopItalic/caption): Caption e-ink recorder firmware.
- This repository: production Ring hardware and firmware.
