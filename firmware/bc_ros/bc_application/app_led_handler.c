#include "app_led_handler.h"

#include "bc_led.h"
#include "bc_util.h"
#include "bc_delay.h"
#include "bc_timer.h"
#include "bc_logger.h"

#include <stdint.h>

#include "app_ppg_data_handler.h"
#include "app_tsdb_handler.h"


struct led_struct
{
	uint8_t led_cycle;	
	uint8_t led_modle[16];
	uint32_t led_frequency;
};

static bool led_status = false;


static struct led_struct led_info = {0};

static uint8_t led_model_buff_index = 0, led_modle_offset_index = 0,led_falsh_count = 0;


static void app_led_cycle_falsh_on_timer_callback(void * pvParameter);

static void app_led_cycle_falsh_off_timer_callback(void * pvParameter);


static bc_timer_struct  timer_struct[APP_LED_TIMER_NUM] = {	
	{
		.timer_name = "button check timer",
		.uxAutoReload = false,
		.xTimerPeriodInTicks = 1200,
		.lock = false,
		.timer_callback_function = app_led_cycle_falsh_on_timer_callback,
	},
	{
		.timer_name = "button check timer",
		.uxAutoReload = false,
		.xTimerPeriodInTicks = 1200,
		.lock = false,
		.timer_callback_function = app_led_cycle_falsh_off_timer_callback,
	},
};

static void app_led_cycle_falsh_on_timer_callback(void * pvParameter)
{
	if(led_falsh_count < led_info.led_cycle)
	{
		uint32_t temp = get_bit_lr(led_info.led_modle,led_falsh_count);
		if(temp == 1)
		{
			bc_led_blue_on();
		}
		else
		{
			bc_led_red_on();
		}
		
		bc_timer_start(&timer_struct[APP_LED_CYCLE_FLASH_OFF_TIMER_EVENT]);
	}
	else
	{
		bc_led_red_off();
		bc_led_blue_off();
		led_status = false;
	}
}

static void app_led_cycle_falsh_off_timer_callback(void * pvParameter)
{
	uint32_t temp = get_bit_lr(led_info.led_modle,led_falsh_count);
	if(temp == 1)
	{
		bc_led_blue_off();
	}
	else
	{
		bc_led_red_off();
	}
	led_falsh_count++;
	bc_timer_start(&timer_struct[APP_LED_CYCLE_FLASH_ON_TIMER_EVENT]);
}

static void app_led_modle_init(uint8_t length,uint8_t *modle,uint8_t led_cycle)
{
	for(uint8_t i = 0; i < led_cycle;i+=length)
	{
		for(uint8_t b = 0; b < length;b++)
		{  
			set_bit(led_info.led_modle,i+b,modle[b]);
			if(i+b >= led_cycle)
			{
				break;
			}
		}
	}
//	BC_LOG_HEX_P("led",led_info.led_modle,sizeof(led_info.led_modle));
	
}



void app_led_falsh_handler(uint8_t cycle,uint8_t *model,uint8_t length)
{
	if(led_status)
	{
		return;
	}
	if(cycle == 0)
	{
		led_info.led_cycle = app_ppg_hr_get();
		if(led_info.led_cycle == 0)
		{
			led_info.led_cycle = app_tsdb_hr_get();
		}
		
	}
	else
	{
		led_info.led_cycle = cycle;
	}
	
	app_led_modle_init(length,model,led_info.led_cycle);
	
	led_info.led_frequency = (60*1000) / led_info.led_cycle;
	timer_struct[APP_LED_CYCLE_FLASH_ON_TIMER_EVENT].xTimerPeriodInTicks = led_info.led_frequency;
	timer_struct[APP_LED_CYCLE_FLASH_OFF_TIMER_EVENT].xTimerPeriodInTicks = led_info.led_frequency;
	
	led_model_buff_index = 0
	;led_modle_offset_index = 0;
	led_falsh_count = 0;
	led_status = true;
	
	bc_timer_start(&timer_struct[APP_LED_CYCLE_FLASH_ON_TIMER_EVENT]);
	
}

void app_led_init(void)
{
	for(uint8_t i = 0;i < APP_LED_TIMER_NUM; i++)
	{
		if(!bc_timer_create(&timer_struct[i]))
		{
			BC_LOG_INFO("create %s fial!! \r\n",timer_struct[i].timer_name);
		}
		else
		{
			BC_LOG_INFO("create %s success!! \r\n",timer_struct[i].timer_name);
		}		
	}
}



















