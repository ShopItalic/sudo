#include "bc_ble.h"
#include "bc_watchdog.h"



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

#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
//广播需要引用的头文件
#include "ble_advdata.h"
#include "ble_advertising.h"

#include "nrf_pwr_mgmt.h"

#include "app_timer.h"

#include "app_error.h"
#include "bc_logger.h"
#include "bc_ble_info_service.h"
#include "bc_ble_adv.h"
#include "bc_ble_gap.h"
#include "bc_ble_gatt.h"
#include "bc_ble_dfu.h"

#include "bc_logger.h"

#include "bc_ble_modu_interface.h"
#include "bc_ble_hids_service.h"
#include "bc_device_info.h"
#include "bc_delay.h"
#include "bc_rtos.h"
#include "bc_ble_tx.h"
#include "bc_queue.h"
////#include "bc_watchdog.h"

#include "peer_manager.h"
#include "peer_manager_handler.h"

#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                  0                                           /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS              0                                           /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                        /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size. */

 pm_peer_id_t      m_peer_id;                                                 /**< Device reference handle to the current bonded central. */


#define APP_BLE_OBSERVER_PRIO           3               //应用程序BLE事件监视者优先级，应用程序不能修改该数值
#define APP_BLE_CONN_CFG_TAG            1               //SoftDevice BLE配置标志

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)              // 定义首次调用sd_ble_gap_conn_param_update()函数更新连接参数延迟时间（5秒）
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)             // 定义每次调用sd_ble_gap_conn_param_update()函数更新连接参数的间隔时间（30秒）

#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

//#define APP_ADV_INTERVAL                MSEC_TO_UNITS(500, UNIT_0_625_MS)           /**< The advertising interval (in units of 0.625 ms. This value corresponds to 750 ms). */
//#define APP_ADV_DURATION                6000                                       /**< The advertising duration (60 seconds) in units of 10 milliseconds. */


NRF_BLE_QWR_DEF(m_qwr);                                  //定义一个名称为m_qwr的排队写入实例
BLE_INFO_SERVICE_DEF(m_info_service, NRF_SDH_BLE_TOTAL_LINK_COUNT);    //定义名称为info service服务实例

//该变量用于保存连接句柄，初始值设置为无连接
uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; 
//static bool info_service_enabled = false;


static volatile bool ble_connect_status = false;
static volatile uint32_t tx_session = 0;
static volatile bool tx_failed = false;
static SemaphoreHandle_t tx_event = NULL;
static bool pm_connect_status = false;
#if defined(HANDWARE_1_23_4)
static bool m_conn_rejected = false;
static bc_ble_connect_guard_callback m_ble_connect_guard = NULL;
#endif

uint32_t bc_ble_session_id(void)
{
    return tx_session;
}

void bc_ble_tx_wake(void)
{
    if (tx_event == NULL)
        return;
    if (__get_IPSR() != 0)
    {
        BaseType_t wake = pdFALSE;
        (void)xSemaphoreGiveFromISR(tx_event, &wake);
        portYIELD_FROM_ISR(wake);
    }
    else
        (void)xSemaphoreGive(tx_event);
}
//static void fml_ble_recv_callback(uint8_t *recv_data,uint8_t recv_length)
//{
//	LOG_HEX("ble-recv:",recv_data,recv_length);
//	fml_ble_send(recv_data,recv_length);
//}

//断开当前连接，设备准备进入bootloader之前，需要先断开连接
static void disconnect(uint16_t conn_handle, void * p_context)
{
    UNUSED_PARAMETER(p_context);
    //断开当前连接
    ret_code_t err_code = sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (err_code != NRF_SUCCESS)
    {
       BC_LOG_WARN("Failed to disconnect connection. Connection handle: %d Error: %d\r\n", conn_handle, err_code);
    }
    else
    {
        BC_LOG_DEBUG("Disconnected connection handle %d\r\n", conn_handle);
    }
}

//DFU事件处理函数。如果需要在DFU事件中执行操作，可以在相应的事件里面加入处理代码
static void ble_dfu_evt_handler(ble_dfu_buttonless_evt_type_t event)
{
    switch (event)
    {
        case BLE_DFU_EVT_BOOTLOADER_ENTER_PREPARE://该事件指示设备正在准备进入bootloader
        {
            BC_LOG_INFO("Device is preparing to enter bootloader mode.\r\n");

            //阻止设备在断开连接时广播
            ble_adv_modes_config_t config;
            fml_ble_advertising_config_get(&config);
					  //连接断开后设备不自动进行广播
            config.ble_adv_on_disconnect_disabled = true;
					  //修改广播配置
            fml_ble_advertising_config_set(&config);

					  //断开当前已经连接的所有其他绑定设备。在设备固件更新成功（或中止）后，需要在启动时接收服务更改指示
            uint32_t conn_count = ble_conn_state_for_each_connected(disconnect, NULL);
            BC_LOG_INFO("Disconnected %d links.", conn_count);
            break;
        }

        case BLE_DFU_EVT_BOOTLOADER_ENTER://该事件指示函数返回后设备即进入bootloader
            //如果应用程序有数据需要保存到Flash，通过app_shutdown_handler返回flase以延迟复位，从而保证数据正确写入到Flash
            BC_LOG_INFO("Device will enter bootloader mode.");
            break;

        case BLE_DFU_EVT_BOOTLOADER_ENTER_FAILED://该事件指示进入bootloader失败
            BC_LOG_ERROR("Request to enter bootloader mode failed asynchroneously.\r\n");
				    //进入bootloader失败，应用程序需要采取纠正措施来处理问题，如使用APP_ERROR_CHECK复位设备
            break;

        case BLE_DFU_EVT_RESPONSE_SEND_ERROR://该事件指示发送响应失败
            BC_LOG_ERROR("Request to send a response to client failed.\r\n");
				    //发送响应失败，应用程序需要采取纠正措施来处理问题，如使用APP_ERROR_CHECK复位设备
            APP_ERROR_CHECK(false);
            break;

        default:
            BC_LOG_ERROR("Unknown event from ble_dfu_buttonless.\r\n");
            break;
    }
}

//info service事件回调函数，服务初始化时注册
static void info_service_data_handler(ble_info_service_evt_t * p_evt)
{
  switch(p_evt->type)
  {
    //通知使能后
    case BLE_NUS_EVT_COMM_STARTED:
    {
      bc_ble_tx_wake();
      break;
    }
   //通知关闭后，
    case BLE_NUS_EVT_COMM_STOPPED:
    {
      bc_ble_tx_wake();
      break;
    }
     //判断事件类型:接收到新数据事件
    case BLE_INFO_SERVICE_EVT_RX_DATA:
    {
      struct bc_ble_calss  ble_calss = bc_ble_new();
      ble_calss.ble_recv((uint8_t*)p_evt->params.rx_data.p_data,p_evt->params.rx_data.length);
      break;
    }
    //判断事件类型:发送就绪事件，该事件在后面的试验会用到，当前我们在该事件中翻转指示灯D4的状态，指示该事件的产生
    case BLE_INFO_SERVICE_EVT_TX_RDY:
    {
//      ret_code_t         err_code;
//      do
//      {
//        err_code = ble_info_service_data_send(&m_info_service, respose_data, &respose_len, m_conn_handle);
//        if ( (err_code != NRF_ERROR_INVALID_STATE) && (err_code != NRF_ERROR_RESOURCES) &&
//           (err_code != NRF_ERROR_NOT_FOUND) )
//        {
//                APP_ERROR_CHECK(err_code);
//        }
//      } while (err_code != NRF_SUCCESS);
      break;
    }
  }
}

//排队写入事件处理函数，用于处理排队写入模块的错误
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    //检查错误代码
	  APP_ERROR_HANDLER(nrf_error);
}

//服务初始化，包含初始化排队写入模块和初始化应用程序使用的服务
static void services_init(void)
{
    ret_code_t         err_code;
	  //定义info service初始化结构体
	  ble_info_service_init_t     info_service_init;
	  //定义排队写入初始化结构体变量
    nrf_ble_qwr_init_t qwr_init = {0};
	ble_dfu_buttonless_init_t dfus_init = {0};

    //排队写入事件处理函数
    qwr_init.error_handler = nrf_qwr_error_handler;
    //初始化排队写入模块
    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
		//检查函数返回值
    APP_ERROR_CHECK(err_code);
    
		
		/*------------------以下代码初始info_service化服务-------------*/
		//清零info service服务初始化结构体
	memset(&info_service_init, 0, sizeof(info_service_init));
		//设置info service事件回调函数
    info_service_init.data_handler = info_service_data_handler;
    //初始化info service服务
    err_code = ble_info_service_init(&m_info_service, &info_service_init);
    APP_ERROR_CHECK(err_code);
		/*------------------初始化info service服务-END-----------------*/
		
		//初始化DFU服务
		dfus_init.evt_handler = ble_dfu_evt_handler;

    err_code = ble_dfu_buttonless_init(&dfus_init);
    APP_ERROR_CHECK(err_code);
	
	bc_device_hid_info *hid_info;
	hid_info = bc_device_info_get_hid_info();
//	if(hid_info->device_hid_type == 1 && (hid_info->device_hid_gesture_mode != 0xFF || hid_info->device_hid_touch_mode != 0xFF))
    if(hid_info->device_hid_type == 1)
	{
		hids_init(&m_conn_handle);
	}
}


//BLE事件处理函数
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code = NRF_SUCCESS;
	 struct bc_ble_calss  ble_calss = bc_ble_new();
    if (p_ble_evt->header.evt_id != BLE_GATTS_EVT_HVN_TX_COMPLETE)
        BC_LOG_INFO("BLE event: %04x\r\n", p_ble_evt->header.evt_id);
    //判断BLE事件类型，根据事件类型执行相应操作
    switch (p_ble_evt->header.evt_id)
    {
        //断开连接事件
			  case BLE_GAP_EVT_DISCONNECTED:
        {
#if defined(HANDWARE_1_23_4)
            // 被守卫拒绝的连接断开，不触发任何回调
            if(m_conn_rejected)
            {
                m_conn_rejected = false;
                m_conn_handle = BLE_CONN_HANDLE_INVALID;
            ++tx_session;
            bc_ble_gatt_reset();
            bc_ble_tx_wake();
                BC_LOG_INFO("Rejected connection disconnected.\r\n");
                advertising_start();
                break;
            }
#endif
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
			ble_connect_status = false;	 
			pm_connect_status = false;			  
            
            if(ble_calss.ble_disconnect_callback != NULL)
			{
				ble_calss.ble_disconnect_callback();
			}
			
			if(ble_calss.ble_pm_disconnect_callback != NULL)	
			{
				ble_calss.ble_pm_disconnect_callback();
			}	
				    //打印提示信息
		    BC_LOG_INFO("Disconnected.  handle:%04x,reason:%04x  \r\n",p_ble_evt->evt.gap_evt.conn_handle,p_ble_evt->evt.gap_evt.params.disconnected.reason);
			uint8_t reason = p_ble_evt->evt.gap_evt.params.disconnected.reason;
            switch (reason) {
                case BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION:
                    BC_LOG_INFO("Remote device terminated the connection.");
                    break;
                case BLE_HCI_CONN_INTERVAL_UNACCEPTABLE:
                    BC_LOG_INFO("Connection interval unacceptable.");
                    break;
//                case BLE_HCI_CONN_TIMEOUT:
//                    BC_LOG_INFO("Connection timeout.");
//                    break;
                case BLE_HCI_LOCAL_HOST_TERMINATED_CONNECTION:
                    BC_LOG_INFO("Local host terminated the connection.");
                    break;
                default:
                    BC_LOG_INFO("Disconnected for reason: 0x%02X", reason);
                    break;
					}
            // 蓝牙断开后重新启动广播，使设备可以被再次搜索到
            advertising_start(); // add by liukun 20260416 for after disconnecting, restart the broadcast
        }
                    break;
				
        //连接事件
        case BLE_GAP_EVT_CONNECTED:
        {
            BC_LOG_INFO("Connected.\r\n");

#if defined(HANDWARE_1_23_4)
            // 检查连接守卫（如离线录音中禁止连接）
            if(m_ble_connect_guard != NULL && m_ble_connect_guard())
            {
                BC_LOG_INFO("Connection rejected by guard (offline recording)\r\n");
                ret_code_t err = sd_ble_gap_disconnect(p_ble_evt->evt.gap_evt.conn_handle,
                                                        BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
                if(err != NRF_SUCCESS)
                {
                    BC_LOG_WARN("sd_ble_gap_disconnect on rejected conn failed: %d\r\n", err);
                }
                m_conn_rejected = true;
                break;
            }
#endif

				    //保存连接句柄
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            ++tx_session;
            tx_failed = false;
            bc_ble_gatt_reset();
            bc_ble_tx_wake();
				    //将连接句柄分配给排队写入实例，分配后排队写入实例和该连接关联，这样，当有多个连接的时候，通过关联不同的排队写入实例，很方便单独处理各个连接
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
#if ( HARDWARE_BCL601_151_ENABLED == 1 || HARDWARE_153_ENABLED == 1)	
            err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, p_ble_evt->evt.gap_evt.conn_handle, RADIO_TXPOWER_TXPOWER_Pos8dBm);
            APP_ERROR_CHECK(err_code);
#elif ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	

            err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, p_ble_evt->evt.gap_evt.conn_handle, RADIO_TXPOWER_TXPOWER_Pos4dBm);
            APP_ERROR_CHECK(err_code);		

#endif		
			
            ble_connect_status = true;
                  if(ble_calss.ble_connect_callback != NULL)
            {
              ble_calss.ble_connect_callback();
            }
            } break;
				
        //PHY更新事件
        case BLE_GATTS_EVT_HVN_TX_COMPLETE:
            bc_ble_tx_wake();
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
#if defined(HANDWARE_1_23_4)
            // 被守卫拒绝的连接，跳过PHY更新
            if(m_conn_rejected)
            {
                break;
            }
#endif
            BC_LOG_DEBUG("PHY update request.\r\n");
            ble_gap_phys_t const phys =
            {
                //.rx_phys = BLE_GAP_PHY_AUTO,
//                .tx_phys = BLE_GAP_PHY_AUTO,
              .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
						//响应PHY更新规程
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;
				//安全参数请求事件
		case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
		{
#if defined(HANDWARE_1_23_4)
			// 被守卫拒绝的连接，跳过安全参数回复
			if(m_conn_rejected)
			{
				break;
			}
#endif
			bc_device_hid_info *hid_info;
			hid_info = bc_device_info_get_hid_info();
//			if(hid_info->device_hid_type == 1 && (hid_info->device_hid_gesture_mode != 0xFF || hid_info->device_hid_touch_mode != 0xFF))	
			if(hid_info->device_hid_type == 1)
			{
			}
			else
			{			
				//不支持配对
				err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
				APP_ERROR_CHECK(err_code);
			}			
//            //不支持配对
//            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
//            APP_ERROR_CHECK(err_code);
            /* HID pairing is answered by Peer Manager. Never clear GATT
             * system attributes in response to a GAP security request. */
            break;
		} 
				//系统属性访问正在等待中
		case BLE_GATTS_EVT_SYS_ATTR_MISSING:
#if defined(HANDWARE_1_23_4)
            // 被守卫拒绝的连接，跳过系统属性设置
            if(m_conn_rejected)
            {
                break;
            }
#endif
            //系统属性没有存储，更新系统属性
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;
		
        //GATT客户端超时事件
        case BLE_GATTC_EVT_TIMEOUT:
            BC_LOG_DEBUG("GATT Client Timeout.!!!!!!!!!!!!!!!!!!! \r\n");
				    //断开当前连接
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
				
        //GATT服务器超时事件
        case BLE_GATTS_EVT_TIMEOUT:
            BC_LOG_DEBUG("GATT Server Timeout.!!!!!!!!!!!!!!!!!!!\r\n");
				    //断开当前连接
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_GAP_EVT_TIMEOUT:
		{
			BC_LOG_DEBUG("BLE_GAP_EVT_TIMEOUT.!!!!!!!!!!!!!!!!!!!\r\n");
			break;
		}
        default:
            break;
    }
}




//初始化BLE协议栈
static void ble_stack_init(void)
{
    ret_code_t err_code;
    //请求使能SoftDevice，该函数中会根据sdk_config.h文件中低频时钟的设置来配置低频时钟
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);
    
    //定义保存应用程序RAM起始地址的变量
    uint32_t ram_start = 0;
	  //使用sdk_config.h文件的默认参数配置协议栈，获取应用程序RAM起始地址，保存到变量ram_start
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    //使能BLE协议栈
#if defined(SUDO_VOICE_ONLY)
    {
        /* Request a useful notification window without borrowing app RAM.
         * sd_ble_enable writes its required RAM base back through the pointer;
         * always restore the LINKER base before a retry. */
        const uint32_t linker_ram_start = ram_start;
        ble_cfg_t config = {0};
        config.conn_cfg.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;
        config.conn_cfg.params.gatts_conn_cfg.hvn_tx_queue_size = 6;
        err_code = sd_ble_cfg_set(BLE_CONN_CFG_GATTS, &config, linker_ram_start);
        if (err_code == NRF_SUCCESS)
            err_code = nrf_sdh_ble_enable(&ram_start);
        if (err_code == NRF_ERROR_NO_MEM)
        {
            config.conn_cfg.params.gatts_conn_cfg.hvn_tx_queue_size = 1;
            err_code = sd_ble_cfg_set(BLE_CONN_CFG_GATTS, &config, linker_ram_start);
            APP_ERROR_CHECK(err_code);
            ram_start = linker_ram_start;
            err_code = nrf_sdh_ble_enable(&ram_start);
            BC_LOG_WARN("BLE TX window: 1 (RAM fallback)\r\n");
        }
        else if (err_code == NRF_SUCCESS)
            BC_LOG_INFO("BLE TX window: 6\r\n");
    }
#else
    err_code = nrf_sdh_ble_enable(&ram_start);
#endif
    APP_ERROR_CHECK(err_code);

    //注册BLE事件回调函数
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

//连接参数协商模块事件处理函数
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;
    //判断事件类型，根据事件类型执行动作
	  //连接参数协商失败，断开当前连接
    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
		//连接参数协商成功
		if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_SUCCEEDED)
    {
       //功能代码;
    }
}

//连接参数协商模块错误处理事件，参数nrf_error包含了错误代码，通过nrf_error可以分析错误信息
static void conn_params_error_handler(uint32_t nrf_error)
{
    //检查错误代码
	  APP_ERROR_HANDLER(nrf_error);
}

//连接参数协商模块初始化
static void conn_params_init(void)
{
    ret_code_t             err_code;
	  //定义连接参数协商模块初始化结构体
    ble_conn_params_init_t cp_init;
    //配置之前先清零
    memset(&cp_init, 0, sizeof(cp_init));
    //设置为NULL，从主机获取连接参数
    cp_init.p_conn_params                  = NULL;
	  //连接或启动通知到首次发起连接参数更新请求之间的时间设置为5秒
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
	  //每次调用sd_ble_gap_conn_param_update()函数发起连接参数更新请求的之间的间隔时间设置为：30秒
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
	  //放弃连接参数协商前尝试连接参数协商的最大次数设置为：3次
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
	  //连接参数更新从连接事件开始计时
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
	  //连接参数更新失败不断开连接
    cp_init.disconnect_on_fail             = false;
	  //注册连接参数更新事件句柄
    cp_init.evt_handler                    = on_conn_params_evt;
	  //注册连接参数更新错误事件句柄
    cp_init.error_handler                  = conn_params_error_handler;
    //调用库函数（以连接参数更新初始化结构体为输入参数）初始化连接参数协商模块
    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}




/**@brief Function for setting filtered whitelist.
 *
 * @param[in] skip  Filter passed to @ref pm_peer_id_list.
 */
static void whitelist_set(pm_peer_id_list_skip_t skip)
{
    pm_peer_id_t peer_ids[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
    uint32_t     peer_id_count = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

    ret_code_t err_code = pm_peer_id_list(peer_ids, &peer_id_count, PM_PEER_ID_INVALID, skip);
    APP_ERROR_CHECK(err_code);

    BC_LOG_INFO("\tm_whitelist_peer_cnt %d, MAX_PEERS_WLIST %d",
                   peer_id_count + 1,
                   BLE_GAP_WHITELIST_ADDR_MAX_COUNT);

    err_code = pm_whitelist_set(peer_ids, peer_id_count);
    APP_ERROR_CHECK(err_code);
}

//删除Flash中存储的绑定信息
static void delete_bonds(void)
{
    ret_code_t err_code;

    BC_LOG_INFO("Erase bonds!");
    //删除Flash中存储的绑定信息
    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}
//启动广播，该函数所用的模式必须和广播初始化中设置的广播模式一样
static void peer_advertising_start(bool erase_bonds)
{
    if (erase_bonds == true)
    {
        //删除Flash中存储的配对信息，执行完删除操作后，会产生PM_EVT_PEERS_DELETE_SUCCEEDED事件，在该事件下会启动广播
			  delete_bonds();
    }
    else
    {
//		 delete_bonds();
        //使用广播初始化中设置的广播模式启动广播
		advertising_start();
    }
}

//配对管理器事件处理函数
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    //打印日志，连接已绑定设备时启动加密，错误处理
	  pm_handler_on_pm_evt(p_evt);
	  //理配对设备在Flash中的保存
    pm_handler_flash_clean(p_evt);
	struct bc_ble_calss  ble_calss = bc_ble_new();
    switch (p_evt->evt_id)
    {
        case PM_EVT_CONN_SEC_SUCCEEDED:
		{
            m_peer_id = p_evt->peer_id;
			pm_connect_status = true;
			if(ble_calss.ble_pm_connect_callback != NULL)
			{
				ble_calss.ble_pm_connect_callback();
			}
			
            break;
		}

        case PM_EVT_PEERS_DELETE_SUCCEEDED:  //存储的绑定信息已成功删除
			 //若程序启动时执行了删除绑定信息操作，在该事件下启动广播
		BC_LOG_INFO("perr delete id:%d \r\n",p_evt->peer_id);
		//    pm_peer_delete(p_evt->peer_id);
            peer_advertising_start(false);
            break;

        case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
            if (     p_evt->params.peer_data_update_succeeded.flash_changed
                 && (p_evt->params.peer_data_update_succeeded.data_id == PM_PEER_DATA_ID_BONDING))
            {
                BC_LOG_INFO("New Bond, add the peer to the whitelist if possible");
                // Note: You should check on what kind of white list policy your application should use.

                whitelist_set(PM_PEER_ID_LIST_SKIP_NO_ID_ADDR);
            }
            break;
		case PM_EVT_CONN_SEC_CONFIG_REQ:

        {

            BC_LOG_INFO("PM_EVT_CONN_SEC_CONFIG_REQ: peer_id=%d, accept to fix bonding\r\n",

                           p_evt->peer_id);

            // Accept pairing request from an already bonded peer.

            pm_conn_sec_config_t conn_sec_config = {.allow_repairing = true};

            pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);

        } break;
        default:
            break;
    }
	BC_LOG_INFO("perr  id:%d \r\n",p_evt->evt_id);
}

//配对管理器初始化
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;
    //初始化配对管理器软件库
    err_code = pm_init();
    APP_ERROR_CHECK(err_code);
    //配置安全参数之前先清零sec_param
    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    //初始化安全性参数结构体
    sec_param.bond           = SEC_PARAM_BOND;              //支持绑定
    sec_param.mitm           = SEC_PARAM_MITM;              //无MITM保护
    sec_param.lesc           = SEC_PARAM_LESC;              //不支持安全连接配对，即使用传统配对
    sec_param.keypress       = SEC_PARAM_KEYPRESS;          //无按键通知
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;   //无IO能力
    sec_param.oob            = SEC_PARAM_OOB;               //不支持OOB
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;      //最小加密密钥大小：7字节
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;      //最大加密密钥大小：16字节
    sec_param.kdist_own.enc  = 1;            //分发本地LTK
    sec_param.kdist_own.id   = 1;            //分发本地IRK
    sec_param.kdist_peer.enc = 1;            //要求对方分发LTK
    sec_param.kdist_peer.id  = 1;            //要求对方分发IRK
    //配置安全参数
    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);
    //向配对管理器注册事件句柄
    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}



	

/* The TX task is the sole caller. Stack acceptance is not a phone ACK. */
static uint32_t tx_now(void *context)
{
    (void)context;
    return (uint32_t)xTaskGetTickCount();
}

static bool tx_current(void *context, uint32_t session)
{
    (void)context;
    return ble_connect_status && !tx_failed && tx_session == session;
}

static void tx_wait(void *context, uint32_t ticks)
{
    (void)context;
    (void)xSemaphoreTake(tx_event, (TickType_t)ticks);
}

static bc_ble_tx_attempt_result tx_attempt(void *context, const uint8_t *data,
                                         uint16_t length)
{
    uint32_t error;
    uint16_t accepted_length = length;
    uint32_t session = *(const uint32_t *)context;
    /* Keep the epoch/handle check and SVC together across RTOS switches. */
    taskENTER_CRITICAL();
    if (!tx_current(NULL, session) || length > bc_ble_payload_limit())
        error = NRF_ERROR_INVALID_STATE;
    else
        error = ble_info_service_data_send(&m_info_service, (uint8_t *)data,
                                            &accepted_length, m_conn_handle);
    taskEXIT_CRITICAL();
    if (error == NRF_SUCCESS)
        return accepted_length == length ? BC_BLE_TX_ATTEMPT_ACCEPTED
                                         : BC_BLE_TX_ATTEMPT_FATAL;
    if (error == NRF_ERROR_RESOURCES || error == NRF_ERROR_INVALID_STATE ||
        error == BLE_ERROR_GATTS_SYS_ATTR_MISSING || error == NRF_ERROR_BUSY)
        return BC_BLE_TX_ATTEMPT_RETRY;
    BC_LOG_WARN("BLE TX failed: %lu\r\n", (unsigned long)error);
    return BC_BLE_TX_ATTEMPT_FATAL;
}

void bc_ble_send_session(uint8_t *data, uint16_t length, uint32_t session)
{
    bc_ble_tx_port port = { &session, tx_attempt, tx_now, tx_wait, tx_current };
    bc_ble_tx_result result;
    if (tx_event == NULL)
        return;
    result = bc_ble_tx_write(&port, data, length, session,
                              pdMS_TO_TICKS(10000), pdMS_TO_TICKS(100));
    if (result != BC_BLE_TX_ACCEPTED && result != BC_BLE_TX_CANCELLED)
    {
        /* Abort this link so queued EOF cannot follow a lost audio packet.
         * Stored recordings remain available for a fresh sync after reconnect. */
        taskENTER_CRITICAL();
        if (tx_current(NULL, session))
        {
            tx_failed = true;
            (void)sd_ble_gap_disconnect(m_conn_handle,
                                        BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        }
        taskEXIT_CRITICAL();
        BC_LOG_WARN("BLE TX aborted: %u\r\n", (unsigned int)result);
    }
}

void bc_ble_send(uint8_t *send_data, uint16_t send_length)
{
    struct bc_ble_data_package packet = {0};
    if (!send_data || !send_length || send_length > BC_BLE_TX_MAX_LENGTH ||
        !bc_ble_connect_status())
        return;
    memcpy(packet.data, send_data, send_length);
    packet.data_length = send_length;
    if (__get_IPSR() != 0)
        (void)bc_queue_isr_enqueue(BC_QUEUE_TYPE_BLE_SEND, &packet);
    else
        (void)bc_queue_ble_send(&packet, bc_ble_session_id(), pdMS_TO_TICKS(100));
}


bool bc_ble_connect_status(void)
{
	return ble_connect_status && !tx_failed;
}

bool bc_ble_pm_connect_status(void)
{
	return pm_connect_status;
}

void bc_ble_disconnect(void)
{
	//断开当前连接
    ret_code_t err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (err_code != NRF_SUCCESS)
    {
       BC_LOG_WARN("Failed to disconnect connection. Connection handle: %d Error: %d\r\n", m_conn_handle, err_code);
    }
    else
    {
        BC_LOG_DEBUG("Disconnected connection handle %d\r\n", m_conn_handle);
    }
}

#if defined(HANDWARE_1_23_4)
void bc_ble_connect_guard_register(void *guard)
{
    m_ble_connect_guard = (bc_ble_connect_guard_callback)guard;
}
#endif

void bc_ble_mac_get(uint8_t *ble_mac)
{
	BC_LOG_INFO("device MAC:  %02x:%02x:%02x:%02x:%02x:%02x \r\n",((NRF_FICR->DEVICEADDR[1] >> 8) & 0xFF) | 0xc0,
	                                                         (NRF_FICR->DEVICEADDR[1] >> 0) & 0xFF,
													    	 (NRF_FICR->DEVICEADDR[0] >> 24) & 0xFF,
															 (NRF_FICR->DEVICEADDR[0] >> 16) & 0xFF,
															 (NRF_FICR->DEVICEADDR[0] >> 8) & 0xFF,
															 (NRF_FICR->DEVICEADDR[0] >> 0) & 0xFF
	                                                       );	
	ble_mac[0] = ((NRF_FICR->DEVICEADDR[1] >> 8) & 0xFF) | 0xc0;
	ble_mac[1] = (NRF_FICR->DEVICEADDR[1] >> 0) & 0xFF;
	ble_mac[2] = (NRF_FICR->DEVICEADDR[0] >> 24) & 0xFF;
	ble_mac[3] = (NRF_FICR->DEVICEADDR[0] >> 16) & 0xFF;
	ble_mac[4] = (NRF_FICR->DEVICEADDR[0] >> 8) & 0xFF;
	ble_mac[5] = (NRF_FICR->DEVICEADDR[0] >> 0) & 0xFF;
	
}

void bc_ble_mac_set(uint8_t *ble_mac)
{
  uint32_t err_code;
  static ble_gap_addr_t  mac_addr;
  memcpy(mac_addr.addr,ble_mac,6);
  mac_addr.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;//地址类型设置为随机静态地址
	//写入地址
	err_code = sd_ble_gap_addr_set(&mac_addr);//
	if(err_code != NRF_SUCCESS)
	{
    BC_LOG_HEX_P("set mac fail:",mac_addr.addr,6);
  }
  else
  {
    BC_LOG_HEX_P("set mac ok:",mac_addr.addr,6);
  }
  
}

void bc_ble_init(void)
{
    tx_event = xSemaphoreCreateBinary();
    if (tx_event == NULL)
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
	bc_device_hid_info *hid_info;
	hid_info = bc_device_info_get_hid_info();
#if APP_CHECK_BOOT_VALID	
	ble_dfu_svc_init();
#endif
	//初始化协议栈
	ble_stack_init();
	
	sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
	//配置GAP参数
	gap_params_init();
	//初始化GATT
	gatt_init();
	//初始化服务
	services_init();
	//初始化广播
	advertising_init();	
	//连接参数协商初始化
    conn_params_init();
	
//	if(hid_info->device_hid_type == 1 && (hid_info->device_hid_gesture_mode != 0xFF || hid_info->device_hid_touch_mode != 0xFF))
	if(hid_info->device_hid_type == 1)
	{
		//配对管理器初始化
		peer_manager_init();
	}
	//启动广播
	peer_advertising_start(false);


}



