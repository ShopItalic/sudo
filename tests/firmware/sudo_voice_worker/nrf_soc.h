#ifndef SUDO_VOICE_WORKER_NRF_SOC_H
#define SUDO_VOICE_WORKER_NRF_SOC_H

#include <stdint.h>

#include "nrf_error.h"

uint32_t sd_rand_application_vector_get(uint8_t *buffer, uint8_t length);

#endif
