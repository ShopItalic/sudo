#ifndef SUDO_BSP_ADC_TEST_Q_DEVICE_H
#define SUDO_BSP_ADC_TEST_Q_DEVICE_H

#include <stddef.h>

#define array_size(array) (sizeof(array) / sizeof((array)[0]))
#define q_device_delay_ms(milliseconds) ((void)(milliseconds))
#define Q_DEVICE_LOG_INFO(...) ((void)0)
#define device_initcall(function) \
    static void __attribute__((unused)) test_device_initcall_##function(void)

enum result_state {
    RESULT_OK = 0,
    RESULT_GPIO_OUTPUT_DEV_NULL_ERR = 1,
    RESULT_I2C_DEV_NULL_ERR = 8
};

typedef struct q_device q_device_t;

struct q_device_ops {
    int (*init)(q_device_t *device);
    int (*uninit)(q_device_t *device);
    int (*open)(q_device_t *device);
    int (*close)(q_device_t *device);
    int (*read)(q_device_t *device, int position, const void *buffer,
                int size);
    int (*write)(q_device_t *device, int position, const void *buffer,
                 int size);
    int (*control)(q_device_t *device, int command, void *args);
    int (*config)(q_device_t *device, void *args, void *var);
    int (*register_callback)(q_device_t *device, int position, void *callback);
};

struct q_device {
    const char *name;
    const struct q_device_ops *dops;
    void *owner;
    void *argv;
    int data;
    struct q_device *next;
};

int q_device_register(q_device_t *device);

#endif
