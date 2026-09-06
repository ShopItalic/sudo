#include "app_sport_adv_handler.h"

#include "app_ble_handler.h"
#include "app_g_sensor_handler.h"
#include "app_ppg_data_handler.h"
#include "app_pmic_handler.h"



#include "bc_event.h"
#include "bc_timer.h"
#include "bc_logger.h"
#include "bc_device_info.h"

#pragma pack (1)
typedef struct{
    uint16_t sport_count;
	uint8_t  hr;
	uint8_t  spo2;
	uint32_t temper;
	uint32_t percent;
	uint8_t percent_sport_status;
}ble_diy_data_pyload;
#pragma pack ()

static ble_diy_data_pyload ble_data_pyload = {0};

static void adv_update_timeout_timer_callback(void * p_context);

static bc_timer_struct  timer_struct = {
	.timer_name = "adv update timer",
	.uxAutoReload = true,
	.xTimerPeriodInTicks = 5000,
	.timer_callback_function = adv_update_timeout_timer_callback,
};

static void adv_update_event_callback(void * p_context);

static bc_event_struct event_struct={
	.event_name = "adv update event",
	.event_callback_function = adv_update_event_callback,
};


static void adv_update_timeout_timer_callback(void * p_context)
{
	bc_event_set(&event_struct);
}

static void adv_update_event_callback(void * p_context)
{
	ble_data_pyload.sport_count = app_g_sensor_sport_step_count_get();
	app_spo2_info_get(&ble_data_pyload.hr,&ble_data_pyload.spo2,&ble_data_pyload.temper);
	ble_data_pyload.percent = app_pmic_precent_get();
	ble_data_pyload.percent_sport_status = pmic_state_get();
	uint32_t temp = 0;
	bc_device_info_get_ppg_file_flag(&temp);
	ble_data_pyload.percent_sport_status = (ble_data_pyload.percent_sport_status << 4) | (temp & 0x0F);
	app_adv_data_update((uint8_t*)&ble_data_pyload,sizeof(ble_data_pyload));
}

void app_sport_adv_init(void)
{
	if(!bc_timer_create(&timer_struct))
	{
		BC_LOG_INFO("create %s fial!! \r\n",timer_struct.timer_name);
	}
	else
	{
		BC_LOG_INFO("create %s success!! \r\n",timer_struct.timer_name);
		bc_timer_start(&timer_struct);
	}
	
	
	if(!bc_event_create(&event_struct))                         //创建事件
	{
		BC_LOG_WARN("create %s fial!! \r\n",event_struct.event_name);
	}
	else
	{
		BC_LOG_INFO("create %s success!! \r\n",event_struct.event_name);
	}
}










































