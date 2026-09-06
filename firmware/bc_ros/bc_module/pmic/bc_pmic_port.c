#include <string.h>
#include "ring_config.h"
#include "bc_pmic_port.h"
#include "q_device.h"
#include "bc_pmic.h"

#if (PMIC_TYPE == 0) || (PMIC_TYPE == 1)

static q_device_t *i2c_dev;

static struct i2c_package i2c_pack = {
	                                     .slave_addr = 0x07 << 1,
	                                     .write_length = 1,
	                                     .read_length = 1,
                                     };

//I2C初始化
void pmic_i2c_init(void)
{
    i2c_dev = q_device_find("i2c_2_sys");
    q_device_assert(i2c_dev);
}

//I2C打开
void pmic_i2c_open(void)
{
    q_device_open(i2c_dev);
}

//I2C关闭
void pmic_i2c_colse(void)
{
    q_device_close(i2c_dev);
}

//读I2C
int pmic_i2c_read(uint8_t reg, uint8_t *pData, uint8_t size)
{
	int32_t ret = PMIC_FAIL;
	int32_t retry = 0;

	while((ret==PMIC_FAIL) && (retry++ < 5))
	{
		i2c_pack.reg_addr = reg;
		i2c_pack.read_length =size;
        
		if(q_device_read(i2c_dev,0,&i2c_pack,0) == RESULT_OK)
		{
			memcpy(pData,i2c_pack.read_buff,size);
			
			return PMIC_SUCCESS;
		}
	}

	return ret;
}

//写I2C
int pmic_i2c_write(uint8_t reg, uint8_t *pData, uint8_t size)
{
	int ret = PMIC_FAIL;
	int retry = 0;

	while((ret==PMIC_FAIL) && (retry++ < 5))
	{
		i2c_pack.reg_addr = reg;
	    i2c_pack.write_length = size;
	    i2c_pack.write_buff[0] = *pData;
		 
		if(q_device_write(i2c_dev,0,&i2c_pack,0)== RESULT_OK)
		{
			return PMIC_SUCCESS;
		}
	}
	return ret;
}

const pmic_i2c_t pmic_i2c = {
    .i2c_read = pmic_i2c_read,
    .i2c_write = pmic_i2c_write,
    .i2c_init = pmic_i2c_init,
    .i2c_open = pmic_i2c_open,
    .i2c_close = pmic_i2c_colse,
};

#elif (PMIC_TYPE == 2)
#endif