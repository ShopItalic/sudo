#include "bc_fuel_gauge_port.h"

#include "q_device.h"
#include "bc_logger.h"
#include "bc_delay.h"
#include <string.h>

#define ADDR_CW2215					0x64

static q_device_t *cw221x_i2c_dev;

static void *int_io_irq_callback = NULL;

static struct i2c_package i2c_pack = {
                                       .slave_addr = ADDR_CW2215,
	
	                                     
	
	                                     .write_length = 1,
	                                     .read_length = 1,
                                     };





int cw221x_i2c_open(void)
{
    //BC_LOG_INFO("touch_i2c_open**************\r\n");
	return q_device_open(cw221x_i2c_dev);
}


int cw221x_i2c_close(void)
{
    //BC_LOG_INFO("touch_i2c_close**************\r\n");
	q_device_close(cw221x_i2c_dev);
}




// bool touch_irq_register_callback(void *callback)
// {
	// if(callback != NULL)
	// {
		// int_io_irq_callback = callback;
		// return true;
	// }
	// return false;
// }
	
bool cw221x_i2c_write(uint8_t slave_addr,uint8_t reg_add ,uint8_t *data,uint8_t length)
{
    //BC_LOG_INFO("touch_i2c_write regadd:0x%02x, length:%d\r\n", reg_add,length);
	i2c_pack.reg_addr = reg_add;
	i2c_pack.write_length = length;
	i2c_pack.write_buff = data;
	if(q_device_write(cw221x_i2c_dev,0,&i2c_pack,0)== RESULT_OK)
	{
		bc_delay_ms(10);
		return true;
	}
	else
	{
		return false;
	}
}
																 
bool cw221x_i2c_read(uint8_t slave_addr,uint8_t reg_add ,uint8_t *data,uint8_t length)
{
    //BC_LOG_INFO("touch_i2c_read regadd:0x%02x, length:%d\r\n", reg_add,length);
	i2c_pack.reg_addr = reg_add;
	i2c_pack.read_length = length;
	i2c_pack.read_buff = data;
	if(q_device_read(cw221x_i2c_dev,0,&i2c_pack,0) == RESULT_OK)
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

void bc_cw221x_device_find(void)
{

	cw221x_i2c_dev = q_device_find("i2c_1");
	q_device_assert(cw221x_i2c_dev);

}



 





