#include "app_sleep_handler.h"


#include "bc_logger.h"
#include "bc_temp.h"
#include "bc_alg.h"
#include "bc_rtc.h"
#include "bc_gsensor.h"
#include "bc_delay.h"
#include "bc_gsensor.h"

#include "app_tsdb_handler.h"
#include "app_g_sensor_handler.h"
#include "app_ppg_data_handler.h"
#include "app_pmic_handler.h"

#include "bc_pmic.h"

#include "app_model_handler.h"
#include "bc_device_info.h"
#include "bc_rtos.h"

static uint8_t sleep_status = 0;

static uint32_t sleep_count = 0;

static uint32_t sleep_mode = 0;
static uint32_t sleep_mode_count = 0;

enum app_sleep_status
{
	APP_SLEEP_IDIE = 0,
	APP_SLEEP_STORAGE_RECORD,
	APP_SLEEP_STOP,
};

static enum app_sleep_status  sleep_task_status = APP_SLEEP_IDIE;

enum app_sleep_task
{
	SLEEP_TASK_TYPE_STORAGE_RECORD = 0,
	SLEEP_TASK_TYPE_NUM
};
static void app_sleep_storage_record_handler_thread(void *thread_handler);

static bc_rtos_thread_struct task_thread[SLEEP_TASK_TYPE_NUM] = {
																		{
																		  .thread_name          = "app sleep handler task",
																		  .thread_stack_depth   = APP_TASK_SLEEP_STACK_SIZE,
																		  .thread_priority      = APP_TASK_SLEEP_PRIO,
																		  .thread_parameters    = NULL,
																		  .thread_task_code     = app_sleep_storage_record_handler_thread,
																		},																	
																	  };

static void app_sleep_task_status_set(enum app_sleep_status status)
{
	sleep_task_status = status;
}

static enum app_sleep_status app_sleep_task_status_get(void)
{
	return sleep_task_status;
}

static void app_sleep_storage_record_handler(void)
{
	if(pmic_state_get() != PMIC_CHARGED_NOT  || app_model_state_get() != APP_MODEL_WORKING_STATE)
	{
		return;
	}
	store_data_unit_t store_data_unit = {0};
	bc_device_sleep *device_sleep = bc_device_info_sleep_get();
	struct tm cur_time = {0};
	//´æ¼ÇÂ¼
	store_data_unit.unix_time_s = bg_rtc_time_get_uinx_time();
	store_data_unit.accumulated_step = app_g_sensor_sport_step_count_get();
	store_data_unit.sport_mode = bc_gsensor_sport_num_get();
//	if(store_data_unit.sport_mode == 0)
//	{
//		sleep_count++;
//	}
//	else
//	{
//		sleep_count = 0;
//	}
	if(store_data_unit.sport_mode !=0 || ppg_wear_flag_get() || bc_temper_check())
	{
		bc_rtc_time_get_bj_time(&cur_time);
		store_data_unit.sleep_mode = sleepClassification(bg_rtc_time_get_uinx_time(),
														 store_data_unit.accumulated_step,
														 store_data_unit.sport_mode,
														 cur_time.tm_hour,
														 app_ppg_hr_get()
														 );
		sleep_mode = store_data_unit.sleep_mode;
		if(device_sleep->device_sleep_mode_flag == 1 && sleep_mode_count < 2)
		{
			store_data_unit.sleep_mode = device_sleep->device_sleep_mode;
			sleep_mode_count++;
		}
		if(sleep_mode_count == 3)
		{
			device_sleep->device_sleep_mode = sleep_mode;
			device_sleep->device_sleep_mode_flag = 0;
			bc_device_info_sleep_set(device_sleep);
		}
		
		bc_gsensor_sport_num_clear();
		sleep_status = store_data_unit.sleep_mode;
		BC_LOG_INFO("sleep storage \r\n");
		for(uint8_t i = 0; i < 10; i++)
		{
			if(app_tsdb_data_write(&store_data_unit))
			{
				return;
			}
			bc_delay_ms(500);
		}	
	}
	
}


static void app_sleep_storage_record_handler_thread(void *thread_handler)
{
	while(true)
	{
		switch(app_sleep_task_status_get())
		{
			case APP_SLEEP_IDIE:
			{
				bc_delay_ms(1000*60*5);
				if(app_sleep_task_status_get() != APP_SLEEP_STOP)
				{
					app_sleep_task_status_set(APP_SLEEP_STORAGE_RECORD);
				}
				break;
			}
			case APP_SLEEP_STORAGE_RECORD:
			{
				app_sleep_storage_record_handler();
				if(app_sleep_task_status_get() != APP_SLEEP_STOP)
				{
					app_sleep_task_status_set(APP_SLEEP_IDIE);
				}				
				break;
			}
			case APP_SLEEP_STOP:
			{
				bc_rtos_thread_suspend(task_thread[SLEEP_TASK_TYPE_STORAGE_RECORD].thread_handler);
				app_sleep_task_status_set(APP_SLEEP_IDIE);
				break;
			}
		}
	}
}

bool app_sleep_get_status(void)
{
	if(sleep_status == 0 || sleep_status == 1)
	{
		return false;
	}
	return true;
}

void app_sleep_check_start(void)
{
	
	bc_rtos_thread_resume(task_thread[SLEEP_TASK_TYPE_STORAGE_RECORD].thread_handler);
	BC_LOG_INFO(" %s start!! \r\n",task_thread[SLEEP_TASK_TYPE_STORAGE_RECORD].thread_name);
}

void app_sleep_check_stop(void)
{
	app_sleep_task_status_set(APP_SLEEP_STOP);
	BC_LOG_INFO(" %s stop!! \r\n",task_thread[SLEEP_TASK_TYPE_STORAGE_RECORD].thread_name);
}

uint32_t app_sleep_mode_get(void)
{
	return sleep_mode;
}

void app_sleep_handler_init(void)
{
	bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < SLEEP_TASK_TYPE_NUM; i++)
	{
		x_return  = bc_rtos_thread_create((TaskFunction_t )task_thread[i].thread_task_code,     	
                                     (const char*    )task_thread[i].thread_name,   	
                                     (uint16_t       )task_thread[i].thread_stack_depth, 
                                     (void*          )&task_thread[i].thread_parameters,				
                                     (UBaseType_t    )task_thread[i].thread_priority,	
                                     (TaskHandle_t*  )&task_thread[i].thread_handler); 
		if(x_return != NULL)
		{
			BC_LOG_INFO("create %s succeed \r\n",task_thread[i].thread_name);
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",task_thread[i].thread_name);
		}	
	}
	
	

}


