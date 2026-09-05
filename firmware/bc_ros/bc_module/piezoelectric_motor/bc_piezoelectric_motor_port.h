#ifndef __BC_PIEZOELECTRIC_MOBOR_PORT_H__
#define __BC_PIEZOELECTRIC_MOBOR_PORT_H__


#include <stdbool.h>
#include <stdint.h>

bool bc_piezoelectric_motor_i2c_write(uint8_t slave_addr,uint8_t reg_addr,uint8_t *write_data,uint8_t write_length);

bool bc_piezoelectric_motor_i2c_read(uint8_t slave_addr,uint8_t reg_addr,uint8_t *read_data,uint8_t read_length);

void bc_piezoelectric_motor_int_io_irq_enable(void);
void bc_piezoelectric_motor_int_io_irq_disable(void);

void bc_piezoelectric_motor_i2c_open(void);

void bc_piezoelectric_motor_i2c_close(void);


bool bc_piezoelectric_motor_io_irq_register_callback(void *callback);
void bc_piezoelectric_motor_device_i2c_find(void);







#endif

