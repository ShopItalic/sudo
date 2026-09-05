#ifndef __BC_TEMP_PORT_H__
#define __BC_TEMP_PORT_H__




#include <stdint.h>
#include <stdbool.h>





bool bc_temper_i2c_write(uint8_t slave_addr,uint8_t reg_addr,uint8_t *write_data,uint8_t write_length);

bool bc_temper_i2c_read(uint8_t slave_addr,uint8_t reg_addr,uint8_t *read_data,uint8_t read_length);

void bc_temper_device_i2c_find(void);

void bc_temper_i2c_bus_open(void);

void bc_temper_i2c_bus_close(void);

void bc_temper_device_i2c_open(void);

void bc_temper_device_i2c_close(void);



#endif


