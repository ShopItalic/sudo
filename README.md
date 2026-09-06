# Italic Ring — firmware, downloads and hardware

Firmware and hardware evidence for the **standard Bravechip 603V1.23.2 Ring**.

## Download RC1

**[Open the RC1 release and download its assets](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S01-rc.1)**

Start with **`italic-ring-603v1.23.2-6.0.3.3S01-rc.1-supplier-review.zip`**.
It contains the application, debug files, checksums and instructions. This is
an **unsigned engineering prerelease**, for supplier bench testing on a spare
standard board. The review ZIP is **not an OTA update package**.

**[Download, flash and test guide →](docs/how-to/test-release-candidate.md)**

| What you need | Where to go |
| --- | --- |
| Download the candidate | [RC1 release assets](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S01-rc.1) |
| Understand each file and choose SWD or OTA | [Flashing and testing guide](docs/how-to/test-release-candidate.md) |
| Browse/build the exact candidate source | [RC1 source tag](https://github.com/ShopItalic/sudo/tree/v6.0.3.3S01-rc.1), [build instructions](https://github.com/ShopItalic/sudo/blob/v6.0.3.3S01-rc.1/docs/reference/ring-firmware-candidate.md#build-profile-and-toolchain) |
| Review the firmware changes | [Firmware PR #1](https://github.com/ShopItalic/sudo/pull/1) |
| Build the matching iPhone app | [App PR #10](https://github.com/ShopItalic/app/pull/10), [client contract](https://github.com/ShopItalic/app/blob/064c265356e32ea9819a8eaa57929ebc17f66f0a/docs/reference/ring-voice-client.md) |
| Check features and acceptance tests | [Requirements matrix](https://github.com/ShopItalic/sudo/blob/v6.0.3.3S01-rc.1/docs/reference/ring-recording-and-ptt.md) |
| Implement the BLE protocol | [Native wire protocol](https://github.com/ShopItalic/sudo/blob/v6.0.3.3S01-rc.1/docs/reference/ring-voice-protocol.md) |
| Identify components / supplier BOM | [Hardware reference](docs/reference/sudo-ring-hardware.md), [component BOM](docs/reference/sudo-ring/bom.csv), [quoted cost BOM](docs/reference/sudo-ring/quoted-bom.csv) |
| Find the preserved factory image | [Factory extraction guide](docs/reference/ring-firmware.md) |

## What's in RC1?

- Hold to record, release to stop; configurable double-tap memos.
- Complete Flash recording while connected or standalone, with checked stop,
  recovery and explicit final-file results.
- Resumable Bluetooth transfer, startup audio buffering, and deletion only
  after verified durable custody.
- Persisted touch, LED and haptic settings; checked battery and Flash I/O.
- Matching iPhone client with native state/settings/sync and bounded PTT
  dictation through the existing Sudo keyboard.

Default PTT is ten seconds, configurable; memos are unlimited by default.
The candidate retains the existing 8 kHz mono vendor ADPCM contract.

**Verified:** 13,174 firmware checks, six archive-normalizer tests, 225/225 ARM
sources compiled and linked, and [green CI on the released source](https://github.com/ShopItalic/sudo/actions/runs/33998477755).
The matching app passes 267 selected Ring tests plus production fault harnesses.
**Still pending:** physical audio/radio/power/keyboard qualification, supplier
compiler/ABI review, signing and demonstrated update recovery. No Ring was
flashed during preparation. One broader iOS simulator test needs an unavailable
Apple model; details are in the app PR.

## Candidate versus factory

| | RC1 candidate | Preserved factory distribution |
| --- | --- | --- |
| Firmware readback | `6.0.3.3S01` | `6.0.3.3Z62` |
| Board | Standard `603V1.23.2` | Standard `603V1.23.2` |
| Purpose | Test the new recording and reliability behavior | Original baseline and supplier recovery analysis |
| Files | GitHub release assets | [`artifacts/ring-firmware/603v1.23.2-6.0.3.3z62`](artifacts/ring-firmware/603v1.23.2-6.0.3.3z62) |
| OTA status | Supplier must sign/package | Original signed package; applicability to a particular unit is unverified |

Do not use `1.23.2_one_sec`, another board, or the historical Nordic/Seeed test
prototype as a substitute. Factory files are a distribution, **not a backup of
an individual Ring**. The repository is private; testers need GitHub access.

## Repository map

The default branch is the download/documentation front door. The RC tag contains
all candidate source; the full firmware change remains in PR #1 for review.

| Location | Contents |
| --- | --- |
| [`docs/`](docs/README.md) | Documentation index, download guide, hardware and factory evidence |
| [`docs/reference/sudo-ring/`](docs/reference/sudo-ring) | BOM CSVs, workbook extracts and source manifests |
| [`artifacts/ring-firmware/`](artifacts/ring-firmware) | Preserved factory artifacts, hashes and notices |
| [`firmware/` on RC1](https://github.com/ShopItalic/sudo/tree/v6.0.3.3S01-rc.1/firmware) | Candidate/vendor source; `bc_ros` owns application logic, `BCL603S2X` board/SDK support |
| [`tools/firmware/` on RC1](https://github.com/ShopItalic/sudo/tree/v6.0.3.3S01-rc.1/tools/firmware) | Baseline verification, profile generator, GNU build and host test entry points |
| [`tests/firmware/` on RC1](https://github.com/ShopItalic/sudo/tree/v6.0.3.3S01-rc.1/tests/firmware) | Production-code fault harnesses |
| [`build/firmware/gnu/sudo_voice/`](https://github.com/ShopItalic/sudo/blob/v6.0.3.3S01-rc.1/docs/reference/ring-firmware-candidate.md) | Local generated output after building; not checked into Git |

The iOS/web/backend client lives in [ShopItalic/app](https://github.com/ShopItalic/app),
Caption firmware in [ShopItalic/caption](https://github.com/ShopItalic/caption),
and commerce in [ShopItalic/store](https://github.com/ShopItalic/store).
`botnetai/ring-firmware` is an older test-unit prototype, not this production Ring.
