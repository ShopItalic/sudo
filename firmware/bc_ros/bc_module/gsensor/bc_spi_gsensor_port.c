/*
 * bc_spi_gsensor_port.c - SPI GSensor 端口层（基于 spi_3 软件模拟 SPI）
 *
 * 说明: 为 SPI 版本的 GSensor 模块提供底层 SPI 操作接口
 *       基于 bc_spi3 (spi_3) 软件模拟 SPI 驱动
 *       与 bc_g_sensor_device_port.c 功能对等，但使用 SPI 总线
 */

#include "bc_spi_gsensor_port.h"

#include "q_device.h"
#include <string.h>

#include "bc_ldo_switch.h"
#include "bc_delay.h"

/* SPI 设备句柄 */
static q_device_t *spi_gsensor_dev = NULL;

/* SPI 数据包 */
static struct spi_package spi_pack = {0};

/* ============ I2C 兼容接口（空实现，保留接口兼容） ============ */

bool bc_g_sensor_i2c_write(uint8_t slave_addr, uint8_t reg_addr,
                           uint8_t *write_data, uint8_t write_length)
{
    (void)slave_addr;
    (void)reg_addr;
    (void)write_data;
    (void)write_length;
    return true;
}

bool bc_g_sensor_i2c_read(uint8_t slave_addr, uint8_t reg_addr,
                          uint8_t *read_data, uint8_t read_length)
{
    (void)slave_addr;
    (void)reg_addr;
    (void)read_data;
    (void)read_length;
    return true;
}

/* ============ SPI 设备管理 ============ */

void bc_spi_gsensor_device_open(void)
{
    if (spi_gsensor_dev == NULL) {
        return;
    }
    bc_ldo_imu_power_on();
    q_device_open(spi_gsensor_dev);
    bc_delay_ms(1);
}

void bc_spi_gsensor_device_close(void)
{
    if (spi_gsensor_dev == NULL) {
        return;
    }
    q_device_close(spi_gsensor_dev);
    bc_ldo_imu_power_off();
}

/* ============ CS 片选控制 ============ */

void bc_spi_gsensor_cs_high(void)
{
    if (spi_gsensor_dev == NULL) {
        return;
    }
    q_device_ctrl(spi_gsensor_dev, GPIO_OUTPUT_HIGH, 0);
}

void bc_spi_gsensor_cs_low(void)
{
    if (spi_gsensor_dev == NULL) {
        return;
    }
    q_device_ctrl(spi_gsensor_dev, GPIO_OUTPUT_LOW, 0);
}

/* ============ SPI 读写传输 ============ */

bool bc_spi_gsensor_write_and_read(uint8_t *write_buff, uint32_t write_length,
                                   uint8_t *read_buff, uint32_t read_length)
{
    bool ret = false;

    if (spi_gsensor_dev == NULL || write_buff == NULL || write_length == 0) {
        return false;
    }

    spi_pack.write_buff   = write_buff;
    spi_pack.read_buff    = read_buff;
    spi_pack.write_length = (uint8_t)write_length;
    spi_pack.read_length  = (uint8_t)read_length;

    if (q_device_write(spi_gsensor_dev, 0, &spi_pack, 0) == RESULT_OK) {
        ret = true;
    }

    return ret;
}

/* ============ 设备查找 ============ */

void bc_spi_gsensor_port_device_find(void)
{
    spi_gsensor_dev = q_device_find("spi_3");
    q_device_assert(spi_gsensor_dev);
    BC_LOG_INFO("spi gsensor: found spi_3 device\r\n");
}
