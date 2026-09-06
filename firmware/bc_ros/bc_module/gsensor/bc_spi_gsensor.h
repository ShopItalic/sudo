#ifndef __BC_SPI_GSENSOR_H__
#define __BC_SPI_GSENSOR_H__

#include "ring_config.h"
#include <stdint.h>
#include <stdbool.h>

#define G_SENSOR_TIMER_NUM 1

enum g_sensor_result
{
    G_SENSOR_SUCCESS = 0,
    G_SENSOR_FAILD,
};

/* ============ 设备查找 ============ */

void bc_spi_gsensor_device_find(void);

/* ============ 初始化与检测 ============ */

enum g_sensor_result bc_gsensor_init(void);
enum g_sensor_result bc_gsensor_init_status(void);
uint8_t bc_gsensor_getId(void);
bool bc_gsensor_id_hardware_check(void);
bool bc_gsensor_hardware_check(void);

/* ============ 运动状态 ============ */

void bc_gsensor_set_sport_state(uint8_t odr);

/* ============ 步数 ============ */

uint32_t bc_gsensor_getStep(void);
void bc_gsensor_clearSteps(void);

/* ============ 中断控制 ============ */

void bc_gsensor_irqOn(void);
void bc_gsensor_irqOff(void);
bool bc_g_sensor_int_irq_register_callback(const void *error_callback);
bool bc_g_sensor_tap_irq_register_callback(void *callback);
bool bc_g_sensor_any_motion_irq_register_callback(void *callback);
void bc_g_sensor_irq_reg(void);

/* ============ 数据读取 ============ */

void bc_gsensor_dataRead(int *pdata);
void bc_gsensor_Gyroscope_dataRead(int *pdata);
void bc_gsensor_RawData_dataRead(void *pdata_Accelerometer, void *Gyroscope);
void bc_gsensor_fifoRead(int16_t rdata[][3]);

/* ============ 加速度+陀螺仪状态 ============ */

bool bc_g_sensor_acc_and_gyro_status(void);
void bc_g_sensor_acc_and_gyro(void);
void bc_g_sensor_acc_and_gyro_config(uint8_t acc);

/* ============ 运动次数 ============ */

uint8_t bc_gsensor_sport_num_get(void);
void bc_gsensor_sport_num_clear(void);

#endif
