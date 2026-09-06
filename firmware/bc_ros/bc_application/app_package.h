#ifndef __APP_PACKAGE_H__
#define __APP_PACKAGE_H__





#include "app_cmd_handler.h"

#include "stdint.h"


struct app_package_basic
{
	uint8_t frame_type;
	uint8_t frame_id;
	uint8_t cmd;
	uint8_t subcmd;
};


enum app_button_type
{
	BUTTON_LONG_PRESS = 0,
	BUTTON_CLICK_PRESS,
	BUTTON_DOUBLE_PRESS,
	BUTTON_THREE_PRESS,
	BUTTON_SWIPE_UP_PRESS,
	BUTTON_SWIPE_DOWN_PRESS,
  BUTTON_SWIPE_LEFT_PRESS,
	BUTTON_SWIPE_RIGHT_PRESS,
};

//struct app_package
//{
//	
//	uint8_t pyload[243];
//	uint16_t pyload_length;
//};



void app_package_send_enqueue(struct app_cmd_package * cmd_package,uint8_t length);


void app_package_ppg(struct app_cmd_package *package,void  const *pack_pyload,uint16_t pyload_length,uint8_t pack_type);

void app_package_history_record_up(struct app_cmd_package *package,void const *pack_pyload,uint16_t pyload_length,uint32_t total_num,uint32_t seq);

void app_package_ppg_ir_midvalue_up(uint8_t seq,uint8_t midvalue_num,uint8_t *midvalue_data,uint8_t midvalue_length);

void app_package_authentication_req_code_up(void);

void app_package_sports_stop(void);

void app_package_authentication_test_recv(void);

void app_package_button_up(enum app_button_type button_type);

void app_package_temper_up(uint8_t id,uint8_t status,uint16_t temper,uint8_t subcmd);

void app_package_button_rawdata_up(uint8_t *rawdata,uint8_t rawdata_length);

void app_package_button_rawdata_check_up(uint8_t *rawdata,uint8_t rawdata_length);

void app_package_ble_log_up(uint8_t *send_data,uint8_t length);

void app_package_ppg_file_uplaod(struct app_cmd_package *package, uint16_t length);

#if 0
void app_package_precent_up(uint16_t data);
#else
void app_package_precent_up(uint8_t data);
#endif

void app_package_mic_recording_start(void);

void app_package_mic_recording_stop(void);

void app_package_mic_recording_stop_isr(void);

void app_package_mic_capture_recording_start(void);

void app_package_mic_capture_recording_stop(void);

void app_package_mic_touch_start(void);

void app_package_mic_touch_stop(void);

void app_package_init(void);

void app_ble_recv_enent(uint8_t *recv_data,uint16_t recv_length);

void app_package_mouse_event_up(uint8_t *send_data,uint8_t length);

void app_package_pdm_upload_over(void);

void app_package_speed_test_up(uint32_t seq,uint8_t leng);

void app_package_file_spi_uplaod(struct app_cmd_package *package, uint16_t length);

void app_package_active_upload_file_name(uint8_t *file_name,uint8_t name_length);
void app_package_active_upload_check(void);
void app_package_precent_status_up(uint16_t data);

void app_package_ipc_ic_led_ble_connect(void);
void app_package_ipc_ic_led_ble_disconnect(void);

void app_package_pdm_switch_online_to_offline(void);
void app_package_pdm_key_flag_clear(void);

#endif
