#include "fml_ble.h"



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
#include "log.h"
#include "fml_ble_info_service.h"
#include "fml_ble_adv.h"
#include "fml_ble_gap.h"
#include "fml_ble_gatt.h"


#define APP_BLE_OBSERVER_PRIO           3               //应用程序BLE事件监视者优先级，应用程序不能修改该数值
#define APP_BLE_CONN_CFG_TAG            1               //SoftDevice BLE配置标志

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)              // 定义首次调用sd_ble_gap_conn_param_update()函数更新连接参数延迟时间（5秒）
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)             // 定义每次调用sd_ble_gap_conn_param_update()函数更新连接参数的间隔时间（30秒）

#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

//#define APP_ADV_INTERVAL                MSEC_TO_UNITS(500, UNIT_0_625_MS)           /**< The advertising interval (in units of 0.625 ms. This value corresponds to 750 ms). */
//#define APP_ADV_DURATION                6000                                       /**< The advertising duration (60 seconds) in units of 10 milliseconds. */


NRF_BLE_QWR_DEF(m_qwr);                                  //定义一个名称为m_qwr的排队写入实例
BLE_INFO_SERVICE_DEF(m_info_service, NRF_SDH_BLE_TOTAL_LINK_COUNT);    //定义名称为m_uarts的串口透传服务实例

//该变量用于保存连接句柄，初始值设置为无连接
uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; 
//static bool info_service_enabled = false;


//断开当前连接，设备准备进入bootloader之前，需要先断开连接
static void disconnect(uint16_t conn_handle, void * p_context)
{
    UNUSED_PARAMETER(p_context);
    //断开当前连接
    ret_code_t err_code = sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_WARNING("Failed to disconnect connection. Connection handle: %d Error: %d\r\n", conn_handle, err_code);
    }
    else
    {
        LOG_DEBUG("Disconnected connection handle %d\r\n", conn_handle);
    }
}

//DFU事件处理函数。如果需要在DFU事件中执行操作，可以在相应的事件里面加入处理代码
static void ble_dfu_evt_handler(ble_dfu_buttonless_evt_type_t event)
{
    switch (event)
    {
        case BLE_DFU_EVT_BOOTLOADER_ENTER_PREPARE://该事件指示设备正在准备进入bootloader
        {
            LOG_INFO("Device is preparing to enter bootloader mode.\r\n");

            //阻止设备在断开连接时广播
            ble_adv_modes_config_t config;
            fml_ble_advertising_config_get(&config);
					  //连接断开后设备不自动进行广播
            config.ble_adv_on_disconnect_disabled = true;
					  //修改广播配置
            fml_ble_advertising_config_set(&config);

					  //断开当前已经连接的所有其他绑定设备。在设备固件更新成功（或中止）后，需要在启动时接收服务更改指示
            uint32_t conn_count = ble_conn_state_for_each_connected(disconnect, NULL);
            LOG_INFO("Disconnected %d links.", conn_count);
            break;
        }

        case BLE_DFU_EVT_BOOTLOADER_ENTER://该事件指示函数返回后设备即进入bootloader
            //如果应用程序有数据需要保存到Flash，通过app_shutdown_handler返回flase以延迟复位，从而保证数据正确写入到Flash
            LOG_INFO("Device will enter bootloader mode.");
            break;

        case BLE_DFU_EVT_BOOTLOADER_ENTER_FAILED://该事件指示进入bootloader失败
            LOG_ERROR("Request to enter bootloader mode failed asynchroneously.\r\n");
				    //进入bootloader失败，应用程序需要采取纠正措施来处理问题，如使用APP_ERROR_CHECK复位设备
            break;

        case BLE_DFU_EVT_RESPONSE_SEND_ERROR://该事件指示发送响应失败
            LOG_ERROR("Request to send a response to client failed.\r\n");
				    //发送响应失败，应用程序需要采取纠正措施来处理问题，如使用APP_ERROR_CHECK复位设备
            APP_ERROR_CHECK(false);
            break;

        default:
            LOG_ERROR("Unknown event from ble_dfu_buttonless.\r\n");
            break;
    }
}

//info service事件回调函数，串口透出服务初始化时注册
static void info_service_data_handler(ble_info_service_evt_t * p_evt)
{
	  //通知使能后
	  if (p_evt->type == BLE_NUS_EVT_COMM_STARTED)
		{
	
		}
		//通知关闭后，
		else if(p_evt->type == BLE_NUS_EVT_COMM_STOPPED)
		{

		}
	  //判断事件类型:接收到新数据事件
    if (p_evt->type == BLE_INFO_SERVICE_EVT_RX_DATA)
    {

    }
		//判断事件类型:发送就绪事件，该事件在后面的试验会用到，当前我们在该事件中翻转指示灯D4的状态，指示该事件的产生
    if (p_evt->type == BLE_INFO_SERVICE_EVT_TX_RDY)
    {

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
    
		
		/*------------------以下代码初始化串口透传服务-------------*/
		//清零info service服务初始化结构体
	memset(&info_service_init, 0, sizeof(info_service_init));
		//设置info service事件回调函数
    info_service_init.data_handler = info_service_data_handler;
    //初始化串口透传服务
    err_code = ble_info_service_init(&m_info_service, &info_service_init);
    APP_ERROR_CHECK(err_code);
		/*------------------初始化info service服务-END-----------------*/
		
		//初始化DFU服务
		dfus_init.evt_handler = ble_dfu_evt_handler;

    err_code = ble_dfu_buttonless_init(&dfus_init);
    APP_ERROR_CHECK(err_code);
}


//BLE事件处理函数
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code = NRF_SUCCESS;
    //判断BLE事件类型，根据事件类型执行相应操作
    switch (p_ble_evt->header.evt_id)
    {
        //断开连接事件
			  case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
				    
				    //打印提示信息
				    LOG_INFO("Disconnected.\r\n");
            break;
				
        //连接事件
        case BLE_GAP_EVT_CONNECTED:
            LOG_INFO("Connected.\r\n");
				    //设置指示灯状态为连接状态，即指示灯D1常亮

				    //保存连接句柄
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
				    //将连接句柄分配给排队写入实例，分配后排队写入实例和该连接关联，这样，当有多个连接的时候，通过关联不同的排队写入实例，很方便单独处理各个连接
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            break;
				
        //PHY更新事件
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            LOG_DEBUG("PHY update request.\r\n");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
						//响应PHY更新规程
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;
				//安全参数请求事件
				case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            //不支持配对
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
				 
				//系统属性访问正在等待中
				case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            //系统属性没有存储，更新系统属性
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;
        //GATT客户端超时事件
        case BLE_GATTC_EVT_TIMEOUT:
            LOG_DEBUG("GATT Client Timeout. \r\n");
				    //断开当前连接
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
				
        //GATT服务器超时事件
        case BLE_GATTS_EVT_TIMEOUT:
            LOG_DEBUG("GATT Server Timeout.\r\n");
				    //断开当前连接
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

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
    err_code = nrf_sdh_ble_enable(&ram_start);
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



//初始化电源管理模块
static void power_management_init(void)
{
    ret_code_t err_code;
	  //初始化电源管理
    err_code = nrf_pwr_mgmt_init();
	  //检查函数返回的错误代码
    APP_ERROR_CHECK(err_code);
}

//初始化APP定时器模块
static void timers_init(void)
{
    //初始化APP定时器模块
    ret_code_t err_code = app_timer_init();
	  //检查返回值
    APP_ERROR_CHECK(err_code);

    //加入创建用户定时任务的代码，创建用户定时任务。 

}


void fml_ble_init(void)
{
	//初始化协议栈
	ble_stack_init();
//	timers_init();
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
//	power_management_init();
	advertising_start();
}






