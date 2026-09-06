#include "fml_ble_adv.h"



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

#include "fml_ble_info_service.h"
#include "log.h"

#define APP_ADV_INTERVAL                MSEC_TO_UNITS(500, UNIT_0_625_MS)           /**< The advertising interval (in units of 0.625 ms. This value corresponds to 750 ms). */
#define APP_ADV_DURATION                6000                                       /**< The advertising duration (60 seconds) in units of 10 milliseconds. */


#define INFO_SERVICE_UUID_TYPE         BLE_UUID_TYPE_VENDOR_BEGIN         // info service服务UUID类型：厂商自定义UUID
#define APP_BLE_CONN_CFG_TAG            1                                 //SoftDevice BLE配置标志

//定义info service服务UUID列表
static ble_uuid_t m_adv_uuids[]          =                                          
{
    {BLE_UUID_INFO_SERVICE_SERVICE, INFO_SERVICE_UUID_TYPE}
};


BLE_ADVERTISING_DEF(ble_advertising);                      //定义名称为m_advertising的广播模块实例


//广播事件处理函数
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;
    //判断广播事件类型
    switch (ble_adv_evt)
    {
        //快速广播启动事件：快速广播启动后会产生该事件
			  case BLE_ADV_EVT_FAST:
            LOG_INFO("Fast advertising.\r\n");
			      //设置广播指示灯为正在广播（D1指示灯闪烁）
//            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
//            APP_ERROR_CHECK(err_code);
            break;
        //广播IDLE事件：广播超时后会产生该事件
        case BLE_ADV_EVT_IDLE:
					  //设置广播指示灯为广播停止（D1指示灯熄灭）
//            err_code = bsp_indication_set(BSP_INDICATE_IDLE);
//            APP_ERROR_CHECK(err_code);
            break;

        default:
            break;
    }
}


//获取广播模式、间隔和超时时间
 void fml_ble_advertising_config_get(ble_adv_modes_config_t * p_config)
{
    memset(p_config, 0, sizeof(ble_adv_modes_config_t));

    p_config->ble_adv_fast_enabled  = true;
    p_config->ble_adv_fast_interval = APP_ADV_INTERVAL;
    p_config->ble_adv_fast_timeout  = APP_ADV_DURATION;
}


 void fml_ble_advertising_config_set(ble_adv_modes_config_t * p_config)
 {
	 ble_advertising_modes_config_set(&ble_advertising, p_config);
 }


//广播初始化
void advertising_init(void)
{
    ret_code_t             err_code;
	  //定义广播初始化配置结构体变量
    ble_advertising_init_t init;
    //配置之前先清零
    memset(&init, 0, sizeof(init));
    //设备名称类型：全称
    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
	  //是否包含外观：包含
    init.advdata.include_appearance      = false;
	  //Flag:一般可发现模式，不支持BR/EDR
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
	  //UUID放到扫描响应里面
	  init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;
	
    //设置广播模式为快速广播
    init.config.ble_adv_fast_enabled  = true;
	  //设置广播间隔和广播持续时间
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    //广播事件回调函数
    init.evt_handler = on_adv_evt;
    //初始化广播
    err_code = ble_advertising_init(&ble_advertising, &init);
    APP_ERROR_CHECK(err_code);
    //设置广播配置标记。APP_BLE_CONN_CFG_TAG是用于跟踪广播配置的标记，这是为未来预留的一个参数，在将来的SoftDevice版本中，
		//可以使用sd_ble_gap_adv_set_configure()配置新的广播配置
		//当前SoftDevice版本（S140 V7.2.0版本）支持的最大广播集数量为1，因此APP_BLE_CONN_CFG_TAG只能写1。
    ble_advertising_conn_cfg_tag_set(&ble_advertising, APP_BLE_CONN_CFG_TAG);
	
	
	
//	    uint32_t               err_code;
//    ble_advertising_init_t init;
//    ble_advdata_manuf_data_t    p_manuf_specific_data;               /**< Manufacturer specific data. */
//    memset(&init, 0, sizeof(init));
//    
//    p_manuf_specific_data.company_identifier = 0xFF01;
//    p_manuf_specific_data.data.size = 6;
//    
//    ble_gap_addr_t p_addr;
//    sd_ble_gap_addr_get(&p_addr);
//    p_manuf_specific_data.data.p_data = p_addr.addr;
//    init.advdata.p_manuf_specific_data  = &p_manuf_specific_data;
//    
//    init.srdata.name_type          = BLE_ADVDATA_FULL_NAME;
//    init.advdata.include_appearance = false;
//    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;//BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

//    init.config.ble_adv_fast_enabled  = true;
//    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
//    init.config.ble_adv_fast_timeout  = 0;//APP_ADV_DURATION;
//    init.evt_handler = on_adv_evt;

//    err_code = ble_advertising_init(&m_advertising, &init);
//    APP_ERROR_CHECK(err_code);

//    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
	
}

//启动广播，该函数所用的模式必须和广播初始化中设置的广播模式一样
 void advertising_start(void)
{
   //使用广播初始化中设置的广播模式启动广播
	 ret_code_t err_code = ble_advertising_start(&ble_advertising, BLE_ADV_MODE_FAST);
	 //检查函数返回的错误代码
   APP_ERROR_CHECK(err_code);
}
