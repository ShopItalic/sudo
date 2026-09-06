#ifndef SUDO_BSP_ADC_TEST_NRF_DRV_SAADC_H
#define SUDO_BSP_ADC_TEST_NRF_DRV_SAADC_H

#include <stdint.h>

typedef int32_t ret_code_t;
typedef int16_t nrf_saadc_value_t;
typedef int nrf_saadc_input_t;

typedef struct {
    nrf_saadc_input_t resistor_p;
    nrf_saadc_input_t resistor_n;
    int gain;
    int reference;
    int acq_time;
    int mode;
    int burst;
    nrf_saadc_input_t pin_n;
    nrf_saadc_input_t pin_p;
} nrf_saadc_channel_config_t;

typedef struct {
    int resolution;
} nrf_drv_saadc_config_t;

#define NRF_DRV_SAADC_DEFAULT_CONFIG {0}
#define NRF_SAADC_INPUT_AIN0 0
#define NRF_SAADC_INPUT_AIN1 1
#define NRF_SAADC_INPUT_AIN2 2
#define NRF_SAADC_INPUT_AIN3 3
#define NRF_SAADC_INPUT_AIN4 4
#define NRF_SAADC_INPUT_AIN5 5
#define NRF_SAADC_INPUT_AIN6 6
#define NRF_SAADC_INPUT_AIN7 7
#define NRF_SAADC_INPUT_DISABLED (-1)
#define NRF_SAADC_RESISTOR_DISABLED 0
#define NRF_SAADC_GAIN1_6 0
#define NRF_SAADC_REFERENCE_INTERNAL 0
#define NRF_SAADC_ACQTIME_10US 0
#define NRF_SAADC_MODE_SINGLE_ENDED 0
#define NRF_SAADC_BURST_DISABLED 0
#define NRF_SAADC_RESOLUTION_12BIT 12

ret_code_t nrf_drv_saadc_init(const nrf_drv_saadc_config_t *config,
                              void *event_handler);
ret_code_t nrf_drv_saadc_channel_init(uint8_t channel,
                                      nrf_saadc_channel_config_t *config);
ret_code_t nrf_drv_saadc_channel_uninit(nrf_saadc_input_t input);
void nrfx_saadc_uninit(void);
ret_code_t nrfx_saadc_sample_convert(uint8_t channel,
                                     nrf_saadc_value_t *value);

#define APP_ERROR_CHECK(error) ((void)(error))

#endif
