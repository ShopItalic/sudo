# Ring recording and push-to-talk requirements

Reviewed September 6, 2026 for **603V1.23.2**, standard **1.23.2**. Factory
artifact version is **6.0.3.3Z62**; the engineering candidate identifies itself
as **6.0.3.3S01**. It does not select `1.23.2_one_sec`.

The candidate now implements a firmware-owned recording lifecycle, complete
local capture while connected, native hold/release and configurable double-tap
memos. The matching client is in `ShopItalic/app`. These are source changes
with host fault tests and target build checks, not changes to the user's ring.
Physical acceptance remains pending. See the [candidate](ring-firmware-candidate.md)
for current build evidence and the [wire contract](ring-voice-protocol.md) for
exact packets, errors and compatibility gates.

## Requirements and implementation

| ID | Requirement and source | Candidate implementation | Remaining acceptance |
| --- | --- | --- | --- |
| PTT-01 | Hold starts; release stops and finalizes, connected or standalone. R1, R2 §1/§3.1, R3 B1. | IQS7211E validated contact/release reports feed one gesture owner. Release during Start is retained. An independent capture guard stops on stale touch (750 ms lease), invalid report or duration limit, even while the recording worker is occupied. | Measure actual sensor/IRQ/PDM timing and the requested one-second release bound. |
| PTT-02 | Optional double-tap memo start/stop. R1, R4. | Debounced double tap uses the same local recording owner. Memo enable and duration persist. Hold remains enabled when memo is disabled; no swipe/media action is enabled. | Verify accidental-contact rates and the configured gesture mask on the fitted sensor. |
| REC-01 | Complete local audio from the start while connected; optional live stream. R4, R1. | File identity/open precede capture success. Each complete ADPCM block is accepted locally before optional live delivery. BLE readiness affects preview only. | Connected double tap must produce a new nonzero file with audible beginning and end. |
| REC-02 | Explicit Start, Stop, state and final metadata. R4. | Versioned request IDs, persistent recording IDs, idempotent commands, state revisions, result codes, final name/size/frame count/CRC and complete/partial flags. Stop waits for finalization; lost replies can be resolved by querying state. | Validate the BCL notification forwarding hook on the physical phone/ring pair. |
| REC-03 | Stop production, drain accepted capture tail, then sync/close. R4, Caption adaptation. | Session-tagged PDM buffers and a drain barrier replace queue clearing. Write/checkpoint/close failures propagate; uncertain audio becomes partial. Empty capture is reported as empty. | Measure encoder and Flash latency; inject physical disconnects during finalization. |
| REC-04 | Disconnect/app death preserves capture; restart recovers a verified prefix. R4 §5. | Phone lifecycle never owns the microphone. A real LittleFS store uses checked metadata, checkpoints and completion markers. Cold mount recovers valid frames without restarting capture or formatting the volume. | Physical abrupt-power tests must establish the actual tail-loss envelope. |
| SYNC-01 | Resume and delete only after durable phone receipt; never overwrite unsynced audio. R1, R2, R3 B1.6. | Separate stream ACK, durable raw download verification, phone WAV/SHA validation, exact custody receipt and deletion. Resume binds ID/size/CRC/token/offset. Full storage rejects capture instead of reclaiming pending files. | Validate raw ADPCM decoding and interrupted app storage on device. Old factory root files remain read-only under the candidate until custody migration is defined. |
| UI-01 | Green while recording; consistent start/stop/error haptics. R2 §3.2–3.3, R3 B2–B3. | Recording owns the LED. Connection/custom LED events cannot override it. Default finite PWM pulses are 120 ms start, 280 ms successful stop, 400 ms error, with user-configurable strength/start/stop duration. | Measure visible behavior and physical motor response; PWM active time is not measured mechanical vibration time. |
| UI-02 | Standalone persisted LED/haptic settings and truthful readback. R2, R3. | SETTINGS and TUNING use checksummed LittleFS attributes with exact readback before success. App keeps confirmed values separate from edits. LED/haptic enable, strength and durations persist. | Power-cycle and read back every boundary setting on a spare unit. |
| TOUCH-01 | Reduce swipes, configure thresholds and optional gestures. R1, R2 §3.4, R3 B4. | Sensor mask admits hold and optional double tap only. Desired thresholds persist; I2C writes occur in a valid no-contact communication window and require register readback. Pending/applied/error is explicit. | Establish comfortable thresholds and verify release/no-contact semantics on the fitted IQS revision. |
| LINK-01 | Bounded live/file flow, resume, errors and stable connected use. R2 §3.5, R3 B5. | One BLE writer, bounded retry/deadline, epochs, fragment CRC, Ready lease, live/file windows, offset validation and checked cancellation. Local storage survives preview failure. | Measure goodput, MTU/PHY/window behavior, weak signal, ten-minute mixed use and iOS suspension. |
| POWER-01 | Stable, truthful battery and charging reporting. R2/R3 power list. | Checked ADC open/read/close and a single-flight status/measurement transaction. Trimmed filter resets across charging phases; invalid readings are UNKNOWN, not empty battery. Existing voltage table/cutoff and charging values remain. | Calibrate actual cells, charge/full states, thermal behavior and recording/standby current. |
| HID-01 | Long press for voice-to-text input in keyboard mode. R3 B1.7; phone ASR in R2 §1. | The Ring supplies hold/release capture and audio; the app's optional foreground preview publishes complete transcripts to its existing shared dictation store. | **Incomplete:** native live ASR does not run while another app is foregrounded. The keyboard extension has no BLE/model engine. The intended same-phone or Mac host must be selected before completing the text-input path. |
| HID-02 | Related supplier feasibility question: accept UTF-8 over BLE and emit HID keyboard reports. R3 B1.7/B4.16. | No UTF-8-to-HID command is implemented. Existing app transcript → shared store → iPhone keyboard-extension insertion is a different path. | Define host, keyboard layout, Unicode and acknowledgement behavior if this route is selected. The exploratory transport question does not remove the HID-01 product requirement. |

The implemented recording, storage, transfer, gesture and feedback paths are
exercised with production logic in host tests; hardware-facing tests replace
the peripheral/RTOS boundary. The wire/ADPCM
adapter and preview also have client tests. This establishes deterministic
software behavior under the modeled failures, not physical qualification.

## Decisions and historical conflicts

| Earlier requests | Current interpretation |
| --- | --- |
| June describes online-only PTT; R4 requires connected recordings to appear in Flash. | R4 and the latest reliability request supersede online-only storage: all new recordings have local backup from their first captured block. Live preview is optional. |
| April forbids overwriting unsynced clips; one June agenda item says delete oldest. | Preserve unsynced audio. Deletion requires exact durable custody and remains a separate acknowledged operation. No automatic oldest-file reclamation. |
| April asks for white recording LED; June asks for green in every mode. | Use the later green requirement, with persisted LED enable/disable. |
| April specifies a ten-second clip; later requests include memos and a ten-minute stability test. | Provisional default: PTT is ten seconds, configurable to another supported limit or until release; memo/app recording defaults to no duration limit. The ten-minute test is a connection/use test. User selection can change the defaults without a protocol change. |
| April requests 16 kHz mono Opus; current firmware and app use vendor ADPCM. | Preserve 220 encoded bytes → 440 PCM16 samples, interpreted as 8 kHz mono. An Opus/rate change requires measured codec, CPU/RAM/power and negotiated app support. |
| Optional media/HID actions are unwanted; June requests keyboard dictation and explores a Ring HID transport. | Keep media/swipe side effects disabled. Long-press keyboard dictation remains a product requirement; the host is unresolved and its background text-input path is incomplete. Ring UTF-8-to-HID forwarding is a related feasibility question. |

The two product questions—preferred PTT default and intended keyboard host—were
raised during this work. Until answered, the defaults above preserve the old
short-PTT requirement without imposing it on long memos. The keyboard exploration
and its unfinished text-input path remain visible rather than being declared
removed or finished.

## Recording lifecycle and durability

```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> Starting: Hold / double tap / app Start
    Starting --> Recording: Local file open + capture accepted
    Starting --> Failed: Open/capture failure
    Recording --> Recording: BLE lost / app unavailable
    Recording --> Stopping: Release / stop tap / Stop / limit
    Stopping --> Saved: Tail drained + checked finalization
    Stopping --> Partial: Uncertain tail or I/O fault
    Stopping --> Empty: No complete captured block
    Saved --> Delivered: Exact durable phone receipt
    Delivered --> Retired: Separately acknowledged delete
    Partial --> Delivered: Recoverable prefix verified on phone
```

`bc_recording.c` owns transitions; `app_sudo_voice.c` is the only mounted
recording/filesystem worker; `app_sudo_capture.c` owns session-tagged PDM/encoder
buffers. No factory online-to-offline timing conversion is involved in candidate
recording. Reconnect cannot terminate a recording; a reboot never restarts the
microphone automatically.

The selected configuration checkpoints every **1,000 ms or 4,096 accepted raw
bytes**, whichever is reached first, and uses a **500 ms stop-drain timeout**.
These are worker scheduling targets, not guaranteed Flash completion times.
Abrupt power loss can discard an uncheckpointed tail; a failed sync can increase
that loss. Recovery reports verified audio and a partial flag rather than a
fabricated complete file. The current LittleFS cold-cut harness exercises **147
cuts** during recording, receipt and deletion with fresh mounts and no graceful
unmount. It verifies prefixes and preservation of earlier identities.

Native storage keeps full 64-bit identities, checksummed metadata, raw ADPCM, checkpoint byte/frame counts and CRC, completion information and durable receipt/tombstone
state. App export remains raw vendor ADPCM. Catalog reads metadata; file resume
first verifies the stored content in bounded scan steps. This avoids loading a
whole memo into RAM. A full file download starting at offset zero proves raw
CRC; a resumed suffix alone cannot authorize deletion.

The optional phone preview requires a verified account, exact candidate version
and capabilities, active foreground app, valid connection epoch and exclusive
transport/decoder leases. A gap, stale token, stalled ASR or partial result falls
back to stored-file sync. READY confirmation is distinct from first decoded
PCM, stream ACK is distinct from durable custody, and preview does not create a
second canonical memo. iOS background assertions are finite; checkpoint/resume
handles the next permitted connection or execution opportunity.

## Configuration contract

Defaults are ten-second PTT, unlimited memo, double-tap memo enabled, recording
LED enabled and haptics enabled. Tuning defaults are touch set/clear **54/52**,
strength **100%**, start **120 ms** and stop **280 ms**.

Supported touch set is **32–80**; clear is **30 through set−2**. These are sensor
threshold register values, not measured touch-force units. Haptic strength is
**1–100% of the retained safe PWM range** (the supplier waveform's 68% maximum
duty); start/stop active time is **20–400 ms in 20 ms steps**. Setup delay and a
quiet PWM tail are separate. Failed hardware setup returns failure for the
legacy pulse command; recording state continues to report storage truth even
if feedback hardware fails.

Persisted desired touch settings can be `pending`, `applied` after write/readback,
or `I/O error`. They are applied only in the sensor's no-contact communication
window and are re-applied after sensor reset. A success response confirms the
persisted desired settings; it does not falsely claim the sensor has already
accepted them. The client exposes this distinction. Unsafe manufacturing,
reset and legacy configuration controls cannot race an active capture owner.

## Factory diagnosis retained

The user's Chinese message correctly diagnoses the **factory** standard target:
`app_pdm_touch_start()` selects `PDM_MODE_ONLINE`, and its Flash open/write/close
branches are compiled only for `HANDWARE_1_23_3` or
`HANDWARE_1_23_2_ONE_SEC`. Standard 1.23.2 defines neither. Offline
`app_pdm_recording_start()` creates a LittleFS recording. Factory hold-start/stop
branches do not activate this standard board's recorder; IQS release coordinates
were not forwarded as a release event. Stop cleared queued capture audio, and
some recording writes ignored errors.

At app baseline
[`60c37a3`](https://github.com/ShopItalic/app/tree/60c37a3dcfed32f80e7b615b57d089c1c9a8835b),
the factory double-tap workaround sends a live Stop, waits 150 ms, then issues
`ringStartRecording(true)`. It serializes commands and retains early Stop, but
cannot recover an already-streamed prefix or make the unacknowledged live Stop
a durable file receipt. The matching app keeps that path for **6.0.3.3Z62**;
**6.0.3.3S01 plus HELLO capabilities** uses the native protocol.

The malformed internal five-byte Starts declaring length four were also fixed;
99 actual producer/parser/dispatcher checks retain that regression coverage.
Factory artifacts and the imported source baseline remain unchanged. Neither
factory artifact identity nor candidate source proves what is currently flashed
on the user's ring.

## Physical acceptance matrix

Every row below remains pending on a verified spare **standard 603V1.23.2**.
Record board/firmware readback, image hash, app commit, configuration, starting
file list, timing logs and resulting audio. Do not substitute a `one_sec` board.

| Test | Required result |
| --- | --- |
| Connected double tap | BLE stays connected; double tap, speak, double tap. A new nonzero complete file appears, downloads and plays beginning and end. This is R4's central acceptance. |
| Connected and standalone PTT | Hold, speak, release. Correct green/start/stop feedback, explicit state and complete local file. Measure release-to-microphone-stop within one second. |
| Very short and rapid gestures | Release/second tap during Start, duplicate reports and taps during finalization never leave capture running or invent a saved clip. Empty audio is explicit. |
| App death and BLE loss | Local gestures and finalization continue; reconnect lists and retrieves the correct identity. No accidental restart or stop on reconnect. |
| Abrupt power | Cut power across open/write/checkpoint/finalize/receipt/delete boundaries. Recover verified prefixes, mark partial and retain unrelated recordings. Measure tail loss. |
| Full storage and I/O errors | Existing pending audio is preserved. Open/write/sync/close failures never report false success or advance verified bytes. |
| Tail and sustained memo | Known audio tail appears exactly once. Long memo remains one identified file until Stop/limit/fault; automatic rollover is not needed by the current contract. |
| Retry and resume | Lost Start/Stop ACKs converge on one recording. Interrupted transfers resume at verified aligned offsets; stale callbacks/cancellation cannot act on a new epoch. |
| Custody and deletion | Complete raw CRC, playable WAV and durable app identity/integrity checks precede receipt and delete; failed phone storage leaves the Ring copy. |
| Settings and feedback | Boundary values persist through power cycle; pending sensor settings become verified only after actual readback; no unwanted swipe/media behavior. |
| Power and charging | Calibrate percentage and unknown handling, charge/full presentation, battery current, temperature and case interaction with real cells. |
| Ten-minute mixed use | Record/stop/sync permitted clips under weak signal and foreground/background changes; record goodput, memory/stack, errors and current draw. |
| Update and recovery | Supplier reconciles the baseline/compiler, reviews GNU ABI/map, assigns/signs a release and demonstrates interrupted-update recovery before any user-ring update. |

## Evidence retained in this repository

The three historical documents are exact byte copies. Their original Drive
locators, sizes and SHA-256 hashes are in the
[requirements source manifest](sudo-ring/requirements/sources.json). They retain
historical names and contradictory requests deliberately; this review records
which requirements supersede them.

- **R1:** [April 13 master feature spec](sudo-ring/requirements/master-feature-spec-2026-04-13.md).
- **R2:** [June 13 supplier pre-read](sudo-ring/requirements/supplier-pre-read-2026-06-13.md).
- **R3:** [June 13 call agenda and change requests](sudo-ring/requirements/supplier-call-agenda-2026-06-13.md).
- **R4:** [User-supplied Chinese connected-recording request](sudo-ring/requirements/connected-recording-request-2026-09-06.zh.md).

The [candidate reference](ring-firmware-candidate.md) records the implementation, reproduction commands
and validation limits. The [backlog](../backlog.md) tracks remaining integration
and device acceptance. This review does not send a message to the supplier or
change installed firmware.
