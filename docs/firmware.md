# Production firmware baseline

## Bluetooth reconnect recording fix — P11-r3 and P11-r4, September 16, 2026

Both P11 recorder variants receive new revisions with the offline-stop command
removed from the Bluetooth connected callback:

| P11 revision | Recorder | Charging lights | Reconnect fix | Relationship |
| --- | --- | --- | --- | --- |
| r1 | Factory ADPCM | Yes | No | Earlier recipe, unchanged |
| r2 | Opus | Yes | No | Earlier recipe, unchanged |
| r3 | Factory ADPCM | Yes | Yes | New replacement for r1 |
| r4 | Opus | Yes | Yes | New replacement for r2 |

This gives four P11 revisions and two current development paths. Push-to-talk
and gesture memos continue across disconnect/reconnect; release, deliberate
stop commands, and sensor-fault protections retain their existing behavior.
Package revision is required to distinguish these candidates from their parents
with the same wire version. A separate P10-r4 recipe retains the reconnect fix
as a P10 fallback; it is not another P11 variant.

Prepare a fresh source tree with:

```sh
python3 tools/firmware/prepare_factory_ptt_v10_r4.py --output <fresh-p10-path>
python3 tools/firmware/prepare_factory_ptt_v11_r3.py --output <fresh-p11-adpcm-path>
python3 tools/firmware/prepare_factory_ptt_v11_r4.py --output <fresh-p11-opus-path>
```

After an ArmCC5 build through the established
[qualification workflow](how-to/qualify-firmware.md), run the matching verifier
using the qualification Python environment (with pyelftools and Unicorn):

```sh
python tools/firmware/verify_factory_ptt_v10_r4.py --source <p10-source> --evidence <p10-compiler-evidence> --output <new-p10-verification.json>
python tools/firmware/verify_factory_ptt_v11_r3.py --source <p11-adpcm-source> --evidence <p11-adpcm-compiler-evidence> --output <new-p11-adpcm-verification.json>
python tools/firmware/verify_factory_ptt_v11_r4.py --source <p11-opus-source> --evidence <p11-opus-compiler-evidence> --output <new-p11-opus-verification.json>
```

The regression test compiles actual generated BLE callbacks and the PTT state
machine. All 1,204 runtime checks pass across P10-r4, P11-r3, P11-r4, and the
diagnostic ADPCM pipeline control. P11-r3 uses the original factory recorder,
not that diagnostic control. The released P10-r2 negative control detects the
original bug. The full firmware host suite also passes with this final revision
mapping. Both new revisions have now been built with licensed ArmCC5, signed,
and published: [P11-r3](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3P11-r3)
(187,340 bytes) and [P11-r4](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3P11-r4)
(337,704 bytes). Their public browser and asset-API downloads match the
qualified binaries, package digests, lengths and verified signatures.
Source/object identity, static stack checks and 138 exact ARM retirement probes
passed for each image; r4 also passed the Opus pipeline CPU probes.

All four revisions are offered by the new schema-2
[`releases-v2.json`](../tools/firmware-hosting/releases-v2.json) catalog. The
matching app keys selection and installation confirmation by version and
revision, validates the release tag and digest, and admits only Opus r2/r4 on
a Ring already reporting P11. The legacy `releases.json` remains unchanged.
The separate Dex task installed and launched the matching shared app build on
Jeremy iPhone. This release task did not install an app or flash a Ring.
Physical recording, transfer, power and recovery qualification remain open.
See the [investigation and fix evidence](reference/ptt-bluetooth-interruption-investigation.html).


## P10-r3 / P11-r1 / P11-r2 downloads — September 16, 2026

All four retirements are implemented in these **new** application-only
sample-test prereleases: BLE/TCP speed tests, motion sampling/tasks, swipe
output, and automatic duration-based recording deletion. Hold/release and
optional double/triple-tap recording, saved-file transfer and explicit
receipt-controlled cleanup remain. Old releases have not been overwritten.

| Release | Codec | Charging light | Signed ZIP bytes |
| --- | --- | --- | ---: |
| [P10-r3](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3P10-r3) | Factory ADPCM | No addition | 187,188 |
| [P11-r1](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3P11-r1) | Factory ADPCM | Amber charging / green full | 187,516 |
| [P11-r2](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3P11-r2) | Opus | Amber charging / green full | 337,868 |

Each recipe independently uses released **P10 revision 2** as its immutable
parent; see `firmware/factory_ptt_v11/base-lock.json`. Source and release tags
are pinned to `31ddf23abe6fd1caec742f82cc7b40bb4f6dd960`. Licensed ArmCC5 builds,
source/object/load identity, static stack budgets, host fault tests and exact
ARM CPU probes passed. All three public ZIPs passed independent signature,
digest and length checks through the browser and asset-API routes.
[Main-source CI](https://github.com/ShopItalic/sudo/actions/runs/35070912196)
passed; its GNU artifact is not a distribution image.

The GitHub links can be used from iPhone Safari. The legacy catalog offers
P10-r3 to earlier clients. P11-r1/r2 were initially staged outside that catalog;
the compatible schema-2 catalog now includes all four P11 revisions as
described above. Their base-version readback cannot identify the installed
package revision or establish whether an ADPCM downgrade would be safe.

No Ring was flashed. Physical boot, recovery, audio timing, dynamic memory,
charging behavior, sensor power-off and recording custody still need testing
on an identified recoverable spare. CPU probes do not establish these results.
See [hosting details and exact manifests](../tools/firmware-hosting/README.md#p10-r3-and-staged-p11-downloads-2026-09-16).

**September 10 software result:** the restored ArmCC 5.06u7 build 960 supplier
baseline exactly matches the factory application. The repaired Sudo candidate
retains recording and Opus, builds with zero errors, and passes the full host
suite plus 33 linked ARM scenarios. The
[software qualification report](reference/ring-armcc5-qualification.html)
records the runtime fixes, all task/interrupt budgets, compiler/map/input/output
hashes and explicit physical limits. No signing or flashing was performed.

**September 9 update:** main includes experimental S05 source. A physical S05
update failed post-transfer connection/version verification; recovery is still
unresolved. Follow the [recovery, supplier build and physical qualification workflow](how-to/qualify-firmware.md)
before another installation. The S04 measurements below are historical.

The preceding **6.0.3.3S04** source introduced PTT until release, three mappable inputs,
configurable hold activation, persisted light/haptic controls and conservative cleanup. S04 RC1 is the current unsigned engineering prerelease; S03 RC1 remains historical. See the [S04 change ledger](reference/supplier-firmware-change-log.md#s04-controls-and-cleanup--september-7-2026).
Originally developed on `codex/opus-audio`, the experimental **6.0.3.3S05** Opus recording
source is now on main: pinned libopus 1.6.1 under `firmware/bc_ros/bc_module/opus/opus-1.6.1`
(verified by `tools/firmware/import_opus.py --verify`), a 16 kHz Opus capture
path, metadata version 2 with a per-recording descriptor and the Sudo Opus
container. See the [S05 ledger](reference/supplier-firmware-change-log.md#s05-opus-recording--september-8-2026).

The source task inspected `Firmware/1.23.2_6033固件SDK.zip` in the Sudo Google
Drive folder on the MBA. Its source board selection agrees with the production
Ring baseline for `603V1.23.2`. The recovered [raw factory firmware
extraction](reference/ring-firmware.md) includes the 184,132-byte
`application.bin`, the combined `BCL603S2P_6.0.3.3Z62.hex`, the application-only
OTA ZIP, and seven exact memory regions. SHA-256, ZIP CRC, HEX consistency, DFU
metadata, and ECDSA signature checks passed against the saved factory
distribution.

The reviewed SDK is imported under `firmware/`, with its unmodified source
baseline saved at commit `102bfd2`. The factory distribution is exactly
`6.0.3.3Z62`. The separate Sudo Voice engineering profile is `6.0.3.3S04`, a
ten-byte build string plus NUL in the legacy version field; it is not a factory
version, signed release, or anti-rollback-approved image. The matching iOS
adapter is [ShopItalic/app](https://github.com/ShopItalic/app).

## Implemented source tranche

The current firmware source provides:

- A single worker-owned recording/archive lifecycle. The capture adapter uses
  the standard 1.23.2 mono PDM path, an eight-slot bounded queue, and a
  completed-block tail guard. Stop drains complete blocks after capture is
  quiesced and never fabricates an unfinished DMA tail.
- A bounded LittleFS store using the vendor `lfs.c` and `lfs_util.c` directly.
  Raw ADPCM lives at `/.sudo-rec/<16 lowercase hex recording ID>.raw`; the
  binary ID path is validated and the legacy export filename remains metadata.
  Checksummed/versioned attributes on the same audio file carry the original
  Start parameters, durable bytes and frames, raw CRC-32/ISO-HDLC, completion
  and recovery flags, and the NUL-terminated legacy name.
- Atomic metadata checkpoints and honest failure handling. Append advances
  counts only for bytes accepted by LittleFS; a failed write or sync leaves the
  last durable prefix available. Normal successful finish uses the running CRC,
  commits and verifies metadata/counts/file length, and then marks completion.
  It does not rescan the complete raw file. The staged archive reader performs
  the full raw CRC in bounded `reader_verify_step` work before exposing reads;
  read-only recovery marks partial records as recovered and never repairs or
  truncates their original data.
- Versioned native voice operations for Start, Stop, state/query, live data,
  catalog, resume, transfer, receipt, cancellation, settings, and touch
  tuning. Custody deletion requires a nonzero exact byte count and matching CRC,
  a persisted checked receipt tombstone, and a terminal complete/recovered
  record; raw removal remains retryable and tombstones are retained.
- PTT until release, optional memo limits, three independently mapped inputs,
  adjustable hold activation, checked ADC/power errors, a battery filter with
  charging-phase resets and recovery, and persisted light/haptic controls.
  Fresh defaults are one-second hold to PTT, double tap off and triple tap to
  memo toggle. Host tests cover these paths; sensor calibration, physical
  ranges, power draw and device behavior remain open.

All public storage calls are serialized by the recording/archive worker, and the
store receives its already mounted shared `lfs_t` from the platform. The store
uses a 256-byte file cache/read buffer and has no platform or FreeRTOS
requirement.

## Evidence to retain on import

| Area | Archive locator recorded by the hardware task |
| --- | --- |
| Board selection | `bc_ros/bc_config/ring_config.h` |
| Flash configuration | `bc_ros/bc_module/spi_flash/sfud/inc/sfud_cfg.h`, `sfud_flash_def.h` |
| Touch | `bc_ros/bc_device/touch_button/IQS7211E` |
| Microphone | `bc_ros/bc_module/pdm/bc_pdm.h` |
| PMIC | `bc_ros/bc_module/pmic/bc_pmic.c`, `bc_device/yhm2712` |
| Power transitions | `bc_power.c` |
| Haptics | `app_linear_motor_handler.c`, `bc_linear_motor.c` |
| Nordic SDK configuration | `BCL603S2X/app/user/inc/sdk_config.h` |

The complete [factory firmware evidence](reference/sudo-ring-hardware.md#factory-firmware-evidence)
records interface details and limitations. The [component BOM](reference/sudo-ring/bom.csv),
[quoted cost BOM](reference/sudo-ring/quoted-bom.csv), [workbook extracts](reference/sudo-ring/source-extracts.json),
and [source manifest](reference/sudo-ring/sources.json) preserve the related hardware
provenance. Preserve the vendor's filenames, copyright notices, licenses, build
projects, linker scripts, bootloader, and SoftDevice requirements. The reviewed
factory images are preserved under `artifacts/ring-firmware`; keep new local
build output and device-specific provisioning material out of Git. Do not
replace the supplier pin map or adapt the older Nordic prototype by assumption.

## Current S04 validation boundary

Current source passes **17,006 checks across 24 C suites**, six archive-normalizer
tests and both BLE event-routing harness builds. The GNU build compiles/links
**225 objects with zero undefined symbols**. The [current S04 evidence
table](reference/ring-firmware-candidate.md#current-s04-evidence-and-controls)
pins source `c002956`, the measured local BIN and its checksum.
`sh tools/firmware/test.sh` remains the host validation entry point.
Host assertions, archive normalization, BLE routing harnesses and the GNU
link do not establish physical Ring behavior.

S04 RC1 is refreshed at the existing tag `v6.0.3.3S04-rc.1` with the merged
reliability fixes. The package's `provenance.json` records its exact main
source, successful CI run and artifact hashes. The original `84c91fc`
bundle is superseded; the firmware readback and RC number remain unchanged. Vendor Arm Compiler 5.06
update 7 (build 960) reproduction, physical stack/heap high-water
measurements, signing/package and recovery review were pending at that S04
review. The subsequent S05 physical update failed acceptance as recorded above.

### Historical S03 validation boundary

The recording, capture, protocol, touch-tuning, battery, and LittleFS fault tests
have recorded sanitizer-backed host runs, including RAM-NOR erase/program
semantics and bounded power-cut cases. The S03 validation passed 14,330 C checks
across 24 suites and six archive-normalizer tests; baseline verification matched
all 7,056 original files. See the [S03 reliability report](reference/ring-s03-reliability.md)
for the S03 validation and [final audit](reference/ring-s03-final-audit.html)
for the feature inventory and remaining gates.

The integrated Arm GNU 15.2.rel1 target compiled and linked all 225 sources with
zero undefined symbols and passing startup/vector checks. The local S03 load
image was 311,452 bytes; static RAM was 203,968 bytes plus separate 8 KiB C-heap
and 8 KiB main-stack reservations. The released Linux image has its own hashes
and sizes in the [release guide](how-to/test-s03-release-candidate.md). These
figures are historical S03 evidence, not current S04 measurements.
S03 RC1 is published as an unsigned engineering prerelease.

## Remaining build and hardware gates

1. Verify the saved source baseline with `tools/firmware/verify_baseline.py` and
   review the candidate diff from `102bfd2`.
2. Reproduce the unmodified vendor build and the candidate with recorded tool
   versions, commands, output hashes and memory use. The software reproduction
   and checks completed September 10; supplier/physical acceptance remains open.
3. Read back a correctly identified physical board. Verify the nominal
   16.125 kHz PDM rate and, on S04, the ADPCM framing against the app's 8 kHz
   mono interpretation; on the S05 branch verify the 16 kHz Opus resampling
   ratio and encode timing through FORMAT_GET. Neither establishes the fitted
   firmware contract without a physical readback.
4. Verify live streaming and stored-file transfer, interruption/resume,
   battery and charging states, touch, motion, haptics, and recording cleanup
   through the current app adapter.
5. Validate the upgrade path and recovery on the correct hardware before
   publishing a firmware release.

The factory extraction remains unchanged. The GNU build is an engineering
candidate; source checks and a successful link do not establish physical behavior.
