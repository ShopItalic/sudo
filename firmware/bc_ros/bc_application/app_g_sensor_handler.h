#ifndef __APP_G_SENSOR_HANDLER_H__
#define __APP_G_SENSOR_HANDLER_H__

#include "stdint.h"
#include "stdbool.h"

enum app_g_sensor_timer_type
{
	APP_G_SENSOR_START_TIMER = 0,
	APP_G_SENSOR_SPORT_STEP_TIMER,
	APP_G_SENSOR_TIMER_TYPE_NUM
};

enum app_g_sensor_data_time_type
{
    G_SENSOR_DATA_GET = 0,
    G_SENSOR_TIMER_TYPE_NUM,
};

enum app_g_sensor_task_event
{
	APP_G_SENSOR_SPORT_STORAGE_TASK_EVENT = 0,
	APP_G_SENSOR_EVENT_NUM
};



void app_g_sensor_time_create(void);

uint16_t app_g_sensor_sport_step_count_get(void);
void app_g_sensor_sport_step_count_clear(void);


#if (defined(HANDWARE_1_14_1))			

bool app_gsensor_gyro_data_read_callback(uint8_t *data_buff,uint16_t *data_count);


#endif	





#endif



