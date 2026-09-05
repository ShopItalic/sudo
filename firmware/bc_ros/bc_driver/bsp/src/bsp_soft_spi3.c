/*
 * bsp_soft_spi3.c - SPI3 软件模拟驱动 (GPIO bit-banging)
 * 设备名称: "spi_3"
 * 说明: 使用普通 IO 口模拟 SPI 4 线时序，不依赖硬件 SPI 外设
 *       遵循 q_device 设备模型，与 bsp_spi1/bsp_spi2 接口兼容
 *
 * SPI 模式: Mode 0 (CPOL=0, CPHA=0)
 * 数据位宽: 8-bit, MSB first
 * 硬件版本: 支持 HANDWARE_1_23_3 等多版本引脚配置
 */

#include "q_device.h"
#include <string.h>

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)
#include "nrf_gpio.h"
#include "nrf_delay.h"
#endif

/* ============ 引脚配置表（按硬件版本区分） ============ */

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)

#if defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4)
/* HANDWARE_1_23_3 / 1_23_4: GSensor SPI 引脚 */
#define SPI3_CS_PIN     NRF_GPIO_PIN_MAP(0, 10)
#define SPI3_SCLK_PIN   NRF_GPIO_PIN_MAP(0, 14)
#define SPI3_MOSI_PIN   NRF_GPIO_PIN_MAP(0, 12)
#define SPI3_MISO_PIN   NRF_GPIO_PIN_MAP(1, 9)

#elif defined(HANDWARE_1_23_1)
/* HANDWARE_1_23_1 默认引脚（可根据实际原理图调整） */
#define SPI3_CS_PIN     NRF_GPIO_PIN_MAP(0, 10)
#define SPI3_SCLK_PIN   NRF_GPIO_PIN_MAP(0, 14)
#define SPI3_MOSI_PIN   NRF_GPIO_PIN_MAP(0, 12)
#define SPI3_MISO_PIN   NRF_GPIO_PIN_MAP(1, 9)

#else
/* 默认引脚 */
#define SPI3_CS_PIN     NRF_GPIO_PIN_MAP(0, 10)
#define SPI3_SCLK_PIN   NRF_GPIO_PIN_MAP(0, 14)
#define SPI3_MOSI_PIN   NRF_GPIO_PIN_MAP(0, 12)
#define SPI3_MISO_PIN   NRF_GPIO_PIN_MAP(1, 9)
#endif

#endif /* HARDWARE_ARCH_TYPE_NORDIC */

/* ============ 私有数据结构 ============ */

struct bsp_soft_spi3_config {
    uint32_t delay_us;          /* 时钟半周期延时（控制 SPI 速率） */
    uint32_t cs_pin;
    uint32_t sclk_pin;
    uint32_t mosi_pin;
    uint32_t miso_pin;
};

struct BSP_SOFT_SPI3 {
    const char *name;
    bool        lock;
    struct bsp_soft_spi3_config  cfg;
    struct bsp_spi_mutex_lock mutex_lock;
    q_device_t dev;
};

/* ============ 静态变量 ============ */

static struct BSP_SOFT_SPI3 spi3_dev = {
    .name = "spi_3",
    .lock = false,
    .cfg = {
        .delay_us = 0,           /* 0 = 最大速率，无延时，纯 GPIO 翻转 */
        .cs_pin   = SPI3_CS_PIN,
        .sclk_pin = SPI3_SCLK_PIN,
        .mosi_pin = SPI3_MOSI_PIN,
        .miso_pin = SPI3_MISO_PIN,
    },
    .mutex_lock = {0},
    .dev = {0},
};

/* ============ 私有函数声明 ============ */

static void bsp_soft_spi3_gpio_init(void);
static void bsp_soft_spi3_gpio_uninit(void);
static uint8_t bsp_soft_spi3_transfer_byte(uint8_t data);

/* ============ GPIO 操作封装 ============ */

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)

static inline void spi3_cs_set(uint32_t val) {
    nrf_gpio_pin_write(spi3_dev.cfg.cs_pin, val);
}
static inline void spi3_sclk_set(uint32_t val) {
    nrf_gpio_pin_write(spi3_dev.cfg.sclk_pin, val);
}
static inline void spi3_mosi_set(uint32_t val) {
    nrf_gpio_pin_write(spi3_dev.cfg.mosi_pin, val);
}
static inline uint32_t spi3_miso_get(void) {
    return nrf_gpio_pin_read(spi3_dev.cfg.miso_pin);
}
static inline void spi3_pin_output(uint32_t pin) {
    nrf_gpio_cfg_output(pin);
}
static inline void spi3_pin_input(uint32_t pin) {
    nrf_gpio_cfg_input(pin, NRF_GPIO_PIN_PULLUP);
}
static inline void spi3_pin_default(uint32_t pin) {
    nrf_gpio_cfg_default(pin);
}

#endif

/* ============ GPIO 初始化/反初始化 ============ */

static void bsp_soft_spi3_gpio_init(void)
{
    spi3_pin_output(spi3_dev.cfg.cs_pin);
    spi3_pin_output(spi3_dev.cfg.sclk_pin);
    spi3_pin_output(spi3_dev.cfg.mosi_pin);
    spi3_pin_input(spi3_dev.cfg.miso_pin);

    spi3_cs_set(1);      /* CS 默认高电平（未选中） */
    spi3_sclk_set(0);    /* SCLK 默认低电平（Mode 0） */
    spi3_mosi_set(0);    /* MOSI 默认低 */
}

static void bsp_soft_spi3_gpio_uninit(void)
{
    spi3_pin_default(spi3_dev.cfg.cs_pin);
    spi3_pin_default(spi3_dev.cfg.sclk_pin);
    spi3_pin_default(spi3_dev.cfg.mosi_pin);
    spi3_pin_default(spi3_dev.cfg.miso_pin);
}

/* ============ SPI 单字节收发（Mode 0, MSB first） ============ */

static inline uint8_t bsp_soft_spi3_transfer_byte(uint8_t data)
{
    uint8_t recv = 0;
    int8_t i;
    uint32_t delay = spi3_dev.cfg.delay_us;

    for (i = 7; i >= 0; i--) {
        /* SCLK 低电平期间，输出 MOSI 数据位 */
        spi3_sclk_set(0);
        spi3_mosi_set((data >> i) & 0x01);
        if (delay) {
            nrf_delay_us(delay);
        }

        /* SCLK 上升沿，读取 MISO 数据位 */
        spi3_sclk_set(1);
        if (spi3_miso_get()) {
            recv |= (1 << i);
        }
        if (delay) {
            nrf_delay_us(delay);
        }
    }

    spi3_sclk_set(0);  /* 恢复低电平 */
    return recv;
}

/* ============ q_device 接口实现 ============ */

static int bsp_soft_spi3_open(q_device_t *dev)
{
    if (spi3_dev.lock) {
        return RESULT_OK;
    }

    bsp_soft_spi3_gpio_init();
    spi3_dev.lock = true;
    Q_DEVICE_LOG_INFO("open %s \r\n", spi3_dev.name);

    return RESULT_OK;
}

static int bsp_soft_spi3_close(q_device_t *dev)
{
    if (!spi3_dev.lock) {
        return RESULT_OK;
    }

    bsp_soft_spi3_gpio_uninit();
    spi3_dev.lock = false;
    Q_DEVICE_LOG_INFO("close %s \r\n", spi3_dev.name);

    return RESULT_OK;
}

static int bsp_soft_spi3_write(q_device_t *dev, int pos, const void *buffer, int size)
{
    struct spi_package *package = (struct spi_package *)buffer;
    uint32_t i;

    if (package == NULL) {
        return RESULT_UART_CONFIG_NULL_ERR;
    }
    if (!spi3_dev.lock) {
        return RESULT_DEV_UNOPENED_ERR;
    }
    if (package->write_length == 0) {
        return RESULT_OK;
    }

    /* 互斥锁保护 */
    if (spi3_dev.mutex_lock.spi_mutex_lock_enable &&
        spi3_dev.mutex_lock.spi_mutex_lock_take != NULL) {
        spi3_dev.mutex_lock.spi_mutex_lock_take();
    }

    /* 写数据：同时读回 MISO（SPI 全双工特性） */
    for (i = 0; i < package->write_length; i++) {
        uint8_t r = bsp_soft_spi3_transfer_byte(package->write_buff[i]);
        if (package->read_buff != NULL && i < package->read_length) {
            package->read_buff[i] = r;
        }
    }

    if (spi3_dev.mutex_lock.spi_mutex_lock_enable &&
        spi3_dev.mutex_lock.spi_mutex_lock_give != NULL) {
        spi3_dev.mutex_lock.spi_mutex_lock_give();
    }

    return RESULT_OK;
}

static int bsp_soft_spi3_read(q_device_t *dev, int pos, const void *buffer, int size)
{
    struct spi_package *package = (struct spi_package *)buffer;
    uint32_t i;

    if (package == NULL) {
        return RESULT_I2C_CONFIG_NULL_ERR;
    }
    if (!spi3_dev.lock) {
        return RESULT_DEV_NULL_ERR;
    }
    if (package->read_length == 0) {
        return RESULT_OK;
    }

    /* 互斥锁保护 */
    if (spi3_dev.mutex_lock.spi_mutex_lock_enable &&
        spi3_dev.mutex_lock.spi_mutex_lock_take != NULL) {
        spi3_dev.mutex_lock.spi_mutex_lock_take();
    }

    /* 读数据：MOSI 输出 0，仅读取 MISO */
    for (i = 0; i < package->read_length; i++) {
        package->read_buff[i] = bsp_soft_spi3_transfer_byte(0x00);
    }

    if (spi3_dev.mutex_lock.spi_mutex_lock_enable &&
        spi3_dev.mutex_lock.spi_mutex_lock_give != NULL) {
        spi3_dev.mutex_lock.spi_mutex_lock_give();
    }

    return RESULT_OK;
}

static int bsp_soft_spi3_cs_ctrl(q_device_t *dev, int cmd, void *args)
{
    if (!spi3_dev.lock) {
        return RESULT_DEV_UNOPENED_ERR;
    }

    if (cmd == GPIO_OUTPUT_LOW) {
        spi3_cs_set(0);
    } else if (cmd == GPIO_OUTPUT_HIGH) {
        spi3_cs_set(1);
    }

    return RESULT_OK;
}

static int bsp_soft_spi3_config(q_device_t *dev, void *args, void *var)
{
    struct bsp_spi_mutex_lock *cfg = (struct bsp_spi_mutex_lock *)args;

    if (cfg == NULL) {
        return RESULT_GPIO_CONFIG_NULL_ERR;
    }

    spi3_dev.mutex_lock.spi_mutex_lock_enable = cfg->spi_mutex_lock_enable;
    spi3_dev.mutex_lock.spi_mutex_lock_take   = cfg->spi_mutex_lock_take;
    spi3_dev.mutex_lock.spi_mutex_lock_give   = cfg->spi_mutex_lock_give;

    return RESULT_OK;
}

/* ============ 设备操作函数表 ============ */

static struct q_device_ops bsp_soft_spi3_ops = {
    .open    = bsp_soft_spi3_open,
    .close   = bsp_soft_spi3_close,
    .read    = bsp_soft_spi3_read,
    .write   = bsp_soft_spi3_write,
    .control = bsp_soft_spi3_cs_ctrl,
    .config  = bsp_soft_spi3_config,
};

/* ============ 设备注册 ============ */

static void bsp_soft_spi3_register(void)
{
    spi3_dev.dev.name  = spi3_dev.name;
    spi3_dev.dev.dops  = &bsp_soft_spi3_ops;
    q_device_register(&spi3_dev.dev);
    Q_DEVICE_LOG_INFO("register %s ok\r\n", spi3_dev.name);
}

device_initcall(bsp_soft_spi3_register);
