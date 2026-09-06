#ifndef TEST_MOTOR_NRFX_PWM_H
#define TEST_MOTOR_NRFX_PWM_H

#include <stdbool.h>
#include <stdint.h>

typedef uint32_t ret_code_t;
typedef uint32_t nrfx_err_t;
typedef struct
{
    uint32_t instance;
} nrfx_pwm_t;

typedef enum
{
    NRFX_PWM_EVT_FINISHED = 0,
    NRFX_PWM_EVT_END_SEQ0,
    NRFX_PWM_EVT_END_SEQ1,
    NRFX_PWM_EVT_STOPPED,
} nrfx_pwm_evt_type_t;

typedef void (*nrfx_pwm_handler_t)(nrfx_pwm_evt_type_t event_type);

typedef struct
{
    uint32_t output_pins[4];
    uint8_t irq_priority;
    uint32_t base_clock;
    uint32_t count_mode;
    uint16_t top_value;
    uint32_t load_mode;
    uint32_t step_mode;
} nrfx_pwm_config_t;

typedef uint16_t nrf_pwm_values_common_t;
typedef struct
{
    union
    {
        uint16_t *p_common;
        uint16_t *p_raw;
    } values;
    uint16_t length;
    uint32_t repeats;
    uint32_t end_delay;
} nrf_pwm_sequence_t;

#define NRFX_PWM_INSTANCE(index) { (index) }
#define NRFX_PWM_PIN_NOT_USED UINT32_MAX
#define NRF_PWM_CLK_1MHz 1u
#define NRF_PWM_MODE_UP 0u
#define NRF_PWM_LOAD_COMMON 0u
#define NRF_PWM_STEP_AUTO 0u
#define NRF_PWM_VALUES_LENGTH(values) (sizeof(values) / sizeof((values)[0]))
#define APP_IRQ_PRIORITY_LOWEST 7u
#define NRF_SUCCESS 0u
#define NRFX_SUCCESS 0u

nrfx_err_t nrfx_pwm_init(nrfx_pwm_t const *instance,
                        nrfx_pwm_config_t const *config,
                        nrfx_pwm_handler_t handler);
void nrfx_pwm_uninit(nrfx_pwm_t const *instance);
uint32_t nrfx_pwm_simple_playback(nrfx_pwm_t const *instance,
                                  nrf_pwm_sequence_t const *sequence,
                                  uint16_t playback_count,
                                  uint32_t flags);
uint32_t nrfx_pwm_complex_playback(nrfx_pwm_t const *instance,
                                   nrf_pwm_sequence_t const *sequence0,
                                   nrf_pwm_sequence_t const *sequence1,
                                   uint16_t playback_count,
                                   uint32_t flags);
bool nrfx_pwm_stop(nrfx_pwm_t const *instance, bool wait_until_stopped);

#endif
