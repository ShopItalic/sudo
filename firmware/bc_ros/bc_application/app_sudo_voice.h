#ifndef APP_SUDO_VOICE_H
#define APP_SUDO_VOICE_H

#include <stdbool.h>
#include <stdint.h>

/* Returns true for a command owned by the Sudo worker, including a rejected
 * or saturated request. Such commands must never fall through to the vendor
 * recording/file owner. Request/response retry is the caller's responsibility. */
bool app_sudo_voice_command(const uint8_t *packet, uint16_t length, uint32_t epoch);

#endif
