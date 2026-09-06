#include "lfs.h"

#include <sfud.h>
#include <stdio.h>
#include <stdlib.h>

#include "bc_spi_flash_port.h"
#include "bc_logger.h"
#include "bc_delay.h"

static const sfud_flash *flash = NULL;

static uint8_t read_buffer[300];
static uint8_t prog_buffer[300];
static uint8_t lookahead_buffer[300];

#if defined(SUDO_VOICE_ONLY)
static int lfs_sfud_flash_ready(void)
{
	if (flash == NULL || !flash->init_ok || flash->chip.capacity == 0)
	{
		return LFS_ERR_IO;
	}

	return LFS_ERR_OK;
}

static int lfs_sfud_validate_range(const struct lfs_config *c,
		lfs_block_t block, lfs_off_t off, lfs_size_t size,
		uint32_t *address)
{
	uint64_t start;
	uint64_t end;
	int err;

	if (c == NULL || c->block_size == 0 || c->block_count == 0 ||
			block >= c->block_count || off > c->block_size ||
			size > c->block_size - off)
	{
		return LFS_ERR_INVAL;
	}

	err = lfs_sfud_flash_ready();
	if (err != LFS_ERR_OK)
	{
		return err;
	}

	start = (uint64_t)c->block_size * block + off;
	end = start + size;
	if (end > flash->chip.capacity || start > UINT32_MAX ||
			end > (uint64_t)UINT32_MAX + 1U)
	{
		return LFS_ERR_INVAL;
	}

	if (address != NULL)
	{
		*address = (uint32_t)start;
	}

	return LFS_ERR_OK;
}
#endif

#if defined(SUDO_VOICE_ONLY)
uint32_t lfs_sfud_jedec_id(void)
{
	/* sfud_flash_chip stores the three cached JEDEC bytes. */
	if (flash == NULL || !flash->init_ok || flash->chip.capacity == 0)
	{
		return 0U;
	}

	return ((uint32_t)flash->chip.mf_id << 16) |
		((uint32_t)flash->chip.type_id << 8) |
		(uint32_t)flash->chip.capacity_id;
}
#endif

/**
 * lfs与底层flash读数据接口
 * @param  c
 * @param  block  块编号
 * @param  off    块内偏移地址
 * @param  buffer 用于存储读取到的数据
 * @param  size   要读取的字节数
 * @return
 */
static int lfs_deskio_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
#if defined(SUDO_VOICE_ONLY)
	uint32_t address;
	sfud_err result;
	int err;

	if (buffer == NULL)
	{
		return LFS_ERR_INVAL;
	}

	err = lfs_sfud_validate_range(c, block, off, size, &address);
	if (err != LFS_ERR_OK)
	{
		return err;
	}

	result = sfud_read(flash, address, size, (uint8_t *)buffer);
	return result == SFUD_SUCCESS ? LFS_ERR_OK : LFS_ERR_IO;
#else
	sfud_read(flash, c->block_size * block + off, size, (uint8_t *)buffer);
	return LFS_ERR_OK;
#endif
}

/**
 * lfs与底层flash写数据接口
 * @param  c
 * @param  block  块编号
 * @param  off    块内偏移地址
 * @param  buffer 待写入的数据
 * @param  size   待写入数据的大小
 * @return
 */
static int lfs_deskio_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
#if defined(SUDO_VOICE_ONLY)
	uint32_t address;
	sfud_err result;
	int err;

	if (buffer == NULL)
	{
		return LFS_ERR_INVAL;
	}

	err = lfs_sfud_validate_range(c, block, off, size, &address);
	if (err != LFS_ERR_OK)
	{
		return err;
	}

	result = sfud_write(flash, address, size, (const uint8_t *)buffer);
	return result == SFUD_SUCCESS ? LFS_ERR_OK : LFS_ERR_IO;
#else
	sfud_write(flash, c->block_size * block + off, size, (uint8_t *)buffer);
	return LFS_ERR_OK;
#endif
}

/**
 * lfs与底层flash擦除接口
 * @param  c
 * @param  block 块编号
 * @return
 */
static int lfs_deskio_erase(const struct lfs_config *c, lfs_block_t block)
{
#if defined(SUDO_VOICE_ONLY)
	uint32_t address;
	sfud_err result;
	int err;

	err = lfs_sfud_validate_range(c, block, 0, c != NULL ? c->block_size : 0,
			&address);
	if (err != LFS_ERR_OK)
	{
		return err;
	}

	result = sfud_erase(flash, address, c->block_size);
	return result == SFUD_SUCCESS ? LFS_ERR_OK : LFS_ERR_IO;
#else
    //printf("lfs_deskio_erase c->block_size: %d, block: %d, flash cap: %d******************************", c->block_size,block,flash->chip.capacity); // liukun
	sfud_erase(flash, c->block_size * block, c->block_size);
	return LFS_ERR_OK;
#endif
}

static int lfs_deskio_sync(const struct lfs_config *c)
{
	/*
	 * SFUD source evidence (sfud/src/sfud.c): sfud_read() waits for the
	 * device before transfer; page256_or_1_byte_write() and sfud_erase()
	 * wait after each program or erase command. Those APIs are synchronous,
	 * so no additional readiness API is required here.
	 */
	(void)c;
	return LFS_ERR_OK;
}


const struct lfs_config cfg =
{
	// block device operations
	.read  = lfs_deskio_read,
	.prog  = lfs_deskio_prog,
	.erase = lfs_deskio_erase,
	.sync  = lfs_deskio_sync,

	// block device configuration
	.read_size = 1,         //×îÐ¡¶ÁÈ¡µ¥Î»Îª1×Ö½Ú
	.prog_size = 1,         //×îÐ¡±à³Ìµ¥Î»Îª1×Ö½Ú
	.block_size = 4096,     //¿é´óÐ¡Îª4096

#if defined(HANDWARE_4_1_3)

	.block_count = 1024,     //¿é¸öÊýÎª4096¸ö

#elif defined(HANDWARE_1_5_8)

#define SFUD_FLASH_DEVICE_TABLE                                                \

	.block_count = 1024,     //¿é¸öÊýÎª4096¸ö
#elif defined(HANDWARE_4_5_1)

	.block_count = 8192,     //¿é¸öÊýÎª4096¸ö
	
#elif (defined(HANDWARE_1_14_1)  || defined(HANDWARE_1_17_1) || defined(HANDWARE_1_23_1))

#if 1 // modify by liukun 20260417
#if defined(HANDWARE_1_23_3)
	.block_count = 2048,     //¿é¸öÊýÎª4096¸ö	
#elif defined(HANDWARE_1_23_4)
    .block_count = 16384,     //¿é¸öÊýÎª4096¸ö	
#else
	.block_count = 4096,     //¿é¸öÊýÎª4096¸ö	
#endif
#else
	.block_count = 32768,     //¿é¸öÊýÎª4096¸ö	
#endif
#elif (defined(HANDWARE_1_19_1)  )

	.block_count = 32768,     //¿é¸öÊýÎª4096¸ö	  

#endif	



	
	.cache_size = 64,//16,
	.lookahead_size = 64,//16,   //¶ÁÐ´»º´æÎª4096×Ö½Ú
	.block_cycles = 500,

	//
	// Ê¹ÓÃ¾²Ì¬ÄÚ´æ±ØÐëÉèÖÃÕâ¼¸¸ö»º´æ
	//
	.read_buffer = read_buffer,
	.prog_buffer = prog_buffer,
	.lookahead_buffer = lookahead_buffer,
};


int lfs_sfud_init(lfs_t *lfs)
{
#if defined(SUDO_VOICE_ONLY)
	sfud_err sfud_result;
	uint64_t filesystem_size;
	int err;

	flash = NULL;
	if (lfs == NULL)
	{
		return LFS_ERR_INVAL;
	}

	/*
	 * SUDO_VOICE_ONLY callers own the flash power lease. Keep that lease
	 * held through sfud_init(), lfs_mount(), and every lfs handle
	 * operation; this port deliberately does not acquire or release power.
	 */
	sfud_result = sfud_init();
	if (sfud_result != SFUD_SUCCESS)
	{
		err = LFS_ERR_IO;
		goto lfs_sfud_init_exit;
	}

	flash = sfud_get_device_table();
	if (flash == NULL || !flash->init_ok)
	{
		err = LFS_ERR_IO;
		goto lfs_sfud_init_exit;
	}

	filesystem_size = (uint64_t)cfg.block_size * cfg.block_count;
	if (cfg.block_size == 0 || cfg.block_count == 0 ||
			filesystem_size > flash->chip.capacity)
	{
		err = LFS_ERR_INVAL;
		goto lfs_sfud_init_exit;
	}

	err = lfs_mount(lfs, &cfg);
	BC_LOG_INFO("lfs_mount :%d \r\n",err);

lfs_sfud_init_exit:
	if (err != LFS_ERR_OK)
	{
		flash = NULL;
	}
	return err;
#else
	bc_spi_flash_device_open();
	sfud_init();
	flash = sfud_get_device_table() + 0;
	
	int err = lfs_mount(lfs, &cfg);
	BC_LOG_INFO("lfs_mount :%d \r\n",err);
	if (err)
	{
		err = lfs_format(lfs, &cfg);
		BC_LOG_INFO("lfs_format :%d \r\n",err);
		err = lfs_mount(lfs, &cfg);
		BC_LOG_INFO("lfs_mount :%d \r\n",err);
	}
	else if(err != 0)
	{
		for(uint8_t i = 0; i < 5; i++)
		{
			err = lfs_format(lfs, &cfg);
			BC_LOG_INFO("lfs_format :%d \r\n",err);
			err = lfs_mount(lfs, &cfg);
			BC_LOG_INFO("lfs_mount :%d \r\n",err);
			if(err)
			{
				break;
			}
			else
			{
				bc_delay_ms(300);
			}
		}
	}
	
	
	bc_spi_flash_device_close();
	return err;
#endif
}


int lfs_sfud_format(lfs_t *lfs)
{
#if defined(SUDO_VOICE_ONLY)
	(void)lfs;
	return LFS_ERR_INVAL;
#else
	bc_spi_flash_device_open();
    int err = 0;
	err = lfs_format(lfs, &cfg);
	BC_LOG_INFO("lfs_format :%d \r\n",err);
	err = lfs_mount(lfs, &cfg);
	BC_LOG_INFO("lfs_mount :%d \r\n",err);
	
	if(err != 0)
	{
		for(uint8_t i = 0; i < 5; i++)
		{
			err = lfs_format(lfs, &cfg);
			BC_LOG_INFO("lfs_format :%d \r\n",err);
			err = lfs_mount(lfs, &cfg);
			BC_LOG_INFO("lfs_mount :%d \r\n",err);
			if(err)
			{
				break;
			}
			else
			{
				bc_delay_ms(300);
			}
		}
	}
	
	
	bc_spi_flash_device_close();
	return err;
#endif
}


