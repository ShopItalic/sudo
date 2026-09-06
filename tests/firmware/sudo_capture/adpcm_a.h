#ifndef SUDO_CAPTURE_TEST_ADPCM_A_H
#define SUDO_CAPTURE_TEST_ADPCM_A_H

#include <stdint.h>

/* Minimal ABI-compatible declarations for the two supplier calls used by
 * app_sudo_capture.c. The implementation records every input and emits a
 * deterministic 220-byte frame for host assertions. */
typedef struct adpcm_state_t {
    short valprev;
    char index;
} adpcm_state;

typedef struct {
    adpcm_state mono_state;
} MonoAdpcmProcessor;

void mono_adpcm_init(MonoAdpcmProcessor *processor);
void adpcm_encoder(short *indata, char *outdata, int len, adpcm_state *state);

#endif
