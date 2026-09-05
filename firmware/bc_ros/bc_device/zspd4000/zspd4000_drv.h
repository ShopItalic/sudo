#ifndef _ZSPD4000_DRV_H_
#define _ZSPD4000_DRV_H_

#include "zspd4000_port.h"
#include "stdbool.h"
#define ECG_ALG_IN_SIZE	100

#define SAMPLE_FS_1HZ   	1 	/* for calibration only */
#define SAMPLE_FS_5HZ   	5 	/* IR wearalbe detect */
#define SAMPLE_FS_10HZ   	10 	/* IR wearalbe detect */
#define SAMPLE_FS_25HZ   	25 	/* Factory test, HRS*/
#define SAMPLE_FS_50HZ  	50 	/* HRV */
#define SAMPLE_FS_100HZ  	100 /* SPO2 */
#define SAMPLE_FS_300HZ		300	/* ECG */

#define DEFAULT_AGC_CAL_SAMPLE_FS	SAMPLE_FS_100HZ
#define FIFO_DEPTH_AGC_CAL_IR_DETECT		6		/* 3 aligned */
#define FIFO_DEPTH_AGC_PROCESS	4		/* 4 aligned */
#define FIFO_DEPTH_HRS_HRV			25	/* 25/50/100 Hz, 25 aligned */
#define FIFO_DEPTH_SPO2					40	/* multy pulse, 4 aligned */
#define FIFO_DEPTH_ECG					45	/* ecg 300hz, Right/PPG 50hz, 9 aligned */	

//#define AGC_CAL_FIFO_DEPTH				4			// Used for AGC OFFSET/AGC cal, FIFO (3)

/**************************************
Note: IR always set to timeslot A 
			Green always set to timeslot B
			Red always set to timeslot C
			ledchannel 1 -> IR
			ledchannel 2 -> Green
			ledchannel 3 -> RED
**************************************/
//#define USE_INTERNAL_PD
#ifdef USE_INTERNAL_PD
   #define TIA_GAIN_CAP						TIA_12K5_CAP_8P86
   #define TIA_VREF							TIA_VREF_0P64V
   #define LED_IR_CURRENT_REG				0x15
   #define LED_RED_CURRENT_REG				0x14
   #define LED_GREEN_CURRENT_REG			0x13
   #define PD0_CONNECTION					(INP01_PRE_CON_FLOAT | INP01_ACT_CON_FLOAT)
   #define PD1_CONNECTION					(INP01_PRE_CON_FLOAT | INP01_ACT_CON_FLOAT)
#else
   #define TIA_GAIN_CAP						TIA_12K5_CAP_8P86
   #define TIA_VREF								TIA_VREF_1P26V
   #define LED_RED_CURRENT_REG			0x14
   #define LED_IR_CURRENT_REG				0x15
   #define LED_GREEN_CURRENT_REG		0x13
   #define PD0_CONNECTION						0x42 //(INP01_PRE_CON_FLOAT | INP01_ACT_CON_VC)
   #define PD1_CONNECTION						0x12 //(INP01_PRE_CON_FLOAT | INP1_ACT_CON_TIAN)
#endif

#define IR_GAIN_MAX_LV		4
#define GREEN_GAIN_MAX_LV	5
#define RED_GAIN_MAX_LV		5
#define AGC_IR_LED_CURREN_MIN				2
#define AGC_IR_LED_CURREN_MAX				100
#define AGC_GREEN_LED_CURREN_MIN		2
#define AGC_GREEN_LED_CURREN_MAX		100
#define AGC_RED_LED_CURREN_MIN			2
#define AGC_RED_LED_CURREN_MAX			100

#define AGC_PPG_THRESHOLD_LV1	13107
#define AGC_PPG_THRESHOLD_LV2	19000//24576
#define AGC_PPG_THRESHOLD_LV3	26500//29491
#define AGC_PPG_THRESHOLD_LV4	32767

#define AGC_CAL_IR_INIT_CURR			30
#define AGC_CAL_IR_TIA_GAIN_CAP		TIA_12K5_CAP_8P86
/*
#define AGC_CAL_GREEN_INIT_CURR		50
#define AGC_CAL_RED_INIT_CURR			20
*/
#define IR_DETEC_THRESHOLD_MIN		3000
#define IR_DETEC_THRESHOLD_MAX		10000

#define ZSPD4000_FIFO_MAX_DEPTH		128

#define HRS_NORMAL_GAIN						TIA_50K_CAP_4P74
	
#define ZSPD_READ_REG16(reg, buf, num)		ZSPD4000_ReadMultyWord(reg, buf, num)
#define ZSPD_WRITE_REG16(reg, buf, num)		ZSPD4000_WriteMultyWord(reg, buf, num)



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
	uint16_t REG18;
	uint16_t REG19; 
	uint16_t REG1A;  
} ZSPD_PD_CONN_T;

typedef struct {  
	uint8_t led_cur;
	uint16_t tia_cap_group; 
	uint8_t status;  
} ZSPD_LED_CFG_T;

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
	
} ZSPD_ERROR_CODE_T;

typedef enum {
	RESET_STATUS = 0,
	CLK32K_CAL,
	
	AGC_OFFSET_CAL,

	IR_DETEC_LOW_FS,

	IR_AGC_CAL,				// testonly
	RED_AGC_CAL,			// testonly

	GREEN_AGC_CAL,		// HRS & HRV
	IR_GREEN_AGC_CAL,	// 
	IR_RED_AGC_CAL,		// SPO2

	IR_NORMAL,  			// testonly
	RED_NORMAL,  
	GREEN_NORMAL,  
	IR_GREEN_NORMAL,
	IR_RED_NORMAL,

	FT_GREEN_NORMAL,	// factory test only
	FT_RED_NORMAL,	//  factory test only
	FT_GREEN_RED_NORMAL,	//  factory test only
	FT_GREEN_RED_IR_NORMAL,	//  factory test only
	FT_TEST_JDI_PDS,
	
	ECG_NORMAL,
} ZSPD_SENSOR_STATUS_T;

extern ZSPD_SENSOR_STATUS_T zspd_satus ;
extern WORK_MODE_T workmode ;
extern uint8_t ZSPD4000_ReadFifoCount(void) ;
extern void ZSPD4000_ReadFifoData(uint8_t read_fifo_size, int16_t *buf) ;

extern bool ZSPD4000_DataHandle(void);
extern void ZSPD4000_CommonInit(void);
extern ZSPD_ERROR_CODE_T ZSPD4000_Init(WORK_MODE_T mode);
extern void ZSPD4000_EcgPreInit(void);
extern void ZSPD4000_EcgInit(void);
extern void ZSPD4000_GpioIntConfig(uint8_t en);
extern void ZSPD_IRDeteLowFs(void);
void ZSPD4000_SetLedCurrent( ZSPD_LED_T ledchannel, uint16_t ledcurrent);
extern void ZSPD4000_DumpRegister(void);
void ZSPD_GreenNormal(uint16_t samplerate);
void ZSPD_IrRedNormal(uint16_t samplerate);

void ZSPD4000_unint(void);

bool ZSPD4000_hr_data_callback_register(void *function_callback);

bool ZSPD4000_spo2_data_callback_register(void *function_callback);

bool ZSPD4000_hr_result_callback_register(void *function_callback);

bool ZSPD4000_spo2_result_callback_register(void *function_callback);

bool ZSPD4000_spo2_signal_check_callback_register(void *function_callback);

bool ZSPD4000_hr_signal_check_callback_register(void *function_callback);

bool ZSPD4000_gary_card_callback_register(void *function_callback);

uint8_t* zspd_algo_buffer_addr(void);

uint16_t ZSPD4000_ID_get(void);

void ppg_ir_flag_set(bool flag);

bool ZSPD4000_algo_init_para(void *init_para);

bool zspd400_agc_comp_flg(void);

void zsbm_algo_fix_jump_point_v1(int32_t *data, int32_t length, int32_t thres);					//32bit数据挑点滤波处理
void zsbm_algo_fix_jump_point_v1_16bit(int16_t *data, int16_t length, int16_t thres) ;	//16bit跳点滤波处理

uint16_t SPO2_data_processing(int32_t * fifo_data, uint16_t len,void ** ts_b_data,void ** ts_c_data,uint8_t clear_first_in);//SPO2模式数据拆分提取程序  滤波算法
uint16_t Hr_Dhr_data_process(int16_t * fifo_data, uint16_t len,uint8_t clear_first_in);				//DHR,HR 模式滤波算法
uint16_t SPO2_data_processingV2(uint8_t * red_data, uint16_t len_red,uint8_t * ir_data, uint16_t len_ir,uint8_t clear_first_in);

#endif
