/*******************************************************************************
此为rtc模块文件，为系统提供统一rtc api接口
使用q_device api接口实现

日  期：2024年1月30日
编写人：邱成凯
 *******************************************************************************/

#include "bc_rtc.h"


#include "q_device.h"

#include "string.h"




static q_device_t *rtc_dev;

static q_device_t *reset_rtc_dev;

#if defined(ALARM_CLOCK)	

typedef struct {
    q_device_t *dev;
} alarm_rtc_dev_t;

static alarm_rtc_dev_t alarm_rtc_devs[RTC_ALARM_NUM] = {0};

#endif //ALARM_CLOCK

static void bc_rtc_test(void)
{
  char beijing_time_str[20] = {0};
  bc_rtc_format_beijing_time(beijing_time_str,sizeof(beijing_time_str));
  BC_LOG_INFO("beijing_time: %s\r\n",beijing_time_str);  
}


void bc_rtc_time_set_uinx_time(uint32_t uinx_time,uint8_t zone)
{
	struct rtc_time time={0};
	time.unix_time = uinx_time;
	q_device_write(rtc_dev,0,(void*)&time,0);	
}

uint32_t bg_rtc_time_get_uinx_time(void)
{
	struct rtc_time time={0};
	q_device_read(rtc_dev,0,(void*)&time,0);
	BC_LOG_INFO("get_date_time:%d-%d-%d %d:%d:%d \r\n",
                                    time.bj_time.tm_year,
                                    time.bj_time.tm_mon,
                                    time.bj_time.tm_mday,
                                    time.bj_time.tm_hour,
                                    time.bj_time.tm_min,
                                    time.bj_time.tm_sec);
//	uint32_t unix_time = time.unix_time / 1000;
    BC_LOG_INFO("get uinx time:%d ",time.unix_time);
//  bc_rtc_test();	
	return time.unix_time;
}

uint64_t bg_rtc_time_get_uinx_ms_time(void)
{
	struct rtc_time time={0};
	q_device_read(rtc_dev,0,(void*)&time,0);

//	uint32_t unix_time = time.unix_time / 1000;
//    BC_LOG_INFO("get uinx time:%d ",time.unix_ms_time);	
	BC_LOG_HEX_P("unix_ms_time:",(uint8_t*)&time.unix_ms_time,8);
	return time.unix_ms_time;
}


void bc_rtc_time_get_bj_time(struct tm *bj_time)
{
	struct rtc_time time={0};
	q_device_read(rtc_dev,0,(void*)&time,0);
//	bj_time = &time.bj_time;
	memcpy((uint8_t*)bj_time,(uint8_t*)&time.bj_time,sizeof(struct tm));
}

bool bc_rtc_format_beijing_time(char *buffer, size_t size) 
{
    if (!buffer || size < 20) // 至少需要20个字符空间
    {  
        return false;
    }
    struct rtc_time time={0};
    q_device_read(rtc_dev,0,(void*)&time,0);
 
    sprintf(buffer, "%04d_%02d_%02d:%02d:%02d:%02d", 
            time.bj_time.tm_year, time.bj_time.tm_mon, time.bj_time.tm_mday, 
            time.bj_time.tm_hour, time.bj_time.tm_min, time.bj_time.tm_sec);
    return true;
}

void bc_uinx_to_bj_time(struct tm *ble_date_time,uint32_t uinx_time)
{
	
	uint32_t temp_time = uinx_time + (8*60*60);
	struct tm *p_real_time = localtime(&temp_time);
	ble_date_time->tm_year = p_real_time->tm_year + 1900;
	ble_date_time->tm_mon = p_real_time->tm_mon + 1;
	ble_date_time->tm_mday = p_real_time->tm_mday;
	ble_date_time->tm_hour = p_real_time->tm_hour;
	ble_date_time->tm_min = p_real_time->tm_min;
	ble_date_time->tm_sec = p_real_time->tm_sec;
	ble_date_time->tm_wday = p_real_time->tm_wday;
	ble_date_time->tm_yday = p_real_time->tm_yday + 1;

    BC_LOG_INFO("bj_time:%u-%u-%u %u:%u:%u",
                                    ble_date_time->tm_year,
                                    ble_date_time->tm_mon,
                                    ble_date_time->tm_mday,
                                    ble_date_time->tm_hour,
                                    ble_date_time->tm_min,
                                    ble_date_time->tm_sec);
}

void bc_uinx_to_bj_time_print(uint32_t uinx_time)
{
	
	uint32_t temp_time = uinx_time + (8*60*60);
	struct tm *p_real_time = localtime(&temp_time);

    BC_LOG_INFO("bj_time:%u-%u-%u %u:%u:%u",
                                    p_real_time->tm_year + 1900,
                                    p_real_time->tm_mon + 1,
                                    p_real_time->tm_mday,
                                    p_real_time->tm_hour,
                                    p_real_time->tm_min,
                                    p_real_time->tm_sec);
}

void bc_rtc_open(void *rtc_timer_callback)
{
	q_device_open(rtc_dev);
	q_device_reg_callback(rtc_dev,0,rtc_timer_callback);	//注册定时中断回调，nordic支持，phy6222不支持
}

void bc_reset_rtc_open(void *rtc_timer_callback)
{
	q_device_reg_callback(reset_rtc_dev,0,rtc_timer_callback);	//注册定时中断回调，nordic支持，phy6222不支持
}

#if defined(ALARM_CLOCK)

void bc_alarm_rtc_open(enum bc_rtc_alarm alarm_index,struct tm *tm_config,void *rtc_timer_callback)
{
	q_device_cfg(alarm_rtc_devs[alarm_index].dev, tm_config, NULL);
	q_device_reg_callback(alarm_rtc_devs[alarm_index].dev,0,rtc_timer_callback);	//注册定时中断回调，nordic支持，phy6222不支持
}

void bc_alarm_rtc_close(enum bc_rtc_alarm alarm_index)
{
	q_device_reg_callback(alarm_rtc_devs[alarm_index].dev,0,NULL);	//注册定时中断回调，nordic支持，phy6222不支持
}

#endif //ALARM_CLOCK

void bc_rtc_device_find(void)
{
	rtc_dev = q_device_find("sys rtc");
	q_device_assert(rtc_dev);	
	
	reset_rtc_dev = q_device_find("reset rtc_time");
	q_device_assert(reset_rtc_dev);
	
#if defined(ALARM_CLOCK)	
	
	alarm_rtc_devs[RTC_ALARM_0].dev = q_device_find("rtc_alarm_0");
	q_device_assert(alarm_rtc_devs[RTC_ALARM_0].dev);
	
	alarm_rtc_devs[RTC_ALARM_1].dev = q_device_find("rtc_alarm_1");
	q_device_assert(alarm_rtc_devs[RTC_ALARM_1].dev);
	
	alarm_rtc_devs[RTC_ALARM_2].dev = q_device_find("rtc_alarm_2");
	q_device_assert(alarm_rtc_devs[RTC_ALARM_2].dev);
	
	alarm_rtc_devs[RTC_ALARM_3].dev = q_device_find("rtc_alarm_3");
	q_device_assert(alarm_rtc_devs[RTC_ALARM_3].dev);
	
	alarm_rtc_devs[RTC_ALARM_4].dev = q_device_find("rtc_alarm_4");
	q_device_assert(alarm_rtc_devs[RTC_ALARM_4].dev);


#endif //ALARM_CLOCK	
	
}


