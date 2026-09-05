# Italic Ring firmware

Production Ring engineering lives in `ShopItalic/sudo`. The hardware baseline
is the Fitwatch / Feiyang Bravechip **603V1.23.2** board.

This repository starts with the completed [component stack and BOM](docs/reference/sudo-ring-hardware.md)
from the “Document Ring BOM and stack” task on September 5, 2026. It covers
electronics, mechanics, size variants, charging case, supplier costs, firmware
evidence, and conflicting or missing specifications.

The vendor SDK has been identified but is **not imported or built here yet**.
There is no flashable release in this repository. The separate
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

Start with [firmware import and verification](docs/firmware.md) and the
[open work](docs/backlog.md). Hardware documents are evidence, not an approved
manufacturing or purchasing release.

## Repository boundaries

- [ShopItalic/app](https://github.com/ShopItalic/app): iOS, signed-in web app,
  backend, and client BLE adapters.
- [ShopItalic/store](https://github.com/ShopItalic/store): public website and store.
- [ShopItalic/caption](https://github.com/ShopItalic/caption): Caption e-ink recorder firmware.
- This repository: production Ring hardware and firmware.
