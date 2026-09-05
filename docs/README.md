# Documentation

This repository documents the production Italic Ring hardware and firmware,
including the current Sudo Voice engineering candidate and the evidence needed
for a production decision.

## Start here

- [Sudo Voice firmware candidate](reference/ring-firmware-candidate.md): preserved source, BLE improvements, lean target, validation boundaries, and supplier acceptance.
- [Recording and push-to-talk requirements](reference/ring-recording-and-ptt.md): lifecycle, historical decisions, protocol expectations, and the physical acceptance matrix.
- [Caption comparison](reference/ring-firmware-candidate.md#comparison-with-caption): reusable recording/recovery/session work and source-evidence limits.
- [Firmware baseline](firmware.md): factory source, exact version identities, current recording owner, toolchain, host validation, and acceptance gates.
- [Raw factory firmware extraction](reference/ring-firmware.md): recovered image files, memory map, hashes, and signature checks.
- [Component stack and BOM](reference/sudo-ring-hardware.md): detailed electronics, mechanics, supplier costs, factory evidence, and open specifications.
- [Component BOM](reference/sudo-ring/bom.csv), [quoted cost BOM](reference/sudo-ring/quoted-bom.csv), [workbook extracts](reference/sudo-ring/source-extracts.json), and [source manifest](reference/sudo-ring/sources.json).
- [Open work](backlog.md): remaining supplier, app, firmware-build, and physical-device gates.

## Current source tranche

The firmware now has one worker-owned recording lifecycle with a completed-block
capture tail guard, a bounded durable LittleFS raw-ADPCM store, and read-only
recovery that exposes only a checksummed committed prefix. The native ID-based
voice protocol and [ShopItalic/app](https://github.com/ShopItalic/app) adapter
cover recording state, live data, catalog/resume, transfer, and custody receipts.
Touch tuning, battery error/filter handling, and haptic settings plumbing are
also implemented and host-tested.

The factory distribution remains exactly `6.0.3.3Z62`. The separate Sudo Voice
candidate is `6.0.3.3S01`, a ten-byte build string plus NUL in the legacy version
field. Candidate source changes and host tests do not establish a signed image,
a flashed device, or physical qualification. Current final build validation is
pending the root integration rerun.

## Repository boundaries

Production Ring hardware and firmware live here. The iOS app and its BLE adapter
remain in [ShopItalic/app](https://github.com/ShopItalic/app); Caption firmware
remains in [ShopItalic/caption](https://github.com/ShopItalic/caption). The older
Nordic/Seeed prototype remains in
[`botnetai/ring-firmware`](https://github.com/botnetai/ring-firmware) and does not
define the production board or BLE contract.

The raw [factory firmware extraction](reference/ring-firmware.md) is an
extraction of a factory distribution. It does not prove the firmware currently
flashed to a physical ring or establish an approved release.
