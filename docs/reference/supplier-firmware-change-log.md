# Supplier firmware change log

This ledger records the firmware change from the preserved factory **603V1.23.2 /
6.0.3.3Z62** distribution to the Sudo Voice candidate. It is written for a
supplier reviewing board compatibility, Bluetooth behavior, performance
assumptions, and the SDK boundary.

The ledger separates:

- **factory source preservation**: the imported supplier files remain available
  byte-for-byte;
- **profile exclusion**: an entry is omitted from the generated Sudo Voice
  project and is not deleted from the factory import;
- **implemented software**: source and host/target checks support the behavior;
- **device acceptance pending**: a physical Ring, fitted sensor/Flash, RF
  measurements, and supplier recovery/compiler checks are still required.

S03 entries describe the engineering candidate in this source tree. Its
[separate release](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S03-rc.1)
records immutable source, CI, image hashes and SDK identity in `provenance.json`.
No entry is a claim about firmware installed on a physical Ring or signed OTA readiness.

## Version and evidence map

| ID | Version/status | Evidence boundary |
| --- | --- | --- |
| F0 | Factory baseline, 6.0.3.3Z62 | Git baseline 102bfd2; the imported SDK is recorded in [firmware/source-manifest.json](../../firmware/source-manifest.json). The extracted factory image is documented in [ring-firmware.md](ring-firmware.md). |
| S01 | 6.0.3.3S01; published unsigned RC1 | Tag [v6.0.3.3S01-rc.1](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S01-rc.1), source commit bde62175ea2c3c64a0f633abb6d06d05595d9cc7, CI run 33998477755. The GitHub release assets and tag are immutable evidence for that build. |
| S02 | 6.0.3.3S02; published on main | Commits 2ba1aa5, 7ffd2c8, and merge 0860373. S02 has source and host/CI evidence, but no separate S02 release asset set. The older S01 binary/checksums remain unchanged. |
| S03 | 6.0.3.3S03; engineering candidate | The candidate changes [sudo_voice_profile.h](../../firmware/bc_ros/bc_config/sudo_voice_profile.h), recording service/protocol, motor, ADC, PMIC, and related tests. Use the separate S03 release provenance to identify the compiled source. The matching app transport/protocol work is commit [492af2a](https://github.com/ShopItalic/app/commit/492af2ad40949de3d54419df5e2fa140c912b94f), merged through [app PR #12](https://github.com/ShopItalic/app/pull/12) into main `664ea438c7265d57885f926e589acb0faa1d05ca`. |

The relevant source sequence is visible with:

~~~text
102bfd2  Import reviewed 603V1.23.2 factory SDK source baseline
700f307  Improve Ring BLE transfers and add a recording-focused firmware candidate
6d382ca  Implement local Ring recording and reliable PTT candidate
72b9c52  Restore PTT requirements and fix truncated recording starts
bde6217  Preserve live audio prefix and finalize Ring PTT review evidence
2ba1aa5  Make double-tap opt-in, add hold-to-stop and SDK feedback controls
7ffd2c8  Document S02 validation and keep RC1 download provenance explicit
0860373  Merge S02 work into main
~~~

These are repository-history facts; they do not establish physical timing,
radio throughput, audio fidelity, battery life, or qualification.

## Change ledger

### F0-001 — Preserve the supplier baseline

**Version/status:** F0, preserved factory source and extracted artifacts.

**Before → after:** The review starts from the supplier archive for standard
603V1.23.2, whose application identifies itself as 6.0.3.3Z62. The archive,
source manifest, factory combined HEX, application image, OTA metadata, and
component notices are retained. The extracted factory HEX includes MBR, S140,
application, bootloader, DFU settings, and UICR words; a Sudo application image
must not be described as a complete device backup.

**Why:** Suppliers need a reproducible reference and must be able to distinguish
the standard board from 1.23.2_one_sec and other factory variants.

**Principal evidence:**

- [firmware/source-manifest.json](../../firmware/source-manifest.json)
- [docs/reference/ring-firmware.md](ring-firmware.md)
- [artifacts/ring-firmware/603v1.23.2-6.0.3.3z62](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62)
- [firmware/BCL603S2X/app/project/mdk5/bc_ring_app.uvprojx](../../firmware/BCL603S2X/app/project/mdk5/bc_ring_app.uvprojx) at baseline 102bfd2

**Compatibility and supplier validation:** The source archive and extracted
artifacts have recorded hashes and Intel HEX/DFU consistency checks. This is
archive evidence, not a readout of a physical Ring. A supplier must confirm the
board revision, installed SoftDevice/bootloader, per-device settings, bonds,
calibration, access protection, and external recording Flash before updating
hardware.

### PROFILE-001 — Generate the Sudo Voice target without deleting factory code

**Version/status:** S01/S02/S03 target profile; implemented in the project
generator and generated profile.

**Before → after:** The factory project contains the supplier's broad target
selection. [make_voice_project.py](../../tools/firmware/make_voice_project.py)
copies only the standard 1.23.2 target, renames it Sudo Voice 1.23.2, adds
the reviewed recording/archive sources and BLE transport helpers, defines
SUDO_VOICE_ONLY, and emits a 227-entry project. [voice-profile.json](../../firmware/voice-profile.json)
records **238 excluded project entries**:

- **108** were already disabled in the supplier 1.23.2 target;
- **126** are the Opus tree, because the production recording path uses the
  retained vendor ADPCM contract;
- **2** legacy recording files, app_ppg_file_data_handler.c and
  app_pdm_handler.c, are replaced by the Sudo recording/archive worker;
- **2** unused traffic/alternative-encoder files, app_ble_speed_handler.c
  and app_opus.c.

The exclusions include PPG/health paths (app_ppg_*, bc_ppg, Goodix/HX3605
health algorithms and libraries), alternate motion/IMU, NFC, alternate touch,
alternate haptic, alternate fuel-gauge, and other supplier-disabled modules.
The profile also sets PPG_ENABLED 0 and disables the optional phone/media/
presentation gesture macros. Those factory files remain in the 7,056-file
import and can still be inspected or reproduced; they were not deleted or
proved to be defective hardware drivers.

**Why:** Make the production recording target auditable, prevent disabled
supplier features from entering the link accidentally, keep the standard-board
guard, and avoid implying that unused health or hardware modules were removed
from the supplier SDK.

**Principal files:**

- [tools/firmware/make_voice_project.py](../../tools/firmware/make_voice_project.py)
- [firmware/voice-profile.json](../../firmware/voice-profile.json)
- [firmware/bc_ros/bc_config/sudo_voice_profile.h](../../firmware/bc_ros/bc_config/sudo_voice_profile.h)
- [firmware/BCL603S2X/app/project/mdk5/sudo_voice.uvprojx](../../firmware/BCL603S2X/app/project/mdk5/sudo_voice.uvprojx)

**Compatibility and supplier validation:** The profile requires
HANDWARE_1_23_2, rejects HANDWARE_1_23_3 and USE_OPUS, and retains the
supplier board/SoftDevice/memory selection. Run the generator and compare its
manifest against the intended supplier target. Confirm that any supplier
change to enabled modules is an explicit profile decision; do not infer
hardware removal from a profile exclusion.

### S01-001 — Make recording local-first

**Version/status:** S01 implemented and included in the immutable RC1; retained
in S02 main and S03 working source. Physical acceptance pending.

**Before → after:** On the standard factory path, connected PDM capture used the
online/live branch while the local Flash recording branches were guarded for
other hardware variants. The standard target did not provide a durable local
copy for the requested connected recording flow. Sudo gives one firmware-owned
recording worker and one LittleFS-backed path for connected and standalone
capture. A complete accepted ADPCM block is committed locally before optional
live delivery; disconnect or app death does not own or terminate the microphone.

**Why:** Bluetooth readiness or phone availability must not decide whether audio
exists. The Ring must retain recoverable audio when preview, app execution, or
radio delivery fails.

**Principal files:**

- [firmware/bc_ros/bc_application/app_sudo_voice.c](../../firmware/bc_ros/bc_application/app_sudo_voice.c)
- [firmware/bc_ros/bc_application/app_sudo_capture.c](../../firmware/bc_ros/bc_application/app_sudo_capture.c)
- [firmware/bc_ros/bc_module/recording/bc_recording.c](../../firmware/bc_ros/bc_module/recording/bc_recording.c)
- [firmware/bc_ros/bc_module/recording/bc_capture.c](../../firmware/bc_ros/bc_module/recording/bc_capture.c)
- [firmware/bc_ros/bc_module/recording/bc_rec_store.c](../../firmware/bc_ros/bc_module/recording/bc_rec_store.c)
- [docs/reference/ring-recording-and-ptt.md](ring-recording-and-ptt.md), factory diagnosis section

**Compatibility and supplier validation:** The raw contract remains 220 ADPCM
bytes to 440 PCM16 samples, interpreted by the app as 8 kHz mono. Host
fault tests and the integrated GNU build exercise the source. A supplier must
measure actual PDM/encoder/Flash timing, beginning/end audio, and connected
double-tap behavior on a standard fitted Ring.

### S01-002 — Own PTT and memo gestures, including the stuck-stop escape

**Version/status:** S01 hold/release implemented; S02 double-tap/hold-to-stop
controls implemented on main; S03 inherits them. Sensor acceptance pending.

**Before → after:** Factory IQS event handling did not provide the candidate's
validated release ownership; the historical path could clear queued capture
audio during stop. Sudo decodes validated IQS7211E contact/release reports,
retains a release received during Start, and has a capture guard independent
of a busy storage worker. Missing/invalid reports expire a 750 ms touch lease.
Double-tap memo recording is opt-in in S02; a hold can stop an enabled memo,
and release cannot immediately start a second clip. PTT remains available when
memo mode is disabled.

**Why:** A release must stop the correct recording without losing an accepted
tail or leaving capture running. Opt-in memo mode reduces accidental activation.

**Principal files:**

- [firmware/bc_ros/bc_module/recording/bc_voice_gesture.c](../../firmware/bc_ros/bc_module/recording/bc_voice_gesture.c)
- [firmware/bc_ros/bc_module/recording/bc_touch_report.c](../../firmware/bc_ros/bc_module/recording/bc_touch_report.c)
- [firmware/bc_ros/bc_application/app_sudo_capture.c](../../firmware/bc_ros/bc_application/app_sudo_capture.c)
- [firmware/bc_ros/bc_device/touch_button/IQS7211E/IQS7211E.c](../../firmware/bc_ros/bc_device/touch_button/IQS7211E/IQS7211E.c)
- [firmware/bc_ros/bc_module/recording/bc_touch_tuning.c](../../firmware/bc_ros/bc_module/recording/bc_touch_tuning.c)
- [docs/reference/ring-recording-and-ptt.md](ring-recording-and-ptt.md), configuration and physical-matrix sections

**Compatibility and supplier validation:** The candidate uses the standard IQS7211E
path and does not select the one_sec board. Host gesture checks include early
Stop, duplicate reports, stale touch, and memo escape. A supplier must verify
the fitted sensor's release/no-contact behavior, thresholds, accidental-contact
rate, and release-to-microphone-stop time. Passing host tests does not identify
the cause of a prior physical stuck-stop report.

### S01-003 — Bound capture tail, storage, and restart recovery

**Version/status:** S01/S02 implemented in source and host fault harnesses; no
physical power-cut qualification.

**Before → after:** Factory stop behavior could clear queued capture work and
some recording writes did not propagate errors. The candidate tags capture
buffers with recording identity and sequence, drains accepted complete frames
before finalization, and reports empty/partial/error states explicitly. LittleFS
metadata records identity, verified prefix, byte/frame counts, CRC, completion,
recovery, and phone-receipt state. Checkpoints occur at 1,000 ms or 4,096 raw
bytes; stop drain is bounded at 500 ms. Fresh mount validates recoverable data
and never restarts the microphone automatically.

**Why:** Power loss, storage failure, or a late stop must not become a fabricated
complete file or overwrite unrelated unsynced audio.

**Principal files:**

- [firmware/bc_ros/bc_module/recording/bc_recording.c](../../firmware/bc_ros/bc_module/recording/bc_recording.c)
- [firmware/bc_ros/bc_module/recording/bc_rec_store.c](../../firmware/bc_ros/bc_module/recording/bc_rec_store.c)
- [firmware/bc_ros/bc_module/recording/bc_capture.c](../../firmware/bc_ros/bc_module/recording/bc_capture.c)
- [firmware/bc_ros/bc_module/file/LittleFS/lfs_port.c](../../firmware/bc_ros/bc_module/file/LittleFS/lfs_port.c)
- [docs/reference/ring-firmware-candidate.md](ring-firmware-candidate.md), recording/recovery sections

**Compatibility and supplier validation:** The host LittleFS cold-cut harness
covers 147 simulated interruptions across recording, receipt, and deletion.
This is modeled fault evidence. A supplier must cut power at open/write/
checkpoint/finalize/receipt/delete boundaries on a recoverable spare and record
the actual tail-loss envelope, Flash geometry, mount behavior, and preservation
of prior recordings.

### S01-004 — Separate readiness, live preview, and native wire integrity

**Version/status:** S01 protocol implemented and published in RC1; S02 retained;
S03 extends it with phone outcome kind 16. S03 wire changes require the matching S03 SDK.

**Before → after:** The factory online path had no Sudo request/response
contract for request identity, recording identity, readiness lease, or exact
fragment integrity. Sudo native wire v1 uses little-endian request IDs,
recording IDs, explicit kinds, message IDs, fragment offsets, and CRC. READY
is a bounded lease; stream ACK is distinct from a durable phone receipt. Live
preview uses a connection epoch and stream token, and a missing/stalled preview
cannot delete or replace the local recording.

S03 adds BC_VOICE_PHONE_OUTCOME kind 16 and capability bit 8 for a bounded
keyboard-insertion outcome. Its ordinary result response remains the normal
five-byte request/result response. The S03 implementation accepts only
outcome 1 (keyboard inserted) with matching recording ID, live token, epoch,
READY admission, complete saved state, and a ten-second terminal window.

**Why:** Suppliers and app developers need a deterministic, versioned
compatibility boundary. Preview success must never be confused with local
recording custody or arbitrary HID text injection.

**Principal files:**

- [firmware/bc_ros/bc_module/recording/bc_voice_protocol.h](../../firmware/bc_ros/bc_module/recording/bc_voice_protocol.h)
- [firmware/bc_ros/bc_module/recording/bc_voice_wire.c](../../firmware/bc_ros/bc_module/recording/bc_voice_wire.c)
- [firmware/bc_ros/bc_module/recording/bc_voice_wire.h](../../firmware/bc_ros/bc_module/recording/bc_voice_wire.h)
- [firmware/bc_ros/bc_module/recording/bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c)
- [firmware/bc_ros/bc_module/recording/bc_voice_service.h](../../firmware/bc_ros/bc_module/recording/bc_voice_service.h)
- [docs/reference/ring-voice-protocol.md](ring-voice-protocol.md)

**Compatibility and supplier validation:** Malformed fragment, CRC, stale
epoch, wrong-token, duplicate, and capability-gating checks are source/host
checks. The supplier must validate notification forwarding, MTU/fragment
behavior, READY renewal, and reconnect on a physical Ring/phone pair. No RF
speed, PHY gain, or throughput figure is claimed here.

### S01-005 — Resume transfer only from verified data and require custody

**Version/status:** S01/S02 implemented in firmware and matching app paths;
S03 app proof reuse is implemented in the merged SDK commit. Deletion and radio
performance remain device/app acceptance items.

**Before → after:** The factory file path used legacy upload framing and did not
provide the candidate's persistent identity/CRC/custody contract. Sudo catalog
and resume bind recording ID, file size, CRC, transfer token, and aligned
offset. The reader verifies the committed prefix before sending. Client ACKs
mean bytes durably retained by the client; a separate exact whole-file receipt
is required before Ring deletion. Failed phone storage, reconnect, partial
files, and old factory-root recordings retain the Ring copy.

**Why:** Queue acceptance is not phone receipt. A resumed suffix cannot prove a
whole file and must not authorize deletion.

**Principal files:**

- [firmware/bc_ros/bc_module/recording/bc_rec_store.c](../../firmware/bc_ros/bc_module/recording/bc_rec_store.c)
- [firmware/bc_ros/bc_module/recording/bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c)
- [firmware/bc_ros/bc_module/ble/src/bc_file_transfer.c](../../firmware/bc_ros/bc_module/ble/src/bc_file_transfer.c)
- [firmware/bc_ros/bc_module/ble/src/bc_ble_tx.c](../../firmware/bc_ros/bc_module/ble/src/bc_ble_tx.c)
- [docs/reference/ring-s03-reliability.md](ring-s03-reliability.md), transfer section
- Matching app commit [492af2a](https://github.com/ShopItalic/app/commit/492af2ad40949de3d54419df5e2fa140c912b94f)

**Compatibility and supplier validation:** The app computes full raw SHA-256
alongside CRC from offset zero and persists proof only for the exact connection,
recording, size, and CRC. The S03 optimization can reuse unchanged proof and
avoid one full read in that exact case; it is not a measured speed multiplier.
Supplier validation must cover offset-zero, interrupted/resumed, delayed ACK,
reconnect, full phone disk, WAV playback, and deletion ordering.

### S01-006 — Keep BLE writes bounded and epoch-scoped

**Version/status:** S01/S02 implemented on main; S03 worker hardening is
tracked separately below as S03 candidate work.

**Before → after:** The legacy BLE writer retried in a blocking/spinning path and
could allow stale callbacks, resource errors, or a queued terminal packet to
cross a changed connection. Sudo uses one BLE writer, connection epochs,
stack-completion wakeups, bounded retry/deadline behavior, and failure
disconnects. The S140 notification window requests six entries and falls back
to one if allocation fails; this is an allocation policy, not a throughput
claim. PHY remains automatic.

**Why:** Bound abandoned work, preserve framing and cumulative ACK evidence
during retry, and prevent a transfer burst from starving recording/touch work.

**Principal files:**

- [firmware/bc_ros/bc_module/ble/src/bc_ble.c](../../firmware/bc_ros/bc_module/ble/src/bc_ble.c)
- [firmware/bc_ros/bc_module/ble/src/bc_ble_tx.c](../../firmware/bc_ros/bc_module/ble/src/bc_ble_tx.c)
- [firmware/bc_ros/bc_module/ble/inc/bc_ble_tx.h](../../firmware/bc_ros/bc_module/ble/inc/bc_ble_tx.h)
- [firmware/bc_ros/bc_module/recording/bc_voice_protocol.h](../../firmware/bc_ros/bc_module/recording/bc_voice_protocol.h)
- [firmware/bc_ros/bc_module/recording/bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c)
- [firmware/bc_ros/bc_module/recording/bc_voice_service.h](../../firmware/bc_ros/bc_module/recording/bc_voice_service.h)

**Compatibility and supplier validation:** Existing packet sizes, S140 service
identity, explicit message boundaries, CRC, and six-entry window semantics
remain the compatibility contract. Host checks can prove bounded state
transitions; they do not prove RF speed, PHY selection, packet goodput, weak
signal behavior, or ten-minute physical stability.

### S01-007 — Make feedback ownership explicit

**Version/status:** S01/S02 implemented in source and RC1/S02 checks; physical
LED and motor acceptance pending. S03 extends motor activity publication.

**Before → after:** Factory LED and haptic events could be interleaved with
recording state and legacy connection events. Sudo gives the recording worker
authoritative green recording indication, finite start/stop/error pulses, and
persisted LED/haptic policy. Disabled feedback drops pending normal requests
instead of replaying them after re-enable. S03 schedules two bounded 80 ms
outcome pulses only after the protocol accepts a valid phone outcome; a new
capture, link change, or mute clears pending confirmation.

**Why:** A supplier should be able to distinguish saved-on-Ring feedback from
phone text confirmation, and feedback must not block capture or replay stale
state.

**Principal files:**

- [firmware/bc_ros/bc_module/led/bc_ic_led.c](../../firmware/bc_ros/bc_module/led/bc_ic_led.c)
- [firmware/bc_ros/bc_module/led/bc_ic_led.h](../../firmware/bc_ros/bc_module/led/bc_ic_led.h)
- [firmware/bc_ros/bc_module/motor/bc_linear_motor.c](../../firmware/bc_ros/bc_module/motor/bc_linear_motor.c)
- [firmware/bc_ros/bc_module/motor/bc_linear_motor.h](../../firmware/bc_ros/bc_module/motor/bc_linear_motor.h)
- [firmware/bc_ros/bc_application/app_sudo_voice.c](../../firmware/bc_ros/bc_application/app_sudo_voice.c)

**Compatibility and supplier validation:** The documented default active times
are 120 ms start, 280 ms successful stop, and 400 ms error, within the retained
safe PWM range. These are software active times, not measured mechanical
vibration. Supplier testing must measure color, perceptibility, current,
mute behavior, and interaction with touch and battery sampling.

### S01-008 — Persist touch, gesture, LED, and haptic settings truthfully

**Version/status:** S01/S02 implemented; S03 inherits the settings contract.

**Before → after:** Factory settings and sensor writes did not provide the
candidate's checked persistence/readback distinction. Sudo stores versioned,
checksummed settings and tuning values in LittleFS. It reports pending,
applied only after an IQS no-contact write/readback window, or I/O error.
Default touch set/clear is 54/52; supported set is 32–80 and clear is 30 through
set minus 2. Fresh post-RC1 settings disable double-tap; existing persisted
choices are retained. Optional phone/media/presentation actions stay disabled.

**Why:** A successful settings response must mean durable desired state, not an
unverified sensor write, and a supplier must be able to reproduce the default
gesture policy.

**Principal files:**

- [firmware/bc_ros/bc_module/recording/bc_touch_tuning.c](../../firmware/bc_ros/bc_module/recording/bc_touch_tuning.c)
- [firmware/bc_ros/bc_module/recording/bc_touch_tuning.h](../../firmware/bc_ros/bc_module/recording/bc_touch_tuning.h)
- [firmware/bc_ros/bc_module/recording/bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c)
- [firmware/bc_ros/bc_application/app_sudo_voice.c](../../firmware/bc_ros/bc_application/app_sudo_voice.c)
- [firmware/bc_ros/bc_config/sudo_voice_profile.h](../../firmware/bc_ros/bc_config/sudo_voice_profile.h)

**Compatibility and supplier validation:** Boundary values, power-cycle
persistence, sensor reset/re-apply, and disabled feedback need physical
readback on the fitted IQS revision. Do not substitute source defaults for
measurements of touch force or accidental gestures.

### S01-009 — Make battery and power errors observable

**Version/status:** S01/S02 implemented with host fault coverage; S03 motor/ADC
changes have focused host coverage; physical validation remains pending.

**Before → after:** The candidate's battery path now treats failed ADC
transactions as UNKNOWN, prevents reentrant divider ownership, and resets
trimmed filtering across charging phases. S03 adds motor activity publication
from the legacy pre-LDO/PWM path, rejects battery acquisition while motor
activity is active or during a **250 ms** post-motor settling interval, and
invalidates a batch if activity changes during the transaction. Standard-board
battery SAADC acquisition changes from **10 us to 40 us**; other board/channel
settings remain unchanged. Valid filter samples can recover upward or downward
instead of being permanently clamped in one direction.

**Why:** Motor supply/PWM transients and stale filter history can make battery
status misleading. The change bounds when a sample is trusted without changing
the supplier voltage curve, cutoff, charge voltage, or PMIC protections.

**Principal files:**

- [firmware/bc_ros/bc_module/pmic/bc_battery_filter.c](../../firmware/bc_ros/bc_module/pmic/bc_battery_filter.c)
- [firmware/bc_ros/bc_module/pmic/bc_battery_filter.h](../../firmware/bc_ros/bc_module/pmic/bc_battery_filter.h)
- [firmware/bc_ros/bc_module/pmic/bc_power.c](../../firmware/bc_ros/bc_module/pmic/bc_power.c)
- [firmware/bc_ros/bc_driver/bsp/src/bsp_adc.c](../../firmware/bc_ros/bc_driver/bsp/src/bsp_adc.c)
- [firmware/bc_ros/bc_application/app_linear_motor_handler.c](../../firmware/bc_ros/bc_application/app_linear_motor_handler.c)
- [firmware/bc_ros/bc_module/motor/bc_linear_motor.c](../../firmware/bc_ros/bc_module/motor/bc_linear_motor.c)

**Compatibility and supplier validation:** Host battery/motor fault tests cover
ordering and invalidation. A supplier must measure voltage and reported
percentage before/during/after motor pulses, across charge levels, charging
phases, temperature, and actual board components. No battery-life or accuracy
improvement is claimed without that measurement.

### S01-010 — Use a reproducible GNU build and explicit runtime/ABI boundary

**Version/status:** S01 RC1 and S02 main have integrated GNU build evidence;
S03 source changes must be rebuilt by the final CI run.

**Before → after:** The factory target selects Arm Compiler 5.06 update 7
(build 960) and includes the supplier's broad project. Sudo preserves the
factory Keil project and separately provides a pinned Arm GNU 15.2.rel1 build
using the real supplier SDK headers, Nordic startup, FreeRTOS port, and a
linker script with application/heap/stack assertions. The build checks all
225 linked sources, undefined symbols, startup/vector bounds, section fit,
archive hash, and runtime lock backend.

The GNU port supplies checked _sbrk heap bounds and recursive newlib locks
through FreeRTOS task suspension, while leaving interrupts enabled. ISR use of
those locks is a contract failure. The output uses four-byte wchar_t; opaque
supplier objects retain two-byte wchar attributes, so the boundary code avoids
wide-character interchange. GNU stdout retains the failing libnosys behavior;
that is recorded rather than presented as working logging.

**Why:** Suppliers need a repeatable compiler path and a visible compatibility
argument around opaque ArmCC objects, memory reservations, and newlib
concurrency.

**Principal files:**

- [tools/firmware/build_gnu.py](../../tools/firmware/build_gnu.py)
- [tools/firmware/summarize_gnu_build.py](../../tools/firmware/summarize_gnu_build.py)
- [firmware/gnu/sudo_voice.ld](../../firmware/gnu/sudo_voice.ld)
- [firmware/gnu/sbrk.c](../../firmware/gnu/sbrk.c)
- [firmware/gnu/newlib_locks.c](../../firmware/gnu/newlib_locks.c)
- [firmware/BCL603S2X/app/user/inc/FreeRTOSConfig.h](../../firmware/BCL603S2X/app/user/inc/FreeRTOSConfig.h)
- [firmware/README.md](../../firmware/README.md)

**Compatibility and supplier validation:** RC1 CI compiled/linked 225/225 with
zero undefined symbols using the pinned Linux toolchain; S02 documentation
also records local host and memory checks. Static section fit is not heap/stack
high-water proof. The supplier must rebuild with its compiler, review map/ABI
warnings, compare the opaque algorithm boundary, and test runtime memory,
logging, timing, power, and recovery on hardware.

### S03-001 — Add bounded keyboard-insertion outcome feedback

**Version/status:** S03 candidate source; firmware implementation and matching
SDK protocol work exist, with physical confirmation still pending.

**Before → after:** S02 provided recording saved/stop feedback and a separate
app keyboard insertion path without a firmware outcome packet. S03 adds native
kind 16 with a 17-byte request body:
request ID u32, recording ID u64, live token u32, outcome u8, accepting only
outcome 1. The response is the ordinary five-byte request/result response.
The capability is advertised only when the outcome callback is installed.

Firmware requires the same connected epoch, READY admission, live token,
recording identity, complete saved state, and first terminal transition within
ten seconds. Wrong IDs/tokens, reconnects, new capture, partial/recovered
files, expiry, app death, rejected keyboard admission, and duplicates cannot
produce another cue. The application callback only queues state; the worker
schedules two 80 ms pulses without waiting for pulse duration or blocking
capture. Haptic disable cancels or suppresses the cue.

**Why:** Suppliers need an unambiguous distinction between “saved on Ring” and
“the keyboard acknowledged inserting this exact dictation,” with bounded stale
event protection.

**Principal files:**

- [firmware/bc_ros/bc_module/recording/bc_voice_protocol.h](../../firmware/bc_ros/bc_module/recording/bc_voice_protocol.h)
- [firmware/bc_ros/bc_module/recording/bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c)
- [firmware/bc_ros/bc_module/recording/bc_voice_service.h](../../firmware/bc_ros/bc_module/recording/bc_voice_service.h)
- [firmware/bc_ros/bc_application/app_sudo_voice.c](../../firmware/bc_ros/bc_application/app_sudo_voice.c)
- Matching app commit [492af2a](https://github.com/ShopItalic/app/commit/492af2ad40949de3d54419df5e2fa140c912b94f)

**Compatibility and supplier validation:** Protocol/gating tests and the
matching Swift wire/keyboard-ack changes are source-level evidence. The app
commit is merged through app PR #12 and covers transport/protocol/keyboard acknowledgement,
not UI polish or CI changes. Physical testing must cover successful insertion,
field/edit changes, expiry, force-quit, disconnect, new capture, duplicate
outcome, muted haptics, and local-audio retention.

### S03-002 — Reuse exact verified SDK download proof

**Version/status:** S03 app working commit; firmware custody contract retained.
Implemented in SDK commit `492af2a`, merged through app PR #12.

**Before → after:** The app's safe path performed the full offset-zero raw
verification whenever custody required it. S03 retains the full SHA-256 plus
CRC proof and binds it to account/Ring identity, connection, recording ID,
size, and CRC. An unchanged complete catalog can reuse that proof for the
matching durable receipt and avoid one redundant full read. Reconnects,
partial/resumed suffixes, changed metadata, failed checkpoints, and
cancellation fall back to the conservative full-read path.

**Why:** Reduce redundant work while keeping exact raw-file custody and delete
ordering.

**Principal files:** The implementation is in app commit
[492af2a](https://github.com/ShopItalic/app/commit/492af2ad40949de3d54419df5e2fa140c912b94f), principally:

- apps/ios/Sudo/Services/BCLRingFileTransport.swift
- apps/ios/Sudo/Services/RingSyncPipeline.swift
- apps/ios/Sudo/Services/RingVoiceRecordingTransport.swift
- apps/ios/Sudo/Services/RingVoiceProtocol.swift
- apps/ios/SudoTests/RingVoiceRecordingTransportTests.swift

Firmware custody remains in [bc_rec_store.c](../../firmware/bc_ros/bc_module/recording/bc_rec_store.c)
and [bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c).

**Compatibility and supplier validation:** This is a proof-reuse optimization,
not an RF-speed claim and not permission to delete on an ACK alone. Validate
the full-read proof, receipt hash/size equality, reconnect invalidation,
partial-file fallback, phone-storage failure, and deletion only after exact
durable receipt.

### S03-003 — Expire stalled archive work independently of retry timing

**Version/status:** S03 candidate source; implemented with focused BLE worker
coverage, included in S03 RC1 with passing firmware CI; physical validation remains pending.

**Before → after:** Archive verification or transfer could remain active while
the retry clock continued to schedule attempts. S03 adds a separate
30-second no-progress archive lease. It starts on verification/transfer work
and renews only when the verified reader offset or acknowledged transfer
offset advances; the 1.5-second retransmission clock remains separate.

**Why:** A blocked first read, response-slot starvation, or abandoned transfer
must eventually release archive ownership without treating retry wakeups as
progress.

**Principal files:**

- [firmware/bc_ros/bc_module/recording/bc_voice_protocol.h](../../firmware/bc_ros/bc_module/recording/bc_voice_protocol.h)
- [firmware/bc_ros/bc_module/recording/bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c)
- [firmware/bc_ros/bc_module/recording/bc_voice_service.h](../../firmware/bc_ros/bc_module/recording/bc_voice_service.h)

**Compatibility and supplier validation:** The focused worker suite reports
1,454 passing checks, including the 30-second no-progress expiry. This is
modeled execution; supplier testing must exercise blocked reads, response
starvation, cancellation, reconnect, and actual Flash/BLE timing.

After expiry, explicit cancellation or successful completion, an identical
RESUME using the retired token returns `CANCELLED` and cannot reopen the reader.
Active same-token retries remain idempotent. A higher token starts a new attempt;
a wrong recording/offset remains `INVALID`. This matches the SDK's fresh-token
allocation for each read operation and prevents old retry traffic from repeatedly
reopening an abandoned archive.

### S03-004 — Preserve sent ACK boundaries across transfer rewind

**Version/status:** S03 candidate source; implemented with focused BLE worker
coverage, included in S03 RC1 with passing firmware CI; physical validation remains pending.

**Before → after:** A retry rewind reset the outstanding sent-boundary history.
S03 keeps up to six sent message end offsets while rewinding the next cursor to
the phone's acknowledged offset. A delayed cumulative ACK can therefore prove
an already sent boundary without mixing fragments or falsely regressing the
cursor; only a valid advancing ACK renews progress.

**Why:** Preserve the meaning of cumulative ACKs through retransmission while
keeping the six-message bounded window and exact durable receipt requirement.

**Principal files:**

- [firmware/bc_ros/bc_module/recording/bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c)
- [firmware/bc_ros/bc_module/recording/bc_voice_service.h](../../firmware/bc_ros/bc_module/recording/bc_voice_service.h)
- [firmware/bc_ros/bc_module/recording/bc_voice_protocol.h](../../firmware/bc_ros/bc_module/recording/bc_voice_protocol.h)

**Compatibility and supplier validation:** The focused worker suite reports
1,454 passing checks, including six outstanding ACK boundaries surviving a
rewind. Validate delayed cumulative ACKs, duplicate ACKs, reconnect/epoch
changes, and receipt/delete ordering with a supplier harness.

### S03-005 — Bound each transfer poll to one message and four fragments

**Version/status:** S03 candidate source; implemented with focused BLE worker
coverage, included in S03 RC1 with passing firmware CI; physical validation remains pending.

**Before → after:** One poll sent one fragment and the surrounding service had
no explicit per-poll burst bound. S03 permits at most four fragments from the
same message per poll, stops on backpressure or message completion, and keeps
one archive read per worker step. It never starts a second message in the same
burst.

**Why:** Improve bounded scheduling and preserve recording/touch service time
without changing message framing, archive ownership, or claiming a radio-speed
increase.

**Principal files:**

- [firmware/bc_ros/bc_module/recording/bc_voice_protocol.h](../../firmware/bc_ros/bc_module/recording/bc_voice_protocol.h)
- [firmware/bc_ros/bc_module/recording/bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c)
- [firmware/bc_ros/bc_module/recording/bc_voice_service.h](../../firmware/bc_ros/bc_module/recording/bc_voice_service.h)
- [firmware/bc_ros/bc_module/recording/bc_voice_wire.c](../../firmware/bc_ros/bc_module/recording/bc_voice_wire.c)

**Compatibility and supplier validation:** The focused worker suite reports
1,454 passing checks, including 29 ATT-20 packets in 8 polls and one
ATT-244 poll for the bounded burst. These are harness scheduling results, not
RF goodput or PHY measurements. Measure physical packet timing, weak signal,
and ten-minute mixed recording/transfer use separately.

### S03-006 — Record the codec decision and proposed Opus evaluation

**Version/status:** Decision recorded after S03 RC1 publication. Opus evaluation
is proposed; no codec implementation or released audio format changed.

**Before → after:** S03 retains the supplier-compatible 8 kHz mono IMA ADPCM
contract (220 encoded bytes per 440 decoded samples, approximately 32 kbps).
The implementation prioritized recording and Bluetooth reliability while
preserving SDK/file compatibility. That decision was not based on measured
CPU limits or a comparison proving ADPCM the best codec for this Ring.
The earlier product requirement identified 16 kHz mono Opus. It should have
been evaluated explicitly alongside the performance work.

The supplier source includes an optional Opus encoder using mono input,
16 kbps CBR and complexity 0, but its input-rate define is 8 kHz and USE_OPUS
is commented out. Included source is not evidence that the standard board has
been qualified with Opus. The S03 profile excludes that encoder and Opus
sources. Both codecs are not running simultaneously.

**Why / proposed target:** Evaluate 16 kHz mono Opus at an initial 16 kbps against
S03. At the target bitrate, a ten-second audio payload would be approximately
20 KB instead of 40 KB before framing/container overhead. This arithmetic does
not establish equal quality, half the transfer time, improved transcription,
or lower battery consumption. Higher-rate capture must be verified; upsampling
an existing 8 kHz recording cannot restore lost detail.

**Principal evidence:**

- [app_opus.c](../../firmware/bc_ros/bc_application/app_opus.c), optional encoder initialization and controls
- [app_opus.h](../../firmware/bc_ros/bc_application/app_opus.h), actual rate/bitrate and disabled USE_OPUS switch
- [app_sudo_capture.c](../../firmware/bc_ros/bc_application/app_sudo_capture.c), current decimation and ADPCM path
- [FreeRTOSConfig.h](../../firmware/BCL603S2X/app/user/inc/FreeRTOSConfig.h), runtime statistics disabled
- [sudo_voice_profile.h](../../firmware/bc_ros/bc_config/sudo_voice_profile.h), current codec/board guard
- [Hardware audio boundaries](sudo-ring-hardware.md#storage-and-audio-boundaries), earlier requirement versus current source evidence

**Compatibility and acceptance:** A new candidate must update codec metadata,
framing, recoverable storage, resumed decoding and the matching SDK together,
while preserving old ADPCM recordings and immutable S03 RC1 assets. Measure
worst-case frame encoding time during BLE/Flash activity, dropped samples,
PTT release latency, stack/heap high-water use, transfer bytes/time, transcription
quality and total recording-plus-sync energy on the identified board. CPU load
is currently unmeasured; reserved RAM is not CPU utilization. Promote Opus only
when the complete audio path meets the reliability and resource requirements.

## Features intentionally retained or outside this candidate

The Sudo target retains the standard board pin map, S140/peer manager, BLE HID
service identity/bonding, IMU/motion and temperature/power support needed for
supplier validation, FreeRTOS, LittleFS, vendor ADPCM, DFU/recovery scaffolding,
and the preserved factory source tree. The candidate does not add ASR, Opus,
Wi-Fi, modem, display, arbitrary UTF-8-to-HID, Mac forwarding, or a signed DFU
package. Disabled phone/media/presentation/swipe actions are profile policy;
they are not proof that the corresponding supplier source files are absent.

The S01 RC1 artifact is immutable and remains an unsigned application-only
engineering prerelease. S02 source is on main without a separate release.
The S03 package must identify its source and matching SDK commits, successful
Linux CI artifact, independently checked application-only HEX/BIN, provenance,
checksums and supplier documents. These are engineering release evidence;
physical acceptance and supplier signing remain separate gates.

## Supplier acceptance checklist

For each future candidate, record the exact board/version readback, source and
app commits, image hashes, active profile, starting file list, configuration,
and test logs. On a recoverable standard 603V1.23.2 Ring:

1. Verify connected and standalone PTT, optional double-tap, hold-to-stop,
   release timing, tail audio, empty/partial outcomes, and no unintended
   media/presentation/swipe behavior.
2. Exercise app death, BLE loss, reconnect, READY renewal, stale epochs/tokens,
   malformed fragments, delayed cumulative ACKs, retry expiry, and four-fragment
   message bursts. Record bytes and elapsed time; do not infer RF speed from
   source constants.
3. Verify catalog/resume from offset zero and interrupted offsets, full raw
   SHA/CRC, WAV playback, phone receipt, full-disk failure, and deletion
   ordering.
4. Power-cut recording, checkpoint, finalization, receipt, and deletion; check
   recovered prefixes and preservation of previous identities.
5. Measure LED/motor timing, perceptibility/current, battery voltage and
   percentage around motor activity, charging phases, temperature, transfer,
   and recording. Verify 40 us SAADC and the 250 ms motor-settling guard on the
   actual board.
6. Rebuild with the supplier compiler and review the GNU map, wchar ABI
   warnings, newlib runtime locks, heap/stack high water, startup/vector
   placement, archive hash, signed update path, and interrupted-update
   recovery.

## S03 validation amendment — September 6, 2026

The final local gate passed **14,330 C checks** under ASan/UBSan and **six
archive-normalizer tests**. Service coverage is 1,454 checks; the integrated
worker contributes 463 checks, including archive expiry followed by Flash idle
shutdown and hold/release within two worker passes when injected during a
packet burst. Source bytes, CRC and custody survive cancellation and capture.
The final local GNU build links 225/225 sources with zero undefined symbols;
startup, memory bounds and runtime-lock checks pass. The image spans 311,452
bytes; static RAM is 203,968 bytes, plus reserved 8 KiB heap and 8 KiB MSP.
Supplier wchar ABI/libnosys warnings remain disclosed.

The matching SDK changes are merged through app PR #12, source `492af2a`.
All Ring simulator unit tests pass; the complete unit run is 597/598 because
one Apple-model test needs an unavailable runtime. Native preview/controller/
epoch fault harnesses and cloud gates pass. Firmware release provenance records
the actual Linux CI artifact and final source. No hardware/RF-speed, battery
calibration, runtime-stack or signed-update qualification is implied.

## S03 publication amendment — September 6, 2026

S03 firmware is committed as `da712a2877c98278a647c0beb66befd7d98d9b09`,
merged to `e1b196292934fbc62fa3b5effbfa4e5265b310d1` through
[firmware PR #2](https://github.com/ShopItalic/sudo/pull/2), and published as
[Pre-release v6.0.3.3S03-rc.1](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S03-rc.1).
[Main CI run 34023848753](https://github.com/ShopItalic/sudo/actions/runs/34023848753)
passed host tests and the GNU build. The released Linux BIN is 311,304 bytes,
SHA-256 `5592f039f3725e1f7a160cc206e84d3830e3ac3b7c942a621e42d58ad72b19f4`;
its load range is `[0x27000, 0x73008)`. These released-image values are distinct
from the local macOS build reported above. Download-back verification passed
for all 12 checksum-listed assets and all 22 checksum-listed bundle files.

Matching SDK source `492af2ad40949de3d54419df5e2fa140c912b94f` is merged as
`664ea438c7265d57885f926e589acb0faa1d05ca` through app PR #12. Its local Ring
tests passed as recorded above; separate remote
[iOS CI run 34023194481](https://github.com/ShopItalic/app/actions/runs/34023194481)
ended cancelled and is not a passing validation result. No physical Ring was
flashed. Later decision notes in this log do not alter the tagged RC or its ZIP.

## Final S03 software audit — September 6, 2026

Status: **validated in software; physical qualification pending**. Source HEAD
`f217db959aa2f42da70cb6be17410f1976d0f7c2` has identical firmware/tools/tests to
the published S03 source `e1b196292934fbc62fa3b5effbfa4e5265b310d1`.
The fresh run on MBA passed 14,330 C checks in 24 ASan/UBSan suites and six
archive-normalizer tests; all 7,056 factory source blobs match the import.
The fresh GNU build compiles/links 225/225 sources with zero undefined symbols
and reproduces the local BIN hash
`f86ed4863af35c4e5cee7249f75838bfe137b82a26f5c72093a478ddfdbb5d69`.
[Main CI 34042683808](https://github.com/ShopItalic/sudo/actions/runs/34042683808)
is now verified successful for both host and GNU jobs.

The [full feature inventory and audit](ring-s03-final-audit.html) records the
reviewed paths, residual warnings, opaque supplier-codec test boundary and
physical gates. No new recording/storage/transfer defect was identified in the reviewed paths.
The final warning review found nine inherited task-start checks comparing the
integer result with `NULL`; FreeRTOS allocation failure is `-1`, so failure can
be logged as success. The new Sudo worker correctly checks `pdPASS`. This remains
a recorded follow-up, not a correction included in the released RC. Ten
pointer/integer compiler warnings, two wchar ABI warnings and eight libnosys
warnings remain. This is not a proof of every vendor SDK line or a physical Ring test.

Documentation-only corrections identify S03 as current in `docs/firmware.md`,
`ring-firmware-candidate.md` and `ring-recording-and-ptt.md`, distinguish older
S01 RC1 defaults, and replace stale current build/filter statements. Existing
unrelated edits to the root README, docs index, factory firmware reference and
hardware reference are preserved. The report and backlog also record proposed
simplifications and long-lived catalog/tombstone measurements. No proposed
firmware deletion or Opus change was implemented; release assets are unchanged.

## README component overview — September 7, 2026

Added a component table to the repository README covering the ring PCBA, SiP,
MCU/radio, microphone, touch controller, IMU, recording flash, haptic motor,
size-specific battery choices, power/clock functions and unspecified support
parts. The table follows the existing hardware reference and BOM, distinguishes
SiP-internal functions from separate components, and retains unresolved fitted
part and battery-rating qualifications. No firmware or release asset changes.

## S04 controls and cleanup — September 7, 2026

**Request:** remove the push-to-talk duration cap; provide exactly three
mappable physical inputs—press-and-hold, double tap (off by default), triple
tap—with an adjustable hold activation threshold; expose persisted light/haptic
controls and apply the concrete cleanup findings. This is new **6.0.3.3S04 source
work**. Published S03 RC1 assets and checksums remain unchanged. No Ring was flashed.

### S04-001 — PTT runs until release

- Fresh PTT settings and gesture configuration use zero. Valid older SVS1
  settings load with PTT forced to zero while retaining memo and feedback
  preferences; a successful subsequent save persists zero with readback.
- S04 SETTINGS_SET and PTT START reject nonzero limits before persistence or
  capture. Memo/app recording retains its independent optional limit. Touch
  activation delay is a separate setting: it delays the start, not the end.
- Capture/touch faults, storage exhaustion and power loss remain real end
  conditions. Normal PTT ends on release, without an artificial duration cap.

### S04-002 — Three independently mapped inputs and hold activation delay

- The only physical action inputs are hold, double tap and triple tap. Each
  maps to disabled, memo toggle or an SDK/app event; hold additionally maps to
  PTT until release. A tap cannot map to PTT because it has no ongoing hold.
- Fresh defaults are **hold 1 second → PTT**, **double tap → disabled**,
  **triple tap → memo toggle**. Existing SVS1 memo opt-out is preserved as a
  disabled triple tap on upgrade. All mappings persist in a separate checked
  SVI1 LittleFS attribute. Feedback saves preserve the mappings.
- Hold activation accepts **500–10,000 ms**. The sensor's 16-bit millisecond
  hold register **0x4F** is written and read back in the existing valid
  no-contact communication window. Its units and gesture behavior are defined
  by the [Azoteq IQS7211E datasheet, sections 8 and 12](https://www.azoteq.com/images/stories/pdf/iqs7211e_datasheet.pdf).
  This is a configured threshold, not a measured physical activation time.
- The sensor mask contains only enabled hold/double/triple bits (**0x08,
  0x02, 0x04**). Initial setup enables hold only; verified defaults apply 0x0C.
  Single tap, palm and swipes remain disabled; legacy single/swipe callbacks
  cannot produce a second action when the Sudo input consumer is installed.
- Queued reports carry the sensor configuration generation and apply status.
  Old or unverified gesture flags cannot start an action under a new mapping;
  release/fault reports still reach the active recording owner.
- Mapped SDK/app events are live, bounded and connection-scoped. Hold produces
  activated then released/cancelled; taps produce activated only. Events expire
  after one second of unsent/backpressured time and never replay on reconnect.
  They do not claim offline delivery or exactly-once execution in another app.
- Files: new `bc_voice_inputs.h`; `bc_voice_gesture.[ch]`,
  `bc_touch_report.[ch]`, `bc_touch_tuning.[ch]`, `bc_voice_protocol.h`,
  `bc_voice_service.[ch]`, `app_sudo_voice.c`, `sudo_voice_profile.h`,
  `IQS7211E.c` and `IQS7211E_init_1232.h`. Electrode maps and channel cycles
  remain unchanged.

### S04-003 — Firmware control API and compatibility

- Exact S04 identity plus HELLO bits **9: triple tap**, **10: PTT until release**,
  **11: input mappings** identify the new firmware contract. INPUTS_SET/GET add
  acknowledged mappings and threshold readback with pending/applied/error sensor
  status. INPUT_EVENT is a separate typed event stream for host integrations.
- SETTINGS keeps its prior payload layout; memo-enabled becomes a compatibility
  mirror of whether any input maps to memo toggle. Hosts must edit mappings
  through INPUTS_SET and preserve this mirror in feedback saves. Changes require
  no active capture or held contact. Failed writes do not publish unconfirmed
  drafts as current settings.
- Firmware provides persisted light/haptic enable switches through SETTINGS and
  strength/start/stop duration through TUNING. S04 inherits S03 PHONE_OUTCOME and
  S02 master feedback semantics. No unsupported light brightness/color control
  or physical motor calibration is implied.
- Scope is **firmware only**, per the user's final clarification. Experimental
  app edits were stopped and parked locally, with the app checkout restored and
  no app commit, PR or publication. Matching app UI/SDK adoption is outside this
  firmware change; the protocol document specifies the integration contract.

### S04-004 — Remove proven dead state and duplicate recording paths

- Removed the unused `bc_voice_service.live` message and never-true
  `live_pending` flag; the existing prefix FIFO and transmit message own live
  delivery. Removed ten unused online/health/file stubs after checking every
  selected ARM object for references.
- Excluded the supplier PDM/history dispatch functions and switch cases from
  Sudo. The Sudo worker already unconditionally intercepts both command families.
  Preserved these functions for other vendor profiles. Removed the unused
  Sudo health-file initialization and IPC health-list no-op/delay.
- Kept the legacy archive reader and any compatibility entry point with a
  remaining caller. Identity, time, battery, bonding, motion, update and
  non-recording command behavior remain outside this pruning.

### S04-005 — Truthful task-start failures

- Nine inherited `xTaskCreate` result checks now require `bc_pdPASS`; FreeRTOS
  allocation failure is **-1**, so comparison against NULL could log success.
  In Sudo, failure enters the Nordic fatal-error handler with `NRF_ERROR_NO_MEM`
  instead of continuing after loss of a required worker. The configured Nordic
  handler resets in release and halts for a debugger in DEBUG builds.
- Sites: BLE, RTC, hardware check, IMU, linear motor, touch, hardline TSDB,
  IC LED and watchdog task creation. Other vendor profiles retain their existing
  failure policy with the corrected success test.
- Corrected the retained gamepad helper's disconnected-handle check to compare
  against `BLE_CONN_HANDLE_INVALID` and check the pointer before dereference.
  Connection handle zero is valid.

### S04 validation and remaining work

- Full sanitizer-backed host run passed **16,894 checks across 24 C suites**
  and **six archive-normalizer tests**. The final gesture-only rerun passed
  **2,751 checks** (eight added checks), bringing current suite coverage to
  **16,902 checks**. It also verifies an SDK tap cannot discard the release of
  an active SDK hold and that an expired hold emits cancellation.
- Arm GNU **15.2.Rel1** compiled/linked **225/225 sources**, with **zero undefined
  symbols**. Startup/vector, newlib locking and memory-layout checks passed.
  The gesture object was built after the final gesture source edit.
- Local BIN: **312,436 bytes**, SHA-256
  `27b6f33f33fd8cf1bbd831508f779997aafe0b96164bc4f4e61f57a32cc5dbe5`.
  Static RAM: **203,800 bytes**, plus separate **8 KiB C heap** and **8 KiB MSP**;
  the remaining link-layout gap is **23,696 bytes**, not measured free RTOS heap.
- The link still reports **one supplier wchar ABI warning** and **eight newlib
  syscall-stub warnings**. These remain qualifications, not target acceptance.
- Factory provenance verification passed: all **7,056** baseline source files
  match commit `102bfd2`; the private supplier archive remains outside Git.
- Evidence is retained locally under `.local/s04-validation/` (host log,
  final gesture log, GNU report/summary/artifacts and baseline log). The binary
  is an unsigned local application image; published release assets are unchanged.
- Remote source commit `36a645e63aec511e95ddec65515c35aaad761ace` passed both
  [push CI](https://github.com/ShopItalic/sudo/actions/runs/34047940409) and
  [PR CI](https://github.com/ShopItalic/sudo/actions/runs/34047970004), including
  the complete **16,902-check** host run and GNU target build. Source review is
  [firmware PR #3](https://github.com/ShopItalic/sudo/pull/3).
- App compilation/testing is not part of firmware validation or acceptance.

Tests cover held
PTT beyond the former limits, all three mappings, disabled inputs, sensor
hold/mask readback, custom hold release/cancellation, stale/expired events,
migration from an old persisted cap, rejected nonzero PTT requests,
muting/readback, and negative FreeRTOS task-allocation injection in the LED
worker. Physical IQS behavior, microphone/BLE timing, vibration, light output,
battery consumption, power interruption and supplier signing/recovery remain
qualification gates in `docs/backlog.md`.

### S04 documentation synchronization

The root README and documentation index now distinguish S04 source review from
published S03 RC1, remove the stale ten-second PTT default, summarize the three
mappings and record current validation. The protocol, requirements, candidate
build record and backlog describe S04. Versioned release guides and historical
audit records retain their original version-specific evidence. Existing
uncommitted branding and hardware-reference edits remain separate.

## S04 RC1 publication

S04 was merged through PR #3 as `338f7f69223a4cdcebf545faf3ac9af1ba4bdd45`.
S04 RC1 (`v6.0.3.3S04-rc.1`) is the primary unsigned engineering RC; the root
README and docs index point to its dedicated guide and release. Package
`provenance.json` pins the exact main source and successful CI artifact used
for the BIN/ELF/map; the HEX is reconstructed and checked against that BIN.
S01/S03 tags and assets remain unchanged. No matching S04 client is included.

The repository is public at the user's explicit request. Supplier headers and
license notices remain; visibility does not relicense third-party components.
A targeted scan of 4,818 reachable Git blobs found no complete private-key PEM
blocks or matches for the tested GitHub/AWS/OpenAI/Slack token formats. Two
private-key-header matches are parser/writer string constants in mbedTLS.
This records the scan's scope, not an exhaustive security or legal audit.

## S04 post-RC1 review fixes

### S04-006 — Fail explicitly when the voice worker cannot start

Status: implemented on `codex/s04-review-fixes`, after RC1; not in the
published RC1 assets. The command queue, touch queue and 2048-word voice task
previously could fail allocation and leave a running application with no
recording owner. Both allocation paths now clean up their queues and invoke
`APP_ERROR_HANDLER(NRF_ERROR_NO_MEM)`, matching the other required workers.
The command dispatch boolean still means ownership, not successful execution
or a protocol ACK; rejected requests must not reach the vendor recording owner.

Source: `app_sudo_voice.c`. The worker harness injects failure of each queue
and task allocation (`-1`), checks fatal handling/cleanup and then runs the
normal worker after a successful retry. Reset/recovery needs target testing.

### S04-007 — Unlock configuration after a failed touch lease

Status: implemented on the same branch, after RC1. Missing or invalid sensor
reports previously ended PTT as partial but retained contact/hold flags, so
SETTINGS, INPUTS and TUNING stayed BUSY indefinitely. The gesture owner now
clears those busy flags on a fault; active recording/drain still blocks writes.
A separate release gate suppresses stale hold/tap actions until a valid release
arrives, including after remapping. An already released contact does not gain
an artificial release requirement when its report ages out. Memo capture
retains its separate lifecycle.

Source: `bc_voice_gesture.c/.h`. Gesture tests cover timeout and invalid-report
recovery, stale holds, configuration and release rearming. Service tests send
all three real configuration commands before timeout, during drain and after
drain. Physical IQS silence/reset and touch tuning recovery remain to qualify.

### S04-008 — Separate GAP security from GATT attribute recovery

Status: implemented on the same branch, after RC1. Restored the missing break
in `bc_ble.c` after `BLE_GAP_EVT_SEC_PARAMS_REQUEST`. A security request must
not fall through and clear GATT system attributes. Non-HID mode still rejects
pairing; HID type 1 still delegates to the initialized Nordic Peer Manager
(`bc_ble_init`, `peer_manager_init`, SDK `security_dispatcher.c` and
`security_manager.c`). The review's assertion that HID pairing has no responder
was incomplete; adding a second reply here would conflict with that owner.

`test_ble_security.py` compiles the exact adjacent production switch cases with
SoftDevice call spies. It checks HID delegation, non-HID rejection, explicit
SYS_ATTR_MISSING handling and the existing rejected-connection guard. This is
host event-routing coverage, not a physical pairing harness. Test unbonded
pairing, bonded reconnect and notification subscriptions on a spare Ring.

### Post-RC1 local verification

- 16,970 checks across the existing 24 C suites, plus six archive-normalizer
  tests and both new BLE event-routing builds, passed with ASan/UBSan.
- New allocation, gesture and BLE regressions all fail against the original
  RC1 source and pass with these fixes.
- Arm GNU 15.2.rel1: 225 objects compiled, zero undefined symbols; startup,
  vector and memory summary checks passed. Local BIN: 312,500 bytes,
  SHA-256 `f63be8fb91a29455325562e40da2f48b3935795faf8e94e0e748d362c02f13eb`.
- No wire format, default mapping, supplier license or published release asset
  changed. No physical flash, radio pairing, ArmCC reproduction or signing was
  performed. Existing GNU supplier-archive wchar/syscall warnings remain.

## How to append future changes

Append a new monotonically named entry such as S03-007 or S04-001; do not
rewrite an earlier entry's historical before/after. Set its version and status
to one of implemented, implemented-pending-validation, planned, published,
or rejected. Include:

- the exact source commit or explicit dirty working tree status;
- the behavior before and after, including wire sizes, timeouts, bounds, and
  defaults where relevant;
- why the change exists and which supplier failure it addresses;
- principal repository-relative source/profile/test links;
- compatibility impact and concrete supplier/device checks;
- any artifact/app/CI identity needed to reproduce it.

When a later release lands, add a new entry or a short status amendment that
points to the immutable tag and checksums. Never call an S03 planned item
released until the release owner verifies the source commit, matching app adapter, green CI,
image/HEX/address checks, and package provenance.
