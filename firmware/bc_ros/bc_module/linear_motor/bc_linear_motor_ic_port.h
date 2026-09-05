#ifndef __BC_LINEAR_MOBOR_IC_PORT_H__
#define __BC_LINEAR_MOBOR_IC_PORT_H__


#include <stdbool.h>
#include <stdint.h>


bool bc_linear_motor_i2c_write(uint8_t slave_addr,uint8_t reg_addr,uint8_t *write_data,uint8_t write_length);

bool bc_linear_motor_i2c_read(uint8_t slave_addr,uint8_t reg_addr,uint8_t *read_data,uint8_t read_length);

void bc_linear_motor_int_io_irq_enable(void);

void bc_linear_motor_int_io_irq_disable(void);

void bc_linear_motor_i2c_open(void);

void bc_linear_motor_i2c_close(void);

void bc_linear_motor_rst_open(void);

void bc_linear_motor_rst_close(void);

void bc_linear_motor_timer_start(void);

void bc_linear_motor_timer_stop(void);

void bc_linear_motor_rst_low(void);

void bc_linear_motor_rst_high(void);

void bc_linear_motor_device_i2c_find(void);


void bc_motor_stop(void);

#endif
