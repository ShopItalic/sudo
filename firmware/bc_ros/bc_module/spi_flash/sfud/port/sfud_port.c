/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2016-04-23
 */

#include <sfud.h>
#include <stdarg.h>
#include <string.h>

#include "bc_spi_flash_port.h"
#include "bc_delay.h"
#include "bc_sem.h"

static char log_buf[256] = {0};



void sfud_log_debug(const char *file, const long line, const char *format, ...);



static void spi_lock(const sfud_spi *spi) {
//    __disable_irq();
  bc_rtos_sem_take(BC_SFUD_SEM);
}

static void spi_unlock(const sfud_spi *spi) {
//    __enable_irq();
  bc_rtos_sem_give(BC_SFUD_SEM);
}

/**
 * SPI write data then read data
 */
static sfud_err spi_write_read(const sfud_spi *spi, uint8_t *write_buf, size_t write_size, uint8_t *read_buf,
        size_t read_size) {
    sfud_err result = SFUD_SUCCESS;
  


    if (write_size) {
        SFUD_ASSERT(write_buf);
    }
    if (read_size) {
        SFUD_ASSERT(read_buf);
    }

#if defined(SUDO_VOICE_ONLY)
     bc_spi_flash_cs_low();	

	if(write_size > 0)
    {
//		memcpy(send_data,write_buf,write_size);
		if(!bc_spi_flash_write_and_read(write_buf,write_size,read_buf,0))
		{
			result = SFUD_ERR_WRITE;
			goto spi_write_read_cleanup;
		}

	}
	if(read_size > 0)	
	{
//		memset(send_data,0,write_size);
		if(!bc_spi_flash_write_and_read(write_buf,0,read_buf,read_size))
		{
			result = SFUD_ERR_READ;
		}
	}

spi_write_read_cleanup:
	/* A failed command ends the transaction without issuing a read. */
    bc_spi_flash_cs_high();
#else
     bc_spi_flash_cs_low();	

	if(write_size > 0)
    {
//		memcpy(send_data,write_buf,write_size);
		if(!bc_spi_flash_write_and_read(write_buf,write_size,read_buf,0))
		{
			result = SFUD_ERR_WRITE;
		}

	}
	if(read_size > 0)	
	{
//		memset(send_data,0,write_size);
		if(!bc_spi_flash_write_and_read(write_buf,0,read_buf,read_size))
		{
			result = SFUD_ERR_READ;
		}
	}
	
    bc_spi_flash_cs_high();
#endif
    return result;
}
		
static void retry_delay_100us(void) {
    bc_delay_us(100);
}

#ifdef SFUD_USING_QSPI
/**
 * read flash data by QSPI
 */
static sfud_err qspi_read(const struct __sfud_spi *spi, uint32_t addr, sfud_qspi_read_cmd_format *qspi_read_cmd_format,
        uint8_t *read_buf, size_t read_size) {
    sfud_err result = SFUD_SUCCESS;

    /**
     * add your qspi read flash data code
     */

    return result;
}
#endif /* SFUD_USING_QSPI */

typedef struct {
    void *spix;
    void *cs_gpiox;
    uint16_t cs_gpio_pin;
} spi_user_data, *spi_user_data_t;
static spi_user_data spi1 = { .spix = NULL, .cs_gpiox = NULL, .cs_gpio_pin = 2 };
sfud_err sfud_spi_port_init(sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;
	
	    switch (flash->index) {
			case SFUD_GD25Q32E_DEVICE_INDEX: {
		;
				/* 同步 Flash 移植所需的接口及数据 */
				flash->spi.wr = spi_write_read;
				flash->spi.lock = spi_lock;
				flash->spi.unlock = spi_unlock;
				flash->spi.user_data = &spi1;
				/* about 100 microsecond delay */
				flash->retry.delay = retry_delay_100us;
				/* adout 60 seconds timeout */
				flash->retry.times = 60 * 10000;

				break;
			}
    }

    return result;
}

/**
 * This function is print debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 */
void sfud_log_debug(const char *file, const long line, const char *format, ...) {
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    printf("[SFUD](%s:%ld) ", file, line);
    /* must use vprintf to print */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\n", log_buf);
    va_end(args);
}

/**
 * This function is print routine info.
 *
 * @param format output format
 * @param ... args
 */
void sfud_log_info(const char *format, ...) {
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    printf("[SFUD]");
    /* must use vprintf to print */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\n", log_buf);
    va_end(args);
}
