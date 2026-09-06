#include "fml_ble_gap.h"



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



#include "app_error.h"
#include "log.h"


#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(30, UNIT_1_25_MS)   // 最小连接间隔 (0.03 秒) 
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(30, UNIT_1_25_MS)   // 最大连接间隔 (0.03 秒) 
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)   // 监督超时(4 秒) 
#define SLAVE_LATENCY                   0                                 // 从机延迟 




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
	memcpy(device_name_buffer, (uint8_t *)"BCL603", 6);
		  //设置GAP设备名称，
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)device_name_buffer,
                                          6);																				
    //检查函数返回的错误代码
		APP_ERROR_CHECK(err_code);
																				
    //设置首选连接参数，设置前先清零gap_conn_params
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;//最小连接间隔
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;//最小连接间隔
    gap_conn_params.slave_latency     = SLAVE_LATENCY;    //从机延迟           
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT; //监督超时
    //调用协议栈API sd_ble_gap_ppcp_set配置GAP参数
    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
																					
}




















