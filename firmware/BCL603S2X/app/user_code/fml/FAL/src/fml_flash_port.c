#include "fml_flash_port.h"

#include "fml_freertos.h"

#include "log.h"
#include "q_device.h"
#include "fal.h"

static q_device_t *flash_dev;	
static struct flash_write_package flash_write_pack;
static struct flash_read_package flash_read_pack;
static struct flash_config flash_cfg;
static struct flash_mutex_lock  mutex_lock;

static fml_rtos_sem_struct  sem_handler  = {
											.sem_name = "fml_flash_sem",
											.sem_handler = NULL,
										   };



static void fml_flash_sem_take(void)
{
	fml_rtos_semaphore_take(sem_handler.sem_handler);
}

static void fml_flash_sem_give(void)
{
	fml_rtos_semaphore_give(sem_handler.sem_handler);
}

static bool fml_flash_sem_create(void)
{

	sem_handler.sem_handler = fml_rtos_sem_create();
	if(sem_handler.sem_handler != NULL)
	{
		LOG_INFO("create %s succeed \r\n",sem_handler.sem_name);
		return true;
	}
	else
	{
		LOG_ERROR("create  %s fail",sem_handler.sem_name);
		return false;
	}	

}

static int init(void)
{
    flash_dev = q_device_find("device_flash");
	q_device_assert(flash_dev);
	if(fml_flash_sem_create())
	{
		mutex_lock.mutex_lock_enable = true;
		mutex_lock.mutex_lock_take = fml_flash_sem_take;
		mutex_lock.mutex_lock_give = fml_flash_sem_give;
		q_device_cfg(flash_dev,&mutex_lock,0);
	}
    q_device_init(flash_dev);
    return 1;
}

static int read(long offset, uint8_t *buf, size_t size)
{	
	memset((uint8_t *)&flash_read_pack,0,sizeof(flash_read_pack));
	flash_read_pack.data = buf;
	flash_read_pack.data_length = size;
	flash_read_pack.offset = offset;
	if(q_device_read(flash_dev,0,&flash_read_pack,0) != RESULT_OK)
	{
		LOG_ERROR("flash read error!!! \r\n");
	}
    return size;
}

static int write(long offset, const uint8_t *buf, size_t size)
{	
	memset((uint8_t *)&flash_write_pack,0,sizeof(flash_write_pack));
	flash_write_pack.offset = offset;
	flash_write_pack.data_length = size;
    flash_write_pack.data = buf;
	if(size == 0)
	{
		flash_write_pack.data_length = strlen((char*)buf) ;
	}
//	LOG_DEBUG("size:%d \r\n",size);
	if(q_device_write(flash_dev,0,&flash_write_pack,0) != RESULT_OK)
	{
		LOG_ERROR("flash write error!!!! \r\n");
	}
	return size;
}


static int erase(long offset, size_t size)
{
	memset((uint8_t *)&flash_write_pack,0,sizeof(flash_write_pack));
	flash_write_pack.offset = offset;
	flash_write_pack.data_length = size;
	if(q_device_ctrl(flash_dev,ERASE_FLASH,&flash_write_pack) != RESULT_OK)
	{
		LOG_ERROR("falsh erase error!!!!!!\r\n");
	}
	LOG_INFO("lllllllllll\r\n");
	return size;
}



/*
  "device_flash" : Flash 设备的名字。
  0x08000000: 对 Flash 操作的起始地址。
  1024*1024：Flash 的总大小（1MB）。
  128*1024：Flash 块/扇区大小（因为 STM32F2 各块大小不均匀，所以擦除粒度为最大块的大小：128K）。
  {init, read, write, erase} ：Flash 的操作函数。 如果没有 init 初始化过程，第一个操作函数位置可以置空。
  8 : 设置写粒度，单位 bit， 0 表示未生效（默认值为 0 ），该成员是 fal 版本大于 0.4.0 的新增成员。各个 flash 写入粒度不尽相同，可通过该成员进行设置，以下列举几种常见 Flash 写粒度：
  nor flash:  1 bit
  stm32f2/f4: 8 bit
  stm32f1:    32 bit
  stm32l4:    64 bit
 */

//1.定义 flash 设备

const struct fal_flash_dev fml_device_onchip_flash =
{
    .name       = "device_flash",
    .addr       = 0x43000,
    .len        = 50*4096,
    .blk_size   = 1*4096,
    .ops        = {init, read, write, erase},
    .write_gran = 32
};



