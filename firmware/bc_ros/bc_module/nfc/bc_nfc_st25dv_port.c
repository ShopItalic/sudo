#include "bc_nfc_st25dv_port.h"

#include "q_device.h"

#include <string.h>
#include "bc_logger.h"
#include "nrf_gpio.h"

#if ( HARDWARE_158_ENABLED == 1 )	

#define ST25_LPD_GPIO           NRF_GPIO_PIN_MAP(0,30)

#elif ( HARDWARE_156_ENABLED == 1)	

#define ST25_LPD_GPIO           NRF_GPIO_PIN_MAP(0,3);

#endif	

static q_device_t *i2c_dev = NULL;                                     //设备描述

static struct i2c_package i2c_pack = {
										 .slave_addr = 0x12 <<1,        //i2c从机地址
										 .write_length = 1,
										 .read_length = 1,
//	                                    .reg_addr_bit = I2C_REG_ADDR_8bit,
									 };

																				 
																				 



																				 

void bc_nfc_st25dv_i2c_write(uint8_t slave_addr,uint16_t reg_addr,uint8_t *write_data,uint16_t write_length)
{
	i2c_pack.reg_addr = reg_addr;
	i2c_pack.slave_addr = slave_addr  ;
	
	if(write_data != NULL)
	{
//		memcpy(i2c_pack.write_buff,write_data,write_length);
		i2c_pack.write_buff = write_data;
	}
	i2c_pack.write_length = write_length;
	q_device_write(i2c_dev,0,&i2c_pack,0);  

}

void bc_nfc_st25dv_i2c_read(uint8_t slave_addr,uint16_t reg_addr,uint8_t *read_data,uint16_t read_length)
{
	i2c_pack.reg_addr = reg_addr;
	i2c_pack.slave_addr = slave_addr ;
	i2c_pack.read_length = read_length;
	i2c_pack.read_buff = read_data;
	q_device_read(i2c_dev,0,&i2c_pack,0);

	if(read_data != NULL)
	{
//		memcpy(read_data,i2c_pack.read_buff,i2c_pack.read_length);
	}
}

void bc_nfc_st25dv_i2c_open(void)
{


#if ( HARDWARE_158_ENABLED == 1 )	

//	nrf_gpio_pin_clear(ST25_LPD_GPIO);
    nrf_gpio_pin_set(ST25_LPD_GPIO);
//	nrf_gpio_cfg(
//            ST25_LPD_GPIO,
//            NRF_GPIO_PIN_DIR_OUTPUT,
//            NRF_GPIO_PIN_INPUT_DISCONNECT,
//            NRF_GPIO_PIN_PULLUP,
//            NRF_GPIO_PIN_H0H1,
//            NRF_GPIO_PIN_NOSENSE);	

#elif ( HARDWARE_156_ENABLED == 1)	

	nrf_gpio_pin_clear(ST25_LPD_GPIO);

#endif		
	
    q_device_open(i2c_dev);                                                       //打开设备
	

}

void bc_nfc_st25dv_i2c_close(void)
{
	  
                                                                     //关闭设备
#if ( HARDWARE_158_ENABLED == 1 )	

	nrf_gpio_pin_clear(ST25_LPD_GPIO);
//	nrf_gpio_cfg(
//            ST25_LPD_GPIO,
//            NRF_GPIO_PIN_DIR_OUTPUT,
//            NRF_GPIO_PIN_INPUT_DISCONNECT,
//            NRF_GPIO_PIN_PULLUP,
//            NRF_GPIO_PIN_H0H1,
//            NRF_GPIO_PIN_NOSENSE);


#elif ( HARDWARE_156_ENABLED == 1)	

nrf_gpio_cfg(
            ST25_LPD_GPIO,
            NRF_GPIO_PIN_DIR_OUTPUT,
            NRF_GPIO_PIN_INPUT_DISCONNECT,
            NRF_GPIO_PIN_PULLUP,
            NRF_GPIO_PIN_H0H1,
            NRF_GPIO_PIN_NOSENSE);

#endif		
	q_device_close(i2c_dev); 
	
}



void bc_nfc_st25dv_device_find(void)
{
	i2c_dev = q_device_find("i2c_5");
	q_device_assert(i2c_dev);
	
#if ( HARDWARE_158_ENABLED == 1 )	

	nrf_gpio_cfg_output(ST25_LPD_GPIO);
    nrf_gpio_pin_clear(ST25_LPD_GPIO);
//	nrf_gpio_cfg(
//            ST25_LPD_GPIO,
//            NRF_GPIO_PIN_DIR_OUTPUT,
//            NRF_GPIO_PIN_INPUT_DISCONNECT,
//            NRF_GPIO_PIN_PULLUP,
//            NRF_GPIO_PIN_H0H1,
//            NRF_GPIO_PIN_NOSENSE);

#elif ( HARDWARE_156_ENABLED == 1)	

    nrf_gpio_cfg_output(ST25_LPD_GPIO);

	nrf_gpio_cfg(
            ST25_LPD_GPIO,
            NRF_GPIO_PIN_DIR_OUTPUT,
            NRF_GPIO_PIN_INPUT_DISCONNECT,
            NRF_GPIO_PIN_PULLUP,
            NRF_GPIO_PIN_H0H1,
            NRF_GPIO_PIN_NOSENSE);

#endif	
	

}



















