#ifndef __APP_KVDB_HANDLER_H__
#define __APP_KVDB_HANDLER_H__

#include "stdint.h"
#include "stdbool.h"

struct device_info_kv
{
	char key[40];
	uint8_t value[40];
};

enum device_info_kv_type
{
	DEVICE_INFO_PPG_UPDATA_RECORD = 0,              //ppg 历史记录上传成功后的时间，用于记录上传时间标志
	DEVICE_INFO_PPG_AUTOMATIC_CYCLE_TIME ,          //ppg 自动周期采集的间隔时间
	DEVICE_INFO_G_SENSOR_SPORT_STEP,                //运动步数
	DEVICE_INFO_BLE_NAME,
	DEVICE_INFO_TYPE_NUM
};







void app_kvdb_init(void);





void app_kvdb_read_device_name(void);

void app_kvdb_read_device_info(void);

//void app_tsdb_handler_thread_create(void);



bool app_kvdb_set_handler(enum device_info_kv_type kv_id,uint8_t *value,uint8_t value_length);
bool app_kvdb_get_handler(enum device_info_kv_type kv_id,uint8_t *value,uint8_t value_length);

void app_kvdb_test(void);

#endif




