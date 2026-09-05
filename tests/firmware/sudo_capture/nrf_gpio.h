#ifndef SUDO_CAPTURE_TEST_NRF_GPIO_H
#define SUDO_CAPTURE_TEST_NRF_GPIO_H

#include <stdint.h>

/* Nordic's GPIO numbering is port * 32 + pin. */
#define NRF_GPIO_PIN_MAP(port, pin) \
    ((uint8_t)((((uint32_t)(port)) << 5) | ((uint32_t)(pin) & 0x1fU)))

#endif
