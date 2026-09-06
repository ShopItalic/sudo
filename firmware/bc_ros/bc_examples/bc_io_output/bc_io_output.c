/*******************************************************************************
此为io输出demo文件，使用q_device api接口

q_device规则
日  期：2024年1月18日
编写人：邱成凯
 *******************************************************************************/
#include "bc_io_output.h"

#include "q_device.h"

#include "bc_timer.h"
#include "bc_logger.h"



static q_device_t *led_red_dev;                          //设备描述

static void test_timer_callback(void * p_context);


static bc_timer_struct  test_timer = {
	.timer_name = "test timer",                          //定时器名字
	.uxAutoReload = true,                                //周期定时器
	.xTimerPeriodInTicks = 3000,                         //定时器时间
	.timer_callback_function = test_timer_callback,      //定时器回调
};

static uint32_t temp_count = 0;

/*******************************************************************************
 * Function Name     : io_out_toggle_and_close_test
 * Description       : io输出翻转信号，加close测试
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void io_out_toggle_and_close_test(void)
{
	
	
	if(temp_count< 5)
	{
		q_device_ctrl(led_red_dev,GPIO_OUTPUT_TOGGLE,0);                //输出翻转
		q_device_ctrl(led_red_dev,GPIO_REGISTER,0);                     //常保持，注意：在nordic平台不必设置常保持，仅phy6222需要
	}
	
	if(temp_count == 5)
	{
		q_device_close(led_red_dev);                                    //关闭设备
	}
	if(temp_count > 8)
	{
		q_device_ctrl(led_red_dev,GPIO_OUTPUT_TOGGLE,0);                //输出翻转
		q_device_ctrl(led_red_dev,GPIO_REGISTER,0);                     //常保持，注意：在nordic平台不必设置常保持，仅phy6222需要
	}	     
	temp_count++;
}

/*******************************************************************************
 * Function Name     : io_out_register_test
 * Description       : io输出翻转信号，常保持
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void io_out_register_test(void)
{
	if(temp_count%2)
	{
		q_device_ctrl(led_red_dev,GPIO_OUTPUT_HIGH,0);                //输出翻转
		q_device_ctrl(led_red_dev,GPIO_REGISTER,0);                   //常保持，注意：在nordic平台不必设置常保持，仅phy6222需要
	}
	else
	{
		q_device_ctrl(led_red_dev,GPIO_OUTPUT_LOW,0);                 //输出翻转
		q_device_ctrl(led_red_dev,GPIO_REGISTER,0);                   //常保持，注意：在nordic平台不必设置常保持，仅phy6222需要
	}
	temp_count++;
}
/*******************************************************************************
 * Function Name     : io_out_test
 * Description       : io输出
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void io_out_test(void)
{
	if(temp_count%2)
	{
		q_device_ctrl(led_red_dev,GPIO_OUTPUT_HIGH,0);               //输出高电平
	}
	else
	{
		q_device_ctrl(led_red_dev,GPIO_OUTPUT_LOW,0);                //输出低电平
	}	
	temp_count++;
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
//	io_out_toggle_and_close_test();
//	io_out_register_test();
	io_out_test();
}

/*******************************************************************************
 * Function Name     : test_io_output_led_device_find
 * Description       : 查找led设备
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void test_io_output_led_device_find(void)
{
	/**********  led  **********/
	led_red_dev = q_device_find("led");                          //查找led设备
	q_device_assert(led_red_dev);	                             //断言
	q_device_open(led_red_dev);                                  //打开led设备
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
	test_io_output_led_device_find();
	if(!bc_timer_create(&test_timer))                            //创建led定时器
	{
		BC_LOG_WARN("create %s fial!! \r\n",test_timer.timer_name);              
	}
	else
	{
		BC_LOG_INFO("create %s success!! \r\n",test_timer.timer_name);
		bc_timer_start(&test_timer);                             //启动led定时器
	}
	
}



























