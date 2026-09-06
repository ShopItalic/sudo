#ifndef __APP_DB_HANDLER_H__
#define __APP_DB_HANDLER_H__


#include "stdint.h"
#include "app_cmd_handler.h"
#include "stdbool.h"

#pragma pack (1)
typedef struct{
    uint8_t size[3];//当前数据的长度，不包括自身
    uint32_t unix_time_s;
    uint16_t accumulated_step;
    uint8_t hr;
    uint8_t spo2;
    uint8_t hrv;
    uint8_t sprit;
    int16_t temp;
    uint8_t sport_mode;
    uint8_t sleep_mode;
//    uint16_t sleep_accumulated_time;
	uint8_t perfusion;
	uint8_t reserve;
    uint8_t rr_num;
    uint16_t rr_array[30];
}store_data_unit_t;
#pragma pack ()


enum app_tsdb_event
{
	APP_TSDB_UP_DATA_EVENT = 0,
	APP_TSDB_UP_TIMEOUT_EVENT,
	APP_TSDB_EVENT_NUM
};


void app_flashdb_init(void);


bool app_tsdb_data_write(store_data_unit_t *store_data);

void app_tsdb_data_port_updata(struct app_cmd_package * cmd_package);
void app_tsdb_data_all_updata(struct app_cmd_package * cmd_package);

void app_tsdb_data_stop_updata(void);

void app_tsdb_clear(struct app_cmd_package * cmd_package);

uint8_t app_tsdb_hr_get(void);

void app_tsdb_test(void);

uint16_t app_tsdb_sport_count_get(void);

void app_tsdb_clear_event(void);

void app_tsdb_data_port_updata_event(void);













#endif





