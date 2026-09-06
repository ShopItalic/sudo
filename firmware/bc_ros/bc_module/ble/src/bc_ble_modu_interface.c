/*******************************************************************************
此为ble对外接口文件，通过宏定义来兼容nordic、phy6222硬件平台

日  期：2024年1月17日
编写人：邱成凯
 *******************************************************************************/

#include "bc_ble_modu_interface.h"
#include "ring_config.h"
//#include "app_ble_handler.h"
#include "bc_logger.h"
#include "app_cmd_handler.h"
#include "app_pdm_handler.h"

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)
#include "bc_ble.h"

#include "bc_queue.h"
#include "bc_ble_gap.h"
#include "bc_ble_adv.h"

#endif


#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)
#include "ring_service.h"
#include "app_ring.h"
#endif

#include "string.h"
#include "bc_logger.h"

static struct bc_ble_calss ble_calss = {0};

struct bc_ble_data_package ble_port_cmd_pack;


static void bc_ble_recv(uint8_t *recv_data,uint16_t recv_length)
{
#if (HARDWARE_ARCH_TYPE_NORDIC == 1)	
	struct bc_ble_data_package ble_recv_pack = {0};
    if (!recv_data || !recv_length || recv_length > sizeof(ble_recv_pack.data))
        return;
	memcpy(ble_recv_pack.data,recv_data,recv_length);
    ble_recv_pack.data_length = recv_length;	
	BC_LOG_HEX("ble recv:",ble_recv_pack.data,ble_recv_pack.data_length);
//  if(ble_recv_pack.data[2] == 0x71 && ble_recv_pack.data[3] == 0x05 && ble_recv_pack.data[4] == 0x00)
//  {
////    app_cmd_package_parse(ble_recv_pack.data,ble_recv_pack.data_length);
//    app_pdm_recording_stop();
//  }
//  else
//  {
    if (__get_IPSR() != 0)
        (void)bc_queue_isr_enqueue(BC_QUEUE_TYPE_BLE_RECV, &ble_recv_pack);
    else
        (void)bc_queue_enqueue(BC_QUEUE_TYPE_BLE_RECV, &ble_recv_pack);
//  }
//	
#endif	
	
}


void bc_ble_connect_callabck_register(void *callabck_register)
{
	if(callabck_register == NULL)
	{
		return;
	}
	ble_calss.ble_connect_callback = (bc_ble_connect_callback)callabck_register;
}

void bc_ble_disconnect_callabck_register(void *callabck_register)
{
	if(callabck_register == NULL)
	{
		return;
	}
	ble_calss.ble_disconnect_callback = (bc_ble_disconnect_callback)callabck_register;
}

void bc_ble_pm_connect_callabck_register(void *callabck_register)
{
	if(callabck_register == NULL)
	{
		return;
	}
	ble_calss.ble_pm_connect_callback = (bc_ble_pm_connect_callback)callabck_register;
}

void bc_ble_pm_disconnect_callabck_register(void *callabck_register)
{
	if(callabck_register == NULL)
	{
		return;
	}
	ble_calss.ble_pm_disconnect_callback = (bc_ble_pm_disconnect_callback)callabck_register;
}

struct bc_ble_calss bc_ble_new(void)
{
	
	
	ble_calss.ble_recv = bc_ble_recv;
	ble_calss.ble_send = bc_ble_send;
	ble_calss.ble_init = bc_ble_init;
	ble_calss.ble_connect_status = bc_ble_connect_status;
	ble_calss.ble_disconnect = bc_ble_disconnect;
	ble_calss.ble_mac_get = bc_ble_mac_get;
  ble_calss.ble_mac_set = bc_ble_mac_set;
	ble_calss.ble_mouse_button_control = ble_mouse_button_control;
	ble_calss.ble_mouse_movement = mouse_movement_send;
	ble_calss.ble_hid_send_cmd = ble_hid_send_cmd;
	ble_calss.ble_connect_params_update = bc_conn_params_change;
	ble_calss.ble_adv_pyload_update = ble_adv_data_update;
	ble_calss.ble_adv_start = advertising_start;
	ble_calss.ble_adv_stop = advertising_stop;
	ble_calss.ble_hid_phone_screen_set = ble_hid_phone_screen_set;
	ble_calss.ble_hid_phone_screen_get = ble_hid_phone_screen_get;
	ble_calss.ble_pm_connect_status = bc_ble_pm_connect_status;
	
	return ble_calss;
}




















