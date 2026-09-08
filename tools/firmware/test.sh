#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/firmware/tests
firmware_host_cc=${CC:-cc}
. tools/firmware/opus_host_sources.sh
python3 tools/firmware/import_opus.py --verify
build_opus_host_library
app_package_source=${RECORDING_APP_PACKAGE_SOURCE:-firmware/bc_ros/bc_application/app_package.c}
cmd_handler_source=${RECORDING_CMD_HANDLER_SOURCE:-firmware/bc_ros/bc_application/app_cmd_handler.c}
python3 tools/firmware/extract_recording_sources.py \
  --app-package-source "$app_package_source" \
  --cmd-handler-source "$cmd_handler_source" \
  --app-package-output build/firmware/tests/recording_app_package.c \
  --cmd-handler-output build/firmware/tests/recording_cmd_handler.c
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -g -fsanitize=address,undefined \
  -Itests/firmware \
  -Ifirmware/bc_ros/bc_application \
  -Ifirmware/bc_ros/bc_config \
  -Ifirmware/bc_ros/bc_module/ble/inc \
  -Ifirmware/bc_ros/bc_module/queue \
  build/firmware/tests/recording_app_package.c \
  build/firmware/tests/recording_cmd_handler.c \
  tests/firmware/test_recording_commands.c -o build/firmware/tests/test_recording_commands
build/firmware/tests/test_recording_commands
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -g -fsanitize=address,undefined \
  -Ifirmware/bc_ros/bc_module/ble/inc \
  firmware/bc_ros/bc_module/ble/src/bc_ble_tx.c \
  firmware/bc_ros/bc_module/ble/src/bc_file_transfer.c \
  tests/firmware/test_transfer.c -o build/firmware/tests/test_transfer
build/firmware/tests/test_transfer
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined \
  -Ifirmware/bc_ros/bc_module/recording \
  firmware/bc_ros/bc_module/recording/bc_audio_format.c \
  firmware/bc_ros/bc_module/recording/bc_recording.c \
  tests/firmware/test_recording_owner.c -o build/firmware/tests/test_recording_owner
build/firmware/tests/test_recording_owner
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined \
  -Ifirmware/bc_ros/bc_module/recording \
  firmware/bc_ros/bc_module/recording/bc_capture.c \
  tests/firmware/test_capture.c -o build/firmware/tests/test_capture
build/firmware/tests/test_capture
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined \
  -DSUDO_VOICE_ONLY -DHANDWARE_1_23_2 -DGLOBAL_STACK_SIZE=$opus_scratch_bytes \
  -Itests/firmware/sudo_capture \
  -Itests/firmware/opus_host \
  -Ifirmware/bc_ros/bc_application \
  -Ifirmware/bc_ros/bc_module/recording \
  $opus_includes \
  firmware/bc_ros/bc_module/recording/bc_audio_format.c \
  firmware/bc_ros/bc_module/recording/bc_recording.c \
  firmware/bc_ros/bc_module/recording/bc_capture.c \
  firmware/bc_ros/bc_module/recording/bc_resampler.c \
  firmware/bc_ros/bc_module/recording/bc_opus_stream.c \
  firmware/bc_ros/bc_module/recording/bc_opus_encoder.c \
  firmware/bc_ros/bc_application/app_sudo_capture.c \
  tests/firmware/test_sudo_capture.c build/firmware/tests/opus/libopus_profile.a -lm \
  -o build/firmware/tests/test_sudo_capture
build/firmware/tests/test_sudo_capture
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined \
  -Ifirmware/bc_ros/bc_module/recording \
  firmware/bc_ros/bc_module/recording/bc_resampler.c \
  tests/firmware/test_resampler.c -o build/firmware/tests/test_resampler -lm
build/firmware/tests/test_resampler
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined \
  -Ifirmware/bc_ros/bc_module/recording \
  firmware/bc_ros/bc_module/recording/bc_audio_format.c \
  firmware/bc_ros/bc_module/recording/bc_opus_stream.c \
  tests/firmware/test_opus_stream.c -o build/firmware/tests/test_opus_stream
build/firmware/tests/test_opus_stream
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined \
  -DGLOBAL_STACK_SIZE=$opus_host_scratch_bytes \
  -Ifirmware/bc_ros/bc_module/recording -Itests/firmware/opus_host $opus_includes \
  firmware/bc_ros/bc_module/recording/bc_audio_format.c \
  firmware/bc_ros/bc_module/recording/bc_opus_stream.c \
  firmware/bc_ros/bc_module/recording/bc_opus_encoder.c \
  tests/firmware/opus_host/bc_opus_port_host.c \
  tests/firmware/test_opus_codec.c build/firmware/tests/opus/libopus_host.a -lm \
  -o build/firmware/tests/test_opus_codec
python3 tools/firmware/export_opus_fixtures.py --check
build/firmware/tests/test_opus_codec
python3 tools/firmware/export_opus_fixtures.py --check
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined \
  -Ifirmware/bc_ros/bc_module/recording \
  firmware/bc_ros/bc_module/recording/bc_voice_wire.c \
  tests/firmware/test_voice_wire.c -o build/firmware/tests/test_voice_wire
build/firmware/tests/test_voice_wire
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined \
  -Ifirmware/bc_ros/bc_module/recording \
  firmware/bc_ros/bc_module/recording/bc_touch_report.c \
  tests/firmware/test_touch_report.c -o build/firmware/tests/test_touch_report
build/firmware/tests/test_touch_report
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined \
  -Ifirmware/bc_ros/bc_module/recording \
  firmware/bc_ros/bc_module/recording/bc_audio_format.c \
  firmware/bc_ros/bc_module/recording/bc_recording.c \
  firmware/bc_ros/bc_module/recording/bc_voice_gesture.c \
  tests/firmware/test_voice_gesture.c -o build/firmware/tests/test_voice_gesture
build/firmware/tests/test_voice_gesture
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined \
  -Ifirmware/bc_ros/bc_module/recording \
  -Ifirmware/bc_ros/bc_module/file/LittleFS \
  firmware/bc_ros/bc_module/file/LittleFS/lfs.c \
  firmware/bc_ros/bc_module/file/LittleFS/lfs_util.c \
  firmware/bc_ros/bc_module/recording/bc_audio_format.c \
  firmware/bc_ros/bc_module/recording/bc_rec_store.c \
  firmware/bc_ros/bc_module/recording/bc_recording.c \
  firmware/bc_ros/bc_module/recording/bc_voice_gesture.c \
  firmware/bc_ros/bc_module/recording/bc_voice_service.c \
  firmware/bc_ros/bc_module/recording/bc_voice_wire.c \
  tests/firmware/test_voice_service.c -o build/firmware/tests/test_voice_service
build/firmware/tests/test_voice_service
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic \
  -Wno-unused-parameter -Wno-pointer-integer-compare \
  -g -fsanitize=address,undefined \
  -DSUDO_VOICE_ONLY -DHANDWARE_1_23_2 \
  -Itests/firmware/led \
  -Ifirmware/bc_ros/bc_module/led \
  firmware/bc_ros/bc_module/led/bc_ic_led.c \
  tests/firmware/test_ic_led.c -o build/firmware/tests/test_ic_led
build/firmware/tests/test_ic_led
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined \
  -Ifirmware/bc_ros/bc_module/recording \
  -Ifirmware/bc_ros/bc_module/file/LittleFS \
  firmware/bc_ros/bc_module/file/LittleFS/lfs.c \
  firmware/bc_ros/bc_module/file/LittleFS/lfs_util.c \
  firmware/bc_ros/bc_module/recording/bc_audio_format.c \
  firmware/bc_ros/bc_module/recording/bc_rec_store.c \
  tests/firmware/test_recording_store.c \
  -o build/firmware/tests/test_recording_store
build/firmware/tests/test_recording_store
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined \
  -Ifirmware/bc_ros/bc_module/recording \
  -Ifirmware/bc_ros/bc_module/file/LittleFS \
  firmware/bc_ros/bc_module/file/LittleFS/lfs.c \
  firmware/bc_ros/bc_module/file/LittleFS/lfs_util.c \
  firmware/bc_ros/bc_module/recording/bc_audio_format.c \
  firmware/bc_ros/bc_module/recording/bc_rec_store.c \
  firmware/bc_ros/bc_module/recording/bc_voice_legacy_archive.c \
  tests/firmware/test_voice_legacy_archive.c \
  -o build/firmware/tests/test_voice_legacy_archive
build/firmware/tests/test_voice_legacy_archive
python3 tests/firmware/test_armcc_archive.py
python3 tests/firmware/test_ble_security.py
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -g -fsanitize=address,undefined \
  -DSUDO_VOICE_ONLY -DHANDWARE_1_23_1 -DHANDWARE_1_23_2 \
  -Itests/firmware/flash_io \
  -Ifirmware/bc_ros/bc_module/file/LittleFS \
  -Ifirmware/bc_ros/bc_module/spi_flash/sfud/inc \
  firmware/bc_ros/bc_module/file/LittleFS/lfs_port.c \
  tests/firmware/test_flash_io.c -o build/firmware/tests/test_flash_io
build/firmware/tests/test_flash_io
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -Wno-sign-compare -Wno-invalid-utf8 \
  -pedantic -O2 -g -fsanitize=address,undefined \
  -DSUDO_VOICE_ONLY -DHANDWARE_1_23_2 \
  -Itests/firmware/sudo_voice_worker \
  -Itests/firmware \
  -Ifirmware/bc_ros/bc_application \
  -Ifirmware/bc_ros/bc_config \
  -Ifirmware/bc_ros/bc_module/recording \
  -Ifirmware/bc_ros/bc_module/file/LittleFS \
  -Ifirmware/bc_ros/bc_module/ble/inc \
  -Ifirmware/bc_ros/bc_module/queue \
  -Ifirmware/bc_ros/bc_module/spi_flash \
  -Ifirmware/bc_ros/bc_module/led \
  -Ifirmware/bc_ros/bc_module/motor \
  -Ifirmware/bc_ros/bc_module/wtd \
  -Ifirmware/bc_ros/bc_module/touch_button \
  -Ifirmware/bc_ros/bc_module/rtc \
  -Ifirmware/bc_ros/bc_module \
  -Ifirmware/bc_ros/bc_algorithm \
  firmware/bc_ros/bc_module/file/LittleFS/lfs.c \
  firmware/bc_ros/bc_module/file/LittleFS/lfs_util.c \
  firmware/bc_ros/bc_module/recording/bc_audio_format.c \
  firmware/bc_ros/bc_module/recording/bc_rec_store.c \
  firmware/bc_ros/bc_module/recording/bc_recording.c \
  firmware/bc_ros/bc_module/recording/bc_voice_wire.c \
  firmware/bc_ros/bc_module/recording/bc_voice_gesture.c \
  firmware/bc_ros/bc_module/recording/bc_voice_service.c \
  firmware/bc_ros/bc_module/recording/bc_voice_legacy_archive.c \
  firmware/bc_ros/bc_module/recording/bc_touch_tuning.c \
  firmware/bc_ros/bc_application/app_sudo_voice.c \
  tests/firmware/test_sudo_voice_worker.c \
  -o build/firmware/tests/test_sudo_voice_worker
build/firmware/tests/test_sudo_voice_worker
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror \
  -Wno-invalid-utf8 -Wno-unused-variable -Wno-unused-function -Wno-pedantic \
  -O2 -g -fsanitize=address,undefined \
  -DSUDO_VOICE_ONLY -DHANDWARE_1_23_1 -DHANDWARE_1_23_2 \
  -Itests/firmware/touch_tuning \
  -Ifirmware/bc_ros/bc_module/recording \
  -Ifirmware/bc_ros/bc_device/touch_button/IQS7211E \
  firmware/bc_ros/bc_module/recording/bc_touch_tuning.c \
  firmware/bc_ros/bc_module/recording/bc_touch_report.c \
  firmware/bc_ros/bc_device/touch_button/IQS7211E/IQS7211E.c \
  tests/firmware/test_touch_tuning.c \
  -o build/firmware/tests/test_touch_tuning
build/firmware/tests/test_touch_tuning
case "$(uname -s)" in
  Darwin) motor_gc_sections=-Wl,-dead_strip ;;
  *) motor_gc_sections=-Wl,--gc-sections ;;
esac
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror \
  -Wno-unused-parameter -Wno-pedantic -Wno-pointer-integer-compare \
  -g -fsanitize=address,undefined \
  -ffunction-sections -fdata-sections "$motor_gc_sections" \
  -DSUDO_VOICE_ONLY -DHANDWARE_1_23_2 -DHARDWARE_ARCH_TYPE_NORDIC=1 \
  -Itests/firmware/motor \
  -Ifirmware/bc_ros/bc_application \
  -Ifirmware/bc_ros/bc_module/motor \
  firmware/bc_ros/bc_module/motor/bc_linear_motor.c \
  firmware/bc_ros/bc_application/app_linear_motor_handler.c \
  tests/firmware/test_linear_motor.c \
  -o build/firmware/tests/test_linear_motor
build/firmware/tests/test_linear_motor
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined \
  -Ifirmware/bc_ros/bc_module/pmic \
  firmware/bc_ros/bc_module/pmic/bc_battery_filter.c \
  tests/firmware/test_battery_filter.c \
  -o build/firmware/tests/test_battery_filter
build/firmware/tests/test_battery_filter
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic \
  -Wno-unused-parameter -Wno-unused-but-set-variable \
  -g -fsanitize=address,undefined \
  -DSUDO_VOICE_ONLY -DHANDWARE_1_23_2 \
  -Itests/firmware/battery_power \
  -Ifirmware/bc_ros/bc_module/motor \
  -Ifirmware/bc_ros/bc_module/pmic \
  tests/firmware/test_battery_power.c \
  -o build/firmware/tests/test_battery_power
build/firmware/tests/test_battery_power
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -Wno-unused-parameter -g \
  -fsanitize=address,undefined \
  -DSUDO_VOICE_ONLY -DHARDWARE_ARCH_TYPE_NORDIC=1 -DHANDWARE_1_23_2 \
  -Itests/firmware/bsp_adc \
  tests/firmware/test_bsp_adc.c \
  -o build/firmware/tests/test_bsp_adc
build/firmware/tests/test_bsp_adc
case "$(uname -s)" in
  Darwin) battery_app_gc_sections=-Wl,-dead_strip ;;
  *) battery_app_gc_sections=-Wl,--gc-sections ;;
esac
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic \
  -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function \
  -Wno-extra-semi -Wno-int-to-pointer-cast -Wno-sign-compare \
  -Wno-invalid-utf8 -DDEBUG_INFO=0 \
  -g -fsanitize=address,undefined -ffunction-sections -fdata-sections \
  "$battery_app_gc_sections" \
  -DSUDO_VOICE_ONLY -DHANDWARE_1_23_2 \
  -Itests/firmware/battery_app \
  -Itests/firmware/sudo_voice_worker \
  -Itests/firmware \
  -Ifirmware/bc_ros/bc_application \
  -Ifirmware/bc_ros/bc_config \
  -Ifirmware/bc_ros/bc_module/recording \
  -Ifirmware/bc_ros/bc_module/file/LittleFS \
  -Ifirmware/bc_ros/bc_module/ble/inc \
  -Ifirmware/bc_ros/bc_module/queue \
  -Ifirmware/bc_ros/bc_module/spi_flash \
  -Ifirmware/bc_ros/bc_module/led \
  -Ifirmware/bc_ros/bc_module/wtd \
  -Ifirmware/bc_ros/bc_module/touch_button \
  -Ifirmware/bc_ros/bc_module/rtc \
  -Ifirmware/bc_ros/bc_module/pmic \
  -Ifirmware/bc_ros/bc_module/device_info \
  -Ifirmware/bc_ros/bc_module \
  -Ifirmware/bc_ros/bc_algorithm \
  -Ifirmware/bc_ros/bc_util/bc_delay \
  -Ifirmware/bc_ros/bc_util/bc_log \
  -Ifirmware/bc_ros/bc_rtos \
  tests/firmware/test_battery_app.c \
  firmware/bc_ros/bc_application/app_pmic_handler.c \
  firmware/bc_ros/bc_module/pmic/bc_battery_filter.c \
  -o build/firmware/tests/test_battery_app
build/firmware/tests/test_battery_app

case "$(uname -s)" in
  Darwin) spi_flash_gc_sections=-Wl,-dead_strip ;;
  *) spi_flash_gc_sections=-Wl,--gc-sections ;;
esac
"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror \
  -Wno-unused-parameter -Wno-unused-function -Wno-pedantic \
  -O2 -g -fsanitize=address,undefined \
  -ffunction-sections -fdata-sections "$spi_flash_gc_sections" \
  -DSUDO_VOICE_ONLY -DHANDWARE_1_23_1 -DHANDWARE_1_23_2 \
  -DHARDWARE_ARCH_TYPE_NORDIC=1 \
  -Itests/firmware/spi_flash \
  -Ifirmware/bc_ros/bc_module/file/LittleFS \
  -Ifirmware/bc_ros/bc_module/spi_flash \
  -Ifirmware/bc_ros/bc_module/spi_flash/sfud/inc \
  firmware/bc_ros/bc_driver/bsp/src/bsp_spi2.c \
  firmware/bc_ros/bc_module/spi_flash/bc_spi_flash.c \
  firmware/bc_ros/bc_module/spi_flash/bc_spi_flash_port.c \
  firmware/bc_ros/bc_module/spi_flash/sfud/port/sfud_port.c \
  firmware/bc_ros/bc_module/file/LittleFS/lfs_port.c \
  tests/firmware/test_spi_flash_faults.c \
  -o build/firmware/tests/test_spi_flash_faults
build/firmware/tests/test_spi_flash_faults

"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror \
  -Wno-unused-parameter -Wno-unused-function -Wno-pedantic \
  -O2 -g -fsanitize=address,undefined \
  -ffunction-sections -fdata-sections "$spi_flash_gc_sections" \
  -DSUDO_VOICE_ONLY -DHANDWARE_1_23_1 -DHANDWARE_1_23_2 \
  -DHARDWARE_ARCH_TYPE_NORDIC=1 \
  -Itests/firmware/spi_flash \
  -Ifirmware/bc_ros/bc_module/spi_flash \
  -Ifirmware/bc_ros/bc_module/spi_flash/sfud/inc \
  firmware/bc_ros/bc_driver/bsp/src/bsp_spi2.c \
  firmware/bc_ros/bc_module/spi_flash/bc_spi_flash.c \
  firmware/bc_ros/bc_module/spi_flash/bc_spi_flash_port.c \
  firmware/bc_ros/bc_module/spi_flash/sfud/port/sfud_port.c \
  firmware/bc_ros/bc_module/spi_flash/sfud/src/sfud.c \
  tests/firmware/test_spi_flash_sfud_lengths.c \
  -o build/firmware/tests/test_spi_flash_sfud_lengths
build/firmware/tests/test_spi_flash_sfud_lengths

"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g \
  -fsanitize=address,undefined \
  -DSUDO_GNU_SBRK_HOST_TEST \
  -Ifirmware/gnu \
  firmware/gnu/sbrk.c \
  tests/firmware/test_gnu_sbrk.c \
  -o build/firmware/tests/test_gnu_sbrk
build/firmware/tests/test_gnu_sbrk

"$firmware_host_cc" -std=c99 -Wall -Wextra -Werror -pedantic -g \
  -fsanitize=address,undefined \
  -DSUDO_GNU_LOCK_HOST_TEST \
  -Ifirmware/gnu \
  firmware/gnu/newlib_locks.c \
  tests/firmware/test_gnu_newlib_locks.c \
  -o build/firmware/tests/test_gnu_newlib_locks
build/firmware/tests/test_gnu_newlib_locks
