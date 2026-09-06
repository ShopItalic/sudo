#include "bc_linear_motor_ic_port.h"

#include "q_device.h"

#include <string.h>
#include "bc_logger.h"
#include "bc_timer.h"
#include "bc_delay.h"
#include "haptic_nv.h"


enum bc_linear_motor_ic_timer_type
{
	BC_LINEAR_MOTOR_IC_PORT_TIMER = 0,
	BC_LINEAR_MOTOR_IC_TIMER_TYPE_NUM
};

extern void haptic_nv_tim_periodelapsedcallback(void *htim);


static bc_rtos_timer_struct  timer_struct[BC_LINEAR_MOTOR_IC_TIMER_TYPE_NUM] = {
	{
		.timer_name = "bc linear timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 100,
		.lock = false,
		.timer_callback_function = haptic_nv_tim_periodelapsedcallback,
	}
};															   
  

static q_device_t *i2c_dev = NULL;                                     //设备描述
static q_device_t *irq_io_dev = NULL;                                  //设备描述
static q_device_t *rst_io_dev = NULL;                                  //设备描述




static struct i2c_package i2c_pack = {
										 .slave_addr = 0x12,        //i2c从机地址
										 .write_length = 1,
										 .read_length = 1,
									 };
                                     
extern void haptic_nv_gpio_exti_callback(uint16_t GPIO_Pin);                             
static void irq_io_irq_callback(uint8_t pin,uint8_t state)
{	
  haptic_nv_gpio_exti_callback(0);
}

bool bc_linear_motor_i2c_write(uint8_t slave_addr,uint8_t reg_addr,uint8_t *write_data,uint8_t write_length)
{
	i2c_pack.reg_addr = reg_addr;
	i2c_pack.slave_addr = slave_addr  ;
	if(write_data != NULL)
	{
		i2c_pack.write_buff = write_data;
	}
	i2c_pack.write_length = write_length;
	if(q_device_write(i2c_dev,0,&i2c_pack,0) != RESULT_OK)
	{

    return true;
	}
//	bc_delay_ms(5);	
  return false;
}

bool bc_linear_motor_i2c_read(uint8_t slave_addr,uint8_t reg_addr,uint8_t *read_data,uint8_t read_length)
{
	i2c_pack.reg_addr = reg_addr;
	i2c_pack.slave_addr = slave_addr  ;
	i2c_pack.read_length = read_length;
	i2c_pack.read_buff = read_data;
	if(q_device_read(i2c_dev,0,&i2c_pack,0) != RESULT_OK)
	{
//		return false;
    return true;
	}
//	bc_delay_ms(5);	
//	return true;
  return false;
}

void bc_linear_motor_int_io_irq_enable(void)
{

	q_device_open(irq_io_dev);
	q_device_reg_callback(irq_io_dev,GPIOT_CONFIG_POLARITY_HiToLo,irq_io_irq_callback);	  //注册回调

}

void bc_linear_motor_int_io_irq_disable(void)
{
		q_device_close(irq_io_dev);
}


void bc_linear_motor_i2c_open(void)
{
	q_device_open(i2c_dev);                                                                        //打开设备

}

void bc_linear_motor_i2c_close(void)
{
	q_device_close(i2c_dev);                                                                        //关闭设备
}

void bc_linear_motor_rst_low(void)
{
    //q_device_open(rst_io_dev);
    q_device_ctrl(rst_io_dev,GPIO_OUTPUT_LOW,0);	
    //q_device_close(rst_io_dev);
}

void bc_linear_motor_rst_high(void)
{
    //q_device_open(rst_io_dev);
    q_device_ctrl(rst_io_dev,GPIO_OUTPUT_HIGH,0);	
    //q_device_close(rst_io_dev);
}

void bc_linear_motor_rst_open(void)
{
    q_device_open(rst_io_dev);
}

void bc_linear_motor_rst_close(void)
{
    q_device_close(rst_io_dev);
}

void bc_linear_motor_timer_start(void)
{
	bc_rtos_timer_start(timer_struct[BC_LINEAR_MOTOR_IC_PORT_TIMER].timer_handler,50);
}

void bc_linear_motor_timer_stop(void)
{
    bc_rtos_timer_stop(timer_struct[BC_LINEAR_MOTOR_IC_PORT_TIMER].timer_handler,50);
}


void bc_motor_stop(void)
{
    bc_linear_motor_i2c_open();
    bc_delay_ms(5);
    g_func_haptic_nv->play_stop();
    bc_linear_motor_i2c_close();
}


//bool bc_linear_motor_io_irq_register_callback(void *callback)
//{
//	if(callback != NULL)
//	{
//		irq_io_irq_callback = callback;
//		return true;
//	}
//	return false;
//}

/*******************************************************************************
 * Function Name     : test_io_output_led_device_find
 * Description       : 查找led设备
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
void bc_linear_motor_device_i2c_find(void)
{
    if(i2c_dev == NULL)
    {
        i2c_dev = q_device_find("i2c_1");
        q_device_assert(i2c_dev);
    }
#if defined(HANDWARE_1_23_3)
    if(rst_io_dev == NULL)
    {
        rst_io_dev = q_device_find("aw86235_reset");
        q_device_assert(rst_io_dev);
        q_device_open(rst_io_dev);
    }

    if(irq_io_dev == NULL)
    {
        irq_io_dev = q_device_find("aw86235_int");
        q_device_assert(irq_io_dev);
    }
 #endif   
//    for(uint8_t i = 0;i < BC_LINEAR_MOTOR_IC_TIMER_TYPE_NUM; i++)
//	{
//		if(!bc_timer_create(&timer_struct[i]))
//		{
//			BC_LOG_INFO("create %s fial!! \r\n",timer_struct[i].timer_name);
//		}
//		else
//		{
//			BC_LOG_INFO("create %s success!! \r\n",timer_struct[i].timer_name);
//		}		
//	}
    for(uint8_t i = 0;i < BC_LINEAR_MOTOR_IC_TIMER_TYPE_NUM; i++)
    {
        timer_struct[i].timer_handler = bc_rtos_timer_create(timer_struct[i].timer_name,
														  timer_struct[i].xTimerPeriodInTicks,
														  timer_struct[i].uxAutoReload, 
														   (void *)timer_struct[i].timer_id,
															timer_struct[i].timer_callback_function);
		if(timer_struct[i].timer_handler != NULL)
		{
			BC_LOG_INFO("create %s succeed\r\n",timer_struct[i].timer_name);
		}
		else
		{
			BC_LOG_ERROR("create %s fail\r\n",timer_struct[i].timer_name);
		}	
    }
}

