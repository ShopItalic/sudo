# Open work

The source tranche is implemented and host-tested. The remaining work is
external qualification, supplier/build review, compatibility migration, and
product decisions:

- **Qualify post-RC1 review fixes.** Exercise voice queue/task allocation failure
  reset/recovery, IQS silence/reset through partial recording drain and settings
  recovery, and non-HID/HID pairing plus bonded reconnect/notification retention.
  Host fault and event-routing tests pass; hardware evidence is still required.
  See [S04-006 through S04-008](reference/supplier-firmware-change-log.md#s04-post-rc1-review-fixes).
- **Qualify corrected task-start failures.** S04 fixes the nine inherited
  `xTaskCreate` checks and adds required-task fatal error handling. LED worker
  allocation-failure injection passes on the host; qualify reset/recovery on
  target before closing the [S03 audit finding](reference/ring-s03-final-audit.html).
- **Measured simplification and long-lived storage.** S04 removes proven dead
  live state, unused stubs and duplicate recording dispatch paths from the
  [final S03 audit](reference/ring-s03-final-audit.html). Wider command removal
  needs a supported-command inventory. Preserve old-file reads and custody;
  measure catalog/start latency and space with many recordings and `.done`
  receipts. No lifetime-capacity result is claimed.

- **Broaden focused validation.** Add an expected-version check to the built
  image validation, allocation-failure tests for the remaining required workers,
  and a real-encoder known-vector test. The current capture harness uses a
  deterministic encoder shim and worker tests stub capture; those results do
  not establish supplier codec fidelity or physical timing.

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
  candidates S01 (historical RC1), S02, S03 (published RC1) and S04 (current
  unsigned RC). S04 host adoption is separate from this firmware-only change;
  no app workaround establishes a release.
- **Keep release and source documentation aligned.** S04 supersedes the old
  ten-second default: PTT runs until release; memo is unlimited when
  `memo_limit_ms=0`. The three mappings and configurable hold activation are
  recorded in [the requirements record](reference/ring-recording-and-ptt.md).
  Keep S04 release status and exact provenance current; retain historical
  release guides unchanged.
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

## S05 Opus qualification (branch codex/opus-audio)

- The experimental S05 source implements 16 kHz mono Opus at 12 kbps CBR with
  bounded memory, a per-recording descriptor and the Sudo container; see
  [S05 in the ledger](reference/supplier-firmware-change-log.md#s05-opus-recording--september-8-2026).
  Remaining gates are physical: encode CPU deadline through FORMAT_GET
  instrumentation, battery versus ADPCM, listening/transcription checks,
  the fitted microphone's actual 16.125 kHz rate, FreeRTOS heap and worker
  stack high-water with the boot-time allocations, and supplier ArmCC
  reproduction. Bitrate tuning stays a firmware profile change; the app has
  no bitrate selection. Preserve S04 RC1 and existing ADPCM files.
- Resolve the cancelled [matching SDK main CI run](https://github.com/ShopItalic/app/actions/runs/34023194481)
  in the app/runner workflow before claiming remote iOS validation; local Ring
  test success does not replace that result.

## S04 qualification

- Measure continuous hold/release recording beyond 10 seconds, one minute and
  multiple minutes on a spare standard Ring. Validate touch-loss shutdown,
  full storage, release during Flash backpressure and completed audio tails.
- Qualify hold, double tap and triple tap on the fitted IQS7211E: initial safe
  mask 0x08, verified fresh mappings mask 0x0C, optional double tap bit 0x02,
  no single/palm/swipe or duplicate legacy actions, 0.5–10-second hold thresholds,
  all action mappings and persisted choices after upgrade/reboot.
- Validate SDK/app events under disconnect/backpressure, sequence gaps,
  press/release/cancel and absent app execution. They are live input events,
  not an offline action queue or an exactly-once remote execution guarantee.
- Validate firmware light/haptic switches and strength/start/stop timing on
  hardware through SETTINGS/TUNING, including readback, mute and reboot.
- Confirm required-task allocation faults reach the Nordic fatal-error/recovery
  path on target. Host injection covers the LED worker; all nine corrected
  task sites require the same success result and fatal error handling.
- Keep the published S03 RC unchanged. The unsigned S04 RC still needs supplier compiler/signing and
  physical audio/BLE/power qualification before production release.
- Wider supplier command-surface reduction still requires a supported-command
  inventory. Keep identity, pairing, time, battery, motion, update compatibility
  and old-recording retrieval intact; do not remove callers without evidence.
