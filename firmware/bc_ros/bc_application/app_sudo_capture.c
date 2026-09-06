#include "app_sudo_capture.h"

#include "bc_capture.h"
#include "adpcm_a.h"
#include "bc_ldo_switch.h"
#include "nrfx_pdm.h"
#include "nrf_gpio.h"
#include "nrf_pdm.h"

#if !defined(SUDO_VOICE_ONLY) || !defined(HANDWARE_1_23_2) || \
    defined(HANDWARE_1_23_2_ONE_SEC) || defined(HANDWARE_1_23_3)
#error "This capture adapter is for Sudo Voice on standard 1.23.2 only"
#endif

#if NRFX_PDM_CONFIG_IRQ_PRIORITY < configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#error "PDM notifications require a FreeRTOS-safe interrupt priority"
#endif

static bc_capture capture;
static int16_t samples[BC_CAPTURE_BUFFERS][BC_CAPTURE_SAMPLES];
static MonoAdpcmProcessor codec;
static TaskHandle_t capture_worker;
static volatile bool initialized;
static volatile bool stop_issued;
static bool mic_powered;
static volatile bool ptt_guard;
static volatile TickType_t ptt_started, ptt_reported, last_progress;
static volatile uint32_t ptt_lease_ticks, ptt_limit_ticks;

/* These are the exact standard 1.23.2 pin, edge, clock and gain selections
 * from the supplier app_pdm_handler.c. No new microphone pin map or codec. */
static const nrfx_pdm_config_t config = {
    .mode = NRF_PDM_MODE_MONO,
    .edge = NRF_PDM_EDGE_LEFTRISING,
    .pin_clk = NRF_GPIO_PIN_MAP(0, 4),
    .pin_din = NRF_GPIO_PIN_MAP(0, 21),
    .clock_freq = (nrf_pdm_freq_t)NRFX_PDM_CONFIG_CLOCK_FREQ,
    .gain_l = NRF_PDM_GAIN_MAXIMUM,
    .gain_r = NRF_PDM_GAIN_MAXIMUM,
    .interrupt_priority = NRFX_PDM_CONFIG_IRQ_PRIORITY
};

static uint8_t slot_for(const int16_t *buffer)
{
    uint8_t slot;
    for (slot = 0; slot < BC_CAPTURE_BUFFERS; ++slot)
        if (buffer == samples[slot])
            return slot;
    return BC_CAPTURE_NONE;
}

/* IRQ context. Do not encode, use Flash, wait, log, or call the owner here. */
static void stop_peripheral(void)
{
    nrfx_err_t error;
    if (stop_issued)
        return;
    stop_issued = true;
    error = nrfx_pdm_stop();
    if (error != NRFX_SUCCESS)
        bc_capture_fault(&capture, capture.id, BC_REC_CAPTURE_ERROR);
    /* A stop during driver STARTING disables immediately and has no STOPPED
     * callback. A running stop remains asynchronous. */
    if (!nrf_pdm_enable_check())
        (void)bc_capture_quiesced(&capture, capture.id, error != NRFX_SUCCESS);
}

static void pdm_event(const nrfx_pdm_evt_t *event)
{
    BaseType_t wake = pdFALSE;
    uint8_t slot;
    if (!initialized || !capture.running)
        return;
    last_progress = xTaskGetTickCountFromISR();
    if (event->error != NRFX_PDM_NO_ERROR) {
        bc_capture_fault(&capture, capture.id, BC_REC_CAPTURE_OVERFLOW);
        stop_peripheral();
    } else if (event->buffer_requested) {
        if (event->buffer_released != NULL) {
            if (ptt_guard) {
                TickType_t now = xTaskGetTickCountFromISR();
                if ((uint32_t)(now - ptt_reported) >= ptt_lease_ticks)
                    bc_capture_fault(&capture, capture.id, BC_REC_TOUCH_ERROR);
                else if (ptt_limit_ticks && (uint32_t)(now - ptt_started) >= ptt_limit_ticks)
                    (void)bc_capture_stop(&capture, capture.id);
            }
            slot = slot_for(event->buffer_released);
            if (!bc_capture_full(&capture, capture.id, slot))
                stop_peripheral();
            /* Stop at a completed block boundary. The nrfx STOPPED event
             * provides no sample count for its unfinished buffers. Keeping
             * this final full block avoids fabricating or discarding a tail. */
            if (capture.stop_requested)
                stop_peripheral();
        }
        if (!stop_issued) {
            slot = bc_capture_acquire_dma(&capture, capture.id);
            if (slot == BC_CAPTURE_NONE ||
                nrfx_pdm_buffer_set(samples[slot], BC_CAPTURE_SAMPLES) != NRFX_SUCCESS) {
                bc_capture_fault(&capture, capture.id, BC_REC_CAPTURE_OVERFLOW);
                stop_peripheral();
            }
        }
    } else if (!nrf_pdm_enable_check()) {
        /* Hardware is disabled before nrfx releases its STOPPED buffers.
         * Free the DMA slots; retain every READY/ENCODING slot for the worker. */
        (void)bc_capture_quiesced(&capture, capture.id, false);
    }
    if (capture_worker != NULL) {
        vTaskNotifyGiveFromISR(capture_worker, &wake);
        portYIELD_FROM_ISR(wake);
    }
}

void app_sudo_capture_init(TaskHandle_t worker)
{
    capture_worker = worker;
}

static void power_down(void)
{
    /* Prevent a queued buffer-request IRQ from touching a retired session.
     * nrfx_pdm_init restores the IRQ configuration for the next capture. */
    taskENTER_CRITICAL();
    NRFX_IRQ_DISABLE(PDM_IRQn);
    if (initialized) {
        nrfx_pdm_uninit();
        initialized = false;
    }
    NRFX_IRQ_PENDING_CLEAR(PDM_IRQn);
    taskEXIT_CRITICAL();
    if (mic_powered) { bc_ldo_mic_power_off(); mic_powered = false; }
}

bc_rec_result app_sudo_capture_start(void *ctx, uint64_t id)
{
    nrfx_err_t error;
    bool begun;
    (void)ctx;
    if (capture_worker == NULL)
        return BC_REC_CAPTURE_ERROR;
    taskENTER_CRITICAL();
    begun = !initialized && bc_capture_begin(&capture, id);
    taskEXIT_CRITICAL();
    if (!begun)
        return BC_REC_CAPTURE_ERROR;
    mono_adpcm_init(&codec);
    bc_ldo_mic_power_on();
    mic_powered = true;
    /* NRFX init does not start DMA; set initialized before start can post its
     * initial buffer request. Every failure returns with hardware disabled. */
    error = nrfx_pdm_init(&config, pdm_event);
    if (error == NRFX_SUCCESS) {
        initialized = true;
        stop_issued = false;
        ptt_guard = false;
        last_progress = xTaskGetTickCount();
        error = nrfx_pdm_start();
    }
    if (error != NRFX_SUCCESS) {
        power_down();
        taskENTER_CRITICAL();
        (void)bc_capture_quiesced(&capture, id, true);
        taskEXIT_CRITICAL();
        return BC_REC_CAPTURE_ERROR;
    }
    return BC_REC_OK;
}

void app_sudo_capture_ptt_arm(uint64_t id, uint32_t lease_ms, uint32_t limit_ms)
{
    taskENTER_CRITICAL();
    if (capture.running && capture.id == id && lease_ms >= 100U && lease_ms <= 750U) {
        ptt_guard = true;
        ptt_started = xTaskGetTickCount(); ptt_reported = ptt_started;
        ptt_lease_ticks = (uint32_t)(((uint64_t)lease_ms * configTICK_RATE_HZ + 999U) / 1000U);
        ptt_limit_ticks = (uint32_t)(((uint64_t)limit_ms * configTICK_RATE_HZ + 999U) / 1000U);
    }
    taskEXIT_CRITICAL();
}

void app_sudo_capture_ptt_touch(uint64_t id, bool valid, bool contact)
{
    taskENTER_CRITICAL();
    if (capture.running && capture.id == id && ptt_guard) {
        TickType_t now = xTaskGetTickCount();
        if (!valid || (uint32_t)(now - ptt_reported) >= ptt_lease_ticks)
            bc_capture_fault(&capture, id, BC_REC_TOUCH_ERROR);
        else if (!contact)
            (void)bc_capture_stop(&capture, id);
        else
            ptt_reported = now;
    }
    taskEXIT_CRITICAL();
}

bc_rec_result app_sudo_capture_stop(void *ctx, uint64_t id)
{
    bool requested;
    (void)ctx;
    taskENTER_CRITICAL();
    requested = bc_capture_stop(&capture, id);
    taskEXIT_CRITICAL();
    return requested ? BC_REC_OK : BC_REC_WRONG_SESSION;
}

bool app_sudo_capture_abort(void *ctx, uint64_t id)
{
    bool drained;
    (void)ctx;
    taskENTER_CRITICAL();
    if (capture.id != id || id == 0U) {
        taskEXIT_CRITICAL();
        return false;
    }
    taskEXIT_CRITICAL();
    power_down();
    taskENTER_CRITICAL();
    (void)bc_capture_quiesced(&capture, id, true);
    drained = bc_capture_drained(&capture, id);
    taskEXIT_CRITICAL();
    /* If completed buffers remain, keep ownership until poll encodes them.
     * An abort cannot report a normal successful saved recording. */
    return drained;
}

bool app_sudo_capture_poll(bc_recording *owner, uint32_t now_ms)
{
    uint64_t id;
    uint32_t sequence = 0;
    uint8_t slot;
    uint8_t encoded[BC_REC_FRAME_MAX];
    bc_rec_result error;
    bool drained;
    bool requested;
    bool stalled;
    unsigned sample;
    const bc_rec_snapshot *snapshot = bc_recording_snapshot(owner);
    if (snapshot == NULL || !bc_recording_active(owner))
        return false;
    id = snapshot->start.id;
    taskENTER_CRITICAL();
    stalled = capture.id == id && capture.running &&
        (uint32_t)(xTaskGetTickCount() - last_progress) >= (configTICK_RATE_HZ + 1U) / 2U;
    taskEXIT_CRITICAL();
    if (stalled) {
        /* No DMA progress for half a second: quiesce hardware now, retain
         * completed blocks, and finalize only as a partial capture error. */
        (void)app_sudo_capture_abort(NULL, id);
    }
    taskENTER_CRITICAL();
    error = capture.id == id ? capture.error : BC_REC_WRONG_SESSION;
    requested = capture.id == id && capture.stop_requested;
    slot = bc_capture_take(&capture, id, &sequence);
    taskEXIT_CRITICAL();
    if (error != BC_REC_OK && snapshot->error == BC_REC_OK)
        (void)bc_recording_fault(owner, id, error, now_ms);
    else if (requested && snapshot->phase == BC_REC_RECORDING)
        (void)bc_recording_stop(owner, id, now_ms);
    if (slot != BC_CAPTURE_NONE) {
        /* Retain supplier decimation and continuous IMA ADPCM state. DMA
         * cannot reuse this slot until the encoder explicitly releases it. */
        for (sample = 0; sample < BC_CAPTURE_SAMPLES / 2U; ++sample)
            samples[slot][sample] = samples[slot][sample * 2U];
        adpcm_encoder(samples[slot], (char *)encoded,
                      BC_CAPTURE_SAMPLES / 2U, &codec.mono_state);
        taskENTER_CRITICAL();
        (void)bc_capture_release(&capture, id, slot);
        taskEXIT_CRITICAL();
        (void)bc_recording_frame(owner, id, sequence, encoded, sizeof(encoded), now_ms);
    }
    taskENTER_CRITICAL();
    drained = bc_capture_drained(&capture, id);
    taskEXIT_CRITICAL();
    if (drained) {
        power_down();
        (void)bc_recording_drained(owner, id);
    }
    return slot != BC_CAPTURE_NONE;
}
