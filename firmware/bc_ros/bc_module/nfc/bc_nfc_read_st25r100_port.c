#include "bc_nfc_read_st25r100_port.h"


#include "q_device.h"

#include <string.h>
#include "bc_logger.h"
#include "bc_delay.h"


static q_device_t *spi_flash_dev = NULL;
static q_device_t *int_io_dev = NULL;                                  //设备描述
static q_device_t *reset_io_dev = NULL;                                  //设备描述


static void *int_io_irq_callback = NULL;

static struct spi_package  spi_pack = {0};

void bc_nfc_read_spi_flash_cs_high(void)
{
	q_device_ctrl(spi_flash_dev,GPIO_OUTPUT_HIGH,0);
}

void bc_nfc_read_spi_flash_cs_low(void)
{
	q_device_ctrl(spi_flash_dev,GPIO_OUTPUT_LOW,0);
}

bool bc_nfc_read_spi_flash_write_and_read(const uint8_t *write_buff,uint8_t *read_buff,uint16_t length)
{
	bool ret = false;
  uint8_t temp_buff[256] = {0};
  memcpy(temp_buff,write_buff,length);
	spi_pack.write_buff = temp_buff;
	spi_pack.read_buff = read_buff;
	spi_pack.write_length = length;
	spi_pack.read_length = length;
	if(q_device_write(spi_flash_dev,0,&spi_pack,0) == RESULT_OK)
	{
		ret = true;
	}
	
	return    ret;
}


void bc_nfc_read_int_io_irq_enable(void)
{

	q_device_open(int_io_dev);
	q_device_reg_callback(int_io_dev,GPIOT_CONFIG_POLARITY_HiToLo,int_io_irq_callback);	  //注册回调

}

void bc_nfc_read_int_io_irq_disable(void)
{
		q_device_close(int_io_dev);
}

bool bc_nfc_read_io_irq_register_callback(void *callback)
{
	if(callback != NULL)
	{
		int_io_irq_callback = callback;
		return true;
	}
	return false;
}

uint8_t bc_nfc_read_int_io_status_get(uint8_t port,uint8_t pin)
{
	uint8_t io_status = 0;
	q_device_read(int_io_dev,0,&io_status,0);
	return io_status;
}


void bc_nfc_reset_io_enable(void)
{
  q_device_open(reset_io_dev);
	if(reset_io_dev == NULL)
  {
  BC_LOG_INFO("reset_io_dev NULL\r\n");
      
  }
}

void bc_nfc_reset_io_disable(void)
{
  q_device_close(reset_io_dev);
}

void bc_nfc_reset_io_control(uint8_t pin_level)
{
  q_device_ctrl(reset_io_dev,pin_level,0);
}

void bc_nfc_reset_low(void)
{
	q_device_ctrl(reset_io_dev,GPIO_OUTPUT_LOW,0);
}

void bc_nfc_reset_high(void)
{
	q_device_ctrl(reset_io_dev,GPIO_OUTPUT_HIGH,0);
}


void bc_nfc_reset_output_low(uint8_t port,uint8_t pin,uint8_t status)
{
	q_device_ctrl(reset_io_dev,GPIO_OUTPUT_LOW,0);
}

void bc_nfc_reset_output_high(uint8_t port,uint8_t pin,uint8_t status)
{
	q_device_ctrl(reset_io_dev,GPIO_OUTPUT_HIGH,0);
}

void bc_nfc_reset_output_toggle(uint8_t port,uint8_t pin)
{
	q_device_ctrl(reset_io_dev,GPIO_OUTPUT_TOGGLE,0);
}


void bc_nfc_read_spi_flash_device_find(void)
{
	spi_flash_dev = q_device_find("spi_2");
	q_device_assert(spi_flash_dev);
//	bc_spi_flash_device_open();
  
  if(int_io_dev == NULL)
  {
    int_io_dev = q_device_find("st25_int");
    q_device_assert(int_io_dev); 
  }
  
  if(reset_io_dev == NULL)
  {
    reset_io_dev = q_device_find("st25_reset");
    q_device_assert(reset_io_dev);     
  }
}

































