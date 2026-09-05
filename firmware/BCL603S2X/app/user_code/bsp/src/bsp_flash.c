#include "q_device.h"

#include <string.h>



#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"

#define FLASH_PAGE_NUM    50


//声明FS操作结构体
typedef struct
{
    bool need_write;        //写标志
	 bool need_read;         //读标志
    bool busy;              //FS忙标志
}my_fs_info_t;

//定义FS操作结构体变量
static my_fs_info_t my_fs_info;

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);

//定义名称为my_fs的FS实例：定义回调函数和FS空间
NRF_FSTORAGE_DEF(nrf_fstorage_t my_fs) =
{
    //FS事件回调函数
    .evt_handler = fstorage_evt_handler,

	  //定义FS的空间（起始地址和结束地址），我们必须自己设置起始地址和结束地址
	  //在调用nrf_fstorage_init()函数初始化之前，我们也可以使用nrf5_flash_end_addr_get()
	  //函数获取可用于写入数据的Flash的最后一页上的结束地址
//    .start_addr = 0x75000,         //临时
//    .end_addr   = 0x75000+4096*5,  //临时
};


struct  BSP_FLASH
{
	const char   *name;
	bool          lock;
	struct flash_mutex_lock mutex_lock;
	q_device_t dev;
};


static struct BSP_FLASH bsp_list[] = 
{
	{
	  .name = "device_flash",
	  .lock = false,
	  .mutex_lock = {false,NULL,NULL},
	  .dev = {0},
	}
};




//FS事件处理函数
static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    //FS操作错误
	if (p_evt->result != NRF_SUCCESS)
    {
        LOG_INFO("--> Event received: ERROR while executing an fstorage operation.\r\n");
        return;
    }
//		LOG_DEBUG("fs resut:%02x \r\n",p_evt->result);
    //判断事件类型，执行相应处理，这里只是打印出事件信息
    switch (p_evt->id)
    {
        //FS写完成事件
			  case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
          my_fs_info.busy = false;  
        } break;
        //擦除完成事件
        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
					  my_fs_info.busy = false; 
        } break;
				case NRF_FSTORAGE_EVT_READ_RESULT:
				{
					my_fs_info.busy = false;
				}break;

        default:
            break;
    }
}


/*******************************************************************************
 * Function Name     : hal_flash_erase
 * Description       : erase the flash data
 * Input             : flash_event            event number(start addr)
 * Output            : 
 * Return            : io state
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/

static bool device_flash_erase(long offset, size_t size)
{
	
	
	ret_code_t rc;
	uint8_t temp = 0;
	uint32_t addr = my_fs.start_addr + offset;
	uint32_t const code_page_size         = NRF_FICR->CODEPAGESIZE;
	uint32_t erase_pages = size / code_page_size;
    if (size % code_page_size != 0) 
	{
        erase_pages++;
    }
	
	for (uint32_t i = 0; i < erase_pages; i++) 
	{
		rc = nrf_fstorage_erase(&my_fs,addr + i*code_page_size, 1,NULL);
		LOG_INFO("flash erase addr:0x%08x \r\n",addr + i*code_page_size);
		APP_ERROR_CHECK(rc);
		while(my_fs_info.busy)
		{
			q_device_delay_ms(5);
			if(temp > 200)
			{
				LOG_WARN("hal erase flash timeout!!!\r\n");//打印错误代码
				return false;
			}
			temp++;
		}
		temp = 0;
	}
	return true;
}


/*******************************************************************************
 * Function Name     : hal_flash_write_data
 * Description       : write the flash data
 * Input             : 
 *                     * const data           a cache pointer to write data
 *                     length                 write the length
 *                     offset                 start addr offset length
 * Output            : 
 * Return            : teur
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/

static bool device_flash_write_data(const uint8_t * data,uint16_t length,uint32_t offset)
{
	
	ret_code_t rc;
	uint8_t i =0;

  uint32_t store_data_length=length/4+1;
  if(length%4==0)
  {
    store_data_length-=1;
  }
	uint32_t const code_page_size         = NRF_FICR->CODEPAGESIZE;
	uint32_t addr = my_fs.start_addr + offset;
	rc = nrf_fstorage_write(&my_fs, addr,(uint32_t*)data,store_data_length*4, NULL);
	if(rc != NRF_SUCCESS)//FS写不成功
	{
		LOG_WARN("hal write flash error!!!! error_code:0x%x     addr:0x%08x   length:%d \r\n",rc,addr,store_data_length*4);//打印错误代码
		return false;
	}
	i=0;

	while(my_fs_info.busy)
	{
		fml_rtos_delay(5);
		if(i > 200)
		{
			LOG_WARN("hal write flash timeout!!!\r\n");//打印错误代码
			return false;
		}
		i++;
	}
  return true;	
}

/*******************************************************************************
 * Function Name     : hal_flash_read_data
 * Description       : read the flash data
 * Input             : 
 *                     *data                 a cache pointer to read data
 *                     length                read the length
 *                     offset                start addr offset length
 * Output            : *data                 flash data
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
static void device_flash_read_data(uint8_t *data,uint16_t length,uint32_t offset)
{
	uint32_t i = 0;
	uint32_t flash_read_addr = my_fs.start_addr + offset;
	for(i = 0; i < length; i++)
	{
		data[i] = *(__IO uint8_t*)(flash_read_addr + i);
	}
}


static void device_flash_stroge_init(void)
{
	
	  ret_code_t rc;
  uint32_t const bootloader_addr = NRF_UICR->NRFFW[0];
  uint32_t const page_sz         = NRF_FICR->CODEPAGESIZE;
  uint32_t const code_sz         = NRF_FICR->CODESIZE;
  nrf_fstorage_api_t * p_fs_api=&nrf_fstorage_sd;
  rc = nrf_fstorage_init(&my_fs, p_fs_api, NULL);
  APP_ERROR_CHECK(rc);
//  uint32_t end_addr=((bootloader_addr != 0xFFFFFFFF ?bootloader_addr : (code_sz * page_sz)));
  uint32_t end_addr=0x75000;
//  my_fs.end_addr=end_addr;
  my_fs.end_addr = end_addr ;//APP_FLASH_START_ADDR+page_sz*5;
  my_fs.start_addr=my_fs.end_addr-page_sz*FLASH_PAGE_NUM;
  Q_DEVICE_LOG_INFO("flash_start_addr:%x,end_addr:%x\r\n",my_fs.start_addr,my_fs.end_addr);
	
}



/*******************************************************************************
 * Function Name     : bsp_flash_init
 * Description       : flash init
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_flash_init(q_device_t*dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].lock)
			{
				return RESULT_OK;
			}
			device_flash_stroge_init();
			bsp_list[i].lock = true;
			return RESULT_OK;
		}
	}
	return RESULT_DOG_DEV_NULL_ERR;	
}



/*******************************************************************************
 * Function Name     : bsp_i2c_write
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_flash_write(q_device_t *dev, int pos,const void *buffer, int size)
{
	
	struct flash_write_package *package = (struct flash_write_package *)buffer;
	if(package == NULL)
	{
		return RESULT_POINTER_NULL_ERR;
	}
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_DEV_UNIMITIALIZED_ERR;
			}
			if(bsp_list[i].mutex_lock.mutex_lock_enable)
			{
				bsp_list[i].mutex_lock.mutex_lock_take();
				if(!device_flash_write_data(package->data,package->data_length,package->offset))
				{
					bsp_list[i].mutex_lock.mutex_lock_give();
					return RESULT_FLASH_WRITE_ERR;
				}
				bsp_list[i].mutex_lock.mutex_lock_give();
			}
			else 
			{
				if(!device_flash_write_data(package->data,package->data_length,package->offset))
				{
					return RESULT_FLASH_WRITE_ERR;
				}
			}
			
			return RESULT_OK;
		}
	}
  return RESULT_FLASH_DEV_NULL_ERR;
}




/*******************************************************************************
 * Function Name     : bsp_i2c_read
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_flash_read(q_device_t *dev, int pos, const void *  buffer, int size)
{
	
	struct flash_read_package *package = (struct flash_read_package *)buffer;
	if(package == NULL)
	{
		return RESULT_POINTER_NULL_ERR;
	}
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_DEV_UNOPENED_ERR;
			}
			if(bsp_list[i].mutex_lock.mutex_lock_enable)
			{
				bsp_list[i].mutex_lock.mutex_lock_take();
				device_flash_read_data(package->data,package->data_length,package->offset);
				bsp_list[i].mutex_lock.mutex_lock_give();
			}
			else
			{
				device_flash_read_data(package->data,package->data_length,package->offset);
			}

			return RESULT_OK;
		}
	}
  return RESULT_FLASH_DEV_NULL_ERR;
}


/*******************************************************************************
 * Function Name     : bsp_gpio_output_ctrl
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_flash_ctrl(q_device_t *dev, int cmd, void *args)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_DEV_UNOPENED_ERR;
			}
			switch(cmd)
			{
				case ERASE_FLASH:
				{
					struct flash_write_package *package = (struct flash_write_package *)args;
					if(package == NULL)
					{
						return RESULT_POINTER_NULL_ERR;
					}
					if(bsp_list[i].mutex_lock.mutex_lock_enable)
					{
						bsp_list[i].mutex_lock.mutex_lock_take();
						device_flash_erase(package->offset,package->data_length);
						bsp_list[i].mutex_lock.mutex_lock_give();
					}
					else
					{
						device_flash_erase(package->offset,package->data_length);
					}
					break;
				}
				case READ_FLASH_CONFIG:
				{
					struct flash_config *config = (struct flash_config *)args;
					if(config == NULL)
					{
						return RESULT_POINTER_NULL_ERR;
					}
					uint32_t const page_sz         = NRF_FICR->CODEPAGESIZE;
					config->en_addr = my_fs.end_addr;
					config->strat_addr = my_fs.start_addr;
					config->page_size = page_sz;
					config->page_num = FLASH_PAGE_NUM;
					break;
				}
				default :
				{
					return RESULT_INVALID_COMMAND_ERR;
				}
			}			
			return RESULT_OK;
		}
	}
	return RESULT_FLASH_DEV_NULL_ERR;
}

/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_flash_config(q_device_t *dev, void *args, void *var)
{
	struct flash_mutex_lock *cfg = (struct flash_mutex_lock *)args;
	if(cfg == NULL)
	{
		return RESULT_CONFIG_NULL_ERR;
	}	
	
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			
			bsp_list[i].mutex_lock.mutex_lock_enable = cfg->mutex_lock_enable;
			bsp_list[i].mutex_lock.mutex_lock_take = cfg->mutex_lock_take;
			bsp_list[i].mutex_lock.mutex_lock_give = cfg->mutex_lock_give;
			return RESULT_OK;
		}
	}
	return RESULT_FLASH_DEV_NULL_ERR;
}


/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static struct q_device_ops ops =
{
	.control = bsp_flash_ctrl,
	.read = bsp_flash_read,
	.write = bsp_flash_write,
	.init = bsp_flash_init,
	.config = bsp_flash_config,
};

/*******************************************************************************
 * Function Name     : bsp_gpio_output_register
 * Description       : 设备注册
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_flash_register(void)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		bsp_list[i].dev.name = bsp_list[i].name;
		bsp_list[i].dev.dops  = &ops;
		q_device_register(&bsp_list[i].dev);		
	}
}

device_initcall(bsp_flash_register);




