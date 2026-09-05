#include "bc_spi_flash_port.h"

#include "bc_spi_flash.h"

#include "q_device.h"


#include "string.h"

#include "bc_ldo_switch.h"

#include "bc_delay.h"

static q_device_t *spi_flash_dev = NULL;

static struct spi_package  spi_pack = {0};

void bc_spi_flash_device_open(void)
{
	bc_ldo_flash_power_on();
    //bc_delay_ms(5);
    bc_delay_ms(15);
	q_device_open(spi_flash_dev);
	//bc_delay_ms(1);
    bc_delay_ms(10);
	spi_flash_device_wakeup();
}

void bc_spi_flash_device_close(void)
{
	spi_flash_device_lowpower();
	q_device_close(spi_flash_dev);
	bc_ldo_flash_power_off();
}

void bc_spi_flash_cs_high(void)
{
	q_device_ctrl(spi_flash_dev,GPIO_OUTPUT_HIGH,0);
}

void bc_spi_flash_cs_low(void)
{
	q_device_ctrl(spi_flash_dev,GPIO_OUTPUT_LOW,0);
}


bool bc_spi_flash_write_and_read(uint8_t *write_buff,uint32_t write_length,uint8_t *read_buff,uint32_t read_length)
{
	bool ret = false;

	spi_pack.write_buff = write_buff;
	spi_pack.read_buff = read_buff;
	spi_pack.write_length = write_length;
	spi_pack.read_length = read_length;
	if(q_device_write(spi_flash_dev,0,&spi_pack,0) == RESULT_OK)
	{
		ret = true;
	}
	
	return    ret;
}





 void bc_spi_flash_device_find(void)
{
	spi_flash_dev = q_device_find("spi_2");
	q_device_assert(spi_flash_dev);
//	bc_spi_flash_device_open();
}























