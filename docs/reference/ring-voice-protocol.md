# Ring Voice protocol v1 (candidate)

This document records the candidate voice protocol implemented by the firmware and iOS source in this review. It is a source contract for the candidate service on the existing BCL characteristic. It makes no claim of a physical-ring test, measured performance, signed image, or flash/deployment result. The firmware labels this extension candidate-only: firmware/bc_ros/bc_module/recording/bc_voice_wire.h:7-16.

Primary firmware sources are firmware/bc_ros/bc_module/recording/bc_voice_wire.h and .c for framing, bc_voice_protocol.h and bc_voice_service.c for messages and service behavior, bc_recording.h for state/results, and firmware/bc_ros/bc_application/app_sudo_voice.c for the standard worker. The app mirror is apps/ios/Sudo/Services/RingVoiceWire.swift, RingVoiceProtocol.swift, RingVoiceConnection.swift, RingVoiceRecordingTransport.swift, RingVoiceLiveReceiver.swift, RingVoiceLivePreview.swift, and RingProductionBoard.swift.

## 1. Envelope and integrity

All multibyte integers are unsigned little endian. The ATT limit is the complete packet limit, including the 12-byte voice header.

| Offset | Size | Field | Value or meaning |
|---:|---:|---|---|
| 0 | 1 | Existing BCL frame type | 0 |
| 1 | 1 | Message-ID low byte | Must equal byte 6 |
| 2 | 1 | Voice command marker | 0x7e |
| 3 | 1 | Kind | Request, response, or event kind |
| 4 | 1 | Version | 1 |
| 5 | 1 | Direction | 0 request, 1 response, 2 event |
| 6 | 2 | Message ID | u16 little endian |
| 8 | 2 | Fragment offset | u16 little endian, into body including CRC |
| 10 | 2 | Total body length | u16 little endian, payload plus 4-byte CRC |
| 12 onward | variable | Body fragment | Logical payload followed by CRC32 little endian |

Wire constants are header 12 bytes, maximum body 232 bytes, maximum logical payload 228 bytes, maximum packet 250 bytes, and minimum ATT packet limit 20 bytes. Each fragment carries at most ATT-limit minus 12 bytes: firmware/bc_ros/bc_module/recording/bc_voice_wire.h:10-18; firmware/bc_ros/bc_module/recording/bc_voice_wire.c:55-73; apps/ios/Sudo/Services/RingVoiceWire.swift:9-21,263-303.

The body CRC is over the exact prefix [version, kind, direction, message-ID low, message-ID high] followed by the logical payload. It uses the reflected polynomial 0xedb88320 with 0xffffffff initialization and final XOR; the four CRC bytes are appended little endian and removed before the message is exposed. This transport CRC is distinct from the raw-file CRC in recording metadata: firmware/bc_ros/bc_module/recording/bc_voice_wire.c:41-53,83-89,143-156; apps/ios/Sudo/Services/RingVoiceWire.swift:281-345.

Receivers accept only the contiguous frontier. Offset zero replaces an incomplete assembly; a nonzero offset cannot start one. Exact replay of a wholly received range is accepted, but changed overlap, a gap, invalid length, a bad CRC, an epoch change, or an assembly older than 1000 ms cannot produce a message: firmware/bc_ros/bc_module/recording/bc_voice_wire.h:41-50; firmware/bc_ros/bc_module/recording/bc_voice_wire.c:94-156; apps/ios/Sudo/Services/RingVoiceWire.swift:87-93,123-256.

## 2. Logical kinds and payloads

Every request begins with a nonzero request ID u32 at payload offset 0. The lengths below exclude the envelope CRC.

| Kind | Direction | Length | Payload after request ID |
|---:|---|---:|---|
| 1 HELLO | request | 4 | none |
| 2 START | request | 17 | recording ID u64 at 4; trigger u8 at 12; duration limit ms u32 at 13 |
| 3 STOP | request | 12 | recording ID u64 at 4 |
| 4 QUERY | request | 12 | recording ID u64 at 4; zero means current owner |
| 5 READY | request | 5 | enabled u8 at 4, 0 or 1 |
| 6 LIVE_ACK | request | 12 | stream token u32 at 4; next sequence u32 at 8 |
| 7 SETTINGS_SET | request | 15 | PTT limit ms u32 at 4; memo limit ms u32 at 8; memo, LED, haptic enabled u8 at 12, 13, 14 |
| 8 SETTINGS_GET | request | 4 | none |
| 9 CATALOG | request | 12 | after-recording ID u64 at 4; zero is the initial cursor |
| 10 RESUME | request | 20 | recording ID u64 at 4; file offset u32 at 12; transfer token u32 at 16 |
| 11 TRANSFER_ACK | request | 12 | transfer token u32 at 4; next durable byte offset u32 at 8 |
| 12 RECEIPT | request | 21 | recording ID u64 at 4; file bytes u32 at 12; raw CRC u32 at 16; delete u8 at 20 |
| 13 CANCEL | request | 8 | transfer token u32 at 4 |
| 14 TUNING_SET | request | 11 | touch set u8 at 4; touch clear u8 at 5; haptic strength u8 at 6; start-active ms u16 at 7; stop-active ms u16 at 9 |
| 15 TUNING_GET | request | 4 | none |

Trigger values are PTT 1, memo 2, and app 3. START IDs are persistent idempotency keys across reconnects and reboots. Lengths, IDs, tokens, booleans, duration bounds, and tuning bounds are enforced by firmware/bc_ros/bc_module/recording/bc_voice_protocol.h:6-29, firmware/bc_ros/bc_module/recording/bc_voice_service.c:264-484, and apps/ios/Sudo/Services/RingVoiceProtocol.swift:54-71,243-321,445-645.

Event kinds are:

| Kind | Direction | Length | Payload |
|---:|---|---:|---|
| 0x40 STATE | event | 62 plus name length | Snapshot, request ID 0; idle/end sentinel has recording ID 0 |
| 0x41 LIVE | event | exactly 228 | stream token u32 at 0; sequence u32 at 4; exactly 220 raw bytes at 8 |
| 0x42 FILE | event | 9 through 228 | transfer token u32 at 0; absolute file offset u32 at 4; 1 through 220 raw bytes at 8 |

These event shapes are defined in firmware/bc_ros/bc_module/recording/bc_voice_protocol.h:24-29 and apps/ios/Sudo/Services/RingVoiceProtocol.swift:12-27,231-241,391-401,930-968.

## 3. Responses, snapshots, and results

Every response begins with request ID u32 at offset 0 and result u8 at offset 4. A rejected operation can use the five-byte response. Successful extensions are:

| Response | Length | Fields after the common five bytes |
|---|---:|---|
| HELLO | 20 | capabilities u32 at 5; sample rate u16 at 9; samples/block u16 at 11; bytes/block u16 at 13; checkpoint interval ms u16 at 15; release bound ms u16 at 17; transfer window u8 at 19 |
| READY | 17 | live token u32 at 5; current recording ID u64 at 9 |
| SETTINGS_GET or SETTINGS_SET | 16 | PTT limit u32 at 5; memo limit u32 at 9; memo, LED, haptic u8 at 13, 14, 15 |
| TUNING_GET or TUNING_SET | 13 | touch set u8 at 5; touch clear u8 at 6; haptic strength u8 at 7; start-active ms u16 at 8; stop-active ms u16 at 10; apply status u8 at 12 |
| RESUME | 29 | recording ID u64 at 5; transfer token u32 at 13; file bytes u32 at 17; raw CRC u32 at 21; requested offset u32 at 25 |

START, STOP, QUERY, and CATALOG normally return a snapshot response of length 62 plus name length; the Swift decoder also accepts a five-byte error response. Response construction is in firmware/bc_ros/bc_module/recording/bc_voice_protocol.h:48-72, firmware/bc_ros/bc_module/recording/bc_voice_service.c:31-145,497-506, and apps/ios/Sudo/Services/RingVoiceProtocol.swift:323-389,648-929.

Snapshot offsets are:

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | Request ID u32 |
| 4 | 1 | Result u8 |
| 5 | 8 | Recording ID u64 |
| 13 | 1 | Trigger: PTT 1, memo 2, app 3; zero only for idle/catalog sentinel |
| 14 | 1 | Phase |
| 15 | 1 | Recording error/result |
| 16 | 1 | Flags: complete 0x01, recovered 0x02, delivered 0x04 |
| 17 | 4 | Duration limit ms |
| 21 | 4 | Revision |
| 25 | 4 | Accepted bytes |
| 29 | 4 | Accepted frames |
| 33 | 4 | Durable bytes |
| 37 | 4 | File bytes |
| 41 | 4 | File frames |
| 45 | 4 | Raw file CRC |
| 49 | 4 | Live queued frames |
| 53 | 4 | Live dropped frames |
| 57 | 4 | Live stream token |
| 61 | 1 | Name length |
| 62 onward | 0 through 39 | Name bytes, without a NUL |

Phase values are idle 0, starting 1, recording 2, stopping 3, saved 4, partial 5, failed 6, empty 7 and delivered 8.

The complete result enum is: 0 OK; 1 INVALID; 2 BUSY; 3 WRONG_SESSION; 4 DUPLICATE; 5 NO_SPACE; 6 OPEN_ERROR; 7 WRITE_ERROR; 8 SYNC_ERROR; 9 CLOSE_ERROR; 10 CAPTURE_ERROR; 11 CAPTURE_OVERFLOW; 12 SEQUENCE_GAP; 13 STOP_TIMEOUT; 14 ALREADY_EXISTS; 15 EMPTY_AUDIO; 16 INTERRUPTED; 17 TOUCH_ERROR; 18 NOT_FOUND; 19 CUSTODY_REQUIRED; 20 UNSUPPORTED; 21 CANCELLED; 22 CRC_ERROR. The enum is firmware/bc_ros/bc_module/recording/bc_recording.h:19-29 and apps/ios/Sudo/Services/RingVoiceProtocol.swift:73-98.

HELLO capability bits are local storage 0, PTT 1, memo 2, live 3, resume 4, custody 5, settings 6, and tuning 7: firmware/bc_ros/bc_module/recording/bc_voice_protocol.h:31-46; apps/ios/Sudo/Services/RingVoiceProtocol.swift:100-115.

## 4. Handshake, epochs, and ordering

RingVoiceConnection performs HELLO before another request and validates the candidate capability set, 8 kHz sample rate, 440 samples/block, 220 bytes/block, release bound at most 1000 ms, and transfer window 1 through 6: apps/ios/Sudo/Services/RingVoiceConnection.swift:93-301.

READY is a separate stream lease. READY enabled 1 when not ready allocates a new nonzero live token, resets the live ACK/window state, and publishes it. The recording sequence and ADPCM predictor do not restart; a preview that missed sequence one falls back to the stored file. Repeating READY enabled 1 while already ready renews the lease and keeps the same token. READY enabled 0 disables live delivery. The service expires readiness after 10,000 ms without renewal and after a 2,000 ms live stall; the client renews at most every five seconds: firmware/bc_ros/bc_module/recording/bc_voice_protocol.h:31-35; firmware/bc_ros/bc_module/recording/bc_voice_service.c:341-357,534-553; apps/ios/Sudo/Services/RingVoiceLivePreview.swift:431-474.

Epochs are local connection-generation fences, not an extra on-air field in the 12-byte header; the two peers do not need matching epoch numbers. A link/epoch change revokes readiness and transport state, clears incomplete wire assembly, and leaves the local recording intact for recovery. The client cancels pending operations and resets receiver/event state for the new epoch: firmware/bc_ros/bc_module/recording/bc_voice_service.c:164-190; apps/ios/Sudo/Services/RingVoiceWire.swift:123-137; apps/ios/Sudo/Services/RingVoiceClient.swift:131-159.

Service polling performs one bounded archive verification/read step and at most one fragment enqueue. TX order is control responses, pending/urgent STATE, LIVE, then FILE; a control or STATE can therefore precede the tail of an already queued LIVE stream. Client/preview drains are bounded: up to four LIVE blocks can wait before READY, and terminal preview permits a one-second drain before a five-second finish deadline. ASR startup has its own five-second deadline, including waiting for prior ASR cleanup: firmware/bc_ros/bc_module/recording/bc_voice_service.h:82-95; firmware/bc_ros/bc_module/recording/bc_voice_service.c:509-604; apps/ios/Sudo/Services/RingVoiceLivePreview.swift:266-317,500-540,742-830.

The standard 1.23.2 worker declares `{1000U, 4096U, 500U}`. With bc_rec_config order checkpoint_ms, checkpoint_bytes, stop_timeout_ms, this is a 1000 ms checkpoint interval, 4096-byte checkpoint threshold, and 500 ms stop timeout: firmware/bc_ros/bc_application/app_sudo_voice.c:471-478; firmware/bc_ros/bc_module/recording/bc_recording.h:64-68.

## 5. LIVE and stateful decoding

LIVE is accepted only for the current recording and stream token, with sequence beginning at 1 and exactly 220 bytes per block. LIVE_ACK carries cumulative next sequence. The iOS receiver decodes each contiguous block once, accepts an exact retained duplicate without decoding again, rejects a conflicting duplicate or gap, and uses a stateful ADPCM decoder. Explicit start/end resets the decoder; reconnect/rejoin and a lost ACK do not. A suffix without the required sequence-1 prefix falls back to archive sync: firmware/bc_ros/bc_module/recording/bc_voice_service.c:215-231,358-364,534-604; apps/ios/Sudo/Services/RingVoiceLiveReceiver.swift:103-239.

## 6. Catalog, native transfer, and custody

CATALOG returns one entry per request in ascending nonzero ID order. The end response is NOT_FOUND with recording ID 0 and the idle sentinel, not an OK snapshot. iOS skips zero/delivered terminal entries and active writing entries, rejects duplicate names or invalid metadata, and merges legacy entries after removing native-name duplicates: firmware/bc_ros/bc_module/recording/bc_voice_protocol.h:48-58; firmware/bc_ros/bc_module/recording/bc_voice_service.c:411-422; apps/ios/Sudo/Services/RingVoiceRecordingTransport.swift:245-329.

RESUME requires an existing nonempty source. Firmware verifies raw bytes in steps of at most 1024 bytes and sends metadata only after verification. Verification/read failure emits an error and cancels the operation. A token is nonzero and monotonic; equal-token retry is limited to the same recording and original offset: firmware/bc_ros/bc_module/recording/bc_voice_service.c:423-449,497-531.

FILE carries transfer token, absolute offset, and up to 220 raw bytes. The firmware allows six outstanding FILE blocks and retries stalled progress after 1500 ms. The wire service accepts byte offsets within the verified file; the iOS ADPCM transport imposes the stricter 220-byte alignment rule except the exact end, full blocks before the final block, exact contiguous offsets/tokens, and a bounded six-block window: firmware/bc_ros/bc_module/recording/bc_voice_protocol.h:31-35; firmware/bc_ros/bc_module/recording/bc_voice_service.c:534-604; apps/ios/Sudo/Services/RingVoiceRecordingTransport.swift:90-244.

The client calls its checkpoint before TRANSFER_ACK. It independently computes the raw CRC for a read beginning at offset zero and compares metadata. A resumed prefix is immutable; a replay-prefix digest is only a local pipeline check and is not a fabricated whole-file CRC. Failed, incomplete, or cancelled downloads cannot authorize a receipt. A recording marked partial can still be downloaded and verified in full, then receipted as that exact recoverable prefix.

| Custody level | Evidence and meaning |
|---|---|
| TX accepted | Local send callback true means the BLE/ATT path accepted a packet fragment; it is not a remote receipt or file custody. |
| LIVE ACK | Kind 6 cumulative next sequence proves live sequence progress only; it says nothing about native-file durability. |
| Checkpoint ACK | Kind 11 cumulative next byte offset reports a durably retained client prefix, not a complete recording identity. |
| Exact receipt | Kind 12 exact recording ID, byte count, and raw CRC follows complete proof; only this permits firmware delivered state, with delete as a separate requested action. |

RECEIPT is accepted only with exact ID/size/CRC; firmware records the receipt and honors delete only for that identity: firmware/bc_ros/bc_module/recording/bc_voice_protocol.h:68-72; firmware/bc_ros/bc_module/recording/bc_voice_service.c:450-481; firmware/bc_ros/bc_module/recording/bc_recording.h:40-48,131-134; apps/ios/Sudo/Services/RingVoiceRecordingTransport.swift:330-448.

## 7. Settings and touch tuning

Standard defaults are PTT limit 10,000 ms, memo limit 0 for unlimited, and memo, LED, and haptic enabled: firmware/bc_ros/bc_application/app_sudo_voice.c:34-39,72-73,478-487.

Settings persist in a 20-byte A6 attribute: SVS1 bytes 0-3; flags byte 4 with memo bit 0, LED bit 1, haptic bit 2; reserved bytes 5-7; PTT u32 at 8; memo u32 at 12; CRC32 over bytes 0-15 at 16. The firmware reads back the attribute before reporting SET success: firmware/bc_ros/bc_application/app_sudo_voice.c:34-39,274-315.

Tuning defaults are set 54, clear 52, haptic strength 100, start-active 120 ms, and stop-active 280 ms. Ranges are set 32-80, clear 30 through set minus 2, haptic 1-100, and start/stop 20-400 ms divisible by 20: firmware/bc_ros/bc_application/app_sudo_voice.c:72-73; firmware/bc_ros/bc_module/recording/bc_voice_service.c:110-117; apps/ios/Sudo/Services/RingVoiceProtocol.swift:150-172.

Tuning persists in A7: SVT1 bytes 0-3; set 4; clear 5; haptic 6; reserved 7; start u16 at 8; stop u16 at 10; reserved 12-15; CRC32 over bytes 0-15 at 16. Response status is pending 0, applied/verified 1, or I/O error 2. SET confirms durable desired values; the IQS sensor applies them on the next valid no-contact/release report and reads them back. I/O error retains desired values while reporting unapplied state: firmware/bc_ros/bc_application/app_sudo_voice.c:317-350; firmware/bc_ros/bc_module/recording/bc_voice_service.c:127-145; firmware/bc_ros/bc_module/recording/bc_touch_tuning.h:18-34,65-74; firmware/bc_ros/bc_module/recording/bc_touch_tuning.c:123-224.

SUDO_VOICE_ONLY enables hold and, when memo is enabled, double tap; the bits are hold 0x0008 and double tap 0x0002: firmware/bc_ros/bc_module/recording/bc_touch_tuning.h:7-15,25-33; firmware/bc_ros/bc_module/recording/bc_touch_tuning.c:181-195.

## 8. Legacy controls

The app worker cancels legacy work for candidate commands and polls the older 0x36 adapter only when no native recording is active and the native reader is closed: firmware/bc_ros/bc_application/app_sudo_voice.c:509-550.

Legacy list 0x10, upload 0x11, resume 0x18, cancel 0x02, and truthful space 0x14 remain worker-governed. List payloads contain total count u32, one-based sequence u32, file size u32, and a 38-byte name. Uploads contain status 1, remaining bytes u32, chunk count u32, sequence u32, chunk length u32, and up to 220 raw bytes; a full packet requires ATT at least 241. Names are exactly 38 bytes with no NUL, slash, or backslash, and byte 33 must be ASCII 8, B, or D. Old files open read-only: firmware/bc_ros/bc_module/recording/bc_voice_legacy_archive.h:10-146; firmware/bc_ros/bc_module/recording/bc_voice_legacy_archive.c:347-377,428-503,506-604.

Legacy delete 0x12, format 0x13, and batch 0x1a do not claim success. Root legacy files remain read-only through this path; no legacy format or delete is part of custody: firmware/bc_ros/bc_module/recording/bc_voice_legacy_archive.c:695-945.

## 9. BCL binding and compatibility gate

| Role | UUID |
|---|---|
| Voice service | BAE80001-4F05-4503-8E65-3AF1F7329D1F |
| Write | BAE80010-4F05-4503-8E65-3AF1F7329D1F |
| Notify/read | BAE80011-4F05-4503-8E65-3AF1F7329D1F |

BCL remains the CBPeripheral delegate. RingVoiceConnection installs a BCL public peripheral observer, which filters notifications by characteristic and 0x7e command marker and forwards ordered copied values to the native client. This source-level forwarding does not verify physical notification delivery; public observer physical forwarding is UNVERIFIED: apps/ios/Sudo/Services/RingVoiceConnection.swift:6-13,26-57,93-154.

The direct gate accepts hardware 603V1.23.2 and firmware 6.0.3.3S01 after fixed-field trimming. Other board or firmware combinations do not pass the voice-protocol gate: apps/ios/Sudo/Services/RingProductionBoard.swift:3-7,40-48; apps/ios/Sudo/Services/RingVoiceConnection.swift:93-154.
