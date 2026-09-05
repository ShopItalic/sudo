/*
 * bc_spi_gsensor.c - SPI 版本 GSensor 模块（LSM6DSOW）
 *
 * 说明: 对应 HANDWARE_1_23_3 / HANDWARE_1_23_4 硬件版本
 *       GSensor 通过 SPI 接口（spi_3 软件模拟）连接
 *       功能与 bc_gsensor.c 对等，对外 API 完全一致
 */

#include "bc_spi_gsensor.h"

#include "q_device.h"
#include <string.h>
#include <stdlib.h>

#include "bc_spi_gsensor_port.h"
#include "bc_delay.h"
#include "bc_logger.h"
#include "bc_rtos.h"
#include "bc_device_info.h"

#if (G_SENSOR_DEVIECE_TYPE == 4)  /* LSM6DSOW */
#include "lsm6sdo_port.h"
#endif

static q_device_t *g_sensor_int_device_handler;
static void sport_count_timer_callback(void *pvParameter);

typedef void (*g_sensor_int_irq_callback)(void);

static g_sensor_int_irq_callback g_sensor_irq_callback = NULL;

static uint8_t sport_num = 0;
static bool bc_g_sensor_acc_and_gyro_status_flag = false;

/* GSensor 相关定时器 */
static bc_rtos_timer_struct g_sensor_timer[G_SENSOR_TIMER_NUM] = {
    {
        .timer_name = "sport_count_timer",
        .uxAutoReload = true,
        .xTimerPeriodInTicks = 1000 * 5,
        .timer_callback_function = sport_count_timer_callback,
    },
};

/* ============ 中断处理 ============ */

static void bc_g_sensor_int_callback(uint8_t pin, uint8_t pin_status)
{
    (void)pin;
    (void)pin_status;

    BC_LOG_INFO("bc_g_sensor_int_callback sport_num:%d \r\n", sport_num);

    q_device_close(g_sensor_int_device_handler);
    bc_rtos_timer_start(g_sensor_timer[0].timer_handler, 50);
    sport_num += 1;

    if (g_sensor_irq_callback != NULL) {
        g_sensor_irq_callback();
    }
}

static void sport_count_timer_callback(void *pvParameter)
{
    (void)pvParameter;

    BC_LOG_INFO("sport_count_timer_callback");

    q_device_open(g_sensor_int_device_handler);
    q_device_reg_callback(g_sensor_int_device_handler, 0, bc_g_sensor_int_callback);
    bc_rtos_timer_stop(g_sensor_timer[0].timer_handler, 50);
}

static void gsensor_int_timer_create(void)
{
    for (uint8_t i = 0; i < G_SENSOR_TIMER_NUM; i++) {
        g_sensor_timer[i].timer_handler = bc_rtos_timer_create(
            g_sensor_timer[i].timer_name,
            g_sensor_timer[i].xTimerPeriodInTicks,
            g_sensor_timer[i].uxAutoReload,
            (void *)g_sensor_timer[i].timer_id,
            g_sensor_timer[i].timer_callback_function);

        if (g_sensor_timer[i].timer_handler != NULL) {
            BC_LOG_INFO("create %s succeed\r\n", g_sensor_timer[i].timer_name);
        }
    }
}

static void bc_gsensor_int_init(void)
{
    q_device_open(g_sensor_int_device_handler);
    q_device_reg_callback(g_sensor_int_device_handler, 0, bc_g_sensor_int_callback);
    gsensor_int_timer_create();
}

static void bc_g_sensor_int_find(void)
{
    g_sensor_int_device_handler = q_device_find("acc_int_1");
    q_device_assert(g_sensor_int_device_handler);
}

/* ============ 设备查找 ============ */

void bc_spi_gsensor_device_find(void)
{
    /* 查找 SPI 总线设备（端口层） */
    bc_spi_gsensor_port_device_find();

    /* 查找中断输入设备 */
    bc_g_sensor_int_find();

#if (G_SENSOR_DEVIECE_TYPE == 4)  /* LSM6DSOW */
    /* LSM6DSOW 端口层设备查找 */
    lsm6sdo_spi_device_find();
#endif
}

/* ============ 初始化 ============ */

enum g_sensor_result bc_gsensor_init(void)
{
    bc_spi_gsensor_device_open();

#if (G_SENSOR_DEVIECE_TYPE == 4)  /* LSM6DSOW */
    lsm6sdo_init();
    bc_gsensor_set_sport_state(25);
    lsm6sdo_disable_anymotion();
    lsm6sdo_on_and_off(false);
#endif

    bc_gsensor_clearSteps();
    bc_gsensor_int_init();

    bc_spi_gsensor_device_close();

    return G_SENSOR_SUCCESS;
}

/* ============ 初始化状态检查 ============ */

enum g_sensor_result bc_gsensor_init_status(void)
{
    /* 仅设置状态标志，无需操作 SPI 设备，避免并发访问导致的问题 */
    bc_g_sensor_acc_and_gyro_status_flag = false;
    return G_SENSOR_SUCCESS;
}

/* ============ 设备 ID 读取 ============ */

uint8_t bc_gsensor_getId(void)
{
    uint8_t id = 0;

    bc_spi_gsensor_device_open();

#if (G_SENSOR_DEVIECE_TYPE == 4)  /* LSM6DSOW */
    id = lsm6sdo_get_chip_id();
#endif

    bc_spi_gsensor_device_close();

    return id;
}

bool bc_gsensor_id_hardware_check(void)
{
    uint8_t id = bc_gsensor_getId();

#if (G_SENSOR_DEVIECE_TYPE == 4)  /* LSM6DSOW */
    if (id == 0x6C) {
        return true;
    }
    return false;
#else
    return false;
#endif
}

bool bc_gsensor_hardware_check(void)
{
    int32_t pdata[3] = {0};

    bc_spi_gsensor_device_open();

#if (G_SENSOR_DEVIECE_TYPE == 4)  /* LSM6DSOW */
    LSM6DSO_Axes_t acc_data = {0};
    lsm6sdo_get_accData(&acc_data);
    pdata[0] = acc_data.x;
    pdata[1] = acc_data.y;
    pdata[2] = acc_data.z;
#endif

    bc_spi_gsensor_device_close();

    if (abs(pdata[0]) > 2400 && abs(pdata[1]) > 2400 && abs(pdata[2]) > 2400) {
        return false;
    }
    return true;
}

/* ============ 运动状态设置 ============ */

void bc_gsensor_set_sport_state(uint8_t odr)
{
    bc_spi_gsensor_device_open();

#if (G_SENSOR_DEVIECE_TYPE == 4)  /* LSM6DSOW */
    lsm6sdo_sport_state(odr);
#endif

    bc_spi_gsensor_device_close();
}

/* ============ 步数读取 ============ */

uint32_t bc_gsensor_getStep(void)
{
    uint16_t step = 0;

    bc_spi_gsensor_device_open();

#if (G_SENSOR_DEVIECE_TYPE == 4)  /* LSM6DSOW */
    lsm6sdo_get_steps(&step);
#endif

    bc_spi_gsensor_device_close();

    return (uint32_t)step;
}

void bc_gsensor_clearSteps(void)
{
    bc_spi_gsensor_device_open();

#if (G_SENSOR_DEVIECE_TYPE == 4)  /* LSM6DSOW */
    lsm6sdo_clear_steps();
#endif

    bc_spi_gsensor_device_close();
}

/* ============ AnyMotion 控制 ============ */

void bc_gsensor_irqOn(void)
{
    bc_spi_gsensor_device_open();

#if (G_SENSOR_DEVIECE_TYPE == 4)  /* LSM6DSOW */
    lsm6sdo_enable_anymotion();
#endif

    bc_spi_gsensor_device_close();
}

void bc_gsensor_irqOff(void)
{
    bc_spi_gsensor_device_open();

#if (G_SENSOR_DEVIECE_TYPE == 4)  /* LSM6DSOW */
    lsm6sdo_disable_anymotion();
#endif

    bc_spi_gsensor_device_close();
}

void bc_g_sensor_irq_reg(void)
{
    bc_spi_gsensor_device_open();
    bc_spi_gsensor_device_close();
}

/* ============ 加速度数据读取 ============ */

void bc_gsensor_dataRead(int *pdata)
{
#if (G_SENSOR_DEVIECE_TYPE == 4)  /* LSM6DSOW */
    LSM6DSO_Axes_t acc_data = {0};

    bc_spi_gsensor_device_open();
    lsm6sdo_get_accData(&acc_data);
    pdata[0] = acc_data.x;
    pdata[1] = acc_data.y;
    pdata[2] = acc_data.z;
    bc_spi_gsensor_device_close();
#else
    (void)pdata;
#endif
}

/* ============ 陀螺仪数据读取 ============ */

void bc_gsensor_Gyroscope_dataRead(int *pdata)
{
    (void)pdata;
    /* 当前未启用陀螺仪 */
}

/* ============ 加速度+陀螺仪原始数据读取 ============ */

void bc_gsensor_RawData_dataRead(void *pdata_Accelerometer, void *Gyroscope)
{
    bc_gsensor_dataRead((int *)pdata_Accelerometer);
    bc_gsensor_Gyroscope_dataRead((int *)Gyroscope);
}

/* ============ FIFO 数据读取 ============ */

void bc_gsensor_fifoRead(int16_t rdata[][3])
{
#if (G_SENSOR_DEVIECE_TYPE == 4)  /* LSM6DSOW */
    LSM6DSO_AxesRaw_t acc_data[128] = {0};
    LSM6DSO_AxesRaw_t gyr_data[128] = {0};
    uint8_t data_num = 0;

    bc_spi_gsensor_device_open();
    lsm6sdo_get_data_from_fifo(acc_data, gyr_data, &data_num);
    for (uint8_t i = 0; i < data_num && i < 128; i++) {
        rdata[i][0] = acc_data[i].x;
        rdata[i][1] = acc_data[i].y;
        rdata[i][2] = acc_data[i].z;
    }
    bc_spi_gsensor_device_close();
#else
    (void)rdata;
#endif
}

/* ============ 中断回调注册 ============ */

bool bc_g_sensor_int_irq_register_callback(const void *error_callback)
{
    if (error_callback == NULL) {
        return false;
    }
    g_sensor_irq_callback = (g_sensor_int_irq_callback)error_callback;
    return true;
}

bool bc_g_sensor_tap_irq_register_callback(void *callback)
{
    (void)callback;
    return false;
}

bool bc_g_sensor_any_motion_irq_register_callback(void *callback)
{
    (void)callback;
    return false;
}

/* ============ 加速度+陀螺仪状态 ============ */

bool bc_g_sensor_acc_and_gyro_status(void)
{
    return bc_g_sensor_acc_and_gyro_status_flag;
}

void bc_g_sensor_acc_and_gyro(void)
{
    bc_spi_gsensor_device_open();
    bc_g_sensor_acc_and_gyro_status_flag = true;
    bc_spi_gsensor_device_close();
}

void bc_g_sensor_acc_and_gyro_config(uint8_t acc)
{
    bc_spi_gsensor_device_open();
    bc_g_sensor_acc_and_gyro_status_flag = true;
    bc_spi_gsensor_device_close();
    (void)acc;
}

/* ============ 运动次数 ============ */

uint8_t bc_gsensor_sport_num_get(void)
{
    return sport_num;
}

void bc_gsensor_sport_num_clear(void)
{
    sport_num = 0;
}
