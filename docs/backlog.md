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
  candidate `6.0.3.3S01` ten-byte build and do not let the app workaround imply
  a release.
- **Product decisions and historical requirements.** Keep the root/main
  requirements aligned on a configurable default 10-second PTT limit and an
  unlimited memo when `memo_limit_ms=0`. Resolve the historical conflicts in
  [the requirements record](reference/ring-recording-and-ptt.md) explicitly
  before changing defaults or app behavior.
- **Complete keyboard dictation after host selection.** Long-press voice input
  was a requested product behavior. The native app preview currently requires
  Sudo in the foreground, so dictation into another app remains incomplete.
  Select same-iPhone, Mac, or both, then implement the appropriate finite
  background/host consumer. App text → Ring → HID is the related exploratory
  transport question; no cross-host or arbitrary Unicode behavior is established.
- **Physical specifications.** Close the [open specifications](reference/sudo-ring-hardware.md#open-specifications),
  including battery cell and current ratings, fitted PMIC and haptic parts,
  case/mechanical clearance, RF and antenna evidence, crystal/microphone data,
  and supplier cost/BOM discrepancies.
