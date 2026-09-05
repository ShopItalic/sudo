#include "app_model_handler.h"

#include "bc_logger.h"
#include "bc_strategy_value.h"
#include "bc_delay.h"
#include "bc_rtos.h"


#include "app_ppg_data_handler.h"
#include "app_pmic_handler.h"
#include "app_sleep_handler.h"


static enum app_model_status model_status = APP_MODEL_HARDWARE_CHECK_STATE;




static void app_enter_silence_model_timer_callback(void * pvParameter);




static bc_rtos_timer_struct  timer_struct[APP_MODEL_TIMER_TYPE_NUM] = {	
	{
		.timer_name = "enter silence timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 1000*60*35,
		.lock = false,
		.timer_callback_function = app_enter_silence_model_timer_callback,
	}
};


static void app_model_charging_handler_callback(void);
static void app_model_working_handler_callback(void);
static void app_model_silence_handler_callback(void);
static void app_model_warehousing_handler_callback(void);
static void app_model_hardware_check_handler_callback(void);

static void (*app_model_event_callback[APP_MODEL_NUM])(void) = {
	app_model_charging_handler_callback,
	app_model_working_handler_callback,
	app_model_silence_handler_callback,
	app_model_warehousing_handler_callback,
	app_model_hardware_check_handler_callback,
};


static void app_model_timer_start(enum app_model_timer_type time_id)
{
  bc_rtos_timer_reset(timer_struct[time_id].timer_handler,20);
}

static void app_model_timer_stop(enum app_model_timer_type time_id)
{
	bc_rtos_timer_stop(timer_struct[time_id].timer_handler,20);
}

static void app_enter_silence_model_timer_callback(void * pvParameter)
{
	app_model_state_set(APP_MODEL_SILENCE_STATE);
}



/*******************************************************************************
 * Function Name     : app_model_charging_handler_callback
 * Description       : 充电模式处理
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void app_model_charging_handler_callback(void)
{
	BC_LOG_INFO("app charging model \r\n");
#if(PPG_ENABLED)     
	app_ppg_hrm_and_spo2_automatic_cycle_collection_stop();                //停止自动周期定时ppg检测
#endif  
//	app_sleep_check_stop();
	app_model_timer_stop(APP_ENTER_SILENCE_TIMER);                         //停止进入静默模式定时器
}


/*******************************************************************************
 * Function Name     : app_model_working_handler_callback
 * Description       : 工作模式处理
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void app_model_working_handler_callback(void)
{
	BC_LOG_INFO("app working model \r\n");
	app_pmic_handler_timer_start();                                       //启动定时查询电量与pmic状态
#if(PPG_ENABLED)    
	app_ppg_hrm_and_spo2_automatic_cycle_collection_start();              //启动自动周期定时ppg检测
#endif  
//	app_sleep_check_start();
	app_model_timer_start(APP_ENTER_SILENCE_TIMER);                       //启动进入静默模式定时器，等待进入静默态
}

/*******************************************************************************
 * Function Name     : app_model_silence_handler_callback
 * Description       : 静默模式处理
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void app_model_silence_handler_callback(void)
{
	BC_LOG_INFO("app silence model \r\n");
#if(PPG_ENABLED)    
	app_ppg_hrm_and_spo2_automatic_cycle_collection_stop();                //停止自动周期定时ppg检测
#endif
//	app_sleep_check_stop();
}

/*******************************************************************************
 * Function Name     : app_model_warehousing_handler_callback
 * Description       : 仓储模式处理
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void app_model_warehousing_handler_callback(void)
{
	BC_LOG_INFO("app warehousing model \r\n");
}

/*******************************************************************************
 * Function Name     : app_model_hardware_check_handler_callback
 * Description       : 硬件自检模式处理
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void app_model_hardware_check_handler_callback(void)
{
	BC_LOG_INFO("app hardware check model \r\n");
//	app_hardware_checkr_start();                                           //启动定时硬件检测
//	app_pmic_handler_timer_stop();                                         //停止定时查询电量与pmic状态
//	app_ppg_hrm_and_spo2_automatic_cycle_collection_stop();                //停止自动周期定时ppg检测
//	app_model_timer_stop(APP_ENTER_SILENCE_TIMER);                         //停止进入静默模式定时器
}



void app_enter_silence_model_timer_update(void)
{
	BC_LOG_INFO("app enter silence model timer update \r\n");
	if(app_model_state_get() != APP_MODEL_WORKING_STATE && app_model_state_get() != APP_MODEL_CHARGING_STATE)
	{
		app_model_state_set(APP_MODEL_WORKING_STATE);
		return;
	}
	app_model_timer_start(APP_ENTER_SILENCE_TIMER);
}

void app_model_state_set(enum app_model_status status)
{
	if(status >= APP_MODEL_NUM || app_model_event_callback[status] == NULL || model_status == status)
	{
		return;
	}
	model_status = status;
	app_model_event_callback[status]();
}


enum app_model_status app_model_state_get(void)
{
	return model_status;
}

void app_model_silence_timer_update(uint32_t time)
{
	timer_struct[APP_ENTER_SILENCE_TIMER].xTimerPeriodInTicks = time + (1000*60*5);
	bc_rtos_timer_change_period(timer_struct[APP_ENTER_SILENCE_TIMER].timer_handler, timer_struct[APP_ENTER_SILENCE_TIMER].xTimerPeriodInTicks, 50);
	app_model_timer_start(APP_ENTER_SILENCE_TIMER);
}


void app_model_time_create(void)	
{
	timer_struct[APP_ENTER_SILENCE_TIMER].xTimerPeriodInTicks = (1000*bc_get_business_strategy_value(BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_TIME)) + (1000*60*5);
  
	for(uint8_t i = 0;i < APP_MODEL_TIMER_TYPE_NUM; i++)
	{
		timer_struct[i].timer_handler = bc_rtos_timer_create(timer_struct[i].timer_name,
														  timer_struct[i].xTimerPeriodInTicks,
														  timer_struct[i].uxAutoReload, 
														   (void *)timer_struct[i].timer_id,
															timer_struct[i].timer_callback_function);
		if(timer_struct[i].timer_handler != NULL)
		{
			BC_LOG_INFO("create %s succeed\r\n",timer_struct[i].timer_name);
			bc_rtos_timer_change_period(timer_struct[APP_ENTER_SILENCE_TIMER].timer_handler, timer_struct[APP_ENTER_SILENCE_TIMER].xTimerPeriodInTicks, 50);
			app_model_timer_start(APP_ENTER_SILENCE_TIMER);
		}
		else
		{
			BC_LOG_ERROR("create %s fail\r\n",timer_struct[i].timer_name);
		}		
	}
}




