#include "bc_nfc.h"

#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"

#include "bc_util.h"

#include "app_error.h"

#include "nfc_t4t_lib.h"
#include "nfc_ndef_msg.h"
#include "nfc_t2t_lib.h"
#include "nfc_uri_msg.h"

#include "ring_config.h"


#if ( HARDWARE_181_ENABLED == 1  )	

#include "bc_nfc_rs2323_port.h"
	
#endif

#define NDEF_FILE_SIZE 512

static uint8_t m_ndef_msg_buf[NDEF_FILE_SIZE];      /**< Buffer for NDEF file. */
//static uint8_t m_ndef_msg_len;                      /**< Length of the NDEF message. */
static uint8_t file_txt_handler[9] = {0x00,0x1B,0xD1,0x01,0x17,0x54,0x02,0x65,0x6e};

//static  uint8_t url[] =
//    {'n', 'o', 'r', 'd', 'i', 'c', 's', 'e', 'm', 'i', '.', 'c', 'o', 'm'}; //URL "nordicsemi.com"

static  char url[128] = "bravechip.com";

typedef void (*nfc_recv_callback)(const uint8_t *data,uint32_t length); 

static nfc_recv_callback recv_callback = NULL;

static uint8_t apdu_buf[256] = {0}; // Buffer for APDU data
static uint8_t recv_buf[256] = {0}; // Buffer for APDU data


static void nfc_callback(void * p_context, nfc_t2t_event_t event, const uint8_t * p_data, size_t data_length)
{
    (void)p_context;

    switch (event)
    {
        case NFC_T2T_EVENT_FIELD_ON:
//            bsp_board_led_on(BSP_BOARD_LED_0);
            break;

        case NFC_T2T_EVENT_FIELD_OFF:
//            bsp_board_led_off(BSP_BOARD_LED_0);
            break;

        default:
            break;
    }
}


bool bc_nfc_recv_register_callback(void *callback)
{
	if(callback != NULL)
	{
		recv_callback = (nfc_recv_callback)callback;
		return true;
	}
	return false;
}

bool bc_nfc_write(uint8_t *data,uint16_t length)
{
	if(length > NDEF_FILE_SIZE - sizeof(file_txt_handler))
	{
		return false;
	}
	memcpy(m_ndef_msg_buf,file_txt_handler,sizeof(file_txt_handler));
	memcpy(&m_ndef_msg_buf[sizeof(file_txt_handler)],data,length);
	m_ndef_msg_buf[1] = length+3+4;
	m_ndef_msg_buf[4] = length+3;
	return true;
}

bool bc_nfc_url_set(uint8_t *url_data,uint8_t url_length)
{
	if(url_length > 128)
	{
		return false;
	}
	memset((uint8_t*)url,0,sizeof(url));
	memcpy((uint8_t*)url,url_data,url_length);
	return true;
}

void bc_nfc_init_start(void)
{
	ret_code_t err_code;
#if ( HARDWARE_181_ENABLED == 1  )	

    bc_nfc_nordic_on();
	
#endif
		
	
	err_code = nfc_t2t_setup(nfc_callback, NULL);
    APP_ERROR_CHECK(err_code);

    /** @snippet [NFC URI usage_1] */
    /* Provide information about available buffer size to encoding function */
    uint32_t len = sizeof(m_ndef_msg_buf);
     printf("url length:%d \r\n",strlen(url));
	 printf("url:%s \r\n",(char*)url);
    /* Encode URI message into buffer */
    err_code = nfc_uri_msg_encode( NFC_URI_HTTP_WWW,
                                   (uint8_t*)url,
                                   strlen(url),
                                   m_ndef_msg_buf,
                                   &len);

    APP_ERROR_CHECK(err_code);
    /** @snippet [NFC URI usage_1] */

    /* Set created message as the NFC payload */
    err_code = nfc_t2t_payload_set(m_ndef_msg_buf, len);
    APP_ERROR_CHECK(err_code);

    /* Start sensing NFC field */
    err_code = nfc_t2t_emulation_start();
    APP_ERROR_CHECK(err_code);
}




void bc_nfc_init_stop(void)
{
	ret_code_t err_code;
	err_code =  nfc_t2t_emulation_stop();
	APP_ERROR_CHECK(err_code);
}
















