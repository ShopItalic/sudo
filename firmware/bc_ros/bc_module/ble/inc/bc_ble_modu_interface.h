#ifndef __BC_BLE_MODU_INTERFACE_H__
#define __BC_BLE_MODU_INTERFACE_H__


#include "stdint.h"
#include "stdbool.h"
#include "bc_ble_hids_service.h"

enum bc_ble_conn_params
{
	BLE_CONN_PARAMS_SLOW = 0,
	BLE_CONN_PARAMS_FAST,
	BLE_CONN_PARAMS_AUDIO
};

enum bc_ble_event
{
	BLE_RECV_EVENT = 0,
	BLE_SEND_EVENT,
	BLE_SET_ADV_CONFIG_EVENT,
	BLE_SET_ADV_4_2_EVENT,
	BLE_SET_ADV_5_0_EVENT,
	BLE_CONNECT_EVENT,
	BLE_DISCONNECT_EVENT,
	BLE_START_SCAN_EVENT,
	BLE_STOP_SCAN_EVENT,
	BLE_SCAN_DATA_EVENT,
	BLE_SCAN_CONFIG_EVENT,
//	BLE_EVENT_NUM,
};

struct bc_ble_data_package
{
	uint8_t data[250];
	uint16_t data_length;
    uint32_t session_id; /* Queue metadata only; never transmitted. */
};


typedef void (*bc_ble_recv_callback)(uint8_t *recv_data,uint16_t recv_length); 
typedef void (*bc_ble_send_callback)(uint8_t *send_data,uint16_t send_length); 
typedef void (*bc_ble_init_callback)(void); 
typedef bool (*bc_ble_connect_status_callback)(void);
typedef void (*bc_ble_connect_callback)(void); 
typedef void (*bc_ble_disconnect_callback)(void);

#if defined(HANDWARE_1_23_4)
typedef bool (*bc_ble_connect_guard_callback)(void);
#endif

typedef bool (*bc_ble_pm_connect_status_callback)(void);
typedef void (*bc_ble_pm_connect_callback)(void); 
typedef void (*bc_ble_pm_disconnect_callback)(void);

typedef void (*bc_ble_disconnect_operate)(void); 
typedef void (*bc_ble_mac)(uint8_t *ble_mac); 
typedef void (*bc_ble_mouse_button_control)(enum ble_hid_mouse_button_cmd mouse_button_cmd);
typedef void (*bc_ble_mouse_movement)(int16_t x_delta, int16_t y_delta);
typedef void (*bc_ble_hid_send_cmd)(enum ble_hid_cmd  hid_cmd);
typedef bool (*bc_ble_connect_params)(enum bc_ble_conn_params conn_params); 

typedef void (*bc_ble_adv_pyload_update)(uint8_t *adv_update_data,uint8_t update_length); 
typedef void (*bc_ble_adv_start)(void);
typedef void (*bc_ble_adv_stop)(void); 

typedef void (*bc_ble_hid_phone_screen_set)(uint32_t phone_screen_high,uint32_t phone_screen_width,char *phone_type_name,uint8_t phone_type_name_length);
typedef void (*bc_ble_hid_phone_screen_get)(uint32_t *phone_screen_high,uint32_t *phone_screen_width,char *phone_type_name,uint8_t *phone_type_name_length);

struct bc_ble_calss
{
	bc_ble_recv_callback               ble_recv;
	bc_ble_send_callback               ble_send;
	bc_ble_init_callback               ble_init;
	bc_ble_connect_status_callback     ble_connect_status;
	bc_ble_connect_callback            ble_connect_callback;
	bc_ble_disconnect_callback         ble_disconnect_callback;
	bc_ble_disconnect_operate          ble_disconnect;
	bc_ble_mac                         ble_mac_get;
	bc_ble_mac                         ble_mac_set;
	bc_ble_mouse_button_control        ble_mouse_button_control;
	bc_ble_mouse_movement              ble_mouse_movement;
	bc_ble_hid_send_cmd                ble_hid_send_cmd;
	bc_ble_connect_params              ble_connect_params_update;

	bc_ble_hid_phone_screen_set        ble_hid_phone_screen_set;
	bc_ble_hid_phone_screen_get        ble_hid_phone_screen_get;	
	bc_ble_pm_connect_status_callback  ble_pm_connect_status;
	bc_ble_pm_connect_callback         ble_pm_connect_callback;
	bc_ble_pm_disconnect_callback      ble_pm_disconnect_callback;
  
  bc_ble_adv_pyload_update           ble_adv_pyload_update;
	bc_ble_adv_start                   ble_adv_start;
	bc_ble_adv_stop                    ble_adv_stop;
	
};


void bc_ble_connect_callabck_register(void *callabck_register);

void bc_ble_disconnect_callabck_register(void *callabck_register);

void bc_ble_pm_connect_callabck_register(void *callabck_register);

void bc_ble_pm_disconnect_callabck_register(void *callabck_register);

#if defined(HANDWARE_1_23_4)
void bc_ble_connect_guard_register(void *guard);
#endif

struct bc_ble_calss bc_ble_new( void );

void ble_port_cmd_pack_handler(void);










#endif







