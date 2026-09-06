#include "q_device.h"


#include <string.h>

#include "bc_delay.h"
#include "nrf_gpio.h"

typedef void (*bsp_spi_gpio_output_callback)(uint32_t gpio_pin,uint32_t value); 

typedef void (*bsp_spi_gpio_output_config_callback)(uint32_t gpio_pin);

typedef void (*bsp_spi_gpio_input_config_callback)(uint32_t gpio_pin,nrf_gpio_pin_pull_t pull_config); 

typedef void (*bsp_spi_gpio_freed_callback)(uint32_t gpio_pin); 

typedef void (*bsp_spi_delay_callback)(uint32_t delay_length); 

typedef uint32_t (*bsp_spi_gpio_read_callback)(uint32_t gpio_pin); 


static void bsp_spi_delay(uint32_t delay_time);

struct simulation_spi_config
{
	uint32_t delay_length;
	uint32_t                                 spi_io_cs_pin;
	uint32_t                                 spi_io_sclk_pin;
	uint32_t                                 spi_io_mosi_pin;
	uint32_t                                 spi_io_miso_pin;
	bsp_spi_gpio_output_callback             spi_gpio_output;
	bsp_spi_gpio_output_config_callback      spi_gpio_output_config;
	bsp_spi_gpio_input_config_callback       spi_gpio_input_config;
	nrf_gpio_pin_pull_t                      spi_gpio_input_pull;
	bsp_spi_delay_callback                   spi_delay;
	bsp_spi_gpio_read_callback               spi_gpio_read;
	bsp_spi_gpio_freed_callback              spi_gpio_freed;
};



struct  BSP_SPI
{
	const char   *name;
	bool          lock;
    struct simulation_spi_config spi_config;
	struct bsp_spi_mutex_lock    mutex_lock;
	q_device_t dev;
};



static struct BSP_SPI bsp_list[] =   
{
	{
	  .name = "spi_3",  //spi0,spi1,spi2为硬件spi，spi3之后为模拟spi
		.lock = false,
	  .spi_config = {
			              .delay_length = 1,
			              .spi_io_cs_pin =   NRF_GPIO_PIN_MAP(0,10),
						  .spi_io_sclk_pin = NRF_GPIO_PIN_MAP(0,14),
			              .spi_io_mosi_pin = NRF_GPIO_PIN_MAP(0,12),
						  .spi_io_miso_pin = NRF_GPIO_PIN_MAP(1,9),		  
			              .spi_gpio_output = nrf_gpio_pin_write,
						  .spi_gpio_output_config = nrf_gpio_cfg_output,
			              .spi_gpio_input_config = nrf_gpio_cfg_input,
			              .spi_gpio_input_pull = NRF_GPIO_PIN_PULLUP,
			              .spi_delay = bsp_spi_delay,
			              .spi_gpio_read = nrf_gpio_pin_read,
			              .spi_gpio_freed = nrf_gpio_cfg_default,
					 },
		.dev = {0},
	},
	
};

static void bsp_spi_delay(uint32_t delay_time)
{
	bc_delay_ms(delay_time);
}


static void bsp_spi_init(struct BSP_SPI *spi_config)
{
	spi_config->spi_config.spi_gpio_output_config(spi_config->spi_config.spi_io_cs_pin);
	spi_config->spi_config.spi_gpio_output_config(spi_config->spi_config.spi_io_sclk_pin);
	spi_config->spi_config.spi_gpio_output_config(spi_config->spi_config.spi_io_mosi_pin);
	spi_config->spi_config.spi_gpio_input_config(spi_config->spi_config.spi_io_miso_pin,spi_config->spi_config.spi_gpio_input_pull);
	spi_config->spi_config.spi_gpio_output(spi_config->spi_config.spi_io_cs_pin,GPIO_OUTPUT_HIGH);
}

static void bsp_spi_uninit(struct BSP_SPI *spi_config)
{
	spi_config->spi_config.spi_gpio_freed(spi_config->spi_config.spi_io_cs_pin);
	spi_config->spi_config.spi_gpio_freed(spi_config->spi_config.spi_io_sclk_pin);
	spi_config->spi_config.spi_gpio_freed(spi_config->spi_config.spi_io_mosi_pin);
	spi_config->spi_config.spi_gpio_freed(spi_config->spi_config.spi_io_miso_pin);
}

static bool bsp_spi_ssid_write_data_bang(struct spi_package *package,struct BSP_SPI *spi)
{
	uint32_t value=0;
	uint32_t miso_bit = 0;
	uint8_t mask = 0;;
    uint8_t temp = 0;
	if(package->write_length == 0)
	{
		return false;
	}
	package->read_length = 0;
	for(uint32_t j = 0; j < package->write_length;j++)
	{
		for (int8_t i = 7; i >= 0; i--)
		{
			mask = (1 << i);
			temp = (package->write_buff[j] & mask);
			spi->spi_config.spi_gpio_output(spi->spi_config.spi_io_sclk_pin,GPIO_OUTPUT_LOW);
			spi->spi_config.spi_gpio_output(spi->spi_config.spi_io_mosi_pin,temp);
			spi->spi_config.spi_delay(spi->spi_config.delay_length);
			spi->spi_config.spi_gpio_output(spi->spi_config.spi_io_sclk_pin,GPIO_OUTPUT_HIGH);
			miso_bit = ((spi->spi_config.spi_gpio_read(spi->spi_config.spi_io_miso_pin) &1) << i);
			value = (value | miso_bit);
			spi->spi_config.spi_delay(spi->spi_config.delay_length);
		}
		package->read_buff[package->read_length] = value;
		value = 0;
//		Q_DEVICE_LOG_INFO("write:%02x \r\n",package->write_buff[j]);
		package->read_length++;
		
	}
	return true;
}

static bool bsp_spi_ssid_read_data_bang(struct spi_package *package,struct BSP_SPI *spi)
{
   uint32_t value=0;
   uint32_t miso_bit = 0;
   uint8_t mask = 0;
   uint8_t temp = 0;
   if(package->read_length == 0)
   {
	   return false;
   }
   memset(package->write_buff,0,package->read_length);
   for(uint32_t j = 0;j < package->read_length;j++)
   {
		for (int8_t i = 7; i >= 0; i--)
		{
			mask = (1 << i);
			temp = (package->write_buff[j] & mask);
			spi->spi_config.spi_gpio_output(spi->spi_config.spi_io_sclk_pin,GPIO_OUTPUT_LOW);
			spi->spi_config.spi_gpio_output(spi->spi_config.spi_io_mosi_pin,temp);
			spi->spi_config.spi_delay(spi->spi_config.delay_length);
			spi->spi_config.spi_gpio_output(spi->spi_config.spi_io_sclk_pin,GPIO_OUTPUT_HIGH);
			miso_bit = ((spi->spi_config.spi_gpio_read(spi->spi_config.spi_io_miso_pin) &1) << i);
			value = (value | miso_bit);
			spi->spi_config.spi_delay(spi->spi_config.delay_length);
		}
		
		package->read_buff[j] = value;
		 value = 0;
//		Q_DEVICE_LOG_INFO("read:%02x \r\n",package->read_buff[j]);
	}
	return true;
}




/*******************************************************************************
 * Function Name     : bsp_spi_open
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_spi_open(q_device_t*dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].lock)
			{
				return RESULT_OK;
			}
			bsp_spi_init(&bsp_list[i]);
			Q_DEVICE_LOG_INFO("open %s \r\n",bsp_list[i].name);		
			bsp_list[i].lock = true;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
}




/*******************************************************************************
 * Function Name     : bsp_gpio_output_open
 * Description       : gpio out put open
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_spi_close(q_device_t*dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_OK;
			}
			bsp_spi_uninit(&bsp_list[i]);
			Q_DEVICE_LOG_INFO("close %s \r\n",bsp_list[i].name);	
            bsp_list[i].lock = false;			
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
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
static int bsp_spi_write(q_device_t *dev, int pos,const void *buffer, int size)
{
	
	struct spi_package *package = (struct spi_package *)buffer;
	if(package == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
            printf("find name is \r\n");
			if(!bsp_list[i].lock)
			{
				return RESULT_DEV_UNOPENED_ERR;
			}
            printf("start find mutex lock\r\n");
			if(bsp_list[i].mutex_lock.spi_mutex_lock_enable && bsp_list[i].mutex_lock.spi_mutex_lock_take != NULL)
			{
                printf("bsp_spi_ssid_write_data_bang\r\n");
				bsp_list[i].mutex_lock.spi_mutex_lock_take();
				if(!bsp_spi_ssid_write_data_bang(package,&bsp_list[i]))
				{
					bsp_list[i].mutex_lock.spi_mutex_lock_give();
					return RESULT_SPI_SEND_ERR;
				}
				bsp_list[i].mutex_lock.spi_mutex_lock_give();
			}
			else
			{
				if(!bsp_spi_ssid_write_data_bang(package,&bsp_list[i]))
				{
                    printf("RESULT_SPI_SEND_ERR\r\n");
					return RESULT_SPI_SEND_ERR;
				}
			}
			
			return RESULT_OK;
		}
	}
  return RESULT_DEV_NULL_ERR;
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
static int bsp_spi_read(q_device_t *dev, int pos, const void *  buffer, int size)
{
	
	struct spi_package *package = (struct spi_package *)buffer;
	if(package == NULL)
	{
		return RESULT_I2C_CONFIG_NULL_ERR;
	}
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_DEV_NULL_ERR;
			}
//			Q_DEVICE_LOG_INFO(" %s i2c read： s:%x, r:%x, l:%d \r\n",bsp_list[i].name,package->slave_addr, package->reg_addr,package->read_length);
			if(bsp_list[i].mutex_lock.spi_mutex_lock_enable && bsp_list[i].mutex_lock.spi_mutex_lock_take != NULL)
			{
				bsp_list[i].mutex_lock.spi_mutex_lock_take();
				if(!bsp_spi_ssid_read_data_bang(package,&bsp_list[i]))
				{
					bsp_list[i].mutex_lock.spi_mutex_lock_give();
					return RESULT_I2C_READ_ERR;
				}
				bsp_list[i].mutex_lock.spi_mutex_lock_give();
				
			}
            else
			{
				if(!bsp_spi_ssid_read_data_bang(package,&bsp_list[i]))
				{
					return RESULT_I2C_READ_ERR;
				}
			}
		
			return RESULT_OK;
		}
	}
  return RESULT_DEV_NULL_ERR;
}




static int bsp_spi_config(q_device_t *dev, void *args, void *var)
{
	struct bsp_spi_mutex_lock *cfg = (struct bsp_spi_mutex_lock *)args;
	if(cfg == NULL)
	{
		return RESULT_GPIO_CONFIG_NULL_ERR;
	}	
	
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			
			bsp_list[i].mutex_lock.spi_mutex_lock_enable = cfg->spi_mutex_lock_enable;
			bsp_list[i].mutex_lock.spi_mutex_lock_take = cfg->spi_mutex_lock_take;
			bsp_list[i].mutex_lock.spi_mutex_lock_give = cfg->spi_mutex_lock_give;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;
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
static int bsp_spi_cs_ctrl(q_device_t *dev, int cmd, void *args)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_DEV_UNOPENED_ERR;
			}
			if(cmd == GPIO_OUTPUT_LOW)
			{
				bsp_list[i].spi_config.spi_gpio_output(bsp_list[i].spi_config.spi_io_cs_pin,0);
			}
			else if(cmd == GPIO_OUTPUT_HIGH)
			{
				bsp_list[i].spi_config.spi_gpio_output(bsp_list[i].spi_config.spi_io_cs_pin,1);
			}
		
			
			
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;
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
	.read = bsp_spi_read,
	.open = bsp_spi_open,
	.close = bsp_spi_close,
	.write = bsp_spi_write,
	.control = bsp_spi_cs_ctrl,
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
static void bsp_simulation_spi_register(void)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		bsp_list[i].dev.name = bsp_list[i].name;
		bsp_list[i].dev.dops  = &ops;
		q_device_register(&bsp_list[i].dev);		
	}
}


device_initcall(bsp_simulation_spi_register);

