#include "q_device.h"
#include "qma6100.h"

static q_device_t *i2c_dev;

static struct i2c_package i2c_pack = {
#if (QMA_ADO_TYPE == 0)
	                                     .slave_addr = 0x12,
#elif (QMA_ADO_TYPE == 1)
                                         .slave_addr = 0x13,
#endif
	                                     .write_length = 1,
	                                     .read_length = 1,
                                     };

void gsensor_i2c_init(void)
{
	i2c_dev = q_device_find("i2c_0_acc");
	q_device_assert(i2c_dev);
}

void gsensor_i2c_open(void)
{
	q_device_open(i2c_dev);
}

void gsensor_i2c_close(void)
{
	q_device_close(i2c_dev);
}

int32_t gsensor_writereg(uint8_t reg_add,uint8_t reg_dat)
{
	int32_t ret = QMA6100_FAIL;
	int32_t retry = 0;

	while((ret==QMA6100_FAIL) && (retry++ < 5))
	{
		i2c_pack.reg_addr = reg_add;
	    i2c_pack.write_length = 1;
	    i2c_pack.write_buff[0] = reg_dat;
		 
		if(q_device_write(i2c_dev,0,&i2c_pack,0)== RESULT_OK)
		{
			return QMA6100_SUCCESS;
		}
	}
	return ret;
}

int32_t gsensor_readreg(uint8_t reg_add,uint8_t *buf,uint16_t num)
{
	int32_t ret = QMA6100_FAIL;
	int32_t retry = 0;

	while((ret==QMA6100_FAIL) && (retry++ < 5))
	{

		i2c_pack.reg_addr = reg_add;
		i2c_pack.read_length =num;
		if(q_device_read(i2c_dev,0,&i2c_pack,0) == RESULT_OK)
		{
			memcpy(buf,i2c_pack.read_buff,num);
			
			return QMA6100_SUCCESS;
		}
		qma6100_delay(5);
	}

	return ret;
}
