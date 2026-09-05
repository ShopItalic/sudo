# Production firmware baseline

The source task inspected `Firmware/1.23.2_6033固件SDK.zip` in the Sudo Google
Drive folder on the MBA. Its source board selection agrees with the iOS
production gate for `603V1.23.2`. The source archive and its original hash
manifest are not yet available in this checkout. Import them before adding a
build command or claiming a reproducible firmware release.

## Evidence to retain on import

| Area | Archive locator recorded by the hardware task |
| --- | --- |
| Board selection | `bc_ros/bc_config/ring_config.h` |
| Flash configuration | `bc_ros/bc_module/spi_flash/sfud/inc/sfud_cfg.h`, `sfud_flash_def.h` |
| Touch | `bc_ros/bc_module/touch_button/IQS7211E` |
| Microphone | `bc_ros/bc_module/pdm/bc_pdm.h` |
| PMIC | `bc_ros/bc_module/pmic/bc_pmic.c`, `bc_device/yhm2712` |
| Power transitions | `bc_power.c` |
| Haptics | `app_linear_motor_handler.c`, `bc_linear_motor.c` |
| Nordic SDK configuration | `BCL603S2X/app/user/inc/sdk_config.h` |

The complete [factory firmware evidence](reference/sudo-ring-hardware.md#factory-firmware-evidence)
records interface details and limitations. Preserve the vendor's filenames,
copyright notices, licenses, build projects, linker scripts, bootloader, and
SoftDevice requirements. Keep compiled output and device-specific provisioning
material out of Git. Do not generate replacement pin maps or adapt the older
Nordic prototype by assumption.

## Build and hardware acceptance

1. Import the original vendor archive with its SHA-256 and source revision.
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

No device was flashed or tested during this repository initialization.
