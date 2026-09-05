#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/firmware/tests
app_package_source=${RECORDING_APP_PACKAGE_SOURCE:-firmware/bc_ros/bc_application/app_package.c}
cmd_handler_source=${RECORDING_CMD_HANDLER_SOURCE:-firmware/bc_ros/bc_application/app_cmd_handler.c}
python3 tools/firmware/extract_recording_sources.py \
  --app-package-source "$app_package_source" \
  --cmd-handler-source "$cmd_handler_source" \
  --app-package-output build/firmware/tests/recording_app_package.c \
  --cmd-handler-output build/firmware/tests/recording_cmd_handler.c
cc -std=c99 -Wall -Wextra -Werror -g -fsanitize=address,undefined \
  -Itests/firmware \
  -Ifirmware/bc_ros/bc_application \
  -Ifirmware/bc_ros/bc_config \
  -Ifirmware/bc_ros/bc_module/ble/inc \
  -Ifirmware/bc_ros/bc_module/queue \
  build/firmware/tests/recording_app_package.c \
  build/firmware/tests/recording_cmd_handler.c \
  tests/firmware/test_recording_commands.c -o build/firmware/tests/test_recording_commands
build/firmware/tests/test_recording_commands
cc -std=c99 -Wall -Wextra -Werror -g -fsanitize=address,undefined \
  -Ifirmware/bc_ros/bc_module/ble/inc \
  firmware/bc_ros/bc_module/ble/src/bc_ble_tx.c \
  firmware/bc_ros/bc_module/ble/src/bc_file_transfer.c \
  tests/firmware/test_transfer.c -o build/firmware/tests/test_transfer
build/firmware/tests/test_transfer
