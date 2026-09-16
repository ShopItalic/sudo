#ifndef FACTORY_RECORDING_CONNECTION_FIXTURE_H
#define FACTORY_RECORDING_CONNECTION_FIXTURE_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uintptr_t TaskHandle_t;
extern uint32_t fixture_ticks;
extern TaskHandle_t fixture_task;
#define pdMS_TO_TICKS(ms) ((uint32_t)(ms) * 1024U / 1000U)
#define pdTRUE 1
#define xTaskGetTickCount() fixture_ticks
#define xTaskGetCurrentTaskHandle() fixture_task
#define taskENTER_CRITICAL() ((void)0)
#define taskEXIT_CRITICAL() ((void)0)
#define xTaskNotifyGive(task) ((void)(task))
#define ulTaskNotifyTake(clear, ticks) ((void)(clear), (void)(ticks), 0U)
#define HARDWARE_1231_ENABLED 1
#define HANDWARE_1_23_2
#define PPG_ENABLED 0
#define BLE_CONNECT_IDIE_TIMEOUT_TIMER 1
#define PDM_MODE_ONLINE 1
#define PDM_MODE_OFFLINE 2
#define BC_QUEUE_TYPE_BLE_RECV 1

struct app_cmd_package {
    uint8_t frame_type, frame_id, cmd, subcmd, data[250], length;
};
struct bc_ble_data_package { uint8_t data[256]; unsigned data_length; };
bool app_pdm_work_status(void);
bool app_pdm_recording_start(void);
bool app_pdm_recording_stop(void);
void factory_capture_fault(void);
unsigned app_factory_controls_action(unsigned gesture);
unsigned app_factory_controls_hold_delay_ms(void);
void app_package_send_enqueue(struct app_cmd_package *p, unsigned length);
#endif
