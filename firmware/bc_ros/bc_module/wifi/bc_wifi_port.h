#ifndef __BC_WIFI_PORT_H__
#define __BC_WIFI_PORT_H__


#include <stdint.h>
#include <stdbool.h>





void bc_wifi_device_enable(void);

void bc_wifi_device_disable(void);

void bc_wifi_device_find(void);


void bc_wifi_spi_device_open(void);

void bc_wifi_spi_device_close(void);

void bc_wifi_spi_cs_high(void);

void bc_wifi_spi_cs_low(void);

void bc_wifi_spi_write_test(uint32_t seq,uint16_t length);

bool bc_wifi_spi_write_and_read(uint8_t *write_buff,uint32_t write_length,uint8_t *read_buff,uint32_t read_length);








#endif


