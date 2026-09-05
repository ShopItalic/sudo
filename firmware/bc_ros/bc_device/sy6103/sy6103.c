#include "sy6103.h"

#include "bc_pmic_device_port.h"

#include "stdint.h"
#include "stdio.h"
#include "stdbool.h"

#define LOG_INFO  printf


typedef void (*sy6103_hardware_error_callback)(void); 

static sy6103_hardware_error_callback hardware_error_callback = NULL;




															 

//i2c回调接口
static const struct sy6103_i2c_port sy6103_i2c = {	
													  .read_callback = pmic_i2c_read,
													  .write_callback = pmic_i2c_write,
													  .init_callback =  pmic_i2c_init,
													  .open_callback =  pmic_i2c_open,
													  .close_callback = pmic_i2c_close,
													  .int_chg_open_callback = pmic_int_chg_open,
													  .int_chg_close_callback = pmic_int_chg_close,
                                                 };	

/***********      sy6103       ******************/

//static void fml_sy6103(void)
//{
//	sy6103_i2c.open_callback();
//	sy6103_i2c.write_callback( 0x00, 0x84,1);
//	sy6103_i2c.write_callback( 0x01, 0xA6,1);
//	sy6103_i2c.write_callback( 0x02, 0x00,1);
//	sy6103_i2c.write_callback( 0x03, 0x91,1);
//	sy6103_i2c.write_callback( 0x04, 0x43,1);
//	sy6103_i2c.write_callback( 0x05, 0x3A,1);
//	sy6103_i2c.write_callback( 0x06, 0x40,1);
//	sy6103_i2c.write_callback( 0x07, 0x24,1);
//	sy6103_i2c.close_callback();
//}	

void sy6103_delay(uint16_t ms)
{
	pmic_delay(ms);
}	

/*******************************************************************************
 * Function Name     : fml_eth4662_config_init
 * Description       : 初始配置
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void sy6103_config_init(void)
{
	sy6103_i2c.open_callback();
    uint8_t data;
    sy6103_i2c.read_callback( 0x01, &data, 1);
    data |= 0xC0;//按键拉低后的复位超时
    data &= ~0x08;//充电使能
    data &= ~0x07;//设置低的电流
    sy6103_i2c.write_callback( 0x1, data,1); // enable charge
#if ( defined(HANDWARE_1_17_1))	
    sy6103_i2c.write_callback( 0x2, 0x0A,1); // 88ma，实测98ma
#else
    sy6103_i2c.write_callback( 0x2, 0x1,1); // 16ma，实测21ma
#endif	
    

    sy6103_i2c.read_callback( 0x04, &data, 1);
#if ( defined(HANDWARE_1_17_1))	
    data = 0x62; //4.3v
#else
    data = 0x3A;  //4.175v
#endif	  
    
    sy6103_i2c.write_callback( 0x4, data,1); // 4.175
    
    sy6103_i2c.read_callback( 0x05, &data, 1);
    data  = 0x7A;   //未充电看门狗不使能
    sy6103_i2c.write_callback( 0x5, data,1);
	
	
	data = 0;
	sy6103_i2c.read_callback( 0x06, &data, 1);
  
#if defined(HANDWARE_1_17_1)
    data = 0x40;  //开启ntc
#else
    data = 0xC0;  //开启ntc
#endif
    
	sy6103_i2c.write_callback( 0x06, data,1);
	
    sy6103_i2c.read_callback( 0x07, &data, 1);
    data = 0x20;
    sy6103_i2c.write_callback( 0x7, data,1);
    
    sy6103_i2c.read_callback( 0x1B, &data, 1);
    data &= ~0x90;
    data &= ~0x04;
    sy6103_i2c.write_callback( 0x1B, data,1);
	
		data = 0;
	sy6103_i2c.read_callback( 0x1C, &data, 1);
  
#if defined(HANDWARE_1_17_1)
    data = 0x00;  //开启ntc  NTC_RES_SEL = 1
#else
    data = 0x04;  //开启ntc  NTC_RES_SEL = 1
#endif  
    
	sy6103_i2c.write_callback( 0x1C, data,1);
	
	data = 0;
	sy6103_i2c.read_callback( 0x19, &data, 1);
    data = 0x44;  //
	sy6103_i2c.write_callback( 0x19, data,1);
	
	sy6103_i2c.close_callback();
	
}



/*******************************************************************************
 * Function Name     : fml_eth4662_check_chip_id
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
bool sy6103_check_chip_id(void)
{
	sy6103_i2c.open_callback();
	uint8_t tmp = 0;
    sy6103_i2c.read_callback(0x0A, &tmp, 1);
	if(tmp != 0xE0)
	{
		LOG_INFO("sys chip id check error    id:%x \r\n",tmp);
		sy6103_i2c.close_callback();
		
		return false;
	}
	LOG_INFO("sys chip id check ok    id:%x \r\n",tmp);
	sy6103_i2c.close_callback();
	return true;
}

 /*******************************************************************************
 * Function Name     : fml_eth4662_feed_dog
 * Description       : 喂狗
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
void sy6103_feed_dog(void)
{
//	sy6103_i2c.open_callback();
//	uint8_t tmp = 0x41;
//  sy6103_i2c.write_callback(0x2, tmp, 1);
//	sy6103_i2c.close_callback();
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
enum sy6103_charge_status sy6103_charge_get_status(void)
{
	sy6103_i2c.open_callback();
	uint8_t data;
	if (sy6103_i2c.read_callback(0x08, &data, 1))
	{
		data >>= 3;
		data &= 0x3;
		if (data == 2 || data == 1)
		{
			sy6103_feed_dog();
			sy6103_i2c.close_callback();
			return SY6103_CHARGED_ING;
		}
		else if (data == 3)
		{
			sy6103_feed_dog();
			sy6103_i2c.close_callback();
			return SY6103_CHARGED_OVER;
		}
		else
		{
		  sy6103_i2c.close_callback();
			return SY6103_CHARGED_NOT;
		}
	}
	sy6103_i2c.close_callback();
  return SY6103_CHARGED_NOT;
}

/*******************************************************************************
 * Function Name     : fml_eth4662_get_chip_id
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
bool sy6103_get_chip_id(uint8_t *data)
{
	sy6103_i2c.open_callback();
    sy6103_i2c.read_callback(0x0A, data, 1);
	if(data[0] != 0xE0)
	{
		sy6103_i2c.close_callback();
		if(hardware_error_callback != NULL)
		{
			hardware_error_callback();
		}
		return false;
	}
	sy6103_i2c.close_callback();
	return true;
}


/*******************************************************************************
 * Function Name     : fml_eth4662_shlp_mode
 * Description       : 设置进入shlp模式
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
void sy6103_shlp_mode(void)
{
	sy6103_i2c.open_callback();
	uint8_t status = 0;
//	sy6103_i2c.read_callback(0x0A,&status,1);
//	status |= 0x04;
//	sy6103_i2c.write_callback(0x0A, status, 1);
//	//2.disable enb_ocb_otp
//	sy6103_i2c.read_callback(0x07, &status, 1);
//	status |= 0x80;
//	sy6103_i2c.write_callback(0x07, status, 1);
	
//	nrf_delay_ms(100);
	//3.disable bfet_dis
    sy6103_i2c.read_callback(0x06, &status, 1);
	status &= ~0x80;
	status |= 0x20;
	sy6103_i2c.write_callback( 0x06, status, 1);
	sy6103_i2c.close_callback();
}


/*******************************************************************************
 * Function Name     : fml_eth4662_gte_reg_all
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
bool sy6103_gte_reg_all(uint8_t *data)
{
	sy6103_i2c.open_callback();
	if(data == NULL)
	{
		sy6103_i2c.close_callback();
		return false;
	}
	else
	{
		for(uint8_t i = 0; i < 12; i++)
		{
			sy6103_i2c.read_callback(i, &data[i], 1);
		}
	}
//	LOG_HEX("pmic all reg:",data,12);
    sy6103_i2c.close_callback();
	return true;
}



/*******************************************************************************
 * Function Name     : fml_eth4662_init
 * Description       : 初始配置
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
bool sy6103_init(void)
{
	sy6103_i2c.init_callback();
	sy6103_i2c.int_chg_open_callback();
	sy6103_delay(10);
	if(!sy6103_check_chip_id())
	{
		return false;
	}
	
	sy6103_config_init();
	return true;
}

bool sy6103_hardware_error_register_callback(const void *error_callback)
{
	if(error_callback == NULL)
	{
		return false;
	}
	hardware_error_callback = (sy6103_hardware_error_callback)error_callback;
	return true;

}

void sy6103_int_close(void)
{
	sy6103_i2c.int_chg_close_callback();
}

void sy6103_find(void)
{
	sy6103_i2c.init_callback();
}











