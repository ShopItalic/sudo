#include "q_device.h"


#include "q_device.h"


#include <string.h>

#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_assert.h"

//此驱动不做防抖处理，如需防抖，建议在上一层做例如 fml  app

typedef void (*bsp_gpio_exit_input_irq_callback)(uint8_t gpio_pin,uint8_t gpio_status); 

struct  BSP_GPIO_INPUT
{
	const char   *name;
	bool          io_lock;
	uint32_t      bsp_io_pin;
	nrf_gpio_pin_pull_t   pull;
	nrf_gpiote_polarity_t sense;
	q_device_t    dev;
	bsp_gpio_exit_input_irq_callback gpio_exit_input_irq_callback;
};

static struct BSP_GPIO_INPUT bsp_list[] = 
{
	{
	  .name = "acc_int_1",
		.io_lock = false,
	  .bsp_io_pin = NRF_GPIO_PIN_MAP(0,31),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
	  .name = "ppg_int",
		.io_lock = false,
	  .bsp_io_pin = NRF_GPIO_PIN_MAP(0,5),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
};


/*******************************************************************************
 * Function Name     : gpiote_event_handler
 * Description       : gpio irq  callback
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/

static void gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if(pin == bsp_list[i].bsp_io_pin && bsp_list[i].gpio_exit_input_irq_callback != NULL)
		{
			bsp_list[i].gpio_exit_input_irq_callback(pin,nrf_gpio_pin_read(pin));
		}
	}
}

/*******************************************************************************
 * Function Name     : bsp_gpio_input_init
 * Description       : gpio初始化
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
//static void bsp_gpio_input_init(void)
//{
//	if(!nrf_drv_gpiote_is_init())
//	{
//		nrf_drv_gpiote_init();
//	}
//	nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
//	
//	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
//	{
//		config.sense = bsp_list[i].sense;
//		config.pull = bsp_list[i].pull;
//		nrf_drv_gpiote_in_init(bsp_list[i].bsp_io_pin, &config, gpiote_event_handler);
//		nrf_drv_gpiote_in_event_enable(bsp_list[i].bsp_io_pin, true);	
//	}
//}

/*******************************************************************************
 * Function Name     : bsp_gpio_input_open
 * Description       : gpio初始化
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_gpio_input_open(q_device_t *dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].io_lock)
			{
				return RESULT_OK;
			}
			if(!nrf_drv_gpiote_is_init())
			{
				nrf_drv_gpiote_init();
			}
			nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
			
			config.sense = bsp_list[i].sense;
			config.pull = bsp_list[i].pull;
			nrf_drv_gpiote_in_init(bsp_list[i].bsp_io_pin, &config, gpiote_event_handler);
			nrf_drv_gpiote_in_event_enable(bsp_list[i].bsp_io_pin, true);	
			bsp_list[i].io_lock = true;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;	
}
/*******************************************************************************
 * Function Name     : bsp_gpio_input_colse
 * Description       : gpio初始化
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_gpio_input_close(q_device_t *dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].io_lock)
			{
				return RESULT_OK;
			}
			nrfx_gpiote_in_event_disable(bsp_list[i].bsp_io_pin);
			nrfx_gpiote_in_uninit(bsp_list[i].bsp_io_pin);
			bsp_list[i].io_lock = false;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;		
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
static int bsp_gpio_read(q_device_t *dev, int pos,const void *buffer, int size)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			*(uint8_t*)buffer = nrf_gpio_pin_read(bsp_list[i].bsp_io_pin);
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;	
} 

/*******************************************************************************
 * Function Name     : bsp_gpio_exit_irq_register_callback
 * Description       : 驱动注册外部中断回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_gpio_exit_irq_register_callback(q_device_t *dev,int pos, void *exit_irq_callback)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			bsp_list[i].gpio_exit_input_irq_callback = (bsp_gpio_exit_input_irq_callback)exit_irq_callback;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;		
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
	.read = bsp_gpio_read,
	.register_callback = bsp_gpio_exit_irq_register_callback,
	.open = bsp_gpio_input_open,
	.close = bsp_gpio_input_close,
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
static void bsp_gpio_input_register(void)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		bsp_list[i].dev.name = bsp_list[i].name;
		bsp_list[i].dev.dops  = &ops;
		q_device_register(&bsp_list[i].dev);		
	}
}


device_initcall(bsp_gpio_input_register);
