#include "HD13HS10K.h"

#include "bc_puf_i2c_driver.h"

#include "string.h"
#include "stdio.h"
#include "bc_logger.h"

static bool hd13hs10k_puf_send(uint8_t *data,uint8_t length)
{
	return bc_buf_i2c_device_write(PUF_DATA_ADDR,data,length);
}

static bool hd13hs10k_puf_read(uint8_t *data,uint8_t length)
{
	return bc_buf_i2c_device_read(PUF_DATA_ADDR,data,length);
}

/* 使能PUF设备 */
bool hd13hs10k_puf_set_enable(void)
{
	unsigned char command = ENABLE_PUF;
	return hd13hs10k_puf_send(&command,1);
}

/* 设置PUF为disable状态 */
bool hd13hs10k_puf_set_disable(void)
{
	unsigned char command = DISABLE_PUF;
	return hd13hs10k_puf_send(&command,1);
}



/* 获取PUF的ID */
char zero[33] = {0};
bool hd13hs10k_get_puf_id(char *id, unsigned int size)
{
	bool ret = true;
	zero[0] = WORK_PUF;
//	BC_LOG_HEX("send id:",zero,sizeof(zero));
	ret = hd13hs10k_puf_send((uint8_t*)zero, sizeof(zero));
	if(!ret)
	{
		return ret;
	}
	ret = hd13hs10k_puf_read((uint8_t*)id, size);
	return ret;
}

/* 通过挑战值获取PUF设备响应值 */
 bool hd13hs10k_get_puf_resp(char *challenge, unsigned int challenge_len, 
		 	 char *resp, unsigned int resp_len)
{
	bool ret = true;
	char buffer[36] = {0};

	buffer[0] = WORK_PUF;
	uint8_t  len  =0;
	if (challenge_len > 32)
		len= 32;
	else
		len = challenge_len;

	memcpy(&buffer[1], challenge, challenge_len);
    BC_LOG_HEX("test challenge:",buffer,len + 1);
	ret = hd13hs10k_puf_send((uint8_t*)buffer, len + 1);
	if (!ret) {
		return ret;
	}

	ret = hd13hs10k_puf_read((uint8_t*)resp, (uint8_t)resp_len);
	BC_LOG_HEX("test resp :",resp,resp_len);
	return ret;
}

