#include "bc_voice_gesture.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ARRAY_LEN(value) (sizeof(value) / sizeof((value)[0]))
#define MAX_CALLS 256U
#define MAX_LIVE 32U
#define STORAGE_CAPACITY 1024U

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
    CALL_NEW_ID = 1,
    CALL_OPEN,
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
} live_observation;

typedef struct {
    int calls[MAX_CALLS];
    unsigned call_count;
    unsigned new_id_calls;
    unsigned open_calls;
    unsigned append_calls;
    unsigned checkpoint_calls;
    unsigned finish_calls;
    unsigned capture_start_calls;
    unsigned capture_stop_calls;
    unsigned capture_abort_calls;
    unsigned live_calls;
    unsigned changed_calls;

    uint64_t next_id;
    bc_rec_start last_open_start;
    bc_rec_start last_capture_start;
    bc_rec_start last_capture_stop;
    bc_rec_start fresh_start;
    bool capture_running;
    bool storage_open;

    uint8_t storage[STORAGE_CAPACITY];
    uint32_t storage_bytes;
    uint32_t storage_frames;
    live_observation lives[MAX_LIVE];
    bc_rec_file last_checkpoint_file;
    bc_rec_file last_finish_file;
    bool last_finish_complete;
    bc_rec_snapshot last_changed;

    bc_rec_result id_error;
    bool return_zero_id;
    bc_rec_result open_error;
    bc_rec_result append_error;
    unsigned append_error_call;
    bc_rec_result checkpoint_error;
    bc_rec_result finish_error;
    bc_rec_result capture_start_error;
    bc_rec_result capture_stop_error;
    bool capture_abort_result;
    bool live_result;
} gesture_fixture;

static uint32_t crc32_bytes(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xffffffffU;
    uint32_t i;

    for (i = 0U; i < length; ++i)
    {
        uint32_t bit;
        crc ^= data[i];
        for (bit = 0U; bit < 8U; ++bit)
            crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
    }
    return crc ^ 0xffffffffU;
}

static void log_call(gesture_fixture *fixture, call_kind kind)
{
    if (fixture->call_count < ARRAY_LEN(fixture->calls))
        fixture->calls[fixture->call_count++] = (int)kind;
}

static void fixture_reset(gesture_fixture *fixture)
{
    memset(fixture, 0, sizeof(*fixture));
    fixture->next_id = 0x1000U;
    fixture->id_error = BC_REC_OK;
    fixture->open_error = BC_REC_OK;
    fixture->append_error = BC_REC_OK;
    fixture->checkpoint_error = BC_REC_OK;
    fixture->finish_error = BC_REC_OK;
    fixture->capture_start_error = BC_REC_OK;
    fixture->capture_stop_error = BC_REC_OK;
    fixture->capture_abort_result = true;
    fixture->live_result = true;
}

static bc_rec_result fixture_new_id(void *context, uint64_t *id)
{
    gesture_fixture *fixture = context;

    log_call(fixture, CALL_NEW_ID);
    ++fixture->new_id_calls;
    if (fixture->id_error != BC_REC_OK)
        return fixture->id_error;
    if (fixture->return_zero_id)
    {
        *id = 0U;
        return BC_REC_OK;
    }
    *id = fixture->next_id++;
    return BC_REC_OK;
}

static bc_rec_result fixture_open(void *context, const bc_rec_start *start,
                                  bc_rec_file *file)
{
    gesture_fixture *fixture = context;

    log_call(fixture, CALL_OPEN);
    ++fixture->open_calls;
    fixture->last_open_start = *start;
    if (fixture->open_error != BC_REC_OK)
        return fixture->open_error;

    fixture->fresh_start = *start;
    fixture->storage_open = true;
    fixture->storage_bytes = 0U;
    fixture->storage_frames = 0U;
    memset(fixture->storage, 0, sizeof(fixture->storage));
    memset(file, 0, sizeof(*file));
    (void)snprintf(file->name, sizeof(file->name), "rec-%llx.raw",
                   (unsigned long long)start->id);
    return BC_REC_OK;
}

static bc_rec_result fixture_append(void *context, const uint8_t *data,
                                    uint16_t length)
{
    gesture_fixture *fixture = context;

    log_call(fixture, CALL_APPEND);
    ++fixture->append_calls;
    if (fixture->append_error != BC_REC_OK &&
        fixture->append_calls == fixture->append_error_call)
        return fixture->append_error;
    if (data == NULL || length == 0U ||
        length > STORAGE_CAPACITY - fixture->storage_bytes)
        return BC_REC_WRITE_ERROR;
    memcpy(fixture->storage + fixture->storage_bytes, data, length);
    fixture->storage_bytes += length;
    ++fixture->storage_frames;
    return BC_REC_OK;
}

static bc_rec_result fixture_checkpoint(void *context, bc_rec_file *file)
{
    gesture_fixture *fixture = context;

    log_call(fixture, CALL_CHECKPOINT);
    ++fixture->checkpoint_calls;
    if (fixture->checkpoint_error != BC_REC_OK)
        return fixture->checkpoint_error;
    file->bytes = fixture->storage_bytes;
    file->frames = fixture->storage_frames;
    file->crc32 = crc32_bytes(fixture->storage, fixture->storage_bytes);
    file->complete = false;
    file->recovered = false;
    file->delivered = false;
    fixture->last_checkpoint_file = *file;
    return BC_REC_OK;
}

static bc_rec_result fixture_finish(void *context, bool complete,
                                    bc_rec_file *file)
{
    gesture_fixture *fixture = context;

    log_call(fixture, CALL_FINISH);
    ++fixture->finish_calls;
    fixture->last_finish_complete = complete;
    file->bytes = fixture->storage_bytes;
    file->frames = fixture->storage_frames;
    file->crc32 = crc32_bytes(fixture->storage, fixture->storage_bytes);
    file->complete = complete;
    file->recovered = !complete;
    file->delivered = false;
    fixture->last_finish_file = *file;
    fixture->storage_open = false;
    if (fixture->finish_error != BC_REC_OK)
        return fixture->finish_error;
    return BC_REC_OK;
}

static bc_rec_result fixture_capture_start(void *context, uint64_t id)
{
    gesture_fixture *fixture = context;

    log_call(fixture, CALL_CAPTURE_START);
    ++fixture->capture_start_calls;
    fixture->last_capture_start.id = id;
    if (fixture->capture_start_error != BC_REC_OK)
        return fixture->capture_start_error;
    fixture->capture_running = true;
    return BC_REC_OK;
}

static bc_rec_result fixture_capture_stop(void *context, uint64_t id)
{
    gesture_fixture *fixture = context;

    log_call(fixture, CALL_CAPTURE_STOP);
    ++fixture->capture_stop_calls;
    fixture->last_capture_stop.id = id;
    if (fixture->capture_stop_error != BC_REC_OK)
        return fixture->capture_stop_error;
    fixture->capture_running = false;
    return BC_REC_OK;
}

static bool fixture_capture_abort(void *context, uint64_t id)
{
    gesture_fixture *fixture = context;

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
    gesture_fixture *fixture = context;
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
    gesture_fixture *fixture = context;

    log_call(fixture, CALL_CHANGED);
    ++fixture->changed_calls;
    fixture->last_changed = *snapshot;
}

static bc_rec_port fixture_port(gesture_fixture *fixture)
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

static bc_rec_config recording_config(void)
{
    bc_rec_config config;

    config.checkpoint_ms = 1000U;
    config.checkpoint_bytes = 1000U;
    config.stop_timeout_ms = 50U;
    return config;
}

static bc_voice_gesture_config gesture_config(void)
{
    bc_voice_gesture_config config;

    config.ptt_limit_ms = 0U;
    config.memo_limit_ms = 0U;
    config.touch_timeout_ms = 200U;
    config.tap_debounce_ms = 200U;
    config.memo_enabled = true;
    return config;
}

static bool init_recording(bc_recording *recording,
                           gesture_fixture *fixture)
{
    bc_rec_port port = fixture_port(fixture);
    bc_rec_config config = recording_config();
    return bc_recording_init(recording, &port, &config);
}

static bool init_pair(bc_recording *recording, bc_voice_gesture *gesture,
                      gesture_fixture *fixture,
                      const bc_voice_gesture_config *config)
{
    CHECK(init_recording(recording, fixture));
    return bc_voice_gesture_init(gesture, recording, config, fixture_new_id,
                                 fixture);
}

static bc_touch_report_t touch_report(bool valid, bool contact, bool hold,
                                      bool triple_tap)
{
    bc_touch_report_t report;

    memset(&report, 0, sizeof(report));
    report.valid = valid;
    report.contact = contact;
    report.hold = hold;
    report.triple_tap = triple_tap;
    return report;
}

static int find_call_after(const gesture_fixture *fixture, call_kind kind,
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

static void finish_recording(bc_recording *recording, uint64_t id,
                             uint32_t now_ms)
{
    const bc_rec_snapshot *snapshot = bc_recording_snapshot(recording);

    if (snapshot->phase == BC_REC_RECORDING)
        (void)bc_recording_stop(recording, id, now_ms);
    snapshot = bc_recording_snapshot(recording);
    if (snapshot->phase == BC_REC_STOPPING)
        (void)bc_recording_drained(recording, id);
}

static void test_hold_release_and_local_storage(void)
{
    static const uint8_t first[] = {0x10U, 0x20U, 0x30U};
    static const uint8_t tail[] = {0x41U, 0x42U, 0x43U, 0x44U};
    uint8_t standalone_storage[STORAGE_CAPACITY];
    uint8_t connected_storage[STORAGE_CAPACITY];
    uint32_t standalone_bytes;
    uint32_t connected_bytes;
    uint32_t standalone_frames;
    uint32_t connected_frames;
    unsigned mode;

    for (mode = 0U; mode < 2U; ++mode)
    {
        gesture_fixture fixture;
        bc_recording recording;
        bc_voice_gesture gesture;
        bc_voice_gesture_config config = gesture_config();
        bc_touch_report_t hold = touch_report(true, true, true, false);
        bc_touch_report_t release = touch_report(true, false, false, false);
        bc_rec_snapshot snapshot;
        uint64_t id;
        unsigned before_frame;
        int append_call;
        int live_call;

        fixture_reset(&fixture);
        CHECK(init_pair(&recording, &gesture, &fixture, &config));
        bc_recording_link(&recording, mode != 0U);
        CHECK(bc_voice_gesture_report(&gesture, &hold, 100U) == BC_REC_OK);
        snapshot = *bc_recording_snapshot(&recording);
        id = snapshot.start.id;
        CHECK(id != 0U);
        CHECK(snapshot.phase == BC_REC_RECORDING);
        CHECK(snapshot.start.trigger == BC_REC_PTT);
        CHECK(snapshot.start.duration_limit_ms == config.ptt_limit_ms);
        CHECK(fixture.new_id_calls == 1U);
        CHECK(fixture.open_calls == 1U);
        CHECK(fixture.capture_start_calls == 1U);
        CHECK(find_call_after(&fixture, CALL_OPEN, 0U) >= 0);
        CHECK(find_call_after(&fixture, CALL_CAPTURE_START, 0U) >
              find_call_after(&fixture, CALL_OPEN, 0U));

        before_frame = fixture.call_count;
        CHECK(bc_recording_frame(&recording, id, 1U, first,
                                 (uint16_t)sizeof(first), 110U) == BC_REC_OK);
        append_call = find_call_after(&fixture, CALL_APPEND, before_frame);
        live_call = find_call_after(&fixture, CALL_LIVE, before_frame);
        CHECK(append_call >= 0);
        if (mode != 0U)
        {
            CHECK(live_call > append_call);
            CHECK(fixture.live_calls == 1U);
            CHECK(fixture.lives[0].id == id);
            CHECK(fixture.lives[0].sequence == 1U);
            CHECK(fixture.lives[0].length == sizeof(first));
            CHECK(memcmp(fixture.lives[0].bytes, first, sizeof(first)) == 0);
        }
        else
        {
            CHECK(live_call < 0);
            CHECK(fixture.live_calls == 0U);
        }

        CHECK(bc_voice_gesture_report(&gesture, &hold, 120U) == BC_REC_OK);
        CHECK(fixture.new_id_calls == 1U);
        CHECK(fixture.open_calls == 1U);
        CHECK(bc_voice_gesture_report(&gesture, &release, 130U) == BC_REC_OK);
        snapshot = *bc_recording_snapshot(&recording);
        CHECK(snapshot.phase == BC_REC_STOPPING);
        CHECK(fixture.capture_stop_calls == 1U);
        CHECK(fixture.finish_calls == 0U);

        /* Explicit release only requests Stop; a final in-flight frame is
         * still accepted before the worker observes drain. */
        CHECK(bc_recording_frame(&recording, id, 2U, tail,
                                 (uint16_t)sizeof(tail), 131U) == BC_REC_OK);
        CHECK(bc_recording_drained(&recording, id) == BC_REC_OK);
        snapshot = *bc_recording_snapshot(&recording);
        CHECK(snapshot.phase == BC_REC_SAVED);
        CHECK(snapshot.error == BC_REC_OK);
        CHECK(snapshot.accepted_bytes == sizeof(first) + sizeof(tail));
        CHECK(snapshot.accepted_frames == 2U);
        CHECK(snapshot.file.bytes == snapshot.accepted_bytes);
        CHECK(snapshot.file.frames == snapshot.accepted_frames);
        CHECK(snapshot.file.complete);
        CHECK(fixture.storage_bytes == sizeof(first) + sizeof(tail));
        CHECK(memcmp(fixture.storage, first, sizeof(first)) == 0);
        CHECK(memcmp(fixture.storage + sizeof(first), tail, sizeof(tail)) == 0);

        if (mode == 0U)
        {
            memcpy(standalone_storage, fixture.storage,
                   fixture.storage_bytes);
            standalone_bytes = fixture.storage_bytes;
            standalone_frames = fixture.storage_frames;
        }
        else
        {
            memcpy(connected_storage, fixture.storage, fixture.storage_bytes);
            connected_bytes = fixture.storage_bytes;
            connected_frames = fixture.storage_frames;
        }
    }

    CHECK(standalone_bytes == connected_bytes);
    CHECK(standalone_frames == connected_frames);
    CHECK(memcmp(standalone_storage, connected_storage, standalone_bytes) == 0);
}

static void test_ptt_until_release(void)
{
    gesture_fixture fixture;
    bc_recording recording;
    bc_voice_gesture gesture;
    bc_voice_gesture_config config = gesture_config();
    bc_touch_report_t hold = touch_report(true, true, true, false);
    bc_touch_report_t release = touch_report(true, false, false, false);
    bc_touch_report_t double_tap = touch_report(true, false, false, false);
    uint32_t at;
    uint64_t id;

    fixture_reset(&fixture);
    CHECK(init_pair(&recording, &gesture, &fixture, &config));
    double_tap.double_tap = true;
    CHECK(bc_voice_gesture_report(&gesture, &double_tap, 1U) == BC_REC_OK);
    CHECK(fixture.capture_start_calls == 0U);
    CHECK(bc_voice_gesture_report(&gesture, &hold, 100U) == BC_REC_OK);
    id = bc_recording_snapshot(&recording)->start.id;
    /* Keep healthy sensor reports flowing beyond all former PTT choices. */
    for (at = 200U; at <= 120000U; at += 100U) {
        CHECK(bc_voice_gesture_report(&gesture, &hold, at) == BC_REC_OK);
        bc_recording_tick(&recording, at);
        CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    }
    CHECK(fixture.capture_stop_calls == 0U);
    CHECK(fixture.new_id_calls == 1U);
    CHECK(bc_recording_snapshot(&recording)->start.duration_limit_ms == 0U);
    CHECK(bc_voice_gesture_report(&gesture, &release, 120100U) == BC_REC_OK);
    CHECK(fixture.capture_stop_calls == 1U);
    CHECK(bc_recording_drained(&recording, id) == BC_REC_EMPTY_AUDIO);
    CHECK(bc_voice_gesture_report(&gesture, &hold, 120200U) == BC_REC_OK);
    CHECK(fixture.new_id_calls == 2U);
    finish_recording(&recording, bc_recording_snapshot(&recording)->start.id,
                     120300U);
}

static void test_triple_tap_memo_and_independent_limits(void)
{
    gesture_fixture fixture;
    bc_recording recording;
    bc_voice_gesture gesture;
    bc_voice_gesture_config config = gesture_config();
    bc_touch_report_t triple_tap = touch_report(true, false, false, true);
    bc_touch_report_t hold = touch_report(true, true, true, false);
    bc_touch_report_t release = touch_report(true, false, false, false);
    const bc_rec_snapshot *snapshot;
    uint64_t memo_id;

    config.ptt_limit_ms = 0U;
    config.memo_limit_ms = 222U;
    config.tap_debounce_ms = 300U;
    fixture_reset(&fixture);
    CHECK(init_pair(&recording, &gesture, &fixture, &config));

    CHECK(bc_voice_gesture_report(&gesture, &triple_tap, 100U) == BC_REC_OK);
    snapshot = bc_recording_snapshot(&recording);
    memo_id = snapshot->start.id;
    CHECK(snapshot->phase == BC_REC_RECORDING);
    CHECK(snapshot->start.trigger == BC_REC_MEMO);
    CHECK(snapshot->start.duration_limit_ms == config.memo_limit_ms);
    CHECK(fixture.new_id_calls == 1U);
    CHECK(fixture.open_calls == 1U);

    CHECK(bc_voice_gesture_report(&gesture, &triple_tap, 200U) ==
          BC_REC_DUPLICATE);
    CHECK(fixture.new_id_calls == 1U);
    CHECK(fixture.capture_stop_calls == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);

    CHECK(bc_voice_gesture_report(&gesture, &triple_tap, 401U) == BC_REC_OK);
    CHECK(fixture.capture_stop_calls == 1U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    CHECK(bc_recording_drained(&recording, memo_id) == BC_REC_EMPTY_AUDIO);

    /* PTT and memo duration settings are independent. */
    CHECK(bc_voice_gesture_report(&gesture, &hold, 500U) == BC_REC_OK);
    snapshot = bc_recording_snapshot(&recording);
    CHECK(snapshot->start.trigger == BC_REC_PTT);
    CHECK(snapshot->start.duration_limit_ms == config.ptt_limit_ms);
    CHECK(bc_voice_gesture_report(&gesture, &release, 510U) == BC_REC_OK);
    finish_recording(&recording, snapshot->start.id, 511U);

    /* Disabled memo is rejected without an ID or storage side effect. */
    fixture_reset(&fixture);
    config.memo_enabled = false;
    CHECK(init_pair(&recording, &gesture, &fixture, &config));
    CHECK(bc_voice_gesture_report(&gesture, &triple_tap, 600U) ==
          BC_REC_OK);
    CHECK(fixture.new_id_calls == 0U);
    CHECK(fixture.open_calls == 0U);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_IDLE);
}

static void test_stale_release_does_not_stop_foreign_sessions(void)
{
    static const bc_rec_trigger triggers[] = {BC_REC_PTT, BC_REC_APP};
    unsigned i;

    for (i = 0U; i < ARRAY_LEN(triggers); ++i)
    {
        gesture_fixture fixture;
        bc_recording recording;
        bc_voice_gesture gesture;
        bc_voice_gesture_config config = gesture_config();
        bc_touch_report_t hold = touch_report(true, true, true, false);
        bc_touch_report_t release = touch_report(true, false, false, false);
        bc_rec_start foreign;
        uint64_t own_id;
        unsigned stops;

        fixture_reset(&fixture);
        CHECK(init_pair(&recording, &gesture, &fixture, &config));
        CHECK(bc_voice_gesture_report(&gesture, &hold, 10U) == BC_REC_OK);
        own_id = bc_recording_snapshot(&recording)->start.id;
        CHECK(own_id != 0U);
        CHECK(bc_recording_stop(&recording, own_id, 20U) == BC_REC_OK);
        CHECK(bc_recording_drained(&recording, own_id) ==
              BC_REC_EMPTY_AUDIO);

        foreign.id = 0x9000U + i;
        foreign.trigger = triggers[i];
        foreign.duration_limit_ms = 0U;
        CHECK(bc_recording_start(&recording, &foreign, 30U) == BC_REC_OK);
        CHECK(bc_recording_snapshot(&recording)->start.id == foreign.id);
        stops = fixture.capture_stop_calls;
        CHECK(bc_voice_gesture_report(&gesture, &release, 31U) == BC_REC_OK);
        CHECK(fixture.capture_stop_calls == stops);
        CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
        finish_recording(&recording, foreign.id, 40U);
    }
}

static void test_invalid_combinations_have_no_effect(void)
{
    gesture_fixture fixture;
    bc_recording recording;
    bc_voice_gesture gesture;
    bc_voice_gesture_config config = gesture_config();
    bc_touch_report_t hold_triple = touch_report(true, true, true, true);
    bc_touch_report_t hold_without_contact = touch_report(true, false, true,
                                                           false);
    const bc_rec_snapshot *snapshot;

    fixture_reset(&fixture);
    CHECK(init_pair(&recording, &gesture, &fixture, &config));
    CHECK(bc_voice_gesture_report(&gesture, &hold_triple, 10U) ==
          BC_REC_INVALID);
    CHECK(bc_voice_gesture_report(&gesture, &hold_without_contact, 11U) ==
          BC_REC_INVALID);
    snapshot = bc_recording_snapshot(&recording);
    CHECK(snapshot->phase == BC_REC_IDLE);
    CHECK(snapshot->error == BC_REC_OK);
    CHECK(fixture.new_id_calls == 0U);
    CHECK(fixture.open_calls == 0U);
    CHECK(fixture.capture_start_calls == 0U);
    CHECK(fixture.capture_stop_calls == 0U);
    CHECK(fixture.finish_calls == 0U);
}

static void test_hold_stops_memo_without_restarting(void)
{
    static const uint8_t audio[] = {0x11U, 0x22U};
    gesture_fixture fixture;
    bc_recording recording;
    bc_voice_gesture gesture;
    bc_voice_gesture_config config = gesture_config();
    bc_touch_report_t triple_tap = touch_report(true, false, false, true);
    bc_touch_report_t hold = touch_report(true, true, true, false);
    bc_touch_report_t release = touch_report(true, false, false, false);
    uint64_t id;

    fixture_reset(&fixture);
    CHECK(init_pair(&recording, &gesture, &fixture, &config));
    CHECK(bc_voice_gesture_report(&gesture, &triple_tap, 10U) == BC_REC_OK);
    id = bc_recording_snapshot(&recording)->start.id;
    CHECK(bc_recording_snapshot(&recording)->start.trigger == BC_REC_MEMO);
    CHECK(bc_recording_frame(&recording, id, 1U, audio, sizeof(audio), 20U) == BC_REC_OK);
    CHECK(bc_voice_gesture_report(&gesture, &hold, 30U) == BC_REC_OK);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
    CHECK(fixture.capture_stop_calls == 1U);
    CHECK(fixture.last_capture_stop.id == id);
    CHECK(fixture.new_id_calls == 1U);
    CHECK(bc_voice_gesture_report(&gesture, &hold, 31U) == BC_REC_OK);
    CHECK(fixture.capture_stop_calls == 1U);
    /* The escape uses the normal stop path, preserving the accepted tail. */
    CHECK(bc_recording_frame(&recording, id, 2U, audio, sizeof(audio), 32U) == BC_REC_OK);
    CHECK(bc_recording_drained(&recording, id) == BC_REC_OK);
    CHECK(fixture.last_finish_complete);
    CHECK(fixture.last_finish_file.bytes == 2U * sizeof(audio));
    CHECK(bc_voice_gesture_report(&gesture, &hold, 33U) == BC_REC_OK);
    CHECK(fixture.new_id_calls == 1U);
    CHECK(fixture.capture_start_calls == 1U);
    CHECK(bc_voice_gesture_report(&gesture, &release, 34U) == BC_REC_OK);
    CHECK(fixture.new_id_calls == 1U);
    CHECK(!bc_recording_active(&recording));
    CHECK(bc_voice_gesture_report(&gesture, &hold, 35U) == BC_REC_OK);
    CHECK(fixture.new_id_calls == 2U);
    CHECK(bc_recording_snapshot(&recording)->start.trigger == BC_REC_PTT);
    CHECK(bc_recording_snapshot(&recording)->start.id != id);
    CHECK(bc_voice_gesture_report(&gesture, &release, 36U) == BC_REC_OK);
    finish_recording(&recording, bc_recording_snapshot(&recording)->start.id, 37U);
}

static void test_hold_busy_with_foreign_owner(void)
{
    gesture_fixture fixture;
    bc_recording recording;
    bc_voice_gesture gesture;
    bc_voice_gesture_config config = gesture_config();
    bc_touch_report_t hold = touch_report(true, true, true, false);
    bc_touch_report_t release = touch_report(true, false, false, false);
    bc_rec_start foreign = {0x8800U, BC_REC_APP, 0U};
    unsigned ids;
    unsigned opens;
    unsigned stops;

    fixture_reset(&fixture);
    CHECK(init_pair(&recording, &gesture, &fixture, &config));
    CHECK(bc_recording_start(&recording, &foreign, 10U) == BC_REC_OK);
    ids = fixture.new_id_calls;
    opens = fixture.open_calls;
    stops = fixture.capture_stop_calls;
    CHECK(bc_voice_gesture_report(&gesture, &hold, 11U) == BC_REC_BUSY);
    CHECK(fixture.new_id_calls == ids);
    CHECK(fixture.open_calls == opens);
    CHECK(fixture.capture_stop_calls == stops);
    CHECK(bc_voice_gesture_report(&gesture, &hold, 12U) == BC_REC_OK);
    CHECK(fixture.new_id_calls == ids);
    CHECK(fixture.open_calls == opens);
    CHECK(bc_voice_gesture_report(&gesture, &release, 13U) == BC_REC_OK);
    CHECK(fixture.capture_stop_calls == stops);
    finish_recording(&recording, foreign.id, 14U);
}

static void test_invalid_sensor_and_lease_timeout_are_partial(void)
{
    gesture_fixture fixture;
    bc_recording recording;
    bc_voice_gesture gesture;
    bc_voice_gesture_config config = gesture_config();
    bc_touch_report_t hold = touch_report(true, true, true, false);
    bc_touch_report_t invalid_reset = touch_report(false, false, false, false);
    bc_touch_report_t late_valid = hold;
    const bc_rec_snapshot *snapshot;
    uint64_t id;

    config.touch_timeout_ms = 100U;
    fixture_reset(&fixture);
    CHECK(init_pair(&recording, &gesture, &fixture, &config));
    CHECK(bc_voice_gesture_report(&gesture, &hold, 10U) == BC_REC_OK);
    id = bc_recording_snapshot(&recording)->start.id;
    invalid_reset.reset_flags = BC_TOUCH_REPORT_INFO_RESET;
    CHECK(bc_voice_gesture_report(&gesture, &invalid_reset, 20U) ==
          BC_REC_TOUCH_ERROR);
    snapshot = bc_recording_snapshot(&recording);
    CHECK(snapshot->phase == BC_REC_STOPPING);
    CHECK(snapshot->error == BC_REC_TOUCH_ERROR);
    CHECK(fixture.capture_stop_calls == 1U);
    CHECK(bc_voice_gesture_report(&gesture, &late_valid, 21U) == BC_REC_OK);
    CHECK(fixture.new_id_calls == 1U);
    CHECK(fixture.capture_stop_calls == 1U);
    CHECK(bc_recording_snapshot(&recording)->error == BC_REC_TOUCH_ERROR);
    CHECK(bc_recording_drained(&recording, id) == BC_REC_TOUCH_ERROR);
    snapshot = bc_recording_snapshot(&recording);
    CHECK(snapshot->phase == BC_REC_PARTIAL);
    CHECK(!snapshot->file.complete);
    CHECK(!fixture.last_finish_complete);

    fixture_reset(&fixture);
    CHECK(init_pair(&recording, &gesture, &fixture, &config));
    CHECK(bc_voice_gesture_report(&gesture, &hold, UINT32_MAX - 20U) ==
          BC_REC_OK);
    id = bc_recording_snapshot(&recording)->start.id;
    bc_voice_gesture_tick(&gesture, 78U); /* 99 ms over uint32 wrap. */
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    bc_voice_gesture_tick(&gesture, 79U); /* Exactly 100 ms. */
    snapshot = bc_recording_snapshot(&recording);
    CHECK(snapshot->phase == BC_REC_STOPPING);
    CHECK(snapshot->error == BC_REC_TOUCH_ERROR);
    CHECK(fixture.capture_stop_calls == 1U);
    CHECK(bc_voice_gesture_report(&gesture, &late_valid, 80U) == BC_REC_OK);
    CHECK(fixture.new_id_calls == 1U);
    CHECK(fixture.open_calls == 1U);
    CHECK(bc_recording_drained(&recording, id) == BC_REC_TOUCH_ERROR);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
}

static void test_fault_unlocks_config_without_rearming_hold(void)
{
    unsigned fault;
    for (fault = 0; fault < 2; ++fault) {
        gesture_fixture fixture;
        bc_recording recording;
        bc_voice_gesture gesture;
        bc_voice_gesture_config config = gesture_config();
        bc_touch_report_t hold = touch_report(true, true, true, false);
        bc_touch_report_t release = touch_report(true, false, false, false);
        bc_touch_report_t invalid = {0};
        bc_voice_inputs inputs;
        uint64_t id;
        uint32_t now = 10U + config.touch_timeout_ms;
        fixture_reset(&fixture);
        CHECK(init_pair(&recording, &gesture, &fixture, &config));
        CHECK(bc_voice_gesture_report(&gesture, &hold, 10U) == BC_REC_OK);
        id = bc_recording_snapshot(&recording)->start.id;
        if (fault) CHECK(bc_voice_gesture_report(&gesture, &invalid, 20U) == BC_REC_TOUCH_ERROR);
        else bc_voice_gesture_tick(&gesture, now);
        CHECK(bc_voice_gesture_configure(&gesture, &config) == BC_REC_BUSY);
        CHECK(bc_recording_drained(&recording, id) == BC_REC_TOUCH_ERROR);
        CHECK(!gesture.contact_active && !gesture.hold_attempted);
        CHECK(bc_voice_gesture_configure(&gesture, &config) == BC_REC_OK);
        inputs = gesture.inputs;
        CHECK(bc_voice_gesture_set_inputs(&gesture, &inputs) == BC_REC_OK);
        CHECK(bc_voice_gesture_report(&gesture, &hold, now + 1U) == BC_REC_OK);
        CHECK(fixture.new_id_calls == 1U);
        CHECK(bc_voice_gesture_set_inputs(&gesture, &inputs) == BC_REC_OK);
        CHECK(bc_voice_gesture_report(&gesture, &release, now + 2U) == BC_REC_OK);
        CHECK(bc_voice_gesture_report(&gesture, &hold, now + 3U) == BC_REC_OK);
        CHECK(fixture.new_id_calls == 2U);
    }
}

static void test_invalid_touch_does_not_cancel_memo(void)
{
    gesture_fixture fixture;
    bc_recording recording;
    bc_voice_gesture gesture;
    bc_voice_gesture_config config = gesture_config();
    bc_touch_report_t triple_tap = touch_report(true, false, false, true);
    bc_touch_report_t invalid = touch_report(false, false, false, false);
    const bc_rec_snapshot *snapshot;
    uint64_t memo_id;

    fixture_reset(&fixture);
    CHECK(init_pair(&recording, &gesture, &fixture, &config));
    CHECK(bc_voice_gesture_report(&gesture, &triple_tap, 10U) == BC_REC_OK);
    memo_id = bc_recording_snapshot(&recording)->start.id;
    invalid.error_flags = BC_TOUCH_REPORT_INFO_ATI_ERROR;
    CHECK(bc_voice_gesture_report(&gesture, &invalid, 20U) ==
          BC_REC_TOUCH_ERROR);
    snapshot = bc_recording_snapshot(&recording);
    CHECK(snapshot->phase == BC_REC_RECORDING);
    CHECK(snapshot->error == BC_REC_OK);
    CHECK(fixture.capture_stop_calls == 0U);
    CHECK(fixture.finish_calls == 0U);
    finish_recording(&recording, memo_id, 30U);
}

static void test_failed_hold_attempts_wait_for_release(void)
{
    bc_touch_report_t hold = touch_report(true, true, true, false);
    bc_touch_report_t release = touch_report(true, false, false, false);

    /* An ID failure is one attempt for the entire physical hold. */
    {
        gesture_fixture fixture;
        bc_recording recording;
        bc_voice_gesture gesture;
        bc_voice_gesture_config config = gesture_config();

        fixture_reset(&fixture);
        fixture.id_error = BC_REC_INVALID;
        CHECK(init_pair(&recording, &gesture, &fixture, &config));
        CHECK(bc_voice_gesture_report(&gesture, &hold, 10U) == BC_REC_INVALID);
        CHECK(bc_voice_gesture_report(&gesture, &hold, 11U) == BC_REC_OK);
        CHECK(fixture.new_id_calls == 1U);
        CHECK(fixture.open_calls == 0U);
        CHECK(bc_voice_gesture_configure(&gesture, &config) == BC_REC_BUSY);
        CHECK(bc_voice_gesture_report(&gesture, &release, 12U) == BC_REC_OK);
        fixture.id_error = BC_REC_OK;
        CHECK(bc_voice_gesture_report(&gesture, &hold, 13U) == BC_REC_OK);
        CHECK(fixture.new_id_calls == 2U);
        CHECK(fixture.open_calls == 1U);
        finish_recording(&recording, bc_recording_snapshot(&recording)->start.id,
                         14U);
    }

    /* A successful callback that returns the reserved zero ID is invalid and
     * is subject to the same release gate. */
    {
        gesture_fixture fixture;
        bc_recording recording;
        bc_voice_gesture gesture;
        bc_voice_gesture_config config = gesture_config();

        fixture_reset(&fixture);
        fixture.return_zero_id = true;
        CHECK(init_pair(&recording, &gesture, &fixture, &config));
        CHECK(bc_voice_gesture_report(&gesture, &hold, 15U) == BC_REC_INVALID);
        CHECK(bc_voice_gesture_report(&gesture, &hold, 16U) == BC_REC_OK);
        CHECK(fixture.new_id_calls == 1U);
        CHECK(fixture.open_calls == 0U);
        CHECK(bc_voice_gesture_report(&gesture, &release, 17U) == BC_REC_OK);
        fixture.return_zero_id = false;
        CHECK(bc_voice_gesture_report(&gesture, &hold, 18U) == BC_REC_OK);
        CHECK(fixture.new_id_calls == 2U);
        CHECK(fixture.open_calls == 1U);
        finish_recording(&recording, bc_recording_snapshot(&recording)->start.id,
                         19U);
    }

    /* A storage/open Start failure is also not retried while held. */
    {
        gesture_fixture fixture;
        bc_recording recording;
        bc_voice_gesture gesture;
        bc_voice_gesture_config config = gesture_config();

        fixture_reset(&fixture);
        fixture.open_error = BC_REC_NO_SPACE;
        CHECK(init_pair(&recording, &gesture, &fixture, &config));
        CHECK(bc_voice_gesture_report(&gesture, &hold, 20U) == BC_REC_NO_SPACE);
        CHECK(bc_voice_gesture_report(&gesture, &hold, 21U) == BC_REC_OK);
        CHECK(fixture.new_id_calls == 1U);
        CHECK(fixture.open_calls == 1U);
        CHECK(fixture.capture_start_calls == 0U);
        CHECK(fixture.finish_calls == 0U);
        CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_FAILED);
        CHECK(bc_recording_snapshot(&recording)->error == BC_REC_NO_SPACE);
        CHECK(!bc_recording_snapshot(&recording)->file.complete);
        CHECK(bc_voice_gesture_report(&gesture, &release, 22U) == BC_REC_OK);
        fixture.open_error = BC_REC_OK;
        CHECK(bc_voice_gesture_report(&gesture, &hold, 23U) == BC_REC_OK);
        CHECK(fixture.new_id_calls == 2U);
        CHECK(fixture.open_calls == 2U);
        finish_recording(&recording, bc_recording_snapshot(&recording)->start.id,
                         24U);
    }

    /* Capture-start failure must release the destination, then wait for a
     * physical release before another attempt. */
    {
        gesture_fixture fixture;
        bc_recording recording;
        bc_voice_gesture gesture;
        bc_voice_gesture_config config = gesture_config();

        fixture_reset(&fixture);
        fixture.capture_start_error = BC_REC_CAPTURE_ERROR;
        CHECK(init_pair(&recording, &gesture, &fixture, &config));
        CHECK(bc_voice_gesture_report(&gesture, &hold, 30U) ==
              BC_REC_CAPTURE_ERROR);
        CHECK(bc_voice_gesture_report(&gesture, &hold, 31U) == BC_REC_OK);
        CHECK(fixture.new_id_calls == 1U);
        CHECK(fixture.open_calls == 1U);
        CHECK(fixture.capture_start_calls == 1U);
        CHECK(fixture.finish_calls == 1U);
        CHECK(bc_voice_gesture_report(&gesture, &release, 32U) == BC_REC_OK);
        fixture.capture_start_error = BC_REC_OK;
        CHECK(bc_voice_gesture_report(&gesture, &hold, 33U) == BC_REC_OK);
        CHECK(fixture.new_id_calls == 2U);
        CHECK(fixture.open_calls == 2U);
        finish_recording(&recording, bc_recording_snapshot(&recording)->start.id,
                         34U);
    }

    /* No-space during append faults the PTT, and repeated holds do not create
     * another ID or file until release and drain. */
    {
        static const uint8_t data[] = {0x55U, 0x66U};
        gesture_fixture fixture;
        bc_recording recording;
        bc_voice_gesture gesture;
        bc_voice_gesture_config config = gesture_config();
        uint64_t id;

        fixture_reset(&fixture);
        fixture.append_error = BC_REC_NO_SPACE;
        fixture.append_error_call = 1U;
        CHECK(init_pair(&recording, &gesture, &fixture, &config));
        CHECK(bc_voice_gesture_report(&gesture, &hold, 40U) == BC_REC_OK);
        id = bc_recording_snapshot(&recording)->start.id;
        CHECK(bc_recording_frame(&recording, id, 1U, data,
                                 (uint16_t)sizeof(data), 41U) == BC_REC_NO_SPACE);
        CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_STOPPING);
        CHECK(bc_recording_snapshot(&recording)->error == BC_REC_NO_SPACE);
        CHECK(fixture.capture_stop_calls == 1U);
        CHECK(bc_voice_gesture_report(&gesture, &hold, 42U) == BC_REC_OK);
        CHECK(fixture.new_id_calls == 1U);
        CHECK(fixture.open_calls == 1U);
        CHECK(bc_voice_gesture_report(&gesture, &release, 43U) ==
              BC_REC_NO_SPACE);
        CHECK(fixture.new_id_calls == 1U);
        CHECK(bc_recording_drained(&recording, id) == BC_REC_NO_SPACE);
        CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_PARTIAL);
        CHECK(!bc_recording_snapshot(&recording)->file.complete);
        fixture.append_error = BC_REC_OK;
        CHECK(bc_voice_gesture_report(&gesture, &hold, 44U) == BC_REC_OK);
        CHECK(fixture.new_id_calls == 2U);
        CHECK(fixture.open_calls == 2U);
        finish_recording(&recording, bc_recording_snapshot(&recording)->start.id,
                         45U);
    }
}

static void test_configuration_validation_and_busy(void)
{
    gesture_fixture fixture;
    bc_recording recording;
    bc_voice_gesture gesture;
    bc_voice_gesture_config config = gesture_config();
    bc_voice_gesture_config invalid;
    bc_touch_report_t hold = touch_report(true, true, true, false);
    bc_touch_report_t release = touch_report(true, false, false, false);

    fixture_reset(&fixture);
    CHECK(init_recording(&recording, &fixture));
    CHECK(bc_voice_gesture_init(&gesture, &recording, &config, fixture_new_id,
                                &fixture));

    invalid = config;
    invalid.ptt_limit_ms = (uint32_t)BC_REC_MAX_INTERVAL + 1U;
    CHECK(bc_voice_gesture_configure(&gesture, &invalid) == BC_REC_INVALID);
    invalid = config;
    invalid.memo_limit_ms = (uint32_t)BC_REC_MAX_INTERVAL + 1U;
    CHECK(bc_voice_gesture_configure(&gesture, &invalid) == BC_REC_INVALID);
    invalid = config;
    invalid.touch_timeout_ms = 99U;
    CHECK(bc_voice_gesture_configure(&gesture, &invalid) == BC_REC_INVALID);
    invalid.touch_timeout_ms = 751U;
    CHECK(bc_voice_gesture_configure(&gesture, &invalid) == BC_REC_INVALID);
    invalid = config;
    invalid.tap_debounce_ms = 99U;
    CHECK(bc_voice_gesture_configure(&gesture, &invalid) == BC_REC_INVALID);
    invalid.tap_debounce_ms = 501U;
    CHECK(bc_voice_gesture_configure(&gesture, &invalid) == BC_REC_INVALID);
    CHECK(bc_voice_gesture_configure(&gesture, NULL) == BC_REC_INVALID);
    CHECK(bc_voice_gesture_init(NULL, &recording, &config, fixture_new_id,
                                &fixture) == false);
    CHECK(bc_voice_gesture_init(&gesture, NULL, &config, fixture_new_id,
                                &fixture) == false);
    CHECK(bc_voice_gesture_init(&gesture, &recording, &config, NULL,
                                &fixture) == false);
    invalid = config;
    invalid.touch_timeout_ms = 99U;
    CHECK(bc_voice_gesture_init(&gesture, &recording, &invalid, fixture_new_id,
                                &fixture) == false);
    CHECK(bc_voice_gesture_init(&gesture, &recording, NULL, fixture_new_id,
                                &fixture) == false);

    invalid = config;
    invalid.ptt_limit_ms = 0U;
    invalid.memo_limit_ms = (uint32_t)BC_REC_MAX_INTERVAL;
    invalid.touch_timeout_ms = 100U;
    invalid.tap_debounce_ms = 100U;
    CHECK(bc_voice_gesture_configure(&gesture, &invalid) == BC_REC_OK);
    invalid.touch_timeout_ms = 750U;
    invalid.tap_debounce_ms = 500U;
    CHECK(bc_voice_gesture_configure(&gesture, &invalid) == BC_REC_OK);

    fixture_reset(&fixture);
    config = gesture_config();
    CHECK(init_pair(&recording, &gesture, &fixture, &config));
    CHECK(bc_voice_gesture_report(&gesture, &hold, 100U) == BC_REC_OK);
    invalid = config;
    invalid.memo_limit_ms = 123U;
    CHECK(bc_voice_gesture_configure(&gesture, &invalid) == BC_REC_BUSY);
    CHECK(bc_voice_gesture_report(&gesture, &release, 110U) == BC_REC_OK);
    CHECK(bc_voice_gesture_configure(&gesture, &invalid) == BC_REC_BUSY);
    finish_recording(&recording, bc_recording_snapshot(&recording)->start.id,
                     111U);
    CHECK(bc_voice_gesture_configure(&gesture, &invalid) == BC_REC_OK);
    CHECK(bc_voice_gesture_report(NULL, &hold, 120U) == BC_REC_INVALID);
    CHECK(bc_voice_gesture_report(&gesture, NULL, 120U) == BC_REC_INVALID);
    bc_voice_gesture_tick(NULL, 120U);
}

typedef struct { unsigned count; uint8_t input[8], phase[8]; } input_log;
static bool capture_input(void *ctx, uint8_t input, uint8_t phase)
{
    input_log *log = ctx;
    if (log->count >= 8U) return false;
    log->input[log->count] = input; log->phase[log->count++] = phase;
    return true;
}
static void test_three_input_mappings(void)
{
    gesture_fixture fixture;
    bc_recording recording;
    bc_voice_gesture gesture;
    bc_voice_gesture_config config = gesture_config();
    bc_voice_inputs inputs = {5000U, BC_VOICE_INPUT_APP, BC_VOICE_INPUT_MEMO, BC_VOICE_INPUT_DISABLED};
    bc_touch_report_t hold = touch_report(true, true, true, false);
    bc_touch_report_t release = touch_report(true, false, false, false);
    bc_touch_report_t tap = release;
    bc_touch_report_t invalid = {0};
    input_log log = {0};
    uint64_t id;
    fixture_reset(&fixture);
    config.memo_limit_ms = 0U;
    CHECK(init_pair(&recording, &gesture, &fixture, &config));
    CHECK(bc_voice_gesture_set_inputs(&gesture, &inputs) == BC_REC_OK);
    bc_voice_gesture_set_event_handler(&gesture, capture_input, &log);
    tap.double_tap = true;
    CHECK(bc_voice_gesture_report(&gesture, &tap, 10U) == BC_REC_OK);
    id = bc_recording_snapshot(&recording)->start.id;
    CHECK(bc_recording_snapshot(&recording)->start.trigger == BC_REC_MEMO);
    tap.double_tap = false; tap.triple_tap = true;
    CHECK(bc_voice_gesture_report(&gesture, &tap, 20U) == BC_REC_OK);
    CHECK(bc_recording_snapshot(&recording)->phase == BC_REC_RECORDING);
    tap.double_tap = true; tap.triple_tap = false;
    CHECK(bc_voice_gesture_report(&gesture, &tap, 400U) == BC_REC_OK);
    CHECK(bc_recording_drained(&recording, id) == BC_REC_EMPTY_AUDIO);
    CHECK(bc_voice_gesture_report(&gesture, &hold, 500U) == BC_REC_OK);
    CHECK(bc_voice_gesture_report(&gesture, &hold, 510U) == BC_REC_OK);
    CHECK(log.count == 1U && log.input[0] == BC_VOICE_INPUT_HOLD && log.phase[0] == BC_VOICE_INPUT_ACTIVATED);
    CHECK(bc_voice_gesture_set_inputs(&gesture, &inputs) == BC_REC_BUSY);
    CHECK(bc_voice_gesture_report(&gesture, &release, 520U) == BC_REC_OK);
    CHECK(log.count == 2U && log.phase[1] == BC_VOICE_INPUT_RELEASED);
    CHECK(bc_voice_gesture_report(&gesture, &hold, 600U) == BC_REC_OK);
    CHECK(bc_voice_gesture_report(&gesture, &invalid, 610U) == BC_REC_TOUCH_ERROR);
    CHECK(log.count == 4U && log.phase[3] == BC_VOICE_INPUT_CANCELLED);
    CHECK(bc_voice_gesture_report(&gesture, &release, 620U) == BC_REC_OK);
    CHECK(log.count == 4U);
    inputs.hold_action = BC_VOICE_INPUT_DISABLED;
    inputs.double_action = BC_VOICE_INPUT_DISABLED;
    inputs.triple_action = BC_VOICE_INPUT_APP;
    CHECK(bc_voice_gesture_set_inputs(&gesture, &inputs) == BC_REC_OK);
    tap.double_tap = false; tap.triple_tap = true;
    CHECK(bc_voice_gesture_report(&gesture, &tap, 700U) == BC_REC_OK);
    CHECK(log.count == 5U && log.input[4] == BC_VOICE_INPUT_TRIPLE);
    CHECK(bc_voice_gesture_report(&gesture, &hold, 710U) == BC_REC_OK);
    CHECK(log.count == 5U && fixture.capture_start_calls == 1U);
    CHECK(bc_voice_gesture_report(&gesture, &release, 720U) == BC_REC_OK);
    /* A separately queued tap must not forget an active SDK hold. */
    inputs.hold_action = BC_VOICE_INPUT_APP;
    CHECK(bc_voice_gesture_set_inputs(&gesture, &inputs) == BC_REC_OK);
    log.count = 0U;
    CHECK(bc_voice_gesture_report(&gesture, &hold, 730U) == BC_REC_OK);
    tap.contact = true;
    CHECK(bc_voice_gesture_report(&gesture, &tap, 920U) == BC_REC_OK);
    CHECK(bc_voice_gesture_report(&gesture, &release, 930U) == BC_REC_OK);
    CHECK(log.count == 3U && log.phase[2] == BC_VOICE_INPUT_RELEASED);
    CHECK(bc_voice_gesture_report(&gesture, &hold, 940U) == BC_REC_OK);
    bc_voice_gesture_tick(&gesture, 1140U);
    CHECK(log.count == 5U && log.phase[4] == BC_VOICE_INPUT_CANCELLED);
    CHECK(bc_voice_gesture_report(&gesture, &release, 1150U) == BC_REC_OK);
    inputs.double_action = BC_VOICE_INPUT_PTT;
    CHECK(bc_voice_gesture_set_inputs(&gesture, &inputs) == BC_REC_INVALID);
    inputs.double_action = BC_VOICE_INPUT_DISABLED; inputs.hold_ms = 499U;
    CHECK(bc_voice_gesture_set_inputs(&gesture, &inputs) == BC_REC_INVALID);
    inputs.hold_ms = 10001U;
    CHECK(bc_voice_gesture_set_inputs(&gesture, &inputs) == BC_REC_INVALID);
    inputs.hold_ms = 1000U;
    hold.hold = false;
    CHECK(bc_voice_gesture_report(&gesture, &hold, 1820U) == BC_REC_OK);
    CHECK(bc_voice_gesture_set_inputs(&gesture, &inputs) == BC_REC_BUSY);
}

int main(void)
{
    test_hold_release_and_local_storage();
    test_ptt_until_release();
    test_three_input_mappings();
    test_triple_tap_memo_and_independent_limits();
    test_stale_release_does_not_stop_foreign_sessions();
    test_invalid_combinations_have_no_effect();
    test_hold_stops_memo_without_restarting();
    test_hold_busy_with_foreign_owner();
    test_invalid_sensor_and_lease_timeout_are_partial();
    test_fault_unlocks_config_without_rearming_hold();
    test_invalid_touch_does_not_cancel_memo();
    test_failed_hold_attempts_wait_for_release();
    test_configuration_validation_and_busy();

    if (failures != 0U)
    {
        fprintf(stderr, "FAIL: %u of %u checks\n", failures, checks);
        return 1;
    }
    printf("PASS: %u checks (hold/release, memo, lease timeout, stale sessions,\n"
           "      retry gates, configuration and local storage)\n",
           checks);
    return 0;
}
