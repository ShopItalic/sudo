#ifndef __BC_SPI_FLASH_H__
#define __BC_SPI_FLASH_H__


#include <stdint.h>
#include <stdbool.h>

void spi_flash_device_lowpower(void);

void spi_flash_device_wakeup(void);

uint32_t spi_flash_device_get_id(void);

bool spi_flash_device_check_id(void);



 void bc_spi_flash_device_find(void);






















#endif

