#include "bc_spi_flash_port.h"

#include "bc_spi_flash.h"

#include "q_device.h"


#include "string.h"

#include "bc_ldo_switch.h"

#include "bc_delay.h"

static q_device_t *spi_flash_dev = NULL;

static struct spi_package  spi_pack = {0};

#if defined(SUDO_VOICE_ONLY)
static bool spi_flash_device_opened = false;

static bool spi_flash_write_package(uint8_t *write_buff, uint8_t write_length,
	uint8_t *read_buff, uint8_t read_length)
{
	spi_pack.write_buff = write_buff;
	spi_pack.write_length = write_length;
	spi_pack.read_buff = read_buff;
	spi_pack.read_length = read_length;
	return q_device_write(spi_flash_dev, 0, &spi_pack, 0) == RESULT_OK;
}

static bool spi_flash_transfer(uint8_t *write_buff, uint32_t write_length,
	uint8_t *read_buff, uint32_t read_length)
{
	uint32_t chunk_length;
	bool transferred = false;

	/* q_device's SPI package uses uint8_t lengths. Keep one manual CS
	 * transaction while issuing bounded EasyDMA transfers to the 16-bit
	 * SPIM peripheral. */
	while(write_length != 0U)
	{
		chunk_length = write_length > UINT8_MAX ? UINT8_MAX : write_length;
		if(!spi_flash_write_package(write_buff, (uint8_t)chunk_length, NULL, 0))
		{
			return false;
		}
		transferred = true;
		write_buff += chunk_length;
		write_length -= chunk_length;
	}

	while(read_length != 0U)
	{
		chunk_length = read_length > UINT8_MAX ? UINT8_MAX : read_length;
		if(!spi_flash_write_package(NULL, 0, read_buff, (uint8_t)chunk_length))
		{
			return false;
		}
		transferred = true;
		read_buff += chunk_length;
		read_length -= chunk_length;
	}

	if(!transferred)
	{
		return spi_flash_write_package(write_buff, 0, read_buff, 0);
	}

	return true;
}

bool bc_spi_flash_device_open_checked(void)
{
	int result;

	if(spi_flash_dev == NULL)
	{
		return false;
	}
	if(spi_flash_device_opened)
	{
		return true;
	}

	bc_ldo_flash_power_on();
	bc_delay_ms(15);
	result = q_device_open(spi_flash_dev);
	if(result != RESULT_OK)
	{
		bc_ldo_flash_power_off();
		return false;
	}

	/* The checked wakeup call uses this same open-gated port. */
	spi_flash_device_opened = true;
	bc_delay_ms(10);
	if(!spi_flash_device_wakeup_checked())
	{
		/* The wakeup helper leaves CS high. The device is not usable, so
		 * close the SPI driver and release the power lease before retry. */
		(void)q_device_close(spi_flash_dev);
		spi_flash_device_opened = false;
		bc_ldo_flash_power_off();
		return false;
	}

	return true;
}

void bc_spi_flash_device_open(void)
{
	(void)bc_spi_flash_device_open_checked();
}

void bc_spi_flash_device_close(void)
{
	if(!spi_flash_device_opened)
	{
		return;
	}
	spi_flash_device_lowpower();
	(void)q_device_close(spi_flash_dev);
	spi_flash_device_opened = false;
	bc_ldo_flash_power_off();
}

void bc_spi_flash_cs_high(void)
{
	if(spi_flash_device_opened)
	{
		(void)q_device_ctrl(spi_flash_dev, GPIO_OUTPUT_HIGH, 0);
	}
}

void bc_spi_flash_cs_low(void)
{
	if(spi_flash_device_opened)
	{
		(void)q_device_ctrl(spi_flash_dev, GPIO_OUTPUT_LOW, 0);
	}
}

bool bc_spi_flash_write_and_read(uint8_t *write_buff, uint32_t write_length,
	uint8_t *read_buff, uint32_t read_length)
{
	if(!spi_flash_device_opened || spi_flash_dev == NULL ||
		(write_length != 0U && write_buff == NULL) ||
		(read_length != 0U && read_buff == NULL))
	{
		return false;
	}

	return spi_flash_transfer(write_buff, write_length, read_buff, read_length);
}
#else

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

#endif





 void bc_spi_flash_device_find(void)
{
	spi_flash_dev = q_device_find("spi_2");
	q_device_assert(spi_flash_dev);
#if defined(SUDO_VOICE_ONLY)
	spi_flash_device_opened = false;
#endif
//	bc_spi_flash_device_open();
}























