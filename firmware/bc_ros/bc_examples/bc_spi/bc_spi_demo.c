/*******************************************************************************
此为spi demo文件，使用q_device api接口

q_device规则
日  期：2024年1月18日
编写人：邱成凯
 *******************************************************************************/
#include "bc_spi_demo.h"



#include "q_device.h"

#include "bc_timer.h"
#include "bc_logger.h"

#include "string.h"
#include "ring_config.h"
#include "bc_delay.h"

#include "stdint.h"

static q_device_t *spi_dev;



static struct spi_package spi_pack = {0};


static void test_timer_callback(void * p_context);


static bc_timer_struct  test_timer = {
	.timer_name = "test timer",                          //定时器名字
	.uxAutoReload = true,                                //周期定时器
	.xTimerPeriodInTicks = 3000,                         //定时器时间
	.timer_callback_function = test_timer_callback,      //定时器回调
};


static void spi_write(uint8_t *data,uint8_t length)
{
	
	memcpy(spi_pack.write_buff,data,length);
	spi_pack.write_length = length;

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)
	q_device_ctrl(spi_dev,GPIO_OUTPUT_LOW,0);	
	q_device_write(spi_dev,0,&spi_pack,0);
	q_device_ctrl(spi_dev,GPIO_OUTPUT_HIGH,0);
#elif (HARDWARE_ARCH_TYPE_PHY6222 == 1)	
	q_device_write(spi_dev,0,&spi_pack,0);
#endif	
}


static void spi_write_and_read(uint8_t *write_data,uint8_t write_length,uint8_t *read_data,uint8_t read_length)
{
	
	memcpy(spi_pack.write_buff,write_data,write_length);
	spi_pack.write_length = write_length;

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)
	q_device_ctrl(spi_dev,GPIO_OUTPUT_LOW,0);	
	q_device_write(spi_dev,0,&spi_pack,0);
	
	spi_pack.read_length= read_length;
	q_device_read(spi_dev,0,&spi_pack,0);
	memcpy(read_data,spi_pack.read_buff,spi_pack.read_length);
	q_device_ctrl(spi_dev,GPIO_OUTPUT_HIGH,0);
	
	
#elif (HARDWARE_ARCH_TYPE_PHY6222 == 1)	
	q_device_write(spi_dev,0,&spi_pack,0);
#endif	
}

static void spi_read(uint8_t *data,uint8_t length)
{
	spi_pack.read_length= length;
#if (HARDWARE_ARCH_TYPE_NORDIC == 1)
	q_device_ctrl(spi_dev,GPIO_OUTPUT_LOW,0);	
	q_device_read(spi_dev,0,&spi_pack,0);
	q_device_ctrl(spi_dev,GPIO_OUTPUT_HIGH,0);
#elif (HARDWARE_ARCH_TYPE_PHY6222 == 1)	
	q_device_read(spi_dev,0,&spi_pack,0);
#endif	
	memcpy(data,spi_pack.read_buff,spi_pack.read_length);
}


static void test_nfc(void)
{
	uint8_t a = 0x41;
	uint8_t b = 0x7f;
	uint8_t id = 0;
	spi_dev = q_device_find("spi_0");
	q_device_assert(spi_dev);
	q_device_open(spi_dev);	
	spi_write(&a,1);
	spi_write_and_read(&b,1,&id,1);
	q_device_close(spi_dev);
	BC_LOG_INFO("id:%02x\r\n",id);
}

/*******************************************************************************
 * Function Name     : test_timer_callback
 * Description       : 定时器回调
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static uint8_t test[200] = {0};
static void test_timer_callback(void * p_context)
{
//	spi_dev = q_device_find("spi_0");
//	q_device_assert(spi_dev);
//	q_device_open(spi_dev);
//	for(uint8_t i = 0; i < sizeof(test);i++)
//	{
//		test[i]++;
//	}
//	spi_write(test,sizeof(test));
//	q_device_close(spi_dev);
	
	test_nfc();
	BC_LOG_INFO("test spi\r\n");
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
	spi_dev = q_device_find("spi_0");
	q_device_assert(spi_dev);
	
	
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


















