#ifndef EULER_ATTITUDE_UPDATE_H
#define EULER_ATTITUDE_UPDATE_H

#include <math.h>
#include <string.h>
#include "icm42688.h"
#include "bc_gsensor.h"
#include "bc_logger.h"
#include "nrf_delay.h"
#include "app_ble_handler.h"
#include "app_package.h"

//定义手势类别
typedef enum {
    GESTURE_UNKOWN = 0,     //无手势
    GESTURE_SWIPE_UP = 1,       //上滑
    GESTURE_SWIPE_DOWN = 2,     //下滑
    GESTURE_SWIPE_LEFT = 3,     //左滑
    GESTURE_SWIPE_RIGHT = 4,    //右滑
	GESTURE_CLICK = 5,          //点击
	GESTURE_RIGHT_CLICK = 6,    //右键
} GestureType;


typedef enum {
    STATE_IDLE,
    STATE_WAITING_UP,
    STATE_WAITING_DOWN,
    STATE_SWIPE_UP,
    STATE_SWIPE_DOWN
} GestureState;

// 函数声明

void Euler_Attitude_Update(float* faccel, float* fgyro, float* Euler);
float calculate_pitch_difference(float current_pitch, float last_pitch);
int check_pitch_diff_condition(float* pitch_diffs, size_t count, float threshold);
GestureType alg_mouse_handler(int16_t *acc,int16_t *gyro);


#endif // EULER_ATTITUDE_UPDATE_H







