/*******************************************************************************
此为io输出demo文件，使用q_device api接口

q_device规则
日  期：2024年1月18日
编写人：邱成凯
 *******************************************************************************/

#include "bc_rtc_demo.h"

#include "q_device.h"

#include "bc_timer.h"
#include "bc_logger.h"



static q_device_t *rtc_dev;

static void test_timer_callback(void * p_context);
static uint32_t temp_count = 0;
static bc_timer_struct  test_timer = {
	.timer_name = "test timer",                          //定时器名字
	.uxAutoReload = true,                                //周期定时器
	.xTimerPeriodInTicks = 3000,                         //定时器时间
	.timer_callback_function = test_timer_callback,      //定时器回调
};

static void test_rtc_open(void *rtc_timer_callback)
{
	q_device_open(rtc_dev);
	q_device_reg_callback(rtc_dev,0,rtc_timer_callback);	//注册定时中断回调，nordic支持，phy6222不支持
}

static void rtc_time_set_uinx_time(void)
{
	struct rtc_time time={0};
	time.unix_time = 1705628956;
	q_device_write(rtc_dev,0,(void*)&time,0);	
}

static void rtc_time_get_time(void)
{
	struct rtc_time time={0};
	q_device_read(rtc_dev,0,(void*)&time,0);
	BC_LOG_INFO("get_date_time:%d-%d-%d %d:%d:%d \r\n",
                                    time.bj_time.tm_year,
                                    time.bj_time.tm_mon,
                                    time.bj_time.tm_mday,
                                    time.bj_time.tm_hour,
                                    time.bj_time.tm_min,
                                    time.bj_time.tm_sec);
    BC_LOG_INFO("get uinx time:%d ",time.unix_time);	
}


/*******************************************************************************
 * Function Name     : test_timer_callback
 * Description       : 定时器回调
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void test_timer_callback(void * p_context)
{
	if(temp_count == 0)
	{
		rtc_time_set_uinx_time();
	}
	else
	{
		rtc_time_get_time();
	}
	temp_count++;
}

static void test_rtc_find(void)
{
	rtc_dev = q_device_find("sys rtc");
	q_device_assert(rtc_dev);	
	test_rtc_open(NULL);
	
}


/*******************************************************************************
 * Function Name     : test_timer_create
 * Description       : 创建led设备
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/

void test_timer_create(void)
{
	test_rtc_find();
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


















