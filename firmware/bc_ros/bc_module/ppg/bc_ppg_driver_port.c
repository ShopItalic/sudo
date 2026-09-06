#include "bc_ppg_driver_port.h"

#include "q_device.h"

#include <string.h>
#include "bc_logger.h"

#include "bc_delay.h"

#if (defined(HANDWARE_4_5_1) || defined(HANDWARE_1_5_6) )

#include "bc_device_i2c_bus_handler.h"

#endif

#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

#include "hx3605.h"

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

#include "zspd4000_drv.h"

#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T

#elif (PPG_DEVIECE_TYPE == 4)   //hx 3918

#include "hx3918.h"

#endif


static q_device_t *i2c_dev = NULL;                                     //设备描述

static q_device_t *int_io_dev = NULL;                                  //设备描述

static void *int_io_irq_callback = NULL;

static q_device_t *reset_io_dev = NULL;                                  //设备描述

static struct i2c_package i2c_pack = {
										 .slave_addr = 0x12 <<1,        //i2c从机地址
										 .write_length = 1,
										 .read_length = 1,
//	                                    .reg_addr_bit = I2C_REG_ADDR_8bit,
									 };

																				 
																				 



																				 

void bc_ppg_i2c_write(uint8_t slave_addr,uint16_t reg_addr,uint8_t *write_data,uint16_t write_length)
{
	i2c_pack.reg_addr = reg_addr;
#if ( defined(HANDWARE_1_9_1x) || defined(HANDWARE_1_5_6) || defined(HANDWARE_1_14_1x))
    i2c_pack.slave_addr = slave_addr >> 1;
#else	
	i2c_pack.slave_addr = slave_addr ;
#endif	
	
	if(write_data != NULL)
	{
//		memcpy(i2c_pack.write_buff,write_data,write_length);
		i2c_pack.write_buff = write_data;
	}
	i2c_pack.write_length = write_length;
	q_device_write(i2c_dev,0,&i2c_pack,0);  

}

void bc_ppg_2c_read(uint8_t slave_addr,uint16_t reg_addr,uint8_t *read_data,uint16_t read_length)
{
	i2c_pack.reg_addr = reg_addr;
#if ( defined(HANDWARE_1_9_1x)|| defined(HANDWARE_1_5_6) || defined(HANDWARE_1_14_1x))
    i2c_pack.slave_addr = slave_addr >> 1;
#else	
	i2c_pack.slave_addr = slave_addr ;
#endif		

	i2c_pack.read_length = read_length;
	i2c_pack.read_buff = read_data;
	q_device_read(i2c_dev,0,&i2c_pack,0);

	if(read_data != NULL)
	{
//		memcpy(read_data,i2c_pack.read_buff,i2c_pack.read_length);
	}
}

void bc_ppg_i2c_bus_open(void)
{
	q_device_open(i2c_dev);                                                                        //打开设备
	BC_LOG_INFO("bc ppg bus open \r\n");
}

void bc_ppg_i2c_bus_close(void)
{
	q_device_close(i2c_dev);                                                                        //关闭设备
	BC_LOG_INFO("bc ppg bus close \r\n");
}

void bc_ppg_i2c_open(void)
{
#if (defined(HANDWARE_4_5_1)  || defined(HANDWARE_1_5_6) )
    bc_i2c_bus_device_open(DEVICE_BUS_TYPE_PPG);
#else	
	q_device_open(i2c_dev);  
#endif	
	                                                                      //打开设备
	BC_LOG_INFO("bc ppg open \r\n");
}

void bc_ppg_i2c_close(void)
{
	
#if (defined(HANDWARE_4_5_1)  || defined(HANDWARE_1_5_6) )
    bc_i2c_bus_device_close(DEVICE_BUS_TYPE_PPG);
#else	
	q_device_close(i2c_dev);    
#endif		
	                                                                      //关闭设备
	BC_LOG_INFO("bc ppg close \r\n");
}


void bc_ppg_int_io_irq_enable(void)
{

	q_device_open(int_io_dev);
	q_device_reg_callback(int_io_dev,GPIOT_CONFIG_POLARITY_HiToLo,int_io_irq_callback);	  //注册回调

}

void bc_ppg_int_io_irq_disable(void)
{
		q_device_close(int_io_dev);
}

bool bc_ppg_io_irq_register_callback(void *callback)
{
	if(callback != NULL)
	{
		int_io_irq_callback = callback;
		return true;
	}
	return false;
}


void bc_ppg_reset_io_enable(void)
{
    q_device_open(reset_io_dev);
	if(reset_io_dev == NULL)
    {
		BC_LOG_INFO("reset_io_dev NULL\r\n");
        
    }
}

void bc_ppg_reset_io_disable(void)
{
        q_device_close(reset_io_dev);
}

void bc_ppg_reset_io_control(uint8_t pin_level)
{
        q_device_ctrl(reset_io_dev,pin_level,0);
}

void bc_ppg_reset_low(void)
{
#if (defined(HANDWARE_1_9_1) || defined(HANDWARE_1_14_1) || defined(HANDWARE_1_5_6) || defined(HANDWARE_1_5_8)  || defined(HANDWARE_1_17_1))		
	q_device_ctrl(reset_io_dev,GPIO_OUTPUT_LOW,0);
#endif	

}

void bc_ppg_reset_high(void)
{
#if (defined(HANDWARE_1_9_1) || defined(HANDWARE_1_14_1) || defined(HANDWARE_1_5_6) || defined(HANDWARE_1_5_8)  || defined(HANDWARE_1_17_1))		
	q_device_ctrl(reset_io_dev,GPIO_OUTPUT_HIGH,0);
#endif	
}


void bc_ppg_reset(void)
{
	q_device_open(reset_io_dev);
	q_device_ctrl(reset_io_dev,GPIO_OUTPUT_LOW,0);
	bc_delay_ms(20);
	q_device_ctrl(reset_io_dev,GPIO_OUTPUT_HIGH,0);
	q_device_close(reset_io_dev);
}

/*******************************************************************************
 * Function Name     : test_io_output_led_device_find
 * Description       : 查找led设备
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
void bc_ppg_device_find(void)
{

#if (HARDWARE_451_ENABLED == 1 )	
	if(i2c_dev == NULL)
	{
		i2c_dev = q_device_find("i2c_4");
		q_device_assert(i2c_dev);
	}
#elif (defined(HANDWARE_1_12_1) || defined(HANDWARE_1_9_1) )	
	if(i2c_dev == NULL)
	{
		i2c_dev = q_device_find("i2c_4");
		q_device_assert(i2c_dev);
	}
#elif (defined(HANDWARE_1_5_6) || defined(HANDWARE_1_14_1x) )	
	if(i2c_dev == NULL)
	{
		i2c_dev = q_device_find("i2c_0");
		q_device_assert(i2c_dev);
	}
#else
	if(i2c_dev == NULL)
	{
		i2c_dev = q_device_find("i2c_4");
		q_device_assert(i2c_dev);
	}
#endif		
	
	
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605
	hx3605_timer_init();
#elif (PPG_DEVIECE_TYPE == 4)   //hx 3918
	hx3918_timer_init();
#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000
	if(int_io_dev == NULL)
	{
		int_io_dev = q_device_find("ppg_int");
		q_device_assert(int_io_dev);
		
	}
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
    if(int_io_dev == NULL)
    {
        int_io_dev = q_device_find("ppg_int");
        q_device_assert(int_io_dev);
		BC_LOG_INFO("int_io_dev find \r\n");
        
    }
    
    if(reset_io_dev == NULL)
    {
        reset_io_dev = q_device_find("ppg_reset_en");
        q_device_assert(reset_io_dev);
        
    }
#if (defined(HANDWARE_1_9_1) || defined(HANDWARE_1_14_1) || defined(HANDWARE_1_5_6) || defined(HANDWARE_1_5_8) || defined(HANDWARE_1_17_1) )		
	q_device_open(reset_io_dev);
#endif		
	
#endif
}



















