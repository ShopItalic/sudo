#ifndef __BC_G_SENSOR_DEVICE_PORT_H__
#define __BC_G_SENSOR_DEVICE_PORT_H__


#include <stdint.h>
#include <stdbool.h>


bool bc_g_sensor_i2c_write(uint8_t slave_addr,uint8_t reg_addr,uint8_t *write_data,uint8_t write_length);

bool bc_g_sensor_i2c_read(uint8_t slave_addr,uint8_t reg_addr,uint8_t *read_data,uint8_t read_length);

void bc_g_sensor_int_io_irq_enable(void);

void bc_g_sensor_int_io_irq_disable(void);

void bc_g_sensor_i2c_open(void);

void bc_g_sensor_i2c_close(void);

bool bc_g_sensor_io_irq_register_callback(void *callback);
	
void bc_g_sensor_device_i2c_find(void);


void bc_g_sensor_i2c_bus_open(void);

void bc_g_sensor_i2c_bus_close(void);














#endif

