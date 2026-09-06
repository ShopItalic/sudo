#include "bc_temp.h"

#include "q_device.h"

#include "math.h"

#include "bc_logger.h"
#include "bc_ldo_switch.h"
#include "bc_delay.h"

#if (HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1  || HARDWARE_156_ENABLED == 1 || HARDWARE_191_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))

#include "gxt310.h"
#include "bc_temp_port.h"

#endif

extern void power_manage(void);

static q_device_t *temper_adc_device_handler;





static uint16_t bc_temp_get_temper_volate(void)
{
	uint16_t temper_volate = 0.0;
	uint16_t temp = 0;
	uint16_t temper_temp1 = 0;
	uint16_t temper_temp2 = 0;
	q_device_open(temper_adc_device_handler);
	nrf_delay_ms(3);
	for(uint8_t i = 0; i < 3; i++)
	{
		q_device_read(temper_adc_device_handler,0,&temper_temp1,1);
		q_device_read(temper_adc_device_handler,0,&temper_temp2,1);
		if(temper_temp1 >= temper_temp2 )
		{
			temp += temper_temp1;
		}
		else
		{
			temp += temper_temp2;
		}
		
	}
	temper_volate = (temp / 3);// * 3.6 / 1024;
	q_device_close(temper_adc_device_handler);
	return temper_volate ;	
}


uint16_t bc_temp_get_temperature_value(void)
{

#if (HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_1141_ENABLED == 1  || HARDWARE_1181_ENABLED == 1 || defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))		
    float temper = 0.00;
	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_temper_ic_enable();
	temper = temper + gxt310x0_temper_get();
	temper = temper + gxt310x1_temper_get();
	temper = temper + gxt310x2_temper_get();
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	
	temper = (temper / 3) * 100;
	power_manage();
	return (uint16_t)temper;
#elif (HARDWARE_156_ENABLED == 1 )		
    float temper = 0.00;
	bc_ldo_temper_power_on();
//	bc_temper_device_i2c_open();
	bc_temper_ic_enable();
	temper = temper + gxt310x0_temper_get();
	temper = temper + gxt310x1_temper_get();
	temper = temper + gxt310x2_temper_get();
	temper = temper + gxt310x3_temper_get();
	
	bc_temper_ic_disenable();
//	bc_temper_device_i2c_close();
	
	temper = (temper / 4) * 100;
	power_manage();
	return (uint16_t)temper;	
#elif (HARDWARE_191_ENABLED == 1 )	
	float temper = 0.00;
	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_temper_ic_enable();
	temper = temper + gxt310x0_temper_get();
	temper = temper + gxt310x2_temper_get();
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	
	temper = (temper / 2) * 100;
	power_manage();
	return (uint16_t)temper;
#else	
	
	uint32_t temper_volate = 0.0;
	 uint16_t temp_result_filter = 0;
	
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_451_ENABLED == 1)	

	bc_ldo_temper_power_on();
	
#endif
	
	temper_volate = bc_temp_get_temper_volate();
	BC_LOG_INFO("temper:%d   ", temper_volate);
//	R1 = R2*((ntc_volate -temper_volate) / temper_volate);
	   
	uint32_t v_vcc = 3300; //= (ntc_volate*3600) >> 12; //mV
	uint32_t v_temper = (temper_volate*3600) >> 12; //mV
	
	float gAmbR = v_temper*100000/(v_vcc - v_temper); 
	gAmbR = gAmbR/1000.0f;
	float temp_res = (1/4250.0f)*log(gAmbR/100.0f) + 1/(273.15f+25.0f);
	temp_res = 1/(temp_res) - 273.15f;
    temp_res = temp_res*100;
	BC_LOG_INFO("temp:%d \r\n", (uint16_t)temp_res);
	power_manage();
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_451_ENABLED == 1)	

	 bc_ldo_temper_power_off();
	
#endif	
	return (uint16_t)temp_res;
#endif	
	
}

void bc_temp_ppg_get_temperature_on(void)
{

	
	q_device_open(temper_adc_device_handler);

	nrf_delay_ms(3);
}

void bc_temp_ppg_get_temperature_off(void)
{
	q_device_close(temper_adc_device_handler);
}
uint16_t bc_temp_ppg_get_temperature_value(void)
{
    uint16_t temp_result_filter = 0;

	uint32_t temper_volate = 0;
//	q_device_open(temper_adc_device_handler);
//	nrf_delay_ms(3);
	{
		uint16_t temp = 0;
		uint16_t temper_temp1 = 0;
		uint16_t temper_temp2 = 0;
		for(uint8_t i = 0; i < 3; i++)
		{
			q_device_read(temper_adc_device_handler,0,&temper_temp1,1);
			q_device_read(temper_adc_device_handler,0,&temper_temp2,1);
			if(temper_temp1 >= temper_temp2 )
			{
				temp += temper_temp1;
			}
			else
			{
				temp += temper_temp2;
			}
			
		}
		temper_volate = (temp / 3);// * 3.6 / 1024;
	 }
//	q_device_close(temper_adc_device_handler);
	BC_LOG_INFO("temper:%d  ", temper_volate);
//	R1 = R2*((ntc_volate -temper_volate) / temper_volate);
	   
	uint32_t v_vcc = 3300; //= (ntc_volate*3600) >> 12; //mV
	uint32_t v_temper = (temper_volate*3600) >> 12; //mV
	
	float gAmbR = v_temper*100000/(v_vcc - v_temper); 
	gAmbR = gAmbR/1000.0f;
	float temp_res = (1/4250.0f)*log(gAmbR/100.0f) + 1/(273.15f+25.0f);
	temp_res = 1/(temp_res) - 273.15f;
    temp_res = temp_res*100;
	BC_LOG_INFO("temp:%d", (uint16_t)temp_res);	
	power_manage();
	return (uint16_t)temp_res;
	
}




uint16_t bc_temp_get_temper_adc_value(void)
{

#if (HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1  || HARDWARE_1141_ENABLED == 1  || HARDWARE_1181_ENABLED == 1 || defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))		
    float temper = 0.00;
	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_delay_ms(20);
	bc_temper_ic_enable();
	temper = temper + gxt310x0_temper_get();
	temper = temper + gxt310x1_temper_get();
	temper = temper + gxt310x2_temper_get();
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	bc_ldo_temper_power_off();
	
	temper = (temper / 3) * 100;
	return (uint16_t)temper;
#elif (HARDWARE_156_ENABLED == 1 )		
    float temper = 0.00;
	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_delay_ms(20);
	bc_temper_ic_enable();
	
	temper = temper + gxt310x0_temper_get();
	temper = temper + gxt310x1_temper_get();
	temper = temper + gxt310x2_temper_get();
	temper = temper + gxt310x3_temper_get();
	
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	bc_ldo_temper_power_off();
	
	temper = (temper / 4) * 100;
	return (uint16_t)temper;	
#elif (HARDWARE_191_ENABLED == 1 )	
	float temper = 0.00;
	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_delay_ms(20);
	bc_temper_ic_enable();
	temper = temper + gxt310x0_temper_get();
	temper = temper + gxt310x2_temper_get();
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	bc_ldo_temper_power_off();
	
	temper = (temper / 2) * 100;
	return (uint16_t)temper;
#else		
	
	uint16_t temp = 0;
	uint16_t temper_temp = 0;
	

#if (HARDWARE_153_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_451_ENABLED == 1)	

	bc_ldo_temper_power_on();
	
#endif	

	q_device_open(temper_adc_device_handler);
	bc_delay_ms(10);
	for(uint8_t i = 0; i < 3; i++)
	{
		q_device_read(temper_adc_device_handler,0,&temper_temp,1);
		temp += temper_temp;
	}
	q_device_close(temper_adc_device_handler);
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_413_ENABLED == 1  || HARDWARE_451_ENABLED == 1)	

	 bc_ldo_temper_power_off();
	
#endif	
	temp = temp /3;
	return temp ;	
#endif

}

bool bc_temp_temperature_check(void)
{

#if (HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_1141_ENABLED == 1  || HARDWARE_1181_ENABLED == 1 || defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))		
	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_delay_ms(20);
  bc_temper_ic_enable();
	uint8_t data[3] = {0};
	data[0] =  gxt310x0_temper_get_id();
    data[1] =  gxt310x1_temper_get_id();
	data[2] =  gxt310x2_temper_get_id();
	gxt310_temper_set_frequency_reg(3);
  bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
  
	bc_ldo_temper_power_off();
	
	if(data[0] != 0x50 || data[1] != 0x50 || data[2] != 0x50)
	{
		return false;
	}
	return true;
#elif ( HARDWARE_156_ENABLED == 1 )		
	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_temper_ic_enable();
	bc_delay_ms(20);

	uint8_t data[4] = {0};
	data[0] =  gxt310x0_temper_get_id();
    data[1] =  gxt310x1_temper_get_id();
	data[2] =  gxt310x2_temper_get_id();
	data[3] =  gxt310x2_temper_get_id();
	gxt310_temper_set_frequency_reg(4);
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	bc_ldo_temper_power_off();
	
	if(data[0] != 0x50 || data[1] != 0x50 || data[2] != 0x50 || data[3] != 0x50)
	{
		return false;
	}
	return true;	
#elif (HARDWARE_191_ENABLED == 1 )	
	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_temper_ic_enable();
	bc_delay_ms(20);
    
	uint8_t data[2] = {0};
	data[0] =  gxt310x0_temper_get_id();
    data[1] =  gxt310x2_temper_get_id();
	gxt310_temper_set_frequency_reg(3);
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	bc_ldo_temper_power_off();
	
	if(data[0] != 0x50 || data[1] != 0x50 )
	{
		return false;
	}
	return true;
#else
	
	uint16_t temper_temp = 0;
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_451_ENABLED == 1)	

	bc_ldo_temper_power_on();
	
#endif	
	q_device_open(temper_adc_device_handler);
	bc_delay_ms(10);
	
	q_device_read(temper_adc_device_handler,0,&temper_temp,1);
	if(temper_temp < 100)
	{
		q_device_close(temper_adc_device_handler);

		return false;
	}
	q_device_close(temper_adc_device_handler);
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_451_ENABLED == 1)	

	 bc_ldo_temper_power_off();
	
#endif	
	return true;
#endif	
}

extern void power_manage(void);

void bc_temper_value_get_rawdata(uint16_t *temper_0,uint16_t *temper_1,uint16_t *temper_2,uint16_t *temper_3)
{
#if (HARDWARE_1121_ENABLED == 1 || HARDWARE_1141_ENABLED == 1  || HARDWARE_1181_ENABLED == 1 || defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))		

	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
  bc_temper_ic_enable();
//	bc_delay_ms(20);
	*temper_0 =  gxt310x0_temper_get() * 100;
    *temper_1 =  gxt310x1_temper_get() * 100;
	*temper_2 =  gxt310x2_temper_get() * 100;
	power_manage();
  bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	bc_ldo_temper_power_off();


#elif (HARDWARE_191_ENABLED == 1 )		

	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_temper_ic_enable();
//	bc_delay_ms(20);
	
	*temper_0 =  gxt310x0_temper_get() * 100;
    *temper_1 =  gxt310x2_temper_get() * 100;
	
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	bc_ldo_temper_power_off();	
	
#elif (  HARDWARE_156_ENABLED == 1  || HARDWARE_158_ENABLED == 1 )		
	
	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_temper_ic_enable();
	bc_delay_ms(20);
  
	*temper_0 =  gxt310x0_temper_get() * 100;
  *temper_1 =  gxt310x1_temper_get() * 100;
	*temper_2 =  gxt310x2_temper_get() * 100;
	*temper_3 =  gxt310x2_temper_get() * 100;
	
	gxt310_temper_get_config_all_device();
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	bc_ldo_temper_power_off();
	
#endif	
}


void bc_temper_value_get(uint8_t *data)
{
#if (HARDWARE_1121_ENABLED == 1 || HARDWARE_1141_ENABLED == 1  || HARDWARE_1181_ENABLED == 1 || defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))		

	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_temper_ic_enable();
	bc_delay_ms(20);
	data[0] = 3;
	*(uint16_t*)&data[1] =  gxt310x0_temper_get() * 100;
    *(uint16_t*)&data[3] =  gxt310x1_temper_get() * 100;
	*(uint16_t*)&data[5] =  gxt310x2_temper_get() * 100;
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	bc_ldo_temper_power_off();


#elif (HARDWARE_191_ENABLED == 1 )		

	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_temper_ic_enable();
	bc_delay_ms(20);
	
	data[0] = 2;
	*(uint16_t*)&data[1] =  gxt310x0_temper_get() * 100;
    *(uint16_t*)&data[3] =  gxt310x2_temper_get() * 100;
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	bc_ldo_temper_power_off();	
	
#elif (  HARDWARE_156_ENABLED == 1  || HARDWARE_158_ENABLED == 1 )		
	
	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_temper_ic_enable();
	bc_delay_ms(20);
	data[0] = 4;
	*(uint16_t*)&data[1] =  gxt310x0_temper_get() * 100;
    *(uint16_t*)&data[3] =  gxt310x1_temper_get() * 100;
	*(uint16_t*)&data[5] =  gxt310x2_temper_get() * 100;
	*(uint16_t*)&data[7] =  gxt310x2_temper_get() * 100;
	
	gxt310_temper_get_config_all_device();
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	bc_ldo_temper_power_off();
	
#endif	
}


void bc_temper_id_get(uint8_t *data)
{
#if (HARDWARE_1121_ENABLED == 1 || HARDWARE_1141_ENABLED == 1  || HARDWARE_1181_ENABLED == 1 || defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))		

	bc_ldo_temper_power_on();
  
	bc_temper_device_i2c_open();
  bc_temper_ic_enable();
	bc_delay_ms(20);
	data[0] = 3;
	data[1] =  gxt310x0_temper_get_id();
    data[2] =  gxt310x1_temper_get_id();
	data[3] =  gxt310x2_temper_get_id();
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	bc_ldo_temper_power_off();


#elif (HARDWARE_191_ENABLED == 1 )		

	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_temper_ic_enable();
	bc_delay_ms(20);
	
	data[0] = 2;
	data[1] =  gxt310x0_temper_get_id();
    data[2] =  gxt310x2_temper_get_id();
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	bc_ldo_temper_power_off();	
	
#elif (  HARDWARE_156_ENABLED == 1  || HARDWARE_158_ENABLED == 1 )		
	
	bc_ldo_temper_power_on();
	bc_temper_device_i2c_open();
	bc_temper_ic_enable();
	bc_delay_ms(20);
	data[0] = 4;
	data[1] =  gxt310x0_temper_get_id();
    data[2] =  gxt310x1_temper_get_id();
	data[3] =  gxt310x2_temper_get_id();
	data[4] =  gxt310x3_temper_get_id();
	
	gxt310_temper_get_config_all_device();
	bc_temper_ic_disenable();
	bc_temper_device_i2c_close();
	bc_ldo_temper_power_off();
	
#endif	
}


bool bc_temper_check(void)
{
	uint16_t temper =  bc_temp_get_temperature_value();
	if(temper /100 >= 28)
	{
		return true;
	}
	
	return false;
}

void bc_temper_ic_enable(void)
{
#if (HARDWARE_1121_ENABLED == 1 || HARDWARE_1141_ENABLED == 1  || HARDWARE_1181_ENABLED == 1 || defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))		

	gxt310x0_switch_mode(true);
	gxt310x1_switch_mode(true);
	gxt310x2_switch_mode(true);


#elif (HARDWARE_191_ENABLED == 1 )		

	gxt310x0_switch_mode(true);
	gxt310x2_switch_mode(true);	
	
#elif (  HARDWARE_156_ENABLED == 1  || HARDWARE_158_ENABLED == 1 )		
	
	gxt310x0_switch_mode(true);
	gxt310x1_switch_mode(true);
	gxt310x2_switch_mode(true);
	gxt310x3_switch_mode(true);
	
#endif		
}

void bc_temper_ic_disenable(void)
{
#if (HARDWARE_1121_ENABLED == 1 || HARDWARE_1141_ENABLED == 1  || HARDWARE_1181_ENABLED == 1 || defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))		

	gxt310x0_switch_mode(false);
	gxt310x1_switch_mode(false);
	gxt310x2_switch_mode(false);


#elif (HARDWARE_191_ENABLED == 1 )		

	gxt310x0_switch_mode(false);
	gxt310x2_switch_mode(false);
	
#elif (  HARDWARE_156_ENABLED == 1  || HARDWARE_158_ENABLED == 1 )		
	
	gxt310x0_switch_mode(false);
	gxt310x1_switch_mode(false);
	gxt310x2_switch_mode(false);
	gxt310x3_switch_mode(false);
	
#endif	
	
}

void bc_temp_temperature_adc_find(void)
{
	temper_adc_device_handler = q_device_find("temper_adc_1");
	q_device_assert(temper_adc_device_handler);
}









