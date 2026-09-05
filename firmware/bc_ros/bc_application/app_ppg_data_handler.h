#ifndef __APP_PPG_DATA_HANDLER_H__
#define __APP_PPG_DATA_HANDLER_H__




#include <stdint.h>
#include <stdbool.h>

#include "app_package.h"

//enum app_ppg_data_task_event
//{
//	APP_PPG_HR_DATA_TASK_EVENT = 0,
//	APP_PPG_SPO2_DATA_TASK_EVENT,
//	APP_PPG_DATA_STORAGE_RECORD,
//	APP_PPG_AUTOMATIC_CYCLE_EVENT,
//	APP_PPG_DATA_EVENT_NUM
//};

enum app_ppg_time_type
{
	PPG_AUTOMATIC_CYCLE_COLLECTION_TIME = 0,
	PPG_WEAR_DETECTION_TIMEOUT_TIME,
	PPG_COLLECTION_TIMEOUT_TIME,
	PPG_TIME_TYPE_NUM,
};

void app_ppg_hr_data_queue(int16_t *data,int16_t *acc_data,uint8_t length);

void app_ppg_hr_32bit_data_queue(int32_t *data,int16_t *acc_data,uint8_t length);

void app_ppg_spo2_data_queue(void *red_data,uint8_t red_length,void *ir_data,uint8_t ir_length,int16_t *acc_data);

void app_ppg_spo2_hr_data_queue(void *red_data,uint8_t red_length,void *ir_data,uint8_t ir_length,void *gre_data,uint8_t gre_length,int16_t *acc_data);

void app_ppg_collection_progress_update(bool flag);

void app_ppg_hrm_ble_cmd_start(struct app_cmd_package * pack);

void app_ppg_data_handler_task_event_init(void);

void app_ppa_collection_hrm_result(uint8_t heart_rate,uint8_t hrv1);
void app_ppa_collection_hrv_result(int32_t *rri_data,uint8_t length);

void app_ppa_collection_spo2_result(uint8_t spo2,uint8_t heart_rate);

void app_ppg_spo2_ble_cmd_start(struct app_cmd_package * pack);

uint8_t app_ppg_get_hr(void);

void app_ppg_spo2_hr_led_collecting(uint32_t time,uint8_t process,uint8_t wave,uint8_t *handler);

void app_ppg_hrm_and_spo2_automatic_cycle_collection_start(void);

void app_ppg_hrm_and_spo2_automatic_cycle_collection_stop(void);

void app_ppg_ir_ble_cmd_start(struct app_cmd_package * pack);

void app_ppa_spo2_signal_check_callback(uint16_t signal_strength);

void app_ppa_hr_signal_check_callback(uint16_t signal_strength);

void app_ppg_gary_card_data_callback(void *green_data,void* red_data,void* ir_data);

void app_ppg_gary_card_test_cmd_start(struct app_cmd_package * pack);

void app_ppg_automatic_cycle_collection_time_update(void);

void app_ppg_check_staus(void);

void app_ppg_data_queue_clear(void);

void app_ppg_colllection_stop(void);

uint8_t app_ppg_hr_get(void);

bool ppg_wear_flag_get(void);

struct ppg_body_information * ppg_body_information_get(void);

void app_spo2_info_get(uint8_t *hr,uint8_t *spo2,uint32_t *temper);

void app_ppg_spo2_always_collecting(void);

void ppg_data_spo2_data_port_file_handler_event_callback_poll(void);

void app_ppg_collection_timeout_timer_stop(void);

void app_ppg_test(void);

#endif










