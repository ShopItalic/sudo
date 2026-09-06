/*******************************************************************************
此为event demo文件

q_device规则
日  期：2024年1月18日
编写人：邱成凯
 *******************************************************************************/

#include "bc_event_demo.h"

#include "bc_event.h"
#include "bc_timer.h"
#include "bc_logger.h"

static void test_event_callback(void * p_context);

static bc_event_struct event_struct={
	.event_name = "test event",
	.event_callback_function = test_event_callback,
};


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
	bc_event_set(&event_struct);                        //发送事件
}


static void test_event_callback(void * p_context)
{
	BC_LOG_INFO("test event \r\n");
}

/*******************************************************************************
 * Function Name     : test_timer_create
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/

void test_timer_create(void)
{

	if(!bc_timer_create(&test_timer))                            //创建定时器
	{
		BC_LOG_WARN("create %s fial!! \r\n",test_timer.timer_name);              
	}
	else
	{
		BC_LOG_INFO("create %s success!! \r\n",test_timer.timer_name);
		bc_timer_start(&test_timer);                             //启动定时器
	}
	
	if(!bc_event_create(&event_struct))                         //创建事件
	{
		BC_LOG_WARN("create %s fial!! \r\n",event_struct.event_name);
	}
	else
	{
		BC_LOG_INFO("create %s success!! \r\n",event_struct.event_name);
	}
	
}
