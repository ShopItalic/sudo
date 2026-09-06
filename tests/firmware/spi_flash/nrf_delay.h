#ifndef TEST_SPI_FLASH_NRF_DELAY_H
#define TEST_SPI_FLASH_NRF_DELAY_H

#include <stdint.h>

void test_nrf_delay_us(uint32_t microseconds);
void test_nrf_delay_ms(uint32_t milliseconds);

#define nrf_delay_us test_nrf_delay_us
#define nrf_delay_ms test_nrf_delay_ms

#endif
