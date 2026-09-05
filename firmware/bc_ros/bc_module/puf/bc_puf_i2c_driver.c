#include "bc_puf_i2c_driver.h"


#include "q_device.h"
#include "HD13HS10K.h"

#include "string.h"

static q_device_t *i2c_dev;

static struct i2c_package i2c_pack = {
	                                     .slave_addr = PUF_DEV_ADDR << 1,
	                                     .write_length = 1,
	                                     .read_length = 1,
                                     };



bool bc_buf_i2c_device_write(uint8_t reg_add ,uint8_t *data,uint8_t length)
{
	i2c_pack.reg_addr = reg_add;;
	i2c_pack.write_length = length;
	i2c_pack.write_buff = data;
//	i2c_pack.slave_addr = 0xC8;
//	memcpy(i2c_pack.write_buff,data,i2c_pack.write_length);

	if(q_device_write(i2c_dev,0,&i2c_pack,0)== RESULT_OK)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool bc_buf_i2c_device_read(uint8_t reg_add ,uint8_t *data,uint8_t length)
{
	i2c_pack.reg_addr = reg_add;
	i2c_pack.read_length = length;
	i2c_pack.read_buff = data;
//	i2c_pack.slave_addr = 0xC9;
	if(q_device_read(i2c_dev,0,&i2c_pack,0) == RESULT_OK)
	{
//		BC_LOG_HEX("recv data",i2c_pack.read_buff,length);
//		memcpy(data,i2c_pack.read_buff,length);
		return true;
	}
	else
	{
		return false;
	}
}

									 
									 
void bc_buf_i2c_device_open(void)
{
	q_device_open(i2c_dev);
}


void bc_buf_i2c_device_close(void)
{
	q_device_close(i2c_dev);
}										 
									 
void bc_buf_i2c_device_find(void)
{
	i2c_dev = q_device_find("i2c_3");
	q_device_assert(i2c_dev);

}











