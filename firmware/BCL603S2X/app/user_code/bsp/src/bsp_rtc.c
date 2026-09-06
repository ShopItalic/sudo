#include "q_device.h"

#include <string.h>


#include "nrf_drv_rtc.h"
#include "time.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static void bsp_rtc_get_date_time(struct tm *ble_date_time);

typedef void (*bsp_rtc_timer_callback)(void); 

struct rtc_timer_config
{
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
};

struct  BSP_RTC
{
	const char   *name;
	bool          lock;
  struct rtc_timer_config timer_config;
	nrf_drv_rtc_t rtc_handler;
	bsp_rtc_timer_callback timer_callback;
	q_device_t dev;
};

static struct BSP_RTC bsp_list[1] = 
{
	{
	  .name = "sys rtc",
		.lock = false,
		.timer_config = {
			                .hour = 23,
			                .min = 59,
			                .sec = 59,
		                },
		.rtc_handler = NRF_DRV_RTC_INSTANCE(2),  /**< Declaring an instance of nrf_drv_rtc for RTC2. */
		.timer_callback = NULL,
		.dev = {0},
	},
};


static uint32_t unix_time = 1672502400;  //2023.1.1.0.0.0

static uint8_t count = 0;

static void bsp_rtc_callback(nrf_drv_rtc_int_type_t int_type)
{
	if(count >= 7)
	{
		unix_time++;
		struct tm p_real_time ;
		bsp_rtc_get_date_time(&p_real_time);
		if(p_real_time.tm_hour == bsp_list[0].timer_config.hour
				&& p_real_time.tm_min == bsp_list[0].timer_config.min 
				&& p_real_time.tm_sec == bsp_list[0].timer_config.sec)//ervey day
		{
			if(bsp_list[0].timer_callback != NULL)
			{
				bsp_list[0].timer_callback();
			}
		}		
		count=0;
	}
	else
	{
		count++;
	}
}


static void bsp_rtc_init(nrf_drv_rtc_t *rtc_handler)
{
    uint32_t err_code;

    // Initialize RTC instance
    nrf_drv_rtc_config_t config = NRF_DRV_RTC_DEFAULT_CONFIG;
    config.prescaler = 4095; // 12bit
    err_code = nrf_drv_rtc_init(rtc_handler, &config, bsp_rtc_callback);
    APP_ERROR_CHECK(err_code);
    nrf_drv_rtc_tick_enable(rtc_handler, true);
    nrf_drv_rtc_enable(rtc_handler);
	   
}

static void bsp_rtc_set_date_time(struct tm *date_time)
{
	struct tm t;
	t.tm_year = date_time->tm_year - 1900;
	t.tm_mon = date_time->tm_mon- 1;
	t.tm_mday = date_time->tm_mday;
	t.tm_hour = date_time->tm_hour;
	t.tm_min = date_time->tm_min;
	t.tm_sec = date_time->tm_sec;

	unix_time = mktime(&t);// - 8*60*60;
}

static void bsp_rtc_get_date_time(struct tm *ble_date_time)
{
	
	uint32_t temp_time = unix_time + (8*60*60);
	struct tm *p_real_time = localtime(&temp_time);
	ble_date_time->tm_year = p_real_time->tm_year + 1900;
	ble_date_time->tm_mon = p_real_time->tm_mon + 1;
	ble_date_time->tm_mday = p_real_time->tm_mday;
	ble_date_time->tm_hour = p_real_time->tm_hour;
	ble_date_time->tm_min = p_real_time->tm_min;
	ble_date_time->tm_sec = p_real_time->tm_sec;

//    NRF_LOG_INFO("get_date_time:%u-%u-%u %u:%u:%u",
//                                    ble_date_time->tm_year,
//                                    ble_date_time->tm_mon,
//                                    ble_date_time->tm_mday,
//                                    ble_date_time->tm_hour,
//                                    ble_date_time->tm_min,
//                                    ble_date_time->tm_sec);
}

static time_t bsp_rtc_get_unix_time(struct tm *time)
{
	struct tm t;
	time_t timestamp = 0;
	t.tm_year = time->tm_year - 1900;
	t.tm_mon = time->tm_mon- 1;
	t.tm_mday = time->tm_mday;
	t.tm_hour = time->tm_hour;
	t.tm_min = time->tm_min;
	t.tm_sec = time->tm_sec;

	timestamp = mktime(&t) - 8*60*60;
	return timestamp;
}

static void bsp_rtc_set_unix_time_s(uint32_t s)
{
	unix_time = s;
//  unix_time_s_8x_s += (zone * 60 * 60 * 1000);

	struct tm tmp;
  bsp_rtc_get_date_time(&tmp);
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
static int bsp_rtc_open(q_device_t*dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].lock)
			{
				return RESULT_OK;
			}
			bsp_rtc_init(&bsp_list[i].rtc_handler);
			bsp_list[i].lock = true;
			return RESULT_OK;
		}
	}
	return RESULT_RTC_DEV_NULL_ERR;	
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
static int bsp_rtc_write(q_device_t *dev, int pos,const void *buffer, int size)
{
	
	struct rtc_time *time = (struct rtc_time *)buffer;
	if(time == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}

	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_RTC_DEV_NULL_ERR;
			}
			if(time->unix_time != 0)
			{
				bsp_rtc_set_unix_time_s(time->unix_time);
			}
			else
			{
				bsp_rtc_set_date_time(&time->bj_time);
			}
			return RESULT_OK;
		}
	}
  return RESULT_RTC_DEV_NULL_ERR;
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
static int bsp_rtc_read(q_device_t *dev, int pos,const void *buffer, int size)
{
	
	struct rtc_time *time = (struct rtc_time *)buffer;
	if(time == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}

	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_RTC_DEV_NULL_ERR;
			}
			bsp_rtc_get_date_time(&time->bj_time);
      time->unix_time = bsp_rtc_get_unix_time(&time->bj_time);

			return RESULT_OK;
		}
	}
  return RESULT_RTC_DEV_NULL_ERR;
}

/*******************************************************************************
 * Function Name     : bsp_rtc_timer_register_callback
 * Description       : rtc定时超时回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_rtc_timer_register_callback(q_device_t *dev,int pos, void *exit_irq_callback)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			bsp_list[i].timer_callback = (bsp_rtc_timer_callback)exit_irq_callback;
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
	.read = bsp_rtc_read,
	.write = bsp_rtc_write,
	.register_callback = bsp_rtc_timer_register_callback,
	.open = bsp_rtc_open,
	
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
static void bsp_rtc_register(void)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		bsp_list[i].dev.name = bsp_list[i].name;
		bsp_list[i].dev.dops  = &ops;
		q_device_register(&bsp_list[i].dev);		
	}
}


device_initcall(bsp_rtc_register);

