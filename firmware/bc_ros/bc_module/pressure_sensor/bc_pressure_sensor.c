#include "bc_pressure_sensor.h"


#include "q_device.h"

#include "bc_logger.h"
#include "bc_delay.h"
#include "bc_ldo_switch.h"





static q_device_t *pressure_sensor_adc_device = NULL;




uint16_t bc_pressure_sensor_get_adc_value(void)
{
	uint16_t temp = 0;
	uint16_t adc_temp = 0;
	bc_ldo_pressure_sensors_power_on();
	q_device_open(pressure_sensor_adc_device);
	bc_delay_ms(50);
	for(uint8_t i = 0; i < 3; i++)
	{
		q_device_read(pressure_sensor_adc_device,0,&adc_temp,1);
		temp += adc_temp;
		adc_temp = 0;
	}
	q_device_close(pressure_sensor_adc_device);
	temp = temp /3;
	BC_LOG_INFO("bat temp:%d \r\n",temp);
	bc_ldo_pressure_sensors_power_off();
	return temp ;	
}








void bc_pressure_sensor_adc_devicet_adc_find(void)
{
	pressure_sensor_adc_device = q_device_find("ts2323a_adc");
	q_device_assert(pressure_sensor_adc_device);
}







