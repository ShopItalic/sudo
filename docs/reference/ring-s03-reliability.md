# S03 reliability changes

S03 targets standard `603V1.23.2`, reports `6.0.3.3S03`, and requires the
matching [SDK commit 492af2a](https://github.com/ShopItalic/app/tree/492af2ad40949de3d54419df5e2fa140c912b94f), merged in [app PR #12](https://github.com/ShopItalic/app/pull/12), for the new phone
confirmation and transfer optimization. S01 RC1 remains an immutable earlier
build. Source tests and a linked application do not establish physical readiness.

## User-visible behavior

| Situation | Meaning and behavior |
| --- | --- |
| Hold, then release | Record a clip locally, then finalize it. Optional live preview can provide same-iPhone dictation. |
| Double tap | Off with fresh settings; users can opt into hands-free recording. Existing choices persist. Holding can stop that recording. |
| Ordinary successful stop cue | The recording was saved on Ring. It does not claim the phone received text. |
| Two short confirmation pulses | The keyboard acknowledged inserting this exact dictation, on the same live connection. |
| App absent, killed, model unavailable, keyboard absent or text admission rejected | No insertion confirmation. Ring recording and later file sync remain available. |
| Haptics disabled | Both recording and insertion haptics remain silent. No confirmation replays after re-enabling. |

The confirmation is best effort. The keyboard writes an atomic App Group
receipt only after its existing time/document/edit gate permits insertion and
`insertText` returns. This acknowledges the keyboard operation, not a host
app's durable save. The app checks the exact random payload ID and session ID
for at most two seconds, then makes one bounded protocol attempt. Account,
epoch, transport lease, recording replacement and the existing 25-second
background cap still apply. A missing confirmation never deletes audio or
repeats insertion.

Firmware accepts outcome 1 only for a complete, successfully saved current
recording with a matching live token that accepted READY in this connection,
within ten seconds of its first successful terminal transition. Reconnects,
new captures, wrong tokens/IDs, partial files and expired requests cannot
produce the cue. Valid duplicates do not repeat it. The worker schedules two
80 ms pulses through the worker scheduler; a new capture, link change or mute
cancels pending confirmation. Physical perceptibility remains to be measured.

## Bluetooth transfer

S03 makes three changes in the firmware service:

| Change | Before | S03 behavior and reason |
| --- | --- | --- |
| Fragment scheduling | One fragment per worker pass | Up to four fragments from one message per pass, stopping immediately on backpressure or message completion. The worker returns to recording/touch processing before selecting another message. Flash verification/read work remains bounded to one step. |
| Delayed ACKs during retry | Rewind discarded the six sent block boundaries | Preserve those boundaries until acknowledged. A delayed valid cumulative ACK can advance during retransmission; the acknowledged offset cannot regress and replay does not allocate duplicate window slots. |
| Abandoned transfer | Retries could keep the archive open indefinitely | Cancel after 30 seconds without actual verification progress or an advancing valid ACK. Duplicate ACKs, retries and same-token RESUME do not renew it. Source audio and custody survive; a higher-token RESUME can retry. |

The service harness verifies the identical 29 packets for a full FILE block at
ATT payload 20 are enqueued in **8 polls**, previously 29. At ATT payload 244,
the block still uses one poll. These are scheduler measurements under host
stubs, not measured RF throughput. Negotiated packet size, phone behavior and
radio conditions still determine real transfer speed. Connection interval and
PHY policy were not changed without hardware measurements.

The archive deadline covers verification starvation, blocked first fragments,
mid-message stalls and an EOF resume with zero outstanding blocks. Finishing
a verification scan resets the transfer deadline, so whole-file CRC work is
not deducted from the radio transfer budget. New local recording can still
cancel an archive immediately; it need not wait for expiry. Retired tokens return
`CANCELLED`; retrying with a higher token opens a new attempt.

### Matching SDK optimization

The app computes SHA-256 alongside CRC during a native full read from offset
zero. A successful read yields an in-memory proof bound to the connection,
recording ID, size and CRC. An unchanged complete catalog can retain that proof.
Cleanup reuses it only when the durable receipt's full raw SHA-256 and size
also match. Existing account/Ring identity, retained raw/WAV verification,
saved memo and final deletion checks still run.

This removes one full file transfer in that verified same-connection case.
It is not a measured radio-speed claim. Reconnects, partial resumes, legacy
files, changed metadata, failed writes/checkpoints and cancellations retain
the conservative full-read path. Exact native custody remains mandatory for
deletion. A resumed suffix never becomes a full-file proof by itself.

## Battery reporting

The driver publishes motor activity from power-up/settling through stop and
cleanup. Battery acquisition skips an active motor and the provisional 250 ms
settling period afterward. Any motor activity change during acquisition
invalidates the entire batch. This covers driver pulses and the legacy manual
vibration route; it does not block waiting for a quiet period. Invalid readings
remain unknown rather than becoming zero percent.

The initialized, trimmed voltage filter can recover after sustained valid
readings instead of permanently ratcheting downward while unplugged. Charging
phase changes still reset the history. Standard-board battery SAADC acquisition
uses 40 microseconds instead of 10; other channels/boards retain their settings.
The supplier voltage curve, charge voltage, cutoff and PMIC protections remain
unchanged. These changes reduce identifiable sampling/filter errors; they do
not calibrate state of charge or prove that physical motor proximity caused
the reported inaccuracies.

## Qualification still required

Record actual results on a recoverable spare Ring and iPhone:

- Time full download and cleanup separately, count transmitted bytes, and
  compare offset-zero, interrupted/resumed and reconnect cases. Confirm local
  WAV playback and exact contents before deletion, including full phone disk.
- Delay ACKs across retransmission, repeat duplicate ACK/RESUME requests, disable
  notifications and fill the send queue. Verify the 30-second stall cancellation,
  later Flash shutdown and immediate local capture access. Compare ATT payloads
  20 and 244 while measuring touch/STOP latency during transfer.
- Measure unloaded battery voltage and reported percentage before/during/after
  haptics, LED loads, BLE transfer and charging. Test across charge levels and
  temperature. Adjust the 250 ms quiet period only from measurements.
- Test keyboard insertion, changed fields/manual edits, app suspension,
  force-quit, disconnect and immediate new capture. Confirm local save remains
  reliable and no stale or repeated insertion cue occurs.
- Verify silent mode, touch escape and pulse distinguishability. Measure
  current draw and battery life; no runtime improvement has been measured yet.
- Reproduce with the supplier compiler, review GNU ABI warnings, demonstrate
  SWD recovery and supply signed OTA packaging before field updates.

## Validation

On September 6, 2026, the complete local firmware gate passed **14,330 C checks**
under ASan/UBSan and **six archive-normalizer tests**. This includes 1,454 service
checks and 463 integrated worker checks. The latter use real LittleFS with a
finite NOR model and confirm archive expiry followed by idle Flash shutdown,
retired-token rejection, intact retained identity/CRC/custody, and hold/release
processing within two worker passes when injected during packet bursts.
The worker result measures scheduling under stubs, not physical sensor latency.

The GNU ARM 15.2.rel1 build compiled and linked **225/225 sources**, with zero
undefined symbols and passing startup/vector, memory bounds and runtime-lock
checks. Local BIN: **311,452 bytes**, SHA-256
`f86ed4863af35c4e5cee7249f75838bfe137b82a26f5c72093a478ddfdbb5d69`.
Static RAM spans 203,968 bytes; the image reserves 8 KiB each for heap and MSP,
leaving 23,528 bytes between them. This is static fit, not runtime high-water
proof. Two supplier wchar ABI warnings and eight libnosys stub warnings remain
visible for supplier review. The release uses the independently checked Linux
CI artifact identified in its own provenance; local ELF/map hashes can differ.

Matching SDK validation passed all Ring unit tests and native preview,
connection/controller and epoch fault harnesses. The full simulator unit run
passed 597/598; the remaining test requires an unavailable Apple model runtime.
Cloud test, lint, typecheck and build gates passed. Physical radio, battery,
audio and keyboard qualification are still pending. No Ring was flashed.
