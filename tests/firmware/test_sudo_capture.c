#include "app_sudo_capture.h"
#include "adpcm_a.h"
#include "bc_capture.h"
#include "nrfx_pdm.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ARRAY_LEN(value) (sizeof(value) / sizeof((value)[0]))
#define TEST_MAX_CODEC_CALLS 64U
#define TEST_STORAGE_CAPACITY (BC_REC_FRAME_MAX * TEST_MAX_CODEC_CALLS)

static unsigned checks;
static unsigned failures;

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

#define CHECK(condition) \
    check_condition((condition), #condition, __LINE__)
#define REQUIRE(condition) \
    do { \
        if (!(condition)) { \
            check_condition(false, #condition, __LINE__); \
            return false; \
        } \
        check_condition(true, #condition, __LINE__); \
    } while (0)

/* FreeRTOS critical-section and ISR-notification shims. */
unsigned test_critical_depth;
unsigned test_notify_calls;
unsigned test_yield_calls;
TickType_t test_ticks;

void test_task_enter(void)
{
    ++test_critical_depth;
}

void test_task_exit(void)
{
    CHECK(test_critical_depth > 0U);
    if (test_critical_depth != 0U)
        --test_critical_depth;
}

/* The HAL shim exposes the same enabled bit queried by the production code. */
volatile uint32_t test_pdm_enable_register;
test_pdm_observation test_pdm;

void test_pdm_reset(void)
{
    memset(&test_pdm, 0, sizeof(test_pdm));
    test_pdm_enable_register = 0U;
}

nrfx_err_t nrfx_pdm_init(nrfx_pdm_config_t const *config,
                         nrfx_pdm_event_handler_t handler)
{
    ++test_pdm.init_calls;
    if (config == NULL || handler == NULL)
        return NRFX_ERROR_INVALID_PARAM;
    if (test_pdm.init_error != NRFX_SUCCESS)
        return test_pdm.init_error;
    if (test_pdm.initialized)
        return NRFX_ERROR_INVALID_STATE;

    test_pdm.config = *config;
    test_pdm.handler = handler;
    test_pdm.buffers[0] = NULL;
    test_pdm.buffers[1] = NULL;
    test_pdm.active_buffer = 0U;
    test_pdm.initialized = true;
    test_pdm.irq_enabled = true;
    test_pdm.pending_irq = false;
    test_pdm.stop_requested = false;
    return NRFX_SUCCESS;
}

void nrfx_pdm_uninit(void)
{
    ++test_pdm.uninit_calls;
    test_pdm.initialized = false;
    test_pdm.state = TEST_PDM_IDLE;
    test_pdm_enable_register = 0U;
    /* The production power_down() disables the IRQ and clears pending after
     * calling uninit. Keep those operations visible to the test macros. */
}

nrfx_err_t nrfx_pdm_start(void)
{
    ++test_pdm.start_calls;
    if (test_pdm.start_error != NRFX_SUCCESS)
        return test_pdm.start_error;
    if (!test_pdm.initialized)
        return NRFX_ERROR_INVALID_STATE;
    if (test_pdm.state != TEST_PDM_IDLE)
        return NRFX_ERROR_BUSY;

    /* nrfx_pdm_start() only posts its initial buffer request. The callback is
     * delivered later by test_pdm_deliver_pending_request(). */
    test_pdm.state = TEST_PDM_STARTING;
    test_pdm.pending_irq = true;
    return NRFX_SUCCESS;
}

nrfx_err_t nrfx_pdm_buffer_set(int16_t *buffer, uint16_t length)
{
    unsigned target;

    ++test_pdm.buffer_set_calls;
    if (test_pdm.buffer_set_calls <= ARRAY_LEN(test_pdm.buffer_history))
    {
        unsigned index = test_pdm.buffer_set_calls - 1U;
        test_pdm.buffer_history[index] = buffer;
        test_pdm.length_history[index] = length;
    }
    if (test_pdm.buffer_set_error != NRFX_SUCCESS)
        return test_pdm.buffer_set_error;
    if (!test_pdm.initialized)
        return NRFX_ERROR_INVALID_STATE;
    if (buffer == NULL || length > 32767U)
        return NRFX_ERROR_INVALID_PARAM;

    if (test_pdm.state == TEST_PDM_STARTING)
        target = 0U;
    else if (test_pdm.state == TEST_PDM_RUNNING)
        target = 1U - test_pdm.active_buffer;
    else if (test_pdm.state == TEST_PDM_IDLE)
        target = 0U;
    else
        return NRFX_ERROR_BUSY;

    if (test_pdm.buffers[target] != NULL)
        return NRFX_ERROR_BUSY;
    test_pdm.buffers[target] = buffer;
    test_pdm.lengths[target] = length;
    if (test_pdm.state == TEST_PDM_STARTING || test_pdm.state == TEST_PDM_IDLE)
        test_pdm_enable_register = 1U;
    return NRFX_SUCCESS;
}

nrfx_err_t nrfx_pdm_stop(void)
{
    ++test_pdm.stop_calls;
    if (test_pdm.stop_error != NRFX_SUCCESS)
        return test_pdm.stop_error;
    if (!test_pdm.initialized)
        return NRFX_ERROR_INVALID_STATE;

    if (test_pdm.state == TEST_PDM_RUNNING)
    {
        test_pdm.state = TEST_PDM_STOPPING;
        test_pdm.stop_requested = true;
        /* The real driver leaves the peripheral enabled until STOPPED. */
        return NRFX_SUCCESS;
    }
    if (test_pdm.state == TEST_PDM_STARTING ||
        test_pdm.state == TEST_PDM_IDLE)
    {
        test_pdm.state = TEST_PDM_IDLE;
        test_pdm_enable_register = 0U;
        return NRFX_SUCCESS;
    }
    return NRFX_ERROR_BUSY;
}

bool test_pdm_deliver_pending_request(void)
{
    nrfx_pdm_evt_t event;
    if (!test_pdm.initialized || !test_pdm.irq_enabled ||
        !test_pdm.pending_irq || test_pdm.handler == NULL)
        return false;
    test_pdm.pending_irq = false;
    event.buffer_requested = true;
    event.buffer_released = NULL;
    event.error = NRFX_PDM_NO_ERROR;
    test_pdm.handler(&event);
    return true;
}

bool test_pdm_emit_started(void)
{
    nrfx_pdm_evt_t event;
    if (!test_pdm.initialized || !test_pdm.irq_enabled ||
        test_pdm.state != TEST_PDM_STARTING || test_pdm.buffers[0] == NULL ||
        test_pdm.handler == NULL)
        return false;
    test_pdm.state = TEST_PDM_RUNNING;
    test_pdm.active_buffer = 0U;
    event.buffer_requested = true;
    event.buffer_released = NULL;
    event.error = NRFX_PDM_NO_ERROR;
    test_pdm.handler(&event);
    return true;
}

int16_t *test_pdm_active_buffer(void)
{
    if (!test_pdm.initialized ||
        (test_pdm.state != TEST_PDM_RUNNING &&
         test_pdm.state != TEST_PDM_STOPPING))
        return NULL;
    return test_pdm.buffers[test_pdm.active_buffer];
}

bool test_pdm_emit_full(void)
{
    nrfx_pdm_evt_t event;
    unsigned next;
    int16_t *released;

    if (!test_pdm.initialized || !test_pdm.irq_enabled ||
        test_pdm.state != TEST_PDM_RUNNING || test_pdm.handler == NULL)
        return false;
    next = 1U - test_pdm.active_buffer;
    if (test_pdm.buffers[next] == NULL)
        return test_pdm_emit_overflow();

    released = test_pdm.buffers[test_pdm.active_buffer];
    test_pdm.buffers[test_pdm.active_buffer] = NULL;
    test_pdm.active_buffer = next;
    test_pdm.last_released = released;
    event.buffer_requested = true;
    event.buffer_released = released;
    event.error = NRFX_PDM_NO_ERROR;
    test_pdm.handler(&event);
    return true;
}

bool test_pdm_emit_overflow(void)
{
    nrfx_pdm_evt_t event;
    if (!test_pdm.initialized || !test_pdm.irq_enabled ||
        test_pdm.handler == NULL)
        return false;
    event.buffer_requested = false;
    event.buffer_released = NULL;
    event.error = NRFX_PDM_ERROR_OVERFLOW;
    test_pdm.handler(&event);
    return true;
}

bool test_pdm_emit_stopped(void)
{
    nrfx_pdm_evt_t event;
    unsigned first;
    unsigned i;

    if (!test_pdm.initialized || !test_pdm.irq_enabled ||
        test_pdm.state != TEST_PDM_STOPPING || test_pdm.handler == NULL)
        return false;

    /* nrfx disables PDM before issuing up to two buffer_released callbacks. */
    test_pdm_enable_register = 0U;
    test_pdm.state = TEST_PDM_IDLE;
    first = test_pdm.active_buffer;
    for (i = 0U; i < 2U; ++i)
    {
        unsigned index = (first + i) & 1U;
        int16_t *released = test_pdm.buffers[index];
        if (released == NULL)
            continue;
        test_pdm.buffers[index] = NULL;
        event.buffer_requested = false;
        event.buffer_released = released;
        event.error = NRFX_PDM_NO_ERROR;
        test_pdm.handler(&event);
    }
    return true;
}

/* Deterministic replacement for the supplier archive call. */
typedef struct {
    unsigned init_calls;
    unsigned encode_calls;
    int lengths[TEST_MAX_CODEC_CALLS];
    short inputs[TEST_MAX_CODEC_CALLS][BC_CAPTURE_SAMPLES / 2U];
    uint8_t outputs[TEST_MAX_CODEC_CALLS][BC_REC_FRAME_MAX];
    adpcm_state state_before[TEST_MAX_CODEC_CALLS];
    adpcm_state state_after[TEST_MAX_CODEC_CALLS];
} codec_observation;

codec_observation test_codec;

static void test_codec_reset(void)
{
    memset(&test_codec, 0, sizeof(test_codec));
}

void mono_adpcm_init(MonoAdpcmProcessor *processor)
{
    ++test_codec.init_calls;
    if (processor != NULL)
    {
        processor->mono_state.valprev = 0;
        processor->mono_state.index = 0;
    }
}

void adpcm_encoder(short *indata, char *outdata, int len, adpcm_state *state)
{
    unsigned call = test_codec.encode_calls;
    unsigned i;

    ++test_codec.encode_calls;
    if (call >= TEST_MAX_CODEC_CALLS || indata == NULL || outdata == NULL ||
        state == NULL)
        return;

    test_codec.lengths[call] = len;
    test_codec.state_before[call] = *state;
    for (i = 0U; i < BC_CAPTURE_SAMPLES / 2U; ++i)
        test_codec.inputs[call][i] = i < (unsigned)len ? indata[i] : 0;
    for (i = 0U; i < BC_REC_FRAME_MAX; ++i)
    {
        uint16_t sample = (uint16_t)test_codec.inputs[call][i % (BC_CAPTURE_SAMPLES / 2U)];
        uint8_t value = (uint8_t)(sample ^ (uint16_t)(i +
            (uint8_t)state->index));
        outdata[i] = (char)value;
        test_codec.outputs[call][i] = value;
    }
    if (len > 0)
        state->valprev = indata[len - 1];
    state->index = (char)(state->index + 1);
    test_codec.state_after[call] = *state;
}

unsigned test_power_on_calls;
unsigned test_power_off_calls;

void bc_ldo_mic_power_on(void)
{
    ++test_power_on_calls;
}

void bc_ldo_mic_power_off(void)
{
    ++test_power_off_calls;
}

static void test_power_reset(void)
{
    test_power_on_calls = 0U;
    test_power_off_calls = 0U;
}

/* Observable local-storage port for the actual bc_recording owner. */
typedef struct {
    uint8_t bytes[TEST_STORAGE_CAPACITY];
    uint32_t byte_count;
    uint32_t frame_count;
    uint16_t append_lengths[TEST_MAX_CODEC_CALLS];
    unsigned open_calls;
    unsigned append_calls;
    unsigned checkpoint_calls;
    unsigned finish_calls;
    unsigned changed_calls;
    bool active;
    bool last_finish_complete;
    bc_rec_file last_checkpoint;
    bc_rec_file last_finish;
    bc_rec_result open_error;
    bc_rec_result append_error;
    unsigned append_error_call;
    bc_rec_result checkpoint_error;
    bc_rec_result finish_error;
} storage_fixture;

static uint32_t crc32_bytes(const uint8_t *data, uint32_t length)
{
    uint32_t state = 0xffffffffU;
    uint32_t i;
    for (i = 0U; i < length; ++i)
    {
        unsigned bit;
        state ^= data[i];
        for (bit = 0U; bit < 8U; ++bit)
            state = (state >> 1) ^ (0xedb88320UL & (0U - (state & 1U)));
    }
    return state ^ 0xffffffffU;
}

static void storage_reset(storage_fixture *storage)
{
    memset(storage, 0, sizeof(*storage));
    storage->open_error = BC_REC_OK;
    storage->append_error = BC_REC_OK;
    storage->checkpoint_error = BC_REC_OK;
    storage->finish_error = BC_REC_OK;
}

static bc_rec_result storage_open(void *context, const bc_rec_start *start,
                                  bc_rec_file *file)
{
    storage_fixture *storage = context;
    ++storage->open_calls;
    if (storage->open_error != BC_REC_OK)
        return storage->open_error;
    if (storage->active)
        return BC_REC_BUSY;
    storage->active = true;
    memset(file, 0, sizeof(*file));
    (void)snprintf(file->name, sizeof(file->name), "capture-%llx.raw",
                   (unsigned long long)start->id);
    return BC_REC_OK;
}

static bc_rec_result storage_append(void *context, const uint8_t *data,
                                    uint16_t length)
{
    storage_fixture *storage = context;
    ++storage->append_calls;
    if (storage->append_error != BC_REC_OK &&
        storage->append_calls == storage->append_error_call)
        return storage->append_error;
    if (data == NULL || length > sizeof(storage->bytes) - storage->byte_count)
        return BC_REC_WRITE_ERROR;
    if (storage->frame_count < ARRAY_LEN(storage->append_lengths))
        storage->append_lengths[storage->frame_count] = length;
    memcpy(storage->bytes + storage->byte_count, data, length);
    storage->byte_count += length;
    ++storage->frame_count;
    return BC_REC_OK;
}

static bc_rec_result storage_checkpoint(void *context, bc_rec_file *file)
{
    storage_fixture *storage = context;
    ++storage->checkpoint_calls;
    storage->last_checkpoint = *file;
    if (storage->checkpoint_error != BC_REC_OK)
        return storage->checkpoint_error;
    file->bytes = storage->byte_count;
    file->frames = storage->frame_count;
    file->crc32 = crc32_bytes(storage->bytes, storage->byte_count);
    file->complete = false;
    file->recovered = false;
    storage->last_checkpoint = *file;
    return BC_REC_OK;
}

static bc_rec_result storage_finish(void *context, bool complete,
                                    bc_rec_file *file)
{
    storage_fixture *storage = context;
    ++storage->finish_calls;
    storage->last_finish_complete = complete;
    file->bytes = storage->byte_count;
    file->frames = storage->frame_count;
    file->crc32 = crc32_bytes(storage->bytes, storage->byte_count);
    file->complete = complete;
    file->recovered = !complete;
    storage->last_finish = *file;
    if (storage->finish_error != BC_REC_OK)
        return storage->finish_error;
    storage->active = false;
    return BC_REC_OK;
}

static void storage_changed(void *context, const bc_rec_snapshot *snapshot)
{
    storage_fixture *storage = context;
    (void)snapshot;
    ++storage->changed_calls;
}

static bc_rec_port storage_port(storage_fixture *storage)
{
    bc_rec_port port;
    memset(&port, 0, sizeof(port));
    port.ctx = storage;
    port.open = storage_open;
    port.append = storage_append;
    port.checkpoint = storage_checkpoint;
    port.finish = storage_finish;
    port.capture_start = app_sudo_capture_start;
    port.capture_stop = app_sudo_capture_stop;
    port.capture_abort = app_sudo_capture_abort;
    port.changed = storage_changed;
    return port;
}

static bc_rec_config capture_config(void)
{
    bc_rec_config config;
    config.checkpoint_ms = BC_REC_MAX_INTERVAL;
    config.checkpoint_bytes = BC_REC_MAX_INTERVAL;
    config.stop_timeout_ms = 25U;
    return config;
}

static bc_rec_start capture_start(uint64_t id)
{
    bc_rec_start start;
    memset(&start, 0, sizeof(start));
    start.id = id;
    start.trigger = BC_REC_PTT;
    return start;
}

static bc_rec_start capture_start_with_trigger(uint64_t id,
                                               bc_rec_trigger trigger)
{
    bc_rec_start start;
    memset(&start, 0, sizeof(start));
    start.id = id;
    start.trigger = trigger;
    return start;
}

static bool begin_recording_with_start(storage_fixture *storage,
                                       bc_recording *recording,
                                       const bc_rec_start *start)
{
    bc_rec_port port = storage_port(storage);
    bc_rec_config config = capture_config();

    REQUIRE(!test_pdm.initialized);
    storage_reset(storage);
    test_pdm_reset();
    test_codec_reset();
    test_power_reset();
    test_ticks = 0U;
    REQUIRE(bc_recording_init(recording, &port, &config));
    REQUIRE(bc_recording_start(recording, start, 0U) == BC_REC_OK);
    REQUIRE(bc_recording_snapshot(recording)->phase == BC_REC_RECORDING);
    REQUIRE(test_pdm.pending_irq);
    REQUIRE(test_pdm_deliver_pending_request());
    REQUIRE(test_codec.encode_calls == 0U);
    REQUIRE(test_pdm.buffer_set_calls == 1U);
    REQUIRE(test_pdm_emit_started());
    REQUIRE(test_codec.encode_calls == 0U);
    REQUIRE(test_pdm.buffer_set_calls == 2U);
    REQUIRE(test_pdm.state == TEST_PDM_RUNNING);
    REQUIRE(test_pdm_enable_register != 0U);
    return true;
}

static bool begin_recording(storage_fixture *storage, bc_recording *recording,
                            uint64_t id)
{
    bc_rec_start start = capture_start(id);
    return begin_recording_with_start(storage, recording, &start);
}

static bool start_recording_without_irq(storage_fixture *storage,
                                        bc_recording *recording,
                                        const bc_rec_start *start,
                                        TickType_t initial_ticks)
{
    bc_rec_port port = storage_port(storage);
    bc_rec_config config = capture_config();

    REQUIRE(!test_pdm.initialized);
    storage_reset(storage);
    test_pdm_reset();
    test_codec_reset();
    test_power_reset();
    test_ticks = initial_ticks;
    REQUIRE(bc_recording_init(recording, &port, &config));
    REQUIRE(bc_recording_start(recording, start, 0U) == BC_REC_OK);
    REQUIRE(bc_recording_snapshot(recording)->phase == BC_REC_RECORDING);
    REQUIRE(test_pdm.pending_irq);
    REQUIRE(test_pdm.buffer_set_calls == 0U);
    return true;
}

static void fill_current(int16_t base)
{
    int16_t *buffer = test_pdm_active_buffer();
    unsigned i;
    CHECK(buffer != NULL);
    if (buffer == NULL)
        return;
    for (i = 0U; i < BC_CAPTURE_SAMPLES; ++i)
        buffer[i] = (int16_t)(base + (int16_t)i);
}

static bool stop_and_drain(storage_fixture *storage, bc_recording *recording,
                           uint64_t id, uint32_t now_ms)
{
    unsigned i;
    const bc_rec_snapshot *snapshot;

    snapshot = bc_recording_snapshot(recording);
    if (snapshot != NULL && bc_recording_active(recording) &&
        snapshot->phase == BC_REC_RECORDING)
        (void)bc_recording_stop(recording, id, now_ms);

    if (test_pdm.state == TEST_PDM_RUNNING)
    {
        fill_current(0);
        (void)test_pdm_emit_full();
    }
    if (test_pdm.state == TEST_PDM_STOPPING)
        (void)test_pdm_emit_stopped();
    for (i = 0U; i < TEST_MAX_CODEC_CALLS &&
                 bc_recording_active(recording); ++i)
        (void)app_sudo_capture_poll(recording, now_ms + i + 1U);

    snapshot = bc_recording_snapshot(recording);
    (void)storage;
    return snapshot != NULL && !bc_recording_active(recording);
}

static bool test_initial_request_and_encoding_contract(void)
{
    storage_fixture storage;
    bc_recording recording;
    int16_t *first;
    int16_t *replacement;
    bc_rec_start start = capture_start(0x101U);
    unsigned sets;
    unsigned i;

    REQUIRE(begin_recording(&storage, &recording, start.id));
    first = test_pdm_active_buffer();
    REQUIRE(first != NULL);
    fill_current(1000);
    REQUIRE(test_pdm_emit_full());
    CHECK(test_codec.encode_calls == 0U);
    CHECK(storage.byte_count == 0U);
    CHECK(test_pdm.buffer_set_calls == 3U);
    replacement = test_pdm.buffer_history[2];
    CHECK(replacement != NULL && replacement != first);

    /* The first released slot is still READY. A second completion must choose
     * another capture slot, so the DMA pointer cannot be reused early. */
    fill_current(2000);
    REQUIRE(test_pdm_emit_full());
    CHECK(test_codec.encode_calls == 0U);
    CHECK(test_pdm.buffer_set_calls == 4U);
    CHECK(test_pdm.buffer_history[3] != first);

    CHECK(app_sudo_capture_poll(&recording, 10U));
    CHECK(test_codec.encode_calls == 1U);
    CHECK(test_codec.lengths[0] == (int)(BC_CAPTURE_SAMPLES / 2U));
    CHECK(storage.frame_count == 1U);
    CHECK(storage.byte_count == BC_REC_FRAME_MAX);
    for (i = 0U; i < BC_CAPTURE_SAMPLES / 2U; ++i)
        CHECK(test_codec.inputs[0][i] == (short)(1000 + (int)(i * 2U)));
    CHECK(storage.append_lengths[0] == BC_REC_FRAME_MAX);
    for (i = 0U; i < BC_REC_FRAME_MAX; ++i)
        CHECK(storage.bytes[i] == test_codec.outputs[0][i]);

    /* Once the encoder releases the first slot, the next completed boundary
     * is allowed to reuse precisely that DMA buffer. */
    fill_current(3000);
    REQUIRE(test_pdm_emit_full());
    sets = test_pdm.buffer_set_calls;
    CHECK(sets == 5U);
    CHECK(test_pdm.buffer_history[4] == first);
    CHECK(app_sudo_capture_poll(&recording, 20U));
    CHECK(test_codec.encode_calls == 2U);
    CHECK(test_codec.state_before[1].valprev ==
          test_codec.state_after[0].valprev);
    CHECK(test_codec.state_before[1].index == test_codec.state_after[0].index);
    CHECK(storage.frame_count == 2U);
    CHECK(storage.byte_count == 2U * BC_REC_FRAME_MAX);
    for (i = 0U; i < BC_REC_FRAME_MAX; ++i)
        CHECK(storage.bytes[BC_REC_FRAME_MAX + i] == test_codec.outputs[1][i]);

    CHECK(stop_and_drain(&storage, &recording, start.id, 30U));
    CHECK(storage.finish_calls == 1U);
    CHECK(storage.last_finish_complete);
    CHECK(storage.frame_count == 4U);
    CHECK(storage.byte_count == 4U * BC_REC_FRAME_MAX);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);
    return true;
}

static bool test_stop_boundary_and_stopped_tail(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_start start = capture_start(0x202U);
    unsigned sets;

    REQUIRE(begin_recording(&storage, &recording, start.id));
    fill_current(400);
    REQUIRE(test_pdm_emit_full());
    CHECK(test_codec.encode_calls == 0U);
    CHECK(bc_recording_stop(&recording, start.id, 50U) == BC_REC_OK);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    CHECK(test_pdm.stop_calls == 0U);
    CHECK(test_pdm_enable_register != 0U);

    /* Stop is deferred until the next full PDM boundary. */
    sets = test_pdm.buffer_set_calls;
    fill_current(500);
    REQUIRE(test_pdm_emit_full());
    CHECK(test_pdm.stop_calls == 1U);
    CHECK(test_pdm.state == TEST_PDM_STOPPING);
    CHECK(test_pdm_enable_register != 0U);
    CHECK(test_pdm.buffer_set_calls == sets);
    CHECK(storage.finish_calls == 0U);

    /* The remaining hardware buffer is unfinished. STOPPED releases it, but
     * buffer_requested=false means it cannot become a third audio frame. */
    REQUIRE(test_pdm_emit_stopped());
    CHECK(test_pdm_enable_register == 0U);
    CHECK(app_sudo_capture_poll(&recording, 60U));
    CHECK(app_sudo_capture_poll(&recording, 61U));
    CHECK(!bc_recording_active(&recording));
    CHECK(storage.finish_calls == 1U);
    CHECK(storage.last_finish_complete);
    CHECK(storage.frame_count == 2U);
    CHECK(storage.byte_count == 2U * BC_REC_FRAME_MAX);
    CHECK(test_codec.encode_calls == 2U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);
    return true;
}

static bool test_overflow_retains_ready_tail_as_partial(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_start start = capture_start(0x303U);

    REQUIRE(begin_recording(&storage, &recording, start.id));
    fill_current(700);
    REQUIRE(test_pdm_emit_full());
    fill_current(800);
    REQUIRE(test_pdm_emit_full());
    CHECK(test_codec.encode_calls == 0U);
    CHECK(storage.frame_count == 0U);
    REQUIRE(test_pdm_emit_overflow());
    CHECK(test_pdm.stop_calls == 1U);
    CHECK(test_pdm.state == TEST_PDM_STOPPING);
    REQUIRE(test_pdm_emit_stopped());

    CHECK(app_sudo_capture_poll(&recording, 100U));
    CHECK(test_codec.encode_calls == 1U);
    CHECK(storage.frame_count == 1U);
    CHECK(storage.finish_calls == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_CAPTURE_OVERFLOW);

    CHECK(app_sudo_capture_poll(&recording, 101U));
    CHECK(test_codec.encode_calls == 2U);
    CHECK(storage.frame_count == 2U);
    CHECK(storage.byte_count == 2U * BC_REC_FRAME_MAX);
    CHECK(storage.finish_calls == 1U);
    CHECK(!storage.last_finish_complete);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(!bc_recording_snapshot(&recording)->file.complete);
    return true;
}

static bool test_abort_timeout_retains_ready_tail(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_start start = capture_start(0x404U);

    REQUIRE(begin_recording(&storage, &recording, start.id));
    fill_current(900);
    REQUIRE(test_pdm_emit_full());
    CHECK(bc_recording_stop(&recording, start.id, 200U) == BC_REC_OK);
    CHECK(test_pdm.stop_calls == 0U);
    bc_recording_tick(&recording, 226U);
    CHECK(test_pdm_enable_register == 0U);
    CHECK(!test_pdm.irq_enabled);
    CHECK(!test_pdm.pending_irq);
    CHECK(test_codec.encode_calls == 0U);
    CHECK(storage.byte_count == 0U);
    CHECK(storage.finish_calls == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_STOP_TIMEOUT);

    CHECK(app_sudo_capture_poll(&recording, 227U));
    CHECK(test_codec.encode_calls == 1U);
    CHECK(storage.frame_count == 1U);
    CHECK(storage.byte_count == BC_REC_FRAME_MAX);
    CHECK(storage.finish_calls == 1U);
    CHECK(!storage.last_finish_complete);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(!bc_recording_snapshot(&recording)->file.complete);
    return true;
}

static bool test_start_failures_are_quiescent_and_restartable(void)
{
    const nrfx_err_t failures_to_inject[] = {
        NRFX_ERROR_INTERNAL,
        NRFX_ERROR_BUSY
    };
    unsigned mode;

    for (mode = 0U; mode < ARRAY_LEN(failures_to_inject); ++mode)
    {
        storage_fixture storage;
        bc_recording recording;
        bc_rec_port port;
        bc_rec_config config = capture_config();
        bc_rec_start failed = capture_start(0x500U + mode);
        bc_rec_start fresh = capture_start(0x600U + mode);
        bc_rec_result result;

        storage_reset(&storage);
        test_pdm_reset();
        test_codec_reset();
        test_power_reset();
        port = storage_port(&storage);
        REQUIRE(bc_recording_init(&recording, &port, &config));
        if (mode == 0U)
            test_pdm.init_error = failures_to_inject[mode];
        else
            test_pdm.start_error = failures_to_inject[mode];
        result = bc_recording_start(&recording, &failed, 300U);
        CHECK(result == BC_REC_CAPTURE_ERROR);
        CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_FAILED);
        CHECK(!bc_recording_active(&recording));
        CHECK(storage.finish_calls == 1U);
        CHECK(!storage.last_finish_complete);
        CHECK(test_pdm_enable_register == 0U);
        CHECK(!test_pdm.irq_enabled);
        CHECK(!test_pdm.pending_irq);
        CHECK(test_power_on_calls == 1U);
        CHECK(test_power_off_calls == 1U);
        CHECK(test_critical_depth == 0U);

        /* Clearing only the injected error is enough for the next session;
         * no stale capture owner or PDM state may block a fresh Start. */
        test_pdm.init_error = NRFX_SUCCESS;
        test_pdm.start_error = NRFX_SUCCESS;
        storage.active = false;
        result = bc_recording_start(&recording, &fresh, 310U);
        CHECK(result == BC_REC_OK);
        CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
        CHECK(test_pdm.pending_irq);
        CHECK(test_pdm_deliver_pending_request());
        CHECK(test_pdm_emit_started());
        CHECK(stop_and_drain(&storage, &recording, fresh.id, 320U));
        CHECK(storage.finish_calls == 2U);
        CHECK(storage.last_finish_complete);
    }
    return true;
}

static bool test_abort_clears_pending_irq(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_port port;
    bc_rec_config config = capture_config();
    bc_rec_start start = capture_start(0x707U);
    bc_rec_result result;

    storage_reset(&storage);
    test_pdm_reset();
    test_codec_reset();
    test_power_reset();
    port = storage_port(&storage);
    REQUIRE(bc_recording_init(&recording, &port, &config));
    REQUIRE(bc_recording_start(&recording, &start, 400U) == BC_REC_OK);
    CHECK(test_pdm.pending_irq);
    CHECK(test_pdm.buffer_set_calls == 0U);
    CHECK(test_codec.encode_calls == 0U);
    CHECK(app_sudo_capture_abort(NULL, start.id));
    CHECK(test_pdm_enable_register == 0U);
    CHECK(!test_pdm.pending_irq);
    CHECK(!test_pdm.irq_enabled);
    CHECK(test_pdm.buffer_set_calls == 0U);
    CHECK(test_codec.encode_calls == 0U);
    CHECK(!test_pdm_deliver_pending_request());
    result = bc_recording_fault(&recording, start.id, BC_REC_CAPTURE_ERROR,
                                401U);
    CHECK(result == BC_REC_CAPTURE_ERROR);
    CHECK(bc_recording_drained(&recording, start.id) == BC_REC_CAPTURE_ERROR);
    CHECK(storage.finish_calls == 1U);
    CHECK(!storage.last_finish_complete);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    return true;
}

static bool test_ptt_arm_guards_and_normal_stop(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_start start = capture_start(0x808U);

    REQUIRE(begin_recording(&storage, &recording, start.id));

    /* The lease bounds and session identity are guards, not defaults. An
     * invalid arm must leave the capture unguarded. The full-buffer IRQ is
     * deliberately delivered before the worker polls. */
    test_ticks = 0U;
    app_sudo_capture_ptt_arm(start.id, 99U, 0U);
    app_sudo_capture_ptt_arm(start.id, 751U, 0U);
    app_sudo_capture_ptt_arm(start.id + 1U, 100U, 0U);
    test_ticks = 100000U;
    fill_current(1100);
    REQUIRE(test_pdm_emit_full());
    CHECK(test_pdm.stop_calls == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_OK);

    /* A valid arm accepts the inclusive lower bound. Release requests a
     * normal boundary stop; the owner must enter STOPPING only when polled. */
    test_ticks = 0U;
    app_sudo_capture_ptt_arm(start.id, 100U, 0U);
    test_ticks = 10U;
    app_sudo_capture_ptt_touch(start.id, true, false);
    CHECK(test_pdm.stop_calls == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_OK);
    fill_current(1200);
    REQUIRE(test_pdm_emit_full());
    CHECK(test_pdm.stop_calls == 1U);
    CHECK(test_pdm.state == TEST_PDM_STOPPING);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    CHECK(app_sudo_capture_poll(&recording, 11U));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_OK);
    CHECK(storage.finish_calls == 0U);
    REQUIRE(test_pdm_emit_stopped());
    CHECK(app_sudo_capture_poll(&recording, 12U));
    CHECK(!app_sudo_capture_poll(&recording, 13U));
    CHECK(!bc_recording_active(&recording));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);
    CHECK(storage.finish_calls == 1U);
    CHECK(storage.frame_count == 2U);
    return true;
}

static bool test_ptt_limit_runs_without_owner_poll(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_start start = capture_start(0x909U);

    REQUIRE(begin_recording(&storage, &recording, start.id));
    app_sudo_capture_ptt_arm(start.id, 750U, 10000U);

    /* Renew the short lease from the sensor side while the recording worker
     * remains completely idle. The ten-second limit is checked by the next
     * full DMA boundary in the ISR path. */
    {
        unsigned tick;
        for (tick = 700U; tick < 9500U; tick += 700U)
        {
            test_ticks = tick;
            app_sudo_capture_ptt_touch(start.id, true, true);
        }
    }
    test_ticks = 9500U;
    app_sudo_capture_ptt_touch(start.id, true, true);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_OK);
    test_ticks = 10240U;
    fill_current(1300);
    REQUIRE(test_pdm_emit_full());
    CHECK(test_pdm.stop_calls == 1U);
    CHECK(test_pdm.state == TEST_PDM_STOPPING);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_OK);
    CHECK(storage.frame_count == 0U);

    CHECK(app_sudo_capture_poll(&recording, 1U));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_OK);
    CHECK(storage.frame_count == 1U);
    CHECK(storage.finish_calls == 0U);
    REQUIRE(test_pdm_emit_stopped());
    CHECK(!app_sudo_capture_poll(&recording, 2U));
    CHECK(!bc_recording_active(&recording));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);
    CHECK(storage.frame_count == 1U);
    CHECK(storage.finish_calls == 1U);
    return true;
}

static bool test_ptt_valid_renew_and_tick_wrap(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_start start = capture_start(0xa0aU);

    REQUIRE(begin_recording(&storage, &recording, start.id));
    test_ticks = UINT32_MAX - 50U;
    app_sudo_capture_ptt_arm(start.id, 100U, 0U);

    /* Unsigned tick subtraction must keep a lease alive across wrap. */
    test_ticks = 30U;
    app_sudo_capture_ptt_touch(start.id, true, true);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_OK);
    test_ticks = 130U;
    fill_current(1400);
    REQUIRE(test_pdm_emit_full());
    CHECK(test_pdm.stop_calls == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_OK);
    CHECK(app_sudo_capture_poll(&recording, 1U));
    CHECK(storage.frame_count == 1U);

    /* Once the renewed lease is actually late, the full boundary becomes a
     * partial capture. A late callback cannot revive it. */
    test_ticks = 250U;
    fill_current(1500);
    REQUIRE(test_pdm_emit_full());
    CHECK(test_pdm.stop_calls == 1U);
    CHECK(test_pdm.state == TEST_PDM_STOPPING);
    CHECK(app_sudo_capture_poll(&recording, 2U));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_TOUCH_ERROR);
    REQUIRE(test_pdm_emit_stopped());
    CHECK(!app_sudo_capture_poll(&recording, 3U));
    CHECK(!bc_recording_active(&recording));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(!storage.last_finish_complete);
    CHECK(storage.frame_count == 2U);
    return true;
}

static bool test_ptt_late_and_invalid_reads_do_not_renew(void)
{
    unsigned mode;

    for (mode = 0U; mode < 2U; ++mode)
    {
        storage_fixture storage;
        bc_recording recording;
        bc_rec_start start = capture_start(0xb0bU + mode);

        REQUIRE(begin_recording(&storage, &recording, start.id));
        app_sudo_capture_ptt_arm(start.id, 100U, 0U);
        if (mode == 0U) {
            /* Exactly one lease is late; contact=true must not renew it. */
            test_ticks = 103U;
            app_sudo_capture_ptt_touch(start.id, true, true);
        } else {
            /* An invalid sensor reading is a fault, never a lease renewal. */
            test_ticks = 40U;
            app_sudo_capture_ptt_touch(start.id, false, true);
        }
        CHECK(test_pdm.stop_calls == 0U);
        CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
        CHECK(bc_recording_snapshot(&recording)->error == BC_REC_OK);

        test_ticks = 104U;
        fill_current((int16_t)(1600 + mode * 100));
        REQUIRE(test_pdm_emit_full());
        CHECK(test_pdm.stop_calls == 1U);
        CHECK(test_pdm.state == TEST_PDM_STOPPING);
        CHECK(app_sudo_capture_poll(&recording, 10U + mode));
        CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
        CHECK(bc_recording_snapshot(&recording)->error == BC_REC_TOUCH_ERROR);
        REQUIRE(test_pdm_emit_stopped());
        CHECK(!app_sudo_capture_poll(&recording, 20U + mode));
        CHECK(!bc_recording_active(&recording));
        CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
        CHECK(!storage.last_finish_complete);
    }
    return true;
}

static bool test_stale_ptt_id_and_memo_are_unguarded(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_start old_start = capture_start(0xc0cU);
    bc_rec_start fresh_start = capture_start(0xd0dU);
    bc_rec_start memo_start = capture_start_with_trigger(0xe0eU, BC_REC_MEMO);

    REQUIRE(begin_recording_with_start(&storage, &recording, &old_start));
    CHECK(stop_and_drain(&storage, &recording, old_start.id, 1U));
    REQUIRE(begin_recording_with_start(&storage, &recording, &fresh_start));
    app_sudo_capture_ptt_arm(fresh_start.id, 100U, 0U);
    app_sudo_capture_ptt_touch(old_start.id, false, false);
    CHECK(app_sudo_capture_stop(NULL, old_start.id) == BC_REC_WRONG_SESSION);
    CHECK(test_pdm.stop_calls == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_OK);
    test_ticks = 50U;
    fill_current(1700);
    REQUIRE(test_pdm_emit_full());
    CHECK(test_pdm.stop_calls == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    CHECK(stop_and_drain(&storage, &recording, fresh_start.id, 2U));

    /* Memo starts do not arm the physical PTT lease. A long idle interval and
     * an unsolicited touch report must not stop a memo capture. */
    REQUIRE(begin_recording_with_start(&storage, &recording, &memo_start));
    test_ticks = 100000U;
    app_sudo_capture_ptt_touch(memo_start.id, false, false);
    fill_current(1800);
    REQUIRE(test_pdm_emit_full());
    CHECK(test_pdm.stop_calls == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_OK);
    CHECK(stop_and_drain(&storage, &recording, memo_start.id, 3U));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);
    return true;
}

static bool test_stall_without_initial_irq_and_restart(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_start stalled = capture_start(0xf0fU);
    bc_rec_start fresh = capture_start(0xf1fU);

    REQUIRE(start_recording_without_irq(&storage, &recording, &stalled, 0U));
    CHECK(test_pdm.initialized);
    CHECK(test_pdm.pending_irq);
    CHECK(test_pdm.buffer_set_calls == 0U);
    test_ticks = 512U;
    CHECK(!app_sudo_capture_poll(&recording, 512U));
    CHECK(!test_pdm.initialized);
    CHECK(!test_pdm.irq_enabled);
    CHECK(!test_pdm.pending_irq);
    CHECK(test_pdm_enable_register == 0U);
    CHECK(test_power_off_calls == 1U);
    CHECK(!bc_recording_active(&recording));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_CAPTURE_ERROR);
    CHECK(storage.finish_calls == 1U);
    CHECK(!storage.last_finish_complete);

    /* A stalled session releases every hardware-owned slot before a fresh
     * Start is accepted. */
    REQUIRE(begin_recording(&storage, &recording, fresh.id));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    CHECK(stop_and_drain(&storage, &recording, fresh.id, 520U));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);
    return true;
}

static bool test_stall_mid_memo_preserves_ready_tail(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_start start = capture_start_with_trigger(0xf2fU, BC_REC_MEMO);

    REQUIRE(begin_recording_with_start(&storage, &recording, &start));
    fill_current(1900);
    REQUIRE(test_pdm_emit_full());
    CHECK(storage.frame_count == 0U);
    CHECK(test_pdm.state == TEST_PDM_RUNNING);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);

    /* No PDM event follows this READY block. The worker discovers the 500 ms
     * stall, aborts hardware, and must still encode the completed tail. */
    test_ticks = 512U;
    CHECK(app_sudo_capture_poll(&recording, 600U));
    CHECK(!test_pdm.initialized);
    CHECK(!test_pdm.irq_enabled);
    CHECK(!test_pdm.pending_irq);
    CHECK(storage.frame_count == 1U);
    CHECK(storage.finish_calls == 1U);
    CHECK(!storage.last_finish_complete);
    CHECK(!bc_recording_active(&recording));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_CAPTURE_ERROR);
    return true;
}

static bool test_stall_tick_wrap(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_start start = capture_start_with_trigger(0xf3fU, BC_REC_MEMO);

    REQUIRE(start_recording_without_irq(&storage, &recording, &start,
                                        UINT32_MAX - 100U));
    REQUIRE(test_pdm_deliver_pending_request());
    REQUIRE(test_pdm_emit_started());

    /* 301 ticks across uint32 wrap is below the 512-tick stall threshold. */
    test_ticks = 200U;
    CHECK(!app_sudo_capture_poll(&recording, 1U));
    CHECK(test_pdm.initialized);
    CHECK(bc_recording_active(&recording));
    CHECK(storage.finish_calls == 0U);

    /* The wrapped elapsed interval is now 601 ticks, so the same poll must
     * quiesce the IRQ and finalize the owner as a partial capture. */
    test_ticks = 500U;
    CHECK(!app_sudo_capture_poll(&recording, 2U));
    CHECK(!test_pdm.initialized);
    CHECK(!test_pdm.irq_enabled);
    CHECK(!bc_recording_active(&recording));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_CAPTURE_ERROR);
    CHECK(storage.finish_calls == 1U);
    return true;
}

int main(void)
{
    app_sudo_capture_init((TaskHandle_t)(uintptr_t)1U);
    (void)test_initial_request_and_encoding_contract();
    (void)test_stop_boundary_and_stopped_tail();
    (void)test_overflow_retains_ready_tail_as_partial();
    (void)test_abort_timeout_retains_ready_tail();
    (void)test_start_failures_are_quiescent_and_restartable();
    (void)test_abort_clears_pending_irq();
    (void)test_ptt_arm_guards_and_normal_stop();
    (void)test_ptt_limit_runs_without_owner_poll();
    (void)test_ptt_valid_renew_and_tick_wrap();
    (void)test_ptt_late_and_invalid_reads_do_not_renew();
    (void)test_stale_ptt_id_and_memo_are_unguarded();
    (void)test_stall_without_initial_irq_and_restart();
    (void)test_stall_mid_memo_preserves_ready_tail();
    (void)test_stall_tick_wrap();

    if (test_critical_depth != 0U)
    {
        ++failures;
        fprintf(stderr, "FAIL: critical depth leaked (%u)\n",
                test_critical_depth);
    }
    if (failures != 0U)
    {
        fprintf(stderr, "FAIL: %u of %u checks\n", failures, checks);
        return 1;
    }
    printf("PASS: %u checks (real capture adapter, PDM boundaries, DMA ownership,\n"
           "      continuous ADPCM, partial recovery and cleanup)\n", checks);
    return 0;
}
