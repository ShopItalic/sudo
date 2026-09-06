#ifndef SUDO_CAPTURE_TEST_NRFX_PDM_H
#define SUDO_CAPTURE_TEST_NRFX_PDM_H

#include <stdbool.h>
#include <stdint.h>

#ifndef NRFX_PDM_CONFIG_IRQ_PRIORITY
#define NRFX_PDM_CONFIG_IRQ_PRIORITY 6
#endif
#ifndef NRFX_PDM_CONFIG_CLOCK_FREQ
#define NRFX_PDM_CONFIG_CLOCK_FREQ 138412032
#endif

#ifndef PDM_IRQn
#define PDM_IRQn 29
#endif

typedef int32_t nrfx_err_t;
enum {
    NRFX_SUCCESS = 0,
    NRFX_ERROR_INVALID_STATE = 1,
    NRFX_ERROR_BUSY = 2,
    NRFX_ERROR_INVALID_PARAM = 3,
    NRFX_ERROR_INTERNAL = 4
};

typedef enum {
    NRFX_PDM_NO_ERROR = 0,
    NRFX_PDM_ERROR_OVERFLOW = 1
} nrfx_pdm_error_t;

typedef enum {
    NRF_PDM_MODE_STEREO = 0,
    NRF_PDM_MODE_MONO = 1
} nrf_pdm_mode_t;

typedef enum {
    NRF_PDM_EDGE_LEFTRISING = 0
} nrf_pdm_edge_t;

typedef enum {
    NRF_PDM_GAIN_DEFAULT = 0,
    NRF_PDM_GAIN_MAXIMUM = 0x50
} nrf_pdm_gain_t;

typedef enum {
    NRF_PDM_FREQ_1032K = 0
} nrf_pdm_freq_t;

typedef struct {
    nrf_pdm_mode_t mode;
    nrf_pdm_edge_t edge;
    uint8_t pin_clk;
    uint8_t pin_din;
    nrf_pdm_freq_t clock_freq;
    nrf_pdm_gain_t gain_l;
    nrf_pdm_gain_t gain_r;
    uint8_t interrupt_priority;
} nrfx_pdm_config_t;

typedef struct {
    bool buffer_requested;
    int16_t *buffer_released;
    nrfx_pdm_error_t error;
} nrfx_pdm_evt_t;

typedef void (*nrfx_pdm_event_handler_t)(nrfx_pdm_evt_t const * const event);

nrfx_err_t nrfx_pdm_init(nrfx_pdm_config_t const *config,
                         nrfx_pdm_event_handler_t handler);
void nrfx_pdm_uninit(void);
nrfx_err_t nrfx_pdm_start(void);
nrfx_err_t nrfx_pdm_stop(void);
nrfx_err_t nrfx_pdm_buffer_set(int16_t *buffer, uint16_t length);

#define TEST_PDM_HISTORY 128U
enum {
    TEST_PDM_IDLE = 0,
    TEST_PDM_RUNNING,
    TEST_PDM_STARTING,
    TEST_PDM_STOPPING
};

typedef struct {
    nrfx_pdm_event_handler_t handler;
    nrfx_pdm_config_t config;
    int16_t *buffers[2];
    uint16_t lengths[2];
    unsigned active_buffer;
    unsigned state;
    bool initialized;
    bool irq_enabled;
    bool pending_irq;
    bool stop_requested;
    unsigned init_calls;
    unsigned uninit_calls;
    unsigned start_calls;
    unsigned stop_calls;
    unsigned buffer_set_calls;
    int16_t *buffer_history[TEST_PDM_HISTORY];
    uint16_t length_history[TEST_PDM_HISTORY];
    int16_t *last_released;
    nrfx_err_t init_error;
    nrfx_err_t start_error;
    nrfx_err_t stop_error;
    nrfx_err_t buffer_set_error;
} test_pdm_observation;

extern test_pdm_observation test_pdm;

void test_pdm_reset(void);
bool test_pdm_deliver_pending_request(void);
bool test_pdm_emit_started(void);
bool test_pdm_emit_full(void);
bool test_pdm_emit_overflow(void);
bool test_pdm_emit_stopped(void);
int16_t *test_pdm_active_buffer(void);

/* These macros are the small IRQ surface used by app_sudo_capture.c. */
#define NRFX_IRQ_DISABLE(irq) \
    do { (void)(irq); test_pdm.irq_enabled = false; } while (0)
#define NRFX_IRQ_ENABLE(irq) \
    do { (void)(irq); test_pdm.irq_enabled = true; } while (0)
#define NRFX_IRQ_PENDING_CLEAR(irq) \
    do { (void)(irq); test_pdm.pending_irq = false; } while (0)
#define NRFX_IRQ_PENDING_SET(irq) \
    do { (void)(irq); test_pdm.pending_irq = true; } while (0)

#endif
