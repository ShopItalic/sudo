#include "bc_buf.h"

#include "bc_puf_i2c_driver.h"

#include "bc_logger.h"
#include "bc_ldo_switch.h"
#include "bc_delay.h"

#include <string.h>

#include "HD13HS10K.h"



static char temp_data_1[32] = {1,2,3,4};
static char temp_data_2[32] = {1,2,3,4};



uint8_t bc_buf_chip_id_get(void)
{
	bool ret= true;
	bc_ldo_puf_power_on();
	bc_buf_i2c_device_open();
	bc_delay_ms(10);
	hd13hs10k_puf_set_enable();
    bc_delay_ms(10);
	ret = hd13hs10k_get_puf_id(temp_data_1, 32);
	ret = hd13hs10k_get_puf_id(temp_data_2, 32);
//	BC_LOG_HEX("chip id resp data :",temp_data_1,sizeof(temp_data_1));
//	BC_LOG_HEX("chip id resp data :",temp_data_2,sizeof(temp_data_2));
	if (!ret) {
		printf("get puf id error.\n");
		hd13hs10k_puf_set_disable();
		bc_buf_i2c_device_close();
		bc_ldo_puf_power_off();
		return 0x01;
	}
	hd13hs10k_puf_set_disable();
	bc_buf_i2c_device_close();
	bc_ldo_puf_power_off();
	if(memcmp(temp_data_1,temp_data_2,sizeof(temp_data_1)) != 0)
	{
		return 0x01;  //error
	}
	return 0x02;  //ok
}

bool bc_buf_resp(char *challenge, unsigned int challenge_len, char *resp, unsigned int resp_len)
{
	bool ret= true;
	bc_ldo_puf_power_on();
	bc_buf_i2c_device_open();
	bc_delay_ms(10);
	hd13hs10k_puf_set_enable();
	
	ret = hd13hs10k_get_puf_resp( challenge, challenge_len, resp, resp_len);
	
	hd13hs10k_puf_set_disable();
	bc_buf_i2c_device_close();
	bc_ldo_puf_power_off();
	
	return ret;

}

uint8_t bc_buf_id(char *resp)
{
	bool ret= true;
	bc_ldo_puf_power_on();
	bc_buf_i2c_device_open();
	bc_delay_ms(10);
	hd13hs10k_puf_set_enable();
    bc_delay_ms(10);
	ret = hd13hs10k_get_puf_id(temp_data_1, 32);

	hd13hs10k_puf_set_disable();
	bc_buf_i2c_device_close();
	bc_ldo_puf_power_off();
	memcpy((uint8_t *) resp,temp_data_1,32);
	return 0x02;
}

uint8_t bc_buf_chip_id_hardware_check(void)
{
	if(bc_buf_chip_id_get() == 0x02)
	{
		return true;
	}
	return false;
}


//char resp[32] = {0};
//void test_puf(void)
//{
//	bool ret= true;
//	
//	
//	bc_delay_ms(10);
////	hd13hs10k_puf_set_disable();
//	 bc_delay_ms(10);
//	hd13hs10k_puf_set_enable();
//   bc_delay_ms(10);
//	ret = hd13hs10k_get_puf_id(data, 32);
//	if (!ret) {
//		printf("get puf id error.\n");
//		goto error;
//	}
//	for(uint8_t i = 1; i < 32; i ++)
//	{
//		data[i]=10;
//	}
//	BC_LOG_HEX("send resp data :",data,sizeof(data));
//	bc_delay_ms(10);
//	ret = hd13hs10k_get_puf_resp( data, 32, resp, 32);
//	if (!ret) {
//		printf("get puf resp error.\n");
//		goto error;
//	}

//	BC_LOG_HEX("resp data:",resp,32);

//error:

//	hd13hs10k_puf_set_disable();
////	bc_buf_i2c_device_close();
//}
















