#ifndef __BC_PPG_DRIVER_PORT_H__
#define __BC_PPG_DRIVER_PORT_H__





#include <stdint.h>
#include <stdbool.h>

void bc_ppg_i2c_write(uint8_t slave_addr,uint16_t reg_addr,uint8_t *write_data,uint16_t write_length);

void bc_ppg_2c_read(uint8_t slave_addr,uint16_t reg_addr,uint8_t *read_data,uint16_t read_length);

void bc_ppg_i2c_open(void);

void bc_ppg_i2c_close(void);

void bc_ppg_int_io_irq_enable(void);

void bc_ppg_int_io_irq_disable(void);

bool bc_ppg_io_irq_register_callback(void *callback);

void bc_ppg_device_find(void);

void bc_ppg_i2c_bus_open(void);

void bc_ppg_i2c_bus_close(void);


void bc_ppg_reset_low(void);

void bc_ppg_reset_high(void);








#endif






