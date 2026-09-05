#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/firmware/tests
cc -std=c99 -Wall -Wextra -Werror -g -fsanitize=address,undefined \
  -Ifirmware/bc_ros/bc_module/ble/inc \
  firmware/bc_ros/bc_module/ble/src/bc_ble_tx.c \
  firmware/bc_ros/bc_module/ble/src/bc_file_transfer.c \
  tests/firmware/test_transfer.c -o build/firmware/tests/test_transfer
build/firmware/tests/test_transfer
