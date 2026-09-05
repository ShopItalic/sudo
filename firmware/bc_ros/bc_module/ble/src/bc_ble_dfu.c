#include "bc_ble_dfu.h"






//DFU需要引用的头文件
#include "nrf_dfu_ble_svci_bond_sharing.h"
#include "nrf_svci_async_function.h"
#include "nrf_svci_async_handler.h"
#include "nrf_power.h"
#include "ble_dfu.h"
#include "nrf_bootloader_info.h"


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

//电源管理需要引用的头文件
#include "nrf_pwr_mgmt.h"

#include "bc_logger.h"


//关机准备处理程序。在关闭过程中，将以1秒的间隔调用此函数，直到函数返回true。当函数返回true时，表示应用程序已准备好复位为DFU模式
static bool app_shutdown_handler(nrf_pwr_mgmt_evt_t event)
{
    switch (event)
    {
        case NRF_PWR_MGMT_EVT_PREPARE_DFU:
            BC_LOG_INFO("Power management wants to reset to DFU mode.");
            // YOUR_JOB: Get ready to reset into DFU mode
            //
            // If you aren't finished with any ongoing tasks, return "false" to
            // signal to the system that reset is impossible at this stage.
            //
            // Here is an example using a variable to delay resetting the device.
            //
            // if (!m_ready_for_reset)
            // {
            //      return false;
            // }
            // else
            //{
            //
            //    // Device ready to enter
            //    uint32_t err_code;
            //    err_code = sd_softdevice_disable();
            //    APP_ERROR_CHECK(err_code);
            //    err_code = app_timer_stop_all();
            //    APP_ERROR_CHECK(err_code);
            //}
            break;

        default:
            // YOUR_JOB: Implement any of the other events available from the power management module:
            //      -NRF_PWR_MGMT_EVT_PREPARE_SYSOFF
            //      -NRF_PWR_MGMT_EVT_PREPARE_WAKEUP
            //      -NRF_PWR_MGMT_EVT_PREPARE_RESET
            return true;
    }

    BC_LOG_INFO("Power management allowed to reset to DFU mode.");
    return true;
}
//注册优先级为0的应用程序关闭处理程序
NRF_PWR_MGMT_HANDLER_REGISTER(app_shutdown_handler, 0);
//SoftDevice状态监视者事件处理函数
static void buttonless_dfu_sdh_state_observer(nrf_sdh_state_evt_t state, void * p_context)
{
    if (state == NRF_SDH_EVT_STATE_DISABLED)
    {
			  //表明Softdevice在复位之前已经禁用，告之bootloader启动时应跳过CRC
        nrf_power_gpregret2_set(BOOTLOADER_DFU_SKIP_CRC);

        //进入system off模式
        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
    }
}
//注册SoftDevice状态监视者，用于SoftDevice状态改变或者即将改变时接收SoftDevice事件
NRF_SDH_STATE_OBSERVER(m_buttonless_dfu_state_obs, 0) =
{
    .handler = buttonless_dfu_sdh_state_observer,
};



void ble_dfu_svc_init(void)
{
	ret_code_t err_code;
		//使能中断之前，初始化异步SVCI接口到Bootloader
	err_code = ble_dfu_buttonless_async_svci_init();
    APP_ERROR_CHECK(err_code);
}


