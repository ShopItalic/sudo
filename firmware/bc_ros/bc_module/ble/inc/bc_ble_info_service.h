#ifndef __FML_BLE_INFO_SERVICE__H__
#define __FML_BLE_INFO_SERVICE__H__

#include "stdint.h"
#include "stdbool.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "ble_link_ctx_manager.h"

//定义info service服务实例，该实例完成2件事情
//1：定义了static类型info service服务结构体变量，为串口透传服务结构体分配了内存
//2：注册了BLE事件监视者，这使得info service程序模块可以接收BLE协议栈的事件，从而可以在ble_info_service_on_ble_evt()事件回调函数中处理自己感兴趣的事件
#define BLE_INFO_SERVICE_DEF(_name, _info_service_max_clients)                                      \
    BLE_LINK_CTX_MANAGER_DEF(CONCAT_2(_name, _link_ctx_storage),  \
                             (_info_service_max_clients),                  \
                             sizeof(ble_info_service_client_context_t));   \
	  static ble_info_service_t _name =                                     \
	  {                                                             \
        .p_link_ctx_storage = &CONCAT_2(_name, _link_ctx_storage) \
    };                                                            \
    NRF_SDH_BLE_OBSERVER(_name ## _obs,                           \
                         BLE_NUS_BLE_OBSERVER_PRIO,               \
                         ble_info_service_on_ble_evt,                    \
                         &_name)


//定义info service服务128位UUID基数
#define INFO_SERVICE_BASE_UUID                   {{0x1F, 0x9D, 0x32, 0xF7, 0xF1, 0x3A, 0x65, 0x8E, 0x03, 0x45, 0x05, 0x4F, 0x00, 0x00, 0xE8, 0xBA}} /**< Used vendor specific UUID. */
//定义服务和特征的16位UUID
#define BLE_UUID_INFO_SERVICE_SERVICE 0x0001              //info service服务16位UUID
#define BLE_UUID_INFO_SERVICE_TX_CHARACTERISTIC 0x0011    //TX特征16位UUID           
#define BLE_UUID_INFO_SERVICE_RX_CHARACTERISTIC 0x0010    //RX特征16位UUID


//定义操作码长度
#define OPCODE_LENGTH        1
//定义句柄长度
#define HANDLE_LENGTH        2


//定义最大传输数据长度（字节数）
//#if defined(NRF_SDH_BLE_GATT_MAX_MTU_SIZE) && (NRF_SDH_BLE_GATT_MAX_MTU_SIZE != 0)
//    #define BLE_INFO_SERVICE_MAX_DATA_LEN (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH)
//#else
//    #define BLE_info_service_MAX_DATA_LEN (BLE_GATT_MTU_SIZE_DEFAULT - OPCODE_LENGTH - HANDLE_LENGTH)
//    #warning NRF_SDH_BLE_GATT_MAX_MTU_SIZE is not defined.
//#endif

#if defined(NRF_SDH_BLE_GATT_MAX_MTU_SIZE) && (NRF_SDH_BLE_GATT_MAX_MTU_SIZE != 0)
    #define BLE_INFO_SERVICE_MAX_DATA_LEN (NRF_SDH_BLE_GATT_MAX_MTU_SIZE )
#else
    #define BLE_info_service_MAX_DATA_LEN (BLE_GATT_MTU_SIZE_DEFAULT )
    #warning NRF_SDH_BLE_GATT_MAX_MTU_SIZE is not defined.
#endif

//定义info服务事件类型，这是用户自己定义的，供应用程序使用
typedef enum
{
    BLE_INFO_SERVICE_EVT_RX_DATA,      //接收到新的数据
    BLE_INFO_SERVICE_EVT_TX_RDY,       //准备就绪，可以发送新数据
	BLE_NUS_EVT_COMM_STARTED,   //通知已经使能
    BLE_NUS_EVT_COMM_STOPPED,   //通知已经禁止
} ble_info_service_evt_type_t;	

/* Forward declaration of the ble_nus_t type. */
typedef struct ble_info_service_s ble_info_service_t;

//info服务BLE_NUS_EVT_RX_DATA事件数据结构体，该结构体用于当BLE_NUS_EVT_RX_DATA产生时将接收的数据信息传递给处理函数
typedef struct
{
    uint8_t const * p_data; //指向存放接收数据的缓存
    uint16_t        length; //接收的数据长度
} ble_info_service_evt_rx_data_t;

//记录对端设备是否使能了RX特征的通知
typedef struct
{
    bool is_notification_enabled; 
} ble_info_service_client_context_t;

//info服务事件结构体
typedef struct
{
    ble_info_service_evt_type_t       type;        //事件类型
    ble_info_service_t                * p_info_service;   //指向串口透传实例的指针
    uint16_t                            conn_handle; //连接句柄
    ble_info_service_client_context_t * p_link_ctx;//指向link context
    union
    {
        ble_info_service_evt_rx_data_t rx_data; //BLE_NUS_EVT_RX_DATA事件数据
    } params;
} ble_info_service_evt_t;



//定义函数指针类型ble_info_service_data_handler_t
typedef void (* ble_info_service_data_handler_t) (ble_info_service_evt_t * p_evt);



//串口服务初始化结构体
typedef struct
{
    ble_info_service_data_handler_t data_handler; //处理接收数据的事件句柄
} ble_info_service_init_t;



//串口透传服务结构体，包含所需要的信息
struct ble_info_service_s
{
    uint8_t                         uuid_type;          //UUID类型
    uint16_t                        service_handle;     //info service服务句柄（由协议栈提供）
    ble_gatts_char_handles_t        tx_handles;         //TX特征句柄
    ble_gatts_char_handles_t        rx_handles;           //RX特征句柄
	  blcm_link_ctx_storage_t * const p_link_ctx_storage;      //指向存储所有当前连接及其context的“link context”的句柄的指针
    ble_info_service_data_handler_t        data_handler;       //处理接收数据的事件句柄
};


uint32_t ble_info_service_data_send(ble_info_service_t * p_info_service,
                           uint8_t   * p_data,
                           uint16_t  * p_length,
                           uint16_t    conn_handle);
uint32_t ble_info_service_init(ble_info_service_t * p_info_service, ble_info_service_init_t const * p_info_service_init);
void ble_info_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);




#endif




