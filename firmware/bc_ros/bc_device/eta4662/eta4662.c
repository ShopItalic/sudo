#include "eta4662.h"

#include "bc_pmic_device_port.h"

#include "stdint.h"
#include "stdio.h"
#include "stdbool.h"

#include "nrf_gpio.h"

#define LOG_INFO  printf


typedef void (*eta4662_hardware_error_callback)(void); 

static eta4662_hardware_error_callback hardware_error_callback = NULL;




															 

//i2c回调接口
static const struct eta4662_i2c_port eta4662_i2c = {	
													  .read_callback = pmic_i2c_read,
													  .write_callback = pmic_i2c_write,
													  .init_callback =  pmic_i2c_init,
													  .open_callback =  pmic_i2c_open,
													  .close_callback = pmic_i2c_close,
													  .int_chg_open_callback = pmic_int_chg_open,
													  .int_chg_close_callback = pmic_int_chg_close,
                                                 };	

/***********      eta4662       ******************/


void eta4662_delay(uint16_t ms)
{
	pmic_delay(ms);
}	

/*******************************************************************************
 * Function Name     : fml_eta4662_config_init
 * Description       : 初始配置
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void eta4662_config_init(void)
{
	eta4662_i2c.open_callback();
    uint8_t data;
    eta4662_i2c.read_callback( 0x01, &data, 1);
    data &= ~0x08;//充电使能
    data &= ~0x07;//设置低的电流
    eta4662_i2c.write_callback( 0x1, data,1); // enable charge

    eta4662_i2c.write_callback( 0x2, 0x1,1); // 16ma，实测21ma

    eta4662_i2c.read_callback( 0x04, &data, 1);
    data = 0x9F;
    eta4662_i2c.write_callback( 0x4, data,1); // 4.2125
    
	
	
	data = 0xE2;
	eta4662_i2c.read_callback( 0x0A, &data, 1);
    
	
	eta4662_i2c.close_callback();
	
}



/*******************************************************************************
 * Function Name     : fml_eta4662_check_chip_id
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
bool eta4662_check_chip_id(void)
{
	eta4662_i2c.open_callback();
	uint8_t tmp = 0xFF;
    eta4662_i2c.read_callback(0x0B, &tmp, 1);
	if(tmp != 0x00)
	{
		LOG_INFO("sys chip id check error    id:%x \r\n",tmp);
		eta4662_i2c.close_callback();
		
		return false;
	}
	LOG_INFO("sys chip id check ok    id:%x \r\n",tmp);
	eta4662_i2c.close_callback();
	return true;
}

 /*******************************************************************************
 * Function Name     : fml_eta4662_feed_dog
 * Description       : 喂狗
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
void eta4662_feed_dog(void)
{
//	eta4662_i2c.open_callback();
	uint8_t tmp = 0x41;
	eta4662_i2c.write_callback(0x2, tmp, 1);
//	eta4662_i2c.close_callback();
}

/*******************************************************************************
 * Function Name     : fml_eta4662_charge_get_status
 * Description       : 获取充电状态
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
enum eta4662_charge_status eta4662_charge_get_status(void)
{
	eta4662_i2c.open_callback();
	uint8_t data;
	if (eta4662_i2c.read_callback(0x08, &data, 1))
	{
		data >>= 3;
		data &= 0x3;
		if (data == 2 || data == 1)
		{
			eta4662_feed_dog();
			eta4662_i2c.close_callback();
			return ETA4662_CHARGED_ING;
		}
		else if (data == 3)
		{
			eta4662_feed_dog();
			eta4662_i2c.close_callback();
			return ETA4662_CHARGED_OVER;
		}
		else
		{
		  eta4662_i2c.close_callback();
			return ETA4662_CHARGED_NOT;
		}
	}
	eta4662_i2c.close_callback();
  return ETA4662_CHARGED_NOT;
}

/*******************************************************************************
 * Function Name     : fml_eta4662_get_chip_id
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
bool eta4662_get_chip_id(uint8_t *data)
{
	eta4662_i2c.open_callback();
    eta4662_i2c.read_callback(0x0B, data, 1);
	if(data[0] != 0x00)
	{
		eta4662_i2c.close_callback();
		if(hardware_error_callback != NULL)
		{
			hardware_error_callback();
		}
		return false;
	}
	eta4662_i2c.close_callback();
	return true;
}


/*******************************************************************************
 * Function Name     : fml_eta4662_shlp_mode
 * Description       : 设置进入shlp模式
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
void eta4662_shlp_mode(void)
{
	eta4662_i2c.open_callback();
	uint8_t status = 0;
	eta4662_i2c.read_callback(0x0A,&status,1);
	status |= 0x04;
	eta4662_i2c.write_callback(0x0A, status, 1);
	//2.disable enb_ocb_otp
	eta4662_i2c.read_callback(0x07, &status, 1);
	status |= 0x80;
	eta4662_i2c.write_callback(0x07, status, 1);
	eta4662_i2c.int_chg_open_callback();
	eta4662_delay(100);
	//3.disable bfet_dis
    nrf_gpio_cfg_input(11,  NRF_GPIO_PIN_PULLDOWN);
	nrf_gpio_pin_sense_t pin_sense = NRF_GPIO_PIN_SENSE_HIGH;
  nrf_gpio_cfg_sense_input(11, NRF_GPIO_PIN_PULLDOWN, pin_sense);
	status &= ~0x80;
	status |= 0x20;
	eta4662_i2c.write_callback( 0x06, status, 1);
	eta4662_i2c.close_callback();
}


/*******************************************************************************
 * Function Name     : fml_eta4662_gte_reg_all
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
bool eta4662_gte_reg_all(uint8_t *data)
{
	eta4662_i2c.open_callback();
	if(data == NULL)
	{
		eta4662_i2c.close_callback();
		return false;
	}
	else
	{
		for(uint8_t i = 0; i < 12; i++)
		{
			eta4662_i2c.read_callback(i, &data[i], 1);
		}
	}
//	LOG_HEX("pmic all reg:",data,12);
    eta4662_i2c.close_callback();
	return true;
}



/*******************************************************************************
 * Function Name     : fml_eta4662_init
 * Description       : 初始配置
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
bool eta4662_init(void)
{
	eta4662_i2c.init_callback();
	eta4662_i2c.int_chg_open_callback();
	eta4662_delay(10);
	if(!eta4662_check_chip_id())
	{
		return false;
	}
	
	eta4662_config_init();
	return true;
}

bool eta4662_hardware_error_register_callback(const void *error_callback)
{
	if(error_callback == NULL)
	{
		return false;
	}
	hardware_error_callback = (eta4662_hardware_error_callback)error_callback;
	return true;

}

void eta4662_int_close(void)
{
	eta4662_i2c.int_chg_close_callback();
}

void eta4662_find(void)
{
	eta4662_i2c.init_callback();
}











