#include <string.h>
#include "q_device.h"
#include "bc_ppg.h"

static q_device_t *i2c_dev;

static struct i2c_package i2c_pack = {
	                                     .slave_addr = 0x44,
	                                     .write_length = 1,
	                                     .read_length = 1,
                                     };

void ppg_i2c_init()
{
	i2c_dev = q_device_find("i2c_1_ppg");
	q_device_assert(i2c_dev);
}

void ppg_i2c_open(void)
{
	q_device_open(i2c_dev);
}

void ppg_i2c_close(void)
{
	q_device_close(i2c_dev);
}

bool ppg_write_reg(uint8_t addr, uint8_t data) 
{
	int ret = PPG_FAIL;
	int retry = 0;

	while((ret==PPG_FAIL) && (retry++ < 5))
	{
		i2c_pack.reg_addr = addr;
	    i2c_pack.write_length = 1;
	    i2c_pack.write_buff[0] = data;
		 
		if(q_device_write(i2c_dev,0,&i2c_pack,0)== RESULT_OK)
		{
			return PPG_SUCCESS;
		}
	}
	return ret;
}

uint8_t ppg_read_reg(uint8_t addr)
{
	int32_t ret = PPG_FAIL;
	int32_t retry = 0;
    uint8_t res_data = 0;

	while((ret==PPG_FAIL) && (retry++ < 5))
	{
		i2c_pack.reg_addr = addr;
		i2c_pack.read_length =1;
        
		if(q_device_read(i2c_dev,0,&i2c_pack,0) == RESULT_OK)
		{
            res_data = i2c_pack.read_buff[0];
			
			return res_data;
		}
	}

	return 0;
}

int ppg_brust_read_reg(uint8_t addr , uint8_t *buf, uint8_t length) 
{
    int32_t ret = PPG_FAIL;
	int32_t retry = 0;

	while((ret==PPG_FAIL) && (retry++ < 5))
	{
		i2c_pack.reg_addr = addr;
		i2c_pack.read_length =1;
        
		if(q_device_read(i2c_dev,0,&i2c_pack,0) == RESULT_OK)
		{
            memcpy(buf,i2c_pack.read_buff,length);
			
			return PPG_SUCCESS;
		}
	}

	return ret;
}
