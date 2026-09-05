# Sudo Voice firmware candidate

This is a source proposal for the production Bravechip **603V1.23.2** Ring.
It contains an improved BLE sender and recording-file transfer path, plus a
smaller build profile. It has passed host fault-injection tests and ARM object
compilation. **It is not a linked, signed, or device-tested firmware release.**
No ring was read, flashed, erased, or reset during this work. Transfer speed,
battery use, radio behavior, and recovery on hardware remain unmeasured.

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

## Transfer changes

| Before | Candidate behavior |
| --- | --- |
| Tight retry loop when `sd_ble_gatts_hvx` returns `NRF_ERROR_RESOURCES` | Retains the pending packet and blocks on an RTOS semaphore. Notification completion, CCCD changes, and MTU negotiation wake it; a bounded fallback wait handles missed/coalesced events. |
| Concurrent file, command, and audio writers share global transmit scratch memory | Producers use one FIFO and one radio writer. Each queued packet carries an internal connection epoch that is never sent over the air. |
| Buffered packets can outlive a connection | The sender and consumer reject old epochs, including reused connection handles. A fatal send or 10-second stall aborts that connection before later queued packets can appear to complete a damaged stream. |
| Default one-entry notification queue | Requests six entries within the **existing linker RAM allocation**. Falls back to one on `NRF_ERROR_NO_MEM`; it restores the linker RAM base before retrying. The startup log identifies the selected window. Six entries are not a promise of six packets per event or a measured speedup. |
| Reported MTU includes ATT overhead | Uses negotiated MTU minus three bytes; waits for a sufficient MTU without fragmenting the vendor packet format. PHY responses permit automatic 1M/2M selection. |
| One-second sleep after every received command | The receive queue blocks when idle and drains immediately when work arrives. |
| Audio packet and completion-event logs in hot paths | Removes those logs while retaining useful connection/error diagnostics. |
| Resume subtracts the offset but seeks to zero | The shared reader validates the offset and seeks to it; packet numbering is relative to the remaining bytes, as the supplier protocol intended. |
| Upload ignores read results or advances after a failed send | Accumulates short reads, stops on premature EOF/error, and waits for FIFO acceptance before reading another chunk. Failed transfers do not delete files or emit a fabricated completion. |
| Missing upload file can be created; worker returns on error | Opens uploads read-only, closes resources, and returns to its request loop after failure. Counting notifications replace the resume/suspend wakeup race. |
| A new request overwrites a busy transfer's global header | Checks ownership before copying the request. Busy responses use a separate packet. |
| Parser treats a 250-byte buffer as a larger command struct | Copies into a zero-initialized command struct, checks lengths, and validates file-request lengths before dispatch. RX callbacks never block waiting for queue capacity. |

The on-air service identifiers, four-byte command header, 17-byte file-transfer
metadata, 220-byte file chunks, and ADPCM recording format remain compatible
with the supplier path. A full file packet is 241 bytes. The app-facing
interpretation remains 8 kHz mono; validate it with a physical capture.
`sd_ble_gatts_hvx` success means local stack acceptance, not durable phone receipt.
The application must continue validating byte counts and issuing file deletion
separately. The [client at app commit `60c37a3`](https://github.com/ShopItalic/app/blob/60c37a3dcfed32f80e7b615b57d089c1c9a8835b/apps/ios/Sudo/Services/BCLRingFileTransport.swift#L725)
requests individual files with `getFileData(fileName:)` and accepts ADPCM file
types 8/B/D. The candidate accepts those same file types. The client should
continue its existing transfer flow;
Empty files and resume offsets at/past EOF are rejected by closing the transfer
connection, since the legacy protocol has no established zero-packet response.
Fixed resume support is not an instruction to enable a previously disabled app
resume path without end-to-end testing.

The retry contract follows the supplied S140 headers:
[`ble_gatts.h`](../../firmware/BCL603S2X/app/components/softdevice/s140/headers/ble_gatts.h)
and [`ble.h`](../../firmware/BCL603S2X/app/components/softdevice/s140/headers/ble.h).
These define notification resource exhaustion, completion events, configuration,
and the in/out RAM-base argument used by stack enablement.

## Recording-focused build

Open [`sudo_voice.uvprojx`](../../firmware/BCL603S2X/app/project/mdk5/sudo_voice.uvprojx)
and select **Sudo Voice 1.23.2**. Its generator reads the original project and
preserves the supplier's Arm Compiler **5.06 update 7, build 960**, board,
SoftDevice, pin definitions, and memory settings. The original `.uvprojx` is
retained as provenance; use commit `102bfd2` to build the unmodified supplier
source. The patched sources are intended for the generated Sudo target.

The candidate removes 236 project entries: 108 were already disabled by the
supplier; 126 belong to the unused Opus implementation; two are the alternative
encoder wrapper and factory BLE/Wi-Fi traffic generator. Two new transfer
modules leave **217 entries** in the candidate project. This measures build
inputs, not final flash savings—linker dead-code elimination may already have
removed unused sections from the factory image.

The profile also:

- Uses one recording-file worker for ordinary and resumed uploads. Two redundant
  1,024-word worker stacks are no longer allocated: **8,192 bytes of configured
  stack allocation removed**, before TCB costs and new transport overhead.
- Disables the vendor batch-upload worker; the app requests individual files.
- Compiles out phone/media, presentation, mouse, touch-screen, and joystick HID
  actions, including effects from persisted old settings. Keeps the HID service
  and peer manager so service identity and pairing behavior remain available.
- Retains recording, touch capture, motion support, haptics, battery/charging,
  temperature support, flash/filesystem, bonds, and DFU. PPG was already disabled
  in this board's factory target. Health code already excluded by that target is
  absent from the generated project; shared motion/temperature dependencies are
  retained pending supplier validation.
- Uses an identifiable engineering version, `6.0.3.3-SUDO1`, without assigning a
  release/anti-rollback counter. Disables build-time supplier packaging hooks.

See [profile settings](../../firmware/bc_ros/bc_config/sudo_voice_profile.h) and
[removal manifest](../../firmware/voice-profile.json). Regenerate deterministically
with `python3 tools/firmware/make_voice_project.py`.

## Validation and reproduction

From the repository root:

```sh
python3 tools/firmware/verify_baseline.py
sh tools/firmware/test.sh
python3 tools/firmware/prepare_headers.py
python3 tools/firmware/check_arm.py
```

The host tests compile and execute the actual allocation-free retry and file
transfer modules with AddressSanitizer and UndefinedBehaviorSanitizer. **462
checks passed**: resource exhaustion, repeated early wakeups, cancellation,
connection-epoch changes, fatal errors, finite deadlines, tick rollover, packet
bounds/order, exact and partial final chunks, nonzero resume, short reads,
premature EOF, send failure, invalid offsets, and empty remaining data.

ARM checks use `arm-none-eabi-gcc 16.2.0`, Cortex-M4 Thumb, hard-float, the actual
supplier target defines and SDK headers, and the Nordic GCC FreeRTOS port.
`__MODULE__` maps to `__FILE__` for the vendor diagnostic macro. C library headers
come from pinned [Newlib 4.5.0](https://sourceware.org/pub/newlib/); the helper
verifies its archive SHA-256 before extraction. No synthetic SDK types or
hardware-driver stubs replace the headers in these checks.

The ten originally selected baseline translation units compiled successfully
from the untouched Git archive. The candidate's 13 changed/new units compile
successfully, including service and command integration. Remaining compiler
warnings are existing vendor pointer/integer comparisons in task creation and
an unused HID helper. GitHub Actions runs baseline verification, profile reproducibility, and host
fault-injection tests on pushes and pull requests. It does not build or sign
production firmware. Compiler logs and objects are local build output;
[validation evidence](../../firmware/validation.json) records the source hashes
and results. Compiler object success does not establish a complete link, boot,
stack headroom, production ABI compatibility, or physical behavior.

## Supplier acceptance before any release

1. Rebuild the baseline with the exact Arm Compiler 5 toolchain and reconcile it
   with the factory image; resolve any library/toolchain differences.
2. Build the Sudo target and review its map, app/SoftDevice RAM boundaries, flash
   boundaries, RTOS heap, and worst-case stack usage. Confirm whether the six-entry
   window fits, or record the one-entry fallback. Verify no removed-module
   references remain in the linked image.
3. On a verified spare 603V1.23.2 ring, compare identical recording files and
   phone/app versions. Record bytes, SHA-256, elapsed time, effective KiB/s, MTU,
   PHY, connection interval, selected TX window, retries, and battery use. Test
   weak signal, background/foreground, disconnect/reconnect, small MTU, stalled
   CCCD, queue pressure, truncated file, resume offsets, and cancellation.
4. Confirm capture, playback, ordering, delete-after-verification, touch/motion,
   haptics, charging/thermal behavior, pairing, and persisted settings. Exercise
   malformed requests and missing files without damaging stored recordings.
5. Have the supplier assign the release counter, package and sign the candidate,
   and demonstrate both interrupted-update recovery and the approved factory
   restore procedure on a spare unit before updating the user's ring.

No throughput multiplier, battery improvement, flash-size reduction, or tested
recovery guarantee is claimed by this source proposal.
