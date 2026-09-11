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

## Brick-risk audit — 2026-09-11

A failure-oriented re-audit after the second dev-Ring bricking. Scope: interrupt
context, task stacks/heap, boot/DFU/recovery, and the release pipeline. The
confirmed S05 mechanism is a **GNU-only** defect: `firmware/gnu/newlib_locks.c`
calls `NVIC_SystemReset()` when a Newlib lock is taken with `__get_IPSR() != 0`,
and `NRF_SDH_DISPATCH_MODEL=0` (`firmware/BCL603S2X/app/user/inc/sdk_config.h`)
runs BLE callbacks in SWI2 interrupt context. The production ArmCC5 Sudo project
does not compile `newlib_locks.c`, so that specific reset is not in the ArmCC5
candidate; the source-level and release safeguards below still apply.

### Fixed in this audit

- **Hosted defective binary withdrawn.** `tools/firmware-hosting/s05-manifest.json`
  now marks the S05 GNU binary withdrawn with a null `binary.url` and
  `flashable: false`; a new `tests/firmware/test_release_manifest.py` policy test
  refuses a public manifest that advertises a non-ArmCC5, non-flashable or OTA
  artifact. The live Cloudflare asset must be deleted and verified 404 by an
  operator (see the hosting README).
- **CI cannot publish a failed GNU build.** `.github/workflows/firmware-checks.yml`
  gates `gnu-build` on `host-tests` and uploads artifacts only when the build
  step succeeds, with a `NON-FLASHABLE.txt` notice in the artifact.
- **NULL timer handle HardFault removed.** `app_connect_idie_timer_start` now
  returns when heap-pressure timer creation failed instead of passing NULL to
  `xTimerGenericCommand`, matching the existing ISR helper.
- **Queue allocation fails closed.** Under `SUDO_VOICE_ONLY`, `bc_queue_init`
  now enters the fatal-error path on queue-creation failure instead of leaving a
  NULL transport queue.
- **Reset-reason accessor returned.** `bsp_sys_reset_reason()` now returns the
  value it already reads, so `SYS_RESET_REASON_GET` no longer reports an
  undefined stack value.

### Still open — highest priority first

1. **No boot-time recovery escape (the core brick).** A valid-CRC application
   that resets before serving BLE is booted forever; the factory bootloader
   disables button and pin-reset DFU entry, and buttonless DFU requires a live
   acknowledged GATT connection (`bc_ble_dfu.c`, `ble_dfu_unbonded.c`,
   `nrf_bootloader.c`). The watchdog only turns a hang into the same reset loop.
   Add a retained boot-attempt counter (`.noinit`/System-OFF retained RAM) that
   sets `BOOTLOADER_DFU_START` in `GPREGRET` after N unsuccessful boots, or have
   the supplier enable the pin-reset DFU entry. This needs a target build and a
   recoverable spare, and is intentionally not implemented from source alone.
2. **SDK flash/error calls reachable from SWI2.** Peer Manager event handling can
   drive FDS/flash operations and several `APP_ERROR_*` paths reset from the BLE
   interrupt (`bc_ble.c`, `peer_manager_handler.c`, `bc_ble_adv.c`,
   `bc_ble_hids_service.c`). Verify whether FDS defers `sd_flash_*` out of the
   SWI2 context and move any that do not to task context; do not blanket-reset
   from an interrupt.
3. **Unmeasured stacks and ISR/MSP stack.** The three confirmed overruns are
   rebudgeted, but `ble send` (1 KiB), `ic led` (512 B), the timer task and the
   8 KiB MSP/interrupt stack still need high-water measurement. The
   `configCHECK_FOR_STACK_OVERFLOW` reset hook amplifies an unresolved overflow
   into a boot loop given item 1.
4. **Opus allocation downgrade is silent.** `vApplicationMallocFailedHook` only
   sets a flag with no firmware reader, and a failed 35.8 KiB encoder/scratch
   allocation leaves recording `UNSUPPORTED` while the app looks healthy.
5. **Version identity is not verified at release.** The image can fall back to
   the factory `6.0.3.3Z62` string if `SUDO_VOICE_ONLY` is lost, and no build or
   release check reads the embedded version. Add a post-build identity check
   before any hosted or signed artifact.
6. **GNU build remains buildable.** Even with release gating, CI still emits a
   GNU image with the reset guard. Consider failing the GNU link or removing the
   reset backend once the ArmCC5 path is the sole production route.

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

## S05 download hosting — 2026-09-09

- Deployed `italic-ring-firmware` at `firmware.italic.com` with CI-built S05
  binary, checksums, notices and explicit unsigned/OTA-unavailable metadata.
  Config and source provenance: `tools/firmware-hosting/`.
- Public HTTPS download and SHA-256 verified. A logged Super Bot Fight Mode
  exception applies only to GET/HEAD on firmware.italic.com at `/` or
  `/ring/s05/`; managed WAF and rate limiting remain enabled.
- **Withdrawn 2026-09-11.** The hosted GNU 15.2.rel1 binary
  (`e99d68e9…`, 464,220 bytes) is the exact artifact whose interrupt-context
  MCU reset routes were confirmed in the S05 failure audit. It is unsafe to
  flash. `tools/firmware-hosting/s05-manifest.json` now sets `withdrawn: true`,
  a null `binary.url` and `binary.flashable: false`. An operator must redeploy,
  delete the remote path, purge the cache and confirm the URL returns 404; see
  the hosting README. Do not restore any download until the replacement passes
  the audit and physical qualification.
- Obtain supplier-signed S05 DFU package and validate it for the target Ring.
  ShopItalic/app commit `6e9ceb3` implements HTTPS release checking and verified
  package preparation; keep OTA unavailable until the signed ZIP is published.

## S05 physical update failure — 2026-09-09

- A user-authorized application-only engineering DFU of source
  `8632de604b2bd79c103e43292e91eab4ba13aed1` completed its Bluetooth transfer on
  a Ring freshly identified as standard `603V1.23.2`, factory `6.0.3.3Z62`.
  The transferred 464,220-byte BIN SHA-256 was
  `e99d68e970476da98034e47c6f6a4872766f490be2fe7ea3ea55bf0c4eae3f3d`.
  A locally produced DFU signature was verified against the factory public
  key; this was not supplier qualification or a public OTA release.
- **Failed acceptance:** the app could not read back `6.0.3.3S05` after the
  transfer. Subsequent iPhone pairing timed out. A separately authorized Mac
  check saw the normal Ring name and HID advertisement but could not establish
  an application connection, including with iPhone Bluetooth off. No firmware
  version, working recording, or usable DFU service was read from the updated
  device. Advertising does not establish that the application is healthy.
- Preserve the unresolved app maintenance record and recording custody. Do not
  treat the completed transfer, valid signature, GNU build, or host tests as
  successful installation. Keep public OTA unavailable and withhold this build
  from further device testing until the failure and recovery are resolved.
- The [continued S05 failure audit](reference/ring-s05-failure-audit.html)
  reproduces reset requests in the preserved application binary; the physical
  Ring's first fault and reset sequence still need target evidence. The
  supplied bootloader configuration disables button
  and pin-reset DFU entry; it supports the application-requested GPREGRET path.
  A compatible signed factory package exists, but no usable transport to
  install it is established. Obtain a verified recovery entry procedure and
  physical debug access if an iPhone connection cannot be recovered. Preserve
  device flash and external recordings before any debugger erase/recovery.
- A direct same-iPhone test in Nordic nRF Connect 2.8.2 also remained at
  `Connecting`: advertisements were received and marked connectable, but no
  successful connection callback or service discovery appeared after several
  minutes. Placing the Ring on its powered charger did not immediately restore
  the connection. The user reports the Mac never connected historically, so
  the Mac result does not establish a regression caused by this update.
- USB capture of the same iPhone's `bluetoothd` on September 9, 16:04–16:06
  confirmed 78 controller connection-complete events followed by 78
  disconnections for this Ring. Identity was correlated against the app's
  saved Ring association, not inferred from nearby-device log timing. iOS
  reported reason `762`, `encryptionPending 0`, `linkReady:0`, and skipped the
  application disconnection callback because the link never became ready.
  Nordic nRF Connect still exposed no services. This localizes the observed
  failure before app commands/service discovery; it does not identify the
  firmware fault or prove permanent hardware damage. A status-only HCI logger
  opened but returned zero packets in 45 seconds, so the raw HCI error remains
  unverified. Do not infer a pairing-key failure from the Apple numeric code.

### S05 runtime audit continuation — 2026-09-09

- The hash-pinned original ARM ELF/BIN reproducer now runs 14 scenarios. Six
  interrupt routes reach the GNU Newlib reset guard: BLE connection, RTC
  calendar conversion, Peer Manager security start, BLE receive logging,
  PWM0 stopped notification, and motion GPIO. Cold/warm BLE logging are two
  variants of one route. Keep the guard and remove/defer incompatible library
  calls throughout these callbacks; removing the first BLE log alone is
  insufficient. The PWM probe models the already-idle stopped state, not
  physical PWM activity.
- Rebudget motor, hardware-check and motion worker stacks. The first two
  write 664 bytes against 512-byte budgets. The motion timer-creation helper
  alone writes 672 bytes, excluding its task/driver caller frames. Enable
  stack/allocation diagnostics and measure all worker and timer paths with
  interrupt/preemption margin. These are CPU-model measurements with relocated
  stacks, not a readout of corrupted device RAM.
- Fix the motion GPIO callback's task-context timer command with a 50-tick
  wait (`bc_gsensor.c`, `bc_rtos.h`): use an ISR-safe command or defer to the
  worker. The selected binary contains this mismatch, but the earlier logging
  reset prevents the current probe from reaching it.
- The original ARM Opus startup preparation/silent-frame self-test returns
  success with 4,872-byte helper stack depth in the isolated model. All 352
  selected source hashes match the incident inventory; the full host suite
  and 7,056-file supplier baseline verification pass again (one existing
  Python test skipped). These results do not qualify full boot, sustained
  audio, runtime heap pressure, radio coexistence or physical recovery.
- Preserve the original failing artifact and its forensic reproducer at
  `tools/firmware/reproduce_s05_runtime_failures.py`. A successful reproducer
  exit means expected defects were observed, not that firmware is safe.
  Future repaired builds need integration tests expecting no reset/overrun,
  then the [hardware qualification workflow](how-to/qualify-firmware.md).

### Arm Compiler 5 Docker preparation — 2026-09-09

- The pinned Ubuntu x86-64 host image and 32-bit compatibility dependencies
  build on MBP-M5; both 32-bit and 64-bit x86 execution probes pass.
- Installed the user-selected Linux build 960 mirror after EULA approval.
  It identifies itself as Arm Compiler 5.06u7 **for Certification**. Preserve
  that edition and the archive/tool hashes in build provenance; matching the
  filename does not establish equivalence with Bravechip's actual build.
- Actual `armcc --vsn` and a minimal Cortex-M4 compile both exit 1 with
  `C9555E` because no license file/server is configured. Jeremy confirmed the
  supplier has a license for the sample collaboration. Configure compatible
  compiler access locally or use the supplier's configured build environment
  before baseline/candidate compilation can proceed.
- The host check is not firmware qualification. At this Docker-preparation
  stage neither target had compiled. The Windows route subsequently completed
  the software qualification below. See [the setup and exact evidence](how-to/qualify-firmware.md#linux-docker-host-preparation).

### ArmCC 5 software qualification — 2026-09-10

- **Completed:** legitimate MDK Professional evaluation and exact Arm Compiler
  5.06u7 for Certification build 960 installed on MBP-M5 through CrossOver.
  CMSIS 5.7.0 and Nordic DeviceFamilyPack 8.35.0 are installed. The Linux Docker
  compiler remains separately unlicensed; it is not the selected working build.
- **Completed:** the supplier baseline compiles/links with zero errors and
  exactly matches the preserved factory application: 184,132 bytes,
  SHA-256 `96f1186e20f09ea2b97f48090626965437fb5e8cc4ff8b87679294cea8d853bf`.
  All 7,056 source archive entries verify. The prepared build regenerated only
  comment/formatting content in its RTE header; all other originals match.
- **Completed:** the repaired candidate clean build produces 351 objects,
  zero errors, 588 compiler warnings and no linker warnings. All 123 Opus
  1.6.1 sources remain selected. Original ARM startup, system source, allocator,
  ARM/CMSIS FreeRTOS port and supplier algorithm archive are unchanged.
- **Completed:** interrupt logging/calendar failures, motion timer context,
  the BLE connection tick read, the three demonstrated stack overruns and
  disabled stack/allocation diagnostics are addressed. Touch, BLE receive,
  shared timer and idle budgets also received margin after linked-path review.
  The full host suite and 33 actual ARM instruction scenarios pass, including
  the intended diagnostic reset after deliberate stack-guard corruption.
- **Evidence:** [qualification report](reference/ring-armcc5-qualification.html)
  and [repeatable commands](how-to/qualify-firmware.md#repeat-the-recorded-software-build).
  Candidate BIN: 330,648 bytes, SHA-256
  `227ce7e5cd68d8aaa09660182e39960d55360b8892ea983de6da311f2d2c1238`.
  Compiler/actual input/object/output hashes, maps, logs and model boundaries
  are recorded. Actual flash/RAM placement fits physical/reserved bounds.
- **Still open, physical scope:** verified recovery for the failed Ring,
  identified recoverable test hardware, firmware readback, full boot, bonding,
  reconnect, audio/transfer, sensor/bus/power behavior and actual task/heap
  high-water measurements under sustained workload and interrupt nesting.
  No signing, flashing, publishing or recovery operation was performed here.
- **Follow-up source scope:** retain and triage inherited compiler warnings
  where reachable, including missing-return diagnostics. Static call graphs
  have unresolved indirect calls/cycles and are not worst-case stack proofs.
  Keep the post-link physical-bound checks: the supplier's declared maximum
  RAM/ROM regions extend beyond the usable application boundaries, although
  both actual linked images fit. Keep the original failing GNU reproducer as
  evidence; do not replace its artifact or remove its interrupt guard.
