# Italic Ring — firmware, downloads and hardware

Firmware and hardware evidence for the **standard Bravechip 603V1.23.2 Ring**.
Current main firmware and primary release candidate: **6.0.3.3S04 / S04 RC1**.
S04 is an unsigned engineering prerelease for supplier bench testing; physical
qualification and signed OTA packaging remain pending. This release is firmware only.

**[Supplier change log: what changed from factory firmware and why](docs/reference/supplier-firmware-change-log.md)**
records additions, exclusions, refinements and verification status as work proceeds.

## Download S04 RC1 (current RC)

**[Release assets: v6.0.3.3S04-rc.1](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S04-rc.1)**

Start with **`italic-ring-603v1.23.2-6.0.3.3S04-rc.1-supplier-review.zip`**.
The bundle contains the application, debug files, checksums, build provenance,
supplier change log and instructions. It is an **unsigned engineering prerelease**
for supplier bench testing on a recoverable spare standard board.
The review ZIP is **not a signed OTA update package**.

**[Download, flash and test guide →](docs/how-to/test-s04-release-candidate.md)**

| What you need | Where to go |
| --- | --- |
| Download / identify each file / choose SWD or OTA | [S04 guide](docs/how-to/test-s04-release-candidate.md), [release assets](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S04-rc.1) |
| Review every change from factory and its rationale | [Supplier change log](docs/reference/supplier-firmware-change-log.md) |
| Build the firmware | [Candidate build guide](docs/reference/ring-firmware-candidate.md) |
| Review the published S03 iPhone SDK/client | [App PR #12](https://github.com/ShopItalic/app/pull/12), exact SDK commit [492af2a](https://github.com/ShopItalic/app/tree/492af2ad40949de3d54419df5e2fa140c912b94f) |
| Implement Bluetooth / settings / feedback | [Wire protocol](docs/reference/ring-voice-protocol.md) |
| Validate behavior and remaining physical work | [S03 reliability report](docs/reference/ring-s03-reliability.md), [acceptance matrix](docs/reference/ring-recording-and-ptt.md) |
| Identify components and supplier BOM | [Hardware reference](docs/reference/sudo-ring-hardware.md), [component BOM](docs/reference/sudo-ring/bom.csv), [quoted cost BOM](docs/reference/sudo-ring/quoted-bom.csv) |
| Find the original factory image | [Factory extraction guide](docs/reference/ring-firmware.md) |
| Reproduce the older S01 RC1 | [Unchanged S01 release](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S01-rc.1), [historical guide](docs/how-to/test-release-candidate.md) |

## Components

The documented component stack for standard **603V1.23.2** is below. The
processor and power/clock functions inside the SiP are included components,
not additional standalone parts.

| Component | Manufacturer / part | Role and specification |
| --- | --- | --- |
| Flexible PCBA | Bravechip `BCL603MHV1.23.2` (`BCL603M3` module family) | Four-layer flexible board, 52 × 6.5 mm. |
| Processor/radio SiP | Bravechip `BCL603S2P` | Contains the Nordic MCU/radio and integrated power, clock and support components. |
| MCU and Bluetooth radio | Nordic `nRF52840`, inside the SiP | 64 MHz Cortex-M4F, 256 KB RAM and 1 MB internal flash. |
| Microphone | ST `MP23DB01HP` | Digital PDM MEMS microphone; exact fitted ordering suffix requires confirmation. |
| Touch controller | Azoteq `IQS7211E` | Capacitive touch/proximity input over I²C. |
| Motion sensor | ST LSM6DSO family; supplier lists `LSM6DSOW` | Six-axis accelerometer and gyroscope; exact ordering suffix requires confirmation. |
| Recording storage | GigaDevice `GD25WQ128HQIGR` | 128 Mbit SPI NOR, equivalent to **16 MiB** raw storage. |
| Haptic motor | Supplier lists `LBM0518A4107F`; CAD baseline `0518` | PWM-driven actuator; fitted equivalence and electrical ratings remain to be confirmed. |
| Curved battery | Grepow `GRE170722_10` / `GRE170724` / `GRE170726` | CAD-selected cells for sizes **10 / 11 / 12**, respectively; one cell per ring. Exact capacity and charge ratings remain open. |
| Power management and clock | SiP-integrated PMIC/LDO/clock; firmware selects `YHM2712` | Exact PMIC ordering code and SiP/board allocation require supplier confirmation. |
| Indicators | LEDs and optical parts; exact parts unspecified | Recording/status feedback; fitted LED count and ordering codes remain open. |
| Antenna, protection and support parts | Exact parts unspecified | RF matching, motor drive, protection and board passives require the released electrical BOM. |

This summarizes supplier documents, firmware selections and CAD; it is not a
physical teardown or a complete purchasing BOM. See the [hardware reference](docs/reference/sudo-ring-hardware.md#electronic-bom)
and [component BOM](docs/reference/sudo-ring/bom.csv) for evidence, mechanical
and charging-case parts, and unresolved specifications.

## Features and refinements

- **Three mappable inputs.** Press-and-hold, double tap and triple tap each map
  to disabled, memo toggle or a live host event; hold can additionally map to
  PTT. No single-tap or swipe action is enabled.
- **Hold to record, release to stop.** Initial hold activation is configurable
  from 0.5–10 seconds, including 1, 2 and 5 seconds. PTT has no duration cap.
  Audio is stored on Ring whether connected or standalone. Optional live preview supports same-iPhone dictation; phone
  failure falls back to later archive sync.
- **Fresh defaults:** one-second hold → PTT, double tap → off, triple tap →
  memo toggle. Previous memo opt-out remains respected during migration.
  Mappings persist independently from lights/haptics. A hold mapped to PTT
  also stops an active memo; other hold mappings follow their selected action.
- **Bounded packet scheduling.** Up to four fragments of one message per
  worker pass, with the same wire bytes and one archive read/verification step.
  A full FILE block at ATT payload 20 takes 8 host-harness polls instead of 29;
  actual radio throughput remains to be measured.
- **Reliable transfer retries.** Delayed cumulative ACKs remain valid across
  rewind. A 30-second lack of real progress cancels abandoned archive work;
  retired tokens cannot reopen it. Source audio and custody remain intact.
- **S03 SDK download proof reuse (historical).** The published S03 client can
  avoid a second Bluetooth read after a verified full download when the
  connection, recording and durable receipt match. Partial resumes and
  reconnects retain full verification. S04 client adoption is a separate task.
- **Configurable lights, haptics and touch.** Persisted master settings mute
  normal application output, including manual cues. Strength, recording pulse
  durations and touch thresholds are configurable. Startup/bootloader output
  has a separate boundary.
- **Distinct phone confirmation.** Two short pulses require acknowledgment that
  the keyboard inserted the exact dictation. The ordinary stop cue means audio
  saved locally. Mute, a new capture or connection loss cancels pending cues.
- **More robust battery sampling.** Exclude motor activity and its settling
  interval, reject interrupted ADC batches, allow the filter to recover, and
  increase the standard-board battery acquisition time. Physical calibration
  and battery-life measurements remain outstanding.

The onboard codec is **IMA ADPCM, 8 kHz mono**, approximately 32 kbps.
Opus source is preserved in the supplier SDK but excluded from this target.
PTT runs until release; capture/touch faults, full storage or power loss can
still end capture. Memo/app recording keeps its separate optional duration limit.

Current software evidence is pinned to an exact source commit in the
[current S04 evidence table](docs/reference/ring-firmware-candidate.md#current-s04-evidence-and-controls).
That table records a verified `b75da242f6df0c133b4c8705af33b8a075a8a829`
baseline before the pending result-mapping follow-up; its measurements are not
the eventual combined-head results. The published S04 RC1 assets remain
unchanged at tag `v6.0.3.3S04-rc.1`, which points to source
`84c91fcbb2f2dec2514a8b79ce2908e1c7529fb8`; release-era provenance remains
attached to that immutable bundle. No Ring was flashed during preparation;
physical audio/radio/power qualification, supplier compiler/ABI review,
signing and proven recovery are still required.

## Current S04 release versus factory

| | S04 candidate | Preserved factory distribution |
| --- | --- | --- |
| Firmware readback | `6.0.3.3S04` | `6.0.3.3Z62` |
| Board | Standard `603V1.23.2` | Standard `603V1.23.2` |
| Files | S04 GitHub release assets | [`artifacts/ring-firmware/603v1.23.2-6.0.3.3z62`](artifacts/ring-firmware/603v1.23.2-6.0.3.3z62) |
| OTA status | Supplier must sign/package | Original signed package; applicability to a particular unit is unverified |

Do not substitute `1.23.2_one_sec`, another board, or the historical Nordic/Seeed
prototype. Factory files are a distribution, **not a backup of an individual
Ring**. This repository is public; retained supplier and third-party license terms apply.

## Repository map

`main` contains S04 firmware, tests and current documentation. Each release tag
preserves its exact source and assets. [S03 RC1](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S03-rc.1)
and S01 RC1 remain available unchanged for historical comparison.

| Location | Contents |
| --- | --- |
| [`docs/`](docs/README.md) | Documentation index, download guide, hardware and factory evidence |
| [`docs/reference/sudo-ring/`](docs/reference/sudo-ring) | BOM CSVs, workbook extracts and source manifests |
| [`artifacts/ring-firmware/`](artifacts/ring-firmware) | Preserved factory artifacts, hashes and notices |
| [`firmware/`](firmware) | Candidate/vendor source; `bc_ros` owns application logic, `BCL603S2X` board/SDK support |
| [`tools/firmware/`](tools/firmware) | Baseline verification, profile generator, GNU build and host test entry points |
| [`tests/firmware/`](tests/firmware) | Production-code fault harnesses |
| [`build/firmware/gnu/sudo_voice/`](https://github.com/ShopItalic/sudo/blob/main/docs/reference/ring-firmware-candidate.md) | Local generated output after building; not checked into Git |

The iOS/web/backend client lives in [ShopItalic/app](https://github.com/ShopItalic/app),
Caption firmware in [ShopItalic/caption](https://github.com/ShopItalic/caption),
and commerce in [ShopItalic/store](https://github.com/ShopItalic/store).
`botnetai/ring-firmware` is an older test-unit prototype, not this production Ring.
