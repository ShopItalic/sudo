#ifndef TEST_SPI_FLASH_NRF_GPIO_H
#define TEST_SPI_FLASH_NRF_GPIO_H

#include <stdint.h>

#define NRF_GPIO_PIN_MAP(port, pin) ((uint32_t)((port) * 32u + (pin)))

void nrf_gpio_cfg_output(uint32_t pin);
void nrf_gpio_cfg_default(uint32_t pin);
void nrf_gpio_pin_set(uint32_t pin);
void nrf_gpio_pin_clear(uint32_t pin);

#endif
