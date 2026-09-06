#ifndef __BC_PUF_I2C_DRIVER_H__
#define __BC_PUF_I2C_DRIVER_H__







#include <stdbool.h>
#include <stdint.h>



bool bc_buf_i2c_device_write(uint8_t reg_add ,uint8_t *data,uint8_t length);

bool bc_buf_i2c_device_read(uint8_t reg_add ,uint8_t *data,uint8_t length);								 
									 
void bc_buf_i2c_device_open(void);

void bc_buf_i2c_device_close(void);										 
									 
void bc_buf_i2c_device_find(void);








#endif



