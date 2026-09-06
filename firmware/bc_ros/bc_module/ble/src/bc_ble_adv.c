#include "bc_ble_adv.h"



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

#include "bc_ble_info_service.h"
#include "bc_logger.h"

#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "app_touch_button_handler.h"

#define MICROSOFT_VENDOR_ID             0x0006                                      /**< Microsoft Vendor ID.*/
#define MICROSOFT_BEACON_ID             0x03                                        /**< Microsoft Beacon ID, used to indicate that Swift Pair feature is supported. */
#define MICROSOFT_BEACON_SUB_SCENARIO   0x00                                        /**< Microsoft Beacon Sub Scenario, used to indicate how the peripheral will pair using Swift Pair feature. */
#define RESERVED_RSSI_BYTE              0x80                                        /**< Reserved RSSI byte, used to maintain forwards and backwards compatibility. */

#define APP_ADV_FAST_INTERVAL           MSEC_TO_UNITS(500, UNIT_0_625_MS)          /**< Fast advertising interval (in units of 0.625 ms. This value corresponds to 25 ms.). */
#define APP_ADV_FAST_DURATION           (1000*60*1 / 10)          /**< The advertising duration of fast advertising in units of 10 milliseconds. */


#define APP_ADV_SLOW_INTERVAL           MSEC_TO_UNITS(500, UNIT_0_625_MS)          /**< Slow advertising interval (in units of 0.625 ms. This value corresponds to 100 ms.). */
#define APP_ADV_SLOW_DURATION           (1000*60*1 / 10)          /**< The advertising duration of slow advertising in units of 10 milliseconds. */


// 小米快连相关定义
#define XIAOMI_VENDOR_ID                0x038F//0x0157                                      /**< Xiaomi Vendor ID (0x0157 = 343) */
#define XIAOMI_SIG_ID                   0x03                                        /**< Xiaomi SIG ID */
#define XIAOMI_FAST_CONNECT_TYPE        0x01                                        /**< 快连 Type，固定为 0x01 */
#define XIAOMI_Major_ID                 0x21
#define XIAOMI_Minor_ID                 0x2E

//#define USE_XIAOMI



static uint8_t locmac[BLE_GAP_ADDR_LEN] = {0};
static uint8_t m_sp_payload[] =                                                     /**< Payload of advertising data structure for Microsoft Swift Pair feature. */
{
    MICROSOFT_BEACON_ID,
    MICROSOFT_BEACON_SUB_SCENARIO,
    RESERVED_RSSI_BYTE
};
static ble_advdata_manuf_data_t m_sp_manuf_advdata =                                /**< Advertising data structure for Microsoft Swift Pair feature. */
{
    .company_identifier = MICROSOFT_VENDOR_ID,
    .data               =
    {
        .size   = sizeof(m_sp_payload),
        .p_data = &m_sp_payload[0]
    }
};
static ble_advdata_t m_sp_advdata;
extern  pm_peer_id_t      m_peer_id;


#if ( HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1)	

#define APP_ADV_INTERVAL                MSEC_TO_UNITS(1000, UNIT_0_625_MS)           /**< The advertising interval (in units of 0.625 ms. This value corresponds to 750 ms). */

#else

#define APP_ADV_INTERVAL                MSEC_TO_UNITS(1000, UNIT_0_625_MS)           /**< The advertising interval (in units of 0.625 ms. This value corresponds to 750 ms). */

#endif

#define APP_ADV_DURATION                6000                                       /**< The advertising duration (60 seconds) in units of 10 milliseconds. */


#define INFO_SERVICE_UUID_TYPE         BLE_UUID_TYPE_VENDOR_BEGIN         // info service服务UUID类型：厂商自定义UUID
#define APP_BLE_CONN_CFG_TAG            1                                 //SoftDevice BLE配置标志

static ble_uuid_t        m_adv_uuids[] =                                            /**< Universally unique service identifiers. */
{
    {BLE_UUID_HUMAN_INTERFACE_DEVICE_SERVICE, BLE_UUID_TYPE_BLE},
};


BLE_ADVERTISING_DEF(ble_advertising);                      //定义名称为m_advertising的广播模块实例

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code;
    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

struct __attribute__((__packed__)) ble_adv_info
{
	unsigned int charge_status : 2;
	unsigned int ble_mode : 2;
	unsigned int cmd_version : 4;
	unsigned int null : 8;
};

static struct ble_adv_info adv_info ={
.charge_status = 1,
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1 ||\
     HARDWARE_441_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1191x_ENABLED == 1 || defined(HANDWARE_1_23_2x))	

	.ble_mode = 1,
#elif (HARDWARE_402_ENABLED == 1 )	
	.ble_mode = 0,
#else
	.ble_mode = 0,
#endif	
	
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1  || HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_441_ENABLED == 1 \
   || HARDWARE_1121_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1191_ENABLED == 1 || defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))	

	.cmd_version = 1,
#endif		
    .null = 0xFF,
};


// 小米快连广播包数据结构 (纯厂商特定数据 payload，24 字节)
// SDK 会自动添加：
// - Flags: 3 字节 (Length=0x02 + AD Type=0x01 + Value=0x06)
// - Manufacturer Specific Data header: 4 字节 (Length=0x1B + AD Type=0xFF + Company ID=0x0157)
// 总计：3 + 4 + 24 = 31 字节
// 
// 根据协议图：
// - Byte7: SIG ID (0x03)
// - Byte8: Length (0x16 = 22)
// - Byte9-30: 快连数据 (22 字节)
#if defined(USE_XIAOMI)
struct __attribute__((__packed__)) xiaomi_fast_connect_adv_t
{
    // 小米快连数据 (24 字节)
    uint8_t length3;        // 0x16 - Length of fast connect data (22 字节) (Byte7)
    uint8_t type;           // 0x01 - Type (快连) (Byte8)
    uint8_t major_id;       // MajorID (Byte9)
    uint8_t state1;         // State1 (Byte10)
    uint8_t state2;         // State2 (Byte11)
    uint8_t minor_id;       // MinorID (Byte12)
    uint8_t battery_info;   // 左右耳机电池信息 (Byte13)
    uint8_t box_battery;    // 盒子电池 (Byte14)
    uint8_t host_mac_lap[3]; // 配对手机 MAC 的 LAP (Byte15-17)
    uint8_t right_mac[6];   // 右耳机 MAC 地址 (Byte18-23)
    uint8_t counter;        // 广播计数标识 (Byte24)
    uint8_t left_mac[6];    // 左耳机 MAC 地址 (Byte25-30)
    // 总计：1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 3 + 6 + 1 + 6 = 24 字节
};

// 小米快连广播包数据
static struct xiaomi_fast_connect_adv_t g_xiaomi_adv_data;
static uint8_t g_broadcast_counter = 0;  // 广播计数器

#endif


/**@brief Function for setting filtered device identities.
 *
 * @param[in] skip  Filter passed to @ref pm_peer_id_list.
 */
static void identities_set(pm_peer_id_list_skip_t skip)
{
    pm_peer_id_t peer_ids[BLE_GAP_DEVICE_IDENTITIES_MAX_COUNT];
    uint32_t     peer_id_count = BLE_GAP_DEVICE_IDENTITIES_MAX_COUNT;

    ret_code_t err_code = pm_peer_id_list(peer_ids, &peer_id_count, PM_PEER_ID_INVALID, skip);
    APP_ERROR_CHECK(err_code);

    err_code = pm_device_identities_list_set(peer_ids, peer_id_count);
    APP_ERROR_CHECK(err_code);
}


//广播事件处理函数
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
	ret_code_t err_code;
    //判断广播事件类型
    switch (ble_adv_evt)
    {
        //快速广播启动事件：快速广播启动后会产生该事件
			  case BLE_ADV_EVT_FAST:
            BC_LOG_INFO("Fast advertising.\r\n");
			      //设置广播指示灯为正在广播（D1指示灯闪烁）
//            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
//            APP_ERROR_CHECK(err_code);
            break;
        //广播IDLE事件：广播超时后会产生该事件
        case BLE_ADV_EVT_IDLE:
//            sleep_mode_enter();
            break;

        case BLE_ADV_EVT_WHITELIST_REQUEST:
        {
            ble_gap_addr_t whitelist_addrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
            ble_gap_irk_t  whitelist_irks[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
            uint32_t       addr_cnt = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;
            uint32_t       irk_cnt  = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

            err_code = pm_whitelist_get(whitelist_addrs, &addr_cnt,
                                        whitelist_irks,  &irk_cnt);
            APP_ERROR_CHECK(err_code);
            BC_LOG_DEBUG("pm_whitelist_get returns %d addr in whitelist and %d irk whitelist\r\n",
                           addr_cnt,
                           irk_cnt);

            // Set the correct identities list (no excluding peers with no Central Address Resolution).
            identities_set(PM_PEER_ID_LIST_SKIP_NO_IRK);

            // Apply the whitelist.
            err_code = ble_advertising_whitelist_reply(&ble_advertising,
                                                       whitelist_addrs,
                                                       addr_cnt,
                                                       whitelist_irks,
                                                       irk_cnt);
            APP_ERROR_CHECK(err_code);
        }
        break;

        case BLE_ADV_EVT_PEER_ADDR_REQUEST:
        {
            pm_peer_data_bonding_t peer_bonding_data;

            // Only Give peer address if we have a handle to the bonded peer.
            if (m_peer_id != PM_PEER_ID_INVALID)
            {
                err_code = pm_peer_data_bonding_load(m_peer_id, &peer_bonding_data);
                if (err_code != NRF_ERROR_NOT_FOUND)
                {
                    APP_ERROR_CHECK(err_code);

                    // Manipulate identities to exclude peers with no Central Address Resolution.
                    identities_set(PM_PEER_ID_LIST_SKIP_ALL);
                    ble_gap_addr_t * p_peer_addr = &(peer_bonding_data.peer_ble_id.id_addr_info);
                    err_code = ble_advertising_peer_addr_reply(&ble_advertising, p_peer_addr);
                    APP_ERROR_CHECK(err_code);
                }

            }
            break;
        }

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

 /**@brief Function for handling advertising errors.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void ble_advertising_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

#ifndef USE_XIAOMI
//广播初始化
void advertising_init(void)
{
#ifdef ADVERTISING_UPDATE
    uint8_t adv_pyload[30] = {0};
#endif
	sd_ble_gap_adv_stop(ble_advertising.adv_handle);
	    uint32_t               err_code;
    ble_advertising_init_t init;
    ble_advdata_manuf_data_t    p_manuf_specific_data;               /**< Manufacturer specific data. */
    memset(&init, 0, sizeof(init));

    
    p_manuf_specific_data.company_identifier = *(uint16_t*)&adv_info;

    ble_gap_addr_t p_addr;
    sd_ble_gap_addr_get(&p_addr);
#ifdef ADVERTISING_UPDATE
    memcpy(adv_pyload, p_addr.addr, BLE_GAP_ADDR_LEN);
    memcpy(&adv_pyload[BLE_GAP_ADDR_LEN+1], (const void *)&pdm_stop_utime, sizeof(pdm_stop_utime));
    p_manuf_specific_data.data.size = BLE_GAP_ADDR_LEN+1+sizeof(pdm_stop_utime);
    p_manuf_specific_data.data.p_data = adv_pyload;
#else
    p_manuf_specific_data.data.p_data = p_addr.addr;
    p_manuf_specific_data.data.size = 6;
#endif

    
    
    memcpy(locmac, p_addr.addr, BLE_GAP_ADDR_LEN);
    init.advdata.p_manuf_specific_data  = &p_manuf_specific_data;
    
    init.srdata.name_type          = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = true;

	

    init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids  = m_adv_uuids;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;//BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    init.config.ble_adv_fast_enabled  = false;
    init.config.ble_adv_fast_interval = APP_ADV_FAST_INTERVAL;
    init.config.ble_adv_fast_timeout=APP_ADV_FAST_DURATION;
    init.config.ble_adv_slow_enabled = true;
    init.config.ble_adv_slow_interval=APP_ADV_SLOW_INTERVAL;
    init.config.ble_adv_slow_timeout=0;    
    
 
    
    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&ble_advertising, &init);
    APP_ERROR_CHECK(err_code);

    

#if ( HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1)	
			err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV , ble_advertising.adv_handle, RADIO_TXPOWER_TXPOWER_Pos8dBm);
			APP_ERROR_CHECK(err_code);
#elif ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	

	err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV , ble_advertising.adv_handle, RADIO_TXPOWER_TXPOWER_Pos4dBm);
	APP_ERROR_CHECK(err_code);
#else
  err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV , ble_advertising.adv_handle, RADIO_TXPOWER_TXPOWER_0dBm);
	APP_ERROR_CHECK(err_code);

#endif	
    ble_advertising_conn_cfg_tag_set(&ble_advertising, APP_BLE_CONN_CFG_TAG);

}
#else
void advertising_init(void)
{
	sd_ble_gap_adv_stop(ble_advertising.adv_handle);
    uint32_t               err_code;
    ble_advertising_init_t init;
    ble_advdata_manuf_data_t    p_manuf_specific_data;
    memset(&init, 0, sizeof(init));

    // 获取本地 MAC 地址
    ble_gap_addr_t p_addr;
    sd_ble_gap_addr_get(&p_addr);
    memcpy(locmac, p_addr.addr, BLE_GAP_ADDR_LEN);
    
    // 初始化小米快连广播包数据 (25 字节纯 payload)
    memset(&g_xiaomi_adv_data, 0, sizeof(g_xiaomi_adv_data));
    
    // 小米厂商特定数据 payload
    g_xiaomi_adv_data.length3 = 0x16;  // Length of fast connect data (22 字节)
    g_xiaomi_adv_data.type = XIAOMI_FAST_CONNECT_TYPE;  // 0x01 - 快连 Type
    g_xiaomi_adv_data.major_id = XIAOMI_Major_ID;    // MajorID (由 MIUI 分配，初始为 0)
    
    // State1: 可发现状态配置
    g_xiaomi_adv_data.state1 = 0xA0;
    
    // State2: 配对状态配置
    g_xiaomi_adv_data.state2 = 0x00;
    
    g_xiaomi_adv_data.minor_id = XIAOMI_Minor_ID;    // MinorID (由 MIUI 分配，初始为 0)
    
    // 电池信息 (初始为 0)
    g_xiaomi_adv_data.battery_info = 0;
    g_xiaomi_adv_data.box_battery = 0;
    
    // Host MAC LAP (初始为 0)
    g_xiaomi_adv_data.host_mac_lap[0] = 0;
    g_xiaomi_adv_data.host_mac_lap[1] = 0;
    g_xiaomi_adv_data.host_mac_lap[2] = 0;
    
    // 右耳机 MAC 地址 (使用设备 MAC)
    g_xiaomi_adv_data.right_mac[0] = p_addr.addr[4];
    g_xiaomi_adv_data.right_mac[1] = p_addr.addr[5];
    g_xiaomi_adv_data.right_mac[2] = p_addr.addr[3];
    g_xiaomi_adv_data.right_mac[3] = p_addr.addr[0];
    g_xiaomi_adv_data.right_mac[4] = p_addr.addr[1];
    g_xiaomi_adv_data.right_mac[5] = p_addr.addr[2];
    
    // 广播计数器
    g_xiaomi_adv_data.counter = g_broadcast_counter;
    
    // 左耳机 MAC 地址 (初始与右耳相同)
//    g_xiaomi_adv_data.left_mac[0] = p_addr.addr[0];
//    g_xiaomi_adv_data.left_mac[1] = p_addr.addr[1];
//    g_xiaomi_adv_data.left_mac[2] = p_addr.addr[2];
//    g_xiaomi_adv_data.left_mac[3] = p_addr.addr[3];
//    g_xiaomi_adv_data.left_mac[4] = p_addr.addr[4];
//    g_xiaomi_adv_data.left_mac[5] = p_addr.addr[5];
    
    // 设置厂商特定数据
    p_manuf_specific_data.company_identifier = XIAOMI_VENDOR_ID;
    p_manuf_specific_data.data.p_data = (uint8_t*)&g_xiaomi_adv_data;
    p_manuf_specific_data.data.size = sizeof(g_xiaomi_adv_data);  // 24 字节
    
    // 小米快连广播包配置：
    // 广播数据只包含 Flags(3) + Manufacturer Specific Data(28) = 31 字节
    // 其他数据(名称/UUIDs/Appearance)放到扫描响应中
    
    init.advdata.p_manuf_specific_data  = &p_manuf_specific_data;
    init.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    // 广播数据中不要放 Appearance 和 UUIDs，避免超出 31 字节
    init.advdata.include_appearance = false;
    init.advdata.uuids_complete.uuid_cnt = 0;
    init.advdata.uuids_complete.p_uuids = NULL;
    
    // 扫描响应中包含名称、Appearance 和 UUIDs
    init.srdata.name_type = BLE_ADVDATA_FULL_NAME;
    init.srdata.include_appearance = true;
    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = false;
    init.config.ble_adv_fast_interval = APP_ADV_FAST_INTERVAL;
    init.config.ble_adv_fast_timeout=APP_ADV_FAST_DURATION;
    init.config.ble_adv_slow_enabled = true;
    init.config.ble_adv_slow_interval=APP_ADV_SLOW_INTERVAL;
    init.config.ble_adv_slow_timeout=0;    
    
    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&ble_advertising, &init);
    APP_ERROR_CHECK(err_code);

#if ( HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1)	
    err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV , ble_advertising.adv_handle, RADIO_TXPOWER_TXPOWER_Pos8dBm);
    APP_ERROR_CHECK(err_code);
#elif ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	
    err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV , ble_advertising.adv_handle, RADIO_TXPOWER_TXPOWER_Pos4dBm);
    APP_ERROR_CHECK(err_code);
#else
    err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV , ble_advertising.adv_handle, RADIO_TXPOWER_TXPOWER_0dBm);
    APP_ERROR_CHECK(err_code);
#endif	
    ble_advertising_conn_cfg_tag_set(&ble_advertising, APP_BLE_CONN_CFG_TAG);
}

/**
 * @brief 更新小米快连广播包数据
 * 
 * @param battery_info 左右耳机电量信息
 * @param box_battery 盒子电量
 * @param state1 State1 配置
 * @param state2 State2 配置
 * @param host_mac_lap 配对手机 MAC 的 LAP (3 字节，可为 NULL)
 */
void xiaomi_fast_connect_adv_data_update(uint8_t battery_info, uint8_t box_battery, 
                                          uint8_t state1, uint8_t state2,
                                          const uint8_t *host_mac_lap)
{
    ret_code_t err_code;
    ble_advdata_t adv_data;
    ble_advdata_t sr_data;
    ble_advdata_manuf_data_t manuf_specific_data;
    
    // 更新电池信息
    g_xiaomi_adv_data.battery_info = battery_info;
    g_xiaomi_adv_data.box_battery = box_battery;
    
    // 更新状态信息
    g_xiaomi_adv_data.state1 = state1;
    g_xiaomi_adv_data.state2 = state2;
    
    // 更新 Host MAC LAP
    if (host_mac_lap != NULL)
    {
        g_xiaomi_adv_data.host_mac_lap[0] = host_mac_lap[0];
        g_xiaomi_adv_data.host_mac_lap[1] = host_mac_lap[1];
        g_xiaomi_adv_data.host_mac_lap[2] = host_mac_lap[2];
    }
    
    // 更新广播计数器
    g_broadcast_counter++;
    g_xiaomi_adv_data.counter = g_broadcast_counter;
    
    // 设置厂商特定数据
    manuf_specific_data.company_identifier = XIAOMI_VENDOR_ID;
    manuf_specific_data.data.p_data = (uint8_t*)&g_xiaomi_adv_data;
    manuf_specific_data.data.size = sizeof(g_xiaomi_adv_data);
    
    // 配置广播数据 - 只包含 Flags + Manufacturer Data，确保不超过 31 字节
    memset(&adv_data, 0, sizeof(adv_data));
    memset(&sr_data, 0, sizeof(sr_data));
    
    // 广播数据：Flags + Manufacturer Specific Data = 31 字节
    adv_data.p_manuf_specific_data = &manuf_specific_data;
    adv_data.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    adv_data.include_appearance = false;
    
    // 扫描响应：名称 + Appearance + UUIDs
    sr_data.name_type = BLE_ADVDATA_FULL_NAME;
    sr_data.include_appearance = true;
    sr_data.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    sr_data.uuids_complete.p_uuids = m_adv_uuids;
    
    // 更新广播数据
    err_code = ble_advertising_advdata_update(&ble_advertising, &adv_data, &sr_data);
    APP_ERROR_CHECK(err_code);
}
#endif


void ble_adv_data_update(uint8_t *update_data,uint8_t update_length)
{

ret_code_t err_code;
	  ble_advdata_t           adv_data; //广播数据
    ble_advdata_t           sr_data;  //扫描响应数据
	  //每次更新时，厂商自定义数据加1，方便观察实验现象
	if(update_length > 15)
	{
		return;
	}
	uint8_t adv_pyload[30] = {0};
//	uint8_t adv_pyload_length;
//	ble_gap_addr_t p_addr;
//    sd_ble_gap_addr_get(&p_addr);
//	memcpy(adv_pyload,p_addr.addr,6);
    memcpy(&adv_pyload[0],update_data,update_length);
	  //定义个一个制造商自定义数据的结构体变量
	  ble_advdata_manuf_data_t manuf_specific_data;

    //制造商ID，0x0059是Nordic的厂商ID
	manuf_specific_data.company_identifier = 0xFF01;
		//指向制造商自定义的数据
	 manuf_specific_data.data.p_data = adv_pyload;
		//制造商自定义的数据大小(字节数)
    manuf_specific_data.data.size   =   update_length;
	
	  memset(&adv_data, 0, sizeof(adv_data));
	  memset(&sr_data, 0, sizeof(sr_data));
	  adv_data.name_type               = BLE_ADVDATA_FULL_NAME;
	  //是否包含外观：包含
    adv_data.include_appearance      = false;
	  //广播中加入制造商自定义数据
		adv_data.p_manuf_specific_data    = &manuf_specific_data;
	  //Flag:一般可发现模式，不支持BR/EDR
    adv_data.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
			
	  //UUID放到扫描响应里面
	  sr_data.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    sr_data.uuids_complete.p_uuids  = m_adv_uuids;
	  //更新广播内容
	  err_code = ble_advertising_advdata_update(&ble_advertising, &adv_data, &sr_data);
		APP_ERROR_CHECK(err_code);
}

void bc_ble_adv_data_update(uint8_t *update_data,uint8_t update_length)
{

ret_code_t err_code;
	  ble_advdata_t           adv_data; //广播数据
    ble_advdata_t           sr_data;  //扫描响应数据
	  //每次更新时，厂商自定义数据加1，方便观察实验现象
	if(update_length > 15)
	{
		return;
	}
	uint8_t len, adv_pyload[30] = {0};
//	uint8_t adv_pyload_length;
//	ble_gap_addr_t p_addr;
//    sd_ble_gap_addr_get(&p_addr);
//	memcpy(adv_pyload,p_addr.addr,6);
    //memcpy(&adv_pyload[0],update_data,update_length);
	  //定义个一个制造商自定义数据的结构体变量
	  ble_advdata_manuf_data_t manuf_specific_data;

    if(0 == locmac[0]) {
        ble_gap_addr_t p_addr;
        sd_ble_gap_addr_get(&p_addr);
        len = sizeof(p_addr.addr);
        memcpy(adv_pyload, p_addr.addr, len);
    } else {
        len = sizeof(locmac);
        memcpy(adv_pyload, locmac, len);
    }
    len += 1; // capacity
    memcpy(&adv_pyload[len],update_data,update_length);
    len += update_length;
    //制造商ID，0x0059是Nordic的厂商ID
	manuf_specific_data.company_identifier = *(uint16_t*)&adv_info;
		//指向制造商自定义的数据
	 manuf_specific_data.data.p_data = adv_pyload;
		//制造商自定义的数据大小(字节数)
    manuf_specific_data.data.size   =   len;
	
	  memset(&adv_data, 0, sizeof(adv_data));
	  memset(&sr_data, 0, sizeof(sr_data));
	  adv_data.name_type               = BLE_ADVDATA_FULL_NAME;
	  //是否包含外观：包含
    adv_data.include_appearance      = false;
	  //广播中加入制造商自定义数据
		adv_data.p_manuf_specific_data    = &manuf_specific_data;
	  //Flag:一般可发现模式，不支持BR/EDR
    adv_data.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
			
	  //UUID放到扫描响应里面
	  sr_data.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    sr_data.uuids_complete.p_uuids  = m_adv_uuids;
	  //更新广播内容
	  err_code = ble_advertising_advdata_update(&ble_advertising, &adv_data, &sr_data);
		APP_ERROR_CHECK(err_code);
}

bool is_adv_active(uint8_t adv_handle) 
{
    ble_gap_addr_t addr = {0};
    ret_code_t err = sd_ble_gap_adv_addr_get(adv_handle, &addr);
    
    if (err == NRF_SUCCESS) 
	{
        return true;
    }
    return false;
}


//启动广播，该函数所用的模式必须和广播初始化中设置的广播模式一样
 void advertising_start(void)
{
	if(is_adv_active(ble_advertising.adv_handle))
	{
		return;
	}
   //使用广播初始化中设置的广播模式启动广播
	 ret_code_t err_code = ble_advertising_start(&ble_advertising, BLE_ADV_MODE_FAST);
	 BC_LOG_INFO("advertising_start,err_code=%d\r\n",err_code);
	 //检查函数返回的错误代码
   APP_ERROR_CHECK(err_code);
}

//停止广播
 void advertising_stop(void)
{
    uint32_t err_code;
    if(!is_adv_active(ble_advertising.adv_handle))
	{
		return;
	}
    // 直接调用 SoftDevice API 停止广播
    err_code = sd_ble_gap_adv_stop(ble_advertising.adv_handle);
    
    if (err_code != NRF_SUCCESS)
    {
        BC_LOG_ERROR("Failed to stop advertising (0x%X)", err_code);
    }
}