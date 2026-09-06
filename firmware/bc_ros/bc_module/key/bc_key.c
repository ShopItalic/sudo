#include "bc_key.h"


#include "q_device.h"

#include <string.h>
#include "bc_logger.h"





static q_device_t *int_io_dev = NULL;                                  //设备描述

static void *int_io_irq_callback = NULL;

void bc_key_io_irq_enable(void)
{

	q_device_open(int_io_dev);
	q_device_reg_callback(int_io_dev,GPIOT_CONFIG_POLARITY_HiToLo,int_io_irq_callback);	  //注册回调

}

void bc_key_io_irq_disable(void)
{
		q_device_close(int_io_dev);
}

bool bc_key_io_irq_register_callback(void *callback)
{
	if(callback != NULL)
	{
		int_io_irq_callback = callback;
		return true;
	}
}


uint8_t bc_key_io_key_status_get(uint8_t button_id)
{
	uint8_t io_status = 0;
	q_device_read(int_io_dev,0,&io_status,0);
	return io_status;
}




/*******************************************************************************
 * Function Name     : test_io_output_led_device_find
 * Description       : 查找led设备
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
void bc_key_device_find(void)
{
	

	if(int_io_dev == NULL)
	{
		int_io_dev = q_device_find("key_od");
		q_device_assert(int_io_dev);
		
	}

}








