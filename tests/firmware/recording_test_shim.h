#ifndef RECORDING_TEST_SHIM_H
#define RECORDING_TEST_SHIM_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Match the candidate's production 603V1.23.2 voice profile. */
#define SUDO_VOICE_ONLY 1
#define HANDWARE_1_23_1 1
#define HANDWARE_1_23_2 1
#define HARDWARE_1231_ENABLED 1

#include "app_cmd_handler.h"
#include "app_package.h"
#include "app_pdm_handler.h"
#include "bc_ble_modu_interface.h"
#include "bc_queue.h"

bool bc_queue_enqueue(bc_queue_type queue_type, void *enqueue_data);
bool bc_queue_isr_enqueue(bc_queue_type queue_type, void *enqueue_data);
bool bc_queue_isr_enqueue_not_yield(bc_queue_type queue_type, void *enqueue_data);

bool bc_device_info_set_audio_up_mode(uint8_t mode);
uint8_t bc_device_info_get_audio_up_mode(void);
void app_touch_pdm_key_flag_clear(void);

uint8_t app_cmd_set_time_callback(struct app_cmd_package *package);
uint8_t app_cmd_get_version_callback(struct app_cmd_package *package);
uint8_t app_cmd_get_vbat(struct app_cmd_package *package);
uint8_t app_cmd_get_hrv(struct app_cmd_package *package);
uint8_t app_cmd_get_spo2(struct app_cmd_package *package);
uint8_t app_cmd_get_tempertion(struct app_cmd_package *package);
uint8_t app_cmd_get_step_count(struct app_cmd_package *package);
uint8_t app_cmd_get_hrstory(struct app_cmd_package *package);
uint8_t app_cmd_set_sys(struct app_cmd_package *package);
void app_test_cmd_handler(struct app_cmd_package *package);
uint8_t app_cmd_get_ppg_spo2(struct app_cmd_package *package);
uint8_t app_cmd_puf(struct app_cmd_package *package);
uint8_t app_cmd_authentication(struct app_cmd_package *package);
uint8_t app_cmd_nfc(struct app_cmd_package *package);
uint8_t app_cmd_six_axis_sensor(struct app_cmd_package *package);
uint8_t app_cmd_get_ir(struct app_cmd_package *package);
uint8_t app_cmd_led(struct app_cmd_package *package);
uint8_t app_cmd_hid(struct app_cmd_package *package);
uint8_t app_cmd_config_touch(struct app_cmd_package *package);
uint8_t app_cmd_led_motor_mode_set(struct app_cmd_package *package);
uint8_t app_cmd_led_motor_mode_get(struct app_cmd_package *package);
uint8_t app_cmd_motor(struct app_cmd_package *package);
uint8_t app_cmd_port_mode(struct app_cmd_package *package);
uint8_t app_cmd_app_event(struct app_cmd_package *package);
uint8_t app_cmd_rtc_alarm_clock_event(struct app_cmd_package *package);
uint8_t app_cmd_ppg_led_data_get(struct app_cmd_package *package);
uint8_t app_cmd_wifi_event(struct app_cmd_package *package);
uint8_t app_cmd_ipc_event(struct app_cmd_package *package);

#endif
