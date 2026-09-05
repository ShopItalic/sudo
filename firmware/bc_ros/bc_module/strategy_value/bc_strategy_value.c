#include "bc_strategy_value.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bc_util.h"
#include "bc_logger.h"
#include "q_device.h"

#include "crc.h"

static uint32_t business_strategy_buff[BUSINESS_STRATEGY_MAX] = {0x00};          //加1是留出crc校验  


static q_device_t *flash_dev;

static struct flash_write_package flash_write_pack;
static struct flash_read_package flash_read_pack;

#define FLASH_STROGE_STRATEGY_OFFSET   4096*1

static int bsp_device_find(void)
{
    flash_dev = q_device_find("device_flash");
	q_device_assert(flash_dev);
    q_device_init(flash_dev);
    return 1;
}


static int read(long offset, uint8_t *buf, size_t size)
{	
	memset((uint8_t *)&flash_read_pack,0,sizeof(flash_read_pack));
	flash_read_pack.data = buf;
	flash_read_pack.data_length = size;
	flash_read_pack.offset = offset;
	if(q_device_read(flash_dev,0,&flash_read_pack,0) != RESULT_OK)
	{
		BC_LOG_ERROR("flash read error!!! \r\n");
		return 0;
	}
    return 1;
}

static int write(long offset, const uint8_t *buf, size_t size)
{	
	memset((uint8_t *)&flash_write_pack,0,sizeof(flash_write_pack));
	flash_write_pack.offset = offset;
	flash_write_pack.data_length = size;
    flash_write_pack.data = buf;
	if(q_device_write(flash_dev,0,&flash_write_pack,0) != RESULT_OK)
	{
		BC_LOG_ERROR("flash write error!!!! \r\n");
		return 0;
	}
	return 1;
}


static int erase(long offset, size_t size)
{
	memset((uint8_t *)&flash_write_pack,0,sizeof(flash_write_pack));
	flash_write_pack.offset = offset;
	flash_write_pack.data_length = size;
	if(q_device_ctrl(flash_dev,ERASE_FLASH,&flash_write_pack) != RESULT_OK)
	{
		BC_LOG_ERROR("falsh erase error!!!!!!\r\n");
		return 0;
	}
	return 1;
}



/*******************************************************************************
 * Function Name     : business_strategy_flash_check
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
static bool business_strategy_flash_check(uint32_t *business_strategy_data)
{
	uint32_t business_strategy_buff_crc;
	uint16_t check_crc;
	read(FLASH_STROGE_STRATEGY_OFFSET,(uint8_t*)business_strategy_data,sizeof(business_strategy_buff));
	business_strategy_buff_crc = business_strategy_data[BUSINESS_STRATEGY_CRC];
	check_crc = crc16bitbybit((uint8_t*)business_strategy_data,(BUSINESS_STRATEGY_CRC - 1)*sizeof(uint32_t));
	if(business_strategy_buff_crc != check_crc)
	{
		BC_LOG_ERROR("business_strategy_buff_crc:%04x check crc:%04x \r\n",business_strategy_buff_crc,check_crc);
		return false;
	}
//	BC_LOG_HEX_P("business strategy:",(uint8_t*)business_strategy_buff,sizeof(business_strategy_buff));
	return true;
}

/*******************************************************************************
 * Function Name     : save_business_strategy
 * Description       : 保存策略值
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
static bool save_business_strategy(void)
{

	business_strategy_buff[BUSINESS_STRATEGY_CRC] = crc16bitbybit((uint8_t*)business_strategy_buff,(BUSINESS_STRATEGY_CRC - 1)*sizeof(uint32_t));
	if(erase(FLASH_STROGE_STRATEGY_OFFSET,sizeof(business_strategy_buff)))
	{
		if(write(FLASH_STROGE_STRATEGY_OFFSET,(uint8_t*)business_strategy_buff,sizeof(business_strategy_buff)))
		{
			
//			BC_LOG_HEX_P("save business strategy:",(uint8_t*)business_strategy_buff,sizeof(business_strategy_buff));
			return true;
		}	
	}
	BC_LOG_ERROR("save business strategy error\r\n");
	return false;
}

static void business_strategy_init_config(void)
{

#if (HARDWARE_441_ENABLED == 1)	
		business_strategy_buff[BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_TIME] =  60*10;                      //自动周期检测时间,最小单位/秒
#else
		business_strategy_buff[BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_TIME] =  60*20;                      //自动周期检测时间,最小单位/秒
		
#endif	
	business_strategy_buff[BUSINESS_STRATEGY_PPG_HR_AUTOMATIC_COLLECTION_TIME] = 30;                  //自动周期检测时hr的采集时长，最小单位/秒
	
#if (HARDWARE_441_ENABLED == 1)	
	business_strategy_buff[BUSINESS_STRATEGY_PPG_SPO2_AUTOMATIC_COLLECTION_TIME] = 60;                  //自动周期检测时spo2的采集时长，最小单位/秒
#else
	business_strategy_buff[BUSINESS_STRATEGY_PPG_SPO2_AUTOMATIC_COLLECTION_TIME] = 30;                  //自动周期检测时spo2的采集时长，最小单位/秒
		
#endif		
	
	business_strategy_buff[BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_HR_TO_SPO2] = 5;                  //自动周期检测hr n次后检测spo2


#if defined(RONG_WEI_Z2X)

	business_strategy_buff[BUSINESS_STRATEGY_CH0_TOUCH_THRESHOLD] = 0x10; 
	business_strategy_buff[BUSINESS_STRATEGY_CH1_TOUCH_THRESHOLD] = 0x0C; 
	business_strategy_buff[BUSINESS_STRATEGY_CH2_TOUCH_THRESHOLD] = 0x0A; 
#else
	business_strategy_buff[BUSINESS_STRATEGY_CH0_TOUCH_THRESHOLD] = 0x0A; 
	business_strategy_buff[BUSINESS_STRATEGY_CH1_TOUCH_THRESHOLD] = 0x0C; 
	business_strategy_buff[BUSINESS_STRATEGY_CH2_TOUCH_THRESHOLD] = 0x0A; 	
#endif
	business_strategy_buff[BUSINESS_STRATEGY_CRC] = crc16bitbybit((uint8_t*)business_strategy_buff,(BUSINESS_STRATEGY_CRC - 1)*sizeof(uint32_t));
	save_business_strategy();	
}

/*******************************************************************************
 * Function Name     : fml_get_business_strategy_value
 * Description       : 获取策略值
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
uint32_t bc_get_business_strategy_value(business_strategy business_strategy_id)
{
	if(business_strategy_id < BUSINESS_STRATEGY_MAX && business_strategy_id >= 0)
	{
		return  business_strategy_buff[business_strategy_id];
	}
	BC_LOG_ERROR("get business strategy error\r\n");
	return 0;
}

/*******************************************************************************
 * Function Name     : fml_get_business_strategy_value_all
 * Description       : 获取全部策略值
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
bool bc_get_business_strategy_value_all(uint8_t *data_value,uint8_t *data_len)
{
	memcpy(data_value,business_strategy_buff,BUSINESS_STRATEGY_MAX);
	if(data_len == NULL)
	{
		BC_LOG_ERROR("get business strategy all error\r\n");
		return false;
	}
	*data_len = BUSINESS_STRATEGY_MAX;
	return true;
}


/*******************************************************************************
 * Function Name     : fml_set_business_strategy_value
 * Description       : 设置策略值
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
bool bc_set_business_strategy_value(business_strategy business_strategy_id,uint32_t value)
{
	if(business_strategy_id < BUSINESS_STRATEGY_MAX && business_strategy_id >= 0)
	{
		business_strategy_buff[business_strategy_id] = value;
		if(save_business_strategy())
		{
			return true;
		}	
	}
	BC_LOG_ERROR("set business strategy  error\r\n");
	return false;
}
/*******************************************************************************
 * Function Name     : fml_set_business_strategy_value_all
 * Description       : 设置全部策略值
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
bool bc_set_business_strategy_value_all(uint8_t *data,uint8_t len)
{
	if(len < BUSINESS_STRATEGY_MAX && len >= 0)
	{
	  memcpy(business_strategy_buff,data,len);
		if(save_business_strategy())
		{
			return true;
		}	
	}
	BC_LOG_ERROR("set business strategy all  error\r\n");
	return false;
}

void bc_business_strategy_reset(void)
{
	business_strategy_init_config();
}


/*******************************************************************************
 * Function Name     : fml_business_strategy_init
 * Description       : 策略值初始化
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
void bc_business_strategy_init(void)
{
	bsp_device_find();
	if(!business_strategy_flash_check(business_strategy_buff))
	{
		business_strategy_init_config();
	}
} 
