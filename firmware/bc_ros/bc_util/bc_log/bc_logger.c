#include "bc_logger.h"
#include "app_uart.h"
#include "app_package.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "bc_util.h"

#define LOG_BUFFER_SIZE 240
static    char buf[LOG_BUFFER_SIZE];
static   bool bc_log_ble_enable_flag = false;

struct log_package
{
	uint8_t buff[LOG_BUFFER_SIZE];
	uint8_t length;
};

static struct log_package log_pack = {0};

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)
#include "sdk_config.h"
#include "SEGGER_RTT.h"

#elif (HARDWARE_ARCH_TYPE_PHY6222 == 1)	

extern void _uart_putc(char* data, uint16_t size);

#endif

int fputc(int ch, FILE *stream)
{

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)
	
#if NRF_LOG_ENABLED	
	SEGGER_RTT_Write(0,(uint8_t*)&ch,1);
#endif

#elif (HARDWARE_ARCH_TYPE_PHY6222 == 1)	
	
#if (DEBUG_INFO == 1)
	
    _uart_putc((char*)&ch, 1);    //todo
	
#endif
	
#endif
//	if(bc_log_ble_enable_flag)
//	{
//		if(log_pack.length >= LOG_BUFFER_SIZE)
//		{
//			app_package_ble_log_up(log_pack.buff,log_pack.length);
//			memset((uint8_t*)&log_pack,0,sizeof(log_pack));
//			return ch;
//		}
//		if(ch == 0x0A && log_pack.buff[log_pack.length-1] == 0x0D)
//		{
//			log_pack.buff[log_pack.length] = (uint8_t)ch;
//			log_pack.length++;
//			app_package_ble_log_up(log_pack.buff,log_pack.length);
//			memset((uint8_t*)&log_pack,0,sizeof(log_pack));
//		}
//		else
//		{
//			log_pack.buff[log_pack.length] = (uint8_t)ch;
//			log_pack.length++;
//		}
//		
//	}
	
    return ch;
}




int fgetc(FILE * p_file)
{
 uint8_t input;
#if (HARDWARE_ARCH_TYPE_NORDIC == 1)
	
#if NRF_LOG_ENABLED		
	SEGGER_RTT_Read(0,&input,1);
#endif
    

#elif (HARDWARE_ARCH_TYPE_PHY6222 == 1)	
	
#if (DEBUG_INFO == 1)
	

	
#endif
	
#endif
	
    return input;
}


void BC_LOG_HEX_P(char *_cData, uint8_t *_pu8Data ,uint16_t _u16Len)
{
#if defined(SUDO_VOICE_ONLY) && (HARDWARE_ARCH_TYPE_NORDIC == 1)
  bc_log_task_hex(_cData, _pu8Data, _u16Len);
  return;
#endif
  if(bc_log_ble_enable_flag)
  {
	  if(_u16Len > 120)
	  {
		  return;
	  }
	  
	  BC_LOG_BLE(" %s %s\r\n", _cData, hex2Str( _pu8Data,_u16Len));
  }
  else
  {
	  uint16_t i;
	  printf("%s:",_cData);
	  for(i = 0;i < _u16Len;i++)
	  {
		printf("%02x ",_pu8Data[i]);
	  }
	  printf("\r\n");
  }
}



void BC_LOG_BLE(const char* format, ...)
{
	if(bc_log_ble_enable_flag)
	{
		va_list argptr;
		va_start(argptr, format);
		int cnt = vsnprintf(buf, LOG_BUFFER_SIZE, format, argptr);
		va_end(argptr);
		if(strlen(buf) > 240)
		{
			memset(buf,0,LOG_BUFFER_SIZE);
			return;
		}
//		app_package_ble_log_up((uint8_t*)buf,strlen(buf));
		memset(buf,0,LOG_BUFFER_SIZE);
	}
    return ;
}


void bc_log_ble_enable(void)
{
	bc_log_ble_enable_flag = true;
}

void bc_log_ble_disenable(void)
{
	bc_log_ble_enable_flag = false;
}

#if 1
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
static char ble_log_buf[240];
static uint8_t tx_buffer[256] = {0};
void services_print_log(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    int cnt = vsnprintf(ble_log_buf, 240, format, argptr);
    va_end(argptr);
    if(strlen(ble_log_buf) > 240){
        memset(ble_log_buf,0,240);
        return;
    }
    tx_buffer[0] = 0x0;
    tx_buffer[1] = 0x1;
    tx_buffer[2] = 0xF2;
    tx_buffer[3] = 0x2A;
    tx_buffer[4] = 0x01;
    tx_buffer[5] = strlen(ble_log_buf);
    memcpy(&tx_buffer[6],ble_log_buf,strlen(ble_log_buf));
    //services_info_send(tx_buffer, strlen(ble_log_buf) + 6);
    app_package_ble_log_up(tx_buffer,strlen(ble_log_buf)+6);
}

#endif

