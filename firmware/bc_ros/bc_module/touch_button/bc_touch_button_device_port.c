#include "bc_touch_button_device_port.h"

#include "q_device.h"
#include "bc_logger.h"
#include "bc_delay.h"


#if ( HARDWARE_191_ENABLED == 1 || HARDWARE_1181_ENABLED == 1|| HARDWARE_1231_ENABLED == 1)	

#include "IQS7211E.h"
#else
#include "IQS323.h"
#endif
	

#include <string.h>



static q_device_t *touch_i2c_dev;
static q_device_t *touch_rdy_out_dev;
static q_device_t *touch_rdy_in_dev;

static void *int_io_irq_callback = NULL;

static struct i2c_package i2c_pack = {
#if ( HARDWARE_191_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	
                                       .slave_addr = IQS7211E_ADDR ,
#else
                                       .slave_addr = IQS323_ADDR,
#endif
	
	                                     
	
	                                     .write_length = 1,
	                                     .read_length = 1,
                                     };





int touch_i2c_open(void)
{
    //BC_LOG_INFO("touch_i2c_open**************\r\n");
	return q_device_open(touch_i2c_dev);
}


int touch_i2c_close(void)
{
    //BC_LOG_INFO("touch_i2c_close**************\r\n");
	q_device_close(touch_i2c_dev);
}

void touch_rdy_out_open(void)
{
	q_device_open(touch_rdy_out_dev);
}

void touch_rdy_out_close(void)
{
	q_device_close(touch_rdy_out_dev);
}
void touch_rdy_out_high(void)
{
	//q_device_open(touch_rdy_out_dev);	
	q_device_ctrl(touch_rdy_out_dev,GPIO_OUTPUT_HIGH,0);
}

void touch_rdy_out_low(void)
{
	q_device_ctrl(touch_rdy_out_dev,GPIO_OUTPUT_LOW,0);	
	//q_device_close(touch_rdy_out_dev);
}

void touch_io_irq_enable(void)
{
    printf("in touch_io_irq_enable**********************************\r\n");
    q_device_reg_callback(touch_rdy_in_dev,GPIOT_CONFIG_POLARITY_HiToLo,int_io_irq_callback);	  //注册回调
	q_device_open(touch_rdy_in_dev);
	//q_device_reg_callback(touch_rdy_in_dev,GPIOT_CONFIG_POLARITY_HiToLo,int_io_irq_callback);	  //注册回调

}

void touch_io_irq_disnable(void)
{
    printf("in touch_io_irq_disnable############################\r\n");
	q_device_close(touch_rdy_in_dev);
}

uint8_t touch_io_irq_status(void)
{
	uint8_t io_status = 0;
	q_device_read(touch_rdy_in_dev,0,&io_status,0);
	return io_status;
}

uint8_t touch_io_irq_status_get(uint8_t button_id)
{
	uint8_t io_status = 0;
	q_device_read(touch_rdy_in_dev,0,&io_status,0);
	return io_status;
}

void touch_io_irq_disable(void)
{
		q_device_close(touch_rdy_in_dev);
}

bool touch_irq_register_callback(void *callback)
{
	if(callback != NULL)
	{
		int_io_irq_callback = callback;
		return true;
	}
	return false;
}
	
bool touch_i2c_write(uint8_t reg_add ,uint8_t *data,uint8_t length)
{
    //BC_LOG_INFO("touch_i2c_write regadd:0x%02x, length:%d\r\n", reg_add,length);
	i2c_pack.reg_addr = reg_add;
	i2c_pack.write_length = length;
	i2c_pack.write_buff = data;
	if(q_device_write(touch_i2c_dev,0,&i2c_pack,0)== RESULT_OK)
	{
		bc_delay_ms(10);
		return true;
	}
	else
	{
		return false;
	}
}
																 
bool touch_i2c_read(uint8_t reg_add ,uint8_t *data,uint8_t length)
{
    //BC_LOG_INFO("touch_i2c_read regadd:0x%02x, length:%d\r\n", reg_add,length);
	i2c_pack.reg_addr = reg_add;
	i2c_pack.read_length = length;
	i2c_pack.read_buff = data;
	if(q_device_read(touch_i2c_dev,0,&i2c_pack,0) == RESULT_OK)
	{
		//BC_LOG_INFO("sys i2c read ok reg:%02x  data:%x \r\n",reg_add,i2c_pack.read_buff[0]);
		bc_delay_ms(10);
		return true;
	}
	else
	{
		//BC_LOG_INFO("sys i2c read error reg:%02x  data:%x \r\n",reg_add,i2c_pack.read_buff[0]);
		return false;
	}
}

void bc_touch_button_device_find(void)
{
#if (defined(HANDWARE_1_12_1) || defined(HANDWARE_1_9_1) || defined(HANDWARE_1_14_1)|| HARDWARE_1231_ENABLED == 1)
#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
     touch_i2c_dev = q_device_find("i2c_0");
#else
    touch_i2c_dev = q_device_find("i2c_0");
#endif
	q_device_assert(touch_i2c_dev);
#elif ( defined(HANDWARE_1_5_6) )
	touch_i2c_dev = q_device_find("i2c_1");
	q_device_assert(touch_i2c_dev);
#else
     touch_i2c_dev = q_device_find("i2c_0");
	q_device_assert(touch_i2c_dev);
#endif
	
	
	
	touch_rdy_out_dev = q_device_find("touch_rdy_out");
	q_device_assert(touch_rdy_out_dev);
	
	touch_rdy_in_dev = q_device_find("touch_rdy_in");
	q_device_assert(touch_rdy_in_dev);
}



 





