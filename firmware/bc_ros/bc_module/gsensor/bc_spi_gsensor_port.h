#ifndef __BC_SPI_GSENSOR_PORT_H__
#define __BC_SPI_GSENSOR_PORT_H__


#include <stdbool.h>
#include <stdint.h>

void bc_spi_gsensor_device_open(void);

void bc_spi_gsensor_device_close(void);

void bc_spi_gsensor_cs_high(void);

void bc_spi_gsensor_cs_low(void);


bool bc_spi_gsensor_write_and_read(uint8_t *write_buff,uint32_t write_length,uint8_t *read_buff,uint32_t read_length);





void bc_spi_gsensor_port_device_find(void);




bool bc_g_sensor_i2c_write(uint8_t slave_addr,uint8_t reg_addr,uint8_t *write_data,uint8_t write_length);

bool bc_g_sensor_i2c_read(uint8_t slave_addr,uint8_t reg_addr,uint8_t *read_data,uint8_t read_length);









#endif


