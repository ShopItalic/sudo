# S04 RC1: download, flash and test

This guide describes **S04 RC1**, an unsigned engineering candidate for supplier
bench testing. The package's `provenance.json` identifies the exact firmware
source SHA, CI run and asset hashes. RC1 is refreshed in place with the
merged reliability fixes while keeping the same tag and firmware version.
Re-download and verify the complete asset set; do not mix files from the
original `84c91fc` bundle with the refreshed package.

**Release:** [v6.0.3.3S04-rc.1](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S04-rc.1)

**Target:** standard Bravechip `603V1.23.2` / Nordic nRF52840

**Firmware readback:** `6.0.3.3S04`

**Firmware source and CI:** use `sourceSHA` and
`CIRunURL` from the S04 package's `provenance.json`. Do not copy the S01 RC1
source or CI values into this guide or a test report.

**Client compatibility:** This is a firmware-only release. No matching S04 app
or SDK binary is included. A host must explicitly support `6.0.3.3S04`, HELLO
bits 9/10/11 and the input mapping protocol. Do not use an older app's version
label or settings controls as proof of S04 compatibility.

This is a supplier bench-test candidate. The review bundle is an
unsigned application-only package. It is not a signed RC OTA image, and no
hardware qualification is included. Use a spare standard board with a known
recovery procedure. The package does not contain a signed OTA image,
replacement bootloader, SoftDevice, UICR, per-unit backup, or pre-generated
bootloader settings.

**供应商快速说明：** 这是 S04 固件工程测试资料包，不是可直接通过手机升级的
签名 OTA 包。仅适用于标准 `603V1.23.2`。请使用已确认可恢复的备用样机及贵方
SWD 工装、匹配的 bootloader settings，或由贵方签名生成 OTA 包。
版本应为 `6.0.3.3S04`。PTT 松手结束，无人为时长上限；长按触发时间支持
0.5–10 秒。默认长按一秒进入 PTT、双击关闭、三击切换备忘录录音。
三个输入可独立映射；灯光和震动设置可持久化并读回。本次不提供匹配 App。
请返回触摸、录音、蓝牙、灯光震动、断电恢复的日志和实测结果。

## 1. Download the right files

Open the [S04 release Assets section](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S04-rc.1) and download
`italic-ring-603v1.23.2-6.0.3.3S04-rc.1-supplier-review.zip`. GitHub's automatic **Source code
(zip/tar.gz)** downloads contain source, not prebuilt release assets. The
repository is public. Use this exact release tag rather than the historical S03 package.

With GitHub CLI, download all assets into a new directory:

```sh
gh auth status
gh release download v6.0.3.3S04-rc.1 --repo ShopItalic/sudo --dir ring-rc-s04
cd ring-rc-s04
shasum -a 256 -c SHA256SUMS          # macOS
# Linux equivalent: sha256sum -c SHA256SUMS
unzip italic-ring-603v1.23.2-6.0.3.3S04-rc.1-supplier-review.zip -d unpacked
```

For a browser download of the ZIP alone, compare its SHA-256 against its line
in the release's `SHA256SUMS`. Windows PowerShell can print it with
`Get-FileHash -Algorithm SHA256 <downloaded-zip>`. The unpacked bundle also has
its own `SHA256SUMS` for all included files; verify it from that directory.

| File | Meaning / intended user |
| --- | --- |
| `italic-ring-603v1.23.2-6.0.3.3S04-rc.1-supplier-review.zip` | Complete review folder with instructions, images, provenance, checksums and notices. **Not a Nordic DFU ZIP or OTA package.** |
| `italic-ring-603v1.23.2-6.0.3.3S04-rc.1.hex` | Application-only Intel HEX with addresses. Preferred application input for a supplier SWD programmer. |
| `italic-ring-603v1.23.2-6.0.3.3S04-rc.1.bin` | Raw application bytes; base address **`0x00027000`**, never address zero. For analysis or a supplier packager that requires BIN. |
| `italic-ring-603v1.23.2-6.0.3.3S04-rc.1.elf` | Exact CI-linked application with debug symbols, for J-Link/GDB and fault diagnosis. |
| `italic-ring-603v1.23.2-6.0.3.3S04-rc.1.map` | Linker map for memory and symbol review. |
| `ci-build-summary.json` | CI build, startup, memory, ABI and image-hash evidence. Its original paths and names are CI paths; release names are mapped by `provenance.json`. |
| `provenance.json` | S04 source SHA, CI identity, matching app commit, asset hashes and application-only boundaries. Read this file for final source and CI values. |
| `SHA256SUMS` | Integrity checks; a checksum is not an OTA signature. |
| `README.md`, `FLASHING.md`, `TEST-REPORT.md`, `licenses/` | Offline starting point, this procedure, a results template and preserved notices. |

The published S04 HEX must contain only application flash within
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
  --application italic-ring-603v1.23.2-6.0.3.3S04-rc.1.hex \
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
  --program italic-ring-603v1.23.2-6.0.3.3S04-rc.1.hex --sectorerase --verify
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
  --application italic-ring-603v1.23.2-6.0.3.3S04-rc.1.hex \
  --application-version APPROVED_APP_COUNTER \
  --hw-version 52 --sd-req 0x0100 \
  --key-file SUPPLIER_CONTROLLED_SIGNING_KEY \
  italic-ring-603v1.23.2-6.0.3.3S04-rc.1-supplier-signed-ota.zip
```

Keep the private key with the supplier. No signing key is needed for
downloading or reviewing the unsigned S04 RC. Verify the generated init
packet and signature, then use the supplier's supported BLE updater on the
verified spare board. Entering DFU, anti-rollback and interrupted-update
recovery still need supplier confirmation. Renaming the review ZIP or combining
the new BIN with the factory DAT will not produce a valid signed RC update.

## 3. Firmware bench acceptance

Use a host or supplier harness that implements the [S04 wire contract](../reference/ring-voice-protocol.md).
No S04 app build is bundled. Record board identity, firmware readback, image
SHA-256, source SHA and CI URL from `provenance.json`, probe/tool versions,
starting configuration and recording list. Software checks are not physical results.

| Test | Required result |
| --- | --- |
| First boot | Standard `603V1.23.2`, readback `6.0.3.3S04`, no reset loop, working BLE and HELLO bits 9/10/11. |
| Fresh inputs | One-second hold maps to PTT, double tap is disabled, triple tap toggles memo. Single tap/swipes do not start an action. |
| Persisted input migration | Prior memo opt-out remains disabled on triple tap; prior PTT limit becomes zero; mappings and light/haptic preferences survive reboot. |
| Adjustable activation | Read/write INPUTS_SET/GET for 0.5, 1, 2, 5 and 10 seconds. Confirm sensor pending/applied/error status and measure actual activation timing. |
| PTT | Record beyond 10 seconds, one minute and multiple minutes; release stops capture and drains the tail. Touch loss, full storage and capture faults are reported. |
| Three mappings | Independently test disabled, memo toggle and host event on all inputs; hold also supports PTT. Taps reject PTT. Verify triple tap does not also trigger double tap. |
| Live host events | Hold produces activation then release/cancellation; taps produce activation only. Backpressured events expire; disconnect never replays old actions. |
| Lights and haptics | SETTINGS enable switches and TUNING strength/start/stop durations persist and read back. Mute cancels normal feedback; bootloader output is separate. |
| Transfers | Exercise delayed ACKs, reconnect, partial resume, 30-second no-progress expiry and custody-before-delete. Preserve the source on failed host storage. |
| Phone outcome firmware port | Same-connection, exact terminal recording/READY token and timely outcome produce the distinct cue once; wrong/expired tokens, mute or new capture do not. End-to-end keyboard testing requires a separate compatible client. |
| Battery and power | Measure ADC behavior around motor/LED/BLE/charging; test abrupt power and full storage without deleting unsynced files. |
| Recovery | Supplier demonstrates a compatible image/settings pair and recovery from failed updates on the verified spare. |

Use the packaged `TEST-REPORT.md` and [physical acceptance matrix](../reference/ring-recording-and-ptt.md).
Onboard audio remains **8 kHz mono IMA ADPCM**; this RC does not introduce Opus.
Preserve original logs/audio and remove per-device secrets before sharing.

## 4. If it fails / recovery

Stop repeated writes and capture debugger or bootloader evidence. A successful
program verify is not proof that the bootloader accepted the application or
that physical audio works. Review the published S04 CI summary and map for
compiler, ABI and memory evidence; source and host checks do not establish
runtime stack, heap, radio, audio, battery or touch behavior.

Use the supplier's previously demonstrated recovery procedure and the test
unit's own backup, restoring a mutually compatible application and settings
pair. The preserved factory `6.0.3.3Z62` distribution is available for
comparison in the [factory evidence directory](../../artifacts/ring-firmware/603v1.23.2-6.0.3.3z62).
It does not contain that unit's recordings, bonds or calibration. Demonstrate
interrupted-update recovery on the spare before any user Ring update.

Use the S04 package's `provenance.json` for the final firmware
source SHA, CI run, app identity and asset hashes. No hardware qualification is
claimed by this guide.
