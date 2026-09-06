#ifndef __APP_MODEL_HANDLER_H__
#define __APP_MODEL_HANDLER_H__



#include <stdint.h>



enum app_model_status
{
	APP_MODEL_CHARGING_STATE = 0,   //³äµçÌ¬
	APP_MODEL_WORKING_STATE,        //¹¤×÷Ì¬
	APP_MODEL_SILENCE_STATE,        //¾²Ä¬Ì¬
	APP_MODEL_WAREHOUSING_STATE,    //²Ö´¢Ì¬
	APP_MODEL_HARDWARE_CHECK_STATE,    //Ó²¼þ×Ô¼ìÌ¬
	APP_MODEL_NUM,
};




enum app_model_timer_type
{
	APP_ENTER_SILENCE_TIMER = 0,
	APP_MODEL_TIMER_TYPE_NUM
};


void app_model_time_create(void);

void app_enter_silence_model_timer_update(void);

void app_model_state_set(enum app_model_status status);

enum app_model_status app_model_state_get(void);

void app_model_silence_timer_update(uint32_t time);















#endif










