
/*******************************************************************************
此为bsp 看门狗文件，通过宏定义来兼容nordic、phy6222硬件平台

接口遵循q_device规则
日  期：2024年1月17日
编写人：邱成凯
 *******************************************************************************/

#include "q_device.h"

#include <string.h>


#if (HARDWARE_ARCH_TYPE_NORDIC == 1)

#include "nrf_drv_wdt.h"
#include "bc_delay.h"


struct  BSP_WDT
{
	const char   *name;
	bool          lock;
  nrf_drv_wdt_channel_id wdt_channel_id;
	nrf_drv_wdt_config_t wdt_config;
	q_device_t dev;
};


static struct BSP_WDT bsp_list = 
{
	  .name = "watchdog",
		.lock = false,
		.wdt_config = NRF_DRV_WDT_DEAFULT_CONFIG,
		.dev = {0},
};

/*******************************************************************************
 * Function Name     : wdt_event_handler
 * Description       : 看门狗事件回调
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void wdt_event_handler(void)
{
    //NOTE: The max amount of time we can spend in WDT interrupt is two cycles of 32768[Hz] clock - after that, reset occurs
    printf("wdt reset\r\n");
  Q_DEVICE_LOG_INFO("bsp wdg event \r\n");
}
/*******************************************************************************
 * Function Name     : device_wdt_init
 * Description       : 看门狗初始化
 * Input             : 
 * Output            : 
 * Return            : 
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
 *******************************************************************************/
static int bsp_watchdog_init(q_device_t*dev)
{

	if(bsp_list.lock)
	{
		return RESULT_OK;
	}
	device_wdt_init(&bsp_list);
	bsp_list.lock = true;
	return RESULT_OK;
}


/*******************************************************************************
 * Function Name     : bsp_gpio_output_ctrl
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_watchdog_ctrl(q_device_t *dev, int cmd, void *args)
{
	if(!bsp_list.lock)
	{
		return RESULT_DOG_DEV_UNOPENED_ERR;
	}
	if(cmd != WDT_FEED_DOG)
	{
		return RESULT_INVALID_COMMAND_ERR;
	}
	
	device_feed_dog(&bsp_list);
	return RESULT_OK;
}





#endif

#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)

#include "watchdog.h"

struct  BSP_WDT
{
	const char   *name;
	bool          lock;
    uint8_t       feed_dog_time;
	q_device_t dev;
};


static struct BSP_WDT bsp_list = 
{
	
	.name = "watchdog",
	.lock = false,
	.feed_dog_time = WDG_16S,
	.dev = {0},
	
};


#include "jump_function.h"

//extern volatile uint8 g_clk32K_config;
//extern uint32_t s_config_swClk1;



//__ATTR_SECTION_SRAM__ void watchdog_init(void)
//{
//    volatile uint32_t a;
//    uint8_t delay;

//    if(bsp_list.feed_dog_time > 7)
//        return ;

//    if(g_clk32K_config == CLK_32K_XTAL)//rtc use 32K XOSC,watchdog use the same
//    {
//        AP_PCRM->CLKSEL |= (1UL<<16);
//    }
//    else
//    {
//        AP_PCRM->CLKSEL &= ~(1UL<<16); //rtc use 32K RCOSC,watchdog use the same
//    }

//    hal_clk_gate_enable(MOD_WDT);
//    s_config_swClk1|=_CLK_WDT; //add watchdog clk in pwrmg wakeup restore clk;

//    if((AP_PCR->SW_RESET0 & 0x04)==0)
//    {
//        AP_PCR->SW_RESET0 |= 0x04;
//        delay = 20;

//        while(delay-->0);
//    }

//    if((AP_PCR->SW_RESET2 & 0x04)==0)
//    {
//        AP_PCR->SW_RESET2 |= 0x04;
//        delay=20;

//        while(delay-->0);
//    }

//    AP_PCR->SW_RESET2 &= ~0x20;
//    delay=20;

//    while(delay-->0);

//    AP_PCR->SW_RESET2 |= 0x20;
//    delay=20;

//    while(delay-->0);

//    a = AP_WDT->EOI;
//    AP_WDT->TORR = bsp_list.feed_dog_time;
//    #if (HAL_WDG_CFG_MODE==WDG_USE_INT_MODE)
//    NVIC_SetPriority((IRQn_Type)WDT_IRQn, IRQ_PRIO_HAL);
//    NVIC_EnableIRQ((IRQn_Type)WDT_IRQn);
//    JUMP_FUNCTION(WDT_IRQ_HANDLER) = (uint32_t)&hal_WATCHDOG_IRQHandler;
//    AP_WDT->CR = 0x1F;//use int
//    #else
//    AP_WDT->CR = 0x1D;//not use int
//    #endif
//    AP_WDT_FEED;
//}
/*******************************************************************************
 * Function Name     : bsp_watchdog_open
 * Description       : 看门狗 open
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_watchdog_init(q_device_t*dev)
{

//	if(bsp_list.lock)
//	{
//		return RESULT_OK;
//	}
//	if(bsp_list.feed_dog_time > 7)
//	{
//		return 0;
//	}
//	else
//	{
//		return RESULT_DOG_DEV_NULL_ERR;	
//	}
	
		
//	watchdog_init();
//	JUMP_FUNCTION(HAL_WATCHDOG_INIT) = (uint32_t)&watchdog_init;
	watchdog_config(bsp_list.feed_dog_time);
	bsp_list.lock = true;
	return RESULT_OK;
}


/*******************************************************************************
 * Function Name     : bsp_gpio_output_ctrl
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_watchdog_ctrl(q_device_t *dev, int cmd, void *args)
{

//	if(!bsp_list.lock)
//	{
//		return RESULT_DOG_DEV_UNOPENED_ERR;
//	}
	if(cmd != WDT_FEED_DOG)
	{
		return RESULT_INVALID_COMMAND_ERR;                           
	}
	AP_WDT_FEED;

	return RESULT_OK;
}

#endif

/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
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
 *******************************************************************************/
static void bsp_watchdog_register(void)
{

	bsp_list.dev.name = bsp_list.name;
	bsp_list.dev.dops  = &ops;
	q_device_register(&bsp_list.dev);		
	
}

device_initcall(bsp_watchdog_register);



