#ifndef __BC_RTC_H__
#define __BC_RTC_H__



#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#if defined(ALARM_CLOCK)	


enum bc_rtc_alarm
{
	RTC_ALARM_0 = 0,
	RTC_ALARM_1,
	RTC_ALARM_2,
	RTC_ALARM_3,
	RTC_ALARM_4,
	RTC_ALARM_NUM
};

#endif //ALARM_CLOCK


void bc_rtc_time_set_uinx_time(uint32_t uinx_time,uint8_t zone);

uint32_t bg_rtc_time_get_uinx_time(void);

uint64_t bg_rtc_time_get_uinx_ms_time(void);

void bc_rtc_time_get_bj_time(struct tm *bj_time);

bool bc_rtc_format_beijing_time(char *buffer, size_t size) ;

void bc_uinx_to_bj_time_print(uint32_t uinx_time);

void bc_rtc_open(void *rtc_timer_callback);

void bc_rtc_device_find(void);

void bc_reset_rtc_open(void *rtc_timer_callback);



#if defined(ALARM_CLOCK)

void bc_alarm_rtc_open(enum bc_rtc_alarm alarm_index,struct tm *tm_config,void *rtc_timer_callback);
void bc_alarm_rtc_close(enum bc_rtc_alarm alarm_index);

#endif //ALARM_CLOCK


#endif



