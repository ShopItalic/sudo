#ifndef __APP_SLEEP_HANDLER_H__
#define __APP_SLEEP_HANDLER_H__

#include <stdbool.h>
#include <stdint.h>


enum app_sleep_task_event
{
	APP_SLEEP_CHECK_TASK_EVENT = 0,
	APP_SLEEP_TASK_EVENT_NUM
};

enum app_sleep_timer_event
{
	APP_SLEEP_CHECK_TIMER_EVENT = 0,
	APP_SLEEP_TIMER_NUM
};



void app_sleep_check_start(void);

void app_sleep_check_stop(void);

bool app_sleep_get_status(void);

void app_sleep_handler_init(void);

uint32_t app_sleep_mode_get(void);

#endif







