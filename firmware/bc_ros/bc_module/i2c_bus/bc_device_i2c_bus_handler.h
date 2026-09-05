#ifndef __BC_DEVICE_I2C_BUS_HANDLER_H__
#define __BC_DEVICE_I2C_BUS_HANDLER_H__








enum bc_i2c_bus_device_type
{
	DEVICE_BUS_TYPE_PPG = 0,
	DEVICE_BUS_TYPE_PMIC,
	DEVICE_BUS_TYPE_G_SENSOR,
	DEVICE_BUS_TYPE_TEMPER,
};



void bc_i2c_bus_device_open(enum bc_i2c_bus_device_type i2c_bus_device_type);

void bc_i2c_bus_device_close(enum bc_i2c_bus_device_type i2c_bus_device_type);












#endif



