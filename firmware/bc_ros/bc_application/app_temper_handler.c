#include "app_temper_handler.h"


#include "bc_delay.h"


#include "bc_logger.h"
#include "bc_pmic.h"

#include "app_package.h"
#include "app_ppg_handler.h"
#include "app_ppg_data_handler.h"

#include "bc_temp.h"
#include "bc_alg.h"
#include "bc_rtos.h"

enum temper_status_e
{
	TEMPER_COLLECTIONING = 0,
	TEMPER_SUCCESS,
	TEMPER_FIAL_OVER,
	TEMPER_BUSY,
	TEMPER_CHARING,
	TEMPER_INVALID,
};

enum temper_status_e  temper_status = TEMPER_SUCCESS;

static uint8_t temper_progress_count = 1;
static uint8_t msg_id = 0;
static uint8_t subcmd = 0;
static void temper_status_set(enum temper_status_e  status);

static void app_temper_collection_progress_timer_callback(void * pvParameter);

static bc_rtos_timer_struct  timer_struct[TEMPER_TIME_TYPE_NUM] = {
	{
		.timer_name = "temper progress timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 1000,
		.lock = false,
		.timer_callback_function = app_temper_collection_progress_timer_callback,
	}
};




static void app_temper_collection_progress_timer_callback(void * pvParameter)
{
	if(temper_progress_count < 11 && temper_progress_count >= 2)
	{
		app_package_temper_up(msg_id,TEMPER_COLLECTIONING,(temper_progress_count - 1) *100,subcmd);
	}
	if(temper_progress_count >=11 )
	{
		
		uint16_t temper = bc_temp_get_temperature_value();
		if(temper == 0)
		{
			app_package_temper_up(msg_id,TEMPER_INVALID,temper,subcmd);
		}
		else
		{
			app_package_temper_up(msg_id,TEMPER_SUCCESS,temper,subcmd);
		}
		bc_rtos_timer_stop(timer_struct[TEMPER_COLLECTION_PROGRESS_TIME].timer_handler,20);
		temper_status_set(TEMPER_SUCCESS);
#if(PPG_ENABLED)  
   app_ppg_stop();
#endif
	}
	temper_progress_count++;
}


static void temper_status_set(enum temper_status_e  status)
{
	 temper_status = status;
}

enum temper_status_e temper_status_get(void)
{
	 return temper_status;
}

void app_temper_collection_progress(uint8_t frame_id,uint8_t sub)
{

	if(bc_pmic_get_charge_status() != PMIC_CHARGED_NOT)
	{
		app_package_temper_up(frame_id,TEMPER_CHARING,0,sub);
		return;
	}
	
//	if(temper_status_get() == TEMPER_COLLECTIONING || app_ppg_event_state_get() != PPG_IDIE_EVENT)\
  if(temper_status_get() == TEMPER_COLLECTIONING )
	{
		app_package_temper_up(frame_id,TEMPER_BUSY,0,sub);
		return;
	}
	temper_progress_count = 1;
	msg_id = frame_id;
	subcmd = sub;
	bc_rtos_timer_start(timer_struct[TEMPER_COLLECTION_PROGRESS_TIME].timer_handler,50);
	
	temper_status_set(TEMPER_COLLECTIONING);
//	app_ppg_check_staus();
}

void app_temper_collection_fail(void)
{
	app_package_temper_up(msg_id,TEMPER_FIAL_OVER,0,subcmd);
	bc_rtos_timer_stop(timer_struct[TEMPER_COLLECTION_PROGRESS_TIME].timer_handler,20);
	temper_status_set(TEMPER_FIAL_OVER);
}

 void app_temper_time_create(void)
{

	for(uint8_t i = 0;i < TEMPER_TIME_TYPE_NUM; i++)
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







