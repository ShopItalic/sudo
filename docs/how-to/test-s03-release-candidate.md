# S03 RC1: download, flash and test

This guide describes **S03 RC1**, an unsigned engineering candidate for supplier
bench testing. The package's `provenance.json` identifies the exact firmware
source SHA, CI run, matching SDK and asset hashes.

**Release:** [v6.0.3.3S03-rc.1](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S03-rc.1)

**Target:** standard Bravechip `603V1.23.2` / Nordic nRF52840

**Firmware readback:** `6.0.3.3S03`

**Firmware source and CI:** use `sourceSHA` and
`CIRunURL` from the S03 package's `provenance.json`. Do not copy the S01 RC1
source or CI values into this guide or a test report.

**Matching app SDK:** commit
[`492af2ad40949de3d54419df5e2fa140c912b94f`](https://github.com/ShopItalic/app/commit/492af2ad40949de3d54419df5e2fa140c912b94f)
now merged into `ShopItalic/app` main through [PR #12](https://github.com/ShopItalic/app/pull/12).
Pin this exact commit when building the app.

This is a supplier bench-test candidate. The review bundle is an
unsigned application-only package. It is not a signed RC OTA image, and no
hardware qualification is included. Use a spare standard board with a known
recovery procedure. The package does not contain a signed OTA image,
replacement bootloader, SoftDevice, UICR, per-unit backup, or pre-generated
bootloader settings.

**供应商快速说明：** 这是 S03 工程测试资料包，不是可直接通过手机升级的
OTA 包。仅适用于标准 `603V1.23.2`，不是 `1.23.2_one_sec`。请使用已确认可恢复的备用样机，
通过贵方 SWD 工装和匹配的 bootloader settings 测试，或由贵方签名生成正式 OTA 包。
刷写后版本应为 `6.0.3.3S03`。双击录音在新设置中默认关闭，只有在 app 中打开后才会开始免提录音；
再次双击或按住可停止，松开后再重新按住开始 PTT。请验证 BLE 传输、30 秒无进展取消、
键盘插入结果和电池读数，并返回版本读数、日志、音频、计时和测试结果。

## 1. Download the right files

Open the [S03 release Assets section](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S03-rc.1) and download
`italic-ring-603v1.23.2-6.0.3.3S03-rc.1-supplier-review.zip`. GitHub's automatic **Source code
(zip/tar.gz)** downloads contain source, not prebuilt release assets. This
repository is private: a 404 usually means the GitHub account needs repository
access or sign-in.

With GitHub CLI, download all assets into a new directory:

```sh
gh auth status
gh release download v6.0.3.3S03-rc.1 --repo ShopItalic/sudo --dir ring-rc-s03
cd ring-rc-s03
shasum -a 256 -c SHA256SUMS          # macOS
# Linux equivalent: sha256sum -c SHA256SUMS
unzip italic-ring-603v1.23.2-6.0.3.3S03-rc.1-supplier-review.zip -d unpacked
```

For a browser download of the ZIP alone, compare its SHA-256 against its line
in the release's `SHA256SUMS`. Windows PowerShell can print it with
`Get-FileHash -Algorithm SHA256 <downloaded-zip>`. The unpacked bundle also has
its own `SHA256SUMS` for all included files; verify it from that directory.

| File | Meaning / intended user |
| --- | --- |
| `italic-ring-603v1.23.2-6.0.3.3S03-rc.1-supplier-review.zip` | Complete review folder with instructions, images, provenance, checksums and notices. **Not a Nordic DFU ZIP or OTA package.** |
| `italic-ring-603v1.23.2-6.0.3.3S03-rc.1-*.hex` | Application-only Intel HEX with addresses. Preferred application input for a supplier SWD programmer. |
| `italic-ring-603v1.23.2-6.0.3.3S03-rc.1-*.bin` | Raw application bytes; base address **`0x00027000`**, never address zero. For analysis or a supplier packager that requires BIN. |
| `italic-ring-603v1.23.2-6.0.3.3S03-rc.1-*.elf` | Exact CI-linked application with debug symbols, for J-Link/GDB and fault diagnosis. |
| `italic-ring-603v1.23.2-6.0.3.3S03-rc.1-*.map` | Linker map for memory and symbol review. |
| `ci-build-summary.json` | CI build, startup, memory, ABI and image-hash evidence. Its original paths and names are CI paths; release names are mapped by `provenance.json`. |
| `provenance.json` | S03 source SHA, CI identity, matching app commit, asset hashes and application-only boundaries. Read this file for final source and CI values. |
| `SHA256SUMS` | Integrity checks; a checksum is not an OTA signature. |
| `README.md`, `FLASHING.md`, `TEST-REPORT.md`, `licenses/` | Offline starting point, this procedure, a results template and preserved notices. |

The published S03 HEX must contain only application flash within
`[0x00027000, 0x000E0000)`. It does not initialize an erased nRF52840 by
itself. The retained baseline is S140 **7.2.0**, firmware ID **0x0100**, factory
bootloader at **0xF8000**, settings backup at **0xFE000** and primary settings at
**0xFF000**. These describe the recovered distribution; read back and confirm
the actual test unit.

## 2. Select the update path

| Situation | Procedure |
| --- | --- |
| Supplier has confirmed SWD pads, working probe and a recoverable spare board | Follow the SWD bench procedure below, including matching settings. |
| Ring can only update over BLE through its factory bootloader | Supplier must sign and package the RC with the accepted key and version policy; see OTA preparation below. |
| No verified pads, debug access, bootloader identity or recovery route | Review, build or emulate first. A downloadable application does not establish a flash procedure for that unit. |

### SWD bench procedure

Use the supplier's confirmed SWDIO/SWDCLK/GND/reference-voltage/reset wiring,
power arrangement and SEGGER J-Link setup. The repository does not establish
the physical test-point pin map. The examples below use the supplier-compatible
legacy `nrfjprog` CLI; record its version and check its local help before use.
These commands are **templates for the supplier**, not operations performed by
this project.

1. Identify the physical board and one probe explicitly. Stop existing capture
   and synchronize recordings before a debugger halts or resets the device.
2. Read and preserve internal flash/UICR before writing. Keep device dumps
   private: they can contain identity, bonds or calibration. The external
   **16 MiB SPI recording Flash is not included** in `--readcode`; preserve it
   using the supplier's supported reader. Generic QSPI commands are not a
   substitute for the fitted SPI2 wiring.

```sh
nrfjprog --version
nrfjprog --ids
# Replace PROBE_SERIAL with the confirmed J-Link serial number.
nrfjprog --family NRF52 --snr PROBE_SERIAL --deviceversion
nrfjprog --family NRF52 --snr PROBE_SERIAL --halt
nrfjprog --family NRF52 --snr PROBE_SERIAL --readcode unit-before.hex
nrfjprog --family NRF52 --snr PROBE_SERIAL --readuicr unit-uicr-before.hex
```

If access protection blocks reads, obtain the supplier's recovery decision;
`--recover` erases device contents. Do not turn a failed backup into an erase.
The flash, erase and verification meanings above are documented in
[Nordic's nrfjprog reference](https://docs.nordicsemi.com/r/bundle/ug_nrf_cltools/page/ug/cltools/nrf_nrfjprogexe_reference.html).

3. **Prepare matching bootloader settings.** The old factory settings contain
   the old application's size and CRC. Writing only the new application can
   leave the factory bootloader refusing to boot it. The supplier must confirm
   its actual settings format, version and anti-rollback policy, backup-page
   handling and boot validation. The historical standard-board script uses
   settings version **1**. This is its legacy command form, with release
   counters deliberately left for the supplier:

```sh
nrfutil settings generate --family NRF52840 \
  --application italic-ring-603v1.23.2-6.0.3.3S03-rc.1.hex \
  --application-version APPROVED_APP_COUNTER \
  --bootloader-version CONFIRMED_BOOTLOADER_COUNTER \
  --bl-settings-version 1 supplier-approved-settings.hex
```

The old Python `nrfutil` syntax is not interchangeable with an arbitrary modern
installation. Use the supplier's working toolchain or its documented
`nrf5sdk-tools` equivalent. Inspect the generated settings and verify
application size and CRC, settings CRC and both expected settings pages before
programming. Do not reuse `application.dat` or settings from `6.0.3.3Z62`. See
the retained [factory script](../../firmware/BCL603S2X/dfu/ota_bat/creat_1232_dfu.bat)
and [Nordic settings inspection guide](https://docs.nordicsemi.com/r/bundle/nrfutil/page/nrfutil-nrf5sdk-tools/guides/generating_bootloader_settings.html/displaying-bootloader-settings).

4. After reviewing the application and settings address ranges, program only
   the application and approved settings pages, verify both, then reset:

```sh
nrfjprog --family NRF52 --snr PROBE_SERIAL \
  --program italic-ring-603v1.23.2-6.0.3.3S03-rc.1.hex --sectorerase --verify
nrfjprog --family NRF52 --snr PROBE_SERIAL \
  --program supplier-approved-settings.hex --sectorerase --verify
nrfjprog --family NRF52 --snr PROBE_SERIAL --reset
```

These templates intentionally do not replace the SoftDevice, bootloader or
UICR. Do not add chip erase, UICR erase or external-Flash erase flags. A settings
file with unexpected address ranges must be rejected before these commands.
No package here is presented as a universal one-click recovery image.

### BLE OTA preparation: supplier signing required

The existing factory package uses ECDSA P-256/SHA-256, hardware family value
**52** and SoftDevice requirement **0x0100**. That generic hardware value does
not identify standard `603V1.23.2` by itself. The supplier must confirm the
installed bootloader's accepted key, target identity and release counters.

The legacy packaging template is:

```sh
nrfutil pkg generate \
  --application italic-ring-603v1.23.2-6.0.3.3S03-rc.1.hex \
  --application-version APPROVED_APP_COUNTER \
  --hw-version 52 --sd-req 0x0100 \
  --key-file SUPPLIER_CONTROLLED_SIGNING_KEY \
  italic-ring-603v1.23.2-6.0.3.3S03-rc.1-supplier-signed-ota.zip
```

Keep the private key with the supplier. No signing key is needed for
downloading or reviewing the unsigned S03 RC. Verify the generated init
packet and signature, then use the supplier's supported BLE updater on the
verified spare board. Entering DFU, anti-rollback and interrupted-update
recovery still need supplier confirmation. Renaming the review ZIP or combining
the new BIN with the factory DAT will not produce a valid signed RC update.

## 3. Use the matching app and test

Build the matching app SDK at commit
**`492af2ad40949de3d54419df5e2fa140c912b94f`**. It is now in the app
repository's `main` through PR #12, but the build must remain pinned to this
exact commit. Use the app repository's Xcode setup and the supplier's normal development signing. An
installable IPA or TestFlight build is not included in this firmware package.
The native adapter requires standard `603V1.23.2`, exact `6.0.3.3S03` readback
and a successful HELLO capability exchange. A store app or factory-only client
must not be assumed to speak the S03 protocol.

Record board ID, firmware readback, programmed image SHA-256, app commit,
probe and tool versions, the starting file list and configuration. Record the
firmware source SHA, CI URL and artifact identities from the packaged
`provenance.json`. Then run:

| Test | Pass condition |
| --- | --- |
| First boot | Readback is `6.0.3.3S03`; there is no reset loop; BLE and HELLO work; the board is the standard `603V1.23.2` variant. |
| Fresh settings and double tap | A fresh settings record has double-tap recording off. With it off, hold/release PTT and explicit app Start/Stop remain available. Turn it on in the app, double tap to start hands-free recording, then double tap again or hold to stop. Release before starting PTT again. Existing persisted choices survive a restart. |
| Hold/release PTT | Connected and standalone recording work. Measure release-to-microphone-stop within one second, and confirm feedback agrees with the saved or failed state. |
| S03 phone outcome | With the Sudo keyboard visible in another app, a complete current recording with the matching READY token on the same connection can produce one insertion outcome within ten seconds. Two 80 ms pulses occur only for the accepted keyboard insertion. Reconnects, new captures, wrong IDs or tokens, partial files, expired requests, missing keyboards and rejected text produce no insertion cue. |
| S03 fragment scheduling | Capture host scheduler evidence for a full FILE block: 29 packets at ATT payload 20 in 8 polls, and one poll at ATT payload 244. Treat these as host-stub measurements, not radio throughput. On hardware, record actual bytes, intervals and audio timing separately. |
| Delayed ACK retry | Delay cumulative ACKs across retransmission. The six sent block boundaries remain valid, a delayed valid cumulative ACK can advance, the acknowledged offset never regresses, and replay does not allocate duplicate window slots. |
| 30-second transfer stall | Block verification progress and advancing valid ACKs. The archive closes and the transfer cancels after 30 seconds. Duplicate ACKs, retries and same-token RESUME do not renew the deadline. The source recording and custody state remain available, and a later higher-token resume can retry. A new local recording can start immediately. |
| Interrupted download and cleanup | Offset-zero native reads verify raw CRC and SHA-256 before a receipt. In the same connection, cleanup may reuse a full-read proof only when recording ID, size, CRC and durable raw SHA-256 still match and all existing custody guards pass. Reconnects, partial resumes, legacy files, changed metadata, failed writes or checkpoints and cancellation take the full reread path. Failed phone storage leaves the Ring copy. |
| Initial live audio | First words survive the normal READY delay. Overflow and gaps fall back to stored audio without inventing a complete live transcript. |
| Settings, lights and haptics | SETTINGS and TUNING changes persist and read back exactly. Active recording or held contact returns BUSY. S03 light and haptic switches control normal application feedback after settings load; bootloader behavior is outside this test. Haptic strength and start/stop durations remain separately configurable. |
| Battery and charging | Measure voltage and percentage before, during and after motor activity, LED load, BLE transfer and charging. Acquisition skips an active motor and the provisional 250 ms settling period; a motor change invalidates the batch; invalid readings remain unknown. Charging resets the filter history. |
| App death and BLE loss | Local capture and finalization continue. Reconnect lists and retrieves the correct recording identity without accidental restart or stop. |
| iPhone keyboard | A short PTT inserts one complete final into the visible field. Field change, manual edit, expired admission, missing model or app suspension prevents stale insertion while stored audio remains available. |
| Abrupt power and full storage | On the spare, cut power across open, write, checkpoint, finalize, receipt and delete boundaries. Recover verified prefixes, mark partial records honestly and preserve older unsynced files. |

The S03 archive deadline covers verification starvation, blocked first fragments,
mid-message stalls and an EOF resume with no outstanding blocks. Finishing a
verification scan resets the deadline. The app's native full-read proof is
in-memory and bound to the connection, recording ID, size and CRC; it is not a
durable proof across restart. A resumed suffix never becomes a full-file proof
by itself. Exact raw, WAV, memo, account, Ring and deletion checks still apply.

Use the packaged `TEST-REPORT.md`, the
[S03 reliability and qualification plan](../reference/ring-s03-reliability.md)
and the [full recording acceptance matrix](../reference/ring-recording-and-ptt.md).
Return logs, readback, recorded timings and original test audio through the
team's approved channel; remove device secrets from shared logs.

## 4. If it fails / recovery

Stop repeated writes and capture debugger or bootloader evidence. A successful
program verify is not proof that the bootloader accepted the application or
that physical audio works. Review the published S03 CI summary and map for
compiler, ABI and memory evidence; source and host checks do not establish
runtime stack, heap, radio, audio, battery or touch behavior.

Use the supplier's previously demonstrated recovery procedure and the test
unit's own backup, restoring a mutually compatible application and settings
pair. The preserved factory `6.0.3.3Z62` distribution is available for
comparison in the [factory evidence directory](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62).
It does not contain that unit's recordings, bonds or calibration. Demonstrate
interrupted-update recovery on the spare before any user Ring update.

Use the S03 package's `provenance.json` for the final firmware
source SHA, CI run, app identity and asset hashes. No hardware qualification is
claimed by this guide.
