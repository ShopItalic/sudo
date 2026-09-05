
#ifndef MOBVOI_MOTION_SLEEP_INTERFACE_H_
#define MOBVOI_MOTION_SLEEP_INTERFACE_H_

#include <stdint.h>
#include <math.h>

/*//#define TXTDEBUG*/

/*
type返回睡眠状态：
0：非睡眠状态；
7：睡眠中的清醒状态；
8：快速眼动期；
9：浅睡眠；
10：深度睡眠
99：未定义的状态
*/
typedef void (*sleep_analysis_type_cb)(int timediff, int type);

/**
 * Initialize a sleep analyzer.
 *
 * @param arg  WILL NOT CHANGE on sleep analyzer, will be passed to the callback function.
 * @param update_callback Will be called when updating activity scores.
 */
void mobvoi_sleep_analysis_init(sleep_analysis_type_cb update_callback);

/**
 * Reset the instance.
 */
void mobvoi_sleep_analysis_reset(void);

/**
 * Clean up the instance.
 */
void mobvoi_sleep_analysis_cleanup(void);

/**
 * Process accelerometer magnitude to obtain activity scores.
 *
 * @param timestamp Accelerometer event timestamp.
 * @param ax ay az  Accelerometer x y z. The unit is gravity acceleration.
 * @param rri（ms）
 */
void mobvoi_sleep_analysis_process(uint16_t timestamp, float ax, float ay, float az, float rri);

#endif 
