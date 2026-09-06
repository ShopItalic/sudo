#ifndef __BC_LINEAR_MOBOR_IC_H__
#define __BC_LINEAR_MOBOR_IC_H__


#include <stdbool.h>
#include <stdint.h>
#include "bc_device_info.h"

enum {
    CMD_MOTOR_START = 1,
    CMD_MOTOR_GET_ID,
    CMD_CW221X_GET_ID,
    CMD_CW221X_GET_CAP,
    CMD_TX1812_GET_TEMP_ID,
};

typedef struct {
    uint8_t cmd;
    uint8_t subcmd;
    uint8_t data[10];
} STR_IIC1_DATA;

extern uint8_t iic1_busy;
extern STR_IIC1_DATA diic1;

void bc_linear_motor_ic_device_init(void);
uint8_t app_linear_motor_ic_start(uint8_t count);   /* count=震动次数，0=无限循环 */
#if defined(HANDWARE_1_23_3)
uint8_t app_linear_motor_ic_start_config(bc_device_linear_motor_info *config);
#endif
void app_linear_motor_ic_stop(void);
uint8_t app_linear_motor_get_id(void *params);
uint8_t app_cw221x_get_id(void *params);
uint8_t app_cw221x_get_cap(void *params);
uint8_t bc_temper_get_id(void * params);

void bc_linear_motor_config(uint16_t pwm_seq_values,uint8_t playback_count,uint16_t repeats);
uint8_t app_linear_motor_ic_start_test(void);
uint8_t app_linear_motor_ic_start_timer_vib(uint32_t delay, uint16_t type_vib);

#endif
