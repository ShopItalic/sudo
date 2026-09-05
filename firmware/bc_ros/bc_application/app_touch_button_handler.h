#ifndef __APP_TOUCH_BUTTON_HANDLER_H__
#define __APP_TOUCH_BUTTON_HANDLER_H__

#include <stdint.h>
#include "app_package.h"
#include <stdbool.h>

struct touch_package
{
	uint8_t frame_type;
	uint8_t frame_id;
	uint8_t cmd;
	uint8_t subcmd;
	uint8_t data[10];
};

struct touch_sensor_package
{
	struct touch_package sensor_pack;
	uint8_t length;
};


enum app_touch_task_event
{
	APP_TOUCH_PROCESS_EVENT = 0,
	APP_TOUCH_ERROR_EVENT,
	APP_TOUCH_INIT_EVENT,
	APP_TOUCH_UNINIT_EVENT,
	APP_TOUCH_RAWDATA_EVENT,
	APP_TOUCH_EVENT_NUM
};


enum app_touch_button_timer_event
{
	APP_TOUCH_BUTTON_CHECK_TIMER_EVENT = 0,
	APP_TOUCH_LONG_PRESS_TIMER_EVENT,
	APP_TOUCH_CHECK_TIMER_EVENT,
	APP_TOUCH_POLL_TIMER_EVENT,
	APP_TOUCH_LONG_PRESS_TIMEOUT_TIMER_EVENT,
	APP_TOUCH_TIMER_NUM
};

extern uint32_t pdm_stop_utime;

void app_touch_pdm_key_flag_clear(void);
void app_touch_pdm_key_flag_set(bool flag);

void app_touch_handler_init(void);


void app_touch_init_event(void);

void app_touch_uninit_event(void);


void app_button_handware_chek_start(void);

void app_button_handware_chek_stop(void);

void app_touch_check_start(struct app_cmd_package * pack);

bool app_touch_pdm_audio_status_get(void);

void app_touch_pdm_audio_stop(void);

void app_touch_low_power(void);

void app_touch_test(void);

void app_touch_button_poll(void);

#endif


