#ifndef TEST_SPI_FLASH_NRF_SPIM_H
#define TEST_SPI_FLASH_NRF_SPIM_H

#include <stddef.h>
#include <stdint.h>

typedef uint32_t nrfx_err_t;
typedef uint32_t nrf_spim_frequency_t;
typedef uint32_t nrf_spim_mode_t;
typedef uint32_t nrf_spim_bit_order_t;

typedef struct
{
    uint32_t instance;
} nrfx_spim_t;

typedef struct
{
    uint32_t miso_pin;
    uint32_t mosi_pin;
    uint32_t sck_pin;
    nrf_spim_frequency_t frequency;
    nrf_spim_mode_t mode;
    uint32_t ss_active_high;
    uint32_t orc;
    uint32_t irq_priority;
    nrf_spim_bit_order_t bit_order;
} nrfx_spim_config_t;

typedef struct
{
    const uint8_t *p_tx_buffer;
    size_t tx_length;
    uint8_t *p_rx_buffer;
    size_t rx_length;
} nrfx_spim_xfer_desc_t;

typedef struct
{
    uint32_t type;
} nrf_drv_spi_evt_t;

typedef struct
{
    uint32_t type;
    nrfx_spim_xfer_desc_t xfer_desc;
} nrfx_spim_evt_t;

typedef void (*nrfx_spim_evt_handler_t)(nrfx_spim_evt_t const *event,
                                        void *context);

#define NRFX_SPIM_INSTANCE(index) { (index) }
#define NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY 7u
#define NRF_SPIM_FREQ_8M 8000000u
#define NRF_SPIM_FREQ_32M 32000000u
#define NRF_SPIM_MODE_0 0u
#define NRF_SPIM_BIT_ORDER_MSB_FIRST 0u
#define NRFX_SUCCESS 0u
#define NRFX_ERROR_BUSY 1u
#define NRFX_ERROR_INVALID_PARAM 2u

nrfx_err_t nrfx_spim_init(nrfx_spim_t const *instance,
                          nrfx_spim_config_t const *config,
                          nrfx_spim_evt_handler_t handler,
                          void *context);
void nrfx_spim_uninit(nrfx_spim_t const *instance);
nrfx_err_t nrfx_spim_xfer(nrfx_spim_t const *instance,
                          nrfx_spim_xfer_desc_t const *desc,
                          uint32_t flags);
void nrfx_spim_abort(nrfx_spim_t const *instance);

#endif
