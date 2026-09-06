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
implements hold/release push-to-talk, configurable double-tap memos, local Flash
recording while connected or standalone, explicit final-file results, checked
recovery and resumable Bluetooth delivery. The matching
[app candidate](https://github.com/ShopItalic/app/pull/10) adds verified live
transcription and bounded PTT dictation through the existing iPhone keyboard.
Candidate version
**6.0.3.3S02** is distinct from factory **6.0.3.3Z62**. This post-RC1
follow-up makes double-tap recording opt-in and adds SDK-controlled master
lights/haptics for normal application feedback. Existing saved choices are
retained. The [published S01 RC1](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S01-rc.1)
remains unchanged; its double tap defaults on and feedback switches affect
recording only. See the [controls contract](docs/reference/ring-voice-protocol.md#application-controls-in-s02).

The integrated host tests pass, and the GNU target compiles and links all
225 sources with no undefined symbols. Physical radio/audio,
power-loss, battery, pairing and DFU acceptance remains pending. The candidate
is an unsigned engineering build. No ring has been dumped or flashed.

The [recording requirements and acceptance matrix](docs/reference/ring-recording-and-ptt.md)
records the earlier supplier briefs, the Chinese connected-recording request,
implemented behavior and remaining product/device decisions. The
[native wire protocol](docs/reference/ring-voice-protocol.md) defines the shared
firmware/app contract. The [Caption comparison](docs/reference/ring-firmware-candidate.md#comparison-with-caption)
explains the recording ownership, tail-drain, recovery and session concepts
adapted to this Ring's smaller memory and existing ADPCM codec.

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
