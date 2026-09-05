#ifndef __BC_FUEL_GAUGE_PORT_H__
#define __BC_FUEL_GAUGE_PORT_H__


#include <stdbool.h>
#include <stdint.h>


bool cw221x_i2c_write(uint8_t slave_addr,uint8_t reg_add ,uint8_t *data,uint8_t length);

bool cw221x_i2c_read(uint8_t slave_addr,uint8_t reg_add ,uint8_t *data,uint8_t length);


int cw221x_i2c_close(void);

int cw221x_i2c_open(void);

void bc_cw221x_device_find(void);

#endif
