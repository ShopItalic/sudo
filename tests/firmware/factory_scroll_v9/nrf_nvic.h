#ifndef TEST_FACTORY_SCROLL_NVIC_H
#define TEST_FACTORY_SCROLL_NVIC_H
#include <stdint.h>
uint32_t sd_nvic_critical_region_enter(uint8_t *nested);
uint32_t sd_nvic_critical_region_exit(uint8_t nested);
#endif
