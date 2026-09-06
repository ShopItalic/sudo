#ifndef __APP_PPG_H__
#define __APP_PPG_H__


#include <stdint.h>




enum app_ppg_event
{
	PPG_COLLECTION_SPO2_EVNET = 0,
	PPG_COLLECTION_HRM_EVNET = 1,
	PPG_CHECK_STATUS_EVENT = 2,
	PPG_LED_TEST_EVENT = 3,
	PPG_HRM_TEST_EVENT = 4,
	PPG_SPO2_TEST_EVENT = 5,
	PPG_DIAG_MODE_EVENT = 6,
	PPG_AUTOMATIC_CYCLE_COLLECTION_SPO2_EVENT = 7,
	PPG_AUTOMATIC_CYCLE_COLLECTION_HRM_EVENT = 8,
	PPG_COLLECTION_IR_EVENT = 9,
	PPG_GRAY_CARD_TEST_EVENT = 10,
	PPG_IR_RED_GREEN_EVENT = 11,
	PPG_IDIE_EVENT = 12,
};



struct ppg_package_config
{
//	struct app_package_basic basic_hear;
    uint8_t time;        //采集时间
    uint8_t frez;         //采集频率
    uint8_t wave_upload;  //波形配置
    uint8_t process_upload;//进度配置
    uint8_t rr_upload;      //间期配置 0：不上传，1：上传	
};


enum ppg_wearing_status
{
	PPG_NOT_WEARING = 0,   //未佩戴
	PPG_WEARING,           //佩戴
	PPG_CHARGE,            //充电中
	PPG_COLLECTING,        //采集中
	PPG_BUSY,              //忙
	PPG_COLLECTION_TIMEOUT, //数据采集超时
	PPG_AUDIO,             //音频采集中
};

#pragma pack (1)
struct ppg_hrm_result
{
	enum ppg_wearing_status wearing_status;
	uint8_t  hrs;              //心率
	uint8_t  hrv;              //心率变异性
	uint8_t  stress_index;     //压力指数
	uint16_t temp;             //温度
};
#pragma pack()

#pragma pack (1)
struct ppg_spo2_result
{
	enum ppg_wearing_status wearing_status;
	uint8_t  hrs;         //心率
	uint8_t  spo2;        //心率变异性
	uint16_t temp;        //温度
};
#pragma pack()

struct ppg_result
{
	struct ppg_hrm_result    hrm_result;
	struct ppg_spo2_result   spo2_result;
};

#pragma pack (1)
struct ppg_hrm_data
{
	int32_t gre_dara;
	int16_t g_sensor_x;
	int16_t g_sensor_y;
	int16_t g_sensor_z;
};
#pragma pack()

#pragma pack (1)
struct ppg_spo2_data
{
	int32_t red_dara;
	int32_t ir_dara;
	int16_t g_sensor_x;
	int16_t g_sensor_y;
	int16_t g_sensor_z;
};
#pragma pack()




#if defined(HANDWARE_1_14_1)
#pragma pack (1)
struct ppg_spo2_hr_temper_data
{
	int32_t gre_dara;
	int32_t red_dara;
	int32_t ir_dara;
	int16_t g_sensor_x;
	int16_t g_sensor_y;
	int16_t g_sensor_z;
	int16_t g_sensor_gyro_x;
	int16_t g_sensor_gyro_y;
	int16_t g_sensor_gyro_z;
	uint16_t temper_0;
	uint16_t temper_1;
	uint16_t temper_2;
};
#pragma pack()

#pragma pack (1)
struct ppg_spo2_hr_temper_time_data
{
	uint64_t uinx_time_ms;
	struct ppg_spo2_hr_temper_data spo2_hr_temper_data[5];
};
#pragma pack()

#pragma pack (1)
struct ppg_spo2_hr_temper_pack_pyload
{
	uint8_t seq;
	uint8_t data_num;
	struct ppg_spo2_hr_temper_time_data spo2_hr_temper_data;
};
#pragma pack()

#else

#pragma pack (1)
struct ppg_spo2_hr_temper_data
{
	int32_t gre_dara;
	int32_t red_dara;
	int32_t ir_dara;
	uint16_t temper_dara;
	int16_t g_sensor_x;
	int16_t g_sensor_y;
	int16_t g_sensor_z;
};
#pragma pack()

#pragma pack (1)
struct ppg_spo2_hr_temper_pack_pyload
{
	uint8_t seq;
	uint8_t data_num;
	struct ppg_spo2_hr_temper_data spo2_hr_temper_data[11];
};
#pragma pack()

#endif


#pragma pack (1)
struct ppg_spo2_pack_pyload
{
	uint8_t seq;
	uint8_t data_num;
	struct ppg_spo2_data spo2_data[15];
};
#pragma pack()

#pragma pack (1)
struct ppg_hrm_pack_pyload
{
	uint8_t seq;
	uint8_t data_num;
	struct ppg_hrm_data hrm_data[17];
};
#pragma pack()

enum ppg_pack_type
{
	PPG_PACK_TYPE_RESULT = 0,
	PPG_PACK_TYPE_WAVEFORM_DATA,
	PPG_PACK_TYPE_RR,
	PPG_PACK_TYPE_HRV_TEST = 0x13,
	PPG_PACK_TYPE_SPO2_TEST = 0x14,
	PPG_PACK_TYPE_DIAG_TEST = 0x15,
	PPG_PACK_TYPE_SCHEDULE = 0xFF,
};







#endif

