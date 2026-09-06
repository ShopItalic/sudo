#include "bc_wifi_port.h"

#include "q_device.h"
#include "bc_logger.h"
#include "bc_delay.h"

static q_device_t *wifi_en_dev = NULL;
static q_device_t *spi_dev = NULL;
static struct spi_package  spi_pack = {0};

void bc_wifi_device_enable(void)
{
//  bc_delay_ms(100);
//  q_device_ctrl(wifi_en_dev,GPIO_OUTPUT_LOW,0);	
//  bc_delay_ms(100);
  q_device_ctrl(wifi_en_dev,GPIO_OUTPUT_HIGH,0);	
}

void bc_wifi_device_disable(void)
{
  q_device_ctrl(wifi_en_dev,GPIO_OUTPUT_LOW,0);	
}



void bc_wifi_spi_device_open(void)
{
	q_device_open(spi_dev);
}

void bc_wifi_spi_device_close(void)
{
	q_device_close(spi_dev);
}

void bc_wifi_spi_cs_high(void)
{
	q_device_ctrl(spi_dev,GPIO_OUTPUT_HIGH,0);
}

void bc_wifi_spi_cs_low(void)
{
	q_device_ctrl(spi_dev,GPIO_OUTPUT_LOW,0);
}


bool bc_wifi_spi_write_and_read(uint8_t *write_buff,uint32_t write_length,uint8_t *read_buff,uint32_t read_length)
{
	bool ret = false;

	spi_pack.write_buff = write_buff;
	spi_pack.read_buff = read_buff;
	spi_pack.write_length = write_length;
	spi_pack.read_length = read_length;
	if(q_device_write(spi_dev,0,&spi_pack,0) == RESULT_OK)
	{
		ret = true;
    //BC_LOG_HEX_P("spi send data:",spi_pack.write_buff,spi_pack.write_length );
	}
	
	return    ret;
}



void bc_wifi_device_find(void)
{
	wifi_en_dev = q_device_find("wifi_en");
	q_device_assert(wifi_en_dev);
  q_device_open(wifi_en_dev);	
  
  spi_dev = q_device_find("spi_1");
	q_device_assert(spi_dev);
  bc_wifi_spi_device_open();
}

uint8_t write_buff[1024] = {0};
  uint8_t read_buff[1024] = {0};
static   uint8_t temp = 0;
void bc_wifi_spi_write_test(uint32_t seq,uint16_t length)
{
  *(uint32_t*)write_buff = seq;
  *(uint16_t*)&write_buff[4] = length;
  for(uint16_t i = 6; i < length+6; i++)
  {
    write_buff[i] = temp;
  };
  temp ++;
  bc_wifi_spi_cs_low();
  bc_wifi_spi_write_and_read(write_buff,length,read_buff,length+6);
  bc_wifi_spi_cs_high();
}

