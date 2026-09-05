#ifndef __APP_PPG_HANDLER_H__
#define __APP_PPG_HANDLER_H__


#include "stdint.h"
#include "app_ppg.h"
#include "ring_config.h"

enum app_ppg_task_event
{
	APP_PPG_DATA_HANDLER_POLL_TASK_EVENT = 0,
	APP_PPG_COLLECTION_TIMEOUT_STOP_TASK_EVENT,
};

enum app_ppg_timer_event
{
	APP_PPG_COLLECTION_PROGRESS_TIMER_EVENT = 0,
#if ( HARDWARE_413_ENABLED == 1)	
	
	APP_PPG_POLL_TIMER_EVENT,
	
#endif		
	
	APP_PPG_TIMER_NUM
};



void app_ppg_init(void);


void app_ppg_start(uint32_t collection_timer,enum app_ppg_event ppg_model);

void app_ppg_stop(void);

enum app_ppg_event app_ppg_event_state_get(void);
void app_ppg_event_state_set(enum app_ppg_event ppg_event_id);

#endif



