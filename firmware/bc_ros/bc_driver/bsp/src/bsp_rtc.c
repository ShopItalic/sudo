/*******************************************************************************
此为bsp io输出文件，通过宏定义来兼容nordic、phy6222硬件平台

接口遵循q_device规则
日  期：2024年1月17日
编写人：邱成凯
 *******************************************************************************/

#include "q_device.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>

#define TB_OFFSET          8

typedef void (*bsp_rtc_timer_callback)(void); 

//struct rtc_timer_config
//{
//	uint8_t hour;
//	uint8_t min;
//	uint8_t sec;
//};


#if (HARDWARE_ARCH_TYPE_NORDIC == 1)

#include "nrf_drv_rtc.h"


#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static void bsp_rtc_get_date_time(struct tm *ble_date_time);

typedef void (*bsp_rtc_timer_callback)(void); 


struct  BSP_RTC
{
	const char   *name;
	bool          lock;
    struct tm timer_config;
	nrf_drv_rtc_t rtc_handler;
	bsp_rtc_timer_callback timer_callback;
	q_device_t dev;
};

static struct BSP_RTC bsp_list[] = 
{
	{
	  .name = "sys rtc",
		.lock = false,
		.timer_config = {
			                .tm_hour = 23,
			                .tm_min = 59,
			                .tm_sec = 59,
		                },
		.rtc_handler = NRF_DRV_RTC_INSTANCE(2),  /**< Declaring an instance of nrf_drv_rtc for RTC2. */
		.timer_callback = NULL,
		.dev = {0},
	},
	{
	  .name = "reset rtc_time",
		.lock = false,
		.timer_config = {
			                .tm_hour = 14,
			                .tm_min = 59,
			                .tm_sec = 59,
		                },
		.rtc_handler = NRF_DRV_RTC_INSTANCE(2),  /**< Declaring an instance of nrf_drv_rtc for RTC2. */
		.timer_callback = NULL,
		.dev = {0},
	},
#if defined(ALARM_CLOCK)	
	{
	  .name = "rtc_alarm_0",
		.lock = false,
		.timer_config = {
			                .tm_hour = 0,
			                .tm_min = 0,
			                .tm_sec = 0,
		                },
		.rtc_handler = NRF_DRV_RTC_INSTANCE(2),  /**< Declaring an instance of nrf_drv_rtc for RTC2. */
		.timer_callback = NULL,
		.dev = {0},
	},
	{
	  .name = "rtc_alarm_1",
		.lock = false,
		.timer_config = {
			               .tm_hour = 0,
			                .tm_min = 0,
			                .tm_sec = 0,
		                },
		.rtc_handler = NRF_DRV_RTC_INSTANCE(2),  /**< Declaring an instance of nrf_drv_rtc for RTC2. */
		.timer_callback = NULL,
		.dev = {0},
	},
	{
	  .name = "rtc_alarm_2",
		.lock = false,
		.timer_config = {
			                .tm_hour = 0,
			                .tm_min = 0,
			                .tm_sec = 0,
		                },
		.rtc_handler = NRF_DRV_RTC_INSTANCE(2),  /**< Declaring an instance of nrf_drv_rtc for RTC2. */
		.timer_callback = NULL,
		.dev = {0},
	},
	{
	  .name = "rtc_alarm_3",
		.lock = false,
		.timer_config = {
			                .tm_hour = 0,
			                .tm_min = 0,
			                .tm_sec = 0,
		                },
		.rtc_handler = NRF_DRV_RTC_INSTANCE(2),  /**< Declaring an instance of nrf_drv_rtc for RTC2. */
		.timer_callback = NULL,
		.dev = {0},
	},
	{
	  .name = "rtc_alarm_4",
		.lock = false,
		.timer_config = {
			                .tm_hour = 0,
			                .tm_min = 0,
			                .tm_sec = 0,
		                },
		.rtc_handler = NRF_DRV_RTC_INSTANCE(2),  /**< Declaring an instance of nrf_drv_rtc for RTC2. */
		.timer_callback = NULL,
		.dev = {0},
	},
#endif //ALARM_CLOCK
	
};


static uint32_t unix_time = 1735660800;  //2025-01-01 00:00:00

static uint16_t count = 0;

// add by liukun start 20260415
static uint32_t days_in_month(uint32_t year, uint32_t month) {
    // 每个月的天数
    uint32_t daysInMonths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // 是否为闰年
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        daysInMonths[1] = 29; // 2月为29天
    }
    
    // 返回指定月份的天数
    return daysInMonths[month - 1];
}
// add by liukun end 20260415

static void bsp_rtc_callback(nrf_drv_rtc_int_type_t int_type)
{
#if (defined(HANDWARE_4_5_1) || defined(HANDWARE_1_14_1x))

	//滴答中断 这边是 1/ 32768 * 33 = 1ms
	if(count >= 1000)
#else
   //滴答中断 这边是 125ms
	if(count >= 7)
#endif	
	
	{
		unix_time++;
		struct tm p_real_time ;
		bsp_rtc_get_date_time(&p_real_time);
		for (uint8_t i = 0; i < array_size(bsp_list); i++) 
		{
			if(p_real_time.tm_hour == bsp_list[i].timer_config.tm_hour
				&& p_real_time.tm_min == bsp_list[i].timer_config.tm_min 
				&& p_real_time.tm_sec == bsp_list[i].timer_config.tm_sec)//ervey day
			{
				if(bsp_list[i].timer_callback != NULL)
				{
					bsp_list[i].timer_callback();
				}
			}
		}
		count=0;
	}
	else
	{
		count++;
	}
}


static void bsp_rtc_init(nrf_drv_rtc_t *rtc_handler)
{
    uint32_t err_code;

    // Initialize RTC instance
    nrf_drv_rtc_config_t config = NRF_DRV_RTC_DEFAULT_CONFIG;

#if (defined(HANDWARE_4_5_1) || defined(HANDWARE_1_14_1x))

	config.prescaler = 33; // 1/ (32768/33) = 1ms
#else
    config.prescaler = 4095; // 12bit
#endif	
//    
	
    err_code = nrf_drv_rtc_init(rtc_handler, &config, bsp_rtc_callback);
    APP_ERROR_CHECK(err_code);
    nrf_drv_rtc_tick_enable(rtc_handler, true);
    nrf_drv_rtc_enable(rtc_handler);
	   
}

static void bsp_rtc_set_date_time(struct tm *date_time)
{
	struct tm t;
	t.tm_year = date_time->tm_year - 1900;
	t.tm_mon = date_time->tm_mon- 1;
	t.tm_mday = date_time->tm_mday;
	t.tm_hour = date_time->tm_hour;
	t.tm_min = date_time->tm_min;
	t.tm_sec = date_time->tm_sec;
	t.tm_wday = date_time->tm_wday;
	t.tm_yday = date_time->tm_yday - 1;

	unix_time = mktime(&t);// - 8*60*60;
}

static void bsp_rtc_get_date_time(struct tm *ble_date_time)
{
	
	time_t temp_time = (time_t)unix_time ;//+ (8*60*60);
	struct tm *p_real_time = localtime(&temp_time);
	ble_date_time->tm_year = p_real_time->tm_year + 1900;
	ble_date_time->tm_mon = p_real_time->tm_mon + 1;
	ble_date_time->tm_mday = p_real_time->tm_mday;
	ble_date_time->tm_hour = p_real_time->tm_hour;
	ble_date_time->tm_min = p_real_time->tm_min;
	ble_date_time->tm_sec = p_real_time->tm_sec;
	ble_date_time->tm_wday = p_real_time->tm_wday;
	ble_date_time->tm_yday = p_real_time->tm_yday + 1;

//    NRF_LOG_INFO("get_date_time:%u-%u-%u %u:%u:%u",
//                                    ble_date_time->tm_year,
//                                    ble_date_time->tm_mon,
//                                    ble_date_time->tm_mday,
//                                    ble_date_time->tm_hour,
//                                    ble_date_time->tm_min,
//                                    ble_date_time->tm_sec);
}

static uint64_t bsp_rtc_get_unix_time(struct tm *time)
{
	struct tm t;
	time_t timestamp = 0;
	t.tm_year = time->tm_year - 1900;
	t.tm_mon = time->tm_mon- 1;
	t.tm_mday = time->tm_mday;
	t.tm_hour = time->tm_hour;
	t.tm_min = time->tm_min;
	t.tm_sec = time->tm_sec;

	timestamp = mktime(&t);// - 8*60*60;
	


	uint64_t temp = timestamp;
	return (temp  * 1000) + count;
		
	
	
}

static void bsp_rtc_set_unix_time_s(uint32_t s)
{
	unix_time = s;
//  unix_time_s_8x_s += (zone * 60 * 60 * 1000);

	struct tm tmp;
  bsp_rtc_get_date_time(&tmp);
}


/*******************************************************************************
 * Function Name     : bsp_gpio_output_open
 * Description       : gpio out put open
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_rtc_open(q_device_t*dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].lock)
			{
				return RESULT_OK;
			}
			bsp_rtc_init(&bsp_list[i].rtc_handler);
//			// 在程序初始化时设置时区
//			setenv("TZ", "Asia/Shanghai", 1);
//			tzset();
			bsp_list[i].lock = true;
			return RESULT_OK;
		}
	}
	return RESULT_RTC_DEV_NULL_ERR;	
}

/*******************************************************************************
 * Function Name     : bsp_i2c_write
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_rtc_write(q_device_t *dev, int pos,const void *buffer, int size)
{
	
	struct rtc_time *time = (struct rtc_time *)buffer;
	if(time == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}

	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_RTC_DEV_NULL_ERR;
			}
			if(time->unix_time != 0)
			{
				bsp_rtc_set_unix_time_s(time->unix_time);
			}
			else
			{
				bsp_rtc_set_date_time(&time->bj_time);
			}
			return RESULT_OK;
		}
	}
  return RESULT_RTC_DEV_NULL_ERR;
}

/*******************************************************************************
 * Function Name     : bsp_i2c_read
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_rtc_read(q_device_t *dev, int pos,const void *buffer, int size)
{
	
	struct rtc_time *time = (struct rtc_time *)buffer;
	if(time == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}

	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_RTC_DEV_NULL_ERR;
			}
			bsp_rtc_get_date_time(&time->bj_time);

		
			time->unix_ms_time = bsp_rtc_get_unix_time(&time->bj_time);
			time->unix_time = time->unix_ms_time / 1000;
            
            // add by liukun start 20260415
            time->bj_time.tm_hour += TB_OFFSET;
            // 处理跨天进位
            if (time->bj_time.tm_hour >= 24) {
                time->bj_time.tm_hour -= 24;
                time->bj_time.tm_mday += 1;
                // 处理跨月
                uint32_t crudays = days_in_month(time->bj_time.tm_year, time->bj_time.tm_mon);
                if (time->bj_time.tm_mday > crudays) {
                    time->bj_time.tm_mday = 1;
                    time->bj_time.tm_mon += 1;
                    if(time->bj_time.tm_mon > 12)
                        time->bj_time.tm_mon = 1;
                }
            }
            // add by liukun end 20260415

			return RESULT_OK;
		}
	}
  return RESULT_RTC_DEV_NULL_ERR;
}

/*******************************************************************************
 * Function Name     : bsp_rtc_timer_register_callback
 * Description       : rtc定时超时回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_rtc_timer_register_callback(q_device_t *dev,int pos, void *exit_irq_callback)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(exit_irq_callback != NULL)
			{
				bsp_list[i].timer_callback = (bsp_rtc_timer_callback)exit_irq_callback;
			}
			else
			{
				bsp_list[i].timer_callback = NULL;
			}
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;		
}

static int bsp_rtc_timer_config(q_device_t *dev, void *args, void *var)
{
	struct tm *tm_config = (struct tm *)args;
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			memcpy((uint8_t*)&bsp_list[i].timer_config,tm_config,sizeof(struct tm));
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;	
}


#endif



#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)

#include "OSAL.h"

#include "clock.h"

struct  BSP_RTC
{
	const char   *name;
	bool          lock;
  struct rtc_timer_config timer_config;
	bsp_rtc_timer_callback timer_callback;
	q_device_t dev;
};

static struct BSP_RTC bsp_list[1] = 
{
	{
		.name = "sys rtc",
		.lock = false,
		.timer_config = {
			                .hour = 23,
			                .min = 59,
			                .sec = 59,
		                },
		.timer_callback = NULL,
		.dev = {0},
	},
};


#define DTM2TM(tm, dtm)     {   (tm)->tm_mon = (dtm)->month-1;\
        (tm)->tm_year = (dtm)->year-1900;\
        (tm)->tm_mday = (dtm)->day;\
        (tm)->tm_hour = (dtm)->hour;\
        (tm)->tm_min = (dtm)->minutes;\
        (tm)->tm_sec = (dtm)->seconds;}

		
#if(USE_SYS_TICK)
    #define RTC_CNT_RANGE 0x100000000
    #define TM_RATE (1000000/625)
#else
    #define RTC_CNT_RANGE 0x100000000 // for 32bit rtc
    #define TM_RATE (32768)
#endif		
		
typedef struct
{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} datetime_t;

typedef struct
{
    uint64_t tm_base;
    uint32_t snapshot;
    uint32_t reserved;  //reserved for 4 byte align
} datetime_cfg_t;

static datetime_cfg_t s_stm_cfg = {0};

static const char* const month_str[12] =
{
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};


static void app_datetime_adjust_baseline(struct tm* datetm);

static void print_hex (uint8_t* data, uint16 len)
{
    uint16 i;

    for (i = 0; i < len - 1; i++)
    {
        printf("%x",data[i]);
        printf(" ");
    }

    printf("%x",data[i]);
}

static void bsp_timer_cnt_get(uint32_t* tick)
{
    #if(USE_SYS_TICK)
    *tick = hal_systick();         // read current RTC counter
    #else
    *tick = rtc_get_counter();         // read current RTC counter
    #endif
}

static void get_default_tm(struct tm* datetm)
{
    int i;
    datetime_t dtm;
    char dt[20];
    char* pstr = NULL;
    printf(__DATE__);
    printf(__TIME__);
    strcpy(dt, __DATE__);
    pstr = strtok(dt, " ");

    for(i = 0; i< 12; i++)
    {
        if(strcmp(dt,month_str[i]) == 0)
        {
            dtm.month = i+1;
            break;
        }
    }

    pstr = strtok(NULL, " ");
    dtm.day = atoi(pstr);
    pstr = strtok(NULL, " ");
    dtm.year = atoi(pstr);
    strcpy(dt, __TIME__);
    pstr = strtok(dt, ":");
    dtm.hour = atoi(pstr);
    pstr = strtok(NULL, ":");
    dtm.minutes = atoi(pstr);
    pstr = strtok(NULL, ":");
    dtm.seconds = atoi(pstr);
    DTM2TM(datetm, &dtm);
}

static void check_default_datetime(void)
{
    struct tm datetm;
    get_default_tm(&datetm);
    printf("check_default_datetime\nTime is %d-%d-%d, %d:%d:%d",
        datetm.tm_year+1900,
        datetm.tm_mon+1,
        datetm.tm_mday,
        datetm.tm_hour,
        datetm.tm_min,
        datetm.tm_sec);
    app_datetime_adjust_baseline(&datetm);
    return;
}

static void app_datetime_adjust_baseline(struct tm* datetm)
{
    uint64_t tmstamp;
    uint32_t ticks;
    bsp_timer_cnt_get(&ticks);
    tmstamp = (uint64_t)mktime(datetm);
    printf(" %x\r\n",(uint32_t)tmstamp);
    tmstamp = tmstamp*TM_RATE;
    s_stm_cfg.tm_base = tmstamp - ticks;
    s_stm_cfg.snapshot = ticks;
    printf("app_datetime_adjust_baseline: ");
    print_hex((uint8_t*)&s_stm_cfg, sizeof(s_stm_cfg));
    printf("\n");
}

static int app_datetime_set(datetime_t dtm)
{
    struct tm tm;
    memset(&tm,0, sizeof(tm));
    DTM2TM(&tm, &dtm);
    printf("\napp_datetime_set:\n");
    printf("Time is %d-%d-%d, %d:%d:%d\n", dtm.year, dtm.month, dtm.day, dtm.hour, dtm.minutes, dtm.seconds);
    app_datetime_adjust_baseline(&tm);
    print_hex((uint8_t*)&s_stm_cfg, sizeof(s_stm_cfg));
    printf("\n");
    return 0;
}


//读时间戳 s
static uint64_t get_unix_time(void)
{
    uint32_t ticks;
    time_t tm = (time_t)(s_stm_cfg.tm_base / TM_RATE);
    bsp_timer_cnt_get(&ticks);

    if(s_stm_cfg.snapshot > ticks)
        tm += RTC_CNT_RANGE/TM_RATE;

    tm += ticks/TM_RATE;
    
    return tm;
}

//写时间戳 ms
static void set_unix_time(uint64_t uinx_time)
{
    uint64_t tmstamp;
    uint32_t ticks;
    
    bsp_timer_cnt_get(&ticks);
    tmstamp = uinx_time;
    printf(" %x\r\n",ticks);
    printf(" %x\r\n",(uint32_t)tmstamp);
    tmstamp = tmstamp*TM_RATE;
    s_stm_cfg.tm_base = tmstamp - ticks;
    s_stm_cfg.snapshot = ticks;
    printf("set_unix_time:\n");
    print_hex((uint8_t*)&s_stm_cfg, sizeof(s_stm_cfg));
    printf("\n");
}

static void bsp_rtc_get_date_time(struct tm *ble_date_time,uint32_t uinx_time)
{
	
	uint32_t temp_time = uinx_time + (8*60*60);
	struct tm *p_real_time = localtime(&temp_time);
	ble_date_time->tm_year = p_real_time->tm_year + 1900;
	ble_date_time->tm_mon = p_real_time->tm_mon + 1;
	ble_date_time->tm_mday = p_real_time->tm_mday;
	ble_date_time->tm_hour = p_real_time->tm_hour;
	ble_date_time->tm_min = p_real_time->tm_min;
	ble_date_time->tm_sec = p_real_time->tm_sec;

//    NRF_LOG_INFO("get_date_time:%u-%u-%u %u:%u:%u",
//                                    ble_date_time->tm_year,
//                                    ble_date_time->tm_mon,
//                                    ble_date_time->tm_mday,
//                                    ble_date_time->tm_hour,
//                                    ble_date_time->tm_min,
//                                    ble_date_time->tm_sec);
}

/*******************************************************************************
 * Function Name     : bsp_gpio_output_open
 * Description       : gpio out put open
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_rtc_open(q_device_t*dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].lock)
			{
				return RESULT_OK;
			}
			check_default_datetime();
			bsp_list[i].lock = true;
			return RESULT_OK;
		}
	}
	return RESULT_RTC_DEV_NULL_ERR;	
}


/*******************************************************************************
 * Function Name     : bsp_i2c_write
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_rtc_write(q_device_t *dev, int pos,const void *buffer, int size)
{
	
	struct rtc_time *time = (struct rtc_time *)buffer;
	if(time == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}

	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_RTC_DEV_NULL_ERR;
			}
			if(time->unix_time != 0)
			{
				set_unix_time(time->unix_time);
			}
			else
			{
				datetime_t dtm;
				dtm.day = time->bj_time.tm_mday;
				dtm.hour = time->bj_time.tm_hour;
				dtm.minutes = time->bj_time.tm_min;
				dtm.month = time->bj_time.tm_mon;
				dtm.seconds = time->bj_time.tm_sec;
				dtm.year = time->bj_time.tm_year;
				app_datetime_set(dtm);
			}
			return RESULT_OK;
		}
	}
  return RESULT_RTC_DEV_NULL_ERR;
}

/*******************************************************************************
 * Function Name     : bsp_i2c_read
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_rtc_read(q_device_t *dev, int pos,const void *buffer, int size)
{
	
	struct rtc_time *time = (struct rtc_time *)buffer;
	if(time == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}

	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_RTC_DEV_NULL_ERR;
			}
			time->unix_time = get_unix_time();
			bsp_rtc_get_date_time(&time->bj_time,time->unix_time);
   
			return RESULT_OK;
		}
	}
  return RESULT_RTC_DEV_NULL_ERR;
} 


/*******************************************************************************
 * Function Name     : bsp_rtc_timer_register_callback
 * Description       : rtc定时超时回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_rtc_timer_register_callback(q_device_t *dev,int pos, void *exit_irq_callback)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			bsp_list[i].timer_callback = (bsp_rtc_timer_callback)exit_irq_callback;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;		
}



#endif

/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static struct q_device_ops ops =
{
	.read = bsp_rtc_read,
	.write = bsp_rtc_write,
	.register_callback = bsp_rtc_timer_register_callback,
	.open = bsp_rtc_open,
	.config = bsp_rtc_timer_config,
	
};

/*******************************************************************************
 * Function Name     : bsp_gpio_output_register
 * Description       : 设备注册
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_rtc_register(void)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		bsp_list[i].dev.name = bsp_list[i].name;
		bsp_list[i].dev.dops  = &ops;
		q_device_register(&bsp_list[i].dev);		
	}
}


device_initcall(bsp_rtc_register);





