# Sudo Voice firmware candidate

This is a source proposal for the production Bravechip **603V1.23.2** Ring.
It contains an improved BLE sender and recording-file transfer path, plus a
smaller build profile. It has passed host fault-injection tests and ARM object
compilation. **It is not a linked, signed, or device-tested firmware release.**
No ring was read, flashed, erased, or reset during this work. Transfer speed,
battery use, radio behavior, and recovery on hardware remain unmeasured.

**Push-to-talk and connected recording remain open.** The
[recording requirements review](ring-recording-and-ptt.md) preserves the earlier
supplier briefs and current Chinese request, reconciles conflicting storage
requirements, and defines hold/release, double-tap, LED/haptic, command-result
and recovery acceptance. The BLE changes below do not create the missing Flash
recording. The app also gates its existing workaround to exactly `6.0.3.3Z62`,
so the engineering version needs coordinated app compatibility before use.

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
| Parser treats a 250-byte buffer as a larger command struct | Copies into a zero-initialized command struct, checks lengths, and validates file-request and recording-control payload lengths before dispatch. RX callbacks never block waiting for queue capacity. |
| Internal recording Start helpers declare four bytes for a five-byte packet | Preserves the Start byte. The stricter parser previously decoded these malformed internal Starts as Stops; the September 6 correction fixes both helpers and rejects truncated `0x71/0x05` and `0x71/0xFE` commands. |

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

The September 6 recording-command review adds **99 checks** under the same
sanitizers, for **561 checks total**. It compiles byte-preserved function
extractions of the seven packet producers, the actual bounded parser and PDM
dispatcher using the checked-in packet headers. Queue and hardware effects
are intercepted: this tests command meaning and bounds, not physical recording.
The pre-fix candidate reproduces Start dispatching as Stop. Coverage includes
ordinary/ISR Stop, payload-free internal commands, truncated recording controls,
null/short/oversize input and valid legacy packets. Both changed production
translation units also pass fresh ARM object compilation. The
[recording-command validation](../../firmware/recording-command-validation.json)
records exact source hashes and scope; the original validation record below
remains the September 5 snapshot.

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

## Comparison with Caption

Reviewed on September 6, 2026. Keep the Ring's production board support and
current BLE sender. The most useful Caption work to adapt is its recording
lifecycle, storage integrity checks, session rejoin, and fault-test harness.
These are recommendations for a later source change; this comparison does not
port them or change the Ring's wire format.

### Evidence and scope

- Ring source: this candidate at `700f30748983ace445861b1e223e08776386d75b`.
- Published Caption source: [commit `70ba7b4`](https://github.com/ShopItalic/caption/tree/70ba7b4b1627b24321f40a394a6f58e10760ffa8).
  Twenty-three selected source/doc files were fetched at that ref and their Git
  blob hashes verified. Its shared runtime is older than the current development
  work. The emulator job passes; the [firmware job](https://github.com/ShopItalic/caption/actions/runs/33935778509/job/101223316772)
  fails fetching `ShopItalic/freeink-sdk`, before any compiler step.
- Newer Caption development: actual source diffs, read outputs, documentation
  and completed checks retrieved from [Italic device firmware and emulator](thread://01a06f78-5f22-7d62-bf6e-e1bb10551a2e?hostId=remote-control%3Aenv_e_6a7a3ff7d2308326a8dd16323532bea4).
  This includes wire v2, IPR2 recordings, recovery and checkpoints. That task
  reports 74 Swift tests, 18 web tests, controller fault tests, 14 display
  fixtures, and a successful JieLi integration compile/link on September 5.
  MBP-M5 was unavailable over SSH during this comparison, so those are recorded
  task results, not a fresh build or filesystem verification on that Mac.

| Area | Production Ring candidate | Newer Caption development |
| --- | --- | --- |
| Platform | nRF52840, Nordic S140/FreeRTOS, supplier C code | JieLi AC791N/pi32v2, vendor RTOS/drivers, portable C++ runtime and separate HAL |
| Resources | 256 KB SoC RAM; 16 MiB external recording flash | Board documentation records 8 MB SDRAM and internal SD-NAND storage; much larger buffer budgets |
| Audio contract | Existing ADPCM packets; app interprets 8 kHz mono, pending physical confirmation | JieLi-framed Opus on the device adapter; desktop/WASM fixtures use 16 kHz PCM16 |
| Recording | Offline recording and existing LittleFS synchronization; live backup is conditional on other board defines | Local storage accepts each complete encoded frame before live BLE; explicit drain before finalization |
| Transfer | Single sender, bounded resource retries, connection epochs, checked file reads and corrected resume offsets | Versioned frames, session/sequence checks, live ACK window, same-session rejoin and phone-readiness lease; saved-file catch-up remains unfinished |
| Integrity | Existing raw recording format; recording write-result propagation and low-space policy still need work | Checksummed IPR2 records, completion marker, bounded read-only recovery and checked file reopen/checkpoints |
| Validation | 462 host checks and 13 ARM object compilations; production link pending | Shared core/controller/emulator tests and recorded successful integration ELF; no physical qualification |

The [Nordic specification](https://www.nordicsemi.com/Products/nRF52840) and
[JieLi platform documentation](https://doc.zh-jieli.com/AC79/zh-cn/master/board_description/board_overview/index.html)
confirm different processor architectures and resource envelopes. Firmware
binaries, codec backends, GPIO drivers, radio APIs and update packages cannot be
interchanged. Both modified firmwares still need physical acceptance testing.

### What to adapt first

1. **Store live recordings locally before sending them.**
   The selected Ring target defines `HANDWARE_1_23_1` and `HANDWARE_1_23_2`,
   without `HANDWARE_1_23_2_ONE_SEC`. In
   [`app_pdm_handler.c`](../../firmware/bc_ros/bc_application/app_pdm_handler.c),
   the online branch sends BLE at lines 659-680, while simultaneous flash writes
   at lines 681-691 require `ONE_SEC` or `HANDWARE_1_23_3`. Caption's
   `PocketRuntime::onEncodedAudioFrame` makes storage acceptance a prerequisite
   for streaming. Adapt that policy using a bounded recording worker; a slow
   radio must not prevent local capture. Storage acceptance still needs an
   explicit flush and power-loss contract.
2. **Drain captured audio and propagate recording failures.**
   Ring `app_pdm_close` clears the capture queue before stopping PDM (lines
   413-421). Offline Stop then uses a fixed 20 ms wait before closing the file.
   Caption explicitly waits for the capture-stop callback after its queued
   encoder tail. Adopt the barrier and session-tagged callbacks. In
   [`app_ppg_file_data_handler.c`](../../firmware/bc_ros/bc_application/app_ppg_file_data_handler.c),
   `app_ppg_file_write` ignores `ppg_file_write`'s result and advances its byte
   counter anyway (lines 2251-2259). Propagate write/sync errors, stop visibly,
   retain the partial file, and never finalize an uncertain tail as successful.
3. **Adapt the IPR2 writer/scanner and checkpoint tests.**
   Caption's `JournalWriter` stores header/record checksums, ordered sequences
   and a checked End marker. `ArchiveScanner` returns only verified complete
   audio before the first damaged tail and leaves the original intact.
   `RecordingCheckpoint` checks after the header, periodically after 5 seconds
   or 64 KiB of new data, and before final End. Its close/flush/reopen primitive
   checks file identity, length and append position, then refuses further
   appends after a failure. The Ring already calls `lfs_file_sync` periodically
   (lines 736-750); use LittleFS semantics rather than copying JieLi's file-mode
   and cache-flush implementation. Choose Ring-specific checkpoint intervals.
   Preserve the existing app's raw ADPCM and logical resume-offset contract,
   either through a separate integrity journal or an explicitly versioned
   reader/export path. IPR2's 24-byte record prefix also needs a suitable block
   size for the Ring's smaller flash and audio packets. Checksums and successful
   reopen tests do not prove power-cut recovery or authenticate a recording.
4. **Preserve recordings when storage fills.**
   The Ring's selected `lk_app_ppg_file_open` path can invoke
   `lk_ppg_space_reclamation`; that function can call `app_ppg_file_delete`
   (lines 1793-1818 and 2657-2703). It does not check for a verified phone copy.
   This is separate from the candidate's safe file-transfer completion behavior.
   Caption's preservation policy is a better default: retain originals and
   report full storage, or reclaim only after an explicitly defined durable
   receipt/retention policy. Do not describe the current Ring candidate as
   having no automatic deletion anywhere.
5. **Reuse session rejoin, readiness and measurable counters.**
   Caption's newer wire contract distinguishes radio connection, phone
   processing readiness, flow-control ACK and durable receipt. It expires
   readiness after 10 seconds, supports Hello/SessionSnapshot/SessionResume,
   and recovers a stalled live ACK window after 2 seconds while preserving the
   recording. Adapt the concepts with the iOS adapter and explicit protocol
   negotiation; Sudo's existing internal connection epoch is not an on-air
   recording identity. Keep separate counters for locally accepted audio,
   live drops, retries/rejoins and phone-confirmed bytes. Caption's reported
   queued-payload rate is not measured radio goodput.

The shared desktop/WASM/controller tests are also worth adapting: stale
callbacks, stop-time encoder tails, storage failures, every torn-record boundary,
clock wrap, rejoin and preservation of existing recordings. The Ring's current
portable transport tests provide a base for this work.

### Keep the port small

Do not copy Caption's display, reader, reply-speaker, Wi-Fi or modem layers into
the Ring. The newer Caption controller's 16 event slots of up to 4,096 bytes and
64,000-byte speaker buffer are inappropriate defaults for the Ring's RAM budget.
Its current custom device profile already disables Wi-Fi/modem startup for a
Bluetooth-first recorder; the Ring has no corresponding networking stack to
remove. Keep the working ADPCM contract initially. Using Caption's Opus path
would require a Nordic encoder, CPU/RAM/battery measurements, codec/framing
negotiation and app decoding changes; it is not a drop-in speed improvement.

Reuse can go the other direction too. The Ring's checked file reader, resume
offset validation, cancellation/session checks and fault tests are useful
references for Caption's still-unimplemented saved-file Bluetooth catch-up.
The Ring also has vendor file rollover code; Caption's automatic rollover is
still open. Adapt the rollover idea only after fixing the Ring's write-error
and retention behavior. Neither device's update/recovery package can serve as
the other's recovery mechanism.

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
