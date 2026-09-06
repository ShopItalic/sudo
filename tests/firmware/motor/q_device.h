#ifndef TEST_MOTOR_Q_DEVICE_H
#define TEST_MOTOR_Q_DEVICE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct q_device q_device_t;

struct pwm_parameter
{
    uint16_t top_value;
    uint16_t *p_common;
    uint16_t length;
    uint32_t repeats;
    uint16_t playback_count;
    uint32_t flags;
};

struct pwm_config
{
    bool pwm_aisle0_enable_status;
    bool pwm_aisle1_enable_status;
    bool pwm_aisle2_enable_status;
    bool pwm_aisle3_enable_status;
    struct pwm_parameter pwm_parameter_config;
};

enum pwm_register_callback
{
    PWM_REGISTER_FINISHED_CALLBACK = 0,
    PWM_REGISTER_END_SEQ0_CALLBACK,
    PWM_REGISTER_END_SEQ1_CALLBACK,
    PWM_REGISTER_STOPPED_CALLBACK,
};

enum pwm_flag
{
    PWM_FLAG_STOP = 0x01,
    PWM_FLAG_LOOP = 0x02,
};

enum pwm_status
{
    PWM_IDIE = 0,
    PWM_BUSY,
};

enum result_state
{
    RESULT_OK = 0,
    RESULT_DEV_NULL_ERR,
};

enum pwm_ctrl
{
    PWM_CTRL_START = 0,
    PWM_CTRL_STOP,
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
};

#define q_device_assert(p) ((void)(p))
#define device_initcall(function) \
    __attribute__((constructor)) static void test_initcall_##function(void) { function(); }

int q_device_register(q_device_t *dev);
q_device_t *q_device_find(const char *name);
int q_device_open(q_device_t *dev);
int q_device_close(q_device_t *dev);
int q_device_ctrl(q_device_t *dev, int cmd, void *arg);
int q_device_cfg(q_device_t *dev, void *args, void *var);
int q_device_reg_callback(q_device_t *dev, int pos, void *callback);

#endif
