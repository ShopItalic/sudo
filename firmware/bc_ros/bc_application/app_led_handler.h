#ifndef __APP_LED_HANDLER_H__
#define __APP_LED_HANDLER_H__



#include "stdint.h"


enum app_led_timer_event
{
	APP_LED_CYCLE_FLASH_ON_TIMER_EVENT = 0,
	APP_LED_CYCLE_FLASH_OFF_TIMER_EVENT,
	APP_LED_TIMER_NUM
};


void app_led_falsh_handler(uint8_t cycle,uint8_t *model,uint8_t length);

void app_led_init(void);



#endif


