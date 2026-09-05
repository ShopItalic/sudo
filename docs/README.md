# Documentation

This repository documents the production Italic Ring hardware and firmware.

## Start here

- [Sudo Voice firmware candidate](reference/ring-firmware-candidate.md): preserved source, BLE improvements, lean target, validation, and supplier acceptance.
- [Firmware baseline](firmware.md): factory source, toolchain, and acceptance gates.
- [Raw factory firmware extraction](reference/ring-firmware.md): recovered image files, memory map, hashes, and signature checks.
- [Component stack and BOM](reference/sudo-ring-hardware.md): detailed electronics, mechanics, supplier costs, and open specifications.
- [Component BOM](reference/sudo-ring/bom.csv), [quoted cost BOM](reference/sudo-ring/quoted-bom.csv), [workbook extracts](reference/sudo-ring/source-extracts.json), and [source manifest](reference/sudo-ring/sources.json).
- [Open work](backlog.md): remaining SDK, manufacturing, and physical-device gates.

The raw image is an extraction of a factory distribution. It does not prove the
firmware currently flashed to a physical ring or establish an approved release.

## Repository boundaries

Production Ring hardware and firmware live here. The iOS app and its BLE adapter
remain in [ShopItalic/app](https://github.com/ShopItalic/app); Caption firmware
remains in [ShopItalic/caption](https://github.com/ShopItalic/caption). The older
Nordic/Seeed prototype remains in
[`botnetai/ring-firmware`](https://github.com/botnetai/ring-firmware) and does not
define the production board or BLE contract.
