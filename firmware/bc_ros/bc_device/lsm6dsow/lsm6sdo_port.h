#ifndef __LSM6SDO_PORT_H__
#define __LSM6SDO_PORT_H__

#include <stdint.h>
#include <stdbool.h>
#include "lsm6dso.h"

#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
/* ============ SPI 设备管理 ============ */

/* 查找 SPI 设备（系统初始化阶段调用） */
void lsm6sdo_spi_device_find(void);

/* 打开 SPI 总线 */
void lsm6sdo_spi_open(void);

/* 关闭 SPI 总线 */
void lsm6sdo_spi_close(void);

#endif

/* ============ LSM6DSOW 功能接口 ============ */

uint8_t lsm6sdo_init(void);

uint8_t lsm6sdo_get_chip_id(void);
void lsm6sdo_get_steps(uint16_t *steps);
void lsm6sdo_clear_steps(void);
void lsm6sdo_enable_anymotion(void);
void lsm6sdo_disable_anymotion(void);
void lsm6sdo_get_accData(LSM6DSO_Axes_t *data);
void lsm6sdo_change_acc_odr(uint8_t odr);
void lsm6sdo_get_data_from_fifo(LSM6DSO_AxesRaw_t *acc_data,LSM6DSO_AxesRaw_t *gyr_data,uint8_t *data_num);
void lsm6sdo_sport_state(uint8_t odr);
void lsm6sdo_silent_state(void);
void lsm6sdo_on_and_off(bool status);

#endif

