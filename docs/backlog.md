# Open work

- Review and import the vendor `1.23.2_6033固件SDK.zip` source tree as a
  buildable, licensed snapshot. Use the pinned SHA-256 in
  [the source manifest](reference/sudo-ring/sources.json) and identify the
  exact compiler, SDK, target, linker, and packaging tools. Reproduce the
  unmodified vendor build and record its command, tool versions, output hashes,
  and memory use. The recovered raw image is evidence only and does not by
  itself establish a source build or approved release.
- Resolve the [open specifications](reference/sudo-ring-hardware.md#open-specifications),
  including battery ratings, fitted PMIC / haptic parts, case BOM, mechanical
  clearance, RF evidence, and supplier cost discrepancies.
- On a verified production ring, run the physical capture, streaming,
  stored-file sync, reconnect, charging, firmware-update, and read-back matrix
  against [ShopItalic/app](https://github.com/ShopItalic/app). Confirm the
  device's flashed revision, audio framing, transfer behavior, and recovery
  without treating the factory extraction as a device dump.
