# Sudo Voice firmware candidate

The **6.0.3.3S02** engineering candidate implements local-first recording,
hold/release push-to-talk, configurable double-tap memos, checked storage and
resumable BLE for the production **603V1.23.2** Ring. The corresponding app
adapter lives in [ShopItalic/app](https://github.com/ShopItalic/app).

Production source and host fault harnesses share the same recording, capture,
gesture, storage and protocol logic. The integrated GNU target compiles and links
all 225 sources with no undefined symbols. RC1 is published as an unsigned engineering prerelease; S02 has not been
packaged as a release. No candidate has been flashed or signed here. Physical
radio speed, audio fidelity, touch behavior, battery use and power-loss recovery
remain unmeasured. The [requirements matrix](ring-recording-and-ptt.md) separates
implemented behavior from device acceptance; the [protocol](ring-voice-protocol.md)
defines exact packets and error meanings.

## Post-RC1 controls follow-up

S02 makes double-tap recording opt-in and expands the persisted light/haptic
switches to all normal application feedback. The app retains S01 support and
labels its older switches as recording-only. The published RC1 tag/assets are
unchanged; historical build measurements below describe the S01 development
work unless explicitly identified as S02. Use the RC1 release checksums for its
exact downloadable artifacts. New settings preserve an existing saved choice, and changes
require idle recording state. Physical feedback and gesture acceptance remain
pending.

### S02 local validation (September 6)

- Full host fault suite passes, including 321 gesture checks and 416 LED checks.
  The new memo escape preserves accepted tail audio and requires release before
  restarting. Motor tests cover disable during an active pulse and mute/unmute
  during peripheral setup.
- All 7,056 preserved factory files still match the import manifest.
- Arm GNU 15.2.rel1 compiles/links 225/225 sources with zero undefined symbols;
  startup/vector, memory bounds and runtime lock audits pass.
- Local application BIN: 310,220 bytes; SHA-256
  `d2ec411792811551f0ef91664acf9a386d36ab2ab4f2690137142e3746488a29`.
  Static RAM is 203,880 bytes; reserved C heap and main stack are 8 KiB each.
  The two vendor wchar ABI warnings and libnosys limitations remain open for
  supplier review. Runtime memory use still needs physical measurement.
- Matching app: 272 selected Ring simulator tests pass; cloud test, lint,
  typecheck and build pass. These are local tests, not device qualification.

## Preserved original

The original supplier archive `1.23.2_6033固件SDK.zip` was copied before editing:

- Archive size: **69,437,016 bytes**.
- SHA-256: `ea3db9a85e153a49f77e716cd60c4127463484792235e91311b281f05a111d15`.
- Local copy: `.local/factory-sdk/1.23.2_6033固件SDK.zip`, excluded from Git.
- Reviewed source baseline: Git commit **`102bfd2`** on this branch.
- [Source manifest](../../firmware/source-manifest.json): 7,056 imported files,
  209,717,697 original bytes, and 238 exclusions. Encodings, line endings,
  notices, and vendor library inputs were preserved. Signing material, generated
  images, caches, and unrelated media were excluded.
- The [factory BIN/HEX/OTA evidence](ring-firmware.md) remains unchanged.

The manifest describes the immutable import at `102bfd2`, not the modified
working tree. `python3 tools/firmware/verify_baseline.py` checks those Git blobs
against the archive hashes. The unredacted local archive includes signing
material and belongs outside Git. The candidate neither uses nor exports it.
The source baseline is a factory distribution; it is not a backup of the
physical ring's installed firmware, settings, bonds, calibration, or recordings.

## Recording and reliability changes

| Area | Candidate behavior |
| --- | --- |
| Recording ownership | One firmware worker owns the microphone lifecycle, mounted LittleFS instance, recording identity and final state. Connected and standalone recordings use the same storage path; reconnect does not stop capture. |
| Hold/release | IQS reports include validated contact/release. A separate capture guard enforces release/stale-touch/limit handling independently of a busy storage worker. Stop received during Start is retained. |
| Memo mode | Double tap is off by default and can be enabled in the app to toggle a recording without holding. PTT defaults to ten seconds; memo/app recording defaults to unlimited. Limits, memo enable, LED and haptic enable persist. |
| Capture tail | Eight bounded PDM buffers feed the encoder with recording IDs and sequence checks. Stop halts production and drains accepted complete blocks before finalizing. Overflow, missing sequence and stop timeout produce explicit partial/error outcomes. |
| Storage | Checked opens, appends, checkpoints, close and metadata readback. No implicit format on mount failure, no unsynced reclamation, no invented accepted-byte count. Full storage rejects recording while retaining prior files. |
| Restart recovery | LittleFS atomic attributes record identity, verified prefix, CRC, completion and receipt state. Fresh mount validates recoverable data; a reboot never restarts the microphone. Cold-cut tests cover recording, receipt and deletion. |
| Final results | Versioned idempotent Start/Stop/query expose file identity, name, actual bytes/frames/CRC and complete/partial/delivered state. An accepted command and a finalized file are different outcomes. |
| Live preview | Phone Ready lease and bounded live ACK window are independent from Flash recording. Stalled/missing live data affects preview diagnostics, not the only audio copy. |
| File custody | Resume binds the exact recording identity, size, CRC, token and aligned offset. A persisted exact receipt and separately acknowledged deletion replace delete-on-send. |
| Feedback | Recording owns green LED output. Finite bounded motor pulses replace unbounded legacy loops. Start, stop, error and optional enable/settings are consistent across trigger sources. |
| Touch tuning | Persisted desired thresholds and hold/double-tap mask apply only in a valid no-contact sensor communication window. I2C write/readback yields explicit pending/applied/error status. |
| Power failures | SFUD/LittleFS errors propagate. ADC failures report UNKNOWN; filtering uses actual initialized values and charging-phase-aware single-flight measurement. Hardware voltage table/cutoff policy is retained. |

Important defaults: checkpoint **1,000 ms or 4,096 raw bytes**, stop-drain timeout
**500 ms**, stale-touch lease **750 ms**. These are software scheduling bounds;
physical Flash/interrupt latency and the requested one-second release behavior
still require measurement. The raw audio contract is **220 ADPCM bytes → 440
PCM16 samples**, interpreted by the app as **8 kHz mono**. Source enum names and
compiler success do not substitute for a physical audio-rate check.

The candidate retains the supplier board pin map, S140/peer manager, radio
services, DFU and existing ADPCM algorithm archive. It does not contain ASR,
Opus, keyboard text injection, display, Wi-Fi or modem code. Shared motion,
temperature, battery and charging support remains. Accidental phone/media,
mouse, presentation and swipe actions are disabled. Manufacturing/reset/legacy
configuration commands are denied where they could race recording; the native
settings API gives checked persistence and truthful readback.

## Bluetooth transfer and matching app

The original transfer repair remains: a single radio writer owns all queued
notifications; resource exhaustion waits on completion/wakeup rather than a
spin loop. Packets carry an internal connection epoch. Stale epochs, fatal send
errors and a ten-second stalled connection cannot masquerade as a successful
stream. The sender requests six S140 notification entries within the original
RAM allocation, falling back to one if the stack cannot allocate six. This is
not a measured throughput multiplier.

Negotiated payload is MTU minus three. Native commands/events have explicit
fragment boundaries, message ID and CRC, with local connection-epoch checks,
so the new protocol also works
at small MTU. Legacy upload retains its established four-byte header, 17-byte
metadata and 220-byte chunks (241-byte full packet); it waits for a sufficient
MTU. Checked read-only open, exact resume seek, short-read handling, EOF bounds,
cancellation and FIFO admission avoid lost chunks and false completion. The
command queue no longer sleeps a second after every packet. The malformed
internal Start packet lengths and bounded parser regression remain covered by
99 actual producer/dispatcher checks.

The supplier S140 [GATTS header](../../firmware/BCL603S2X/app/components/softdevice/s140/headers/ble_gatts.h)
and [BLE header](../../firmware/BCL603S2X/app/components/softdevice/s140/headers/ble.h)
are the authority for notification resource errors, completion and RAM-base
negotiation. Notification acceptance is neither phone receipt nor playback.

The matching iOS path requires the standard board, the exact ten-byte version
**6.0.3.3S01**, and negotiated capabilities. It preserves the factory
**6.0.3.3Z62** workaround on factory firmware. `RingVoiceConnection` observes
the BCL SDK's public peripheral delegate without replacing its CoreBluetooth
owner. Physical forwarding of those notifications remains a device gate.

`RingVoiceRecordingTransport` joins native catalog/resume with the existing
sync pipeline. Complete offset-zero raw CRC proof, playable WAV, saved app
identity and WAV SHA verification precede custody/delete. A resumed suffix
cannot independently prove a whole file. Bounded buffering, epoch checks,
immutable request IDs and scoped cancellation prevent a late old callback from
marking or deleting a newer transfer. Factory root recordings can still be
read but are not deleted by the native candidate until a custody migration
contract exists.

Optional live preview claims the shared decoder once per attempted
recording, accepts the READY-confirmed stream token, checks contiguous sequence,
and sends separate flow ACKs. The Ring retains up to 32 locally accepted raw
frames while waiting for initial READY, preserving the prefix across token
confirmation, then drains through its four-message live ACK window. A missing
prefix, overflow or expired/cancelled preview fails explicitly until the next
recording; it cannot reset the ADPCM predictor into the middle of a clip.
Terminal state can precede tail LIVE packets;
a bounded tail drain handles that ordering. Gaps or ASR deadline/failure fall
back to stored audio. Only a complete nonpartial transcript reaches the app's
existing dictation store; preview never claims durable Ring custody or creates
a second canonical memo. Memo/app preview requires the foreground. PTT can use
a finite UIKit background assertion, capped by the app at 25 seconds, to feed
the existing same-iPhone keyboard. That keyboard admits a fresh complete final
only into the same visible document with no later user edit. Denial, expiration
or app death ends preview and relies on local Ring capture plus later sync.
The cap is not an iOS execution guarantee; Mac/HID forwarding is not implemented.

## Build profile and toolchain

[`sudo_voice.uvprojx`](../../firmware/BCL603S2X/app/project/mdk5/sudo_voice.uvprojx)
selects **Sudo Voice 1.23.2**. Its deterministic generator preserves the original
board/SoftDevice/memory settings and original Arm Compiler **5.06 update 7,
build 960** selection. It currently selects **227 project entries** and excludes
**238** unrelated/replaced entries; those are project-entry counts, not linked
object counts or flash savings. The original project and immutable import
remain available for supplier baseline reproduction.

The independent GNU driver uses official **Arm GNU 15.2.rel1** (GCC 15.2.1,
Binutils 2.45.1), real vendor SDK headers, Nordic GCC startup and FreeRTOS port.
It clears BSS and calls the actual `main`; it does not link the generic
ARMv4T `crt0.o`. The linker script asserts app/SoftDevice, heap and stack bounds.
GNU-only runtime ports are selected by this driver, not inserted into the Keil
project. Generated unsigned `.elf`, `.map` and `.bin` stay in local/CI artifacts.

The preserved `bc_algorithm.lib` is hash-locked. A build-only copy normalizes
ArmCC archive symbol/relocation metadata needed by GNU Binutils; it does not
change the factory file or algorithm instructions. Six normalizer regression
tests cover this conversion. The GNU application and Newlib use four-byte
`wchar_t`; the opaque supplier objects retain two-byte wchar attributes. Their
reviewed ADPCM boundary uses bytes/shorts/integers rather than wide characters.
The build records remaining ABI warnings and audits the exact archive hash;
this is a bounded compatibility argument, not a claim to have audited opaque
algorithm internals. A supplier compiler/image comparison is still required.

The GNU `_sbrk` port enforces the linker-reserved heap bounds with checked
arithmetic and a short interrupt-masked cursor update. The GNU profile enables
FreeRTOS Newlib reentrancy and supplies allocation-free recursive retarget
locks. The locks suspend task scheduling while leaving interrupts enabled;
checked nesting prevents an unmatched release from consuming another caller's
scheduler suspension. Calling those locks from an interrupt is a contract
failure that resets the device. The two identified ISR stdout call sites are
disabled for Sudo. GNU UART stdout is currently unavailable: `_write` retains
the error-returning `libnosys` implementation. The original Keil profile is
unchanged by these GNU runtime selections.
Static section fit alone cannot establish FreeRTOS heap high-water marks,
worst-case stack or physical timing. No warning is silently equated with a
successful hardware qualification.

## Validation and reproduction

From the firmware repository root:

```sh
python3 tools/firmware/verify_baseline.py
python3 tools/firmware/make_voice_project.py
sh tools/firmware/test.sh
python3 tools/firmware/build_gnu.py --toolchain /path/to/arm-gnu-toolchain/bin
python3 tools/firmware/summarize_gnu_build.py --build-dir build/firmware/gnu/sudo_voice
```

The host runner executes actual C production modules and actual worker/driver
translation units under AddressSanitizer/UndefinedBehaviorSanitizer. Hardware
and RTOS shims replace only their external boundaries. Coverage includes
recording ownership, stale/session-tagged capture, hold/release, malformed
wire fragments, idempotency, LED arbitration, motor failure, touch I2C readback,
battery/ADC faults, Flash failures, transfer/custody and real LittleFS cold
mounts. The cold-cut suite includes **147 simulated interruptions** across
recording (109), receipt (33) and deletion (5), with no graceful unmount and
checks that previous recording identities survive.

The September 6 integrated host run passed **13,174 C checks across 24 suites**
plus **6 archive-normalizer tests**. These are assertions and fault cases, not
physical measurements. Baseline verification matched all **7,056** original
files. The integrated Arm GNU 15.2.rel1 build compiled and linked **225/225
sources**, with **zero undefined symbols** and passing startup/vector checks.

| Memory allocation | Integrated local build |
| --- | --- |
| App Flash load image, including initial `.data` | 309,364 / 757,760 bytes (40.83%) |
| Static RAM, including the 140 KiB FreeRTOS heap array | 203,856 / 243,880 bytes |
| Separate C-library heap reservation | 8,192 bytes, `0x200363A8..0x200383A8` |
| Separate main/interrupt stack reservation | 8,192 bytes, `0x2003E000..0x20040000` |
| Static RAM plus both reservations | 220,240 / 243,880 bytes (90.31%) |
| Unassigned space between heap and main stack | 23,640 bytes |

The linker prints 302,048 bytes of FLASH before `.data` load bytes; that value
alone understates the image footprint. GNU Newlib's target `_reent` is 512
bytes per task: 13 startup tasks add 6,656 bytes from the existing FreeRTOS heap,
falling to 6,144 after the temporary hardware-check task exits. Task stacks and
other dynamic allocations also consume that heap. These static checks do not
prove runtime heap or stack margin. The final map resolves the lock backend
from `224_newlib_locks.o` and contains no single-threaded `libc_a-lock.o`.
Two supplier wchar-attribute warnings and eight unsupported Newlib syscall
warnings remain recorded; GNU stdout through `_write` fails as described above.

Local MBA/Darwin artifacts (unsigned, not a DFU package):

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `sudo_voice.bin` | 309,364 | `c9cf7c24ccc4171ede41726a00d74c6a1f1b701ef44b7819dd606f2d8d9308aa` |
| `sudo_voice.elf` | 1,236,140 | `871144a5cb06e360a9d65333d96277c486843e62691f2c5eb1175441e6fd074b` |
| `sudo_voice.map` | 2,273,709 | `409e30f907d64d224d5d129207a7a94d51ab7c9c2d79fb908e516e8b2203c9d0` |

CI uses the pinned Linux toolchain and records its own image hashes; this is
not a claim that host-dependent paths/debug data produce identical artifacts.
The older [validation.json](../../firmware/validation.json)
and [recording-command-validation.json](../../firmware/recording-command-validation.json)
are historical, source-hash-pinned checks, not claims about this new tree.
The build emits `build-report.json`; the concise `build-summary.json` checks
startup/vector bounds, actual ELF words, unresolved symbols, sections and ABI
warnings. CI runs both baseline/profile/host checks and a full pinned Linux GNU
build, with an unsigned artifact retained for review. Green CI on the final
published head is required before this source handoff is considered complete.
The Linux host job skips the one normalizer probe that needs an installed Arm
toolchain; its five portable normalizer tests run there, and the separate GNU
job performs the real archive normalization, target compilation and link.
The local run exercises all six normalizer tests.

The matching [app candidate](https://github.com/ShopItalic/app/pull/10) at
`064c265356e32ea9819a8eaa57929ebc17f66f0a` passes **267 simulator tests** for
native wire/protocol/client/receiver/file transport and the existing Ring
transport/sync pipeline, background-window policy and keyboard admission.
Actual-source preview, controller and connection harnesses add eleven, seven
and two bounded suites respectively. Cloud workspace tests, lint, typecheck
and build pass as separate gates. A broader simulator run passed 546 of 547
unit tests and both UI tests; the remaining Apple model test failed because
this simulator lacks the safety model/prompt-template asset. That entire suite
is not green, and its assertion was not weakened to hide the missing asset.

## Comparison with Caption

The September 6 comparison used published Caption
[`70ba7b4`](https://github.com/ShopItalic/caption/tree/70ba7b4b1627b24321f40a394a6f58e10760ffa8)
and the newer development source/results in the referenced “Italic device
firmware and emulator” task. That historical comparison included checked source
blobs and task-reported integration results; it is not a new build or a current
status claim about Caption. The published firmware job then failed fetching
its private SDK before compilation.

| Concept | Adaptation in this Ring candidate |
| --- | --- |
| Local acceptance before live delivery | Implemented with a bounded C owner and LittleFS; radio readiness never substitutes for local capture. |
| Stop barrier and session-tagged tails | Implemented using Nordic PDM buffers and the retained ADPCM encoder. |
| Checked recording/recovery/checkpoints | Implemented with Ring-sized buffers and LittleFS atomic attributes/CRC, without copying Caption's IPR2 container. |
| Versioned state, readiness and ACK separation | Implemented in native Ring v1 and the matching app; durable custody is a separate operation. |
| Fault harnesses | Adapted stale events, early Stop, storage failure, power cuts and transfer interruption cases to the actual Ring code. |
| Saved-file catch-up | The Ring now has native catalog/resume/custody; Caption's catch-up was still open in the earlier comparison and was not copied as a finished implementation. |

Ring uses nRF52840/S140 and 256 KiB SoC RAM; Caption uses JieLi AC791N and a
much larger memory/storage platform. Firmware binaries, codec implementations,
GPIO/radio drivers and update packages cannot be shared. Display, reader,
speaker, Wi-Fi/modem and large C++ runtime buffers were not imported. Opus
remains a separate measured/negotiated codec project. Caption-style automatic
rollover is not needed for the current single-file memo contract; full storage
stops visibly and preserves pending recordings.

## Supplier and device acceptance before release

1. Reproduce the factory source with its exact toolchain and reconcile the
   factory distribution, compiler/library boundary and candidate map.
2. Read back a spare standard 603V1.23.2 unit and confirm bootloader, SoftDevice,
   partition/Flash identity, fitted touch/motor/power parts and recovery access.
3. Run the [physical matrix](ring-recording-and-ptt.md#physical-acceptance-matrix),
   recording image/app hashes, audio, timings, radio parameters, memory and
   current draw. Connected double tap must create and download a complete file.
4. Verify BCL callbacks, real ADPCM rate/framing, background/resume, custody,
   gestures/settings, battery/charge/thermal behavior, pairing and DFU.
5. Supplier assigns the release counter, signs the package, and demonstrates
   interrupted-update recovery and factory restore before updating a user ring.

The factory extraction is not a read-back of the user's installed ring. This
candidate is reviewable source and an unsigned build, not an authorized OTA
release or a tested recovery image.

### S02 hold-to-stop escape

When an enabled double-tap recording is active, a hold requests Stop through
the same drain-and-finalize path as a second double tap. Repeated hold reports
and release cannot start another clip; a new hold after release can start PTT.
This does not interrupt an app-owned recording. It provides another gesture
when a double tap is missed, but still needs a functioning touch sensor.
Supplier testing must reproduce the reported stuck double-tap behavior on
physical hardware; passing host tests does not establish its original cause.
