/*******************************************************************************
此为io输入demo文件，使用q_device api接口

q_device规则
日  期：2024年1月18日
编写人：邱成凯
 *******************************************************************************/
#include "bc_io_input.h"


#include "q_device.h"

#include "bc_timer.h"
#include "bc_logger.h"



static q_device_t *key_dev;  //设备描述



/*******************************************************************************
 * Function Name     : key_lo_to_hi_irq_callback
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void key_lo_to_hi_irq_callback(uint8_t pin,uint8_t status)
{
	BC_LOG_INFO("lo to hi  key:%d  ststus:%d \r\n",pin,status);
}


/*******************************************************************************
 * Function Name     : key_hi_to_lo_irq_callback
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void key_hi_to_lo_irq_callback(uint8_t pin,uint8_t status)
{
	BC_LOG_INFO("hi to lo  key:%d  ststus:%d \r\n",pin,status);
}

/*******************************************************************************
 * Function Name     : bc_io_input_test
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
void bc_io_input_test(void)
{
	/**********  key  **********/
	key_dev = q_device_find("key");
	q_device_assert(key_dev);
	q_device_open(key_dev);	
	q_device_reg_callback(key_dev,GPIOT_CONFIG_POLARITY_LoToHi,key_lo_to_hi_irq_callback);    //注册回调
	q_device_reg_callback(key_dev,GPIOT_CONFIG_POLARITY_HiToLo,key_hi_to_lo_irq_callback);	  //注册回调
}






















