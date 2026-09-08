#include "app_sudo_capture.h"

#include "bc_capture.h"
#include "bc_opus_encoder.h"
#include "bc_opus_profile.h"
#include "bc_opus_stream.h"
#include "bc_resampler.h"
#include "bc_ldo_switch.h"
#include "nrf.h"
#include "nrfx_pdm.h"
#include "nrf_gpio.h"
#include "nrf_pdm.h"

#include <string.h>

#if !defined(SUDO_VOICE_ONLY) || !defined(HANDWARE_1_23_2) || \
    defined(HANDWARE_1_23_2_ONE_SEC) || defined(HANDWARE_1_23_3)
#error "This capture adapter is for Sudo Voice on standard 1.23.2 only"
#endif

#if NRFX_PDM_CONFIG_IRQ_PRIORITY < configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#error "PDM notifications require a FreeRTOS-safe interrupt priority"
#endif

/* The microphone clock is the supplier's 1.032 MHz selection; with the
 * nRF52840 default ratio of 64 it yields the nominal PCM rate below. */
#if NRFX_PDM_CONFIG_CLOCK_FREQ != 138412032
#error "The Opus profile is derived from the 1.032 MHz PDM clock"
#endif

#define SCRATCH_FILL 0xa5U
#define CANARY_FILL 0xc5U
#define RESAMPLE_CHUNK 160U

static bc_capture capture;
static int16_t samples[BC_CAPTURE_BUFFERS][BC_CAPTURE_SAMPLES];
static TaskHandle_t capture_worker;
static volatile bool initialized;
static volatile bool stop_issued;
static bool mic_powered;
static volatile bool ptt_guard;
static volatile TickType_t ptt_started, ptt_reported, last_progress;
static volatile uint32_t ptt_lease_ticks, ptt_limit_ticks;

/* Opus pipeline: resampler -> frame accumulator/encoder -> container. */
static bc_resampler resampler;
static bc_opus_encoder encoder;
static bc_opus_stream_writer writer;
static bc_audio_format format;
static uint8_t *encoder_state;
static size_t encoder_state_size;
static uint8_t *scratch;             /* GLOBAL_STACK_SIZE + canary */
static bool prepared;
static bool encoder_failed;          /* sticky until the next capture start */
static bool tail_flushed;
static uint32_t frame_sequence;
static uint32_t faults;
static uint32_t scratch_high_water;
static uint64_t counted_id;
static uint32_t counted_samples;

/* These are the exact standard 1.23.2 pin, edge, clock and gain selections
 * from the supplier app_pdm_handler.c. No new microphone pin map. */
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

/* ---- libopus allocation port: one-time worker-owned buffers ---- */

void *bc_opus_port_alloc(size_t size)
{
    /* The profile never creates codec objects on the heap. Any such request
     * is a programming error and fails explicitly. */
    (void)size;
    return NULL;
}

void bc_opus_port_free(void *ptr) { (void)ptr; }

void *bc_opus_port_alloc_scratch(size_t size)
{
    if (scratch == NULL || size != (size_t)GLOBAL_STACK_SIZE) return NULL;
    return scratch;
}

static bool canary_intact(void)
{
    unsigned i;
    if (scratch == NULL) return false;
    for (i = 0; i < BC_OPUS_SCRATCH_CANARY_BYTES; ++i)
        if (scratch[GLOBAL_STACK_SIZE + i] != CANARY_FILL) return false;
    return true;
}

static void measure_scratch(void)
{
    size_t i;
    if (scratch == NULL) return;
    for (i = GLOBAL_STACK_SIZE; i > scratch_high_water; --i)
        if (scratch[i - 1U] != SCRATCH_FILL) { scratch_high_water = (uint32_t)i; break; }
}

static uint32_t cycle_counter(void *ctx)
{
    (void)ctx;
    return DWT->CYCCNT;
}

static uint32_t cycles_to_us(uint64_t cycles)
{
    uint32_t hz = SystemCoreClock ? SystemCoreClock : 64000000U;
    return (uint32_t)(cycles / (hz / 1000000U));
}

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

bool app_sudo_capture_prepare(void)
{
    bc_opus_encoder_config profile;
    int16_t silence[BC_OPUS_FRAME_SAMPLES];
    uint8_t packet[BC_OPUS_PACKET_MAX];
    size_t produced = 0U;
    bool ready = false;
    if (prepared) return true;
    if (!bc_resampler_init(&resampler, BC_OPUS_PCM_NOMINAL_HZ, BC_OPUS_SAMPLE_RATE_HZ))
        return false;
    bc_opus_encoder_default_config(&profile);
    if (!bc_opus_encoder_config_valid(&profile)) return false;
    encoder_state_size = bc_opus_encoder_state_size(profile.channels);
    if (encoder_state_size == 0U || encoder_state_size > BC_OPUS_ENCODER_STATE_MAX) return false;
    /* One-time bounded allocations from the RTOS heap, never freed. Both are
     * reported through FORMAT_GET so their cost is visible in qualification. */
    if (encoder_state == NULL) encoder_state = pvPortMalloc(encoder_state_size);
    if (scratch == NULL) scratch = pvPortMalloc((size_t)GLOBAL_STACK_SIZE + BC_OPUS_SCRATCH_CANARY_BYTES);
    if (encoder_state == NULL || scratch == NULL) return false;
    memset(scratch, SCRATCH_FILL, GLOBAL_STACK_SIZE);
    memset(scratch + GLOBAL_STACK_SIZE, CANARY_FILL, BC_OPUS_SCRATCH_CANARY_BYTES);
    if (bc_opus_encoder_init(&encoder, &profile, encoder_state, encoder_state_size) != BC_OPUS_OK)
        return false;
    /* Cycle counter for encode timing instrumentation. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    bc_opus_encoder_set_cycle_source(&encoder, cycle_counter, NULL);
    /* Self-test one silent frame so the pseudostack is exercised before any
     * recording depends on it; a failure here disables recording explicitly. */
    memset(silence, 0, sizeof(silence));
    if (bc_opus_encoder_feed(&encoder, silence, BC_OPUS_FRAME_SAMPLES, &ready) != BC_OPUS_FRAME_SAMPLES || !ready ||
        bc_opus_encoder_encode(&encoder, packet, sizeof(packet), &produced) != BC_OPUS_OK ||
        produced == 0U || !canary_intact() ||
        bc_opus_encoder_reset(&encoder) != BC_OPUS_OK)
        return false;
    memset(&encoder.stats, 0, sizeof(encoder.stats));
    bc_opus_encoder_format(&encoder, &format);
    if (!bc_audio_format_valid(&format) || format.codec != BC_AUDIO_CODEC_OPUS) return false;
    measure_scratch();
    prepared = true;
    return true;
}

bool app_sudo_capture_format(bc_audio_format *out)
{
    if (!prepared || out == NULL) return false;
    *out = format;
    return true;
}

bool app_sudo_capture_audio_stats(void *ctx, bc_audio_format *out, bc_voice_audio_stats *stats)
{
    (void)ctx;
    if (!prepared || out == NULL || stats == NULL) return false;
    *out = format;
    memset(stats, 0, sizeof(*stats));
    stats->state_bytes = (uint32_t)encoder_state_size;
    stats->scratch_bytes = (uint32_t)GLOBAL_STACK_SIZE;
    stats->scratch_high_water = scratch_high_water;
    stats->frames = encoder.stats.frames;
    stats->max_encode_us = cycles_to_us(encoder.stats.max_cycles);
    stats->mean_encode_us = encoder.stats.frames ?
        cycles_to_us(encoder.stats.total_cycles / encoder.stats.frames) : 0U;
    stats->faults = faults;
    return true;
}

uint32_t app_sudo_capture_sample_count(uint64_t id)
{
    return (id != 0U && id == counted_id) ? counted_samples : 0U;
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
    if (!prepared)
        return BC_REC_UNSUPPORTED;
    taskENTER_CRITICAL();
    begun = !initialized && bc_capture_begin(&capture, id);
    taskEXIT_CRITICAL();
    if (!begun)
        return BC_REC_CAPTURE_ERROR;
    /* Fresh codec state, resampler phase and container for every recording.
     * The descriptor must match the one storage wrote at open. */
    bc_resampler_reset(&resampler);
    bc_opus_stream_writer_init(&writer);
    encoder_failed = false;
    tail_flushed = false;
    frame_sequence = 0U;
    counted_id = 0U;
    counted_samples = 0U;
    if (bc_opus_encoder_reset(&encoder) != BC_OPUS_OK || encoder.pre_skip != format.pre_skip ||
        !bc_opus_stream_write_header(&writer, &format)) {
        ++faults;
        taskENTER_CRITICAL();
        (void)bc_capture_quiesced(&capture, id, true);
        taskEXIT_CRITICAL();
        return BC_REC_ENCODER_ERROR;
    }
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
     * An abort cannot report a normal successful saved recording, and the
     * encoder tail is never flushed here: a timeout keeps only the prefix
     * already delivered, reported as PARTIAL. */
    return drained;
}

/* Delivers every due container chunk to the owner as sequenced frames.
 * Returns false when the owner or the writer rejected the data. */
static bool deliver_chunks(bc_recording *owner, uint64_t id, uint32_t now_ms, bool flush)
{
    uint8_t chunk[BC_REC_FRAME_MAX];
    size_t n;
    while ((n = bc_opus_stream_take_chunk(&writer, chunk, sizeof(chunk), flush)) != 0U) {
        bc_rec_result result;
        if (frame_sequence == UINT32_MAX) return false;
        ++frame_sequence;
        result = bc_recording_frame(owner, id, frame_sequence, chunk, (uint16_t)n, now_ms);
        if (result != BC_REC_OK && result != BC_REC_DUPLICATE) return false;
    }
    return true;
}

/* Encodes every completed frame into container records and delivers the
 * chunks that are due. Any codec or container failure is an explicit
 * ENCODER_ERROR fault; audio accepted so far remains a valid prefix. */
static bool encode_pending(bc_recording *owner, uint64_t id, uint32_t now_ms)
{
    static uint8_t packet[BC_OPUS_PACKET_MAX];
    size_t produced;
    while (bc_opus_encoder_pending(&encoder)) {
        if (bc_opus_encoder_encode(&encoder, packet, sizeof(packet), &produced) != BC_OPUS_OK ||
            !canary_intact() ||
            !bc_opus_stream_write_packet(&writer, packet, produced))
            return false;
        if (!deliver_chunks(owner, id, now_ms, false)) return false;
    }
    return true;
}

static bool encode_block(bc_recording *owner, uint64_t id, uint32_t now_ms, const int16_t *block)
{
    int16_t resampled[RESAMPLE_CHUNK];
    size_t consumed = 0U;
    while (consumed < BC_CAPTURE_SAMPLES) {
        size_t used = 0U, fed = 0U;
        size_t count = bc_resampler_process(&resampler, block + consumed,
                                            BC_CAPTURE_SAMPLES - consumed, resampled,
                                            RESAMPLE_CHUNK, &used);
        if (used == 0U && count == 0U) return false;
        consumed += used;
        while (fed < count) {
            bool ready = false;
            size_t taken = bc_opus_encoder_feed(&encoder, resampled + fed, count - fed, &ready);
            fed += taken;
            if (ready && !encode_pending(owner, id, now_ms)) return false;
            if (taken == 0U && !ready) return false;
        }
    }
    return true;
}

/* Runs once when capture is quiescent: pads and flushes the encoder delay,
 * writes the exact sample count trailer and delivers the final chunks. An
 * empty capture leaves nothing behind so the owner reports EMPTY. */
static bool flush_tail(bc_recording *owner, uint64_t id, uint32_t now_ms)
{
    if (tail_flushed) return true;
    tail_flushed = true;
    if (encoder_failed) return false;
    if (bc_opus_encoder_finish(&encoder) != BC_OPUS_OK) return false;
    if (!encode_pending(owner, id, now_ms)) return false;
    if (bc_opus_encoder_sample_count(&encoder) == 0U) {
        (void)bc_opus_stream_discard_header_only(&writer);
        return true;
    }
    if (!bc_opus_stream_write_trailer(&writer, bc_opus_encoder_sample_count(&encoder)))
        return false;
    if (!deliver_chunks(owner, id, now_ms, true)) return false;
    counted_id = id;
    counted_samples = bc_opus_encoder_sample_count(&encoder);
    return true;
}

bool app_sudo_capture_poll(bc_recording *owner, uint32_t now_ms)
{
    uint64_t id;
    uint32_t sequence = 0;
    uint8_t slot;
    bc_rec_result error;
    bool drained;
    bool requested;
    bool stalled;
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
        /* Resample the nominal 16.125 kHz block to 16 kHz and encode outside
         * the interrupt. DMA cannot reuse this slot until it is released. */
        if (!encoder_failed && !encode_block(owner, id, now_ms, samples[slot])) {
            encoder_failed = true;
            ++faults;
            (void)bc_recording_fault(owner, id, BC_REC_ENCODER_ERROR, now_ms);
        }
        taskENTER_CRITICAL();
        (void)bc_capture_release(&capture, id, slot);
        taskEXIT_CRITICAL();
    }
    taskENTER_CRITICAL();
    drained = bc_capture_drained(&capture, id);
    taskEXIT_CRITICAL();
    if (drained) {
        if (!flush_tail(owner, id, now_ms) && !encoder_failed) {
            encoder_failed = true;
            ++faults;
            (void)bc_recording_fault(owner, id, BC_REC_ENCODER_ERROR, now_ms);
        }
        measure_scratch();
        power_down();
        (void)bc_recording_drained(owner, id);
    }
    return slot != BC_CAPTURE_NONE;
}
