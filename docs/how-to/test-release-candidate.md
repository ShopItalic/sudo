# RC1 — download, flash and test

**Release:** [v6.0.3.3S01-rc.1](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S01-rc.1)

**Target:** standard Bravechip `603V1.23.2` / Nordic nRF52840

**Firmware readback:** `6.0.3.3S01`

**Compiled source:** `bde62175ea2c3c64a0f633abb6d06d05595d9cc7`

**Build:** [verified Linux CI run 33998477755](https://github.com/ShopItalic/sudo/actions/runs/33998477755)

This is a supplier bench-test candidate. The application is unsigned and has
not been tested on a physical Ring. Use a spare standard board with a known
recovery procedure. The package does not contain a signed RC OTA image,
replacement bootloader, SoftDevice, UICR, per-unit backup or pre-generated
bootloader settings.

**供应商快速说明：** 请下载下面的 `supplier-review.zip`，它是工程测试资料包，
不是可直接通过手机升级的 OTA 包。仅适用于标准 `603V1.23.2`，不是
`1.23.2_one_sec`。请使用已确认可恢复的备用样机，通过贵方 SWD 工装和匹配的
bootloader settings 测试，或由贵方签名生成正式 OTA 包。刷写后版本应为
`6.0.3.3S01`；核心验收是保持 BLE 连接，双击开始/停止后产生可完整下载播放的
非零 Flash 文件。请返回版本读数、日志、音频和测试结果。

## 1. Download the right files

Open the [release Assets section](https://github.com/ShopItalic/sudo/releases/tag/v6.0.3.3S01-rc.1)
and download `italic-ring-603v1.23.2-6.0.3.3S01-rc.1-supplier-review.zip`.
GitHub's automatic **Source code (zip/tar.gz)** downloads contain source, not
prebuilt release assets. This repository is private: a 404 usually means the
GitHub account needs repository access or sign-in.

With GitHub CLI, download all assets into a new directory:

```sh
gh auth status
gh release download v6.0.3.3S01-rc.1 --repo ShopItalic/sudo --dir ring-rc1
cd ring-rc1
shasum -a 256 -c SHA256SUMS          # macOS
# Linux equivalent: sha256sum -c SHA256SUMS
unzip italic-ring-603v1.23.2-6.0.3.3S01-rc.1-supplier-review.zip -d unpacked
```

For a browser download of the ZIP alone, compare its SHA-256 against its line
in the release's `SHA256SUMS`. Windows PowerShell can print it with
`Get-FileHash -Algorithm SHA256 <downloaded-zip>`. The unpacked bundle also has
its own `SHA256SUMS` for all included files; verify it from that directory.

| File | Meaning / intended user |
| --- | --- |
| `*-supplier-review.zip` | Complete review folder with instructions, images, provenance, checksums and notices. **Not a Nordic DFU ZIP.** |
| `*.hex` | Application-only Intel HEX with addresses. Preferred application input for a supplier SWD programmer. |
| `*.bin` | Raw application bytes; base address **`0x00027000`**, never address zero. For analysis or a supplier packager that requires BIN. |
| `*.elf` | Exact CI-linked application with debug symbols, for J-Link/GDB and fault diagnosis. |
| `*.map` | Linker map for memory and symbol review. |
| `ci-build-summary.json` | CI build, startup, memory, ABI and image-hash evidence. Its original paths/names are CI paths; release names are mapped by `provenance.json`. |
| `provenance.json` | Source/app commits, CI identity, artifact hashes and application-only boundaries. |
| `SHA256SUMS` | Integrity checks; a checksum is not an OTA signature. |
| `README.md`, `FLASHING.md`, `TEST-REPORT.md`, `licenses/` | Offline starting point, this procedure, a results template and preserved notices. |

The HEX contains only application flash within `[0x00027000, 0x000E0000)`.
It does not initialize an erased nRF52840 by itself. The retained baseline is
S140 **7.2.0**, firmware ID **0x0100**, factory bootloader at **0xF8000**, settings
backup at **0xFE000** and primary settings at **0xFF000**. These describe the
recovered distribution; read back and confirm the actual test unit.

## 2. Select the update path

| Situation | Procedure |
| --- | --- |
| Supplier has confirmed SWD pads, working probe and a recoverable spare board | Follow the SWD bench procedure below, including matching settings. |
| Ring can only update over BLE through its factory bootloader | Supplier must sign/package the RC with the accepted key and version policy; see OTA preparation below. |
| No verified pads, debug access, bootloader identity or recovery route | Review/build/emulate first. A downloadable application does not establish a flash procedure for that unit. |

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
The flash/erase/verification meanings above are documented in
[Nordic's nrfjprog reference](https://docs.nordicsemi.com/r/bundle/ug_nrf_cltools/page/ug/cltools/nrf_nrfjprogexe_reference.html).

3. **Prepare matching bootloader settings.** The old factory settings contain
   the old application's size/CRC. Writing only the new application can leave
   the factory bootloader refusing to boot it. The supplier must confirm its
   actual settings format, version/anti-rollback policy, backup-page handling
   and boot validation. The historical standard-board script uses settings
   version **1**, not an assumed version 2. This is its legacy command form,
   with release counters deliberately left for the supplier:

```sh
nrfutil settings generate --family NRF52840 \
  --application italic-ring-603v1.23.2-6.0.3.3S01-rc.1.hex \
  --application-version APPROVED_APP_COUNTER \
  --bootloader-version CONFIRMED_BOOTLOADER_COUNTER \
  --bl-settings-version 1 supplier-approved-settings.hex
```

The old Python `nrfutil` syntax is not interchangeable with an arbitrary modern
installation. Use the supplier's working toolchain or its documented `nrf5sdk-tools`
equivalent. Inspect the generated settings and verify application size/CRC,
settings CRC and both expected settings pages before programming. Do not reuse
`application.dat` or settings from `6.0.3.3Z62`. See the retained
[factory script](https://github.com/ShopItalic/sudo/blob/v6.0.3.3S01-rc.1/firmware/BCL603S2X/dfu/ota_bat/creat_1232_dfu.bat)
and [Nordic settings inspection guide](https://docs.nordicsemi.com/r/bundle/nrfutil/page/nrfutil-nrf5sdk-tools/guides/generating_bootloader_settings.html/displaying-bootloader-settings).

4. After reviewing the application and settings address ranges, program only
   the application and approved settings pages, verify both, then reset:

```sh
nrfjprog --family NRF52 --snr PROBE_SERIAL \
  --program italic-ring-603v1.23.2-6.0.3.3S01-rc.1.hex --sectorerase --verify
nrfjprog --family NRF52 --snr PROBE_SERIAL \
  --program supplier-approved-settings.hex --sectorerase --verify
nrfjprog --family NRF52 --snr PROBE_SERIAL --reset
```

These templates intentionally do not replace the SoftDevice, bootloader or
UICR. Do not add chip erase, UICR erase or external-Flash erase flags. A settings
file with unexpected address ranges must be rejected before these commands.
No package here is presented as a universal one-click recovery image.

### BLE OTA preparation — supplier signing required

The existing factory package uses ECDSA P-256/SHA-256, hardware family value
**52** and SoftDevice requirement **0x0100**. That generic hardware value does
not identify standard `603V1.23.2` by itself. The supplier must confirm the
installed bootloader's accepted key, target identity and release counters.

The legacy packaging template is:

```sh
nrfutil pkg generate \
  --application italic-ring-603v1.23.2-6.0.3.3S01-rc.1.hex \
  --application-version APPROVED_APP_COUNTER \
  --hw-version 52 --sd-req 0x0100 \
  --key-file SUPPLIER_CONTROLLED_SIGNING_KEY \
  supplier-signed-6.0.3.3S01-rc.1-ota.zip
```

Keep the private key with the supplier; no signing key is needed for downloading
or reviewing RC1. Verify the generated init packet/signature, then use the
supplier's supported BLE updater on the verified spare board. Entering DFU,
anti-rollback and interrupted-update recovery still need supplier confirmation.
Renaming the review ZIP or combining the new BIN with the factory DAT will not
produce a valid signed RC update.

## 3. Use the matching app and test

Build app commit **`064c265356e32ea9819a8eaa57929ebc17f66f0a`**, supplied in
[ShopItalic/app PR #10](https://github.com/ShopItalic/app/pull/10), using that
repository's Xcode setup and the supplier's normal development signing. An
installable IPA/TestFlight build is not included in this firmware release.
The native adapter requires standard `603V1.23.2`, exact `6.0.3.3S01` readback
and a successful HELLO capability exchange. A store app or factory-only client
must not be assumed to speak the new protocol.

Record board ID, firmware readback, programmed image SHA-256, app commit,
probe/tool versions, starting file list and configuration. Then run:

| Test | Pass condition |
| --- | --- |
| First boot | Readback is `6.0.3.3S01`; no reset loop; BLE/HELLO works. |
| Connected double tap | Stay connected, double tap, speak, double tap. A new nonzero file appears; download/playback includes beginning and end. |
| Hold/release PTT | Connected and standalone recording works; measure release-to-mic-stop within one second; feedback agrees with saved/failed state. |
| Disconnect and app death | Local capture survives; reconnect can retrieve the correct recording. |
| Initial live audio | First words survive normal READY delay. Overflow/gaps fall back to stored audio without inventing a complete live transcript. |
| Interrupted download | Resume completes with matching raw CRC and playable WAV; failed phone storage leaves the Ring copy. |
| Settings and power | Touch/LED/haptic settings persist; validate real battery/charging behavior and actual current/temperature. |
| iPhone keyboard | Short PTT inserts a complete final once into the visible field. Field change, manual edit or expired iOS window prevents stale insertion; stored audio remains. |
| Abrupt power / full storage | On the spare, execute the full fault matrix; recover verified prefixes and preserve older unsynced files. |

Use the packaged `TEST-REPORT.md` and the
[full acceptance matrix](https://github.com/ShopItalic/sudo/blob/v6.0.3.3S01-rc.1/docs/reference/ring-recording-and-ptt.md).
Return logs, readback, recorded timings and original test audio through the
team's approved channel; remove device secrets from shared logs.

## 4. If it fails / recovery

Stop repeated writes and capture debugger/bootloader evidence. A successful
program verify is not proof that the bootloader accepted the application or
that physical audio works. The GNU candidate retains two supplier wchar ABI
warnings, unsupported stdout through libnosys and unmeasured runtime stack/heap
margin; review the CI summary and map during diagnosis.

Use the supplier's previously demonstrated recovery procedure and the test
unit's own backup, restoring a mutually compatible application/settings pair.
The preserved factory `6.0.3.3Z62` distribution is available for comparison in
[the factory evidence directory](https://github.com/ShopItalic/sudo/tree/v6.0.3.3S01-rc.1/artifacts/ring-firmware/603v1.23.2-6.0.3.3z62).
It does not contain that unit's recordings, bonds or calibration. Demonstrate
interrupted-update recovery on the spare before any user Ring update.
