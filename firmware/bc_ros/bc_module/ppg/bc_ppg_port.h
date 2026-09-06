#ifndef __BC_PPG_PORT_H__
#define __BC_PPG_PORT_H__

#include <stdint.h>
#include <stdbool.h>

/*

*/
void ppg_i2c_init();

/*

*/
void ppg_i2c_open(void);

/*

*/
void ppg_i2c_close(void);

/*

*/
bool ppg_write_reg(uint8_t addr, uint8_t data);

/*

*/
uint8_t ppg_read_reg(uint8_t addr);

/*

*/
int ppg_brust_read_reg(uint8_t addr , uint8_t *buf, uint8_t length);

#endif