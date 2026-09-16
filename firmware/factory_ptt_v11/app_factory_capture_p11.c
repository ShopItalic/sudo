#include "app_factory_capture_p11.h"
#include "app_factory_audio.h"
#include "app_factory_short.h"
#include "bc_capture.h"
#include "nrf.h"
#include "nrf_pdm.h"
#include <string.h>

#if !defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_2_ONE_SEC) || defined(SUDO_VOICE_ONLY)
#error "P11 is an incremental standard 1.23.2 factory target"
#endif
#if NRFX_PDM_CONFIG_IRQ_PRIORITY < configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#error "PDM IRQ priority must permit RTOS notifications"
#endif
#if NRFX_PDM_CONFIG_CLOCK_FREQ != 138412032
#error "Preserve the supplier 1.032 MHz microphone clock"
#endif
#if configUSE_MALLOC_FAILED_HOOK != 0
#error "Optional codec allocation must fail without a fatal malloc hook"
#endif

#define P11_RECORD_QUEUE 64U
#define SCRATCH_FILL 0xa5U
#define CANARY_FILL 0xc5U
typedef struct {
    uint8_t generation[2];
    uint8_t kind, length;
    uint16_t samples;
    uint8_t data[P11_AUDIO_RECORD_BYTES];
} queued_record;
static queued_record records[P11_RECORD_QUEUE];
static unsigned read_at, queued;
static bool writing;
static int16_t pcm[BC_CAPTURE_BUFFERS][BC_CAPTURE_SAMPLES];
static bc_capture capture;
static p11_audio audio;
static TaskHandle_t encoder_task, writer_task;
static nrfx_pdm_config_t requested_config;
static volatile bool start_requested, stop_requested, starting, initialized, stop_issued;
static volatile bool start_ok, session_active, stream_fault;
static bool prepared, prepare_attempted, finishing;
static uint8_t *workspace, *scratch;
static uint32_t session;
static TickType_t last_progress;
volatile p11_capture_diagnostics p11_audio_diagnostics;

static void fault(void)
{
    taskENTER_CRITICAL();
    if (!stream_fault) ++p11_audio_diagnostics.faults;
    stream_fault = true; stop_requested = true;
    if (session_active) bc_capture_fault(&capture, session, BC_REC_ENCODER_ERROR);
    taskEXIT_CRITICAL();
    factory_capture_fault();
}
static bool canary_ok(void)
{
#ifdef P11_ADPCM_CONTROL
    return true;
#else
    unsigned i;
    if (!scratch) return false;
    for (i = 0; i < BC_OPUS_SCRATCH_CANARY_BYTES; ++i)
        if (scratch[BC_OPUS_SCRATCH_BYTES + i] != CANARY_FILL) return false;
    return true;
#endif
}
static void measure(void)
{
#ifndef P11_ADPCM_CONTROL
    unsigned i;
    for (i = BC_OPUS_SCRATCH_BYTES; i > p11_audio_diagnostics.scratch_high_water; --i)
        if (scratch[i-1] != SCRATCH_FILL) { p11_audio_diagnostics.scratch_high_water = i; break; }
    if (audio.encoder.stats.max_cycles > p11_audio_diagnostics.max_encode_cycles)
        p11_audio_diagnostics.max_encode_cycles = audio.encoder.stats.max_cycles;
#endif
}
static uint32_t cycles(void *context) { (void)context; return DWT->CYCCNT; }
static bool discard_test(void *context, p11_audio_record_kind kind,
    const uint8_t *data, unsigned length, unsigned samples)
{ (void)context; (void)kind; (void)data; (void)length; (void)samples; return true; }

static bool sink(void *context, p11_audio_record_kind kind, const uint8_t *data,
                 unsigned length, unsigned samples)
{
    queued_record *record;
    unsigned slot;
    bool ok;
    (void)context;
    if (length > P11_AUDIO_RECORD_BYTES || samples > 320 ||
        (length && !data) || !canary_ok()) return false;
    taskENTER_CRITICAL();
    ok = !stream_fault && queued < P11_RECORD_QUEUE;
    slot = (read_at + queued) % P11_RECORD_QUEUE;
    taskEXIT_CRITICAL();
    if (!ok) return false;
    record = &records[slot];
    record->kind = (uint8_t)kind; record->length = (uint8_t)length;
    record->samples = (uint16_t)samples;
    if (length) memcpy(record->data, data, length);
    if (!factory_capture_queue_begin(record->generation)) return false;
    taskENTER_CRITICAL();
    ++queued;
    if (queued > p11_audio_diagnostics.max_queue_records) p11_audio_diagnostics.max_queue_records = queued;
    taskEXIT_CRITICAL();
    xTaskNotifyGive(writer_task);
    return true;
}
static bool prepare_codec(void)
{
#ifdef P11_ADPCM_CONTROL
    if (prepare_attempted) return prepared;
    prepare_attempted = true;
    p11_audio_diagnostics.pcm_bytes = sizeof(pcm);
    p11_audio_diagnostics.queue_bytes = sizeof(records);
    prepared = p11_audio_init(&audio, NULL, 0, sink, NULL);
    return prepared;
#else
    size_t state_bytes, aligned, total;
    int16_t silence[320] = {0};
    if (prepare_attempted) return prepared;
    prepare_attempted = true;
    state_bytes = bc_opus_encoder_state_size(1);
    if (!state_bytes || state_bytes > BC_OPUS_ENCODER_STATE_MAX) return false;
    aligned = (state_bytes + 7U) & ~(size_t)7U;
    total = aligned + BC_OPUS_SCRATCH_BYTES + BC_OPUS_SCRATCH_CANARY_BYTES;
    p11_audio_diagnostics.pcm_bytes = sizeof(pcm);
    p11_audio_diagnostics.queue_bytes = sizeof(records);
    p11_audio_diagnostics.workspace_bytes = total;
    p11_audio_diagnostics.heap_before = xPortGetFreeHeapSize();
    if (p11_audio_diagnostics.heap_before < total + 8192U) return false;
    ++p11_audio_diagnostics.allocations;
    workspace = pvPortMalloc(total);
    if (!workspace) return false;
    p11_audio_diagnostics.heap_after = xPortGetFreeHeapSize();
    scratch = workspace + aligned;
    memset(scratch, SCRATCH_FILL, BC_OPUS_SCRATCH_BYTES);
    memset(scratch + BC_OPUS_SCRATCH_BYTES, CANARY_FILL, BC_OPUS_SCRATCH_CANARY_BYTES);
    p11_opus_set_scratch(scratch, BC_OPUS_SCRATCH_BYTES);
    if (!p11_audio_init(&audio, workspace, state_bytes, discard_test, NULL)) return false;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    bc_opus_encoder_set_cycle_source(&audio.encoder, cycles, NULL);
    if (!p11_audio_begin(&audio) || !p11_audio_feed(&audio, silence, 320) ||
        !p11_audio_finish(&audio) || !canary_ok()) return false;
    measure();
    audio.sink = sink;
    prepared = true;
    return true;
#endif
}
static uint8_t slot_for(const int16_t *buffer)
{
    uint8_t i;
    for (i = 0; i < BC_CAPTURE_BUFFERS; ++i) if (buffer == pcm[i]) return i;
    return BC_CAPTURE_NONE;
}
/* PDM ISR and IRQ-masked task calls: bounded flags/slot transitions only.
 * No encoding, filesystem, logging, allocation or calendar calls. */
static void stop_peripheral(void)
{
    nrfx_err_t error;
    if (stop_issued) return;
    stop_issued = true;
    error = nrfx_pdm_stop();
    if (error != NRFX_SUCCESS) bc_capture_fault(&capture, session, BC_REC_CAPTURE_ERROR);
    if (!nrf_pdm_enable_check()) bc_capture_quiesced(&capture, session, error != NRFX_SUCCESS);
}
static void pdm_event(const nrfx_pdm_evt_t *event)
{
    BaseType_t wake = pdFALSE;
    uint8_t slot;
    if (!initialized || !capture.running) return;
    last_progress = xTaskGetTickCountFromISR();
    if (event->error != NRFX_PDM_NO_ERROR) {
        bc_capture_fault(&capture, session, BC_REC_CAPTURE_OVERFLOW);
        stop_peripheral();
    } else if (event->buffer_requested) {
        if (event->buffer_released) {
            slot = slot_for(event->buffer_released);
            if (!bc_capture_full(&capture, session, slot)) stop_peripheral();
            if (stop_requested) bc_capture_stop(&capture, session);
            if (capture.stop_requested) stop_peripheral();
        }
        if (!stop_issued) {
            slot = bc_capture_acquire_dma(&capture, session);
            if (slot == BC_CAPTURE_NONE || nrfx_pdm_buffer_set(pcm[slot], BC_CAPTURE_SAMPLES) != NRFX_SUCCESS) {
                bc_capture_fault(&capture, session, BC_REC_CAPTURE_OVERFLOW);
                stop_peripheral();
            }
        }
    } else if (!nrf_pdm_enable_check()) bc_capture_quiesced(&capture, session, false);
    vTaskNotifyGiveFromISR(encoder_task, &wake);
    portYIELD_FROM_ISR(wake);
}
bool p11_capture_idle(void)
{
    bool idle;
    taskENTER_CRITICAL();
    idle = !session_active && !starting && !start_requested && !initialized && !queued && !writing;
    taskEXIT_CRITICAL();
    return idle;
}
bool p11_capture_start(const nrfx_pdm_config_t *config)
{
    TickType_t began;
    if (!config || !encoder_task || !writer_task || !p11_capture_idle()) return false;
    if (!factory_capture_p11_hold()) return false;
    taskENTER_CRITICAL();
    requested_config = *config;
    stream_fault = stop_requested = start_ok = false;
    starting = start_requested = true;
    taskEXIT_CRITICAL();
    factory_capture_enable();
    xTaskNotifyGive(encoder_task);
    began = xTaskGetTickCount();
    while (starting) {
        if ((TickType_t)(xTaskGetTickCount() - began) >= pdMS_TO_TICKS(2000)) {
            fault(); xTaskNotifyGive(encoder_task); return false;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return start_ok;
}
void p11_capture_stop(void)
{
    taskENTER_CRITICAL(); stop_requested = true; taskEXIT_CRITICAL();
    if (encoder_task) xTaskNotifyGive(encoder_task);
}
static void start_session(void)
{
    bool ok;
    start_requested = false;
    session_active = true; finishing = false; stop_issued = false;
    if (++session == 0) ++session;
    ok = !stop_requested && prepare_codec() && bc_capture_begin(&capture, session) && p11_audio_begin(&audio);
    if (ok && !stop_requested) {
        ok = nrfx_pdm_init(&requested_config, pdm_event) == NRFX_SUCCESS;
        initialized = ok;
        if (ok) {
            last_progress = xTaskGetTickCount();
            ok = nrfx_pdm_start() == NRFX_SUCCESS;
        }
    } else ok = false;
    if (!ok) {
        fault();
        if (initialized) { nrfx_pdm_uninit(); initialized = false; }
        if (capture.id == session) bc_capture_quiesced(&capture, session, true);
        audio.active = false;
        finishing = true;
        factory_capture_stop_accepting(); factory_capture_producer_leave();
    }
    start_ok = ok;
    starting = false;
}
void p11_capture_encoder_thread(void *argument)
{
    (void)argument;
    encoder_task = xTaskGetCurrentTaskHandle();
    for (;;) {
        uint8_t slot;
        uint32_t sequence;
        bool drained;
        ulTaskNotifyTake(pdTRUE, session_active ? pdMS_TO_TICKS(25) : portMAX_DELAY);
        if (start_requested) start_session();
        if (!session_active) continue;
        if (!finishing) {
            taskENTER_CRITICAL();
            if (stop_requested) bc_capture_stop(&capture, session);
            if (initialized && !capture.quiescent &&
                (TickType_t)(xTaskGetTickCount() - last_progress) >= pdMS_TO_TICKS(200)) {
                /* Driver stalled: disable before relinquishing DMA slots.
                 * No partial DMA sample count is invented. */
                nrfx_pdm_uninit(); initialized = false;
                bc_capture_quiesced(&capture, session, true);
            }
            taskEXIT_CRITICAL();
            for (;;) {
                taskENTER_CRITICAL(); slot = bc_capture_take(&capture, session, &sequence); taskEXIT_CRITICAL();
                if (slot == BC_CAPTURE_NONE) break;
                if (!stream_fault && (!p11_audio_feed(&audio, pcm[slot], BC_CAPTURE_SAMPLES) || !canary_ok())) fault();
                ++p11_audio_diagnostics.completed_blocks;
                taskENTER_CRITICAL(); bc_capture_release(&capture, session, slot); taskEXIT_CRITICAL();
            }
            taskENTER_CRITICAL(); drained = bc_capture_drained(&capture, session); taskEXIT_CRITICAL();
            if (capture.error != BC_REC_OK) fault();
            if (drained) {
                if (initialized) { nrfx_pdm_uninit(); initialized = false; }
                if (!stream_fault && (!p11_audio_finish(&audio) || !canary_ok())) fault();
                audio.active = false;
                if (scratch) measure();
                finishing = true;
                factory_capture_stop_accepting(); factory_capture_producer_leave();
            }
        }
        taskENTER_CRITICAL();
        if (finishing && !queued && !writing) session_active = false;
        taskEXIT_CRITICAL();
    }
}
void p11_capture_writer_thread(void *argument)
{
    (void)argument;
    writer_task = xTaskGetCurrentTaskHandle();
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        for (;;) {
            queued_record *record;
            bool available, entered, ok;
            taskENTER_CRITICAL(); available = queued != 0; writing = available; record = &records[read_at]; taskEXIT_CRITICAL();
            if (!available) break;
            entered = factory_capture_sender_enter(record->generation);
            ok = entered && !stream_fault;
            if (ok) ok = record->kind == P11_AUDIO_ROLLOVER ? p11_file_rollover() : p11_file_write(record->data, record->length);
            if (entered) {
                factory_capture_p11_samples(record->samples, ok);
                factory_capture_sender_leave();
            }
            if (!ok) fault();
            taskENTER_CRITICAL();
            read_at = (read_at + 1) % P11_RECORD_QUEUE; --queued; writing = false;
            taskEXIT_CRITICAL();
            xTaskNotifyGive(encoder_task);
        }
    }
}
