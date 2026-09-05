# Open work

- Import `1.23.2_6033固件SDK.zip` and the original BOM CSV / source-hash exports
  from the completed MBA task. This Mac could read its task results but could
  not reach the source checkout over SSH; the connected Drive search did not
  expose the archive. Keep the original files intact.
- Establish the vendor build and release workflow from that source; there is
  no verified compiler command or firmware binary in this repository yet.
- Resolve the [open specifications](reference/sudo-ring-hardware.md#open-specifications),
  including battery ratings, fitted PMIC / haptic parts, case BOM, mechanical
  clearance, RF evidence, and supplier cost discrepancies.
- Run the physical Ring capture, streaming, stored-file sync, reconnect,
  charging, and firmware update matrix against `ShopItalic/app`.
