/*******************************************************************************
此为看门狗demo文件，使用q_device api接口

q_device规则
日  期：2024年1月18日
编写人：邱成凯
 *******************************************************************************/
#include "bc_wdg_demo.h"

#include "q_device.h"

#include "bc_timer.h"
#include "bc_logger.h"





static q_device_t *wdg_dev;


static void test_timer_callback(void * p_context);


static bc_timer_struct  test_timer = {
	.timer_name = "test timer",                          //定时器名字
	.uxAutoReload = true,                                //周期定时器
	.xTimerPeriodInTicks = 3000,                         //定时器时间
	.timer_callback_function = test_timer_callback,      //定时器回调
};


/*******************************************************************************
 * Function Name     : test_timer_callback
 * Description       : 定时器回调
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void test_timer_callback(void * p_context)
{
	q_device_ctrl(wdg_dev,WDT_FEED_DOG,0);
}

static void wdg_device_find(void)
{
	wdg_dev = q_device_find("watchdog");
	q_device_assert(wdg_dev);
	q_device_init(wdg_dev);
	q_device_ctrl(wdg_dev,WDT_FEED_DOG,0);		
}



/*******************************************************************************
 * Function Name     : test_timer_create
 * Description       : 创建定时器
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/

void test_timer_create(void)
{
	wdg_device_find();
	if(!bc_timer_create(&test_timer))                            //创建定时器
	{
		BC_LOG_WARN("create %s fial!! \r\n",test_timer.timer_name);              
	}
	else
	{
		BC_LOG_INFO("create %s success!! \r\n",test_timer.timer_name);
		bc_timer_start(&test_timer);                             //启动定时器
	}
	
}



