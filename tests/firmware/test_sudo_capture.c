#include "app_sudo_capture.h"
#include "bc_capture.h"
#include "bc_opus_stream.h"
#include "bc_opus_profile.h"
#include "bc_resampler.h"
#include "nrfx_pdm.h"
#include "nrf.h"

#include "opus.h"
#include "custom_support.h"

#include <math.h>
#include <stdlib.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ARRAY_LEN(value) (sizeof(value) / sizeof((value)[0]))
#define TEST_MAX_CODEC_CALLS 64U
#define TEST_STORAGE_CAPACITY (BC_REC_FRAME_MAX * 256U)
#define TEST_MAX_BLOCKS 64U

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


/* ---- RTOS heap and DWT shims ---- */
unsigned test_heap_allocations;
size_t test_heap_bytes;
unsigned test_heap_fail_remaining;

void *pvPortMalloc(size_t size)
{
    if (test_heap_fail_remaining != 0U) {
        --test_heap_fail_remaining;
        return NULL;
    }
    ++test_heap_allocations;
    test_heap_bytes += size;
    return malloc(size);
}

test_dwt_t test_dwt;
test_coredebug_t test_coredebug;
uint32_t SystemCoreClock = 64000000U;
uint32_t test_cycles_per_read = 32000U;

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
    uint16_t append_lengths[256];
    unsigned open_calls;
    unsigned append_calls;
    unsigned checkpoint_calls;
    unsigned finish_calls;
    unsigned changed_calls;
    bool active;
    bool last_finish_complete;
    bc_rec_file last_checkpoint;
    bc_rec_file last_finish;
    bc_rec_file opened;
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
    /* Like the real store, label the file with the prepared descriptor;
     * an unprepared encoder leaves it unlabeled and the owner rejects it. */
    (void)app_sudo_capture_format(&file->audio);
    storage->opened = *file;
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
    if (data == NULL || length == 0U || length > sizeof(storage->bytes) - storage->byte_count)
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

/* Every PDM block handed to the adapter is remembered so the stored Opus
 * stream can be checked against the exact input the microphone produced. */
static int16_t test_blocks[TEST_MAX_BLOCKS][BC_CAPTURE_SAMPLES];
static unsigned test_block_count;
static double test_signal_hz = 440.0;
static unsigned test_signal_position;

static void test_input_reset(void)
{
    test_block_count = 0U;
    test_signal_position = 0U;
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
    test_power_reset();
    test_input_reset();
    test_ticks = 0U;
    REQUIRE(bc_recording_init(recording, &port, &config));
    REQUIRE(bc_recording_start(recording, start, 0U) == BC_REC_OK);
    REQUIRE(bc_recording_snapshot(recording)->phase == BC_REC_RECORDING);
    REQUIRE(bc_audio_format_equal(&storage->opened.audio, &bc_recording_snapshot(recording)->file.audio));
    REQUIRE(test_pdm.pending_irq);
    REQUIRE(test_pdm_deliver_pending_request());
    REQUIRE(test_pdm.buffer_set_calls == 1U);
    REQUIRE(test_pdm_emit_started());
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
    test_power_reset();
    test_input_reset();
    test_ticks = initial_ticks;
    REQUIRE(bc_recording_init(recording, &port, &config));
    REQUIRE(bc_recording_start(recording, start, 0U) == BC_REC_OK);
    REQUIRE(bc_recording_snapshot(recording)->phase == BC_REC_RECORDING);
    REQUIRE(test_pdm.pending_irq);
    REQUIRE(test_pdm.buffer_set_calls == 0U);
    return true;
}

/* Fills the active DMA buffer with the next block of a continuous sine at
 * the nominal 16.125 kHz microphone rate and remembers it. */
static void fill_current(int16_t base)
{
    int16_t *buffer = test_pdm_active_buffer();
    unsigned i;
    (void)base;
    CHECK(buffer != NULL);
    if (buffer == NULL)
        return;
    for (i = 0U; i < BC_CAPTURE_SAMPLES; ++i) {
        double t = (double)(test_signal_position + i) / BC_OPUS_PCM_NOMINAL_HZ;
        buffer[i] = (int16_t)lrint(12000.0 * sin(2.0 * M_PI * test_signal_hz * t));
    }
    test_signal_position += BC_CAPTURE_SAMPLES;
    if (test_block_count < TEST_MAX_BLOCKS)
        memcpy(test_blocks[test_block_count], buffer, sizeof(test_blocks[0]));
    ++test_block_count;
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

/* Parses the stored container, decodes every packet with the upstream
 * decoder and reports counts. Returns false on any container error. */
typedef struct {
    bc_audio_format format;
    uint32_t packets;
    uint32_t decoded_samples;
    uint32_t trailer_samples;
    bool ended;
    bool header;
    int16_t pcm[BC_OPUS_FRAME_SAMPLES_MAX * 512];
    size_t pcm_count;
} stored_stream;

static bool inspect_stored(const storage_fixture *storage, stored_stream *out)
{
    bc_opus_stream_parser parser;
    OpusDecoder *decoder = NULL;
    size_t pos = 0U, used;
    bool ok = true;
    memset(out, 0, sizeof(*out));
    bc_opus_stream_parser_init(&parser);
    while (pos < storage->byte_count) {
        bc_opus_parse_event e = bc_opus_stream_parse(&parser, storage->bytes + pos,
                                                     storage->byte_count - pos, &used);
        pos += used;
        if (e == BC_OPUS_PARSE_HEADER) {
            /* The adapter's allocation port refuses codec objects, so the
             * reference decoder lives in test-owned memory. */
            out->header = true;
            out->format = parser.format;
            decoder = malloc((size_t)opus_decoder_get_size(1));
            if (decoder == NULL ||
                opus_decoder_init(decoder, (opus_int32)parser.format.sample_rate_hz, 1) != OPUS_OK) {
                ok = false; break;
            }
        } else if (e == BC_OPUS_PARSE_PACKET) {
            size_t n; const uint8_t *packet = bc_opus_stream_parser_packet(&parser, &n);
            int samples;
            if (decoder == NULL) { ok = false; break; }
            samples = opus_decode(decoder, packet, (opus_int32)n, out->pcm + out->pcm_count,
                                  (int)(ARRAY_LEN(out->pcm) - out->pcm_count), 0);
            if (samples != (int)parser.format.block_samples) { ok = false; break; }
            out->pcm_count += (size_t)samples;
            ++out->packets;
            out->decoded_samples += (uint32_t)samples;
        } else if (e == BC_OPUS_PARSE_TRAILER) {
            out->ended = true;
            out->trailer_samples = parser.sample_count;
        } else if (e != BC_OPUS_PARSE_NEED_MORE) {
            ok = false; break;
        }
    }
    free(decoder);
    return ok;
}

/* Every stored frame is at most one chunk and the stream must reflect the
 * exact resampled input: trailer sample count within the resampler window,
 * decoded audio correlated with the microphone sine after pre-skip. */
static bool check_stored_audio(const storage_fixture *storage, bool expect_trailer)
{
    static stored_stream stream;
    bc_resampler reference;
    static int16_t expected[TEST_MAX_BLOCKS * BC_CAPTURE_SAMPLES];
    size_t expected_count = 0U, consumed, i, compare;
    double sa = 0.0, sb = 0.0, sab = 0.0;
    unsigned f;
    for (f = 0U; f < storage->frame_count && f < ARRAY_LEN(storage->append_lengths); ++f)
        CHECK(storage->append_lengths[f] >= 1U && storage->append_lengths[f] <= BC_REC_FRAME_MAX);
    REQUIRE(inspect_stored(storage, &stream));
    REQUIRE(stream.header);
    CHECK(stream.format.codec == BC_AUDIO_CODEC_OPUS);
    CHECK(stream.format.sample_rate_hz == BC_OPUS_SAMPLE_RATE_HZ);
    CHECK(stream.format.block_samples == BC_OPUS_FRAME_SAMPLES);
    CHECK(stream.ended == expect_trailer);
    REQUIRE(bc_resampler_init(&reference, BC_OPUS_PCM_NOMINAL_HZ, BC_OPUS_SAMPLE_RATE_HZ));
    for (i = 0U; i < test_block_count && i < TEST_MAX_BLOCKS; ++i)
        expected_count += bc_resampler_process(&reference, test_blocks[i], BC_CAPTURE_SAMPLES,
                                               expected + expected_count,
                                               ARRAY_LEN(expected) - expected_count, &consumed);
    if (expect_trailer) {
        CHECK(stream.trailer_samples == expected_count);
        CHECK(stream.decoded_samples >= expected_count + stream.format.pre_skip);
        CHECK(stream.decoded_samples < expected_count + stream.format.pre_skip + 2U * BC_OPUS_FRAME_SAMPLES);
    }
    compare = stream.decoded_samples > stream.format.pre_skip ? stream.decoded_samples - stream.format.pre_skip : 0U;
    if (compare > expected_count) compare = expected_count;
    for (i = 0U; i < compare; ++i) {
        double a = expected[i], b = stream.pcm[i + stream.format.pre_skip];
        sa += a * a; sb += b * b; sab += a * b;
    }
    if (compare >= BC_OPUS_FRAME_SAMPLES * 2U) {
        CHECK(sa > 0.0 && sb > 0.0);
        if (sa > 0.0 && sb > 0.0) CHECK(sab / sqrt(sa * sb) > 0.85);
    }
    return true;
}

static bool test_unprepared_encoder_is_explicit(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_port port;
    bc_rec_config config = capture_config();
    bc_rec_start start = capture_start(0x001U);
    bc_audio_format format;
    bc_voice_audio_stats stats;

    /* The first heap request fails: the encoder cannot be prepared, so no
     * descriptor exists and Start reports UNSUPPORTED without touching the
     * microphone. Nothing is labeled Opus. */
    test_heap_fail_remaining = 1U;
    CHECK(!app_sudo_capture_prepare());
    CHECK(!app_sudo_capture_format(&format));
    CHECK(!app_sudo_capture_audio_stats(NULL, &format, &stats));
    storage_reset(&storage);
    test_pdm_reset();
    test_power_reset();
    port = storage_port(&storage);
    REQUIRE(bc_recording_init(&recording, &port, &config));
    CHECK(bc_recording_start(&recording, &start, 0U) == BC_REC_OPEN_ERROR);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_FAILED);
    CHECK(test_power_on_calls == 0U && test_pdm.init_calls == 0U);
    CHECK(test_heap_fail_remaining == 0U);
    storage.active = false;
    /* Recovered allocation: the profile prepares, self-tests one frame and
     * reports its exact allocations. */
    REQUIRE(app_sudo_capture_prepare());
    CHECK(app_sudo_capture_prepare());
    REQUIRE(app_sudo_capture_format(&format));
    CHECK(format.codec == BC_AUDIO_CODEC_OPUS && format.sample_rate_hz == 16000U &&
          format.channels == 1U && format.frame_ms == 20U && format.pre_skip == 104U &&
          format.block_samples == 320U && format.block_bytes == 0U && format.sample_count == 0U);
    REQUIRE(app_sudo_capture_audio_stats(NULL, &format, &stats));
    CHECK(stats.state_bytes == (uint32_t)opus_encoder_get_size(1));
    CHECK(stats.scratch_bytes == BC_OPUS_SCRATCH_BYTES);
    CHECK(stats.scratch_high_water > 0U && stats.scratch_high_water <= BC_OPUS_SCRATCH_BYTES);
    CHECK(stats.frames == 0U && stats.faults == 0U);
    CHECK(test_heap_allocations == 2U);
    CHECK(test_heap_bytes == (size_t)opus_encoder_get_size(1) + BC_OPUS_SCRATCH_BYTES + BC_OPUS_SCRATCH_CANARY_BYTES);
    CHECK(test_coredebug.DEMCR & CoreDebug_DEMCR_TRCENA_Msk);
    CHECK(test_dwt.CTRL & DWT_CTRL_CYCCNTENA_Msk);
    return true;
}

static bool test_initial_request_and_encoding_contract(void)
{
    storage_fixture storage;
    bc_recording recording;
    int16_t *first;
    int16_t *replacement;
    bc_rec_start start = capture_start(0x101U);
    unsigned sets;
    bc_audio_format format;
    bc_voice_audio_stats stats;

    REQUIRE(begin_recording(&storage, &recording, start.id));
    first = test_pdm_active_buffer();
    REQUIRE(first != NULL);
    fill_current(1000);
    REQUIRE(test_pdm_emit_full());
    CHECK(storage.byte_count == 0U);
    CHECK(test_pdm.buffer_set_calls == 3U);
    replacement = test_pdm.buffer_history[2];
    CHECK(replacement != NULL && replacement != first);

    /* The first released slot is still READY. A second completion must choose
     * another capture slot, so the DMA pointer cannot be reused early. */
    fill_current(2000);
    REQUIRE(test_pdm_emit_full());
    CHECK(storage.frame_count == 0U);
    CHECK(test_pdm.buffer_set_calls == 4U);
    CHECK(test_pdm.buffer_history[3] != first);

    /* One block is 880 nominal samples: about 873 at 16 kHz, so two full
     * frames are encoded and the third accumulates. The container waits for
     * a full chunk before delivering anything to the owner. */
    CHECK(app_sudo_capture_poll(&recording, 10U));
    REQUIRE(app_sudo_capture_audio_stats(NULL, &format, &stats));
    CHECK(stats.frames == 2U);
    CHECK(stats.max_encode_us > 0U && stats.mean_encode_us > 0U);
    CHECK(storage.frame_count == 0U);

    /* Once the encoder releases the first slot, the next completed boundary
     * is allowed to reuse precisely that DMA buffer. */
    fill_current(3000);
    REQUIRE(test_pdm_emit_full());
    sets = test_pdm.buffer_set_calls;
    CHECK(sets == 5U);
    CHECK(test_pdm.buffer_history[4] == first);
    CHECK(app_sudo_capture_poll(&recording, 20U));
    REQUIRE(app_sudo_capture_audio_stats(NULL, &format, &stats));
    CHECK(stats.frames == 5U);
    /* 16-byte header plus six 32-byte records exceed a chunk only at the
     * seventh packet; five packets are still buffered. */
    CHECK(storage.frame_count == 0U);
    CHECK(app_sudo_capture_poll(&recording, 21U));
    REQUIRE(app_sudo_capture_audio_stats(NULL, &format, &stats));
    CHECK(stats.frames == 8U);
    CHECK(storage.frame_count == 1U);
    CHECK(storage.append_lengths[0] == 16U + 6U * 32U);

    CHECK(stop_and_drain(&storage, &recording, start.id, 30U));
    CHECK(storage.finish_calls == 1U);
    CHECK(storage.last_finish_complete);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);
    CHECK(check_stored_audio(&storage, true));
    CHECK(app_sudo_capture_sample_count(start.id) != 0U);
    CHECK(app_sudo_capture_sample_count(start.id + 1U) == 0U);
    CHECK(bc_recording_snapshot(&recording)->file.frames == storage.frame_count);
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
     * buffer_requested=false means it cannot become audio. Both completed
     * blocks are encoded, the encoder delay is flushed and the exact sample
     * count is written before finalization. */
    REQUIRE(test_pdm_emit_stopped());
    CHECK(test_pdm_enable_register == 0U);
    CHECK(app_sudo_capture_poll(&recording, 60U));
    CHECK(app_sudo_capture_poll(&recording, 61U));
    CHECK(!bc_recording_active(&recording));
    CHECK(storage.finish_calls == 1U);
    CHECK(storage.last_finish_complete);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);
    CHECK(check_stored_audio(&storage, true));
    CHECK(test_power_off_calls == 1U);
    return true;
}

static bool test_empty_capture_stays_empty(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_start start = capture_start(0x212U);

    REQUIRE(begin_recording(&storage, &recording, start.id));
    CHECK(bc_recording_stop(&recording, start.id, 5U) == BC_REC_OK);
    /* Stop before any completed block: the header is discarded and the
     * owner reports EMPTY rather than a header-only Opus file. */
    REQUIRE(test_pdm_emit_full());
    REQUIRE(test_pdm_emit_stopped());
    while (bc_recording_active(&recording)) (void)app_sudo_capture_poll(&recording, 6U);
    /* The block completed at the stop boundary still carries audio; only a
     * capture with no completed block at all is empty. */
    CHECK(storage.byte_count != 0U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);

    REQUIRE(begin_recording(&storage, &recording, start.id + 1U));
    CHECK(bc_recording_stop(&recording, start.id + 1U, 7U) == BC_REC_OK);
    CHECK(app_sudo_capture_abort(NULL, start.id + 1U));
    CHECK(bc_recording_drained(&recording, start.id + 1U) == BC_REC_EMPTY_AUDIO);
    CHECK(storage.byte_count == 0U && storage.frame_count == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_EMPTY);
    CHECK(app_sudo_capture_sample_count(start.id + 1U) == 0U);
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
    CHECK(storage.frame_count == 0U);
    REQUIRE(test_pdm_emit_overflow());
    CHECK(test_pdm.stop_calls == 1U);
    CHECK(test_pdm.state == TEST_PDM_STOPPING);
    REQUIRE(test_pdm_emit_stopped());

    CHECK(app_sudo_capture_poll(&recording, 100U));
    CHECK(storage.finish_calls == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_CAPTURE_OVERFLOW);

    /* The second READY block is still encoded and the tail flushed, but the
     * recording stays PARTIAL because audio after the overflow was lost. */
    CHECK(app_sudo_capture_poll(&recording, 101U));
    CHECK(storage.finish_calls == 1U);
    CHECK(!storage.last_finish_complete);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(!bc_recording_snapshot(&recording)->file.complete);
    CHECK(check_stored_audio(&storage, true));
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
    CHECK(storage.byte_count == 0U);
    CHECK(storage.finish_calls == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_STOP_TIMEOUT);

    CHECK(app_sudo_capture_poll(&recording, 227U));
    CHECK(storage.frame_count >= 1U);
    CHECK(storage.finish_calls == 1U);
    CHECK(!storage.last_finish_complete);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(!bc_recording_snapshot(&recording)->file.complete);
    CHECK(check_stored_audio(&storage, true));
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
        test_power_reset();
        test_input_reset();
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
         * no stale capture owner, codec or PDM state may block a fresh Start. */
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
        CHECK(check_stored_audio(&storage, true));
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
    test_power_reset();
    test_input_reset();
    port = storage_port(&storage);
    REQUIRE(bc_recording_init(&recording, &port, &config));
    REQUIRE(bc_recording_start(&recording, &start, 400U) == BC_REC_OK);
    CHECK(test_pdm.pending_irq);
    CHECK(test_pdm.buffer_set_calls == 0U);
    CHECK(app_sudo_capture_abort(NULL, start.id));
    CHECK(test_pdm_enable_register == 0U);
    CHECK(!test_pdm.pending_irq);
    CHECK(!test_pdm.irq_enabled);
    CHECK(test_pdm.buffer_set_calls == 0U);
    CHECK(!test_pdm_deliver_pending_request());
    result = bc_recording_fault(&recording, start.id, BC_REC_CAPTURE_ERROR,
                                401U);
    CHECK(result == BC_REC_CAPTURE_ERROR);
    CHECK(bc_recording_drained(&recording, start.id) == BC_REC_CAPTURE_ERROR);
    CHECK(storage.finish_calls == 1U);
    CHECK(!storage.last_finish_complete);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(storage.byte_count == 0U);
    return true;
}

static bool test_encoder_fault_is_explicit_and_keeps_prefix(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_start start = capture_start(0x717U);
    uint8_t *scratch;
    bc_audio_format format;
    bc_voice_audio_stats stats;
    unsigned i;

    REQUIRE(begin_recording(&storage, &recording, start.id));
    for (i = 0U; i < 4U; ++i) {
        fill_current(0);
        REQUIRE(test_pdm_emit_full());
        CHECK(app_sudo_capture_poll(&recording, 10U + i));
    }
    CHECK(storage.frame_count >= 1U);
    /* Corrupt the pseudostack canary: the next encode must report an
     * explicit ENCODER_ERROR, keep the delivered prefix and never label the
     * remaining capture as good audio. */
    scratch = bc_opus_port_alloc_scratch(GLOBAL_STACK_SIZE);
    REQUIRE(scratch != NULL);
    scratch[GLOBAL_STACK_SIZE] ^= 0xffU;
    fill_current(0);
    REQUIRE(test_pdm_emit_full());
    CHECK(app_sudo_capture_poll(&recording, 20U));
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_ENCODER_ERROR);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    REQUIRE(app_sudo_capture_audio_stats(NULL, &format, &stats));
    CHECK(stats.faults == 1U);
    REQUIRE(test_pdm_emit_full());
    REQUIRE(test_pdm_emit_stopped());
    while (bc_recording_active(&recording)) (void)app_sudo_capture_poll(&recording, 21U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(!storage.last_finish_complete);
    /* The prefix is a parseable container without a trailer. */
    {
        static stored_stream stream;
        CHECK(inspect_stored(&storage, &stream));
        CHECK(stream.header && !stream.ended && stream.packets >= 6U);
    }
    CHECK(app_sudo_capture_sample_count(start.id) == 0U);
    scratch[GLOBAL_STACK_SIZE] ^= 0xffU;

    /* The fault is sticky only for that recording; a fresh Start re-arms
     * the encoder and records normally. */
    REQUIRE(begin_recording(&storage, &recording, start.id + 1U));
    fill_current(0);
    REQUIRE(test_pdm_emit_full());
    CHECK(stop_and_drain(&storage, &recording, start.id + 1U, 30U));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);
    CHECK(check_stored_audio(&storage, true));
    REQUIRE(app_sudo_capture_audio_stats(NULL, &format, &stats));
    CHECK(stats.faults == 1U);
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
    CHECK(check_stored_audio(&storage, true));
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
    CHECK(storage.finish_calls == 0U);
    REQUIRE(test_pdm_emit_stopped());
    CHECK(!app_sudo_capture_poll(&recording, 2U));
    CHECK(!bc_recording_active(&recording));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);
    CHECK(storage.frame_count >= 1U);
    CHECK(storage.finish_calls == 1U);
    CHECK(check_stored_audio(&storage, true));
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
    CHECK(check_stored_audio(&storage, true));
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
    CHECK(check_stored_audio(&storage, true));
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
    CHECK(storage.byte_count == 0U);

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
    CHECK(storage.frame_count >= 1U);
    CHECK(storage.finish_calls == 1U);
    CHECK(!storage.last_finish_complete);
    CHECK(!bc_recording_active(&recording));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_CAPTURE_ERROR);
    CHECK(check_stored_audio(&storage, true));
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

static bool test_long_recording_keeps_exact_counts(void)
{
    storage_fixture storage;
    bc_recording recording;
    bc_rec_start start = capture_start(0xf4fU);
    unsigned i;
    bc_audio_format format;
    bc_voice_audio_stats stats;

    /* Forty blocks (about 2.2 s) deliver many chunks; the trailer must still
     * carry the exact resampled count and every chunk stays bounded. */
    REQUIRE(begin_recording(&storage, &recording, start.id));
    for (i = 0U; i < 40U; ++i) {
        fill_current(0);
        REQUIRE(test_pdm_emit_full());
        (void)app_sudo_capture_poll(&recording, 100U + i);
    }
    CHECK(stop_and_drain(&storage, &recording, start.id, 200U));
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);
    CHECK(storage.frame_count >= 15U);
    CHECK(check_stored_audio(&storage, true));
    REQUIRE(app_sudo_capture_audio_stats(NULL, &format, &stats));
    CHECK(stats.frames >= 110U);
    CHECK(stats.scratch_high_water > 8000U && stats.scratch_high_water <= BC_OPUS_SCRATCH_BYTES);
    printf("  capture stats: frames=%u max_encode_us=%u mean_encode_us=%u scratch_high_water=%u state=%u\n",
           stats.frames, stats.max_encode_us, stats.mean_encode_us, stats.scratch_high_water, stats.state_bytes);
    return true;
}

int main(void)
{
    app_sudo_capture_init((TaskHandle_t)(uintptr_t)1U);
    (void)test_unprepared_encoder_is_explicit();
    (void)test_initial_request_and_encoding_contract();
    (void)test_stop_boundary_and_stopped_tail();
    (void)test_empty_capture_stays_empty();
    (void)test_overflow_retains_ready_tail_as_partial();
    (void)test_abort_timeout_retains_ready_tail();
    (void)test_start_failures_are_quiescent_and_restartable();
    (void)test_abort_clears_pending_irq();
    (void)test_encoder_fault_is_explicit_and_keeps_prefix();
    (void)test_ptt_arm_guards_and_normal_stop();
    (void)test_ptt_limit_runs_without_owner_poll();
    (void)test_ptt_valid_renew_and_tick_wrap();
    (void)test_ptt_late_and_invalid_reads_do_not_renew();
    (void)test_stale_ptt_id_and_memo_are_unguarded();
    (void)test_stall_without_initial_irq_and_restart();
    (void)test_stall_mid_memo_preserves_ready_tail();
    (void)test_stall_tick_wrap();
    (void)test_long_recording_keeps_exact_counts();

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
           "      real Opus 1.6.1 encoding, exact sample counts, partial recovery and cleanup)\n", checks);
    return 0;
}
