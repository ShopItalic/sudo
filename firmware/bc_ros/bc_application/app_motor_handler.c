#include "app_motor_handler.h"

#include "bc_event.h"
#include "bc_timer.h"
#include "bc_logger.h"

#include "bc_ldo_switch.h"
#include "bc_delay.h"
#include "bc_piezoelectric_motor.h"

enum app_motor_status
{
	MOTOR_IDIE = 0,
	MOTOR_WORK,
};


enum app_motor_timer_type
{
	MOTOR_TIMEOUT_TIMER = 0,
	MOTOR_START_TIMER,
	MOTOR_TEST_TIMER,
	MOTOR_TIMER_TYPE_NUM
};


enum app_motor_task_event
{
	MOTOR_TASK_TYPE_START = 0,
	MOTOR_TASK_TYPE_NUM
};

static enum app_motor_status motor_status = MOTOR_IDIE;


static void app_motor_idie_timeout_timer_callback(void * pvParameter);
static void app_motor_statrt_timer_callback(void * pvParameter);
static void app_motor_test_timer_callback(void * pvParameter);


static bc_timer_struct  timer_struct[MOTOR_TIMER_TYPE_NUM] = {
	{
		.timer_name = "motor idie timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 3500,
		.lock = false,
		.timer_callback_function = app_motor_idie_timeout_timer_callback,
	},
	{
		.timer_name = "motor start timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 200,
		.lock = false,
		.timer_callback_function = app_motor_statrt_timer_callback,
	},
	{
		.timer_name = "motor test timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 1000*5,
		.lock = false,
		.timer_callback_function = app_motor_test_timer_callback,
	}
};															   

static void app_motor_handler_thread(void *thread_handler);

static bc_event_struct event_struct[MOTOR_TASK_TYPE_NUM] = {	
	{
		.event_name = "motor start task event",
		.event_callback_function = app_motor_handler_thread,
	},
};

static void app_motor_idie_timeout_timer_callback(void * pvParameter)
{
	motor_status = MOTOR_IDIE;
	bc_ldo_motor_power_off();
	BC_LOG_INFO("motor status %d  \r\n",motor_status);
	bc_timer_stop(&timer_struct[MOTOR_TIMEOUT_TIMER]);
}


static void app_motor_statrt_timer_callback(void * pvParameter)
{
	
	bc_timer_stop(&timer_struct[MOTOR_START_TIMER]);	
	
}

static void app_motor_handler_thread(void *thread_handler)
{
		BC_LOG_INFO("motor status %d  \r\n",motor_status);
	if(motor_status == MOTOR_IDIE)
	{
		bc_ldo_motor_power_on();
		bc_delay_ms(50);
		bc_piezoelectric_motor_init();
		bc_delay_ms(50);
		BC_LOG_INFO("motor init \r\n");
	}
	else
	{
		return;
	}
//    bc_ldo_motor_power_on();
		bc_delay_ms(50);
	motor_status = MOTOR_WORK;
	bc_piezoelectric_motor_play_tdk();
		
	bc_timer_start(&timer_struct[MOTOR_TIMEOUT_TIMER]);
}

static void app_motor_test_timer_callback(void * pvParameter)
{
	app_motor_play(NULL);
}

void app_motor_play(uint8_t* play_data)
{
//	bc_timer_start(&timer_struct[MOTOR_START_TIMER]);	
	bc_event_set(&event_struct[MOTOR_TASK_TYPE_START]);
}

void app_motor_play_test(void)
{
	bc_timer_start(&timer_struct[MOTOR_TEST_TIMER]);
}


/*******************************************************************************
 * Function Name     : app_ble_time_create
 * Description       : ble相关定时器创建
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
void app_motor_time_create(void)
{
	for(uint8_t i = 0;i < MOTOR_TIMER_TYPE_NUM; i++)
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
	
	for(uint8_t i = 0; i < MOTOR_TASK_TYPE_NUM; i++)
	{

		if(!bc_event_create(&event_struct[i]))                         //创建事件
		{
			BC_LOG_WARN("create %s fial!! \r\n",event_struct[i].event_name);
		}
		else
		{
			BC_LOG_INFO("create %s success!! \r\n",event_struct[i].event_name);
		}		
  }
	
}





