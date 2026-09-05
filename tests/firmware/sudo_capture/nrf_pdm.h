#ifndef SUDO_CAPTURE_TEST_NRF_PDM_H
#define SUDO_CAPTURE_TEST_NRF_PDM_H

#include <stdbool.h>
#include <stdint.h>

/* nrf_pdm_enable_check() is the only HAL query used by this adapter. */
extern volatile uint32_t test_pdm_enable_register;
static inline bool nrf_pdm_enable_check(void)
{
    return test_pdm_enable_register != 0U;
}

#endif
