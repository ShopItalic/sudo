#include "bc_device_i2c_bus_handler.h"


#include "bc_ppg_driver_port.h"
#include "bc_g_sensor_device_port.h"
#include "bc_pmic_device_port.h"
#include "bc_temp_port.h"


struct __attribute__((__packed__)) bc_i2c_device
{
	unsigned int ppg_i2c : 1;
	unsigned int pmic_i2c : 1;
	unsigned int g_sensor_i2c : 1;
	unsigned int temper_i2c : 1;
	unsigned int  : 4;
};




enum bc_i2c_bus_flag
{
	I2C_BUS_DISENABLE =0,
	I2C_BUS_ENABLE =1,
};


static struct bc_i2c_device i2c_bus_device ={0};



void bc_i2c_bus_device_open(enum bc_i2c_bus_device_type i2c_bus_device_type)
{
	
#if defined(HANDWARE_4_5_1)	
	switch(i2c_bus_device_type)
	{
		case DEVICE_BUS_TYPE_PPG:
		{
			bc_ppg_i2c_bus_open();
			i2c_bus_device.ppg_i2c = I2C_BUS_ENABLE;
			break;
		}
		case DEVICE_BUS_TYPE_PMIC:
		{
			pmic_i2c_bus_open();
			i2c_bus_device.pmic_i2c = I2C_BUS_ENABLE;
			break;
		}
		case DEVICE_BUS_TYPE_G_SENSOR:
		{
			bc_g_sensor_i2c_bus_open();
			i2c_bus_device.g_sensor_i2c = I2C_BUS_ENABLE;
			break;
		}
		default:
		{
			break;
		}
	}
#elif defined(HANDWARE_1_5_6)		
	switch(i2c_bus_device_type)
	{
		case DEVICE_BUS_TYPE_PPG:
		{
			bc_ppg_i2c_bus_open();
			i2c_bus_device.ppg_i2c = I2C_BUS_ENABLE;
			break;
		}

		case DEVICE_BUS_TYPE_TEMPER:
		{
			if(i2c_bus_device.ppg_i2c)
			{
				bc_ppg_i2c_bus_close();
			}
			bc_temper_i2c_bus_open();
			i2c_bus_device.temper_i2c = I2C_BUS_ENABLE;
			break;
		}
		default:
		{
			break;
		}
	}
#endif
}


void bc_i2c_bus_device_close(enum bc_i2c_bus_device_type i2c_bus_device_type)
{
#if defined(HANDWARE_4_5_1)		
	switch(i2c_bus_device_type)
	{
		case DEVICE_BUS_TYPE_PPG:
		{
			if(!i2c_bus_device.pmic_i2c && !i2c_bus_device.g_sensor_i2c)
			{
				bc_ppg_i2c_bus_close();
			}
			i2c_bus_device.ppg_i2c = I2C_BUS_DISENABLE;
			break;
		}
		case DEVICE_BUS_TYPE_PMIC:
		{
			if(!i2c_bus_device.ppg_i2c && !i2c_bus_device.g_sensor_i2c)
			{
				pmic_i2c_bus_close();
			}
			i2c_bus_device.pmic_i2c = I2C_BUS_DISENABLE;
			break;
		}
		case DEVICE_BUS_TYPE_G_SENSOR:
		{
			if(!i2c_bus_device.ppg_i2c && !i2c_bus_device.pmic_i2c)
			{
				bc_g_sensor_i2c_bus_close();
			}
			i2c_bus_device.g_sensor_i2c = I2C_BUS_DISENABLE;
			break;
		}
		default:
		{
			break;
		}
	}
#elif defined(HANDWARE_1_5_6)		
	switch(i2c_bus_device_type)
	{
		case DEVICE_BUS_TYPE_PPG:
		{
			if(!i2c_bus_device.temper_i2c)
			{
				bc_ppg_i2c_bus_close();
			}
			i2c_bus_device.ppg_i2c = I2C_BUS_DISENABLE;
			break;
		}
		case DEVICE_BUS_TYPE_TEMPER:
		{
			
//			if(!i2c_bus_device.ppg_i2c )
//			{
				bc_temper_i2c_bus_close();
//			}
			if(i2c_bus_device.ppg_i2c )
			{
				bc_ppg_i2c_bus_open();
			}
			i2c_bus_device.temper_i2c = I2C_BUS_DISENABLE;
			break;
		}
		default:
		{
			break;
		}
	}	
#endif	
}

















