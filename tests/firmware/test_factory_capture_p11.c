/* Execute the shipping adapter and codec. Only RTOS/HAL/filesystem are fake. */
#include "app_factory_capture_p11.c"
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "P11 capture line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
unsigned test_critical_depth, test_notify_calls, test_yield_calls;
TickType_t test_ticks;
test_dwt_t test_dwt;
test_coredebug_t test_coredebug;
uint32_t SystemCoreClock = 64000000, test_cycles_per_read = 1000;
volatile uint32_t test_pdm_enable_register;
static TaskHandle_t current_task;
static jmp_buf iteration_done;
static unsigned takes;
static bool in_isr, fail_allocation, fail_init, fail_start, fail_stop, fail_buffer, fail_write;
static bool run_start = true;
static size_t free_heap = 60000;
static nrfx_pdm_event_handler_t handler;
static int16_t *dma[2];
static unsigned dma_count, writes, rolls, file_bytes;
static uint8_t file_data[200000];
static unsigned allocation_count;
#ifdef P11_ADPCM_CONTROL
#include "adpcm_a.h"
/* The pipeline control host shim checks pair packing/length/owner only.
 * Exact supplier IMA code is exercised in the ARM control binary probe. */
void adpcm_encoder(short *input, char *output, int count, adpcm_state *state) {
    int i; CHECK(!in_isr && current_task == encoder_task); CHECK(count > 0 && count <= 64 && !(count & 1));
    for (i = 0; i < count / 2; ++i) output[i] = (char)(input[i*2] & 0xff);
    state->valprev = input[count-1];
}
#endif

void test_task_enter(void) { ++test_critical_depth; }
void test_task_exit(void) { CHECK(test_critical_depth); --test_critical_depth; }
size_t xPortGetFreeHeapSize(void) { return free_heap; }
void *pvPortMalloc(size_t size) {
    CHECK(!in_isr); CHECK(current_task == encoder_task); ++allocation_count;
    if (fail_allocation) return NULL;
    CHECK(size <= free_heap); free_heap -= size;
    return malloc(size);
}
TaskHandle_t xTaskGetCurrentTaskHandle(void) { return current_task; }
void xTaskNotifyGive(TaskHandle_t task) { CHECK(task); CHECK(!in_isr); }
unsigned ulTaskNotifyTake(BaseType_t clear, TickType_t wait) {
    (void)clear; (void)wait; CHECK(!test_critical_depth);
    if (takes++) longjmp(iteration_done, 1);
    return 1;
}
static void encoder_once(void) {
    takes = 0; current_task = (void *)1;
    if (!setjmp(iteration_done)) p11_capture_encoder_thread(NULL);
}
static void writer_once(void) {
    takes = 0; current_task = (void *)2;
    if (!setjmp(iteration_done)) p11_capture_writer_thread(NULL);
}
void vTaskDelay(TickType_t ticks) {
    test_ticks += ticks;
    if (run_start && start_requested) encoder_once();
}
nrfx_err_t nrfx_pdm_init(const nrfx_pdm_config_t *config, nrfx_pdm_event_handler_t cb) {
    CHECK(config); CHECK(!in_isr); CHECK(current_task == encoder_task);
    if (fail_init) return NRFX_ERROR_INTERNAL;
    handler = cb; dma_count = 0; return NRFX_SUCCESS;
}
nrfx_err_t nrfx_pdm_start(void) { return fail_start ? NRFX_ERROR_INTERNAL : NRFX_SUCCESS; }
void nrfx_pdm_uninit(void) { test_pdm_enable_register = 0; dma_count = 0; }
nrfx_err_t nrfx_pdm_stop(void) { return fail_stop ? NRFX_ERROR_INTERNAL : NRFX_SUCCESS; }
nrfx_err_t nrfx_pdm_buffer_set(int16_t *buffer, uint16_t length) {
    CHECK(in_isr); CHECK(length == 880); CHECK(dma_count < 2);
    if (fail_buffer) return NRFX_ERROR_INTERNAL;
    CHECK(slot_for(buffer) != BC_CAPTURE_NONE);
    CHECK(capture.slots[slot_for(buffer)] == BC_CAP_DMA);
    if (dma_count) CHECK(dma[0] != buffer);
    dma[dma_count++] = buffer; test_pdm_enable_register = 1;
    return NRFX_SUCCESS;
}
bool p11_file_write(const uint8_t *data, unsigned length) {
    CHECK(!in_isr && !test_critical_depth); CHECK(current_task == writer_task);
    CHECK(length && length <= 32); CHECK(file_bytes + length <= sizeof(file_data));
    ++writes;
    if (fail_write) return false;
    memcpy(file_data + file_bytes, data, length); file_bytes += length; return true;
}
bool p11_file_rollover(void) {
    CHECK(!in_isr && !test_critical_depth); CHECK(current_task == writer_task);
    ++rolls; return !fail_write;
}
static void event(bool request, int16_t *released, nrfx_pdm_error_t error) {
    nrfx_pdm_evt_t e = {request, released, error};
    unsigned prior_writes = writes, prior_allocs = allocation_count;
    uint32_t prior_encodes = audio.encoder.stats.max_cycles;
    in_isr = true; handler(&e); in_isr = false;
    CHECK(writes == prior_writes && allocation_count == prior_allocs);
    CHECK(audio.encoder.stats.max_cycles == prior_encodes);
}
static void initial_requests(void) { event(true, NULL, 0); if (!stop_issued) event(true, NULL, 0); }
static void full(void) {
    int16_t *released; unsigned i;
    CHECK(dma_count == 2); released = dma[0]; dma[0] = dma[1]; --dma_count;
    for (i = 0; i < 880; ++i) released[i] = (int16_t)(i * 317U + test_ticks);
    test_ticks += pdMS_TO_TICKS(54); event(true, released, 0);
}
static void stopped(void) {
    test_pdm_enable_register = 0;
    event(false, dma_count ? dma[0] : NULL, 0); dma_count = 0;
}
static bool begin(unsigned steps) {
    nrfx_pdm_config_t config = {0};
    CHECK(p11_capture_idle()); CHECK(factory_capture_begin(steps));
    writes = rolls = file_bytes = 0;
    return p11_capture_start(&config);
}
static void finish(bool success) {
    encoder_once(); writer_once(); encoder_once();
    CHECK(p11_capture_idle()); CHECK(factory_capture_quiet());
    CHECK(factory_capture_succeeded() == success);
    if (!success) CHECK(!factory_capture_should_discard());
    factory_capture_end(); CHECK(!factory_capture_active());
}
static void fresh_boot(void) {
    CHECK(!session_active);
#ifndef P11_ADPCM_CONTROL
    p11_opus_set_scratch(NULL, 0);
#endif
    free(workspace);
    workspace = scratch = NULL; memset(&audio, 0, sizeof(audio)); memset(&capture, 0, sizeof(capture));
    prepared = prepare_attempted = false; free_heap = 60000;
    fail_allocation = fail_init = fail_start = fail_stop = fail_buffer = fail_write = false;
    run_start = true;
}
static void good_sessions(void) {
    unsigned n, block, allocs;
    for (n = 0; n < 100; ++n) {
        CHECK(begin(10)); initial_requests();
        allocs = allocation_count;
        if (n == 0) audio.segment_limit = 640; /* Same rollover messages/worker ownership. */
        for (block = 0; block < 1 + n % 30; ++block) {
            full(); encoder_once(); writer_once();
        }
        p11_capture_stop(); full(); CHECK(stop_issued); stopped();
        encoder_once(); CHECK(!factory_capture_quiet()); writer_once(); encoder_once();
        CHECK(factory_capture_succeeded()); CHECK(canary_ok()); CHECK(p11_capture_idle());
#ifdef P11_ADPCM_CONTROL
        CHECK(file_bytes == (block+1) * 220); CHECK(allocation_count == 0);
#else
        CHECK(file_bytes >= 56 && memcmp(file_data, "SOPU", 4) == 0);
        CHECK(file_data[file_bytes-8] == 0 && file_data[file_bytes-6] == 1);
#endif
        CHECK(!factory_capture_should_discard()); /* Short captures are retained on the Ring. */
        CHECK(allocation_count == allocs);
        factory_capture_end();
    }
    CHECK(rolls > 0);
}
static void faults(void) {
    unsigned i;
    /* Full PCM ownership: slow encoder cannot recycle any unconsumed slot. */
    CHECK(begin(10)); initial_requests();
    for (i = 0; i < 7; ++i) full();
    CHECK(stop_issued); stopped(); finish(false);
    /* Writer stalls: bounded packet queue fails closed with durable prefix. */
    CHECK(begin(10)); initial_requests(); writer_once();
    for (i = 0; i < 40 && !stream_fault; ++i) { full(); encoder_once(); }
    CHECK(stream_fault); CHECK(queued <= 64); full(); stopped(); finish(false);
#ifdef P11_ADPCM_CONTROL
    CHECK(file_bytes == 0);
#else
    CHECK(file_bytes == 16);
#endif
    /* Lower write failure is sticky; never short-delete a failed capture. */
    CHECK(begin(10)); initial_requests(); full(); encoder_once();
    fail_write = true; writer_once(); fail_write = false;
    CHECK(stream_fault); full(); stopped(); finish(false);
    /* No hardware progress: bounded forced uninit, no invented DMA tail. */
    CHECK(begin(10)); initial_requests(); p11_capture_stop();
    test_ticks += pdMS_TO_TICKS(201); finish(false);
    /* PDM errors, failed buffer handoff and failed stop all drain owned slots. */
    CHECK(begin(10)); initial_requests(); event(false, NULL, NRFX_PDM_ERROR_OVERFLOW); stopped(); finish(false);
    CHECK(begin(10)); fail_buffer = true; initial_requests(); fail_buffer = false; stopped(); finish(false);
    CHECK(begin(10)); initial_requests(); fail_stop = true; p11_capture_stop(); full();
    fail_stop = false; stopped(); finish(false);
    fail_init = true; CHECK(!begin(10)); fail_init = false; finish(false);
    fail_start = true; CHECK(!begin(10)); fail_start = false; finish(false);
    /* Timed-out startup remains owned until encoder drains; cannot close/restart. */
    run_start = false; CHECK(!begin(10)); CHECK(!p11_capture_idle());
    CHECK(!factory_capture_quiet()); run_start = true; finish(false);
#ifndef P11_ADPCM_CONTROL
    fresh_boot(); fail_allocation = true; CHECK(!begin(10)); finish(false);
    fail_allocation = false; CHECK(!begin(10)); finish(false); /* No repeated heap churn. */
    fresh_boot(); free_heap = 100; CHECK(!begin(10)); finish(false);
    fresh_boot(); CHECK(begin(10)); initial_requests();
    scratch[BC_OPUS_SCRATCH_BYTES] = 0; full(); encoder_once();
    CHECK(stream_fault); full(); stopped(); finish(false);
#endif
}
int main(void) {
    encoder_once(); writer_once(); good_sessions(); faults(); fresh_boot();
    CHECK(!test_critical_depth);
#ifdef P11_ADPCM_CONTROL
    printf("P11 ADPCM control capture (codec shim): %u checks, 0 failures\n", checks);
#else
    printf("P11 capture: %u checks, 0 failures\n", checks);
#endif
    return 0;
}
