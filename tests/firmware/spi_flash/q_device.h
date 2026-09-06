#ifndef TEST_SPI_FLASH_Q_DEVICE_H
#define TEST_SPI_FLASH_Q_DEVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*q_device_mutex_lock_take)(void);
typedef void (*q_device_mutex_lock_give)(void);

struct bsp_spi_mutex_lock
{
    bool spi_mutex_lock_enable;
    q_device_mutex_lock_take spi_mutex_lock_take;
    q_device_mutex_lock_give spi_mutex_lock_give;
};

struct spi_package
{
    uint8_t *write_buff;
    uint8_t write_length;
    uint8_t *read_buff;
    uint8_t read_length;
};

enum
{
    GPIO_OUTPUT_LOW = 0,
    GPIO_OUTPUT_HIGH = 1,
};

enum result_state
{
    RESULT_OK = 0,
    RESULT_UART_CONFIG_NULL_ERR,
    RESULT_I2C_CONFIG_NULL_ERR,
    RESULT_I2C_READ_ERR,
    RESULT_SPI_SEND_ERR,
    RESULT_OPEN_ERR = 24,
    RESULT_DEV_UNOPENED_ERR = 28,
    RESULT_DEV_NULL_ERR,
};

enum q_device_result
{
    RESULT_Q_DEVICE_OK = 0,
    RESULT_DEV_POINTER_NULL_ERROR,
    RESULT_READ_POINTER_NULL_ERROR,
    RESULT_WRITE_POINTER_NULL_ERROR,
    RESULT_INIT_POINTER_NULL_ERROR,
    RESULT_OPEN_POINTER_NULL_ERROR,
    RESULT_CLOSE_POINTER_NULL_ERROR,
    RESULT_CONTROL_POINTER_NULL_ERROR,
    RESULT_CONFIG_POINTER_NULL_ERROR,
    RESULT_REG_CALLBACK_POINTER_NULL_ERROR,
    RESULT_GPIO_CONFIG_NULL_ERR,
    RESULT_CONFIG_NULL_ERR,
};

typedef struct q_device q_device_t;

struct q_device_ops
{
    int (*init)(q_device_t *dev);
    int (*uninit)(q_device_t *dev);
    int (*open)(q_device_t *dev);
    int (*close)(q_device_t *dev);
    int (*read)(q_device_t *dev, int pos, const void *buffer, int size);
    int (*write)(q_device_t *dev, int pos, const void *buffer, int size);
    int (*control)(q_device_t *dev, int cmd, void *args);
    int (*config)(q_device_t *dev, void *args, void *var);
    int (*register_callback)(q_device_t *dev, int pos, void *callback);
};

struct q_device
{
    const char *name;
    const struct q_device_ops *dops;
    void *owner;
    void *argv;
    int data;
    struct q_device *next;
};

int q_device_register(q_device_t *dev);
q_device_t *q_device_find(const char *name);
int q_device_open(q_device_t *dev);
int q_device_close(q_device_t *dev);
int q_device_read(q_device_t *dev, int pos, const void *buffer, int size);
int q_device_write(q_device_t *dev, int pos, const void *buffer, int size);
int q_device_ctrl(q_device_t *dev, int cmd, void *args);
int q_device_cfg(q_device_t *dev, void *args, void *var);
int q_device_reg_callback(q_device_t *dev, int pos, void *callback);

void test_nrf_delay_us(uint32_t microseconds);
void test_nrf_delay_ms(uint32_t milliseconds);

#define q_device_assert(p) ((void)(p))
#define Q_DEVICE_LOG_INFO(...) ((void)0)
#define Q_DEVICE_LOG_ERROR(...) ((void)0)
#define BC_LOG_INFO(...) ((void)0)
#define BC_LOG_ERROR(...) ((void)0)
#define nrf_delay_us test_nrf_delay_us
#define nrf_delay_ms test_nrf_delay_ms
#define taskENTER_CRITICAL() ((void)0)
#define taskEXIT_CRITICAL() ((void)0)

#define device_initcall(function) \
    __attribute__((constructor)) static void test_initcall_##function(void) { function(); }

#endif
