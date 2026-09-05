#ifndef __BC_SPI_FLASH_PORT_H__
#define __BC_SPI_FLASH_PORT_H__


#include <stdbool.h>
#include <stdint.h>

void bc_spi_flash_device_open(void);

void bc_spi_flash_device_close(void);

void bc_spi_flash_cs_high(void);

void bc_spi_flash_cs_low(void);


bool bc_spi_flash_write_and_read(uint8_t *write_buff,uint32_t write_length,uint8_t *read_buff,uint32_t read_length);





 void bc_spi_flash_device_find(void);














#endif


