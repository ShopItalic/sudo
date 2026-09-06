#ifndef __APP_LINEAR_MOTOR_HANDLER_H__
#define __APP_LINEAR_MOTOR_HANDLER_H__


#include <stdint.h>
#include <stdbool.h>


/* 震动模式枚举 */
typedef enum
{
    VIBRATE_MODE_VERY_SHORT = 0,  /* 极短振 */
    VIBRATE_MODE_SHORT,           /* 短振 */
    VIBRATE_MODE_LONG,            /* 长振 */
    VIBRATE_MODE_MAX
} vibrate_mode_t;

void app_linear_motor_set(uint32_t time,uint8_t type);

void app_linear_motor_start(uint8_t type);

void app_linear_motor_stop(void);

void app_linear_motor_time_create(void);

/**
 * @brief  按模式启动震动，可指定震动次数
 * @param  mode   震动模式（VIBRATE_MODE_XXX）
 * @param  count  震动次数（0表示无限循环，需手动stop）
 * @return 0=成功，其他=失败
 */
uint8_t app_vibrate_start(vibrate_mode_t mode, uint8_t count);

/**
 * @brief  停止震动
 */
void app_vibrate_stop(void);

#endif



