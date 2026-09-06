#ifndef TEST_MOTOR_NRF_GPIO_H
#define TEST_MOTOR_NRF_GPIO_H
#include <stdint.h>
#define NRF_GPIO_PIN_MAP(port, pin) ((uint32_t)(((port) << 5) | (pin)))
#endif
