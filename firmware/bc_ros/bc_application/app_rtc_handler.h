#ifndef __APP_RTC_HANDLER_H__
#define __APP_RTC_HANDLER_H__






enum app_rtc_task_event
{
	APP_RTC_TIMER_TASK_EVENT = 0,
	APP_RESET_RTC_TIMER_TASK_EVENT,
	APP_RTC_EVENT_NUM
};


void app_rtc_handler_init(void);
void app_rtc_ushut_down_time_record(void);



#endif


