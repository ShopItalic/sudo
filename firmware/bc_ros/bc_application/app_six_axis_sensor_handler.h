#ifndef __APP_SIX_AXIS_SENSOR_HANDLER_H__
#define __APP_SIX_AXIS_SENSOR_HANDLER_H__

#include "app_package.h"
#include "ring_config.h"

struct sensor_package
{
	uint8_t frame_type;
	uint8_t frame_id;
	uint8_t cmd;
	uint8_t subcmd;
	
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
    uint8_t data[70];
#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688
	uint8_t data[130];
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW
	uint8_t data[130];
#else
	uint8_t data[130];
#endif	
	
};

struct imu_sensor_package
{
	struct sensor_package sensor_pack;
	uint8_t length;
};

enum app_six_axis_sensor_time_type
{
	SIX_AXIS_SENSOR_READ_TIME = 0,
	SIX_AXIS_SENSOR_TIME_TYPE_NUM,
};


void app_six_axis_sensor_event(struct app_cmd_package * pack);

void app_six_axis_sensor_time_create(void);


void app_six_axis_sensor_stop(void);

void app_imu_poll(void);

#endif











