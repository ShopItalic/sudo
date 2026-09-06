#ifndef HARDWARE_ARCH_TYPE_NORDIC
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif
#ifndef SUDO_VOICE_ONLY
#define SUDO_VOICE_ONLY 1
#endif
#ifndef HANDWARE_1_23_2
#define HANDWARE_1_23_2 1
#endif

#include "q_device.h"
#include "nrf_drv_saadc.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static unsigned checks;
static unsigned failures;
static q_device_t *registered_device;
static unsigned register_calls;
static unsigned saadc_init_calls;
static unsigned channel_init_calls;
static unsigned channel_uninit_calls;
static unsigned saadc_uninit_calls;
static int mock_init_result;
static int mock_channel_init_result;
static int mock_sample_result;
static nrf_saadc_value_t mock_sample_value;

static void check_condition(bool condition, const char *expression,
                            unsigned line)
{
    ++checks;
    if (!condition)
    {
        ++failures;
        fprintf(stderr, "FAIL line %u: %s\n", line, expression);
    }
}

#define CHECK(condition) check_condition((condition), #condition, __LINE__)

int q_device_register(q_device_t *device)
{
    CHECK(device != NULL);
    registered_device = device;
    ++register_calls;
    return RESULT_OK;
}

ret_code_t nrf_drv_saadc_init(const nrf_drv_saadc_config_t *config,
                              void *event_handler)
{
    CHECK(config != NULL);
    CHECK(event_handler == NULL);
    ++saadc_init_calls;
    return mock_init_result;
}

ret_code_t nrf_drv_saadc_channel_init(uint8_t channel,
                                      nrf_saadc_channel_config_t *config)
{
    CHECK(channel == 0U);
    CHECK(config != NULL);
    ++channel_init_calls;
    return mock_channel_init_result;
}

ret_code_t nrf_drv_saadc_channel_uninit(nrf_saadc_input_t input)
{
    CHECK(input == NRF_SAADC_INPUT_AIN6);
    ++channel_uninit_calls;
    return 0;
}

void nrfx_saadc_uninit(void)
{
    ++saadc_uninit_calls;
}

ret_code_t nrfx_saadc_sample_convert(uint8_t channel,
                                     nrf_saadc_value_t *value)
{
    CHECK(channel == 0U);
    CHECK(value != NULL);
    if (mock_sample_result == RESULT_OK)
        *value = mock_sample_value;
    return mock_sample_result;
}

/* Compile and exercise the production Nordic implementation itself. */
#include "../../firmware/bc_ros/bc_driver/bsp/src/bsp_adc.c"

static void reset_mocks(void)
{
    register_calls = 0U;
    saadc_init_calls = 0U;
    channel_init_calls = 0U;
    channel_uninit_calls = 0U;
    saadc_uninit_calls = 0U;
    mock_init_result = RESULT_OK;
    mock_channel_init_result = RESULT_OK;
    mock_sample_result = RESULT_OK;
    mock_sample_value = 0;
}

int main(void)
{
    uint16_t output = 0xa5a5U;

    reset_mocks();
    bsp_adc_register();
    CHECK(register_calls == 1U);
    CHECK(registered_device != NULL);
    CHECK(registered_device->dops == &ops);

    CHECK(registered_device->dops->open(registered_device) == RESULT_OK);
    CHECK(saadc_init_calls == 1U);
    CHECK(channel_init_calls == 1U);

    mock_sample_result = 17;
    CHECK(registered_device->dops->read(registered_device, 0, &output, 1) ==
          17);
    CHECK(output == 0xa5a5U);

    mock_sample_result = RESULT_OK;
    mock_sample_value = 0x1234;
    CHECK(registered_device->dops->read(registered_device, 0, &output, 1) ==
          RESULT_OK);
    CHECK(output == 0x1234U);

    CHECK(registered_device->dops->close(registered_device) == RESULT_OK);
    CHECK(channel_uninit_calls == 1U);
    CHECK(saadc_uninit_calls == 1U);

    if (failures != 0U)
    {
        fprintf(stderr, "%u/%u checks failed\n", failures, checks);
        return 1;
    }

    printf("bsp adc: %u checks passed\n", checks);
    return 0;
}
