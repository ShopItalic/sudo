# Production firmware baseline

The source task inspected `Firmware/1.23.2_6033固件SDK.zip` in the Sudo Google
Drive folder on the MBA. Its source board selection agrees with the production
Ring baseline for `603V1.23.2`. The recovered [raw factory firmware
extraction](reference/ring-firmware.md) includes the 184,132-byte
`application.bin`, the combined `BCL603S2P_6.0.3.3Z62.hex`, the application-only
OTA ZIP, and seven exact memory regions. SHA-256, ZIP CRC, HEX consistency, DFU
metadata, and ECDSA signature checks passed against the saved factory
distribution.

The vendor SDK source tree is still not imported as a buildable tree. No
compiler command, reproducible rebuild, or approved release has been
established. No physical device has been dumped or flashed, and no source
rebuild has been performed.

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
build output and device-specific provisioning material out of Git. Do not generate
replacement pin maps or adapt the older Nordic prototype by assumption.

## Build and hardware acceptance

1. Review and import the vendor source tree with its archive SHA-256 and revision.
   Identify the exact supported compiler, SDK, target, and packaging tools from
   the supplied project files.
2. Reproduce the unmodified vendor build before changing firmware. Record
   tool versions, command, output hashes, and memory use.
3. Read back the physical board and firmware identity. Verify audio rate and
   ADPCM framing against the app's current 8 kHz mono interpretation; older
   16 kHz / Opus requirements do not establish the fitted firmware contract.
4. Verify live streaming and stored-file transfer, interruption/resume,
   battery and charging states, touch, motion, haptics, and recording cleanup
   through the current app adapter.
5. Validate the upgrade path and recovery on the correct hardware before
   publishing a firmware release.

The extraction involved no physical-device operations or firmware rebuild.
