#ifndef __BC_BLE_HIDS_SERVICE_H__
#define __BC_BLE_HIDS_SERVICE_H__

#include "stdint.h"



enum ble_hid_mouse_button_cmd
{
	MOUSE_LEFT_BUTTON_HOLD = 0,
	MOUSE_RIGHT_BUTTON_HOLD,
	MOUSE_BUTTON_CANCL,
	MOUSE_SLIDE_UP,
	MOUSE_SLIDE_DOWN,
	MOUSE_PULLEY_UP,
	MOUSE_PULLEY_DOWN,
	MOUSE_ANDROID_PULLEY_UP,
	MOUSE_ANDROID_PULLEY_DOWN,
	MOUSE_IOS_PULLEY_UP,
	MOUSE_IOS_PULLEY_DOWN,
	MOUSE_BUTTON_CMD_NUM
	
};

enum ble_hid_cmd
{
	BLE_HID_VOLUSE_UP = 0,
	BLE_HID_VOLUSE_DOWN,
	BLE_HID_PREVIOUS_MUSIC,
	BLE_HID_NEXT_MUSIC,
};


struct ble_hid_mouse_button_cmd_data
{
	uint8_t ouse_cmd_data[13];
	uint8_t ouse_cmd_data_length;
};


void hids_init(uint16_t  *p);
void mouse_movement_send(int16_t x_delta, int16_t y_delta);
void mouse_button_send(int8_t click, int8_t wheel, int8_t pan);

void ble_mouse_button_control(enum ble_hid_mouse_button_cmd mouse_button_cmd);

void ble_hid_send_cmd(enum ble_hid_cmd  hid_cmd);

void ble_hid_phone_screen_set(uint32_t phone_screen_high,uint32_t phone_screen_width,char *phone_type_name,uint8_t phone_type_name_length);
void ble_hid_phone_screen_get(uint32_t *phone_screen_high,uint32_t *phone_screen_width,char *phone_type_name,uint8_t *phone_type_name_length);

// void sensor_simulator_init(void);
// void peer_manager_init(void);
// void hid_services_init(void);
#endif


