#include "fml_ble_gatt.h"


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
#include "fml_ble_info_service.h"

extern uint16_t m_conn_handle;



NRF_BLE_GATT_DEF(m_gatt);                                //定义名称为m_gatt的GATT模块实例

//发送的最大数据长度
static uint16_t   fml_ble_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3; 


//GATT事件处理函数，该函数中处理MTU交换事件
static void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    //如果是MTU交换事件
	  if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {

			  fml_ble_max_data_len  = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        LOG_INFO("Data len is set to 0x%X(%d)", fml_ble_max_data_len , fml_ble_max_data_len );
    }
    LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
}

//初始化GATT程序模块
void gatt_init(void)
{
    //初始化GATT程序模块
	ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
	  //检查函数返回的错误代码
    APP_ERROR_CHECK(err_code);
	  //设置ATT MTU的大小,这里设置的值为247
	err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
}










