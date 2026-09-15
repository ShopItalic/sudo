#ifndef P05_TEST_CMD_H
#define P05_TEST_CMD_H
#include <stdint.h>
struct app_cmd_package { uint8_t frame_type, frame_id, cmd, subcmd; uint8_t data[243]; };
#endif
