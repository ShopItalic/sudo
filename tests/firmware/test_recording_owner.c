#include "bc_recording.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ARRAY_LEN(value) (sizeof(value) / sizeof((value)[0]))
#define MAX_CALLS 512U
#define MAX_FRAMES 64U
#define STORAGE_CAPACITY 8192U

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

#define CHECK(condition) check_condition((condition), #condition, __LINE__)

typedef enum {
    CALL_OPEN = 1,
    CALL_APPEND,
    CALL_CHECKPOINT,
    CALL_FINISH,
    CALL_CAPTURE_START,
    CALL_CAPTURE_STOP,
    CALL_CAPTURE_ABORT,
    CALL_LIVE,
    CALL_CHANGED
} call_kind;

typedef struct {
    uint64_t id;
    uint32_t sequence;
    uint16_t length;
    uint8_t bytes[BC_REC_FRAME_MAX];
} observed_live;

typedef struct {
    int calls[MAX_CALLS];
    unsigned call_count;

    unsigned open_calls;
    unsigned append_calls;
    unsigned checkpoint_calls;
    unsigned finish_calls;
    unsigned capture_start_calls;
    unsigned capture_stop_calls;
    unsigned capture_abort_calls;
    unsigned live_calls;
    unsigned changed_calls;

    bc_rec_start last_open_start;
    bc_rec_start fresh_start;
    bool fresh_opened;
    bool capture_running;

    uint8_t storage[STORAGE_CAPACITY];
    uint32_t storage_bytes;
    uint32_t storage_frames;
    uint16_t append_lengths[MAX_FRAMES];
    uint32_t append_offsets[MAX_FRAMES];

    observed_live lives[MAX_FRAMES];

    bc_rec_file last_checkpoint_file;
    bc_rec_file last_finish_file;
    bool last_finish_complete;
    bool finish_empty_metadata;

    bc_rec_snapshot last_changed;

    /* Port failures are injected for one numbered invocation. */
    bc_rec_result open_error;
    bc_rec_result append_error;
    unsigned append_error_call;
    bc_rec_result checkpoint_error;
    bc_rec_result finish_error;
    bc_rec_result capture_start_error;
    bc_rec_result capture_stop_error;
    bool capture_abort_result;
    bool live_result;

    bool existing;
    bc_rec_start existing_start;
    bc_rec_file existing_file;
    bool active_elsewhere;
} recording_fixture;

static uint32_t crc32_bytes(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xffffffffU;
    uint32_t i;

    for (i = 0; i < length; ++i)
    {
        uint32_t bit;
        crc ^= data[i];
        for (bit = 0; bit < 8U; ++bit)
            crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
    }
    return crc ^ 0xffffffffU;
}

static void log_call(recording_fixture *fixture, call_kind kind)
{
    if (fixture->call_count < ARRAY_LEN(fixture->calls))
        fixture->calls[fixture->call_count++] = (int)kind;
}

static bool starts_equal(const bc_rec_start *left, const bc_rec_start *right)
{
    return left->id == right->id && left->trigger == right->trigger &&
           left->duration_limit_ms == right->duration_limit_ms;
}

static void fixture_reset(recording_fixture *fixture)
{
    memset(fixture, 0, sizeof(*fixture));
    fixture->open_error = BC_REC_OK;
    fixture->append_error = BC_REC_OK;
    fixture->checkpoint_error = BC_REC_OK;
    fixture->finish_error = BC_REC_OK;
    fixture->capture_start_error = BC_REC_OK;
    fixture->capture_stop_error = BC_REC_OK;
    fixture->capture_abort_result = true;
    fixture->live_result = true;
}

static bc_rec_result fixture_open(void *context, const bc_rec_start *start,
                                  bc_rec_file *file)
{
    recording_fixture *fixture = context;

    log_call(fixture, CALL_OPEN);
    ++fixture->open_calls;
    fixture->last_open_start = *start;

    if (fixture->active_elsewhere)
        return BC_REC_BUSY;

    if (fixture->existing)
    {
        if (!starts_equal(start, &fixture->existing_start))
            return BC_REC_INVALID;
        *file = fixture->existing_file;
        /* Fixtures that predate descriptors model legacy supplier records. */
        if (file->audio.codec == BC_AUDIO_CODEC_NONE)
            bc_audio_format_legacy_adpcm(&file->audio);
        return BC_REC_ALREADY_EXISTS;
    }

    if (fixture->open_error != BC_REC_OK)
        return fixture->open_error;

    fixture->fresh_start = *start;
    fixture->fresh_opened = true;
    fixture->active_elsewhere = true;
    fixture->storage_bytes = 0;
    fixture->storage_frames = 0;
    memset(fixture->storage, 0, sizeof(fixture->storage));
    memset(file, 0, sizeof(*file));
    (void)snprintf(file->name, sizeof(file->name), "rec-%llx.raw",
                   (unsigned long long)start->id);
    bc_audio_format_legacy_adpcm(&file->audio);
    return BC_REC_OK;
}

static bc_rec_result fixture_append(void *context, const uint8_t *data,
                                    uint16_t length)
{
    recording_fixture *fixture = context;
    unsigned call_number;

    log_call(fixture, CALL_APPEND);
    ++fixture->append_calls;
    call_number = fixture->append_calls;
    if (fixture->append_error != BC_REC_OK &&
        call_number == fixture->append_error_call)
        return fixture->append_error;

    CHECK(data != NULL);
    CHECK(length <= STORAGE_CAPACITY - fixture->storage_bytes);
    if (data == NULL || length > STORAGE_CAPACITY - fixture->storage_bytes)
        return BC_REC_WRITE_ERROR;

    if (fixture->storage_frames < ARRAY_LEN(fixture->append_lengths))
    {
        fixture->append_lengths[fixture->storage_frames] = length;
        fixture->append_offsets[fixture->storage_frames] = fixture->storage_bytes;
    }
    memcpy(fixture->storage + fixture->storage_bytes, data, length);
    fixture->storage_bytes += length;
    ++fixture->storage_frames;
    return BC_REC_OK;
}

static bc_rec_result fixture_checkpoint(void *context, bc_rec_file *file)
{
    recording_fixture *fixture = context;

    log_call(fixture, CALL_CHECKPOINT);
    ++fixture->checkpoint_calls;
    fixture->last_checkpoint_file = *file;
    if (fixture->checkpoint_error != BC_REC_OK)
        return fixture->checkpoint_error;

    file->bytes = fixture->storage_bytes;
    file->frames = fixture->storage_frames;
    file->crc32 = crc32_bytes(fixture->storage, fixture->storage_bytes);
    file->complete = false;
    file->recovered = false;
    fixture->last_checkpoint_file = *file;
    return BC_REC_OK;
}

static bc_rec_result fixture_finish(void *context, bool complete,
                                    bc_rec_file *file)
{
    recording_fixture *fixture = context;

    log_call(fixture, CALL_FINISH);
    ++fixture->finish_calls;
    fixture->last_finish_complete = complete;
    fixture->last_finish_file = *file;

    file->bytes = fixture->storage_bytes;
    file->frames = fixture->storage_frames;
    file->crc32 = crc32_bytes(fixture->storage, fixture->storage_bytes);
    file->complete = complete;
    file->recovered = !complete;
    if (fixture->finish_empty_metadata) {
        file->bytes = 0U;
        file->frames = 0U;
        file->crc32 = 0U;
    }
    fixture->last_finish_file = *file;

    if (fixture->finish_error != BC_REC_OK)
        return fixture->finish_error;

    fixture->active_elsewhere = false;
    return BC_REC_OK;
}

static bc_rec_result fixture_capture_start(void *context, uint64_t id)
{
    recording_fixture *fixture = context;

    log_call(fixture, CALL_CAPTURE_START);
    ++fixture->capture_start_calls;
    (void)id;
    if (fixture->capture_start_error != BC_REC_OK)
        return fixture->capture_start_error;
    fixture->capture_running = true;
    return BC_REC_OK;
}

static bc_rec_result fixture_capture_stop(void *context, uint64_t id)
{
    recording_fixture *fixture = context;

    log_call(fixture, CALL_CAPTURE_STOP);
    ++fixture->capture_stop_calls;
    (void)id;
    if (fixture->capture_stop_error != BC_REC_OK)
        return fixture->capture_stop_error;
    fixture->capture_running = false;
    return BC_REC_OK;
}

static bool fixture_capture_abort(void *context, uint64_t id)
{
    recording_fixture *fixture = context;

    log_call(fixture, CALL_CAPTURE_ABORT);
    ++fixture->capture_abort_calls;
    (void)id;
    if (fixture->capture_abort_result)
        fixture->capture_running = false;
    return fixture->capture_abort_result;
}

static bool fixture_live(void *context, uint64_t id, uint32_t sequence,
                         const uint8_t *data, uint16_t length)
{
    recording_fixture *fixture = context;
    unsigned index;

    log_call(fixture, CALL_LIVE);
    ++fixture->live_calls;
    index = fixture->live_calls - 1U;
    if (index < ARRAY_LEN(fixture->lives))
    {
        fixture->lives[index].id = id;
        fixture->lives[index].sequence = sequence;
        fixture->lives[index].length = length;
        if (data != NULL && length <= sizeof(fixture->lives[index].bytes))
            memcpy(fixture->lives[index].bytes, data, length);
    }
    return fixture->live_result;
}

static void fixture_changed(void *context, const bc_rec_snapshot *snapshot)
{
    recording_fixture *fixture = context;

    log_call(fixture, CALL_CHANGED);
    ++fixture->changed_calls;
    fixture->last_changed = *snapshot;
}

static bc_rec_port fixture_port(recording_fixture *fixture)
{
    bc_rec_port port;
    memset(&port, 0, sizeof(port));
    port.ctx = fixture;
    port.open = fixture_open;
    port.append = fixture_append;
    port.checkpoint = fixture_checkpoint;
    port.finish = fixture_finish;
    port.capture_start = fixture_capture_start;
    port.capture_stop = fixture_capture_stop;
    port.capture_abort = fixture_capture_abort;
    port.live = fixture_live;
    port.changed = fixture_changed;
    return port;
}

static bc_rec_config default_config(void)
{
    bc_rec_config config;
    config.checkpoint_ms = 1000U;
    config.checkpoint_bytes = 1000U;
    config.stop_timeout_ms = 50U;
    return config;
}

static bool init_owner(bc_recording *recording, recording_fixture *fixture,
                       const bc_rec_config *config)
{
    bc_rec_port port = fixture_port(fixture);
    return bc_recording_init(recording, &port, config);
}

static bc_rec_start start_request(uint64_t id, bc_rec_trigger trigger,
                                  uint32_t duration_limit_ms)
{
    bc_rec_start start;
    memset(&start, 0, sizeof(start));
    start.id = id;
    start.trigger = trigger;
    start.duration_limit_ms = duration_limit_ms;
    return start;
}

static void settle_fault(bc_recording *recording, uint64_t id,
                         uint32_t now_ms)
{
    const bc_rec_snapshot *snapshot = bc_recording_snapshot(recording);

    if (snapshot->phase == BC_REC_RECORDING ||
        snapshot->phase == BC_REC_STARTING)
        (void)bc_recording_stop(recording, id, now_ms);
    snapshot = bc_recording_snapshot(recording);
    if (snapshot->phase == BC_REC_STOPPING)
        (void)bc_recording_drained(recording, id);
}

static int find_call_after(const recording_fixture *fixture, call_kind kind,
                           unsigned after)
{
    unsigned i;
    for (i = after; i < fixture->call_count; ++i)
    {
        if (fixture->calls[i] == (int)kind)
            return (int)i;
    }
    return -1;
}

static void check_no_saved(const bc_recording *recording)
{
    const bc_rec_snapshot *snapshot = bc_recording_snapshot(recording);
    CHECK(snapshot->phase != BC_REC_SAVED);
    CHECK(!snapshot->file.complete);
    CHECK(snapshot->error != BC_REC_OK);
}

static void test_configured_max_intervals(void)
{
    recording_fixture fixture;
    bc_recording recording;
    bc_rec_config config = {
        (uint32_t)BC_REC_MAX_INTERVAL,
        (uint32_t)BC_REC_MAX_INTERVAL,
        (uint32_t)BC_REC_MAX_INTERVAL
    };

    fixture_reset(&fixture);
    CHECK(init_owner(&recording, &fixture, &config));

    config = (bc_rec_config){(uint32_t)BC_REC_MAX_INTERVAL + 1U,
                             (uint32_t)BC_REC_MAX_INTERVAL,
                             (uint32_t)BC_REC_MAX_INTERVAL};
    CHECK(!init_owner(&recording, &fixture, &config));
    config = (bc_rec_config){(uint32_t)BC_REC_MAX_INTERVAL,
                             (uint32_t)BC_REC_MAX_INTERVAL,
                             (uint32_t)BC_REC_MAX_INTERVAL + 1U};
    CHECK(!init_owner(&recording, &fixture, &config));

    fixture_reset(&fixture);
    config = (bc_rec_config){(uint32_t)BC_REC_MAX_INTERVAL,
                             (uint32_t)BC_REC_MAX_INTERVAL,
                             (uint32_t)BC_REC_MAX_INTERVAL};
    CHECK(init_owner(&recording, &fixture, &config));
    {
        bc_rec_start start = start_request(0x1010U, BC_REC_PTT,
                                            (uint32_t)BC_REC_MAX_INTERVAL + 1U);
        CHECK(bc_recording_start(&recording, &start, 1U) == BC_REC_INVALID);
        CHECK(fixture.open_calls == 0U);
    }
}

static void test_append_before_live_and_link_loss(void)
{
    recording_fixture fixture;
    bc_recording recording;
    bc_rec_config config = default_config();
    bc_rec_start start = start_request(0x1001U, BC_REC_PTT, 0U);
    const uint8_t first[] = {1U, 2U, 3U};
    const uint8_t second[] = {4U, 5U};
    const uint8_t tail[] = {6U, 7U, 8U, 9U};
    bc_rec_result result;
    int append_call;
    int live_call;

    fixture_reset(&fixture);
    CHECK(init_owner(&recording, &fixture, &config));
    result = bc_recording_start(&recording, &start, 100U);
    CHECK(result == BC_REC_OK);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);

    bc_recording_link(&recording, true);
    result = bc_recording_frame(&recording, start.id, 1U, first,
                                (uint16_t)sizeof(first), 110U);
    CHECK(result == BC_REC_OK);
    append_call = find_call_after(&fixture, CALL_APPEND, 0U);
    live_call = find_call_after(&fixture, CALL_LIVE, 0U);
    CHECK(append_call >= 0);
    CHECK(live_call > append_call);
    CHECK(fixture.storage_bytes == sizeof(first));
    CHECK(memcmp(fixture.storage, first, sizeof(first)) == 0);
    CHECK(fixture.live_calls == 1U);
    CHECK(fixture.lives[0].id == start.id);
    CHECK(fixture.lives[0].sequence == 1U);
    CHECK(fixture.lives[0].length == sizeof(first));
    CHECK(memcmp(fixture.lives[0].bytes, first, sizeof(first)) == 0);

    bc_recording_link(&recording, false);
    result = bc_recording_frame(&recording, start.id, 2U, second,
                                (uint16_t)sizeof(second), 120U);
    CHECK(result == BC_REC_OK);
    CHECK(fixture.storage_bytes == sizeof(first) + sizeof(second));
    CHECK(fixture.live_calls == 1U);

    bc_recording_link(&recording, true);
    result = bc_recording_frame(&recording, start.id, 3U, tail,
                                (uint16_t)sizeof(tail), 130U);
    CHECK(result == BC_REC_OK);
    CHECK(fixture.storage_bytes == sizeof(first) + sizeof(second) +
          sizeof(tail));
    CHECK(fixture.live_calls == 2U);

    result = bc_recording_stop(&recording, start.id, 140U);
    CHECK(result == BC_REC_OK);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    CHECK(fixture.finish_calls == 0U);

    /* A complete frame already in the capture queue is still accepted. */
    result = bc_recording_frame(&recording, start.id, 4U, first,
                                (uint16_t)sizeof(first), 141U);
    CHECK(result == BC_REC_OK);
    CHECK(fixture.storage_bytes == sizeof(first) + sizeof(second) +
          sizeof(tail) + sizeof(first));
    CHECK(fixture.live_calls == 3U);
    CHECK(fixture.finish_calls == 0U);

    result = bc_recording_drained(&recording, start.id);
    CHECK(result == BC_REC_OK);
    CHECK(fixture.finish_calls == 1U);
    CHECK(fixture.last_finish_complete);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);
    CHECK(bc_recording_snapshot(&recording)->accepted_bytes ==
          fixture.storage_bytes);
    CHECK(bc_recording_snapshot(&recording)->accepted_frames == 4U);
    CHECK(bc_recording_snapshot(&recording)->durable_bytes ==
          fixture.storage_bytes);
    CHECK(bc_recording_snapshot(&recording)->file.bytes ==
          fixture.storage_bytes);
    CHECK(bc_recording_snapshot(&recording)->file.frames == 4U);
    CHECK(bc_recording_snapshot(&recording)->file.crc32 ==
          crc32_bytes(fixture.storage, fixture.storage_bytes));
    CHECK(memcmp(fixture.storage, first, sizeof(first)) == 0);
    CHECK(memcmp(fixture.storage + sizeof(first), second, sizeof(second)) == 0);
    CHECK(memcmp(fixture.storage + sizeof(first) + sizeof(second), tail,
                 sizeof(tail)) == 0);
    CHECK(!bc_recording_active(&recording));
}

static void test_live_failure_is_diagnostic_only(void)
{
    recording_fixture fixture;
    bc_recording recording;
    bc_rec_config config = default_config();
    bc_rec_start start = start_request(0x1002U, BC_REC_MEMO, 0U);
    const uint8_t data[] = {0xa1U, 0xb2U};

    fixture_reset(&fixture);
    fixture.live_result = false;
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, 200U) == BC_REC_OK);
    bc_recording_link(&recording, true);
    CHECK(bc_recording_frame(&recording, start.id, 1U, data,
                             (uint16_t)sizeof(data), 201U) == BC_REC_OK);
    CHECK(fixture.storage_bytes == sizeof(data));
    CHECK(fixture.live_calls == 1U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_OK);
    CHECK(bc_recording_snapshot(&recording)->live_dropped_frames == 1U);
}

static void test_stop_and_start_idempotency(void)
{
    recording_fixture fixture;
    bc_recording recording;
    bc_rec_config config = default_config();
    bc_rec_start start = start_request(0x2001U, BC_REC_PTT, 0U);
    const uint8_t first[] = {9U, 8U};
    const uint8_t tail[] = {7U};
    unsigned capture_stop_calls;
    unsigned finish_calls;
    bc_rec_result result;

    fixture_reset(&fixture);
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, 300U) == BC_REC_OK);
    CHECK(bc_recording_frame(&recording, start.id, 1U, first,
                             (uint16_t)sizeof(first), 301U) == BC_REC_OK);

    /* A retried Start must not create another storage identity. */
    result = bc_recording_start(&recording, &start, 302U);
    CHECK(result == BC_REC_OK);
    CHECK(fixture.open_calls == 1U);
    CHECK(fixture.capture_start_calls == 1U);

    result = bc_recording_stop(&recording, start.id, 310U);
    CHECK(result == BC_REC_OK);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    capture_stop_calls = fixture.capture_stop_calls;
    finish_calls = fixture.finish_calls;

    result = bc_recording_stop(&recording, start.id, 311U);
    CHECK(result != BC_REC_CAPTURE_ERROR);
    CHECK(fixture.capture_stop_calls == capture_stop_calls);
    CHECK(fixture.finish_calls == finish_calls);

    CHECK(bc_recording_frame(&recording, start.id, 2U, tail,
                             (uint16_t)sizeof(tail), 312U) == BC_REC_OK);
    CHECK(fixture.finish_calls == 0U);
    CHECK(bc_recording_drained(&recording, start.id) == BC_REC_OK);
    CHECK(fixture.finish_calls == 1U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);

    /* Replayed drain and Stop cannot finalize or open a second recording. */
    finish_calls = fixture.finish_calls;
    result = bc_recording_drained(&recording, start.id);
    (void)result;
    CHECK(fixture.finish_calls == finish_calls);
    CHECK(fixture.open_calls == 1U);
}

static void test_wrong_sessions_cannot_mutate_owner(void)
{
    recording_fixture fixture;
    bc_recording recording;
    bc_rec_config config = default_config();
    bc_rec_start first = start_request(0x2101U, BC_REC_PTT, 0U);
    bc_rec_start second = start_request(0x2102U, BC_REC_MEMO, 0U);
    const uint8_t data[] = {1U, 3U, 5U};
    unsigned appends;
    unsigned capture_stops;

    fixture_reset(&fixture);
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &first, 320U) == BC_REC_OK);
    appends = fixture.append_calls;
    capture_stops = fixture.capture_stop_calls;
    CHECK(bc_recording_frame(&recording, second.id, 1U, data,
                             (uint16_t)sizeof(data), 321U) ==
          BC_REC_WRONG_SESSION);
    CHECK(bc_recording_stop(&recording, second.id, 322U) ==
          BC_REC_WRONG_SESSION);
    CHECK(bc_recording_drained(&recording, second.id) ==
          BC_REC_WRONG_SESSION);
    CHECK(fixture.append_calls == appends);
    CHECK(fixture.capture_stop_calls == capture_stops);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);

    CHECK(bc_recording_frame(&recording, first.id, 1U, data,
                             (uint16_t)sizeof(data), 323U) == BC_REC_OK);
    CHECK(bc_recording_stop(&recording, first.id, 324U) == BC_REC_OK);
    CHECK(bc_recording_drained(&recording, first.id) == BC_REC_OK);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);

    /* An old session cannot stop a later owner either. */
    CHECK(bc_recording_start(&recording, &second, 330U) == BC_REC_OK);
    appends = fixture.append_calls;
    capture_stops = fixture.capture_stop_calls;
    CHECK(bc_recording_frame(&recording, first.id, 1U, data,
                             (uint16_t)sizeof(data), 331U) ==
          BC_REC_WRONG_SESSION);
    CHECK(bc_recording_stop(&recording, first.id, 332U) ==
          BC_REC_WRONG_SESSION);
    CHECK(fixture.append_calls == appends);
    CHECK(fixture.capture_stop_calls == capture_stops);
    CHECK(bc_recording_stop(&recording, second.id, 333U) == BC_REC_OK);
    CHECK(bc_recording_drained(&recording, second.id) == BC_REC_EMPTY_AUDIO);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_EMPTY);
}

static void test_zero_audio_is_empty(void)
{
    recording_fixture fixture;
    bc_recording recording;
    bc_rec_config config = default_config();
    bc_rec_start start = start_request(0x2201U, BC_REC_MEMO, 0U);
    bc_rec_result result;

    fixture_reset(&fixture);
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, 340U) == BC_REC_OK);
    CHECK(bc_recording_stop(&recording, start.id, 341U) == BC_REC_OK);
    result = bc_recording_drained(&recording, start.id);
    CHECK(result == BC_REC_EMPTY_AUDIO);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_EMPTY);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_EMPTY_AUDIO);
    CHECK(bc_recording_snapshot(&recording)->accepted_bytes == 0U);
    CHECK(bc_recording_snapshot(&recording)->accepted_frames == 0U);
    CHECK(fixture.finish_calls == 1U);
    CHECK(bc_recording_snapshot(&recording)->phase != BC_REC_SAVED);
}

static void test_duplicate_frames_are_not_appended(void)
{
    recording_fixture fixture;
    bc_recording recording;
    bc_rec_config config = default_config();
    bc_rec_start start = start_request(0x2301U, BC_REC_APP, 0U);
    const uint8_t first[] = {0x10U, 0x11U, 0x12U};
    const uint8_t duplicate[] = {0xf0U, 0xf1U, 0xf2U};
    const uint8_t second[] = {0x20U};

    fixture_reset(&fixture);
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, 350U) == BC_REC_OK);
    bc_recording_link(&recording, true);
    CHECK(bc_recording_frame(&recording, start.id, 1U, first,
                             (uint16_t)sizeof(first), 351U) == BC_REC_OK);
    CHECK(bc_recording_frame(&recording, start.id, 1U, duplicate,
                             (uint16_t)sizeof(duplicate), 352U) ==
          BC_REC_DUPLICATE);
    CHECK(fixture.append_calls == 1U);
    CHECK(fixture.live_calls == 1U);
    CHECK(fixture.storage_bytes == sizeof(first));
    CHECK(memcmp(fixture.storage, first, sizeof(first)) == 0);
    CHECK(bc_recording_snapshot(&recording)->duplicate_frames == 1U);

    CHECK(bc_recording_frame(&recording, start.id, 2U, second,
                             (uint16_t)sizeof(second), 353U) == BC_REC_OK);
    CHECK(fixture.append_calls == 2U);
    CHECK(fixture.storage_bytes == sizeof(first) + sizeof(second));
    CHECK(bc_recording_stop(&recording, start.id, 354U) == BC_REC_OK);
    CHECK(bc_recording_drained(&recording, start.id) == BC_REC_OK);
    CHECK(bc_recording_snapshot(&recording)->file.bytes ==
          sizeof(first) + sizeof(second));
    CHECK(bc_recording_snapshot(&recording)->file.frames == 2U);
}

static void test_invalid_frames_retain_partial(void)
{
    recording_fixture fixture;
    bc_recording recording;
    bc_rec_config config;
    bc_rec_start start;
    const uint8_t valid[] = {0x31U, 0x32U};
    uint8_t oversize[BC_REC_FRAME_MAX + 1U];
    bc_rec_result result;
    uint32_t retained_bytes;
    unsigned retained_frames;

    memset(oversize, 0x44, sizeof(oversize));

    /* A sequence gap keeps the durable prefix and closes as PARTIAL. */
    fixture_reset(&fixture);
    config = default_config();
    start = start_request(0x2401U, BC_REC_PTT, 0U);
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, 360U) == BC_REC_OK);
    CHECK(bc_recording_frame(&recording, start.id, 1U, valid,
                             (uint16_t)sizeof(valid), 361U) == BC_REC_OK);
    retained_bytes = fixture.storage_bytes;
    retained_frames = fixture.storage_frames;
    result = bc_recording_frame(&recording, start.id, 3U, valid,
                                (uint16_t)sizeof(valid), 362U);
    CHECK(result == BC_REC_SEQUENCE_GAP);
    settle_fault(&recording, start.id, 363U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_SEQUENCE_GAP);
    CHECK(bc_recording_snapshot(&recording)->accepted_bytes == retained_bytes);
    CHECK(bc_recording_snapshot(&recording)->accepted_frames == retained_frames);
    CHECK(fixture.storage_bytes == retained_bytes);
    CHECK(!bc_recording_snapshot(&recording)->file.complete);

    /* Oversize and null frames fail before touching storage or live. */
    fixture_reset(&fixture);
    start = start_request(0x2402U, BC_REC_MEMO, 0U);
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, 370U) == BC_REC_OK);
    result = bc_recording_frame(&recording, start.id, 1U, oversize,
                                (uint16_t)sizeof(oversize), 371U);
    CHECK(result == BC_REC_CAPTURE_OVERFLOW);
    CHECK(fixture.append_calls == 0U);
    CHECK(fixture.live_calls == 0U);
    settle_fault(&recording, start.id, 372U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_CAPTURE_OVERFLOW);
    CHECK(!bc_recording_snapshot(&recording)->file.complete);

    fixture_reset(&fixture);
    start = start_request(0x2403U, BC_REC_APP, 0U);
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, 380U) == BC_REC_OK);
    result = bc_recording_frame(&recording, start.id, 1U, NULL, 1U, 381U);
    CHECK(result == BC_REC_INVALID);
    CHECK(fixture.append_calls == 0U);
    CHECK(fixture.live_calls == 0U);
    settle_fault(&recording, start.id, 382U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_INVALID);
    CHECK(!bc_recording_snapshot(&recording)->file.complete);
}

static void test_error_paths_never_save(void)
{
    recording_fixture fixture;
    bc_recording recording;
    bc_rec_config config;
    bc_rec_start start;
    const uint8_t data[] = {0x51U, 0x52U};
    bc_rec_result result;

    config = default_config();
    start = start_request(0x2501U, BC_REC_PTT, 0U);
    fixture_reset(&fixture);
    fixture.open_error = BC_REC_NO_SPACE;
    CHECK(init_owner(&recording, &fixture, &config));
    result = bc_recording_start(&recording, &start, 390U);
    CHECK(result == BC_REC_NO_SPACE);
    CHECK(fixture.capture_start_calls == 0U);
    check_no_saved(&recording);

    fixture_reset(&fixture);
    fixture.open_error = BC_REC_OPEN_ERROR;
    CHECK(init_owner(&recording, &fixture, &config));
    result = bc_recording_start(&recording, &start, 400U);
    CHECK(result == BC_REC_OPEN_ERROR);
    CHECK(fixture.capture_start_calls == 0U);
    check_no_saved(&recording);

    fixture_reset(&fixture);
    fixture.capture_start_error = BC_REC_CAPTURE_ERROR;
    CHECK(init_owner(&recording, &fixture, &config));
    result = bc_recording_start(&recording, &start, 410U);
    CHECK(result == BC_REC_CAPTURE_ERROR);
    CHECK(fixture.capture_start_calls == 1U);
    check_no_saved(&recording);

    fixture_reset(&fixture);
    fixture.append_error = BC_REC_WRITE_ERROR;
    fixture.append_error_call = 1U;
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, 420U) == BC_REC_OK);
    result = bc_recording_frame(&recording, start.id, 1U, data,
                                (uint16_t)sizeof(data), 421U);
    CHECK(result == BC_REC_WRITE_ERROR);
    CHECK(fixture.live_calls == 0U);
    check_no_saved(&recording);

    fixture_reset(&fixture);
    config.checkpoint_bytes = 1U;
    fixture.checkpoint_error = BC_REC_SYNC_ERROR;
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, 430U) == BC_REC_OK);
    result = bc_recording_frame(&recording, start.id, 1U, data,
                                (uint16_t)sizeof(data), 431U);
    CHECK(fixture.checkpoint_calls >= 1U);
    CHECK(result != BC_REC_OK ||
          bc_recording_snapshot(&recording)->error == BC_REC_SYNC_ERROR);
    check_no_saved(&recording);

    fixture_reset(&fixture);
    fixture.finish_error = BC_REC_CLOSE_ERROR;
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, 440U) == BC_REC_OK);
    /* The previous config still checkpoints after the frame; disable it. */
    recording.config.checkpoint_bytes = 1000U;
    CHECK(bc_recording_frame(&recording, start.id, 1U, data,
                             (uint16_t)sizeof(data), 441U) == BC_REC_OK);
    CHECK(bc_recording_stop(&recording, start.id, 442U) == BC_REC_OK);
    result = bc_recording_drained(&recording, start.id);
    CHECK(result == BC_REC_CLOSE_ERROR);
    check_no_saved(&recording);

    /* Storage must not turn accepted audio into a successful empty clip. A
     * finish callback that reports EMPTY after accepted bytes is a metadata
     * failure; retain the owner as PARTIAL with an honest error. */
    fixture_reset(&fixture);
    fixture.finish_error = BC_REC_EMPTY_AUDIO;
    fixture.finish_empty_metadata = true;
    config = default_config();
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, 445U) == BC_REC_OK);
    CHECK(bc_recording_frame(&recording, start.id, 1U, data,
                             (uint16_t)sizeof(data), 446U) == BC_REC_OK);
    CHECK(bc_recording_stop(&recording, start.id, 447U) == BC_REC_OK);
    result = bc_recording_drained(&recording, start.id);
    CHECK(result == BC_REC_CLOSE_ERROR);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_CLOSE_ERROR);
    CHECK(bc_recording_snapshot(&recording)->accepted_bytes == sizeof(data));
    CHECK(bc_recording_snapshot(&recording)->accepted_frames == 1U);
    CHECK(!bc_recording_snapshot(&recording)->file.complete);
    CHECK(fixture.finish_calls == 1U);

    fixture_reset(&fixture);
    fixture.capture_stop_error = BC_REC_CAPTURE_ERROR;
    config = default_config();
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, 450U) == BC_REC_OK);
    CHECK(bc_recording_frame(&recording, start.id, 1U, data,
                             (uint16_t)sizeof(data), 451U) == BC_REC_OK);
    result = bc_recording_stop(&recording, start.id, 452U);
    CHECK(result == BC_REC_CAPTURE_ERROR);
    check_no_saved(&recording);
    if (bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING)
    {
        (void)bc_recording_drained(&recording, start.id);
        check_no_saved(&recording);
    }
}

static void test_failed_abort_blocks_new_start_until_drained(void)
{
    recording_fixture fixture;
    bc_recording recording;
    bc_rec_config config = default_config();
    bc_rec_start first = start_request(0x2601U, BC_REC_PTT, 0U);
    bc_rec_start second = start_request(0x2602U, BC_REC_MEMO, 0U);
    const uint8_t data[] = {0x61U};
    bc_rec_result result;

    fixture_reset(&fixture);
    fixture.capture_abort_result = false;
    config.stop_timeout_ms = 5U;
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &first, 460U) == BC_REC_OK);
    CHECK(bc_recording_frame(&recording, first.id, 1U, data,
                             (uint16_t)sizeof(data), 461U) == BC_REC_OK);
    result = bc_recording_fault(&recording, first.id, BC_REC_CAPTURE_ERROR,
                                462U);
    CHECK(result == BC_REC_CAPTURE_ERROR);
    bc_recording_tick(&recording, 467U);
    CHECK(fixture.capture_abort_calls >= 1U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    CHECK(bc_recording_active(&recording));

    result = bc_recording_start(&recording, &second, 463U);
    CHECK(result == BC_REC_BUSY);
    CHECK(fixture.open_calls == 1U);
    CHECK(fixture.capture_start_calls == 1U);

    result = bc_recording_drained(&recording, first.id);
    CHECK(result != BC_REC_OK ||
          bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(!bc_recording_snapshot(&recording)->file.complete);

    fixture.capture_abort_result = true;
    CHECK(bc_recording_start(&recording, &second, 470U) == BC_REC_OK);
    CHECK(fixture.open_calls == 2U);
    CHECK(fixture.capture_start_calls == 2U);
}

static void test_duration_and_checkpoint_across_wrap(void)
{
    recording_fixture fixture;
    bc_recording recording;
    bc_rec_config config = default_config();
    bc_rec_start start = start_request(0x2701U, BC_REC_PTT, 10U);
    const uint8_t data[] = {0x71U, 0x72U};
    uint32_t start_ms = UINT32_MAX - 2U;

    config.checkpoint_ms = 3U;
    config.checkpoint_bytes = 1000U;
    fixture_reset(&fixture);
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, start_ms) == BC_REC_OK);
    CHECK(bc_recording_frame(&recording, start.id, 1U, data,
                             (uint16_t)sizeof(data), UINT32_MAX - 1U) ==
          BC_REC_OK);

    /* Elapsed time is 3 ms at tick zero despite uint32 wrap. */
    bc_recording_tick(&recording, 0U);
    CHECK(fixture.checkpoint_calls == 1U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);

    bc_recording_tick(&recording, 6U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    bc_recording_tick(&recording, 7U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    CHECK(fixture.capture_stop_calls == 1U);
    CHECK(fixture.finish_calls == 0U);
    CHECK(bc_recording_drained(&recording, start.id) == BC_REC_OK);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);
}

static void test_stop_timeout_across_wrap(void)
{
    recording_fixture fixture;
    bc_recording recording;
    bc_rec_config config = default_config();
    bc_rec_start start = start_request(0x2801U, BC_REC_MEMO, 0U);
    const uint8_t data[] = {0x81U};

    config.stop_timeout_ms = 5U;
    fixture_reset(&fixture);
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, UINT32_MAX - 2U) ==
          BC_REC_OK);
    CHECK(bc_recording_frame(&recording, start.id, 1U, data,
                             (uint16_t)sizeof(data), UINT32_MAX - 1U) ==
          BC_REC_OK);
    CHECK(bc_recording_stop(&recording, start.id, UINT32_MAX - 1U) ==
          BC_REC_OK);
    bc_recording_tick(&recording, 2U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    CHECK(fixture.finish_calls == 0U);
    bc_recording_tick(&recording, 3U);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_STOP_TIMEOUT);
    CHECK(bc_recording_snapshot(&recording)->phase != BC_REC_SAVED);
    if (bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING)
        (void)bc_recording_drained(&recording, start.id);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL ||
          bc_recording_snapshot(&recording)->phase == BC_REC_FAILED);
    CHECK(!bc_recording_snapshot(&recording)->file.complete);
}

static void set_existing(recording_fixture *fixture,
                          const bc_rec_start *start, const char *name,
                          uint32_t bytes, uint32_t frames, uint32_t crc32,
                          bool complete, bool recovered)
{
    fixture->existing = true;
    fixture->existing_start = *start;
    memset(&fixture->existing_file, 0, sizeof(fixture->existing_file));
    (void)snprintf(fixture->existing_file.name,
                   sizeof(fixture->existing_file.name), "%s", name);
    fixture->existing_file.bytes = bytes;
    fixture->existing_file.frames = frames;
    fixture->existing_file.crc32 = crc32;
    fixture->existing_file.complete = complete;
    fixture->existing_file.recovered = recovered;
}

static void test_persistent_already_exists_replay(void)
{
    recording_fixture fixture;
    bc_recording recording;
    bc_rec_config config = default_config();
    bc_rec_start start = start_request(0x2901U, BC_REC_APP, 900U);
    bc_rec_start wrong_trigger;
    bc_rec_start wrong_duration;

    fixture_reset(&fixture);
    set_existing(&fixture, &start, "rec-persist.raw", 42U, 3U,
                 0x12345678U, true, false);
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, 500U) == BC_REC_OK);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_SAVED);
    CHECK(bc_recording_snapshot(&recording)->start.id == start.id);
    CHECK(bc_recording_snapshot(&recording)->start.trigger == start.trigger);
    CHECK(bc_recording_snapshot(&recording)->start.duration_limit_ms ==
          start.duration_limit_ms);
    CHECK(strcmp(bc_recording_snapshot(&recording)->file.name,
                 fixture.existing_file.name) == 0);
    CHECK(bc_recording_snapshot(&recording)->file.bytes ==
          fixture.existing_file.bytes);
    CHECK(bc_recording_snapshot(&recording)->file.frames ==
          fixture.existing_file.frames);
    CHECK(bc_recording_snapshot(&recording)->file.crc32 ==
          fixture.existing_file.crc32);
    CHECK(bc_recording_snapshot(&recording)->file.complete ==
          fixture.existing_file.complete);
    CHECK(bc_recording_snapshot(&recording)->file.recovered ==
          fixture.existing_file.recovered);
    CHECK(fixture.capture_start_calls == 0U);
    CHECK(!bc_recording_active(&recording));

    wrong_trigger = start;
    wrong_trigger.trigger = BC_REC_PTT;
    fixture_reset(&fixture);
    set_existing(&fixture, &start, "rec-persist.raw", 42U, 3U,
                 0x12345678U, true, false);
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &wrong_trigger, 501U) ==
          BC_REC_INVALID);
    CHECK(fixture.capture_start_calls == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase != BC_REC_SAVED);

    wrong_duration = start;
    wrong_duration.duration_limit_ms = 901U;
    fixture_reset(&fixture);
    set_existing(&fixture, &start, "rec-persist.raw", 42U, 3U,
                 0x12345678U, true, false);
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &wrong_duration, 502U) ==
          BC_REC_INVALID);
    CHECK(fixture.capture_start_calls == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase != BC_REC_SAVED);

    start.id = 0x2902U;
    fixture_reset(&fixture);
    set_existing(&fixture, &start, "rec-recovered.raw", 17U, 2U,
                 0xabcdef01U, false, true);
    CHECK(init_owner(&recording, &fixture, &config));
    CHECK(bc_recording_start(&recording, &start, 503U) ==
          BC_REC_INTERRUPTED);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
    CHECK(bc_recording_snapshot(&recording)->file.bytes == 17U);
    CHECK(bc_recording_snapshot(&recording)->file.frames == 2U);
    CHECK(!bc_recording_snapshot(&recording)->file.complete);
    CHECK(bc_recording_snapshot(&recording)->file.recovered);
    CHECK(fixture.capture_start_calls == 0U);
}

int main(void)
{
    test_configured_max_intervals();
    test_append_before_live_and_link_loss();
    test_live_failure_is_diagnostic_only();
    test_stop_and_start_idempotency();
    test_wrong_sessions_cannot_mutate_owner();
    test_zero_audio_is_empty();
    test_duplicate_frames_are_not_appended();
    test_invalid_frames_retain_partial();
    test_error_paths_never_save();
    test_failed_abort_blocks_new_start_until_drained();
    test_duration_and_checkpoint_across_wrap();
    test_stop_timeout_across_wrap();
    test_persistent_already_exists_replay();

    if (failures != 0U)
    {
        fprintf(stderr, "FAIL: %u of %u checks\n", failures, checks);
        return 1;
    }
    printf("PASS: %u checks (portable recording owner, storage/live ordering,\n"
           "      sessions, drain/fault handling, checkpoints and recovery)\n",
           checks);
    return 0;
}
