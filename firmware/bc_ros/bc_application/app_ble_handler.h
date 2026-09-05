#ifndef __APP_BLE_HANDLER_H__
#define __APP_BLE_HANDLER_H__



#include "stdint.h"
#include <stdbool.h>

enum app_ble_task_event
{
	BLE_TASK_TYPE_RECV = 0,
	BLE_TASK_TYPE_SEND,
	BLE_TASK_TYPE_NUM
};


enum app_ble_timer_type
{
	BLE_CONNECT_IDIE_TIMEOUT_TIMER = 0,
	BLE_TIMER_TYPE_NUM
};


//enum app_ble_hid_mode
//{
//	BLE_HID_VIDEO_MODE = 0,
//	BLE_HID_PHOTOGRAPH_MODE,
//	BLE_HID_MUISC_MODE,
//	BLE_HID_PPT,
//	BLE_HID_SNAP,
//};

enum app_ble_hid_touch_mode
{
	BLE_HID_TOUCH_VIDEO_MODE = 0,
	BLE_HID_TOUCH_PHOTOGRAPH_MODE,
	BLE_HID_TOUCH_MUISC_MODE,
	BLE_HID_TOUCH_PPT,
	BLE_HID_TOUCH_UP_AUDIO,
};

enum app_ble_hid_gesture_mode
{
	BLE_HID_GESTURE_VIDEO_MODE = 0,
	BLE_HID_GESTURE_PHOTOGRAPH_MODE,
	BLE_HID_GESTURE_MUISC_MODE,
	BLE_HID_GESTURE_PPT,
	BLE_HID_GESTURE_SNAP,
};

void app_connect_idie_timer_start(enum app_ble_timer_type time_id);

void app_connect_idie_timer_start_from_isr(enum app_ble_timer_type time_id);

void app_ble_handler_thread_create(void);



void app_ble_mouse_left_button(void);

void app_ble_mouse_right_button(void);

void app_ble_mouse_cancel_button(void);

void app_ble_mouse_slide_up(void);

void app_ble_mouse_slide_down(void);

void app_ble_mouse_pulley_up(void);

void app_ble_mouse_pulley_down(void);

void app_ble_mouse_android_pulley_up(void);

void app_ble_mouse_android_pulley_down(void);

void app_ble_mouse_ios_pulley_up(void);

void app_ble_mouse_ios_pulley_down(void);

void app_ble_mouse_x_movement(int16_t movement_data);

void app_ble_mouse_y_movement(int16_t movement_data);

void app_ble_mouse_x_y_movement(int16_t x_movement_data,int16_t y_movement_data);

void app_ble_mouse_movement_origin(void);

void app_ble_hid_volume_up(void);

void app_ble_hid_volume_down(void);

void app_ble_hid_previous_music(void);

void app_ble_hid_next_music(void);

void app_ble_hid_touch_mode_set(enum app_ble_hid_touch_mode hid_mode);

enum app_ble_hid_touch_mode app_ble_hid_touch_mode_get(void);

void app_adv_data_update(uint8_t *data,uint8_t length);

void app_ble_mac_get(uint8_t *mac_buff);

void app_ble_mac_set(uint8_t *mac_buff);

bool app_ble_connect_status(void);

bool app_ble_notify_allowed(void);

void app_ble_send(uint8_t *send_data,uint8_t send_length);

void app_ble_conn_time_audio_set(void);

void app_ble_conn_time_audio_reset(void);

void app_ble_hid_phone_screen_set(uint32_t phone_screen_high,uint32_t phone_screen_width,char *phone_type_name,uint8_t phone_type_name_length);
void app_ble_hid_phone_screen_get(uint32_t *phone_screen_high,uint32_t *phone_screen_width,char *phone_type_name,uint8_t *phone_type_name_length);

void app_ble_adv_start(void);

void app_ble_adv_stop(void);

	
#endif



