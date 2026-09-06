#include <time.h>
#include "zs_common.h"
#include "rtc.h"

ZS_SENSOR_STATUS_T zspd_satus = RESET_STATUS ;
WORK_MODE_T workmode ;
ZS_WEAR_STATUS_CHANGE_T wearstatus;
ZS_AGC_WORK_T agcmode = NO_WORK;
ZS_PDLED_T pdled;
ZS_SERIES_T zs_series = NULL_SERIES;
TIMESTAMP_T Timestampbuf, Timestamp; 

volatile uint8_t ppgdatastatus = 0;
volatile uint8_t ecgdatastatus = 0;
/*
 * @description		: 毫秒延时函数
 * @param - ms		: 毫秒数
 * @return 			: 无
 */
void ZS_DelayMs(uint32_t ms)
{
	HAL_Delay(ms);		
}

// /*
//  * @description		: 获取时间戳
//  * @param 	        : none
//  * @return 			: 当前时间戳
//  */
// double ZS_GetTimestampsubs(void)
// {
//     static double timbef = 0;  
//     struct tm tmtmp;
// 	DATA_TIME_T dt;
// 	double tt;

// 	RTC_Get_Time(&dt.hour, &dt.minute, &dt.second, &dt.subsecond);
// 	RTC_Get_Date(&dt.year, &dt.month, &dt.day);  

// 	tmtmp.tm_sec = dt.second;
// 	tmtmp.tm_min = dt.minute;
// 	tmtmp.tm_hour = dt.hour-8;
// 	tmtmp.tm_mday = dt.day;
// 	tmtmp.tm_mon = dt.month-1;
// 	tmtmp.tm_year = dt.year+100;

// 	tt = ((double)(mktime(&tmtmp))) + dt.subsecond;

// 	if((timbef - tt) > 0) {
// 		printf("timestamp+1\n");
// 		tt += 1;
// 	}
// 	timbef = tt;
// 	return tt;
// }

/*
 * @description		: 获取时间戳
 * @param 	        : none
 * @return 			: 当前时间戳
 */
int64_t ZS_GetTimestamp(void)
{
    DATA_TIME_T dt;
    struct tm tmtmp;
	static int64_t timbef = 0; 
	int64_t tt; 
    
    RTC_Get_Time(&dt.hour, &dt.minute, &dt.second, &dt.subsecond);	
    RTC_Get_Date(&dt.year, &dt.month, &dt.day);	

    tmtmp.tm_sec = dt.second;
    tmtmp.tm_min = dt.minute;
    tmtmp.tm_hour = dt.hour-8;
    tmtmp.tm_mday = dt.day;
    tmtmp.tm_mon = dt.month-1;
    tmtmp.tm_year = dt.year+100;

	tt = mktime(&tmtmp);

	if((timbef - tt) > 0) {
		printf("timestamp+1\n");
		tt += 1;
	}
	timbef = tt;
	return tt;
}



