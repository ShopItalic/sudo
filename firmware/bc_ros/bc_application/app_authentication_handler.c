#include "app_authentication_handler.h"

#include "app_package.h"

#include "lib_hr.h"

#include "bc_timer.h"
#include "bc_ble_modu_interface.h"
#include "bc_util.h"
#include "bc_led.h"
//#include "bc_logger.h"
#include "bc_device_info.h"

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>


struct authentication_info
{
	unsigned  char device_name[32];
	unsigned  char device_id[20];
	unsigned char req_code[128];
	unsigned char key_code[128];
	unsigned int req_code_len;
	unsigned int key_code_len;
};

struct ppg_data_buff
{
	int32_t ir_data[10];
};

struct ppg_package
{
	struct ppg_data_buff ppg_data[50];
	uint8_t ppg_count;
	uint16_t ppg_data_length;
	
};

static float(* Fm)[8] = NULL;

struct ppg_package ppg_pack = {0};

static bool app_authentication_staus = false;  //激活状态,flase 未激活，true 已激活

static struct authentication_info authentica_info = {
	.device_name = "hengaigaoke",
	.device_id = "F82870012345",

};

//uint8_t app_hengai_alg_buff[2048] = {0};


static void app_authentication_timer_callback (void * pvParameter);



static bc_timer_struct  timer_struct = {
	
		.timer_name = "authentication timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 1000*2,
		.lock = false,
		.timer_callback_function = app_authentication_timer_callback,
	
};

static void app_authentication_timer_callback (void * pvParameter)
{
	if(!app_authentication_staus)
	{
		app_package_authentication_req_code_up();
	}
	else
	{
		bc_timer_stop(&timer_struct);
	}
}	

static void BC_LOG_HEX_P(char *_cData, int *_pu8Data ,uint16_t _u16Len)
{
  uint16_t i;
  printf("%s:",_cData);
  for(i = 0;i < _u16Len;i++)
  {
	  printf("%0d ",_pu8Data[i]);
	  if(i %10 == 0)
	  {
		   printf("\n");
	  }
  }
  printf("\r\n");
}
static void log_print(const char* format, ...)
{
	#define LOG_BUFFER_SIZE 256
	char buf[LOG_BUFFER_SIZE];
	va_list argptr;
	va_start(argptr, format);
	int cnt = vsnprintf(buf, LOG_BUFFER_SIZE, format, argptr);
	va_end(argptr);

	/* */
	printf("%s",buf);
    return;
}

static void app_authentication_get_version(void)
{
    char ver[20];
    unsigned int ver_len;
    version_get_string(ver, &ver_len);
    printf("result is %s\n%d\n", ver, ver_len);
}

static void app_authentication_request_validation(void)
{
	request_validation(authentica_info.device_name, strlen((char*)authentica_info.device_name),authentica_info.device_id,strlen((char*)authentica_info.device_id),authentica_info.req_code,&authentica_info.req_code_len);
	printf("req code:");
	for(uint8_t i= 0; i < authentica_info.req_code_len;i++)
	{
		printf("0x%02x ",authentica_info.req_code[i]);
	}
	printf("\r\n authentica_info.req_code_len:%d \r\n",authentica_info.req_code_len);
}

static int app_authentication_activate_algorithm(void)
{
	int result;
    result = activate_algorithm(authentica_info.req_code, authentica_info.req_code_len, authentica_info.key_code, authentica_info.key_code_len);  //result=1表示算法激活
	
	if(result == 1)
	{
		//存储key code
		app_authentication_staus = true;
		bc_device_info_set_key_code(authentica_info.key_code,authentica_info.key_code_len);
		printf("authentica_info.key_code:%s \r\n",authentica_info.key_code);
		printf("\r\n authentica_info.key_code_len:%d \r\n",authentica_info.key_code_len);
//		bc_led_blue_on();
	}
	else
	{
		app_authentication_staus = false;
	}
	
	return result;
}

static void authentication_info_init(void)
{
	struct bc_ble_calss ble_calss = bc_ble_new();
	uint8_t ble_mac[6] = {0};
	ble_calss.ble_mac_get(ble_mac);
	uint8_t len = splitHexArray(ble_mac, sizeof(ble_mac), (char*)authentica_info.device_id, sizeof(authentica_info.device_id)) ;
	printf("ble mac:%s \r\n",authentica_info.device_id);
	bc_device_info_get_key_code((uint8_t*)authentica_info.key_code,(uint8_t*)&authentica_info.key_code_len);
	
	
	if(authentica_info.key_code_len != 0 && authentica_info.key_code_len <= 128)
	{
		app_authentication_staus = true;
		printf("key code:%s \r\n",authentica_info.key_code);
	}
	else
	{
		app_authentication_staus = false;
		memset(authentica_info.key_code,0,128);
		authentica_info.key_code_len = 0;
	}
	printf("key_code_len:%d \r\n",authentica_info.key_code_len);
}




int app_authentication_activate_algorithm_key(uint8_t *key_code,uint8_t key_length)
{
	if(key_length > sizeof(authentica_info.key_code))
	{
		return 0;
	}
	memcpy(authentica_info.key_code,key_code,key_length);
	authentica_info.key_code_len = key_length;
	return app_authentication_activate_algorithm();
}

void app_authentcation_states_get(uint8_t *status,uint8_t*code,uint8_t *code_length)
{
	if(app_authentication_staus)
	{
		*status = 1;
		memcpy(code,authentica_info.key_code,authentica_info.key_code_len);
		*code_length = authentica_info.key_code_len;
		return ;
	}
	*status = 0;
	memcpy(code,authentica_info.req_code,authentica_info.req_code_len);
	*code_length = authentica_info.req_code_len;
	return ;
}

void app_authentcation_req_code_get(uint8_t*code,uint8_t *code_length)
{
	memcpy(code,authentica_info.req_code,authentica_info.req_code_len);
	*code_length = authentica_info.req_code_len;
}



void app_authentication_info_init(void)
{
	
	authentication_info_init();
//	
	app_authentication_request_validation();
//	app_authentication_activate_algorithm_key((uint8_t*)temp,strlen(temp));
//	app_package_authentication_test_recv();
//	if(!bc_timer_create(&timer_struct))
//	{
//		printf("create %s fial!! \r\n",timer_struct.timer_name);
//	}
//	else
//	{
//		printf("create %s success!! \r\n",timer_struct.timer_name);
////		bc_timer_start(&timer_struct);
//	}
		
}


















