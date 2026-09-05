#ifndef __APP_TEMPER_HANDLER_H__
#define __APP_TEMPER_HANDLER_H__



#include <stdint.h>


enum app_temper_time_type
{
	TEMPER_COLLECTION_PROGRESS_TIME = 0,
	TEMPER_TIME_TYPE_NUM,
};


void app_temper_collection_fail(void);

void app_temper_time_create(void);

void app_temper_collection_progress(uint8_t frame_id,uint8_t sub);



#endif
