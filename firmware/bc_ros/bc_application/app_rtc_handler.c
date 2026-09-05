#include "app_rtc_handler.h"




#include "bc_logger.h"
#include "bc_rtc.h"
#include "bc_device_info.h"
#include "bc_delay.h"
#include "bc_rtos.h"


#include "app_g_sensor_handler.h"
#include "app_ble_handler.h"
#include "app_sleep_handler.h"

enum app_rtc_status
{
	RTC_IDIE = 0,
	RTC_ZERO_IRQ,
	RTC_RESET_IRQ,
};

static enum app_rtc_status rtc_status = RTC_IDIE;

enum app_rtc_task
{
	RTC_TASK_TYPE_IRQ = 0,
	RTC_TASK_TYPE_NUM
};

static void app_rtc_irq_handler_thread(void *thread_handler);

static bc_rtos_thread_struct app_rtc_thread[RTC_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "app rtc irq handler task",
                                                                      .thread_stack_depth   = APP_TASK_RTC_STACK_SIZE ,
                                                                      .thread_priority      = APP_TASK_RTC_PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = app_rtc_irq_handler_thread,
                                                                    },																	
                                                                  };

static 	void app_rtc_status_set(enum app_rtc_status status)
{
	rtc_status = status;
}	

static enum app_rtc_status app_rtc_status_get(void)
{
	return rtc_status;
}

static void app_reset_rtc_irq_handler(void)
{
	if(!app_ble_connect_status())
	{
//		uint32_t time = bg_rtc_time_get_uinx_time();
//		bc_device_info_app_reset_time_set(time);
//		bc_device_sleep device_sleep;
//		device_sleep.device_sleep_mode = app_sleep_mode_get();
//		device_sleep.device_sleep_mode_flag = 1;
//		bc_device_info_sleep_set(&device_sleep);
//		bc_delay_ms(20);
//		NVIC_SystemReset();
	}	
}

static void app_rtc_irq_handler_thread(void *thread_handler)
{
	while(true)
	{
		switch(app_rtc_status_get())
		{
			case RTC_IDIE:
			{
				bc_rtos_thread_suspend(app_rtc_thread[RTC_TASK_TYPE_IRQ].thread_handler);
				break;
			}
			case RTC_ZERO_IRQ:
			{
//				app_g_sensor_sport_step_count_clear();
				app_rtc_status_set(RTC_IDIE);
				break;
			}
			case RTC_RESET_IRQ:
			{
				app_reset_rtc_irq_handler();
				app_rtc_status_set(RTC_IDIE);
				break;
			}
		}
	}
}	
																  
static void app_rtc_time_isr_callback(void)
{
	app_rtc_status_set(RTC_ZERO_IRQ);
	bc_rtos_thread_resume_from_isr(app_rtc_thread[RTC_TASK_TYPE_IRQ].thread_handler);
}

static void app_rtc_uinx_time_init(void)
{
  uint32_t uinx_time = 0;
  bc_device_shut_down_time_get(&uinx_time);
  bc_rtc_time_set_uinx_time(uinx_time,0);
}


void app_rtc_ushut_down_time_record(void)
{
  uint32_t uinx_time = bg_rtc_time_get_uinx_time();
  bc_device_shut_down_time_set(uinx_time);
}

static void app_reset_rtc_time_isr_callback(void)
{
//	app_rtc_status_set(RTC_RESET_IRQ);
}



void app_rtc_handler_init(void)
{
	bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < RTC_TASK_TYPE_NUM; i++)
	{
		x_return  = bc_rtos_thread_create((TaskFunction_t )app_rtc_thread[i].thread_task_code,     	
                                     (const char*    )app_rtc_thread[i].thread_name,   	
                                     (uint16_t       )app_rtc_thread[i].thread_stack_depth, 
                                     (void*          )&app_rtc_thread[i].thread_parameters,				
                                     (UBaseType_t    )app_rtc_thread[i].thread_priority,	
                                     (TaskHandle_t*  )&app_rtc_thread[i].thread_handler); 
		if(x_return != NULL)
		{
			BC_LOG_INFO("create %s succeed \r\n",app_rtc_thread[i].thread_name);
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",app_rtc_thread[i].thread_name);
		}	
	}
	bc_reset_rtc_open(app_reset_rtc_time_isr_callback);
	bc_rtc_open(app_rtc_time_isr_callback);
  app_rtc_uinx_time_init();
  
}




