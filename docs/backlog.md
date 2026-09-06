# Open work

The source tranche is implemented and host-tested. The remaining work is
external qualification, supplier/build review, compatibility migration, and
product decisions:

- **Physical qualification and measurements.** On a spare, identified
  production ring, read back the board and firmware identity; measure PDM/audio
  framing, PTT release and stop timing, connected and standalone capture, BLE
  loss/rejoin, transfer throughput and resume, battery/charging/thermal
  behavior, touch, haptics, LEDs, and power draw. The factory extraction is not
  a device dump. Record the evidence before enabling a release or app rollout.
- **Supplier build, signing, and recovery review.** Reproduce the factory and
  candidate builds with Arm Compiler 5.06 update 7 (build 960), compare the
  linked GNU engineering candidate, inspect stack/ABI boundaries and warnings, and have
  the supplier confirm signing, packaging, DFU, anti-rollback, and recovery
  behavior for the correct board.
- **GNU runtime measurements and diagnostics.** The candidate now has checked
  `_sbrk`, task-local Newlib state, recursive scheduler locks and guarded ISR
  call sites. Measure scheduler latency and actual stack/heap high-water use
  on hardware. GNU stdout currently fails through `libnosys`; define a bounded
  diagnostic transport before relying on UART stdout during qualification.
- **Legacy-file migration.** The native candidate already reads baseline legacy
  root files and exposes the virtual legacy filename bridge. Add a verified,
  retryable custody migration before any deletion; retain old files
  until that migration is proven and retryable.
- **App capability/version coordination.** Validate the native protocol and
  capability/version gating with [ShopItalic/app](https://github.com/ShopItalic/app).
  Keep the factory `6.0.3.3Z62` identity distinct from the engineering
  candidates `6.0.3.3S01` (RC1) and `6.0.3.3S02` and `6.0.3.3S03` (current source) and do not let the app workaround imply
  a release.
- **Product decisions and historical requirements.** Keep the root/main
  requirements aligned on a configurable default 10-second PTT limit and an
  unlimited memo when `memo_limit_ms=0`. Resolve the historical conflicts in
  [the requirements record](reference/ring-recording-and-ptt.md) explicitly
  before changing defaults or app behavior.
- **Qualify keyboard dictation and settle additional hosts.** The provisional
  same-iPhone path now has finite background PTT and final-only keyboard
  admission. Verify BLE wake, model execution, actual host-field insertion,
  expiration and force-quit on an iPhone. Mac forwarding and app text → Ring →
  HID remain separate scope choices; no cross-host or arbitrary Unicode
  contract is established.
- **Physical specifications.** Close the [open specifications](reference/sudo-ring-hardware.md#open-specifications),
  including battery cell and current ratings, fitted PMIC and haptic parts,
  case/mechanical clearance, RF and antenna evidence, crystal/microphone data,
  and supplier cost/BOM discrepancies.

## S02 application controls follow-up

- Reproduce the reported stuck double-tap recording on hardware; qualify the
  S02 hold-to-stop escape, final file drain, and release-before-restart gate.
- Qualify double-tap opt-in, persisted gesture choices, master lights/haptics
  off, manual-output rejection and mute/unmute on a standard spare Ring.
- Verify the app shows recording-only scope on S01 and application scope on S02.
- Confirm the startup settings-load boundary and measure actual motor/LED off
  timing; bootloader feedback remains independent.
- S03 RC1 carries the S02 controls forward in a separate candidate. Preserve
  the historical S01 RC1 tag and assets.

## S03 measurement gates

- Execute the [S03 qualification plan](reference/ring-s03-reliability.md#qualification-still-required): actual transfer byte/time measurements, motor quiet-period and battery curve calibration, keyboard confirmation under app/connection loss, and physical pulse distinction.
- Measure delayed ACK/retry behavior, the 30-second stall deadline, later Flash
  shutdown and touch/STOP latency under packet backpressure at ATT 20 and 244.
- Test proof reuse on hardware, including catalog refresh, reconnect, partial resume, full phone storage and a retained receipt retry; do not equate the removed duplicate read with a measured throughput figure.
- Confirm S03 supplier build/signing/recovery before consumer OTA distribution. Preserve the separate S01 RC1 tag and files.

## Proposed codec evaluation

- Evaluate 16 kHz mono Opus at an initial 16 kbps against S03 ADPCM, as recorded
  in [decision S03-006](reference/supplier-firmware-change-log.md#s03-006--record-the-codec-decision-and-proposed-opus-evaluation).
  This is a proposed candidate, not an implemented codec change. Verify capture
  rate, peak encoding time, memory, PTT/recording integrity, resumed storage/SDK
  compatibility, transcription quality, transfer cost and total energy on the
  standard Ring. Preserve the S03 RC and existing ADPCM files.
- Resolve the cancelled [matching SDK main CI run](https://github.com/ShopItalic/app/actions/runs/34023194481)
  in the app/runner workflow before claiming remote iOS validation; local Ring
  test success does not replace that result.
