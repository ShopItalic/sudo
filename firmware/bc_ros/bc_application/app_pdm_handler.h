#ifndef __APP_PDM_HANDLER_H__
#define __APP_PDM_HANDLER_H__


#include "app_package.h"

#include "stdbool.h"

enum app_pdm_mode
{
  PDM_MODE_IDIE = 0,
  PDM_MODE_ONLINE,
  PDM_MODE_OFFLINE,
  PDM_MODE_KEY_OFFLINE
};

enum app_pdm_mode app_pdm_mode_get(void);

void app_pdm_start(struct app_cmd_package * pack);

void app_pdm_stop(struct app_cmd_package * pack);

void app_pdm_thread_create(void);

void app_pdm_ble_stop(void);

void app_pdm_touch_start(void);

void app_pdm_touch_stop(void);
bool app_pdm_work_status(void);
void app_pdm_audio_discooenct_stop(void);

bool app_pdm_recording_start(void);
bool app_pdm_recording_stop(void);
bool app_pdm_capture_recording_start(void);
bool app_pdm_capture_recording_stop(void);

void app_pdm_mode_change_to_online(void);
bool app_pdm_switch_online_to_offline(void);

#if defined(HANDWARE_1_23_4)
/* 录音暂停功能 */
bool app_pdm_recording_pause(void);
bool app_pdm_recording_resume(void);
bool app_pdm_recording_is_paused(void);
#endif

#if defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_2_ONE_SEC)
// 蓝牙连接时设置PDM BLE发送延迟（等待GATT就绪）
void app_pdm_ble_connect_set_tx_delay(void);
#if defined(HANDWARE_1_23_3)
bool app_pdm_offline_on_get(void);
#endif
#if defined(HANDWARE_1_23_2_ONE_SEC)
void app_timer_record_start(void);
void app_timer_record_stop(void);

/* 录音优先级：0=无，1=定时，2=双击，3=APP */
uint8_t app_pdm_get_record_priority(void);
void app_pdm_set_next_record_priority(uint8_t prio);
#endif
#endif

#endif


