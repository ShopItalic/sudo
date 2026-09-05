/*******************************************************************************
此为nfc模块接口文件，为nfc提供api接口

日  期：2024年1月30日
编写人：邱成凯
 *******************************************************************************/



#include "bc_nfc_port.h"


#include "q_device.h"

#include "bc_timer.h"
#include "bc_logger.h"

#include "bc_rtc.h"
#include "bc_delay.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "st25r3916_irq.h"

uint8_t globalCommProtectCnt = 0;   /*!< Global Protection counter     */

static q_device_t *spi_dev;
static q_device_t *nfc_exit_dev;  //设备描述

static struct spi_package spi_pack = {0};

void *bc_nfc_exit_lo_to_hi_irq_callback = NULL;
void *bc_nfc_exit_hi_to_lo_irq_callback = NULL;



void bc_nfc_exit_irq_disable(void)
{
	q_device_open(nfc_exit_dev);	
	q_device_reg_callback(nfc_exit_dev,GPIOT_CONFIG_POLARITY_LoToHi,bc_nfc_exit_lo_to_hi_irq_callback);    //注册回调	
//	q_device_reg_callback(nfc_exit_dev,GPIOT_CONFIG_POLARITY_HiToLo,bc_nfc_exit_hi_to_lo_irq_callback);	  //注册回调
}

void bc_nfc_exit_irq_enable(void)
{
	q_device_close(nfc_exit_dev);	
}

uint8_t bc_nfc_exit_irq_io_state(void)
{
	uint8_t state = 0;
	q_device_read(nfc_exit_dev,0,&state,1);
	return state;
}


uint32_t bc_nfc_get_tick(void)
{
	return bc_systick_get();//bg_rtc_time_get_uinx_time();
}

void bc_nfc_delay(uint32_t ms)
{
	bc_delay_ms(ms);
}


void bc_nfc_error_handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while (1)
  {
//	  printf("nfc error:%s(%d)\r\n",file,line);
  }
  /* USER CODE END Error_Handler_Debug */
}

int32_t bc_nfc_spi_write_and_recv(const uint8_t * const write_data, uint8_t * const recv_data, uint16_t length)
{
//	q_device_open(spi_dev);
	if((write_data != NULL) && (recv_data != NULL))
	{
		memcpy(spi_pack.write_buff,write_data,length);
		spi_pack.read_length = length;
		spi_pack.write_length = length;
		q_device_write(spi_dev,0,&spi_pack,0);	
	}
	else if(write_data != NULL && recv_data == NULL)
	{
		memcpy(spi_pack.write_buff,write_data,length);
		spi_pack.read_length = 0;
		spi_pack.write_length = length;
		q_device_write(spi_dev,0,&spi_pack,0);
	}
	else if(recv_data != NULL && write_data == NULL)
	{
		spi_pack.read_length = length;
		spi_pack.write_length = 0;
		q_device_read(spi_dev,0,&spi_pack,0);
		memcpy(recv_data,spi_pack.read_buff,spi_pack.read_length);
	}
	else
	{
//		q_device_close(spi_dev);
		return -2;
	}
//	q_device_close(spi_dev);
	return 0;
}

void bc_nfc_spi_cs_high(void)
{
	q_device_ctrl(spi_dev,GPIO_OUTPUT_HIGH,NULL);
}

void bc_nfc_spi_cs_low(void)
{
	q_device_ctrl(spi_dev,GPIO_OUTPUT_LOW,NULL);
}


void bc_nfc_open_spi(void)
{
	q_device_open(spi_dev);
}

void bc_nfc_close_spi(void)
{
	q_device_close(spi_dev);
}

void bc_nfc_irq_state_handler(void)
{
	st25r3916Isr();
}

void bc_nfc_io_irq_reg_callback(void *lo_to_hi_irq_callback,void *hi_to_lo_irq_callback)
{
	bc_nfc_exit_lo_to_hi_irq_callback = lo_to_hi_irq_callback;
    bc_nfc_exit_hi_to_lo_irq_callback = hi_to_lo_irq_callback;
}

int bc_bfc_log(const char* format, ...)
{

    #define LOG_BUFFER_SIZE 256
    char buf[LOG_BUFFER_SIZE];
    va_list argptr;
    va_start(argptr, format);
    int cnt = vsnprintf(buf, LOG_BUFFER_SIZE, format, argptr);
    va_end(argptr);

    /* */
//    logUsartTx((uint8_t*)buf, strlen(buf));
	BC_LOG_HEX("nfc log:",buf,strlen(buf));
    return cnt;
  
}


/*******************************************************************************
 * Function Name     : bc_io_input_test
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
void bc_nfc_device_find(void)
{
	nfc_exit_dev = q_device_find("key");
	q_device_assert(nfc_exit_dev);
	
	spi_dev = q_device_find("spi_0");
	q_device_assert(spi_dev);
}













