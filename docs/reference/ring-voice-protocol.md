# Ring Voice protocol v1 (candidate)

This document records the candidate voice protocol implemented by the firmware.
The iOS paths cited below show the existing adapter and the requirements for a
future S04 integration; the checked app does not yet accept S04. It is a source
contract for the candidate service on the existing BCL characteristic. It makes
no claim of a physical-ring test, measured performance, signed image, or
flash/deployment result. The firmware labels this extension candidate-only:
[bc_voice_wire.h](../../firmware/bc_ros/bc_module/recording/bc_voice_wire.h).

Primary firmware sources are firmware/bc_ros/bc_module/recording/bc_voice_wire.h and .c for framing, bc_voice_protocol.h and bc_voice_service.c for messages and service behavior, bc_recording.h for state/results, bc_audio_format.h for the per-recording audio descriptor, bc_opus_stream.h for the Opus container, and firmware/bc_ros/bc_application/app_sudo_voice.c for the standard worker. The app mirror is apps/ios/Sudo/Services/RingVoiceWire.swift, RingVoiceProtocol.swift, RingVoiceConnection.swift, RingVoiceRecordingTransport.swift, RingVoiceLiveReceiver.swift, RingVoiceLivePreview.swift, RingProductionBoard.swift and the shared ItalicAudio* decoder files.

## S05 Opus recording and format discovery (experimental)

The **6.0.3.3S05** experimental identity records new audio as Opus and never
reuses the S04 identity, so an S04-gated client cannot mistake an Opus Ring
for ADPCM firmware. Existing ADPCM recordings remain readable and retrievable
through the same catalog, resume, receipt and delete operations.

Every recording carries a 16-byte **audio descriptor** (codec u8, container
version u8, sample rate u16, channels u8, frame ms u8, pre-skip u16, exact
sample count u32, block bytes u16, block samples u16, all little endian). It is
persisted in the recording's metadata (version 2, 104 bytes; version 1 records
from S01-S04 decode as ADPCM `1,0,8000,1,55,0,0,220,440`) and appended after
the name bytes of every START/STOP/QUERY/CATALOG/STATE snapshot. The idle or
end-of-catalog sentinel carries an all-zero descriptor. Codec values are
1 = supplier IMA ADPCM and 2 = Opus; unknown values must be preserved on the
phone without decoding.

New recordings use codec 2 with the **Sudo Opus container v1**: a 16-byte
header (`SOPU`, version 1, codec 2, sample rate u16, channels u8, frame ms u8,
pre-skip u16, reserved u32 = 0), then length-prefixed records (u16 length
followed by one RFC 6716 packet), then an 8-byte trailer (length 0, kind 1,
real sample count u32). The S05 profile writes single-frame packets of at most
1,275 bytes, but that is RFC 6716's per-frame bound, not a packet bound:
readers must accept any valid packet up to the u16 record length, including
multi-frame packets larger than 1,275 bytes, and validate the codec-defined
framing (frames of at most 1,275 bytes, at most 120 ms and 48 frames per
packet). A trailer may only trim samples the packets carried, and a completed
recording (descriptor sample count nonzero) always ends with a trailer that
repeats that count; readers must reject a trailer or descriptor that claims
more samples than the packets decode to. A recording interrupted before its
trailer is a valid prefix whose trailing partial record is ignored and whose
sample count is unknown. The firmware profile is 16 kHz mono, 20 ms frames, 12 kbps
CBR, complexity 0, DTX and in-band FEC off, resampled from the nominal
16.125 kHz microphone rate ([bc_opus_profile.h](../../firmware/bc_ros/bc_module/recording/bc_opus_profile.h)).
Clients must accept any valid CBR or VBR packet sequence, including bitrate
changes between packets, and derive duration from the packets, pre-skip and
trailer rather than from the profile.

HELLO capability bit **12** advertises format discovery. With it, the HELLO
sample rate and samples/block fields describe the current encoder profile
(16000 and 320) and bytes/block is the maximum LIVE/FILE chunk (220).
**FORMAT_GET (19)** returns the descriptor new recordings receive plus encoder
instrumentation: encoder state bytes u32 at 21, scratch bytes u32 at 25,
scratch high-water u32 at 29, encoded frames u32 at 33, maximum encode
microseconds u32 at 37, mean encode microseconds u32 at 41 and encoder faults
u32 at 45 (49 bytes). Without a prepared encoder FORMAT_GET returns
UNSUPPORTED (20) and START reports UNSUPPORTED instead of labeling audio.

**LIVE** blocks are now container chunks of 1..220 bytes; the phone
reassembles the byte stream in sequence order and parses records from it.
Chunks end on a record boundary whenever a record fits, and only a record
larger than 220 bytes is fragmented across consecutive chunks. FILE transfer
is unchanged and byte based; Opus resume offsets need no alignment. The new
result **ENCODER_ERROR (23)** reports an encoder initialization or encoding
failure; audio already committed remains a valid prefix.

## S04 controls

S04 requires a zero PTT limit (until release) in SETTINGS_SET and PTT START.
Memo/app limits remain independent. Exactly three physical inputs are mappable:
hold, double tap and triple tap. Fresh defaults are a one-second hold → PTT,
double tap → disabled, and triple tap → memo toggle. Hold activation delay is
500–10,000 ms; it delays activation rather than capping capture duration.

INPUTS_SET/GET configure hold delay and all three mappings. Actions are disabled
0, PTT 1 (hold only), memo toggle 2 and SDK/app event 3. Host integrations must
require exact S04 identity and HELLO bits 9, 10 and 11, while preserving older
firmware behavior. S04 app adoption is outside this firmware-only change. S04 SETTINGS memo-enabled is a compatibility mirror of whether any
mapping uses memo toggle; changing that field through SETTINGS_SET is rejected.
Feedback saves preserve it. TUNING controls thresholds and haptic parameters.

Valid older SVS1 settings load with PTT forced to zero. In the absence of SVI1
mappings, old memo-enable choices seed triple tap on/off, retaining opt-out.
The S03 PHONE_OUTCOME and S02 application feedback policies remain.

INPUT_EVENT is a connection-scoped live event for the SDK/app action mapping:
sequence u32 at 0; input u8 at 4 (hold 1, double 2, triple 3); phase u8 at 5
(activated 1, released 2, cancelled 3); action 3 at 6; reserved zero at 7.
Hold emits activation then release/cancellation; taps emit activation only.
The four-entry queue and an in-flight event expire after one second; link
changes clear them. S04 clients must reject invalid/duplicate sequences within an epoch
and must not persist or replay events. Consumers must handle loss, cancellation
and disconnect rather than assuming reliable offline action execution.

## Application controls in S02 (historical)

This section preserves the S02 wire behavior. Current S04 input mappings use
`INPUTS_SET`/`INPUTS_GET` above; they do not reinterpret the historical
`memo_enabled` field described here.

The post-RC1 S02 candidate uses the existing acknowledged SETTINGS and TUNING
messages; no new packet layout is needed. SETTINGS enables/disables double-tap
recording, lights and haptics and sets recording limits. TUNING sets touch
thresholds and bounded vibration strength/start/stop durations. Read the current
values before changing a subset, preserve the other fields, and accept a change
only after a successful response containing the exact saved values. Changes are
accepted while idle; active recording/held contact returns BUSY. Desired sensor
settings additionally report pending/applied/I/O-error after actual readback.

S02's light/haptic switches govern all normal output from the running
application, including manual feedback commands. Muted manual-on requests fail;
queued output cannot replay across mute/unmute. The worker restores policy after
mounting settings at startup. These controls do not change bootloader/DFU output
or claim suppression before application settings load. The published S01/RC1
switches govern recording feedback only. The app labels the scope by exact
firmware version instead of promising global mute on S01.

Double tap is disabled by default on fresh S02 settings. Existing persisted
choices are retained; use SETTINGS_SET to opt in or opt out. `memo_enabled` is
kept as the wire field name for compatibility; the app calls it **Double-tap
recording**. Explicit app Start/Stop and hold/release still work when it is off.

## S03 phone confirmation

HELLO capability bit 8 advertises PHONE_OUTCOME when its feedback port is
installed. Hosts must require an exact supported board/version and this capability;
the existing S03 client does not imply S04 client support.
PHONE_OUTCOME returns the ordinary five-byte response. It confirms neither
archive custody nor permission to delete. Only the current complete saved
recording and its READY-accepted live token in the same connection qualify,
within ten seconds of the first successful terminal transition. Duplicate
valid acknowledgments return success without repeated feedback. See the
[S03 behavior and qualification report](ring-s03-reliability.md).

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

Wire constants are header 12 bytes, maximum body 232 bytes, maximum logical payload 228 bytes, maximum packet 250 bytes, and minimum ATT packet limit 20 bytes. Each fragment carries at most ATT-limit minus 12 bytes: [bc_voice_wire.h](../../firmware/bc_ros/bc_module/recording/bc_voice_wire.h); [bc_voice_wire.c](../../firmware/bc_ros/bc_module/recording/bc_voice_wire.c); [RingVoiceWire.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceWire.swift).

The body CRC is over the exact prefix [version, kind, direction, message-ID low, message-ID high] followed by the logical payload. It uses the reflected polynomial 0xedb88320 with 0xffffffff initialization and final XOR; the four CRC bytes are appended little endian and removed before the message is exposed. This transport CRC is distinct from the raw-file CRC in recording metadata: [bc_voice_wire.c](../../firmware/bc_ros/bc_module/recording/bc_voice_wire.c); [RingVoiceWire.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceWire.swift).

Receivers accept only the contiguous frontier. Offset zero replaces an incomplete assembly; a nonzero offset cannot start one. Exact replay of a wholly received range is accepted, but changed overlap, a gap, invalid length, a bad CRC, an epoch change, or an assembly older than 1000 ms cannot produce a message: [bc_voice_wire.h](../../firmware/bc_ros/bc_module/recording/bc_voice_wire.h); [bc_voice_wire.c](../../firmware/bc_ros/bc_module/recording/bc_voice_wire.c); [RingVoiceWire.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceWire.swift).

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
| 16 PHONE_OUTCOME (S03+) | request | 17 | recording ID u64 at 4; live token u32 at 12; outcome u8 at 16 (1 = keyboard inserted) |
| 17 INPUTS_SET (S04) | request | 9 | hold ms u16 at 4; hold/double/triple action u8 at 6, 7, 8 |
| 18 INPUTS_GET (S04) | request | 4 | none |
| 19 FORMAT_GET (S05) | request | 4 | none |
| 0x43 INPUT_EVENT (S04) | event | 8 | sequence u32; input/phase/action/reserved u8, as above |

Trigger values are PTT 1, memo 2, and app 3. START IDs are persistent idempotency keys across reconnects and reboots. Lengths, IDs, tokens, booleans, duration bounds, and tuning bounds are enforced by [bc_voice_protocol.h](../../firmware/bc_ros/bc_module/recording/bc_voice_protocol.h), [bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c), and [RingVoiceProtocol.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceProtocol.swift).

Event kinds are:

| Kind | Direction | Length | Payload |
|---:|---|---:|---|
| 0x40 STATE | event | 62 plus name length plus 16 (S05) | Snapshot, request ID 0; idle/end sentinel has recording ID 0 |
| 0x41 LIVE | event | 9 through 228 | stream token u32 at 0; sequence u32 at 4; 1 through 220 container bytes at 8 (S01-S04: exactly 220 ADPCM bytes) |
| 0x42 FILE | event | 9 through 228 | transfer token u32 at 0; absolute file offset u32 at 4; 1 through 220 raw bytes at 8 |

These event shapes are defined in [bc_voice_protocol.h](../../firmware/bc_ros/bc_module/recording/bc_voice_protocol.h) and [RingVoiceProtocol.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceProtocol.swift).

## 3. Responses, snapshots, and results

Every response begins with request ID u32 at offset 0 and result u8 at offset 4. A rejected operation can use the five-byte response. Successful extensions are:

| Response | Length | Fields after the common five bytes |
|---|---:|---|
| HELLO | 20 | capabilities u32 at 5; sample rate u16 at 9; samples/block u16 at 11; bytes/block u16 at 13; checkpoint interval ms u16 at 15; release bound ms u16 at 17; transfer window u8 at 19 |
| READY | 17 | live token u32 at 5; current recording ID u64 at 9 |
| INPUTS_GET or INPUTS_SET | 11 | hold ms u16 at 5; hold/double/triple action u8 at 7, 8, 9; sensor apply status u8 at 10 |
| FORMAT_GET (S05) | 49 | audio descriptor 16 bytes at 5; encoder state bytes u32 at 21; scratch bytes u32 at 25; scratch high-water u32 at 29; frames u32 at 33; max encode µs u32 at 37; mean encode µs u32 at 41; faults u32 at 45 |
| SETTINGS_GET or SETTINGS_SET | 16 | PTT limit u32 at 5; memo limit u32 at 9; memo, LED, haptic u8 at 13, 14, 15 |
| TUNING_GET or TUNING_SET | 13 | touch set u8 at 5; touch clear u8 at 6; haptic strength u8 at 7; start-active ms u16 at 8; stop-active ms u16 at 10; apply status u8 at 12 |
| RESUME | 29 | recording ID u64 at 5; transfer token u32 at 13; file bytes u32 at 17; raw CRC u32 at 21; requested offset u32 at 25 |

START, STOP, QUERY, and CATALOG normally return a snapshot response of length 62 plus name length, followed in S05 by the 16-byte audio descriptor; the Swift decoder also accepts a five-byte error response and treats an absent descriptor as legacy ADPCM. Response construction is in [bc_voice_protocol.h](../../firmware/bc_ros/bc_module/recording/bc_voice_protocol.h), [bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c), and [RingVoiceProtocol.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceProtocol.swift).

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
| after name | 16 | S05 audio descriptor: codec, container version, sample rate u16, channels, frame ms, pre-skip u16, sample count u32, block bytes u16, block samples u16 |

Phase values are idle 0, starting 1, recording 2, stopping 3, saved 4, partial 5, failed 6, empty 7 and delivered 8.

The complete result enum is: 0 OK; 1 INVALID; 2 BUSY; 3 WRONG_SESSION; 4 DUPLICATE; 5 NO_SPACE; 6 OPEN_ERROR; 7 WRITE_ERROR; 8 SYNC_ERROR; 9 CLOSE_ERROR; 10 CAPTURE_ERROR; 11 CAPTURE_OVERFLOW; 12 SEQUENCE_GAP; 13 STOP_TIMEOUT; 14 ALREADY_EXISTS; 15 EMPTY_AUDIO; 16 INTERRUPTED; 17 TOUCH_ERROR; 18 NOT_FOUND; 19 CUSTODY_REQUIRED; 20 UNSUPPORTED; 21 CANCELLED; 22 CRC_ERROR; 23 ENCODER_ERROR (S05). The enum is [bc_recording.h](../../firmware/bc_ros/bc_module/recording/bc_recording.h) and [RingVoiceProtocol.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceProtocol.swift).

HELLO capability bits are local storage 0, PTT 1, memo 2, live 3, resume 4, custody 5, settings 6, tuning 7, optional phone outcome 8 (S03+, advertised only with an outcome handler), triple tap 9, PTT until release 10, input mappings 11 (S04, advertised with the inputs port), and audio format discovery 12 (S05, advertised only with a prepared encoder): [bc_voice_protocol.h](../../firmware/bc_ros/bc_module/recording/bc_voice_protocol.h); [RingVoiceProtocol.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceProtocol.swift).

## 4. Handshake, epochs, and ordering

RingVoiceConnection performs HELLO before another request and validates the candidate capability set, 8 kHz sample rate, 440 samples/block, 220 bytes/block, release bound at most 1000 ms, and transfer window 1 through 6: [RingVoiceConnection.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceConnection.swift).

READY is a separate, per-recording stream lease. A new recording clears prior readiness. While BLE is connected, the service buffers up to **32 locally accepted raw frames** before initial READY; no LIVE is sent before confirmation. READY enabled 1 allocates a nonzero token and resets the ACK/window state while preserving the raw prefix and original sequences. Repeating enabled 1 while already ready renews the lease with the same token. Frames drain through the existing four-message ACK window and fragment backpressure.

READY enabled 0, a lost prefix/sequence gap, FIFO overflow, link replacement after captured audio, or a Ready/ACK timeout disables preview for that recording and clears live transport state. A subsequent READY enabled 1 returns **INTERRUPTED (16)** in the common five-byte error response; only a new recording can establish a complete new preview. Local Flash capture remains independent. The service expires readiness after **10,000 ms** without renewal and after a **2,000 ms** live ACK stall; the client renews at most every five seconds: [bc_voice_protocol.h](../../firmware/bc_ros/bc_module/recording/bc_voice_protocol.h); [bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c); [RingVoiceLivePreview.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceLivePreview.swift).

Epochs are local connection-generation fences, not an extra on-air field in the 12-byte header; the two peers do not need matching epoch numbers. A link/epoch change revokes readiness and transport state, clears incomplete wire assembly, and leaves the local recording intact for recovery. The client cancels pending operations and resets receiver/event state for the new epoch: [bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c); [RingVoiceWire.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceWire.swift); [RingVoiceClient.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceClient.swift).

In S03, service polling performs at most one bounded archive verification/read step and enqueues up to four fragments of a single message. It stops immediately on backpressure or message completion; it cannot select another message or perform another Flash read in that burst. TX order is control responses, pending/urgent STATE, LIVE, then FILE; a control or STATE can therefore precede the tail of an already queued LIVE stream. Client/preview drains are bounded: up to four incoming LIVE blocks can wait for the READY response at the phone (separate from the Ring's 32-frame raw FIFO), and terminal preview permits a one-second drain before a five-second finish deadline. ASR startup has its own five-second deadline, including waiting for prior ASR cleanup: [bc_voice_service.h](../../firmware/bc_ros/bc_module/recording/bc_voice_service.h); [bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c); [RingVoiceLivePreview.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceLivePreview.swift).

The standard 1.23.2 worker declares `{1000U, 4096U, 500U}`. With bc_rec_config order checkpoint_ms, checkpoint_bytes, stop_timeout_ms, this is a 1000 ms checkpoint interval, 4096-byte checkpoint threshold, and 500 ms stop timeout: [app_sudo_voice.c](../../firmware/bc_ros/bc_application/app_sudo_voice.c); [bc_recording.h](../../firmware/bc_ros/bc_module/recording/bc_recording.h).

## 5. LIVE and stateful decoding

LIVE is accepted only for the current recording and stream token, with sequence beginning at 1 and, on S05, 1 through 220 container bytes per block (exactly 220 ADPCM bytes on S01-S04). LIVE_ACK carries cumulative next sequence. The iOS receiver decodes each contiguous block once, accepts an exact retained duplicate without decoding again, rejects a conflicting duplicate or gap, and uses a stateful decoder selected by the recording's descriptor. Explicit start/end resets the decoder; reconnect/rejoin and a lost ACK do not. A suffix without the required sequence-1 prefix falls back to archive sync: [bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c); [RingVoiceLiveReceiver.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceLiveReceiver.swift).

## 6. Catalog, native transfer, and custody

CATALOG returns one entry per request in ascending nonzero ID order. The end response is NOT_FOUND with recording ID 0 and the idle sentinel, not an OK snapshot. iOS skips zero/delivered terminal entries and active writing entries, rejects duplicate names or invalid metadata, and merges legacy entries after removing native-name duplicates: [bc_voice_protocol.h](../../firmware/bc_ros/bc_module/recording/bc_voice_protocol.h); [bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c); [RingVoiceRecordingTransport.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceRecordingTransport.swift).

RESUME requires an existing nonempty source. Firmware verifies raw bytes in steps of at most 1024 bytes and sends metadata only after verification. Verification/read failure emits an error and cancels the operation. A token is nonzero and monotonic; equal-token retry is limited to an active operation with the same recording and original offset. A fresh-token RESUME for already-deleted audio returns **NOT_FOUND (18)**. In S03, a retired token returns CANCELLED and a new attempt requires a higher token: [bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c).

FILE carries transfer token, absolute offset, and up to 220 raw bytes. The firmware allows six outstanding FILE blocks and retries stalled progress after 1500 ms. S03 preserves the outstanding block boundaries across rewind, so a delayed valid cumulative ACK remains acceptable; replay cannot regress the acknowledged cursor or consume new window slots. After 30 seconds without real verification progress or an advancing valid ACK, S03 closes the archive reader and cancels the transfer. Retries, duplicate ACKs and same-token RESUME requests do not renew that deadline. The deadline also covers blocked first fragments, mid-message stalls and an EOF resume with no outstanding blocks. It preserves the source file and custody state; a new transfer uses a higher token. The wire service accepts byte offsets within the verified file; the iOS ADPCM transport imposes the stricter 220-byte alignment rule except the exact end, full blocks before the final block, exact contiguous offsets/tokens, and a bounded six-block window: [bc_voice_protocol.h](../../firmware/bc_ros/bc_module/recording/bc_voice_protocol.h); [bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c); [RingVoiceRecordingTransport.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceRecordingTransport.swift).

The client calls its checkpoint before TRANSFER_ACK. It independently computes the raw CRC for a read beginning at offset zero and compares metadata. A resumed prefix is immutable; a replay-prefix digest is only a local pipeline check and is not a fabricated whole-file CRC. Failed, incomplete, or cancelled downloads cannot authorize a receipt. A recording marked partial can still be downloaded and verified in full, then receipted as that exact recoverable prefix.

| Custody level | Evidence and meaning |
|---|---|
| TX accepted | Local send callback true means the BLE/ATT path accepted a packet fragment; it is not a remote receipt or file custody. |
| LIVE ACK | Kind 6 cumulative next sequence proves live sequence progress only; it says nothing about native-file durability. |
| Checkpoint ACK | Kind 11 cumulative next byte offset reports a durably retained client prefix, not a complete recording identity. |
| Exact receipt | Kind 12 exact recording ID, byte count, and raw CRC follows complete proof; only this permits firmware delivered state, with delete as a separate requested action. |

RECEIPT is accepted only with exact ID/size/CRC; firmware records the receipt and honors delete only for that identity. If raw removal after custody fails with a storage I/O error, the result is **WRITE_ERROR (7)**; the checked receipt tombstone remains intact and the same exact receipt/delete request can be retried safely to complete cleanup. These mappings keep the existing wire enum and underlying store/tombstone behavior unchanged: [bc_voice_protocol.h](../../firmware/bc_ros/bc_module/recording/bc_voice_protocol.h); [bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c); [bc_recording.h](../../firmware/bc_ros/bc_module/recording/bc_recording.h); [RingVoiceRecordingTransport.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceRecordingTransport.swift).

## 7. Settings and touch tuning

S04 defaults are PTT limit 0 for until release, memo limit 0 for unlimited, hold after 1 second mapped to PTT, double tap disabled, triple tap mapped to memo toggle, and application LED/haptics enabled: [app_sudo_voice.c](../../firmware/bc_ros/bc_application/app_sudo_voice.c).

Settings persist in a 20-byte A6 attribute: SVS1 bytes 0-3; flags byte 4 with memo bit 0, LED bit 1, haptic bit 2; reserved bytes 5-7; PTT u32 at 8; memo u32 at 12; CRC32 over bytes 0-15 at 16. The firmware reads back the attribute before reporting SET success: [app_sudo_voice.c](../../firmware/bc_ros/bc_application/app_sudo_voice.c).

Tuning defaults are set 54, clear 52, haptic strength 100, start-active 120 ms, and stop-active 280 ms. Ranges are set 32-80, clear 30 through set minus 2, haptic 1-100, and start/stop 20-400 ms divisible by 20: [app_sudo_voice.c](../../firmware/bc_ros/bc_application/app_sudo_voice.c); [bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c); [RingVoiceProtocol.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceProtocol.swift).

Tuning persists in A7: SVT1 bytes 0-3; set 4; clear 5; haptic 6; reserved 7; start u16 at 8; stop u16 at 10; reserved 12-15; CRC32 over bytes 0-15 at 16. Response status is pending 0, applied/verified 1, or I/O error 2. SET confirms durable desired values; the IQS sensor applies them on the next valid no-contact/release report and reads them back. I/O error retains desired values while reporting unapplied state: [app_sudo_voice.c](../../firmware/bc_ros/bc_application/app_sudo_voice.c); [bc_voice_service.c](../../firmware/bc_ros/bc_module/recording/bc_voice_service.c); [bc_touch_tuning.h](../../firmware/bc_ros/bc_module/recording/bc_touch_tuning.h); [bc_touch_tuning.c](../../firmware/bc_ros/bc_module/recording/bc_touch_tuning.c).

Input mappings persist in a separate 16-byte A8 attribute: SVI1 bytes 0–3;
hold milliseconds u16 at 4; hold/double/triple action u8 at 6/7/8; reserved zero
at 9–11; CRC32 over bytes 0–11 at 12. SET verifies the saved bytes before
publishing desired mappings. Input and tuning responses report the shared
sensor status only when threshold, hold delay and gesture mask match.

S04 admits only enabled hold (0x08), double-tap (0x02) and triple-tap (0x04)
bits. Initial sensor setup is hold only; applying fresh mappings gives 0x0C.
Enabling all three gives 0x0E. No single, swipe or palm bit is enabled. In the
valid no-contact I2C window, firmware writes/readbacks threshold register 0x38,
hold time 0x4F and gesture enable 0x4B. Old/unverified queued gesture flags cannot
start an action under new mappings; release/fault handling remains active.
See [input types](../../firmware/bc_ros/bc_module/recording/bc_voice_inputs.h) and
[sensor tuning](../../firmware/bc_ros/bc_module/recording/bc_touch_tuning.c).


## 8. Legacy controls

The app worker cancels legacy work for candidate commands and polls the older 0x36 adapter only when no native recording is active and the native reader is closed: [app_sudo_voice.c](../../firmware/bc_ros/bc_application/app_sudo_voice.c).

Legacy list 0x10, upload 0x11, resume 0x18, cancel 0x02, and truthful space 0x14 remain worker-governed. List payloads contain total count u32, one-based sequence u32, file size u32, and a 38-byte name. Uploads contain status 1, remaining bytes u32, chunk count u32, sequence u32, chunk length u32, and up to 220 raw bytes; a full packet requires ATT at least 241. Names are exactly 38 bytes with no NUL, slash, or backslash, and byte 33 must be ASCII 8, B, or D. Old files open read-only: [bc_voice_legacy_archive.h](../../firmware/bc_ros/bc_module/recording/bc_voice_legacy_archive.h); [bc_voice_legacy_archive.c](../../firmware/bc_ros/bc_module/recording/bc_voice_legacy_archive.c).

Legacy delete 0x12, format 0x13, and batch 0x1a do not claim success. Root legacy files remain read-only through this path; no legacy format or delete is part of custody: [bc_voice_legacy_archive.c](../../firmware/bc_ros/bc_module/recording/bc_voice_legacy_archive.c).

## 9. BCL binding and compatibility gate

| Role | UUID |
|---|---|
| Voice service | BAE80001-4F05-4503-8E65-3AF1F7329D1F |
| Write | BAE80010-4F05-4503-8E65-3AF1F7329D1F |
| Notify/read | BAE80011-4F05-4503-8E65-3AF1F7329D1F |

BCL remains the CBPeripheral delegate. RingVoiceConnection installs a BCL public peripheral observer, which filters notifications by characteristic and 0x7e command marker and forwards ordered copied values to the native client. This source-level forwarding does not verify physical notification delivery; public observer physical forwarding is UNVERIFIED: [RingVoiceConnection.swift](https://github.com/ShopItalic/app/blob/main/apps/ios/Sudo/Services/RingVoiceConnection.swift).

The S04 integration requirement is hardware 603V1.23.2 with firmware
6.0.3.3S04 after fixed-field trimming, plus HELLO capability bits 9, 10 and 11
and the INPUTS_SET/GET contract above. This is a requirement for a future S04
client; it is not implemented by the currently verified app. At
ShopItalic/app commit `664ea438`, `RingProductionBoard.swift:48` accepts only
6.0.3.3S01, 6.0.3.3S02 and 6.0.3.3S03, and its capability gate ends at bit 8.
S04 client adoption remains a separate task. Other board or firmware
combinations do not pass the currently verified voice-protocol gate:
[RingProductionBoard.swift](https://github.com/ShopItalic/app/blob/664ea438c7265d57885f926e589acb0faa1d05ca/apps/ios/Sudo/Services/RingProductionBoard.swift);
[RingVoiceConnection.swift](https://github.com/ShopItalic/app/blob/664ea438c7265d57885f926e589acb0faa1d05ca/apps/ios/Sudo/Services/RingVoiceConnection.swift).

### S02 hold-to-stop escape (historical)

When an enabled double-tap recording is active in S02, a hold requests Stop
through the same drain-and-finalize path as a second double tap. Repeated hold
reports and release cannot start another clip; a new hold after release can
start PTT. This does not interrupt an app-owned recording. It provides another
gesture when a double tap is missed, but still needs a functioning touch sensor.
Supplier testing must reproduce the reported stuck double-tap behavior on
physical hardware; passing host tests does not establish its original cause.

### S04 memo-stop mapping

In S04, a hold mapped to PTT requests Stop when an active recording is a memo.
A hold mapped to another action does not add an implicit stop action. Any input
mapped to memo toggle can stop that memo. Repeated reports and release are
handled by the same drain-and-finalize owner; physical sensor behavior remains
pending acceptance.
