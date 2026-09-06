#include "bc_ble_gap.h"



//引用的C库头文件
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
//Log需要引用的头文件
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
//APP定时器需要引用的头文件
#include "app_timer.h"

//广播需要引用的头文件
#include "ble_advdata.h"
#include "ble_advertising.h"
//电源管理需要引用的头文件
#include "nrf_pwr_mgmt.h"
//SoftDevice handler configuration需要引用的头文件
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
//排序写入模块需要引用的头文件
#include "nrf_ble_qwr.h"
//GATT需要引用的头文件
#include "nrf_ble_gatt.h"
//连接参数协商需要引用的头文件
#include "ble_conn_params.h"

//DFU需要引用的头文件
#include "nrf_dfu_ble_svci_bond_sharing.h"
#include "nrf_svci_async_function.h"
#include "nrf_svci_async_handler.h"
#include "nrf_power.h"
#include "ble_dfu.h"
#include "nrf_bootloader_info.h"

#include "ble_conn_params.h"
#include "ble_srv_common.h"
#include "ble_conn_state.h"


#include "app_error.h"
#include "bc_logger.h"
#include "bc_device_info.h"
#include "bc_ble_modu_interface.h"
#include "bc_delay.h"

extern uint16_t m_conn_handle;
static enum bc_ble_conn_params connect_params = BLE_CONN_PARAMS_FAST;

#if 1
#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(24, UNIT_1_25_MS)   // 最小连接间隔 (0.03 秒) 
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(48, UNIT_1_25_MS) 
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(5000, UNIT_10_MS)   // 监督超时(4 秒) 
#define SLAVE_LATENCY                   0                                 // 从机延迟 

#define SLOW_MIN_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)   // 最小连接间隔 (0.03 秒) 
#define SLOW_MAX_CONN_INTERVAL               MSEC_TO_UNITS(220, UNIT_1_25_MS)   // 最大连接间隔 (0.03 秒) 

#define FAST_MIN_CONN_INTERVAL               MSEC_TO_UNITS(24, UNIT_1_25_MS)   // 最小连接间隔 (0.015 秒) 
#define FAST_MAX_CONN_INTERVAL               MSEC_TO_UNITS(48, UNIT_1_25_MS)   // 最大连接间隔 (0.015 秒) 

#define AUDIO_MIN_CONN_INTERVAL               MSEC_TO_UNITS(24, UNIT_1_25_MS)   // 最小连接间隔 (0.015 秒) 
#define AUDIO_MAX_CONN_INTERVAL               MSEC_TO_UNITS(48, UNIT_1_25_MS)   // 最大连接间隔 (0.015 秒) 
#else
#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(24, UNIT_1_25_MS)   // 最小连接间隔 (0.03 秒) 
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(24, UNIT_1_25_MS)   // 最大连接间隔 (0.03 秒) 
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)   // 监督超时(4 秒) 
#define SLAVE_LATENCY                   0                                 // 从机延迟 

#define SLOW_MIN_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)   // 最小连接间隔 (0.03 秒) 
#define SLOW_MAX_CONN_INTERVAL               MSEC_TO_UNITS(220, UNIT_1_25_MS)   // 最大连接间隔 (0.03 秒) 

#define FAST_MIN_CONN_INTERVAL               MSEC_TO_UNITS(30, UNIT_1_25_MS)   // 最小连接间隔 (0.015 秒) 
#define FAST_MAX_CONN_INTERVAL               MSEC_TO_UNITS(50, UNIT_1_25_MS)   // 最大连接间隔 (0.015 秒) 

#define AUDIO_MIN_CONN_INTERVAL               MSEC_TO_UNITS(16, UNIT_1_25_MS)   // 最小连接间隔 (0.015 秒) 
#define AUDIO_MAX_CONN_INTERVAL               MSEC_TO_UNITS(26, UNIT_1_25_MS)   // 最大连接间隔 (0.015 秒) 
#endif


//GAP参数初始化，该函数配置需要的GAP参数，包括设备名称，外观特征、首选连接参数
void gap_params_init(void)
{
    ret_code_t              err_code;
	  //定义连接参数结构体变量
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;
	uint8_t device_name_buffer[32] = {0};
    //设置GAP的安全模式
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
	uint8_t name_length = 6;

	bc_device_info_get_ble_name(device_name_buffer,&name_length);
  if(name_length == 0)
  {
    memcpy(device_name_buffer, (uint8_t *)"BCL603", name_length);
    name_length = 6;
  }

		  //设置GAP设备名称，
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)device_name_buffer,
                                          name_length);																				
    //检查函数返回的错误代码
		APP_ERROR_CHECK(err_code);
//	err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_HID_MOUSE);
//    APP_ERROR_CHECK(err_code);
										  
//	err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_HID_KEYBOARD);
//    APP_ERROR_CHECK(err_code);
	bc_device_hid_info *hid_info;
	hid_info = bc_device_info_get_hid_info();
//	if(hid_info->device_hid_type == 1 && (hid_info->device_hid_gesture_mode != 0xFF || hid_info->device_hid_touch_mode != 0xFF))
	if(hid_info->device_hid_type == 1)
	{		
		err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_HID);
		APP_ERROR_CHECK(err_code);
		
//		err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_HID_MOUSE);
//		APP_ERROR_CHECK(err_code);
	}
																				
    //设置首选连接参数，设置前先清零gap_conn_params
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;//最小连接间隔
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;//最小连接间隔
    gap_conn_params.slave_latency     = SLAVE_LATENCY;    //从机延迟           
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT; //监督超时
    //调用协议栈API sd_ble_gap_ppcp_set配置GAP参数
    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
	  connect_params = BLE_CONN_PARAMS_FAST;																				
}



///falg: 1: fast 0:slow
bool bc_conn_params_change(enum bc_ble_conn_params conn_params)
{
  
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 || HARDWARE_1191_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	
  
	return true;
  
#endif	
	
	ret_code_t ret;
	ble_gap_conn_params_t   gap_conn_params_get;
	ble_gap_conn_params_t   gap_conn_params;
	
	ble_conn_params_get(m_conn_handle,&gap_conn_params_get);
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));
	
	switch(conn_params)
	{
		case BLE_CONN_PARAMS_SLOW:
		{
			if(gap_conn_params_get.min_conn_interval == SLOW_MIN_CONN_INTERVAL &&
				gap_conn_params_get.max_conn_interval == SLOW_MAX_CONN_INTERVAL &&
			    gap_conn_params_get.slave_latency     == SLAVE_LATENCY &&
			    gap_conn_params_get.conn_sup_timeout  == CONN_SUP_TIMEOUT
			  )
			{
				return true;
			}
			else
			{
				gap_conn_params.min_conn_interval = SLOW_MIN_CONN_INTERVAL;//最小连接间隔
				gap_conn_params.max_conn_interval = SLOW_MAX_CONN_INTERVAL;//最小连接间隔
				gap_conn_params.slave_latency     = SLAVE_LATENCY;    //从机延迟           
				gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT; //监督超时
			}
			break;
		}
		case BLE_CONN_PARAMS_FAST:
		{
			if(gap_conn_params_get.min_conn_interval == FAST_MIN_CONN_INTERVAL &&
				gap_conn_params_get.max_conn_interval == FAST_MAX_CONN_INTERVAL &&
			    gap_conn_params_get.slave_latency     == SLAVE_LATENCY &&
			    gap_conn_params_get.conn_sup_timeout  == CONN_SUP_TIMEOUT
			  )
			{
				return true;
			}
			else
			{
				gap_conn_params.min_conn_interval = FAST_MIN_CONN_INTERVAL;//最小连接间隔
				gap_conn_params.max_conn_interval = FAST_MAX_CONN_INTERVAL;//最小连接间隔
				gap_conn_params.slave_latency     = SLAVE_LATENCY;    //从机延迟           
				gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT; //监督超时
			}

			break;
		}
		case BLE_CONN_PARAMS_AUDIO:
		{
			if(gap_conn_params_get.min_conn_interval == AUDIO_MIN_CONN_INTERVAL &&
				gap_conn_params_get.max_conn_interval == AUDIO_MAX_CONN_INTERVAL &&
			    gap_conn_params_get.slave_latency     == SLAVE_LATENCY &&
			    gap_conn_params_get.conn_sup_timeout  == CONN_SUP_TIMEOUT
			  )
			{
				return true;
			}
			else
			{
				gap_conn_params.min_conn_interval = AUDIO_MIN_CONN_INTERVAL;//最小连接间隔
				gap_conn_params.max_conn_interval = AUDIO_MAX_CONN_INTERVAL;//最小连接间隔
				gap_conn_params.slave_latency     = SLAVE_LATENCY;    //从机延迟           
				gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT; //监督超时
			}

			break;
		}
	}
 
	ret = ble_conn_params_change_conn_params(m_conn_handle, &gap_conn_params);
	if(ret == NRF_SUCCESS)
	{
		BC_LOG_INFO("change_conn_params %d success \r\n", conn_params);
		bc_delay_ms(20);
	    return true;
	}
	else
	{
		BC_LOG_INFO("change_conn_params %d fail %d \r\n", conn_params, ret);
	    return false;
	}
}
















