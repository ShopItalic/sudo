#ifndef __BC_GSENSOR_PORT_H__
#define __BC_GSENSOR_PORT_H__

#include <stdint.h>

void gsensor_i2c_init(void);
void gsensor_i2c_open(void);
void gsensor_i2c_close(void);
int32_t gsensor_writereg(uint8_t reg_add,uint8_t reg_dat);
int32_t gsensor_readreg(uint8_t reg_add,uint8_t *buf,uint16_t num);

#endif
