#ifndef __BC_IC_LED_H__
#define __BC_IC_LED_H__



#include "stdint.h"

#if defined(HANDWARE_1_23_4) || defined(HANDWARE_1_23_2_ONE_SEC)
/* 呼吸灯模式 */
enum bc_ic_led_breathing_mode
{
    LED_BREATHING_FAST = 0,   /* 快闪：0.5秒一个周期 */
    LED_BREATHING_SLOW,       /* 慢闪：2秒一个周期 */
    LED_BREATHING_MODE_MAX
};

/* 启动呼吸灯
 * mode  - 呼吸灯模式（快闪/慢闪）
 * count - 呼吸循环次数（0=无限循环，1~N=呼吸N次后自动停止）
 * g/r/b - 目标颜色最大亮度(0-255)
 */
void bc_ic_led_breathing_start(enum bc_ic_led_breathing_mode mode, uint8_t count, uint8_t g, uint8_t r, uint8_t b);

/* 停止呼吸灯 */
void bc_ic_led_breathing_stop(void);
#endif

#if defined(HANDWARE_1_23_2_ONE_SEC)
/* 文件同步LED指示 */
void bc_ic_led_file_sync_start(void);  /* 蓝色慢闪 */
void bc_ic_led_file_sync_stop(void);   /* 停止蓝色呼吸灯 → 绿灯亮1秒 → 熄灭 */
#endif

#if defined(HANDWARE_1_23_4)
/* 长按录音LED指示 */
void bc_ic_led_hold_recording_online_on(void);   /* 在线：绿灯长亮 */
void bc_ic_led_hold_recording_offline_on(void);  /* 离线：紫灯长亮 */
void bc_ic_led_hold_recording_off(void);         /* 关闭LED */

/* 电量LED指示（未充电时，亮2秒后熄灭）
 * percent >= 30 : 绿灯
 * 10 <= percent < 30 : 黄灯
 * percent < 10 : 红灯
 */
void bc_ic_led_battery_indication(uint8_t percent);

/* 录音暂停LED指示 - 黄灯慢闪 */
void bc_ic_led_recording_pause_on(void);
/* 恢复录音LED指示 - 恢复30%绿灯 */
void bc_ic_led_recording_pause_off(void);
#endif

void bc_ic_led_stop(void);

void bc_ic_led_set(uint8_t* rgb_data);

void bc_ic_led_breathing_light_start(void);

void bc_ic_led_breathing_light_stop(void);

void bc_led_device_find(void);

void bc_ic_led_init(void);


void bc_ic_led_pdm_on(void);

void bc_ic_led_pdm_off(void);

void bc_id_led_clear(void);

void bc_ic_led_test_cmd(uint8_t g,uint8_t r,uint8_t b);

void bc_ic_led_ble_connect(void);

void bc_ic_led_ble_connect_from_isr(void);

void bc_ic_led_ble_disconnect(void);

void bc_ic_led_ble_disconnect_from_isr(void);

void bc_ic_led_mic_offline_recording_on(void);

void bc_ic_led_mic_online_recording_on(void);

void bc_ic_led_mic_offline_recording_capture_on(void);

void bc_ic_led_mic_online_recording_capture_on(void);

void bc_ic_led_mic_offline_recording_off(void);

void bc_ic_led_mic_online_recording_off(void);

void bc_ic_led_mic_offline_recording_capture_off(void);

void bc_ic_led_mic_online_recording_capture_off(void);

#endif
