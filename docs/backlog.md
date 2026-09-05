# Open work

- Adapt the [Caption recording lifecycle](reference/ring-firmware-candidate.md#comparison-with-caption)
  for the selected production target: local backup before live BLE, a bounded
  recording worker, and an explicit encoder/capture-queue drain before Stop
  finalizes the file. The current vendor stop clears queued audio.
- Propagate recording write/sync failures instead of ignoring them or advancing
  accepted-byte counts. Preserve partial recordings and expose a fault. Review
  low-space reclamation, which can delete files without a durable phone receipt;
  define retention/full-storage behavior before enabling continuous live backup.
- Adapt Caption's bounded recording journal/recovery and checkpoint fault tests
  to LittleFS and the Ring's memory budget while preserving raw ADPCM and
  logical file-resume offsets. Verify power-loss behavior on hardware.
- Design a versioned Ring/app session-rejoin and readiness contract, separating
  BLE queue acceptance, live ACK progress, durable phone receipt and completed
  transcription. Caption's saved-file catch-up and automatic rollover remain
  unfinished; they are not implementations to copy.
- Rebuild factory source commit `102bfd2` with Arm Compiler 5.06 update 7
  (build 960), then link the [Sudo Voice candidate](reference/ring-firmware-candidate.md).
  Resolve the existing vendor warnings and inspect all memory/stack boundaries,
  opaque library compatibility, and the selected six/one notification window.
  Host fault tests and GNU ARM object checks are complete; no production image
  has been linked, signed, installed, or measured.
- Run the candidate's supplier acceptance matrix on a spare production ring:
  transfer integrity and throughput, resume/cancellation, small MTU and stalled
  notifications, persisted settings, recording, charging/thermal behavior,
  pairing, and DFU/recovery. Enable app resume only after joint validation.
- Confirm the lean feature scope with the supplier. Temperature and shared
  motion dependencies remain; PPG was already disabled. The candidate excludes
  optional phone/media HID actions and vendor batch uploads.
- Resolve the [open specifications](reference/sudo-ring-hardware.md#open-specifications),
  including battery ratings, fitted PMIC / haptic parts, case BOM, mechanical
  clearance, RF evidence, and supplier cost discrepancies.
- On a verified production ring, run the physical capture, streaming,
  stored-file sync, reconnect, charging, firmware-update, and read-back matrix
  against [ShopItalic/app](https://github.com/ShopItalic/app). Confirm the
  device's flashed revision, audio framing, transfer behavior, and recovery
  without treating the factory extraction as a device dump.
