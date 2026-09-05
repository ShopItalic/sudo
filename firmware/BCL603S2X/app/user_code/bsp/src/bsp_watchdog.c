#include "q_device.h"

#include <string.h>

#include "nrf_drv_wdt.h"


struct  BSP_WDT
{
	const char   *name;
	bool          lock;
  nrf_drv_wdt_channel_id wdt_channel_id;
	nrf_drv_wdt_config_t wdt_config;
	q_device_t dev;
};


static struct BSP_WDT bsp_list[] = 
{
	{
	  .name = "watchdog",
		.lock = false,
		.wdt_config = NRF_DRV_WDT_DEAFULT_CONFIG,
		.dev = {0},
	}
};

/*******************************************************************************
 * Function Name     : wdt_event_handler
 * Description       : 看门狗事件回调
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void wdt_event_handler(void)
{
    //NOTE: The max amount of time we can spend in WDT interrupt is two cycles of 32768[Hz] clock - after that, reset occurs
}
/*******************************************************************************
 * Function Name     : device_wdt_init
 * Description       : 看门狗初始化
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void device_wdt_init(struct BSP_WDT *dev)
{
	ret_code_t err_code;
	err_code = nrf_drv_wdt_init(&dev->wdt_config, wdt_event_handler); //
	APP_ERROR_CHECK(err_code);
	err_code = nrf_drv_wdt_channel_alloc(&dev->wdt_channel_id);      
	APP_ERROR_CHECK(err_code);
	nrf_drv_wdt_enable();
}

/*******************************************************************************
 * Function Name     : device_feed_dog
 * Description       : 看门狗喂狗
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void device_feed_dog(struct BSP_WDT *dev)
{
	nrf_drv_wdt_channel_feed(dev->wdt_channel_id);
}


/*******************************************************************************
 * Function Name     : bsp_watchdog_open
 * Description       : 看门狗 open
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_watchdog_init(q_device_t*dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].lock)
			{
				return RESULT_OK;
			}
			device_wdt_init(&bsp_list[i]);
			bsp_list[i].lock = true;
			return RESULT_OK;
		}
	}
	return RESULT_DOG_DEV_NULL_ERR;	
}


/*******************************************************************************
 * Function Name     : bsp_gpio_output_ctrl
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_watchdog_ctrl(q_device_t *dev, int cmd, void *args)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_DOG_DEV_UNOPENED_ERR;
			}
			if(cmd != WDT_FEED_DOG)
			{
				return RESULT_INVALID_COMMAND_ERR;
			}
			
			device_feed_dog(&bsp_list[i]);
			return RESULT_OK;
		}
	}
	return RESULT_DOG_DEV_NULL_ERR;
}



/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static struct q_device_ops ops =
{
	.control = bsp_watchdog_ctrl,
	.init = bsp_watchdog_init,
};

/*******************************************************************************
 * Function Name     : bsp_gpio_output_register
 * Description       : 设备注册
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_watchdog_register(void)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		bsp_list[i].dev.name = bsp_list[i].name;
		bsp_list[i].dev.dops  = &ops;
		q_device_register(&bsp_list[i].dev);		
	}
}

device_initcall(bsp_watchdog_register);





