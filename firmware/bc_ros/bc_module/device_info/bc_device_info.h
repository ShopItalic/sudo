#ifndef __BC_DEVICE_INFO_H__
#define __BC_DEVICE_INFO_H__

//#include "app_mode_handler.h"

#include "stdint.h"
#include "stdbool.h"

enum hid_device_type
{
	ANDROID_HID_DEVICE = 0,
	IOS_HID_DEVICE,
};

#pragma pack (1)
typedef struct
{
	uint32_t device_hid_type;        //0 不支持hid   1 支持hid
	uint32_t device_hid_enable_flag; //0 android  1 ios    
	uint32_t device_hid_touch_mode;
	uint32_t device_hid_gesture_mode;
}bc_device_hid_info; //必须为4的倍数
#pragma pack ()

#pragma pack (1)
typedef struct
{
	uint32_t device_update_time;        //app下发的同步时间
	uint32_t device_update_time_zone;   //app下发的同步时间时区
	uint32_t device_reset_time;         //0点自动重启的时间记录   
}bc_device_time;                        //必须为4的倍数
#pragma pack ()


#pragma pack (1)
typedef struct
{
	uint32_t shut_down_time;        //app下发的同步时间  
}bc_device_shut_down_info;                        //必须为4的倍数
#pragma pack ()

#pragma pack (1)
typedef struct
{
	uint32_t device_sleep_mode;        //app下发的同步时间  
	uint32_t device_sleep_mode_flag;        //app下发的同步时间  
}bc_device_sleep;                      //必须为4的倍数
#pragma pack ()

#pragma pack (1)
typedef struct
{
	uint32_t acc_frequency;        //app下发的同步时间  
	uint32_t gyro_frequency;        //app下发的同步时间  
}bc_device_six_axis_config;                      //必须为4的倍数
#pragma pack ()


#pragma pack (1)
typedef struct
{
	uint32_t ppg_file_flag;
  uint32_t ppg_file_resume_count;
	uint32_t ppg_file_mode;
	char     ppg_file_name[60];
}bc_device_file_info;                      //必须为4的倍数
#pragma pack ()


#pragma pack (1)
typedef struct
{
	uint32_t audio_up_mode;               //0 pcm,1 adpcm
}bc_device_audio_info;                      //必须为4的倍数
#pragma pack ()

#pragma pack (1)
typedef struct
{
	uint32_t nfc_mode;               //0 pcm,1 adpcm
}bc_device_nfc_info;                      //必须为4的倍数
#pragma pack ()

#if defined(BLE_MULTI_MASTER)

#define BLE_MULTI_MASTER_NUM_MAX  5

struct __attribute__((__packed__)) control_type_list
{
	unsigned int ppg : 1;      //0 不支持   1 支持
	unsigned int touch : 1;    //0 不支持   1 支持
	unsigned int gesture : 1;  //0 不支持   1 支持
	unsigned int mic : 1;      //0 不支持   1 支持
	unsigned int :4;
};

enum ble_connect_device_type
{
	CONNECT_DEVICE_TYPE_NULL = 0,
	CONNECT_DEVICE_TYPE_PC,
	CONNECT_DEVICE_TYPE_TV,
	CONNECT_DEVICE_TYPE_GLASSES,
	CONNECT_DEVICE_TYPE_ANDROID,
	CONNECT_DEVICE_TYPE_IOS,
	CONNECT_DEVICE_TYPE_HarmonyOS,
};

#pragma pack (1)
typedef struct
{
	uint8_t name[20];
	uint8_t mac[6];
	uint8_t type_status;     //1主设备机，0 从设备机      本机设备为中控机（戒指）
	uint8_t connect_state;   //0 未连接 ，1 已连接
	uint8_t work_status;     //0 关闭控制，1 开启控制     关闭控制是计算连接也不会控制
	uint16_t handle;         //句柄
	enum ble_connect_device_type device_type;     //设备类型  0为无，1 pc设备（手势与touch默认为上下滑控制ppt），2 电视设备（touch默认为遥控器功能）,3 眼镜设备（touch默认为眼镜控制），4 手机设备android 5 手机设备ios 6 手机设备HarmonyOS
	struct control_type_list control_list;
}bc_device_ble_host_info;                      //必须为4的倍数
#pragma pack ()

#pragma pack (1)
typedef struct
{
	bc_device_ble_host_info ble_host_device_info_list[BLE_MULTI_MASTER_NUM_MAX];
}bc_device_ble_host_device;                      //必须为4的倍数
#pragma pack ()


#endif // defined(BLE_MULTI_MASTER)

#if defined(ALARM_CLOCK)

#define ALARM_CLOCK_NUM  5

enum  device_alarm_type
{
	ALARM_TYPE_SINGLE = 0,  //单次
	ALARM_TYPE_REPEAT,     //重复
	ALARM_TYPE_SMART_WORKDAY,  //智能工作日
	
};

enum  device_vibration_type
{
	VIBRATION_TYPE_STRONG = 0,   //强烈
	VIBRATION_TYPE_WEAK,         //r弱
	VIBRATION_TYPE_GRADIENT,    //渐变
	
};

struct __attribute__((__packed__)) device_week_list
{
	unsigned int week_0: 1;      //0 不支持   1 支持
	unsigned int week_1: 1;      //0 不支持   1 支持
	unsigned int week_2: 1;      //0 不支持   1 支持
	unsigned int week_3: 1;      //0 不支持   1 支持
	unsigned int week_4: 1;      //0 不支持   1 支持
	unsigned int week_5: 1;      //0 不支持   1 支持
	unsigned int week_6: 1;      //0 不支持   1 支持
	unsigned int :1;
};


#pragma pack (1)

struct bc_device_alarm_clock
{
	uint32_t alarm_time;
	enum  device_alarm_type alarm_type;
	enum  device_vibration_type vibration_type;
	uint8_t alarm_switch;
	struct  device_week_list week_list;
	
};

#pragma pack ()


#pragma pack (1)

struct bc_device_holiday_list
{
	uint16_t years;
	uint8_t work_doliday_number;
	uint16_t work_doliday_list[50];
	uint8_t rest_doliday_number;
	uint16_t rest_doliday_list[50];
};

#pragma pack ()

#pragma pack (1)

struct bc_device_alarm_clock_info
{
	struct bc_device_alarm_clock alarm_list[ALARM_CLOCK_NUM];
	struct bc_device_holiday_list holiday_list[2];
};

#pragma pack ()

#endif //ALARM_CLOCK


struct bc_device_identity_info
{
	uint8_t sn[24];
};

struct bc_device_sport_info
{
  uint32_t sport_mode;
  uint32_t sport_start_time;
  uint32_t sport_stop_time;
};


#pragma pack (1)
typedef struct
{
	uint8_t file_name[64];               
}bc_device_capture_audio_info;                      
#pragma pack ()

#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
#pragma pack (1)
typedef struct
{
    uint16_t duration_ms;    /* 单次震动时长（毫秒） */
    uint8_t  vib_count;      /* 震动次数 */
    uint8_t  vib_gain;       /* 震动强度/增益（0x00~0xFF） */
    uint16_t interval_ms;    /* 多次震动之间的间隔（毫秒） */
}bc_device_linear_motor_info;
#pragma pack ()
#endif

#if defined(HANDWARE_1_23_2)
#pragma pack (1)
typedef struct
{
    uint8_t ble_connect_color;      /* 蓝牙连接 LED 颜色：0x01=蓝, 0x02=红, 0x03=绿 */
    uint8_t ble_disconnect_color;   /* 蓝牙断开 LED 颜色：0x01=蓝, 0x02=红, 0x03=绿 */
    uint8_t recording_color;        /* 录音 LED 颜色：0x01=绿, 0x02=红, 0x03=蓝 */
    uint8_t motor_start_mode;       /* 开启录音马达模式：0x01=短振1次, 0x02=长振1次 */
    uint8_t motor_stop_mode;        /* 关闭录音马达模式：0x01=短振2次, 0x02=长振2次 */
}bc_device_led_motor_mode_info;
#pragma pack ()
#endif


#pragma pack (1)
typedef struct
{
  uint8_t mac[8];
	uint32_t reset_io_flag;
	uint8_t ble_name[20];
	uint8_t ble_name_length;
	uint8_t authentication_key_code[128];
	uint8_t authentication_key_code_length;
	uint8_t url[128];
	uint32_t url_length;
	uint32_t ppg_update_record_time;
	uint32_t sport_count;
	bc_device_hid_info hid_info;
	bc_device_time  device_time;
	bc_device_sleep  device_sleep;
	bc_device_six_axis_config six_axis_config;
	bc_device_file_info  file_info;
	bc_device_audio_info audio_info;
	bc_device_nfc_info   nfc_info;
  bc_device_shut_down_info  shut_down_info;
  bc_device_capture_audio_info capture_audio_info;
  struct bc_device_identity_info device_identity_info;
	struct bc_device_sport_info  sport_info;

#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
    bc_device_linear_motor_info linear_motor_info;   // 线性马达配置
#endif
  
  
#if defined(BLE_MULTI_MASTER)
	
	bc_device_ble_host_device ble_host_device_list;                      //必须为4的倍数
	
#endif // defined(BLE_MULTI_MASTER)	


#if defined(ALARM_CLOCK)
    
	struct bc_device_alarm_clock_info alarm_clock_info;

#endif //ALARM_CLOCK

#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
    uint8_t vbat_type;
#endif
#if defined(HANDWARE_1_23_2_ONE_SEC)
    uint8_t  timer_record_enable;     // 定时录音开关：1=开 0=关
    uint32_t timer_record_interval;   // 间隔时间（秒）
    uint32_t timer_record_duration;   // 录音时长（秒），固定30秒
#endif

    uint8_t cang_mac_len;
    uint8_t cang_mac[24];

#if defined(HANDWARE_1_23_2)
    bc_device_led_motor_mode_info led_motor_mode_info;
#endif

	uint8_t  test[3];
	uint16_t crc;
}bc_device_info; //必须为4的倍数
#pragma pack ()


uint32_t bc_device_info_get_ppg_update_record_time(void);

void bc_device_info_set_ppg_update_record_time(uint32_t time);

uint32_t bc_device_info_get_sport_count(void);

void bc_device_info_set_sport_count(uint32_t sport_count);

void bc_device_info_get_key_code(uint8_t *key_code,uint8_t *key_code_len);

void bc_device_info_set_key_code(uint8_t *key_code,uint8_t key_code_len);

void bc_device_info_get_url(uint8_t *url,uint8_t *url_length);

bool bc_device_info_set_url(uint8_t *url,uint8_t url_length);

void bc_device_info_init(void);

void bc_device_info_reset(void);

void bc_device_info_get_ble_name(uint8_t *ble_name,uint8_t *ble_name_length);

bool bc_device_info_set_ble_name(uint8_t *ble_name,uint8_t ble_name_length);

bc_device_hid_info *bc_device_info_get_hid_info(void);

bool bc_device_info_set_hid_info(bc_device_hid_info *hid_info);

bool bc_device_info_app_update_time_set(uint32_t time,uint32_t zone);

uint32_t bc_device_info_app_update_time_get(void);

bool bc_device_info_app_reset_time_set(uint32_t time);

uint32_t bc_device_info_app_reset_time_get(void);

bc_device_sleep *bc_device_info_sleep_get(void);

bool bc_device_info_sleep_set(bc_device_sleep *device_sleep);

bc_device_six_axis_config *bc_device_six_axis_config_get(void);

bool bc_device_six_axis_config_set(bc_device_six_axis_config *six_axis_config);

bool bc_device_info_set_ppg_file_flag(uint32_t flag);

void bc_device_info_get_ppg_file_flag(uint32_t *flag);

bool bc_device_info_set_ppg_file_name(char *file_name);

void bc_device_info_get_ppg_file_name(char *file_name);

bool bc_device_info_set_ppg_file_mode(uint32_t file_mode);

void bc_device_info_get_ppg_file_resume_count(uint32_t *resume_count);

bool bc_device_info_set_ppg_file_resume_count(void);

bool bc_device_info_clear_ppg_file_resume_count(void);

uint32_t bc_device_info_get_ppg_file_mode(void);

bool bc_device_info_set_audio_up_mode(uint32_t mode);

uint32_t bc_device_info_get_audio_up_mode(void);

bool bc_device_nfc_info_set(bc_device_nfc_info  *nfc_info);

bc_device_nfc_info  *bc_device_nfc_info_get(void);

bool bc_device_reset_io_flag_set(uint8_t flag);

void bc_device_identity_info_get(uint8_t *identity_info);

bool bc_device_identity_info_set(uint8_t *identity_info, uint16_t length);

bool bc_device_mac_set(uint8_t *mac);

void bc_device_mac_get(uint8_t *mac);

bool bc_device_sport_info_set(uint8_t mode,uint32_t start_time,uint32_t stop_time);

void bc_device_sport_info_get(uint8_t *mode,uint32_t *start_time,uint32_t *stop_time);

void bc_device_shut_down_time_get(uint32_t *time);

bool bc_device_shut_down_time_set(uint32_t time);

bool bc_device_info_set_capture_audio_file_name(char *file_name);

void bc_device_info_get_capture_audio_file_name(char *file_name);

#if defined(BLE_MULTI_MASTER)
	
void bc_device_info_ble_host_device_list_get(bc_device_ble_host_device *host_device_list);

bool bc_device_info_ble_host_device_list_set(bc_device_ble_host_device *host_device_list);	
	
#endif // defined(BLE_MULTI_MASTER)	


#if defined(ALARM_CLOCK)
    
void bc_device_info_alarm_clock_info_get(struct bc_device_alarm_clock_info *alarm_clock_info);

bool bc_device_info_alarm_clock_info_set(struct bc_device_alarm_clock_info *alarm_clock_info);

#endif //ALARM_CLOCK

#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
bool bc_device_info_set_vbat_type(uint8_t type);
uint8_t bc_device_info_get_vbat_type(void);
#endif

#if defined(HANDWARE_1_23_3)
bool bc_device_info_linear_motor_config_set(uint16_t duration_ms, uint8_t vib_count, uint8_t vib_gain, uint16_t interval_ms);
void bc_device_info_linear_motor_config_get(uint16_t *duration_ms, uint8_t *vib_count, uint8_t *vib_gain, uint16_t *interval_ms);
#endif

#if defined(HANDWARE_1_23_2_ONE_SEC)
void bc_device_info_timer_record_config_get(uint8_t *enable, uint32_t *interval, uint32_t *duration);
bool bc_device_info_timer_record_config_set(uint8_t enable, uint32_t interval, uint32_t duration);
#endif

#if defined(HANDWARE_1_23_2)
void bc_device_info_led_motor_mode_info_get(bc_device_led_motor_mode_info *info);
bool bc_device_info_led_motor_mode_info_set(bc_device_led_motor_mode_info *info);
#endif

void bc_device_cang_mac_get(uint8_t *info);

bool bc_device_cang_mac_set(uint8_t *info);

#endif













