#include "bc_led.h"

#include "q_device.h"
#include "bc_logger.h"
#include "bc_delay.h"
#include "bc_rtos.h"

#if (HARDWARE_153_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1  || HARDWARE_1231_ENABLED == 1)	

#include "bc_ic_led.h"
	
#endif	

#include "string.h"

static q_device_t *led_red_dev;
static q_device_t *led_blue_dev;




static uint8_t red_flash_count = 0;
static bool blue_cycle_flag = false;
static uint8_t blue_flash_count = 0;
static bool red_cycle_flag = false;

static uint8_t red_count = 0;
static uint8_t blue_count = 0;

enum led_timer_event
{
	LED_BLUE_CYCLE_TIMER_EVENT = 0,
	LED_BLUE_FLASH_TIMER_EVENT,
	LED_RED_CYCLE_TIMER_EVENT,
	LED_RED_FLASH_TIMER_EVENT,
	LED_TIMER_NUM
};

static void blue_cycle_timer_callback(void * pvParameter);
static void blue_flash_timer_callback(void * pvParameter);
static void red_cycle_timer_callback(void * pvParameter);
static void red_flash_timer_callback(void * pvParameter);

static bc_rtos_timer_struct  timer_struct[LED_TIMER_NUM] = {
	{
		.timer_name = "blue cycle timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 1000,
		.lock = false,
		.timer_callback_function = blue_cycle_timer_callback,
	},
	{
		.timer_name = "blue flash timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 1000*60*5,
		.lock = false,
		.timer_callback_function = blue_flash_timer_callback,
	},
	{
		.timer_name = "red cycle timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 1000*60*5,
		.lock = false,
		.timer_callback_function = red_cycle_timer_callback,
	},
	{
		.timer_name = "red flash timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 1000*60*5,
		.lock = false,
		.timer_callback_function = red_flash_timer_callback,
	},
};

static bool temp_blue_led_flag = false;
static void blue_cycle_timer_callback(void * pvParameter)
{
	if(temp_blue_led_flag)
	{		
		bc_led_blue_toggle();
	}
	else 
	{
		bc_led_blue_on();
		if(timer_struct[LED_BLUE_FLASH_TIMER_EVENT].xTimerPeriodInTicks != 0)
		{
			bc_rtos_timer_start(timer_struct[LED_BLUE_FLASH_TIMER_EVENT].timer_handler,100);
		}
	}
}

static bool temp_red_led_flag = false;
static void blue_flash_timer_callback(void * pvParameter)
{
	
	if(blue_flash_count != 0)
	{
		blue_count++;
		if(blue_count >= (blue_flash_count * 2) - 1)
		{
			if(!blue_cycle_flag)
			{
				bc_rtos_timer_stop(timer_struct[LED_BLUE_CYCLE_TIMER_EVENT].timer_handler,100);
				
			}
			blue_count = 0;
			 bc_led_blue_off();
			bc_rtos_timer_stop(timer_struct[LED_BLUE_FLASH_TIMER_EVENT].timer_handler,100);
			
		}
		else
		{
			bc_led_blue_toggle();
		}
		
	}
	else
	{
		bc_led_blue_off();
		bc_rtos_timer_stop(timer_struct[LED_BLUE_FLASH_TIMER_EVENT].timer_handler,100);
	}
}

static void red_cycle_timer_callback(void * pvParameter)
{
	if(temp_red_led_flag)
	{
		bc_led_red_toggle();
	}
	else
	{
		bc_led_red_on();
		if(timer_struct[LED_RED_FLASH_TIMER_EVENT].xTimerPeriodInTicks)
		{
			bc_rtos_timer_start(timer_struct[LED_RED_FLASH_TIMER_EVENT].timer_handler,100);
		}
	}
}

static void red_flash_timer_callback(void * pvParameter)
{
	if(red_flash_count != 0)
	{
		red_count++;
		if(red_count >= (red_flash_count*2) - 1)
		{
			if(!red_cycle_flag)
			{
				bc_rtos_timer_stop(timer_struct[LED_RED_CYCLE_TIMER_EVENT].timer_handler,100);
			}
			red_count = 0;
			 bc_led_red_off();
			bc_rtos_timer_stop(timer_struct[LED_RED_FLASH_TIMER_EVENT].timer_handler,100);
		}
		else
		{
			bc_led_red_toggle();
		}
		
	}
	else
	{
		bc_led_red_off();
		bc_rtos_timer_stop(timer_struct[LED_RED_FLASH_TIMER_EVENT].timer_handler,100);
	}
}

static void bule_flash_start(uint8_t cycle_time,uint8_t falsh_time)
{
	timer_struct[LED_BLUE_CYCLE_TIMER_EVENT].xTimerPeriodInTicks = 1000 *cycle_time;
	timer_struct[LED_BLUE_FLASH_TIMER_EVENT].xTimerPeriodInTicks = 100 * falsh_time;
	
	bc_rtos_timer_stop(timer_struct[LED_BLUE_CYCLE_TIMER_EVENT].timer_handler,50);
  bc_rtos_timer_change_period(timer_struct[LED_BLUE_CYCLE_TIMER_EVENT].timer_handler,timer_struct[LED_BLUE_CYCLE_TIMER_EVENT].xTimerPeriodInTicks,50);
  bc_rtos_timer_change_period(timer_struct[LED_BLUE_FLASH_TIMER_EVENT].timer_handler,timer_struct[LED_BLUE_FLASH_TIMER_EVENT].xTimerPeriodInTicks,50);
	bc_rtos_timer_start(timer_struct[LED_BLUE_CYCLE_TIMER_EVENT].timer_handler,50);
	bc_led_blue_on();
}


static void bule_flash_stop(void)
{
	bc_rtos_timer_stop(timer_struct[LED_BLUE_CYCLE_TIMER_EVENT].timer_handler,50);
	bc_led_blue_off();
}

static void red_flash_start(uint8_t cycle_time,uint8_t falsh_time)
{
	timer_struct[LED_RED_CYCLE_TIMER_EVENT].xTimerPeriodInTicks = 1000 *cycle_time;
	timer_struct[LED_RED_FLASH_TIMER_EVENT].xTimerPeriodInTicks = 100 * falsh_time;
	
  
  bc_rtos_timer_stop(timer_struct[LED_RED_CYCLE_TIMER_EVENT].timer_handler,50);
  bc_rtos_timer_change_period(timer_struct[LED_RED_CYCLE_TIMER_EVENT].timer_handler,timer_struct[LED_RED_CYCLE_TIMER_EVENT].xTimerPeriodInTicks,50);
  bc_rtos_timer_change_period(timer_struct[LED_RED_FLASH_TIMER_EVENT].timer_handler,timer_struct[LED_RED_FLASH_TIMER_EVENT].xTimerPeriodInTicks,50);
	bc_rtos_timer_start(timer_struct[LED_RED_CYCLE_TIMER_EVENT].timer_handler,50);
	
}


static void red_flash_stop(void)
{
	bc_rtos_timer_stop(timer_struct[LED_RED_CYCLE_TIMER_EVENT].timer_handler,50);
	bc_led_red_off();
}

void bc_led_test_on(void)
{
	q_device_open(led_red_dev);
	q_device_ctrl(led_red_dev,GPIO_OUTPUT_HIGH,0);	
	
	q_device_open(led_blue_dev);	
	q_device_ctrl(led_blue_dev,GPIO_OUTPUT_HIGH,0);		
}

void bc_led_test_off(void)
{
	q_device_ctrl(led_red_dev,GPIO_OUTPUT_LOW,0);	
	q_device_close(led_red_dev);	
	
	q_device_ctrl(led_blue_dev,GPIO_OUTPUT_LOW,0);	
	q_device_close(led_blue_dev);	
}

void bc_led_blue_on(void)
{
	q_device_open(led_blue_dev);	
	q_device_ctrl(led_blue_dev,GPIO_OUTPUT_HIGH,0);	
}

void bc_led_blue_off(void)
{
	q_device_ctrl(led_blue_dev,GPIO_OUTPUT_LOW,0);	
	q_device_close(led_blue_dev);	
}


void bc_led_blue_toggle(void)
{
	q_device_ctrl(led_blue_dev,GPIO_OUTPUT_TOGGLE,0);		
}

void bc_led_red_on(void)
{
//#if defined(HANDWARE_1_5_3)


//	
//#elif (defined(HANDWARE_4_1_1) || defined(HANDWARE_4_1_2) || defined(RONG_WEI_Z2X) || defined(HANDWARE_4_4_1))
	q_device_open(led_red_dev);
	q_device_ctrl(led_red_dev,GPIO_OUTPUT_HIGH,0);	
//#elif defined(HANDWARE_4_0_2) 
//    bc_led_blue_on();	
//#endif			
	
}

void bc_led_red_off(void)
{

//#if defined(HANDWARE_1_5_3)


//	
//#elif (defined(HANDWARE_4_1_1) || defined(HANDWARE_4_1_2) || defined(RONG_WEI_Z2X) || defined(HANDWARE_4_4_1))
	q_device_ctrl(led_red_dev,GPIO_OUTPUT_LOW,0);	
	q_device_close(led_red_dev);
//#elif defined(HANDWARE_4_0_2) 
//    bc_led_blue_off();	

//#endif		
			
}

void bc_led_red_toggle(void)
{
	q_device_ctrl(led_red_dev,GPIO_OUTPUT_TOGGLE,0);	
	
//#if defined(HANDWARE_1_5_3)


//	
//#elif (defined(HANDWARE_4_1_1) || defined(HANDWARE_4_1_2) || defined(RONG_WEI_Z2X) || defined(HANDWARE_4_4_1))
//	q_device_ctrl(led_red_dev,GPIO_OUTPUT_HIGH,0);	
//	q_device_close(led_red_dev);
//#elif defined(HANDWARE_4_0_2) 
//    bc_led_blue_toggle();

//#endif		
			
}

void bc_led_all_on(void)
{
	q_device_open(led_red_dev);
	q_device_open(led_blue_dev);
}

void bc_led_all_off(void)
{
	q_device_close(led_red_dev);
	q_device_close(led_blue_dev);
}
static bool low_power_flash = false;
void bc_led_low_power_flash_start(void)
{
	if(low_power_flash)
	{
		return;
	}
	low_power_flash = true;
	if(red_cycle_flag)
	{
		return;
	}
	red_cycle_flag = true;
	red_flash_count = 1;
	
	red_flash_start(10,2);
}

void bc_led_low_power_flash_stop(void)
{
	if(!low_power_flash)
	{
		return;
	}
	low_power_flash = false;
	if(!red_cycle_flag)
	{
		return;
	}
	red_cycle_flag = false;
	red_flash_count = 0;
    red_flash_stop();
}

void bc_led_hardware_check_error_hint_flash_start(void)
{
	red_cycle_flag = true;
	red_flash_count = 2;
	red_flash_start(10,6);
}

void bc_led_hardware_check_error_hint_flash_stop(void)
{
	red_cycle_flag = false;
	red_flash_count = 0;
	red_flash_stop();
}

static bool led_charge_flash = false;
void bc_led_charge_flash_start(void)
{
	
//#if (HARDWARE_411_ENABLED == 1 || defined(HANDWARE_4_4_1))	

//	if(led_charge_flash)
//	{
//		return;
//	}
//	led_charge_flash = true;
//	if(red_cycle_flag)
//	{
//		return;
//	}
//	red_cycle_flag = true;
//	temp_red_led_flag = true;
//	red_flash_count = 1;
////	red_flash_start(3,2);
//	red_flash_start(1,2);

//#elif (HARDWARE_412_ENABLED == 1 )	
#if (HARDWARE_412_ENABLED == 1 || HARDWARE_411_ENABLED == 1 || defined(HANDWARE_4_4_1) || HARDWARE_402_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || defined(HANDWARE_4_5_1) || HARDWARE_156_ENABLED == 1 || \
  HARDWARE_1181_ENABLED == 1)		
	
	if(blue_cycle_flag)
	{
		return;
	}
	blue_cycle_flag = true;
	temp_blue_led_flag = true;
	blue_flash_count = 1;
//	bule_flash_start(3,2);
	bule_flash_start(1,2);
	
#endif	
	
}

void bc_led_charge_flash_stop(void)
{
	
//#if (HARDWARE_411_ENABLED == 1 || defined(HANDWARE_4_4_1))	

//	if(!led_charge_flash)
//	{
//		return;
//	}
//	led_charge_flash = false;
//	if(!red_cycle_flag)
//	{
//		return;
//	}
//	red_cycle_flag = false;
//	temp_red_led_flag = false;
//	red_flash_count = 0;
//	red_flash_stop();

//#elif (HARDWARE_412_ENABLED == 1 )	

#if (HARDWARE_412_ENABLED == 1 || HARDWARE_411_ENABLED == 1 || defined(HANDWARE_4_4_1) || HARDWARE_402_ENABLED == 1 || HARDWARE_413_ENABLED == 1  || defined(HANDWARE_4_5_1) || HARDWARE_156_ENABLED == 1 || \
  HARDWARE_1181_ENABLED == 1)		
	
	
	if(!blue_cycle_flag)
	{
		return;
	}
	blue_cycle_flag = false;
	temp_blue_led_flag = false;
	blue_flash_count = 0;
	bule_flash_stop();
	
#endif		
	
}

void bc_led_charge_over_start(void)
{
	bc_led_blue_on();
}

void bc_led_charge_over_stop(void)
{
	bc_led_blue_off();
}
void bc_led_ble_connect_hint(void)
{
	
#if (HARDWARE_411_ENABLED == 1 || defined(HANDWARE_4_4_1) || HARDWARE_402_ENABLED == 1 || HARDWARE_413_ENABLED == 1  || defined(HANDWARE_4_5_1) || HARDWARE_156_ENABLED == 1 || \
    HARDWARE_1181_ENABLED == 1)	

	blue_cycle_flag = false;
	temp_blue_led_flag = false;
	blue_flash_count = 1;
	bule_flash_start(1,10);
//	red_flash_count = 1;
//	red_flash_start(1,10);

#elif (HARDWARE_412_ENABLED == 1 )	
	
	
	
	red_cycle_flag = false;
	temp_red_led_flag = false;
	blue_flash_count = 1;
	bule_flash_start(1,10);
	
#elif (HARDWARE_153_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1  || HARDWARE_1231_ENABLED == 1)	
	
#endif		
}

void bc_led_ble_disconnect_hint(void)
{

#if (HARDWARE_411_ENABLED == 1 || defined(HANDWARE_4_4_1) || HARDWARE_402_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || defined(HANDWARE_4_5_1)  || HARDWARE_156_ENABLED == 1 || \
     HARDWARE_1181_ENABLED == 1)	

	blue_cycle_flag = false;
	temp_blue_led_flag = false;
	blue_flash_count = 3;
	bule_flash_start(1,6);
	
//	red_flash_count = 3;
//	red_flash_start(1,6);

#elif (HARDWARE_412_ENABLED == 1 )	
	
	
	red_cycle_flag = false;
	temp_red_led_flag = false;
	blue_flash_count = 3;
	bule_flash_start(1,6);	
	
#elif (HARDWARE_153_ENABLED == 1 || HARDWARE_1121_ENABLED == 1|| HARDWARE_158_ENABLED == 1  || HARDWARE_1231_ENABLED == 1)	
	
#endif	

}

void bc_led_device_find(void)
{
	led_red_dev = q_device_find("led_red");
	q_device_assert(led_red_dev);
	
	led_blue_dev = q_device_find("led_blue");
	q_device_assert(led_blue_dev);
	
  for(uint8_t i = 0;i < LED_TIMER_NUM; i++)
	{
		timer_struct[i].timer_handler = bc_rtos_timer_create(timer_struct[i].timer_name,
														  timer_struct[i].xTimerPeriodInTicks,
														  timer_struct[i].uxAutoReload, 
														   (void *)timer_struct[i].timer_id,
															timer_struct[i].timer_callback_function);
		if(timer_struct[i].timer_handler != NULL)
		{
			BC_LOG_INFO("create %s succeed\r\n",timer_struct[i].timer_name);
		}
		else
		{
			BC_LOG_ERROR("create %s fail\r\n",timer_struct[i].timer_name);
		}		
	}
}












