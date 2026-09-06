#ifndef _ZS_COMMON_H_
#define _ZS_COMMON_H_

#include <stdio.h>
//#include "n.h"
#include <stdint.h>

#define SAMPLE_FS_1HZ   	1 	/* for calibration only */
#define SAMPLE_FS_5HZ   	5 	/* IR wearalbe detect */
#define SAMPLE_FS_10HZ   	10 	/* IR wearalbe detect */
#define SAMPLE_FS_25HZ   	25 	
#define SAMPLE_FS_50HZ  	50 	
#define SAMPLE_FS_100HZ  	100 
#define SAMPLE_FS_200HZ  	200 
#define SAMPLE_FS_250HZ		250
#define SAMPLE_FS_300HZ		300	
#define SAMPLE_FS_500HZ		500
#define SAMPLE_FS_600HZ		600
#define SAMPLE_FS_1000HZ	1000
#define SAMPLE_FS_1200HZ	1200

// #define HRS_SAMPLE_25HZ
// #define HRS_SAMPLE_50HZ
#define HRS_SAMPLE_100HZ
// #define HRS_SAMPLE_200HZ
#if defined (HRS_SAMPLE_25HZ)
	#define HRS_SAMPLE_FS   		SAMPLE_FS_25HZ 
#elif defined (HRS_SAMPLE_50HZ)
	#define HRS_SAMPLE_FS   		SAMPLE_FS_50HZ 
#elif defined (HRS_SAMPLE_100HZ)
	#define HRS_SAMPLE_FS   		SAMPLE_FS_100HZ 
#else
	#define HRS_SAMPLE_FS   		SAMPLE_FS_200HZ 
#endif 

#define DHR_SAMPLE_25HZ
// #define DHR_SAMPLE_50HZ
// #define DHR_SAMPLE_100HZ
// #define DHR_SAMPLE_200HZ
#if defined (DHR_SAMPLE_25HZ)
	#define DHR_SAMPLE_FS   		SAMPLE_FS_25HZ 
#elif defined (DHR_SAMPLE_50HZ)
	#define DHR_SAMPLE_FS   		SAMPLE_FS_50HZ 
#elif defined (DHR_SAMPLE_100HZ)
	#define DHR_SAMPLE_FS   		SAMPLE_FS_100HZ 
#else
	#define DHR_SAMPLE_FS   		SAMPLE_FS_200HZ 
#endif 

// #define SPO2_SAMPLE_50HZ
#define SPO2_SAMPLE_100HZ
// #define SPO2_SAMPLE_200HZ
#if defined (SPO2_SAMPLE_50HZ)
	#define SPO2_SAMPLE_FS   		SAMPLE_FS_50HZ 
#elif defined (SPO2_SAMPLE_100HZ)
	#define SPO2_SAMPLE_FS   		SAMPLE_FS_100HZ 
#else
	#define SPO2_SAMPLE_FS   		SAMPLE_FS_200HZ 
#endif 

// #define ECG_SAMPLE_250HZ
// #define ECG_SAMPLE_300HZ
// #define ECG_SAMPLE_500HZ
// #define ECG_SAMPLE_600HZ
// #define ECG_SAMPLE_1000HZ
#define ECG_SAMPLE_1200HZ
#if defined (ECG_SAMPLE_250HZ)
	#define ECG_SAMPLE_FS   		SAMPLE_FS_250HZ 
#elif defined (ECG_SAMPLE_300HZ)
	#define ECG_SAMPLE_FS   		SAMPLE_FS_300HZ 
#elif defined (ECG_SAMPLE_500HZ)
	#define ECG_SAMPLE_FS   		SAMPLE_FS_500HZ 
#elif defined (ECG_SAMPLE_600HZ)
	#define ECG_SAMPLE_FS   		SAMPLE_FS_600HZ 
#elif defined (ECG_SAMPLE_1000HZ)
	#define ECG_SAMPLE_FS   		SAMPLE_FS_1000HZ 
#else
	#define ECG_SAMPLE_FS   		SAMPLE_FS_1200HZ 
#endif 


#define ACC_SAMPLE_FS	     	SAMPLE_FS_25HZ	
#define WEARING_CHECK_SAMPLE_FS	SAMPLE_FS_10HZ

#define BIT_WIDTH_32BIT	    32
#define BIT_WIDTH_24BIT	    24
#define BIT_WIDTH_16BIT	    16
#define HRS_BIT_WIDTH	    BIT_WIDTH_16BIT
#define DHR_BIT_WIDTH	    BIT_WIDTH_16BIT
#define ACC_BIT_WIDTH	    BIT_WIDTH_16BIT
#define ECG_BIT_WIDTH	    BIT_WIDTH_16BIT


// #define HRS_BUF_LEN	    (1024*8)
// #define DRS_BUF_LEN	    (1024*21)
// #define SPO2_BUF_LEN		(1024*10)
// #define ECG_BUF_LEN	    (1024*22)
// #define WEARING_BUF_LEN	 (1024*0.5)


#define DEFAULT_AGC_CAL_SAMPLE_FS	SAMPLE_FS_100HZ


#define AGC_PPG_THRESHOLD_LV1	13107
#define AGC_PPG_THRESHOLD_LV2	21000//24576
#define AGC_PPG_THRESHOLD_LV3	27000//29491
#define AGC_PPG_THRESHOLD_LV4	32767
#define AGC_PPG_THRESHOLD_LV5	-32768
#define AGC_GREEN_PPG_THRESHOLD_LV1	AGC_PPG_THRESHOLD_LV1
#define AGC_GREEN_PPG_THRESHOLD_LV2	AGC_PPG_THRESHOLD_LV2
#define AGC_GREEN_PPG_THRESHOLD_LV3	AGC_PPG_THRESHOLD_LV3


#define IR_DETEC_THRESHOLD_MIN		3000
// #define IR_DETEC_THRESHOLD_MAX		10000
// #define IR_JUDGE_THRESHOLD_MIN		3000
#define IR_JUDGE_THRESHOLD_MAX		32000
#define IR_JUDGE_THRESHOLD_32BIT_MIN		100000
#define GREEN_DETEC_THRESHOLD_MIN		3000
#define GREEN_DETEC_THRESHOLD_MAX		32767
#define GREEN_JUDGE_THRESHOLD_MIN		19000
#define GREEN_JUDGE_THRESHOLD_MAX		30000

// #define MOD_COLLECT_DATA

typedef struct {
	uint16_t ChipID;
	uint16_t Version;
}ZSPD_INFO_t;

typedef enum {  
	Z_GREEN_CH = 1, 
	Z_RED_CH = 2,  
	Z_IR_CH = 4, 
} ZSPD_LED_T;

typedef struct {  
	uint16_t REG14CONN; 
	uint16_t REG15CONN; 
} ZS_LED_CONN_T;

typedef enum {  
	USE_INTERNAL_PD = 0,
	USE_OUTTERNAL_PD	
} ZS_PD_IN_OUT_T;

typedef struct {  
	uint16_t REG18; 
	uint16_t REG19; 
	uint16_t REG1A; 
	uint16_t REG1B;
	uint16_t REG1C;
	uint16_t REG25;
	ZS_PD_IN_OUT_T PDInOut;	
	uint8_t PDNum;	
} ZSPD_PD_CONN_T;

typedef struct {  
	uint16_t TiaGainCap;
	uint16_t TiaVref;
	uint8_t RedCurrentReg;
	uint8_t IRCurrentReg;
	uint8_t GreenCurrentReg;
	ZS_LED_CONN_T GreenConnect;
	ZS_LED_CONN_T RedConnect;
	ZS_LED_CONN_T IRConnect;
	ZSPD_PD_CONN_T PDconnect; 
	uint16_t ChannelVal;
	uint16_t DataMode;	
} ZS_PDLED_T;

typedef struct {  
	uint8_t led_cur;
	uint16_t tia_cap_group; 
	uint8_t status;  
} ZSPD_LED_CFG_T;

typedef enum {  
	PDLED_MODE_NULL = 0, 
	PDLED_MODE1,
	PDLED_MODE2,  
	PDLED_MODE3,
	PDLED_MODE4,
	PDLED_MODE5,
	PDLED_MODE6
} ZS_PDLED_MODE_T;

typedef enum {  
	ZSPD_OK = 0,

	ZSPD_ID_FAIL = 101, 
	ZSPD_AGC_OFFSET_FAIL,  
	ZSPD_IR_DETEC_CONTINUE,

	ZSPD_AGC_REACH_GAIN_MAX,
	ZSPD_AGC_REACH_GAIN_MIN,
	ZSPD_AGC_REACH_CURRENT_MAX,
	ZSPD_AGC_REACH_CURRENT_MIN,	
	ZSPD_AGC_CAL_CONTINUE,
	ZSPD_DATA_OUT_FAIL,
	ZSPD_DATA_FAIL
	
} ZSPD_ERROR_CODE_T;


typedef enum {
	RESET_STATUS = 0,
	CLK32K_CAL,		
	AGC_OFFSET_CAL,		/* ADC校准模式 */
	IR_DETEC_LOW_FS,	/* 低功耗模式 */
	GET_BEST_CHANNEL,	/* 获取信号最好的通道 */
	IR_AGC_CAL,			/* IR调光模式 */
	RED_AGC_CAL,		/* RED调光模式 */
	WEAR_JUDGE,
	AGLO_AGC_PREPARE,	/* 调光预备态 */
	GREEN_AGC_CAL,		/* GREEN调光模式 */
	IR_GREEN_AGC_CAL,	/* GREEN IR调光模式 */
	IR_RED_AGC_CAL,		/* red IR调光模式 */
	IR_NORMAL,  		/* ir工作模式 */
	RED_NORMAL,  		/* red工作模式 */
	GREEN_NORMAL,  		/* green工作模式 */
	IR_GREEN_NORMAL,	/* ir green工作模式 */
	IR_RED_NORMAL,		/* ir red工作模式 */
	GREEN_RED_NORMAL,	/* red green工作模式 */
	ECG_NORMAL,			/* ecg工作模式 */
	GREEN_IR_RED_AGC_CAL,	/* GREEN IR red调光模式 */
	GREEN_IR_RED_NORMAL,		/* ir red green工作模式 */
	GREEN_MID_CAL_PRE,
	IR_RED_MID_CAL_PRE,
	GREEN_IR_RED_MID_CAL_PRE,
	GREEN_MID_CAL,		/* green中途调光 */
	IR_RED_MID_CAL,		/* ir red green中途调光 */
	GREEN_IR_RED_MID_CAL		/* ir red green中途调光 */
	
} ZS_SENSOR_STATUS_T;

typedef enum {  
	NO_WEAR= 0,
	WEAR
} ZS_WEAR_STATUS_T;

typedef enum {  
	NO_WORK = 0,
	AGC_LP_WORK,
	AGC_PER_WORK
} ZS_AGC_WORK_T;


typedef struct {  
	ZS_WEAR_STATUS_T status;
	uint8_t change;  
} ZS_WEAR_STATUS_CHANGE_T;

typedef enum {    
	NULL_MODE,
    HR_MODE, 
    DHR_MODE,
    SPO2_MODE,	
	ECG_MODE,
	SPO2SIM_MODE,		
	FR_50HZ_MODE,
    FR_100HZ_MODE,
    FR_200HZ_MODE
} WORK_MODE_T;

typedef enum {    
	NULL_SERIES = 0,
    ZSPD_SERIES, 
    ZSBM_SERIES
} ZS_SERIES_T;

typedef struct {  
	// uint8_t led_cur;
	uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    float subsecond;
} DATA_TIME_T;

typedef struct {  
	uint16_t count;
    int64_t timestamp[5];
} TIMESTAMP_T;

extern TIMESTAMP_T Timestampbuf;
extern TIMESTAMP_T Timestamp;
extern ZS_SERIES_T zs_series;
extern ZS_SENSOR_STATUS_T zspd_satus ;
extern WORK_MODE_T workmode ;
extern ZS_WEAR_STATUS_CHANGE_T wearstatus;
extern ZS_AGC_WORK_T agcmode;
extern ZS_PDLED_T pdled;
extern volatile uint8_t ppgdatastatus;
// extern volatile uint8_t timestampcount;
extern volatile uint8_t ecgdatastatus;

extern void ZS_DelayMs(uint32_t ms);
extern int64_t ZS_GetTimestamp(void);

#endif


