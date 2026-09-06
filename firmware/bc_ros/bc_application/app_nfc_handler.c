#include "app_nfc_handler.h"


#include "bc_nfc.h"
#include "app_cmd_handler.h"
#include "bc_logger.h"
#include "bc_device_info.h"

#if ( HARDWARE_181_ENABLED == 1  )	

#include "bc_nfc_rs2323_port.h"
	
#endif

void bc_nfc_recv_callback(uint8_t *recv_data,uint8_t recv_length)
{
	app_cmd_package_parse(recv_data,recv_length);
	BC_LOG_HEX("nfc recv:",recv_data,recv_length);
}


void app_nfc_start(void)
{
	uint8_t url[128] = {0};
	uint8_t url_length = 0;
	
	bc_device_info_get_url(url,&url_length);
	if(url_length != 0 && url_length != 0xff && url_length <= 128)
	{
		bc_nfc_url_set(url,url_length);
		printf("url length:%d \r\n",url_length);
		printf("url:%s \r\n",(char*)url);
	}
	
	bc_nfc_recv_register_callback(bc_nfc_recv_callback);
	bc_nfc_init_start();
}



void app_nfc_init(void)
{
#if ( HARDWARE_181_ENABLED == 1  )	

	bc_device_nfc_info  *nfc_info =   bc_device_nfc_info_get();
	if(nfc_info->nfc_mode == 0)
	{
		app_nfc_start();
	}
	else if(nfc_info->nfc_mode == 1)
	{
		bc_nfc_exit_FM11RF08_on();
	}
#else
	
	app_nfc_start();
   
#endif	
		
	
}

void app_nfc_stop(void)
{
	bc_nfc_init_stop();
}





