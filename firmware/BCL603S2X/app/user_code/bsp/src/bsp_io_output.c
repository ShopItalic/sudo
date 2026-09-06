#include "q_device.h"


#include <string.h>

#include "nrf_gpio.h"

#include "nrf_drv_gpiote.h"

enum bsp_gpio_mode    //nordic  io操作有两种模式
{
	GPIOTE = 0,
	GPIO,
	GPIO_MODE_NUM,
};



struct  BSP_GPIO_PUT
{
	const char   *name;
	bool          io_lock;
	uint32_t      bsp_io_pin;
	gpio_output_mode  pull;
	enum bsp_gpio_mode gpio_mode;
	nrf_drv_gpiote_out_config_t config;
	struct bsp_gpio_mutex_lock  gpio_mutex_lock;
	q_device_t dev;
};

static struct BSP_GPIO_PUT bsp_list[] = 
{
	{
	  .name = "sw_temp",
		.io_lock = false,
	  .bsp_io_pin = NRF_GPIO_PIN_MAP(0,25),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIOTE,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
	  .name = "ppg_led",
		.io_lock = false,
	  .bsp_io_pin = NRF_GPIO_PIN_MAP(0,12),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	
	{
	  .name = "vbat_adc_en",
		.io_lock = false,
	  .bsp_io_pin = NRF_GPIO_PIN_MAP(0,12),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIOTE,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},	

};






static void bsp_gpiote_output_low_callback(struct BSP_GPIO_PUT*dev);
static void bsp_gpiote_output_high_callback(struct BSP_GPIO_PUT *dev);
static void bsp_gpiote_output_toggle_callback(struct BSP_GPIO_PUT *dev);

static void (*bsp_gpiote_output_callback[GPIO_OUTPUT_MODE_NUM])(struct BSP_GPIO_PUT *dev) = {
																																														bsp_gpiote_output_low_callback,
																																														bsp_gpiote_output_high_callback,
																																														bsp_gpiote_output_toggle_callback,
																																													};

static void bsp_gpio_output_low_callback(struct BSP_GPIO_PUT*dev);
static void bsp_gpio_output_high_callback(struct BSP_GPIO_PUT *dev);
static void bsp_gpio_output_toggle_callback(struct BSP_GPIO_PUT *dev);

static void (*bsp_gpio_output_callback[GPIO_OUTPUT_MODE_NUM])(struct BSP_GPIO_PUT *dev) = {
																																														bsp_gpio_output_low_callback,
																																														bsp_gpio_output_high_callback,
																																														bsp_gpio_output_toggle_callback,
																																													};	

static void bsp_gpiote_output_open_callback(struct BSP_GPIO_PUT *dev);
static void bsp_gpio_output_open_callback(struct BSP_GPIO_PUT *dev);
																																													
static void (*bsp_gpio_open_callback[GPIO_MODE_NUM])(struct BSP_GPIO_PUT *dev) = {
																																									 bsp_gpiote_output_open_callback,
	                                                                                 bsp_gpio_output_open_callback,
																																								 };	

static void bsp_gpiote_output_close_callback(struct BSP_GPIO_PUT *dev);
static void bsp_gpio_output_close_callback(struct BSP_GPIO_PUT *dev);
																																													
static void (*bsp_gpio_close_callback[GPIO_MODE_NUM])(struct BSP_GPIO_PUT *dev) = {
																																									 bsp_gpiote_output_close_callback,
	                                                                                 bsp_gpio_output_close_callback,
																																								 };																																								 
																																								 
																																													
/*******************************************************************************
 * Function Name     : bsp_gpiote_output_low_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_gpiote_output_low_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_drv_gpiote_out_clear(dev->bsp_io_pin);
}	

/*******************************************************************************
 * Function Name     : bsp_gpiote_output_high_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_gpiote_output_high_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_drv_gpiote_out_set(dev->bsp_io_pin);
}

/*******************************************************************************
 * Function Name     : bsp_gpiote_output_toggle_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_gpiote_output_toggle_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_drv_gpiote_out_toggle(dev->bsp_io_pin);
}


/*******************************************************************************
 * Function Name     : bsp_gpiote_output_open_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_gpiote_output_open_callback(struct BSP_GPIO_PUT *dev)
{
	ret_code_t err_code;
	if(!nrf_drv_gpiote_is_init())
	{
		err_code = nrf_drv_gpiote_init();
		APP_ERROR_CHECK(err_code);
	}
	err_code = nrf_drv_gpiote_out_init(dev->bsp_io_pin, &dev->config);
	APP_ERROR_CHECK(err_code);
	nrf_drv_gpiote_out_task_enable(dev->bsp_io_pin);	
	
	if(dev->pull == GPIO_OUTPUT_LOW)
	{
		nrf_drv_gpiote_out_clear(dev->bsp_io_pin);
	}
	else
	{
		nrf_drv_gpiote_out_set(dev->bsp_io_pin);
	}
}

/*******************************************************************************
 * Function Name     : bsp_gpiote_output_close_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_gpiote_output_close_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_drv_gpiote_out_task_disable(dev->bsp_io_pin);
	nrfx_gpiote_out_uninit(dev->bsp_io_pin);
}


/*******************************************************************************
 * Function Name     : bsp_gpio_output_low_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_gpio_output_low_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_gpio_pin_clear(dev->bsp_io_pin);
}	

/*******************************************************************************
 * Function Name     : bsp_gpio_output_high_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_gpio_output_high_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_gpio_pin_set(dev->bsp_io_pin);
}

/*******************************************************************************
 * Function Name     : bsp_gpio_output_toggle_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_gpio_output_toggle_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_gpio_pin_toggle(dev->bsp_io_pin);
}



/*******************************************************************************
 * Function Name     : bsp_gpio_output_open_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_gpio_output_open_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_gpio_cfg_output(dev->bsp_io_pin);
	if(dev->pull == GPIO_OUTPUT_LOW)
	{
		nrf_gpio_pin_clear(dev->bsp_io_pin);

	}
	else
	{
		nrf_gpio_pin_set(dev->bsp_io_pin);

	}
}

/*******************************************************************************
 * Function Name     : bsp_gpio_output_close_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_gpio_output_close_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_gpio_cfg_default(dev->bsp_io_pin);
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
static int bsp_gpio_output_open(q_device_t*dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].io_lock)
			{
				return RESULT_OK;
			}
			if(bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_enable && bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_take != NULL)
			{
				bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_take();
				bsp_gpio_open_callback[bsp_list[i].gpio_mode](&bsp_list[i]);

			}
			else
			{
				bsp_gpio_open_callback[bsp_list[i].gpio_mode](&bsp_list[i]);
			}
			bsp_list[i].io_lock = true;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
}

/*******************************************************************************
 * Function Name     : bsp_gpio_output_close
 * Description       : gpio out put close
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_gpio_output_close(q_device_t *dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].io_lock)
			{
				return RESULT_OK;
			}
			if(bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_enable && bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_give != NULL)
			{
				bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_give();
				bsp_gpio_close_callback[bsp_list[i].gpio_mode](&bsp_list[i]);
			}
			else
			{
				bsp_gpio_close_callback[bsp_list[i].gpio_mode](&bsp_list[i]);			
			}

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
static int bsp_gpio_output_ctrl(q_device_t *dev, int cmd, void *args)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			
			if(bsp_list[i].gpio_mode == GPIOTE)
			{
				bsp_gpiote_output_callback[cmd](&bsp_list[i]);
			}
			else
			{
				bsp_gpio_output_callback[cmd](&bsp_list[i]);
			}
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;
}



/*******************************************************************************
 * Function Name     : bsp_gpio_output_read
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_gpio_output_read(q_device_t *dev, int pos,const void *buffer, int size)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].io_lock)
			{
				return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;
			}
			*(uint8_t*)buffer = nrf_gpio_pin_read(bsp_list[i].bsp_io_pin);
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
}

static int bsp_gpio_output_config(q_device_t *dev, void *args, void *var)
{
	struct bsp_gpio_mutex_lock *cfg = (struct bsp_gpio_mutex_lock *)args;
	if(cfg == NULL)
	{
		return RESULT_GPIO_CONFIG_NULL_ERR;
	}	
	
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			
			bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_enable = cfg->gpio_mutex_lock_enable;
			bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_take = cfg->gpio_mutex_lock_take;
			bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_give = cfg->gpio_mutex_lock_give;
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
	.control = bsp_gpio_output_ctrl,
	.read = bsp_gpio_output_read,
	.open = bsp_gpio_output_open,
	.close = bsp_gpio_output_close,
	.config = bsp_gpio_output_config,
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
static void bsp_gpio_output_register(void)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		bsp_list[i].dev.name = bsp_list[i].name;
		bsp_list[i].dev.dops  = &ops;
		q_device_register(&bsp_list[i].dev);		
	}
}

device_initcall(bsp_gpio_output_register);
