#include "bc_voice_service.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FLASH_BLOCK_SIZE 256U
#define FLASH_BLOCK_COUNT 128U
#define FLASH_SIZE (FLASH_BLOCK_SIZE * FLASH_BLOCK_COUNT)
#define TEST_CACHE_SIZE 64U
#define TEST_LOOKAHEAD_SIZE (FLASH_BLOCK_COUNT / 8U)
#define MAX_MESSAGES 384U
#define MAX_CALLS 512U
#define MAX_PACKET_COUNT 4096U

typedef struct {
    uint8_t bytes[FLASH_SIZE];
    unsigned read_calls;
    unsigned prog_calls;
    unsigned erase_calls;
    unsigned sync_calls;
    unsigned fail_prog_call;
    unsigned fail_erase_call;
    unsigned fail_sync_call;
} ram_nor;

typedef struct {
    lfs_t lfs;
    struct lfs_config config;
    uint8_t read_cache[TEST_CACHE_SIZE];
    uint8_t prog_cache[TEST_CACHE_SIZE];
    uint8_t lookahead[TEST_LOOKAHEAD_SIZE];
} test_fs;

typedef struct {
    bc_voice_receiver receiver;
    bc_voice_message messages[MAX_MESSAGES];
    unsigned message_count;
    unsigned packet_count;
    unsigned invalid_packets;
    unsigned fail_packet;
    bool fail_next;
    bool fail_all;
} sink;

typedef struct {
    ram_nor ram;
    test_fs fs;
    bc_rec_store store;
    bc_recording recording;
    bc_voice_gesture gesture;
    bc_voice_service service;
    sink sink;
    char calls[MAX_CALLS];
    unsigned call_count;
    unsigned open_count;
    unsigned append_count;
    unsigned checkpoint_count;
    unsigned finish_count;
    unsigned capture_start_count;
    unsigned capture_stop_count;
    unsigned capture_abort_count;
    unsigned settings_count;
    unsigned live_count;
    uint16_t next_command_message;
    uint64_t next_gesture_id;
    bc_rec_result capture_start_result;
    bc_rec_result capture_stop_result;
    bc_rec_result settings_result;
    bc_voice_tuning tuning;
    uint8_t touch_status;
    unsigned tuning_writes;
    bc_rec_result tuning_result;
    bool capture_abort_quiescent;
    unsigned outcome_calls;
    uint64_t outcome_recording_id;
    uint8_t outcome_value;
} fixture;

static unsigned checks;
static unsigned failures;

static void check_condition(bool condition, const char *expression,
                            unsigned line)
{
    ++checks;
    if (!condition) {
        ++failures;
        fprintf(stderr, "FAIL line %u: %s\n", line, expression);
    }
}

#define CHECK(condition) check_condition((condition), #condition, __LINE__)

static int ram_read(const struct lfs_config *config, lfs_block_t block,
                    lfs_off_t off, void *buffer, lfs_size_t size)
{
    ram_nor *ram = (ram_nor *)config->context;
    uint64_t address = (uint64_t)block * config->block_size + off;
    ++ram->read_calls;
    if (buffer == NULL || block >= FLASH_BLOCK_COUNT ||
        off > config->block_size || size > config->block_size - off ||
        address + size > FLASH_SIZE)
        return LFS_ERR_INVAL;
    memcpy(buffer, ram->bytes + address, size);
    return LFS_ERR_OK;
}

static int ram_prog(const struct lfs_config *config, lfs_block_t block,
                    lfs_off_t off, const void *buffer, lfs_size_t size)
{
    ram_nor *ram = (ram_nor *)config->context;
    const uint8_t *source = (const uint8_t *)buffer;
    uint64_t address = (uint64_t)block * config->block_size + off;
    lfs_size_t i;

    ++ram->prog_calls;
    if (ram->fail_prog_call != 0U &&
        ram->prog_calls == ram->fail_prog_call) {
        ram->fail_prog_call = 0U;
        return LFS_ERR_IO;
    }
    if (buffer == NULL || block >= FLASH_BLOCK_COUNT ||
        off > config->block_size || size > config->block_size - off ||
        address + size > FLASH_SIZE || off % config->prog_size != 0U ||
        size % config->prog_size != 0U)
        return LFS_ERR_INVAL;
    for (i = 0U; i < size; ++i) {
        if ((ram->bytes[address + i] & source[i]) != source[i])
            return LFS_ERR_CORRUPT;
    }
    for (i = 0U; i < size; ++i)
        ram->bytes[address + i] &= source[i];
    return LFS_ERR_OK;
}

static int ram_erase(const struct lfs_config *config, lfs_block_t block)
{
    ram_nor *ram = (ram_nor *)config->context;
    ++ram->erase_calls;
    if (ram->fail_erase_call != 0U &&
        ram->erase_calls == ram->fail_erase_call) {
        ram->fail_erase_call = 0U;
        return LFS_ERR_IO;
    }
    if (block >= FLASH_BLOCK_COUNT)
        return LFS_ERR_INVAL;
    memset(ram->bytes + (size_t)block * FLASH_BLOCK_SIZE, 0xff,
           FLASH_BLOCK_SIZE);
    return LFS_ERR_OK;
}

static int ram_sync(const struct lfs_config *config)
{
    ram_nor *ram = (ram_nor *)config->context;
    ++ram->sync_calls;
    if (ram->fail_sync_call != 0U &&
        ram->sync_calls == ram->fail_sync_call) {
        ram->fail_sync_call = 0U;
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

static void fs_configure(test_fs *fs, ram_nor *ram)
{
    memset(fs, 0, sizeof(*fs));
    fs->config.context = ram;
    fs->config.read = ram_read;
    fs->config.prog = ram_prog;
    fs->config.erase = ram_erase;
    fs->config.sync = ram_sync;
    fs->config.read_size = 16U;
    fs->config.prog_size = 16U;
    fs->config.block_size = FLASH_BLOCK_SIZE;
    fs->config.block_count = FLASH_BLOCK_COUNT;
    fs->config.block_cycles = 500;
    fs->config.cache_size = TEST_CACHE_SIZE;
    fs->config.lookahead_size = TEST_LOOKAHEAD_SIZE;
    fs->config.read_buffer = fs->read_cache;
    fs->config.prog_buffer = fs->prog_cache;
    fs->config.lookahead_buffer = fs->lookahead;
    fs->config.name_max = 255U;
    fs->config.file_max = 0x7fffffffUL;
    fs->config.attr_max = 1022U;
    fs->config.inline_max = 1U;
}

static bool fs_format_mount(test_fs *fs, ram_nor *ram)
{
    int error;
    fs_configure(fs, ram);
    error = lfs_format(&fs->lfs, &fs->config);
    if (error != LFS_ERR_OK)
        return false;
    memset(&fs->lfs, 0, sizeof(fs->lfs));
    return lfs_mount(&fs->lfs, &fs->config) == LFS_ERR_OK;
}

static bool fs_mount_existing(test_fs *fs, ram_nor *ram)
{
    fs_configure(fs, ram);
    return lfs_mount(&fs->lfs, &fs->config) == LFS_ERR_OK;
}

static bool name_for_start(void *ctx, const bc_rec_start *start,
                           char name[BC_REC_NAME_SIZE])
{
    (void)ctx;
    (void)start;
    memcpy(name, "capture.raw", sizeof("capture.raw"));
    return true;
}

static void call_log(fixture *f, char call)
{
    if (f->call_count < MAX_CALLS)
        f->calls[f->call_count++] = call;
}

static bc_rec_result port_open(void *ctx, const bc_rec_start *start,
                               bc_rec_file *file)
{
    fixture *f = (fixture *)ctx;
    ++f->open_count;
    call_log(f, 'O');
    return bc_rec_store_open(&f->store, start, file);
}

static bc_rec_result port_append(void *ctx, const uint8_t *data,
                                 uint16_t length)
{
    fixture *f = (fixture *)ctx;
    ++f->append_count;
    call_log(f, 'A');
    return bc_rec_store_append(&f->store, data, length);
}

static bc_rec_result port_checkpoint(void *ctx, bc_rec_file *file)
{
    fixture *f = (fixture *)ctx;
    ++f->checkpoint_count;
    call_log(f, 'K');
    return bc_rec_store_checkpoint(&f->store, file);
}

static bc_rec_result port_finish(void *ctx, bool complete, bc_rec_file *file)
{
    fixture *f = (fixture *)ctx;
    ++f->finish_count;
    call_log(f, 'F');
    return bc_rec_store_finish(&f->store, complete, file);
}

static bc_rec_result port_capture_start(void *ctx, uint64_t id)
{
    fixture *f = (fixture *)ctx;
    (void)id;
    ++f->capture_start_count;
    call_log(f, 'S');
    return f->capture_start_result;
}

static bc_rec_result port_capture_stop(void *ctx, uint64_t id)
{
    fixture *f = (fixture *)ctx;
    (void)id;
    ++f->capture_stop_count;
    call_log(f, 'T');
    return f->capture_stop_result;
}

static bool port_capture_abort(void *ctx, uint64_t id)
{
    fixture *f = (fixture *)ctx;
    (void)id;
    ++f->capture_abort_count;
    call_log(f, 'B');
    return f->capture_abort_quiescent;
}

static bool port_live(void *ctx, uint64_t id, uint32_t sequence,
                      const uint8_t *data, uint16_t length)
{
    fixture *f = (fixture *)ctx;
    ++f->live_count;
    call_log(f, 'L');
    return bc_voice_service_live(&f->service, id, sequence, data, length);
}

static void port_changed(void *ctx, const bc_rec_snapshot *snapshot)
{
    fixture *f = (fixture *)ctx;
    call_log(f, 'C');
    bc_voice_service_changed(&f->service, snapshot);
}

static bc_rec_result settings_store(void *ctx,
                                    const bc_voice_settings *settings)
{
    fixture *f = (fixture *)ctx;
    (void)settings;
    ++f->settings_count;
    return f->settings_result;
}

static void outcome_set(void *ctx, uint64_t recording_id, uint8_t outcome)
{
    fixture *f = (fixture *)ctx;
    ++f->outcome_calls;
    f->outcome_recording_id = recording_id;
    f->outcome_value = outcome;
}

static bc_rec_result next_id(void *ctx, uint64_t *id)
{
    fixture *f = (fixture *)ctx;
    ++f->next_gesture_id;
    if (id == NULL)
        return BC_REC_INVALID;
    *id = f->next_gesture_id;
    return BC_REC_OK;
}

static bool sink_send(void *ctx, const uint8_t *packet, uint16_t length,
                      uint32_t epoch)
{
    fixture *f = (fixture *)ctx;
    bc_voice_message message;
    bc_wire_result result;
    ++f->sink.packet_count;
    if (f->sink.fail_all || f->sink.fail_next ||
        f->sink.packet_count == f->sink.fail_packet) {
        f->sink.fail_next = false;
        return false;
    }
    result = bc_voice_receive(&f->sink.receiver, epoch,
                              f->service.now_ms, packet, length, &message);
    if (result == BC_WIRE_MESSAGE) {
        if (f->sink.message_count < MAX_MESSAGES)
            f->sink.messages[f->sink.message_count++] = message;
    } else if (result == BC_WIRE_INVALID) {
        ++f->sink.invalid_packets;
    }
    return true;
}

static void fill_pattern(uint8_t *data, uint16_t length, uint8_t seed)
{
    uint16_t i;
    for (i = 0U; i < length; ++i)
        data[i] = (uint8_t)(seed + (uint8_t)(i * 29U));
}

static bool runtime_init(fixture *f, uint32_t epoch)
{
    bc_rec_port recording_port;
    bc_rec_config recording_config;
    bc_voice_gesture_config gesture_config;
    bc_voice_service_port service_port;
    bc_voice_settings settings;

    memset(&f->store, 0, sizeof(f->store));
    memset(&f->recording, 0, sizeof(f->recording));
    memset(&f->gesture, 0, sizeof(f->gesture));
    memset(&f->service, 0, sizeof(f->service));
    memset(&f->sink, 0, sizeof(f->sink));
    f->next_command_message = 100U;
    f->capture_start_result = BC_REC_OK;
    f->capture_stop_result = BC_REC_OK;
    f->settings_result = BC_REC_OK;
    f->capture_abort_quiescent = true;

    if (!bc_rec_store_init(&f->store, &f->fs.lfs, name_for_start, NULL))
        return false;

    memset(&recording_port, 0, sizeof(recording_port));
    recording_port.ctx = f;
    recording_port.open = port_open;
    recording_port.append = port_append;
    recording_port.checkpoint = port_checkpoint;
    recording_port.finish = port_finish;
    recording_port.capture_start = port_capture_start;
    recording_port.capture_stop = port_capture_stop;
    recording_port.capture_abort = port_capture_abort;
    recording_port.live = port_live;
    recording_port.changed = port_changed;
    recording_config.checkpoint_ms = 100U;
    recording_config.checkpoint_bytes = 440U;
    recording_config.stop_timeout_ms = 1000U;
    if (!bc_recording_init(&f->recording, &recording_port,
                           &recording_config))
        return false;

    gesture_config.ptt_limit_ms = 5000U;
    gesture_config.memo_limit_ms = 3000U;
    gesture_config.touch_timeout_ms = 250U;
    gesture_config.double_tap_debounce_ms = 200U;
    gesture_config.memo_enabled = true;
    if (!bc_voice_gesture_init(&f->gesture, &f->recording, &gesture_config,
                               next_id, f))
        return false;

    service_port.ctx = f;
    service_port.send = sink_send;
    service_port.settings = settings_store;
    settings.ptt_limit_ms = gesture_config.ptt_limit_ms;
    settings.memo_limit_ms = gesture_config.memo_limit_ms;
    settings.memo_enabled = true;
    settings.led_enabled = true;
    settings.haptic_enabled = true;
    if (!bc_voice_service_init(&f->service, &f->recording, &f->store,
                               &f->gesture, &service_port, &settings))
        return false;
    bc_voice_service_link(&f->service, epoch, true);
    return true;
}

static bool fixture_setup(fixture *f)
{
    memset(f, 0, sizeof(*f));
    memset(f->ram.bytes, 0xff, sizeof(f->ram.bytes));
    if (!fs_format_mount(&f->fs, &f->ram))
        return false;
    if (!runtime_init(f, 1U)) {
        (void)lfs_unmount(&f->fs.lfs);
        return false;
    }
    return true;
}

static bool fixture_remount(fixture *f, uint32_t epoch)
{
    if (f->fs.lfs.cfg != NULL && lfs_unmount(&f->fs.lfs) != LFS_ERR_OK)
        return false;
    if (!fs_mount_existing(&f->fs, &f->ram))
        return false;
    return runtime_init(f, epoch);
}

static void fixture_destroy(fixture *f)
{
    if (f->fs.lfs.cfg != NULL)
        CHECK(lfs_unmount(&f->fs.lfs) == LFS_ERR_OK);
}

static uint16_t command_message(fixture *f, bc_voice_message *message,
                                uint8_t kind, uint32_t request_id,
                                const uint8_t *extra, uint16_t extra_length)
{
    uint16_t i;
    memset(message, 0, sizeof(*message));
    ++f->next_command_message;
    if (f->next_command_message == 0U)
        ++f->next_command_message;
    message->message_id = f->next_command_message;
    message->kind = kind;
    message->direction = BC_VOICE_REQUEST;
    bc_voice_put32(message->payload, request_id);
    for (i = 0U; i < extra_length; ++i)
        message->payload[4U + i] = extra[i];
    message->length = (uint16_t)(4U + extra_length);
    return message->message_id;
}

static bool deliver_message(fixture *f, const bc_voice_message *message,
                            uint16_t att_limit, uint32_t epoch,
                            uint32_t now_ms, bool corrupt_crc)
{
    uint16_t offset = 0U;
    uint16_t total = (uint16_t)(message->length + 4U);
    uint8_t packet[BC_VOICE_PACKET_MAX];
    while (offset < total) {
        uint16_t next = 0U;
        uint16_t length = bc_voice_fragment(message, offset, att_limit,
                                             packet, sizeof(packet), &next);
        if (length == 0U || next <= offset || next > total)
            return false;
        if (corrupt_crc && next == total)
            packet[length - 1U] ^= 0x01U;
        /* Deliver a logical command's fragments at one worker timestamp. A
         * caller may pass a later timestamp for the next command; advancing
         * here would make a subsequent poll appear to run backwards when the
         * command took more than one ATT packet. */
        bc_voice_service_receive(&f->service, epoch, packet, length,
                                 now_ms);
        offset = next;
    }
    return true;
}

static unsigned pump(fixture *f, uint32_t now_ms, uint16_t att_limit)
{
    unsigned iterations = 0U;
    while (iterations < 4096U &&
           bc_voice_service_poll(&f->service, now_ms, att_limit))
        ++iterations;
    return iterations;
}

static void clear_messages(fixture *f)
{
    memset(&f->sink.receiver, 0, sizeof(f->sink.receiver));
    f->sink.message_count = 0U;
    f->sink.packet_count = 0U;
    f->sink.invalid_packets = 0U;
}

static const bc_voice_message *find_response(const fixture *f,
                                             unsigned first, uint8_t kind,
                                             uint32_t request_id)
{
    unsigned i;
    for (i = first; i < f->sink.message_count; ++i) {
        const bc_voice_message *message = &f->sink.messages[i];
        if (message->direction == BC_VOICE_RESPONSE &&
            message->kind == kind && message->length >= 5U &&
            bc_voice_get32(message->payload) == request_id)
            return message;
    }
    return NULL;
}

static unsigned count_kind(const fixture *f, unsigned first, uint8_t kind)
{
    unsigned count = 0U;
    unsigned i;
    for (i = first; i < f->sink.message_count; ++i)
        if (f->sink.messages[i].kind == kind)
            ++count;
    return count;
}

static bool send_request(fixture *f, uint8_t kind, uint32_t request_id,
                         const uint8_t *extra, uint16_t extra_length,
                         uint16_t att_limit, uint32_t now_ms,
                         bool corrupt_crc)
{
    bc_voice_message message;
    command_message(f, &message, kind, request_id, extra, extra_length);
    return deliver_message(f, &message, att_limit, f->service.epoch,
                            now_ms, corrupt_crc);
}

static bc_rec_start start_value(uint64_t id, bc_rec_trigger trigger,
                                uint32_t limit_ms)
{
    bc_rec_start start;
    start.id = id;
    start.trigger = trigger;
    start.duration_limit_ms = limit_ms;
    return start;
}

static void encode_start(uint8_t extra[13], const bc_rec_start *start)
{
    bc_voice_put64(extra, start->id);
    extra[8] = (uint8_t)start->trigger;
    bc_voice_put32(extra + 9, start->duration_limit_ms);
}

static void encode_id(uint8_t extra[8], uint64_t id)
{
    bc_voice_put64(extra, id);
}

static void encode_ack(uint8_t extra[8], uint32_t token, uint32_t next)
{
    bc_voice_put32(extra, token);
    bc_voice_put32(extra + 4, next);
}

static void encode_resume(uint8_t extra[16], uint64_t id, uint32_t offset,
                          uint32_t token)
{
    bc_voice_put64(extra, id);
    bc_voice_put32(extra + 8, offset);
    bc_voice_put32(extra + 12, token);
}

static void encode_settings(uint8_t extra[11], uint32_t ptt, uint32_t memo,
                            bool memo_enabled, bool led, bool haptic)
{
    bc_voice_put32(extra, ptt);
    bc_voice_put32(extra + 4, memo);
    extra[8] = memo_enabled ? 1U : 0U;
    extra[9] = led ? 1U : 0U;
    extra[10] = haptic ? 1U : 0U;
}

static void encode_receipt(uint8_t extra[17], uint64_t id, uint32_t bytes,
                           uint32_t crc, bool delete)
{
    bc_voice_put64(extra, id);
    bc_voice_put32(extra + 8, bytes);
    bc_voice_put32(extra + 12, crc);
    extra[16] = delete ? 1U : 0U;
}

static void encode_outcome(uint8_t extra[13], uint64_t id, uint32_t token,
                           uint8_t outcome)
{
    bc_voice_put64(extra, id);
    bc_voice_put32(extra + 8U, token);
    extra[12] = outcome;
}

static bool record_complete(fixture *f, const bc_rec_start *start,
                            unsigned frame_count, uint8_t seed,
                            uint8_t *raw, uint32_t now_ms)
{
    uint8_t frame[BC_REC_FRAME_MAX];
    unsigned i;
    CHECK(bc_recording_start(&f->recording, start, now_ms) == BC_REC_OK);
    for (i = 0U; i < frame_count; ++i) {
        fill_pattern(frame, sizeof(frame), (uint8_t)(seed + i));
        if (raw != NULL)
            memcpy(raw + i * sizeof(frame), frame, sizeof(frame));
        CHECK(bc_recording_frame(&f->recording, start->id, i + 1U, frame,
                                 sizeof(frame), now_ms + i + 1U) == BC_REC_OK);
    }
    CHECK(bc_recording_stop(&f->recording, start->id,
                            now_ms + frame_count + 1U) == BC_REC_OK);
    CHECK(bc_recording_drained(&f->recording, start->id) == BC_REC_OK);
    return bc_recording_snapshot(&f->recording)->phase == BC_REC_SAVED;
}

static bool call_order_has(const fixture *f, char before, char after)
{
    unsigned i;
    for (i = 0U; i < f->call_count; ++i) {
        if (f->calls[i] == before) {
            unsigned j;
            for (j = i + 1U; j < f->call_count; ++j)
                if (f->calls[j] == after)
                    return true;
        }
    }
    return false;
}

static void test_handshake_queue_and_wire_rejection(void)
{
    fixture f;
    uint8_t start_extra[13];
    bc_rec_start start = start_value(0x101U, BC_REC_PTT, 2000U);
    unsigned before;
    const bc_voice_message *response;
    unsigned i;

    CHECK(fixture_setup(&f));
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_HELLO, 1U, NULL, 0U, 20U, 10U,
                        false));
    CHECK(pump(&f, 10U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_HELLO, 1U);
    CHECK(response != NULL && response->length == 20U &&
          response->payload[4] == BC_REC_OK);
    CHECK(response != NULL &&
          (bc_voice_get32(response->payload + 5) &
           (BC_VOICE_CAP_LOCAL | BC_VOICE_CAP_PTT | BC_VOICE_CAP_MEMO |
            BC_VOICE_CAP_LIVE | BC_VOICE_CAP_RESUME | BC_VOICE_CAP_CUSTODY |
            BC_VOICE_CAP_SETTINGS)) ==
              (BC_VOICE_CAP_LOCAL | BC_VOICE_CAP_PTT | BC_VOICE_CAP_MEMO |
               BC_VOICE_CAP_LIVE | BC_VOICE_CAP_RESUME | BC_VOICE_CAP_CUSTODY |
               BC_VOICE_CAP_SETTINGS));

    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_SETTINGS_GET, 2U, NULL, 0U, 244U, 20U,
                       false));
    CHECK(pump(&f, 20U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_SETTINGS_GET, 2U);
    CHECK(response != NULL && response->length == 16U &&
          bc_voice_get32(response->payload + 5) == 5000U &&
          bc_voice_get32(response->payload + 9) == 3000U);

    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_HELLO, 3U, (const uint8_t *)"x", 1U,
                       20U, 30U, false));
    CHECK(pump(&f, 30U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_HELLO, 3U);
    CHECK(response != NULL && response->payload[4] == BC_REC_INVALID);

    /* Four queued controls consume all reservations. A fifth Start must not
     * open storage or start capture until those replies have drained. */
    clear_messages(&f);
    for (i = 0U; i < BC_VOICE_CONTROL_SLOTS; ++i)
        CHECK(send_request(&f, BC_VOICE_HELLO, 10U + i, NULL, 0U, 20U,
                           40U + i, false));
    encode_start(start_extra, &start);
    CHECK(send_request(&f, BC_VOICE_START, 20U, start_extra,
                       sizeof(start_extra), 20U, 50U, false));
    CHECK(f.open_count == 0U && f.capture_start_count == 0U);
    CHECK(pump(&f, 60U, 20U) != 0U);
    for (i = 0U; i < BC_VOICE_CONTROL_SLOTS; ++i)
        CHECK(find_response(&f, 0U, BC_VOICE_HELLO, 10U + i) != NULL);
    CHECK(find_response(&f, 0U, BC_VOICE_START, 20U) == NULL);

    clear_messages(&f);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_START, 21U, start_extra, sizeof(start_extra),
                       20U, 70U, true));
    CHECK(pump(&f, 70U, 20U) == 0U);
    CHECK(f.open_count == 0U && f.capture_start_count == 0U &&
          find_response(&f, before, BC_VOICE_START, 21U) == NULL);
    /* Epoch filtering happens before wire assembly and cannot mutate owner. */
    CHECK(send_request(&f, BC_VOICE_START, 22U, start_extra, sizeof(start_extra),
                       20U, 80U, false));
    /* The helper uses the current epoch; deliver a second copy explicitly on
     * the wrong epoch to exercise the service gate. */
    {
        bc_voice_message command;
        command_message(&f, &command, BC_VOICE_START, 23U, start_extra,
                        sizeof(start_extra));
        CHECK(deliver_message(&f, &command, 244U, f.service.epoch + 1U,
                              81U, false));
    }
    CHECK(f.open_count == 1U && f.capture_start_count == 1U);
    CHECK(pump(&f, 90U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_START, 22U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    CHECK(find_response(&f, before, BC_VOICE_START, 23U) == NULL);
    CHECK(f.sink.invalid_packets == 0U);
    (void)bc_recording_stop(&f.recording, start.id, 100U);
    (void)bc_recording_drained(&f.recording, start.id);
    fixture_destroy(&f);
}

static void test_start_stop_idempotency_and_recovery(void)
{
    fixture f;
    bc_rec_start start = start_value(0x1111222233334444ULL, BC_REC_PTT, 4000U);
    bc_rec_start wrong = start;
    uint8_t extra[13];
    uint8_t frame[BC_REC_FRAME_MAX];
    const bc_voice_message *response;
    unsigned before;
    unsigned capture_starts;
    bc_rec_snapshot snapshot;

    CHECK(fixture_setup(&f));
    encode_start(extra, &start);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_START, 100U, extra, sizeof(extra), 244U,
                       10U, false));
    CHECK(pump(&f, 10U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_START, 100U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK &&
          response->payload[14] == BC_REC_RECORDING);
    CHECK(f.open_count == 1U && f.capture_start_count == 1U);
    CHECK(call_order_has(&f, 'O', 'S'));

    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_START, 101U, extra, sizeof(extra), 20U,
                       20U, false));
    CHECK(pump(&f, 20U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_START, 101U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    CHECK(f.open_count == 1U && f.capture_start_count == 1U);

    wrong.duration_limit_ms++;
    encode_start(extra, &wrong);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_START, 102U, extra, sizeof(extra), 244U,
                       30U, false));
    CHECK(pump(&f, 30U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_START, 102U);
    CHECK(response != NULL && response->payload[4] == BC_REC_INVALID);
    CHECK(f.open_count == 1U && f.capture_start_count == 1U);

    fill_pattern(frame, sizeof(frame), 0x20U);
    CHECK(bc_recording_frame(&f.recording, start.id, 1U, frame,
                             sizeof(frame), 40U) == BC_REC_OK);
    CHECK(call_order_has(&f, 'A', 'L') || f.live_count == 0U);

    before = f.sink.message_count;
    {
        uint8_t id_extra[8];
        encode_id(id_extra, start.id);
        CHECK(send_request(&f, BC_VOICE_STOP, 103U, id_extra, sizeof(id_extra),
                           20U, 50U, false));
    }
    CHECK(pump(&f, 50U, 20U) == 0U ||
          find_response(&f, before, BC_VOICE_STOP, 103U) == NULL);
    CHECK(find_response(&f, before, BC_VOICE_STOP, 103U) == NULL);
    /* The final frame arrives after Stop, before the capture drain event. */
    fill_pattern(frame, sizeof(frame), 0x40U);
    CHECK(bc_recording_frame(&f.recording, start.id, 2U, frame,
                             sizeof(frame), 60U) == BC_REC_OK);
    CHECK(bc_recording_drained(&f.recording, start.id) == BC_REC_OK);
    CHECK(bc_recording_snapshot(&f.recording)->accepted_frames == 2U);
    CHECK(bc_recording_snapshot(&f.recording)->file.bytes == 440U);
    CHECK(bc_recording_snapshot(&f.recording)->file.complete);
    CHECK(call_order_has(&f, 'T', 'F'));
    CHECK(pump(&f, 70U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_STOP, 103U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK &&
          response->payload[14] == BC_REC_SAVED &&
          bc_voice_get32(response->payload + 25) == 440U &&
          bc_voice_get32(response->payload + 29) == 2U);

    before = f.sink.message_count;
    {
        uint8_t id_extra[8];
        encode_id(id_extra, start.id);
        CHECK(send_request(&f, BC_VOICE_STOP, 104U, id_extra, sizeof(id_extra),
                           244U, 80U, false));
    }
    CHECK(pump(&f, 80U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_STOP, 104U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    capture_starts = f.capture_start_count;
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_START, 105U, (uint8_t[13]){
                           (uint8_t)start.id, (uint8_t)(start.id >> 8),
                           (uint8_t)(start.id >> 16), (uint8_t)(start.id >> 24),
                           (uint8_t)(start.id >> 32), (uint8_t)(start.id >> 40),
                           (uint8_t)(start.id >> 48), (uint8_t)(start.id >> 56),
                           (uint8_t)start.trigger, (uint8_t)start.duration_limit_ms,
                           (uint8_t)(start.duration_limit_ms >> 8),
                           (uint8_t)(start.duration_limit_ms >> 16),
                           (uint8_t)(start.duration_limit_ms >> 24)}, 13U,
                       20U, 90U, false));
    CHECK(pump(&f, 90U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_START, 105U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK &&
          response->payload[14] == BC_REC_SAVED);
    CHECK(f.capture_start_count == capture_starts);

    /* Reboot/reconnect keeps the same raw file and never starts the mic. */
    CHECK(fixture_remount(&f, 2U));
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_START, 106U,
                       (uint8_t[13]){
                           (uint8_t)start.id, (uint8_t)(start.id >> 8),
                           (uint8_t)(start.id >> 16), (uint8_t)(start.id >> 24),
                           (uint8_t)(start.id >> 32), (uint8_t)(start.id >> 40),
                           (uint8_t)(start.id >> 48), (uint8_t)(start.id >> 56),
                           (uint8_t)start.trigger, (uint8_t)start.duration_limit_ms,
                           (uint8_t)(start.duration_limit_ms >> 8),
                           (uint8_t)(start.duration_limit_ms >> 16),
                           (uint8_t)(start.duration_limit_ms >> 24)}, 13U,
                       244U, 100U, false));
    CHECK(pump(&f, 100U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_START, 106U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK &&
          response->payload[14] == BC_REC_SAVED);
    CHECK(f.capture_start_count == capture_starts);

    wrong.trigger = BC_REC_MEMO;
    encode_start(extra, &wrong);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_START, 107U, extra, sizeof(extra), 20U,
                       110U, false));
    CHECK(pump(&f, 110U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_START, 107U);
    CHECK(response != NULL && response->payload[4] == BC_REC_INVALID);
    snapshot = *bc_recording_snapshot(&f.recording);
    CHECK(snapshot.start.id == start.id && snapshot.phase == BC_REC_SAVED);

    /* A committed but incomplete record replays as INTERRUPTED/PARTIAL. */
    {
        bc_rec_start partial = start_value(0x5555U, BC_REC_APP, 0U);
        CHECK(bc_recording_start(&f.recording, &partial, 120U) == BC_REC_OK);
        fill_pattern(frame, sizeof(frame), 0x81U);
        CHECK(bc_recording_frame(&f.recording, partial.id, 1U, frame,
                                 sizeof(frame), 121U) == BC_REC_OK);
        CHECK(bc_recording_fault(&f.recording, partial.id,
                                 BC_REC_CAPTURE_ERROR, 122U) ==
              BC_REC_CAPTURE_ERROR);
        CHECK(bc_recording_drained(&f.recording, partial.id) ==
              BC_REC_CAPTURE_ERROR);
        CHECK(fixture_remount(&f, 3U));
        encode_start(extra, &partial);
        before = f.sink.message_count;
        CHECK(send_request(&f, BC_VOICE_START, 108U, extra, sizeof(extra),
                           244U, 130U, false));
        CHECK(pump(&f, 130U, 244U) != 0U);
        response = find_response(&f, before, BC_VOICE_START, 108U);
        CHECK(response != NULL && response->payload[4] == BC_REC_INTERRUPTED &&
              response->payload[14] == BC_REC_PARTIAL &&
              (response->payload[16] & 2U) != 0U);
        CHECK(f.capture_start_count == capture_starts + 1U);
    }
    fixture_destroy(&f);
}

static void test_ready_live_ack_lease_and_disconnect(void)
{
    fixture f;
    bc_rec_start start = start_value(0x2222U, BC_REC_PTT, 0U);
    uint8_t extra[13];
    uint8_t frame[BC_REC_FRAME_MAX];
    uint32_t token;
    unsigned before;
    unsigned i;
    const bc_voice_message *response;

    CHECK(fixture_setup(&f));
    encode_start(extra, &start);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_START, 200U, extra, sizeof(extra), 244U,
                       10U, false));
    CHECK(pump(&f, 10U, 244U) != 0U);
    CHECK(find_response(&f, before, BC_VOICE_START, 200U) != NULL);

    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_READY, 201U, (const uint8_t[]){1U}, 1U,
                       20U, 20U, false));
    CHECK(pump(&f, 20U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_READY, 201U);
    CHECK(response != NULL && response->length == 17U &&
          response->payload[4] == BC_REC_OK);
    token = response == NULL ? 0U : bc_voice_get32(response->payload + 5);
    CHECK(token != 0U && f.service.ready && f.recording.link_ready);

    fill_pattern(frame, sizeof(frame), 0x10U);
    CHECK(!bc_voice_service_live(&f.service, start.id, 0U, frame, sizeof(frame)));
    CHECK(!f.service.live_pending && f.service.live_count == 0U);
    CHECK(bc_recording_frame(&f.recording, start.id, 1U, frame,
                             sizeof(frame), 30U) == BC_REC_OK);
    CHECK(pump(&f, 30U, 20U) != 0U);
    CHECK(count_kind(&f, 0U, BC_VOICE_LIVE) == 1U);
    CHECK(f.sink.message_count != 0U);
    for (i = 0U; i < f.sink.message_count; ++i) {
        const bc_voice_message *message = &f.sink.messages[i];
        if (message->kind == BC_VOICE_LIVE) {
            CHECK(message->length == 228U);
            CHECK(bc_voice_get32(message->payload) == token);
            CHECK(bc_voice_get32(message->payload + 4) == 1U);
            CHECK(memcmp(message->payload + 8, frame, sizeof(frame)) == 0);
        }
    }
    CHECK(f.recording.snapshot.live_sent_frames == 1U);
    CHECK(f.live_count == 1U && call_order_has(&f, 'A', 'L'));

    before = f.sink.message_count;
    encode_ack(extra, token, 2U);
    CHECK(send_request(&f, BC_VOICE_LIVE_ACK, 202U, extra, 8U, 244U, 40U,
                       false));
    CHECK(pump(&f, 40U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_LIVE_ACK, 202U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    /* Duplicate ACK is idempotent; an unadvertised future boundary is not. */
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_LIVE_ACK, 203U, extra, 8U, 20U, 50U,
                       false));
    CHECK(pump(&f, 50U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_LIVE_ACK, 203U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    encode_ack(extra, token, 4U);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_LIVE_ACK, 204U, extra, 8U, 244U, 60U,
                       false));
    CHECK(pump(&f, 60U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_LIVE_ACK, 204U);
    CHECK(response != NULL && response->payload[4] == BC_REC_INVALID);

    /* BLE send loss does not roll back local storage. The service retries the
     * complete logical LIVE message after the sink accepts it again. */
    f.sink.fail_next = true;
    fill_pattern(frame, sizeof(frame), 0x30U);
    CHECK(bc_recording_frame(&f.recording, start.id, 2U, frame,
                             sizeof(frame), 70U) == BC_REC_OK);
    CHECK(f.recording.snapshot.live_sent_frames == 2U);
    CHECK(pump(&f, 70U, 244U) == 0U);
    CHECK(f.sink.invalid_packets == 0U);
    CHECK(pump(&f, 71U, 244U) != 0U);
    CHECK(count_kind(&f, 0U, BC_VOICE_LIVE) == 2U);

    /* Four unacknowledged events fill the wire window. Storage and the bounded
     * prefix FIFO keep accepting frames while the client catches up. */
    for (i = 3U; i <= 7U; ++i) {
        fill_pattern(frame, sizeof(frame), (uint8_t)(0x30U + i));
        CHECK(bc_recording_frame(&f.recording, start.id, i, frame,
                                 sizeof(frame), 80U + i) == BC_REC_OK);
        CHECK(pump(&f, 80U + i, 244U) != 0U || i >= 6U);
    }
    CHECK(f.service.live_count == 4U);
    CHECK(f.service.live_prefix_count == 2U);
    CHECK(f.recording.snapshot.accepted_frames == 7U);
    CHECK(f.recording.snapshot.live_dropped_frames == 0U);

    /* A lease expiry disables live delivery while local append remains valid;
     * link_ready still represents the physical BLE connection. */
    (void)pump(&f, f.service.ready_ms + BC_VOICE_READY_MS, 244U);
    CHECK(!f.service.ready && f.service.live_disabled && f.recording.link_ready);
    fill_pattern(frame, sizeof(frame), 0xa0U);
    CHECK(bc_recording_frame(&f.recording, start.id, 8U, frame,
                             sizeof(frame), f.service.now_ms + 1U) == BC_REC_OK);
    CHECK(f.recording.snapshot.live_dropped_frames > 0U);
    bc_voice_service_link(&f.service, 2U, false);
    fill_pattern(frame, sizeof(frame), 0xb0U);
    CHECK(bc_recording_frame(&f.recording, start.id, 9U, frame,
                             sizeof(frame), f.service.now_ms + 2U) == BC_REC_OK);
    CHECK(!f.service.connected);
    bc_voice_service_link(&f.service, 3U, true);
    CHECK(f.service.connected && !f.service.ready);
    CHECK(bc_recording_stop(&f.recording, start.id, f.service.now_ms + 3U) ==
          BC_REC_OK);
    CHECK(bc_recording_drained(&f.recording, start.id) == BC_REC_OK);
    fixture_destroy(&f);
}

static void test_live_prefix_delayed_ready_and_backpressure(void)
{
    fixture f;
    bc_rec_start start = start_value(0x7301U, BC_REC_PTT, 0U);
    bc_rec_start replacement = start_value(0x7302U, BC_REC_PTT, 0U);
    uint8_t frames[12U][BC_REC_FRAME_MAX];
    uint8_t extra[8];
    uint32_t old_token;
    uint32_t token;
    uint32_t replacement_old_token;
    unsigned before;
    unsigned i;
    unsigned live_seen;
    const bc_voice_message *response;

    CHECK(fixture_setup(&f));
    old_token = f.service.live_token;
    CHECK(bc_recording_start(&f.recording, &start, 10U) == BC_REC_OK);
    for (i = 0U; i < 5U; ++i) {
        fill_pattern(frames[i], sizeof(frames[i]), (uint8_t)(0x90U + i));
        CHECK(bc_recording_frame(&f.recording, start.id, i + 1U, frames[i],
                                 sizeof(frames[i]), 20U + i) == BC_REC_OK);
    }
    CHECK(f.recording.link_ready && f.service.live_prefix_count == 5U &&
          f.recording.snapshot.live_sent_frames == 5U &&
          f.recording.snapshot.live_dropped_frames == 0U);

    /* Local capture can precede the app's first READY, but no LIVE event may
     * escape until the token/lease has been confirmed. */
    clear_messages(&f);
    (void)pump(&f, 30U, 244U);
    CHECK(count_kind(&f, 0U, BC_VOICE_LIVE) == 0U);

    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_READY, 7302U, (const uint8_t[]){1U}, 1U,
                       244U, 100U, false));
    CHECK(pump(&f, 100U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_READY, 7302U);
    CHECK(response != NULL && response->length == 17U &&
          response->payload[4] == BC_REC_OK);
    token = response == NULL ? 0U : bc_voice_get32(response->payload + 5U);
    CHECK(token != 0U && token != old_token && f.service.ready);

    live_seen = 0U;
    for (i = 0U; i < f.sink.message_count; ++i) {
        const bc_voice_message *message = &f.sink.messages[i];
        if (message->kind != BC_VOICE_LIVE)
            continue;
        CHECK(message->length == 8U + BC_REC_FRAME_MAX);
        CHECK(bc_voice_get32(message->payload) == token);
        CHECK(bc_voice_get32(message->payload + 4U) == live_seen + 1U);
        CHECK(memcmp(message->payload + 8U, frames[live_seen],
                     BC_REC_FRAME_MAX) == 0);
        ++live_seen;
    }
    CHECK(live_seen == 4U && f.service.live_count == 4U &&
          f.service.live_prefix_count == 1U);

    /* ACKing the four-message window releases the fifth buffered frame. */
    encode_ack(extra, token, 5U);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_LIVE_ACK, 7303U, extra, sizeof(extra),
                       244U, 110U, false));
    CHECK(pump(&f, 110U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_LIVE_ACK, 7303U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    live_seen = 0U;
    for (i = before; i < f.sink.message_count; ++i) {
        const bc_voice_message *message = &f.sink.messages[i];
        if (message->kind == BC_VOICE_LIVE) {
            CHECK(bc_voice_get32(message->payload) == token);
            CHECK(bc_voice_get32(message->payload + 4U) == 5U);
            CHECK(memcmp(message->payload + 8U, frames[4],
                         BC_REC_FRAME_MAX) == 0);
            ++live_seen;
        }
    }
    CHECK(live_seen == 1U && f.service.live_count == 1U &&
          f.service.live_prefix_count == 0U);

    /* A small ATT limit fragments the same 228-byte logical LIVE message;
     * the raw frame and token remain unchanged. */
    fill_pattern(frames[5], sizeof(frames[5]), 0x95U);
    CHECK(bc_recording_frame(&f.recording, start.id, 6U, frames[5],
                             sizeof(frames[5]), 120U) == BC_REC_OK);
    clear_messages(&f);
    CHECK(pump(&f, 120U, 20U) != 0U);
    CHECK(f.sink.packet_count > 4U && count_kind(&f, 0U, BC_VOICE_LIVE) == 1U);
    for (i = 0U; i < f.sink.message_count; ++i) {
        const bc_voice_message *message = &f.sink.messages[i];
        if (message->kind == BC_VOICE_LIVE) {
            CHECK(message->length == 8U + BC_REC_FRAME_MAX);
            CHECK(bc_voice_get32(message->payload) == token);
            CHECK(bc_voice_get32(message->payload + 4U) == 6U);
            CHECK(memcmp(message->payload + 8U, frames[5],
                         BC_REC_FRAME_MAX) == 0);
        }
    }

    /* Backpressure is bounded at four unacknowledged wire messages, while
     * later locally accepted frames remain in the 32-frame prefix FIFO. */
    for (i = 6U; i < 12U; ++i) {
        fill_pattern(frames[i], sizeof(frames[i]), (uint8_t)(0x95U + i - 5U));
        CHECK(bc_recording_frame(&f.recording, start.id, i + 1U, frames[i],
                                 sizeof(frames[i]), 130U + i) == BC_REC_OK);
        (void)pump(&f, 130U + i, 244U);
    }
    CHECK(f.service.live_count == 4U && f.service.live_prefix_count == 4U &&
          f.recording.snapshot.accepted_frames == 12U &&
          f.recording.snapshot.live_dropped_frames == 0U);

    CHECK(bc_recording_stop(&f.recording, start.id, 160U) == BC_REC_OK);
    CHECK(bc_recording_drained(&f.recording, start.id) == BC_REC_OK);

    /* Starting a new recording invalidates the prior READY lease. Its first
     * locally accepted frames wait for the new recording's READY response. */
    replacement_old_token = token;
    CHECK(bc_recording_start(&f.recording, &replacement, 170U) == BC_REC_OK);
    CHECK(!f.service.ready && !f.service.live_disabled &&
          f.service.live_prefix_count == 0U && f.service.live_sequence == 1U &&
          f.service.live_token != replacement_old_token);
    fill_pattern(frames[0], sizeof(frames[0]), 0xa6U);
    CHECK(bc_recording_frame(&f.recording, replacement.id, 1U, frames[0],
                             sizeof(frames[0]), 171U) == BC_REC_OK);
    CHECK(f.service.live_prefix_count == 1U &&
          f.recording.snapshot.live_sent_frames == 1U);
    clear_messages(&f);
    (void)pump(&f, 172U, 244U);
    CHECK(count_kind(&f, 0U, BC_VOICE_LIVE) == 0U);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_READY, 7304U, (const uint8_t[]){1U}, 1U,
                       244U, 180U, false));
    CHECK(pump(&f, 180U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_READY, 7304U);
    CHECK(response != NULL && response->length == 17U &&
          response->payload[4] == BC_REC_OK && f.service.ready);
    token = response == NULL ? 0U : bc_voice_get32(response->payload + 5U);
    CHECK(token != 0U && token != replacement_old_token);
    live_seen = 0U;
    for (i = before; i < f.sink.message_count; ++i) {
        const bc_voice_message *message = &f.sink.messages[i];
        if (message->kind != BC_VOICE_LIVE)
            continue;
        CHECK(message->length == 8U + BC_REC_FRAME_MAX);
        CHECK(bc_voice_get32(message->payload) == token);
        CHECK(bc_voice_get32(message->payload + 4U) == 1U);
        CHECK(memcmp(message->payload + 8U, frames[0], BC_REC_FRAME_MAX) == 0);
        ++live_seen;
    }
    CHECK(live_seen == 1U && f.service.live_count == 1U);
    CHECK(bc_recording_stop(&f.recording, replacement.id, 190U) == BC_REC_OK);
    CHECK(bc_recording_drained(&f.recording, replacement.id) == BC_REC_OK);
    fixture_destroy(&f);
}

static void test_live_prefix_overflow_cancel_timeout_and_reconnect(void)
{
    fixture f;
    bc_rec_start overflow = start_value(0x7401U, BC_REC_PTT, 0U);
    bc_rec_start reset = start_value(0x7402U, BC_REC_PTT, 0U);
    bc_rec_start timeout = start_value(0x7403U, BC_REC_PTT, 0U);
    bc_rec_start reconnect = start_value(0x7404U, BC_REC_PTT, 0U);
    bc_rec_start retry = start_value(0x7405U, BC_REC_PTT, 0U);
    bc_rec_start offline = start_value(0x7406U, BC_REC_PTT, 0U);
    uint8_t frame[BC_REC_FRAME_MAX];
    unsigned before;
    unsigned i;
    const bc_voice_message *response;

    CHECK(fixture_setup(&f));
    CHECK(bc_recording_start(&f.recording, &overflow, 10U) == BC_REC_OK);
    for (i = 1U; i <= BC_VOICE_LIVE_PREFIX_SLOTS; ++i) {
        fill_pattern(frame, sizeof(frame), (uint8_t)(0x10U + i));
        CHECK(bc_recording_frame(&f.recording, overflow.id, i, frame,
                                 sizeof(frame), 10U + i) == BC_REC_OK);
    }
    CHECK(f.service.live_prefix_count == BC_VOICE_LIVE_PREFIX_SLOTS &&
          !f.service.live_disabled &&
          f.recording.snapshot.accepted_frames == BC_VOICE_LIVE_PREFIX_SLOTS &&
          f.recording.snapshot.live_dropped_frames == 0U);

    /* The 33rd local frame remains durable even though preview has no bounded
     * space left. Overflow fails only the live callback and clears its FIFO. */
    fill_pattern(frame, sizeof(frame), 0x40U);
    CHECK(bc_recording_frame(&f.recording, overflow.id,
                             BC_VOICE_LIVE_PREFIX_SLOTS + 1U, frame,
                             sizeof(frame), 50U) == BC_REC_OK);
    CHECK(f.service.live_prefix_count == 0U && f.service.live_disabled &&
          !f.service.ready && f.recording.link_ready &&
          f.recording.snapshot.accepted_frames == BC_VOICE_LIVE_PREFIX_SLOTS + 1U &&
          f.recording.snapshot.accepted_bytes ==
              (BC_VOICE_LIVE_PREFIX_SLOTS + 1U) * BC_REC_FRAME_MAX &&
          f.recording.snapshot.live_dropped_frames == 1U);

    clear_messages(&f);
    (void)pump(&f, 51U, 244U);
    CHECK(count_kind(&f, 0U, BC_VOICE_LIVE) == 0U);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_READY, 7402U, (const uint8_t[]){1U}, 1U,
                       244U, 60U, false));
    CHECK(pump(&f, 60U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_READY, 7402U);
    CHECK(response != NULL && response->length == 5U &&
          response->payload[4] == BC_REC_INTERRUPTED && !f.service.ready &&
          f.recording.link_ready);
    CHECK(bc_recording_stop(&f.recording, overflow.id, 70U) == BC_REC_OK);
    CHECK(bc_recording_drained(&f.recording, overflow.id) == BC_REC_OK);
    CHECK(f.recording.snapshot.file.bytes ==
              (BC_VOICE_LIVE_PREFIX_SLOTS + 1U) * BC_REC_FRAME_MAX);

    /* A new recording clears the failed preview generation and may establish
     * a fresh READY lease. */
    CHECK(bc_recording_start(&f.recording, &reset, 80U) == BC_REC_OK);
    CHECK(!f.service.live_disabled && f.service.live_sequence == 1U &&
          f.service.live_prefix_count == 0U);
    fill_pattern(frame, sizeof(frame), 0x61U);
    CHECK(bc_recording_frame(&f.recording, reset.id, 1U, frame,
                             sizeof(frame), 81U) == BC_REC_OK);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_READY, 7403U, (const uint8_t[]){1U}, 1U,
                       244U, 82U, false));
    CHECK(pump(&f, 82U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_READY, 7403U);
    CHECK(response != NULL && response->length == 17U &&
          response->payload[4] == BC_REC_OK && f.service.ready);

    /* Explicit READY(false) clears pending live state without unlinking local
     * recording from a connected transport. */
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_READY, 7404U, (const uint8_t[]){0U}, 1U,
                       244U, 90U, false));
    CHECK(pump(&f, 90U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_READY, 7404U);
    CHECK(response != NULL && response->length == 17U &&
          response->payload[4] == BC_REC_OK && !f.service.ready &&
          f.service.live_disabled && f.service.live_prefix_count == 0U &&
          f.recording.link_ready);
    fill_pattern(frame, sizeof(frame), 0x62U);
    CHECK(bc_recording_frame(&f.recording, reset.id, 2U, frame,
                             sizeof(frame), 91U) == BC_REC_OK);
    CHECK(f.recording.snapshot.live_dropped_frames == 1U);
    CHECK(bc_recording_stop(&f.recording, reset.id, 100U) == BC_REC_OK);
    CHECK(bc_recording_drained(&f.recording, reset.id) == BC_REC_OK);

    /* A live ACK/ready lease timeout has the same preview-only effect. */
    CHECK(bc_recording_start(&f.recording, &timeout, 110U) == BC_REC_OK);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_READY, 7405U, (const uint8_t[]){1U}, 1U,
                       244U, 111U, false));
    CHECK(pump(&f, 111U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_READY, 7405U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK &&
          f.service.ready);
    fill_pattern(frame, sizeof(frame), 0x70U);
    CHECK(bc_recording_frame(&f.recording, timeout.id, 1U, frame,
                             sizeof(frame), 112U) == BC_REC_OK);
    (void)pump(&f, 112U, 244U);
    CHECK(f.service.live_count == 1U);
    CHECK(pump(&f, f.service.ready_ms + BC_VOICE_LIVE_STALL_MS + 1U, 244U) == 0U);
    CHECK(!f.service.ready && f.service.live_disabled &&
          f.service.live_prefix_count == 0U && f.recording.link_ready);
    fill_pattern(frame, sizeof(frame), 0x71U);
    CHECK(bc_recording_frame(&f.recording, timeout.id, 2U, frame,
                             sizeof(frame), f.service.now_ms + 1U) == BC_REC_OK);
    CHECK(f.recording.snapshot.live_dropped_frames == 1U);
    CHECK(bc_recording_stop(&f.recording, timeout.id, 120U) == BC_REC_OK);
    CHECK(bc_recording_drained(&f.recording, timeout.id) == BC_REC_OK);

    /* A link loss after a local prefix has been accepted makes the next READY
     * fail explicitly; the partial prefix is never presented as sequence 1. */
    CHECK(bc_recording_start(&f.recording, &reconnect, 130U) == BC_REC_OK);
    fill_pattern(frame, sizeof(frame), 0x80U);
    CHECK(bc_recording_frame(&f.recording, reconnect.id, 1U, frame,
                             sizeof(frame), 131U) == BC_REC_OK);
    bc_voice_service_link(&f.service, 2U, false);
    CHECK(!f.service.connected && !f.recording.link_ready &&
          f.service.live_prefix_count == 0U && f.service.live_disabled);
    bc_voice_service_link(&f.service, 3U, true);
    CHECK(f.service.connected && f.recording.link_ready && !f.service.ready &&
          f.service.live_disabled);
    clear_messages(&f);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_READY, 7406U, (const uint8_t[]){1U}, 1U,
                       244U, 140U, false));
    CHECK(pump(&f, 140U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_READY, 7406U);
    CHECK(response != NULL && response->length == 5U &&
          response->payload[4] == BC_REC_INTERRUPTED && !f.service.ready &&
          f.recording.link_ready);
    CHECK(bc_recording_stop(&f.recording, reconnect.id, 150U) == BC_REC_OK);
    CHECK(bc_recording_drained(&f.recording, reconnect.id) == BC_REC_OK);

    /* The first sequence after a clean new recording is accepted again. */
    CHECK(bc_recording_start(&f.recording, &retry, 160U) == BC_REC_OK);
    CHECK(!f.service.live_disabled && f.service.live_sequence == 1U);
    fill_pattern(frame, sizeof(frame), 0x90U);
    CHECK(bc_recording_frame(&f.recording, retry.id, 1U, frame,
                             sizeof(frame), 161U) == BC_REC_OK);
    CHECK(f.service.live_prefix_count == 1U);

    /* A recording made while disconnected has no resumable live prefix. The
     * first reconnect rejects READY even after the file is terminal, while
     * the durable bytes remain available for the normal archive path. */
    CHECK(bc_recording_stop(&f.recording, retry.id, 170U) == BC_REC_OK);
    CHECK(bc_recording_drained(&f.recording, retry.id) == BC_REC_OK);
    bc_voice_service_link(&f.service, 4U, false);
    CHECK(!f.service.connected && !f.recording.link_ready);
    CHECK(bc_recording_start(&f.recording, &offline, 180U) == BC_REC_OK);
    fill_pattern(frame, sizeof(frame), 0xa0U);
    CHECK(bc_recording_frame(&f.recording, offline.id, 1U, frame,
                             sizeof(frame), 181U) == BC_REC_OK);
    fill_pattern(frame, sizeof(frame), 0xa1U);
    CHECK(bc_recording_frame(&f.recording, offline.id, 2U, frame,
                             sizeof(frame), 182U) == BC_REC_OK);
    CHECK(!f.recording.link_ready && f.service.live_prefix_count == 0U &&
          f.recording.snapshot.accepted_frames == 2U &&
          f.recording.snapshot.live_sent_frames == 0U);
    CHECK(bc_recording_stop(&f.recording, offline.id, 190U) == BC_REC_OK);
    CHECK(bc_recording_drained(&f.recording, offline.id) == BC_REC_OK);
    CHECK(f.recording.snapshot.file.bytes == 2U * BC_REC_FRAME_MAX);
    bc_voice_service_link(&f.service, 5U, true);
    CHECK(f.service.connected && f.recording.link_ready &&
          !f.service.ready && f.service.live_disabled &&
          f.service.live_prefix_count == 0U);
    clear_messages(&f);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_READY, 7407U, (const uint8_t[]){1U}, 1U,
                       244U, 200U, false));
    CHECK(pump(&f, 200U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_READY, 7407U);
    CHECK(response != NULL && response->length == 5U &&
          response->payload[4] == BC_REC_INTERRUPTED && !f.service.ready &&
          f.recording.snapshot.file.bytes == 2U * BC_REC_FRAME_MAX);
    fixture_destroy(&f);
}

static void test_pending_stop_snapshot_and_queue_reservation(void)
{
    fixture f;
    bc_rec_start first = start_value(0x6101U, BC_REC_APP, 0U);
    bc_rec_start second = start_value(0x6102U, BC_REC_APP, 0U);
    bc_rec_start archive = start_value(0x6201U, BC_REC_APP, 0U);
    bc_rec_start replacement = start_value(0x6202U, BC_REC_APP, 0U);
    uint8_t extra[17];
    uint8_t frame[BC_REC_FRAME_MAX];
    const bc_voice_message *response;
    unsigned before;
    unsigned opens;
    unsigned starts;
    unsigned i;

    /* A Stop response can remain queued while the owner has already moved on
     * to a new recording. The service must answer from its cached terminal
     * snapshot, even when all control slots were occupied meanwhile. */
    CHECK(fixture_setup(&f));
    encode_start(extra, &first);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_START, 600U, extra, 13U, 244U, 10U,
                       false));
    CHECK(pump(&f, 10U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_START, 600U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    fill_pattern(frame, sizeof(frame), 0x21U);
    CHECK(bc_recording_frame(&f.recording, first.id, 1U, frame,
                             sizeof(frame), 20U) == BC_REC_OK);
    (void)pump(&f, 20U, 244U);

    encode_id(extra, first.id);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_STOP, 601U, extra, 8U, 20U, 30U,
                       false));
    CHECK(find_response(&f, before, BC_VOICE_STOP, 601U) == NULL);
    CHECK(f.service.stop_pending && !f.service.stop_ready &&
          f.recording.snapshot.phase == BC_REC_STOPPING);
    /* Fill every response slot with unrelated controls before draining the
     * stopped capture. These commands have no owner side effects. */
    for (i = 0U; i < BC_VOICE_CONTROL_SLOTS; ++i)
        CHECK(send_request(&f, BC_VOICE_HELLO, 610U + i, NULL, 0U, 20U,
                           31U + i, false));
    CHECK(f.service.control_count == BC_VOICE_CONTROL_SLOTS);
    fill_pattern(frame, sizeof(frame), 0x41U);
    CHECK(bc_recording_frame(&f.recording, first.id, 2U, frame,
                             sizeof(frame), 40U) == BC_REC_OK);
    CHECK(bc_recording_drained(&f.recording, first.id) == BC_REC_OK);
    CHECK(f.service.stop_ready);

    opens = f.open_count;
    starts = f.capture_start_count;
    CHECK(bc_recording_start(&f.recording, &second, 41U) == BC_REC_OK);
    CHECK(f.recording.snapshot.start.id == second.id &&
          f.recording.snapshot.phase == BC_REC_RECORDING);
    CHECK(f.open_count == opens + 1U && f.capture_start_count == starts + 1U);
    CHECK(pump(&f, 50U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_STOP, 601U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK &&
          response->payload[14] == BC_REC_SAVED &&
          bc_voice_get64(response->payload + 5U) == first.id &&
          bc_voice_get32(response->payload + 25U) == 2U * BC_REC_FRAME_MAX);
    CHECK(!f.service.stop_pending && !f.service.stop_ready);
    CHECK(f.recording.snapshot.start.id == second.id &&
          bc_recording_active(&f.recording));
    CHECK(bc_recording_stop(&f.recording, second.id, 60U) == BC_REC_OK);
    CHECK(bc_recording_drained(&f.recording, second.id) == BC_REC_EMPTY_AUDIO);
    fixture_destroy(&f);

    /* While staged verification is active, the service reserves two response
     * slots before a command that could cancel it and start capture. With
     * only one slot free, the request is ignored before any side effect. */
    CHECK(fixture_setup(&f));
    CHECK(record_complete(&f, &archive, 2U, 0x70U, NULL, 70U));
    CHECK(pump(&f, 100U, 244U) != 0U);
    clear_messages(&f);
    encode_resume(extra, archive.id, 0U, 0x620001U);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_RESUME, 620U, extra, 16U, 244U, 110U,
                       false));
    CHECK(f.service.verifying && f.service.reader.open);
    for (i = 0U; i < BC_VOICE_CONTROL_SLOTS - 1U; ++i)
        CHECK(send_request(&f, BC_VOICE_HELLO, 621U + i, NULL, 0U, 20U,
                           111U + i, false));
    CHECK(f.service.control_count == BC_VOICE_CONTROL_SLOTS - 1U);
    opens = f.open_count;
    starts = f.capture_start_count;
    encode_start(extra, &replacement);
    CHECK(send_request(&f, BC_VOICE_START, 625U, extra, 13U, 244U, 115U,
                       false));
    CHECK(f.service.control_count == BC_VOICE_CONTROL_SLOTS - 1U);
    CHECK(f.service.verifying && f.service.reader.open);
    CHECK(f.open_count == opens && f.capture_start_count == starts);
    CHECK(find_response(&f, before, BC_VOICE_START, 625U) == NULL);
    CHECK(bc_voice_service_cancel_archive(&f.service) == BC_REC_OK);
    CHECK(!f.service.verifying && !f.service.reader.open);
    fixture_destroy(&f);
}

static void test_empty_start_retry_query_and_remount(void)
{
    fixture f;
    bc_rec_start start = start_value(0x6301U, BC_REC_APP, 0U);
    uint8_t extra[17];
    const bc_voice_message *response;
    unsigned before;
    unsigned opens;
    unsigned starts;

    CHECK(fixture_setup(&f));
    encode_start(extra, &start);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_START, 630U, extra, 13U, 244U, 10U,
                       false));
    CHECK(pump(&f, 10U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_START, 630U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK &&
          response->payload[14] == BC_REC_RECORDING);
    opens = f.open_count;
    starts = f.capture_start_count;
    encode_id(extra, start.id);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_STOP, 631U, extra, 8U, 20U, 20U,
                       false));
    CHECK(f.service.stop_pending);
    CHECK(bc_recording_drained(&f.recording, start.id) == BC_REC_EMPTY_AUDIO);
    CHECK(pump(&f, 21U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_STOP, 631U);
    CHECK(response != NULL && response->payload[4] == BC_REC_EMPTY_AUDIO &&
          response->payload[14] == BC_REC_EMPTY &&
          response->payload[15] == BC_REC_EMPTY_AUDIO &&
          bc_voice_get32(response->payload + 25U) == 0U &&
          bc_voice_get32(response->payload + 29U) == 0U);

    /* The current owner replays the same empty identity without reopening or
     * starting the microphone. Query reports the terminal EMPTY snapshot. */
    encode_start(extra, &start);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_START, 633U, extra, 13U, 20U, 31U,
                       false));
    CHECK(pump(&f, 31U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_START, 633U);
    CHECK(response != NULL && response->payload[4] == BC_REC_EMPTY_AUDIO &&
          response->payload[14] == BC_REC_EMPTY &&
          response->payload[15] == BC_REC_EMPTY_AUDIO);
    CHECK(f.open_count == opens && f.capture_start_count == starts);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_QUERY, 634U, extra, 8U, 244U, 32U,
                       false));
    CHECK(pump(&f, 32U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_QUERY, 634U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK &&
          response->payload[14] == BC_REC_EMPTY &&
          response->payload[15] == BC_REC_EMPTY_AUDIO);

    /* After a remount, persisted EMPTY metadata is still replay-only and
     * cannot cause a capture-start side effect. */
    CHECK(fixture_remount(&f, 2U));
    opens = f.open_count;
    starts = f.capture_start_count;
    encode_start(extra, &start);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_START, 635U, extra, 13U, 244U, 40U,
                       false));
    CHECK(pump(&f, 40U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_START, 635U);
    CHECK(response != NULL && response->payload[4] == BC_REC_EMPTY_AUDIO &&
          response->payload[14] == BC_REC_EMPTY &&
          response->payload[15] == BC_REC_EMPTY_AUDIO);
    CHECK(f.open_count == opens + 1U && f.capture_start_count == starts);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_STOP, 636U, extra, 8U, 20U, 41U,
                       false));
    CHECK(pump(&f, 41U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_STOP, 636U);
    CHECK(response != NULL && response->payload[4] == BC_REC_EMPTY_AUDIO &&
          response->payload[14] == BC_REC_EMPTY);
    fixture_destroy(&f);
}

static void test_resume_window_retry_and_crc(void)
{
    fixture f;
    bc_rec_start start = start_value(0x3333U, BC_REC_APP, 0U);
    uint8_t raw[8U * BC_REC_FRAME_MAX];
    uint8_t extra[16];
    uint32_t token = 0xabc001U;
    uint32_t offset = BC_REC_FRAME_MAX;
    uint32_t file_bytes;
    uint32_t file_crc;
    unsigned before;
    unsigned i;
    unsigned files;
    const bc_voice_message *response;
    bc_rec_snapshot snapshot;

    CHECK(fixture_setup(&f));
    CHECK(record_complete(&f, &start, 8U, 0x10U, raw, 10U));
    snapshot = *bc_recording_snapshot(&f.recording);
    file_bytes = snapshot.file.bytes;
    file_crc = snapshot.file.crc32;
    CHECK(file_bytes == sizeof(raw));
    CHECK(file_crc == bc_voice_crc32(raw, sizeof(raw)));
    /* Completion publishes a pending state event; drain it before staging a
     * verification assertion so the first archive poll is attributable to
     * the bounded reader step. */
    (void)pump(&f, 50U, 244U);
    clear_messages(&f);

    encode_resume(extra, start.id, offset, token);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_RESUME, 300U, extra, sizeof(extra), 244U,
                       100U, false));
    CHECK(f.service.verifying && f.service.reader.open);
    /* A 1024-byte verification step is deliberately staged. */
    CHECK(bc_voice_service_poll(&f.service, 100U, 244U) == false);
    CHECK(f.service.verifying && find_response(&f, before, BC_VOICE_RESUME,
                                               300U) == NULL);
    CHECK(bc_voice_service_poll(&f.service, 101U, 244U));
    CHECK(pump(&f, 101U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_RESUME, 300U);
    CHECK(response != NULL && response->length == 29U &&
          response->payload[4] == BC_REC_OK);
    CHECK(response != NULL && bc_voice_get64(response->payload + 5) == start.id &&
          bc_voice_get32(response->payload + 13) == token &&
          bc_voice_get32(response->payload + 17) == file_bytes &&
          bc_voice_get32(response->payload + 21) == file_crc &&
          bc_voice_get32(response->payload + 25) == offset);
    CHECK(f.service.transferring && !f.service.verifying);

    files = 0U;
    for (i = 0U; i < f.sink.message_count; ++i) {
        const bc_voice_message *message = &f.sink.messages[i];
        uint32_t message_offset;
        uint32_t message_length;
        if (message->kind != BC_VOICE_FILE)
            continue;
        ++files;
        CHECK(message->length >= 8U && message->length <= 8U + BC_REC_FRAME_MAX);
        CHECK(bc_voice_get32(message->payload) == token);
        message_offset = bc_voice_get32(message->payload + 4);
        message_length = message->length - 8U;
        CHECK(message_offset >= offset &&
              message_offset + message_length <= file_bytes);
        CHECK(memcmp(message->payload + 8, raw + message_offset,
                     message_length) == 0);
    }
    CHECK(files == BC_VOICE_TRANSFER_WINDOW);
    CHECK(f.service.transfer_count == BC_VOICE_TRANSFER_WINDOW);
    CHECK(f.sink.invalid_packets == 0U);

    /* ACKing an exact later window boundary retires all earlier boundaries. */
    before = f.sink.message_count;
    encode_ack(extra, token, offset + 3U * BC_REC_FRAME_MAX);
    CHECK(send_request(&f, BC_VOICE_TRANSFER_ACK, 301U, extra, 8U, 20U,
                       200U, false));
    CHECK(pump(&f, 200U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_TRANSFER_ACK, 301U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    CHECK(f.service.transfer_ack == offset + 3U * BC_REC_FRAME_MAX);
    /* Replaying the same durable boundary is harmless and does not rewind
     * the transfer window. */
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_TRANSFER_ACK, 3011U, extra, 8U, 20U,
                       250U, false));
    CHECK(pump(&f, 250U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_TRANSFER_ACK, 3011U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    CHECK(f.service.transfer_ack == offset + 3U * BC_REC_FRAME_MAX);
    encode_ack(extra, token, f.service.transfer_ack + 1U);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_TRANSFER_ACK, 302U, extra, 8U, 244U,
                       300U, false));
    CHECK(pump(&f, 300U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_TRANSFER_ACK, 302U);
    CHECK(response != NULL && response->payload[4] == BC_REC_INVALID);
    CHECK(f.service.transfer_ack == offset + 3U * BC_REC_FRAME_MAX);

    /* No ACK arrives for the remaining window: retry starts at the last
     * acknowledged byte and never at a speculative transfer_next. */
    CHECK(f.service.transfer_count != 0U);
    before = f.sink.message_count;
    CHECK(bc_voice_service_poll(&f.service, 300U + BC_VOICE_RETRY_MS,
                                244U));
    CHECK(f.sink.message_count > before);
    response = NULL;
    for (i = before; i < f.sink.message_count; ++i)
        if (f.sink.messages[i].kind == BC_VOICE_FILE) {
            response = &f.sink.messages[i];
            break;
        }
    CHECK(response != NULL && bc_voice_get32(response->payload + 4) ==
          offset + 3U * BC_REC_FRAME_MAX);

    /* Cancel closes the staged reader and stops further FILE events. */
    before = f.sink.message_count;
    bc_voice_put32(extra, token);
    CHECK(send_request(&f, BC_VOICE_CANCEL, 303U, extra, 4U, 20U, 400U,
                       false));
    CHECK(pump(&f, 400U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_CANCEL, 303U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    CHECK(!f.service.verifying && !f.service.transferring &&
          !f.service.reader.open && !f.store.reader_active);
    fixture_destroy(&f);
}

/* Leave a verified transfer immediately after its handshake response, before
 * the first FILE. Verification remains bounded even when it needs >1 poll. */
static void begin_verified_transfer(fixture *f, uint64_t id, uint32_t offset,
                                    uint32_t token, uint32_t now_ms)
{
    uint8_t extra[16];
    unsigned polls = 0U;
    encode_resume(extra, id, offset, token);
    CHECK(send_request(f, BC_VOICE_RESUME, 800U + token, extra, sizeof(extra),
                       244U, now_ms, false));
    while (f->service.verifying && polls++ < 32U)
        (void)bc_voice_service_poll(&f->service, now_ms, 244U);
    CHECK(f->service.transferring && f->service.reader.open);
    CHECK(!f->service.tx_active && f->service.transfer_count == 0U);
}

static void expect_retired_resume(fixture *f, uint64_t id, uint32_t offset,
                                   uint32_t token, uint32_t now_ms,
                                   bc_rec_result expected)
{
    uint8_t extra[16];
    unsigned before = f->sink.message_count;
    unsigned reads = f->ram.read_calls;
    const bc_voice_message *response;
    encode_resume(extra, id, offset, token);
    CHECK(send_request(f, BC_VOICE_RESUME, 899U, extra, sizeof(extra), 244U,
                       now_ms, false));
    (void)pump(f, now_ms, 244U);
    response = find_response(f, before, BC_VOICE_RESUME, 899U);
    CHECK(response != NULL && response->payload[4] == expected);
    CHECK(!f->service.verifying && !f->service.transferring && !f->service.reader.open);
    CHECK(f->ram.read_calls == reads);
}

static void test_archive_retired_tokens_and_active_retry(void)
{
    fixture f;
    bc_rec_start start = start_value(0x3336U, BC_REC_APP, 0U);
    uint8_t extra[16];
    unsigned before;
    const bc_voice_message *response;

    CHECK(fixture_setup(&f));
    CHECK(record_complete(&f, &start, 2U, 0x81U, NULL, 10U));
    (void)pump(&f, 50U, 244U);
    begin_verified_transfer(&f, start.id, 0U, 1U, 100U);
    encode_resume(extra, start.id, 0U, 1U);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_RESUME, 890U, extra, sizeof(extra), 244U, 101U, false));
    (void)pump(&f, 101U, 244U);
    response = find_response(&f, before, BC_VOICE_RESUME, 890U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    CHECK(f.service.transferring && f.service.archive_progress_ms == 100U);
    bc_voice_put32(extra, 1U);
    CHECK(send_request(&f, BC_VOICE_CANCEL, 891U, extra, 4U, 244U, 102U, false));
    (void)pump(&f, 102U, 244U);
    expect_retired_resume(&f, start.id, 0U, 1U, 103U, BC_REC_CANCELLED);
    expect_retired_resume(&f, start.id + 1U, 0U, 1U, 104U, BC_REC_INVALID);
    expect_retired_resume(&f, start.id, BC_REC_FRAME_MAX, 1U, 105U, BC_REC_INVALID);
    begin_verified_transfer(&f, start.id, 0U, 2U, 110U);
    (void)pump(&f, 110U, 244U);
    encode_ack(extra, 2U, 2U * BC_REC_FRAME_MAX);
    CHECK(send_request(&f, BC_VOICE_TRANSFER_ACK, 892U, extra, 8U, 244U, 111U, false));
    (void)pump(&f, 111U, 244U);
    expect_retired_resume(&f, start.id, 0U, 2U, 112U, BC_REC_CANCELLED);
    begin_verified_transfer(&f, start.id, BC_REC_FRAME_MAX, 3U, 120U);
    CHECK(bc_voice_service_cancel_archive(&f.service) == BC_REC_OK);
    fixture_destroy(&f);
}

static void test_archive_fragment_burst_and_control_priority(void)
{
    fixture f;
    bc_rec_start start = start_value(0x3334U, BC_REC_APP, 0U);
    uint8_t raw[8U * BC_REC_FRAME_MAX];
    unsigned reads, packets, polls = 0U;

    CHECK(fixture_setup(&f));
    CHECK(record_complete(&f, &start, 8U, 0x21U, raw, 10U));
    (void)pump(&f, 50U, 244U);
    begin_verified_transfer(&f, start.id, 0U, 1U, 100U);
    clear_messages(&f);
    /* Reject the third fragment of a burst: only the accepted prefix moves. */
    f.sink.fail_packet = 3U;
    CHECK(bc_voice_service_poll(&f.service, 101U, 20U));
    CHECK(f.sink.packet_count == 3U && f.service.tx_offset == 16U);
    CHECK(f.service.tx_active && f.service.tx.kind == BC_VOICE_FILE);
    reads = f.ram.read_calls;
    CHECK(send_request(&f, BC_VOICE_HELLO, 820U, NULL, 0U, 244U, 102U, false));
    CHECK(f.service.control_count == 1U);
    packets = f.sink.packet_count;
    CHECK(bc_voice_service_poll(&f.service, 106U, 20U));
    CHECK(f.sink.packet_count - packets == BC_VOICE_TX_BURST);
    CHECK(f.service.tx_offset == 48U && f.ram.read_calls == reads);
    while (f.service.tx_active && polls++ < 32U) {
        packets = f.sink.packet_count;
        CHECK(bc_voice_service_poll(&f.service, 106U + 5U * polls, 20U));
        CHECK(f.sink.packet_count - packets <= BC_VOICE_TX_BURST);
        CHECK(f.ram.read_calls == reads);
    }
    CHECK(!f.service.tx_active && f.service.control_count == 1U);
    CHECK(f.sink.message_count == 1U && f.sink.invalid_packets == 0U);
    CHECK(f.sink.messages[0].kind == BC_VOICE_FILE);
    CHECK(memcmp(f.sink.messages[0].payload + 8U, raw, BC_REC_FRAME_MAX) == 0);
    CHECK(f.service.transfer_count == 1U);
    /* Completion stops the burst. The queued control wins the next poll. */
    CHECK(bc_voice_service_poll(&f.service, 200U, 244U));
    CHECK(f.sink.message_count == 2U && f.sink.messages[1].kind == BC_VOICE_HELLO);
    CHECK(f.ram.read_calls == reads && f.service.transfer_count == 1U);
    packets = f.sink.packet_count;
    CHECK(bc_voice_service_poll(&f.service, 205U, 244U));
    CHECK(f.sink.packet_count == packets + 1U && f.sink.message_count == 3U);
    CHECK(f.sink.messages[2].kind == BC_VOICE_FILE && f.service.transfer_count == 2U);
    CHECK(memcmp(f.sink.messages[2].payload + 8U, raw + BC_REC_FRAME_MAX,
                  BC_REC_FRAME_MAX) == 0);
    polls = 0U;
    packets = f.sink.packet_count;
    do {
        CHECK(bc_voice_service_poll(&f.service, 210U + 5U * polls, 20U));
        ++polls;
    } while (f.service.tx_active && polls < 32U);
    CHECK(polls == 8U && f.sink.packet_count - packets == 29U);
    CHECK(f.sink.message_count == 4U && f.sink.messages[3].kind == BC_VOICE_FILE);
    CHECK(memcmp(f.sink.messages[3].payload + 8U, raw + 2U * BC_REC_FRAME_MAX,
                  BC_REC_FRAME_MAX) == 0);
    CHECK(bc_voice_service_cancel_archive(&f.service) == BC_REC_OK);
    fixture_destroy(&f);
}

static void test_archive_delayed_ack_during_retransmit(void)
{
    fixture f;
    bc_rec_start start = start_value(0x3335U, BC_REC_APP, 0U);
    uint8_t extra[8];
    uint32_t ack = 5U * BC_REC_FRAME_MAX;
    unsigned polls = 0U, before;
    const bc_voice_message *response;

    CHECK(fixture_setup(&f));
    CHECK(record_complete(&f, &start, 8U, 0x31U, NULL, 10U));
    (void)pump(&f, 50U, 244U);
    begin_verified_transfer(&f, start.id, 0U, 1U, 100U);
    CHECK(pump(&f, 100U, 244U) == BC_VOICE_TRANSFER_WINDOW);
    CHECK(f.service.transfer_count == BC_VOICE_TRANSFER_WINDOW);
    CHECK(bc_voice_service_poll(&f.service, 100U + BC_VOICE_RETRY_MS, 20U));
    CHECK(f.service.tx_active && f.service.tx.kind == BC_VOICE_FILE);
    CHECK(bc_voice_get32(f.service.tx.payload + 4U) == 0U);
    CHECK(f.service.transfer_count == BC_VOICE_TRANSFER_WINDOW);
    /* A late ACK for the original fifth chunk overtakes an in-flight replay
     * of the first. Finish that framing without re-adding an obsolete end. */
    before = f.sink.message_count;
    encode_ack(extra, 1U, ack);
    CHECK(send_request(&f, BC_VOICE_TRANSFER_ACK, 830U, extra, sizeof(extra),
                       244U, 1601U, false));
    CHECK(f.service.transfer_ack == ack && f.service.transfer_next == ack);
    CHECK(f.service.transfer_count == 1U &&
          f.service.transfer_ends[0] == 6U * BC_REC_FRAME_MAX);
    while (f.service.tx_active && polls++ < 32U)
        CHECK(bc_voice_service_poll(&f.service, 1602U, 20U));
    CHECK(!f.service.tx_active && f.service.transfer_next == ack);
    CHECK(f.service.transfer_count == 1U);
    (void)pump(&f, 1602U, 244U);
    response = find_response(&f, before, BC_VOICE_TRANSFER_ACK, 830U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    CHECK(f.service.transfer_count == 3U);
    CHECK(f.service.transfer_ends[0] == 6U * BC_REC_FRAME_MAX &&
          f.service.transfer_ends[1] == 7U * BC_REC_FRAME_MAX &&
          f.service.transfer_ends[2] == 8U * BC_REC_FRAME_MAX);
    CHECK(f.service.transfer_next == 8U * BC_REC_FRAME_MAX);
    CHECK(f.sink.invalid_packets == 0U);
    /* The original last boundary remains valid, and the final new boundary
     * closes the reader normally after all bytes have been acknowledged. */
    encode_ack(extra, 1U, 6U * BC_REC_FRAME_MAX);
    CHECK(send_request(&f, BC_VOICE_TRANSFER_ACK, 831U, extra, sizeof(extra),
                       244U, 1700U, false));
    CHECK(f.service.transfer_count == 2U &&
          f.service.transfer_next == 8U * BC_REC_FRAME_MAX);
    encode_ack(extra, 1U, 8U * BC_REC_FRAME_MAX);
    CHECK(send_request(&f, BC_VOICE_TRANSFER_ACK, 832U, extra, sizeof(extra),
                       244U, 1701U, false));
    CHECK(!f.service.transferring && !f.service.reader.open && !f.store.reader_active);
    (void)pump(&f, 1701U, 244U);
    /* A delayed final ACK can also arrive while the first replay fragment
     * is active. Close normally and allow its response to replace that body. */
    begin_verified_transfer(&f, start.id, 2U * BC_REC_FRAME_MAX, 2U, 1800U);
    (void)pump(&f, 1800U, 244U);
    CHECK(f.service.transfer_count == BC_VOICE_TRANSFER_WINDOW);
    CHECK(bc_voice_service_poll(&f.service, 1800U + BC_VOICE_RETRY_MS, 20U));
    CHECK(f.service.tx_active && f.service.tx.kind == BC_VOICE_FILE);
    encode_ack(extra, 2U, 8U * BC_REC_FRAME_MAX);
    CHECK(send_request(&f, BC_VOICE_TRANSFER_ACK, 833U, extra, sizeof(extra),
                       244U, 3301U, false));
    CHECK(!f.service.reader.open && !f.service.tx_active);
    (void)pump(&f, 3301U, 244U);
    CHECK(f.sink.invalid_packets == 0U);
    fixture_destroy(&f);
}

static void test_archive_stall_expiry_and_resume(void)
{
    unsigned mode;
    for (mode = 0U; mode < 5U; ++mode) {
        fixture f;
        bc_rec_start start = start_value(0x3340U + mode, BC_REC_APP, 0U);
        bc_rec_start stored;
        bc_rec_file file;
        uint8_t raw[8U * BC_REC_FRAME_MAX], reread[BC_REC_FRAME_MAX], extra[16];
        uint32_t at = mode == 4U ? UINT32_MAX - 10000U : 100U;
        uint32_t offset = mode == 3U ? sizeof(raw) : 0U;
        uint32_t elapsed;

        CHECK(fixture_setup(&f));
        CHECK(record_complete(&f, &start, 8U, 0x41U, raw, 10U));
        (void)pump(&f, 50U, 244U);
        begin_verified_transfer(&f, start.id, offset, 1U, at);
        if (mode == 1U) f.sink.fail_all = true; /* No first FILE accepted. */
        if (mode == 2U) {
            CHECK(bc_voice_service_poll(&f.service, at, 20U));
            CHECK(f.service.tx_active && f.service.tx.kind == BC_VOICE_FILE);
            f.sink.fail_all = true; /* Block a partially framed FILE. */
        }
        for (elapsed = 0U; elapsed < BC_VOICE_ARCHIVE_STALL_MS;
             elapsed += BC_VOICE_RETRY_MS) {
            (void)pump(&f, at + elapsed, 244U);
            CHECK(f.service.transferring && f.service.reader.open);
            CHECK(f.service.transfer_count <= BC_VOICE_TRANSFER_WINDOW);
            CHECK(f.service.archive_progress_ms == at);
        }
        CHECK(!bc_voice_service_poll(&f.service, at + BC_VOICE_ARCHIVE_STALL_MS, 244U));
        CHECK(!f.service.transferring && !f.service.verifying && !f.service.reader.open);
        CHECK(!f.store.reader_active && !f.service.tx_active);
        f.sink.fail_all = false;
        for (elapsed = 1U; elapsed <= 3U; ++elapsed)
            expect_retired_resume(&f, start.id, offset, 1U,
                at + BC_VOICE_ARCHIVE_STALL_MS + elapsed, BC_REC_CANCELLED);
        /* Expiry changes no raw data or custody. A fresh token can resume
         * the same file and capture can start again after cancelling it. */
        CHECK(bc_rec_store_stat(&f.store, start.id, &stored, &file) == BC_REC_STORE_OK);
        CHECK(file.bytes == sizeof(raw) && file.crc32 == bc_voice_crc32(raw, sizeof(raw)));
        CHECK(file.complete && !file.delivered);
        CHECK(bc_rec_store_read(&f.store, start.id, 0U, reread, sizeof(reread)) ==
              (int32_t)sizeof(reread));
        CHECK(memcmp(raw, reread, sizeof(reread)) == 0);
        f.sink.fail_all = false;
        clear_messages(&f);
        begin_verified_transfer(&f, start.id, BC_REC_FRAME_MAX, 2U,
                                 at + BC_VOICE_ARCHIVE_STALL_MS + 10U);
        CHECK(bc_voice_service_poll(&f.service, at + BC_VOICE_ARCHIVE_STALL_MS + 11U, 244U));
        CHECK(f.service.transfer_next == 2U * BC_REC_FRAME_MAX);
        bc_voice_put32(extra, 2U);
        CHECK(send_request(&f, BC_VOICE_CANCEL, 840U, extra, 4U, 244U,
                           at + BC_VOICE_ARCHIVE_STALL_MS + 12U, false));
        start.id += 100U;
        CHECK(record_complete(&f, &start, 1U, 0x51U, NULL,
                               at + BC_VOICE_ARCHIVE_STALL_MS + 13U));
        fixture_destroy(&f);
    }
}

static void test_archive_only_real_progress_renews_stall(void)
{
    fixture f;
    bc_rec_start start = start_value(0x3350U, BC_REC_APP, 0U);
    uint8_t extra[16];
    uint32_t progress = 100U + BC_VOICE_ARCHIVE_STALL_MS - 1U;

    CHECK(fixture_setup(&f));
    CHECK(record_complete(&f, &start, 8U, 0x61U, NULL, 10U));
    (void)pump(&f, 50U, 244U);
    begin_verified_transfer(&f, start.id, 0U, 1U, 100U);
    (void)pump(&f, 100U, 244U);
    encode_ack(extra, 1U, BC_REC_FRAME_MAX);
    CHECK(send_request(&f, BC_VOICE_TRANSFER_ACK, 850U, extra, 8U, 244U,
                       progress, false));
    CHECK(f.service.archive_progress_ms == progress);
    (void)pump(&f, progress, 244U);
    /* A duplicate ACK, invalid boundary, wrong token and duplicate Resume
     * are all traffic, but none advances the durable checkpoint. */
    CHECK(send_request(&f, BC_VOICE_TRANSFER_ACK, 851U, extra, 8U, 244U,
                       progress + 1U, false));
    (void)pump(&f, progress + 1U, 244U);
    encode_ack(extra, 1U, BC_REC_FRAME_MAX + 1U);
    CHECK(send_request(&f, BC_VOICE_TRANSFER_ACK, 852U, extra, 8U, 244U,
                       progress + 2U, false));
    (void)pump(&f, progress + 2U, 244U);
    encode_ack(extra, 2U, 2U * BC_REC_FRAME_MAX);
    CHECK(send_request(&f, BC_VOICE_TRANSFER_ACK, 853U, extra, 8U, 244U,
                       progress + 3U, false));
    (void)pump(&f, progress + 3U, 244U);
    encode_resume(extra, start.id, 0U, 1U);
    CHECK(send_request(&f, BC_VOICE_RESUME, 854U, extra, sizeof(extra), 244U,
                       progress + BC_VOICE_ARCHIVE_STALL_MS - 1U, false));
    (void)pump(&f, progress + BC_VOICE_ARCHIVE_STALL_MS - 1U, 244U);
    CHECK(f.service.archive_progress_ms == progress && f.service.reader.open);
    (void)pump(&f, progress + BC_VOICE_ARCHIVE_STALL_MS, 244U);
    CHECK(!f.service.reader.open && !f.service.transferring);
    fixture_destroy(&f);
}

static void test_archive_verification_stall_and_progress(void)
{
    fixture f;
    bc_rec_start start = start_value(0x3351U, BC_REC_APP, 0U);
    uint8_t extra[16];
    uint32_t progress = 100U + BC_VOICE_ARCHIVE_STALL_MS - 1U;
    unsigned i;

    CHECK(fixture_setup(&f));
    CHECK(record_complete(&f, &start, 16U, 0x71U, NULL, 10U));
    (void)pump(&f, 50U, 244U);
    encode_resume(extra, start.id, 0U, 1U);
    CHECK(send_request(&f, BC_VOICE_RESUME, 860U, extra, sizeof(extra), 244U, 100U, false));
    /* An actual bounded CRC step just before the deadline renews verification
     * independently of the total time needed to scan a long archive. */
    (void)bc_voice_service_poll(&f.service, progress, 244U);
    CHECK(f.service.verifying && f.service.reader.verify_offset == 1024U);
    CHECK(f.service.archive_progress_ms == progress);
    f.sink.fail_all = true;
    for (i = 0U; i < BC_VOICE_CONTROL_SLOTS; ++i)
        CHECK(send_request(&f, BC_VOICE_HELLO, 861U + i, NULL, 0U, 244U,
                           progress + 1U, false));
    /* The first blocked poll moves one control into tx; refill that slot so
     * all subsequent verification steps are starved of response capacity. */
    CHECK(!bc_voice_service_poll(&f.service, progress + 1U, 244U));
    CHECK(send_request(&f, BC_VOICE_HELLO, 865U, NULL, 0U, 244U, progress + 1U, false));
    encode_resume(extra, start.id, 0U, 1U);
    CHECK(send_request(&f, BC_VOICE_RESUME, 866U, extra, sizeof(extra), 244U,
                       progress + BC_VOICE_ARCHIVE_STALL_MS - 1U, false));
    CHECK(!bc_voice_service_poll(&f.service, progress + BC_VOICE_ARCHIVE_STALL_MS - 1U, 244U));
    CHECK(f.service.reader.verify_offset == 1024U && f.service.reader.open);
    CHECK(!bc_voice_service_poll(&f.service, progress + BC_VOICE_ARCHIVE_STALL_MS, 244U));
    CHECK(!f.service.verifying && !f.service.reader.open && !f.store.reader_active);
    fixture_destroy(&f);
}

static void test_query_catalog_receipt_and_id_custody(void)
{
    fixture f;
    bc_rec_start first = start_value(0x4001U, BC_REC_PTT, 0U);
    bc_rec_start second = start_value(0x4002U, BC_REC_MEMO, 0U);
    uint8_t first_raw[BC_REC_FRAME_MAX];
    uint8_t second_raw[BC_REC_FRAME_MAX];
    uint8_t extra[17];
    bc_rec_snapshot snapshot;
    bc_rec_start stored_start;
    bc_rec_file stored_file;
    const bc_voice_message *response;
    unsigned before;
    int32_t read_count;

    CHECK(fixture_setup(&f));
    CHECK(record_complete(&f, &first, 1U, 0x50U, first_raw, 10U));
    CHECK(record_complete(&f, &second, 1U, 0x60U, second_raw, 20U));
    snapshot = *bc_recording_snapshot(&f.recording);
    CHECK(snapshot.start.id == second.id && snapshot.file.complete);

    /* RECEIPT commits storage custody first, then reflects the same terminal
     * transition in the current owner snapshot. A retry of Start and a
     * current-owner Query must replay DELIVERED without capture. */
    encode_receipt(extra, second.id, BC_REC_FRAME_MAX,
                   bc_voice_crc32(second_raw, sizeof(second_raw)), false);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_RECEIPT, 399U, extra, 17U, 20U, 25U,
                       false));
    CHECK(pump(&f, 25U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_RECEIPT, 399U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    snapshot = *bc_recording_snapshot(&f.recording);
    CHECK(snapshot.start.id == second.id && snapshot.phase == BC_REC_DELIVERED &&
          snapshot.file.delivered);
    CHECK(f.service.latest.start.id == second.id &&
          f.service.latest.phase == BC_REC_DELIVERED &&
          f.service.latest.file.delivered);
    {
        unsigned starts = f.capture_start_count;
        uint8_t start_extra[13];
        encode_start(start_extra, &second);
        before = f.sink.message_count;
        CHECK(send_request(&f, BC_VOICE_START, 398U, start_extra, 13U,
                           244U, 27U, false));
        CHECK(pump(&f, 27U, 244U) != 0U);
        response = find_response(&f, before, BC_VOICE_START, 398U);
        CHECK(response != NULL && response->payload[4] == BC_REC_OK &&
              response->payload[14] == BC_REC_DELIVERED &&
              (response->payload[16] & 4U) != 0U);
        CHECK(f.capture_start_count == starts);
    }

    encode_id(extra, first.id);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_QUERY, 400U, extra, 8U, 20U, 30U, false));
    CHECK(pump(&f, 30U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_QUERY, 400U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK &&
          response->payload[14] == BC_REC_SAVED &&
          bc_voice_get32(response->payload + 25) == BC_REC_FRAME_MAX);

    encode_id(extra, 0xdeadU);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_QUERY, 401U, extra, 8U, 244U, 40U, false));
    CHECK(pump(&f, 40U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_QUERY, 401U);
    CHECK(response != NULL && response->payload[4] == BC_REC_NOT_FOUND);

    bc_voice_put64(extra, 0U);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_CATALOG, 402U, extra, 8U, 20U, 50U,
                       false));
    CHECK(pump(&f, 50U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_CATALOG, 402U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK &&
          bc_voice_get64(response->payload + 5) == first.id);
    bc_voice_put64(extra, first.id);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_CATALOG, 403U, extra, 8U, 244U, 60U,
                       false));
    CHECK(pump(&f, 60U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_CATALOG, 403U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK &&
          bc_voice_get64(response->payload + 5) == second.id);
    bc_voice_put64(extra, second.id);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_CATALOG, 404U, extra, 8U, 20U, 70U,
                       false));
    CHECK(pump(&f, 70U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_CATALOG, 404U);
    CHECK(response != NULL && response->payload[4] == BC_REC_NOT_FOUND &&
          bc_voice_get64(response->payload + 5) == 0U);

    /* Wrong custody data never retires raw bytes. */
    encode_receipt(extra, first.id, BC_REC_FRAME_MAX + 1U,
                   snapshot.file.crc32, false);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_RECEIPT, 405U, extra, 17U, 244U, 80U,
                       false));
    CHECK(pump(&f, 80U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_RECEIPT, 405U);
    CHECK(response != NULL && response->payload[4] == BC_REC_CUSTODY_REQUIRED);
    read_count = bc_rec_store_read(&f.store, first.id, 0U, first_raw,
                                   sizeof(first_raw));
    CHECK(read_count == (int32_t)sizeof(first_raw));

    encode_receipt(extra, first.id, BC_REC_FRAME_MAX,
                   bc_voice_crc32(first_raw, sizeof(first_raw)), false);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_RECEIPT, 406U, extra, 17U, 20U, 90U,
                       false));
    CHECK(pump(&f, 90U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_RECEIPT, 406U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    CHECK(bc_rec_store_lookup(&f.store, first.id, &stored_start, &stored_file) ==
          BC_REC_STORE_OK && stored_file.delivered);
    CHECK(bc_rec_store_read(&f.store, first.id, 0U, first_raw,
                            sizeof(first_raw)) == (int32_t)sizeof(first_raw));

    encode_receipt(extra, first.id, BC_REC_FRAME_MAX, stored_file.crc32, true);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_RECEIPT, 407U, extra, 17U, 244U, 100U,
                       false));
    CHECK(pump(&f, 100U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_RECEIPT, 407U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    CHECK(bc_rec_store_read(&f.store, first.id, 0U, first_raw,
                            sizeof(first_raw)) == BC_REC_STORE_DELETED);
    CHECK(bc_rec_store_lookup(&f.store, first.id, &stored_start, &stored_file) ==
          BC_REC_STORE_OK && stored_file.delivered);

    /* Tombstone replay is metadata-only and never starts capture. */
    {
        unsigned starts = f.capture_start_count;
        bc_rec_start replay = first;
        encode_start(extra, &replay);
        before = f.sink.message_count;
        CHECK(send_request(&f, BC_VOICE_START, 408U, extra, 13U, 20U, 110U,
                           false));
        CHECK(pump(&f, 110U, 20U) != 0U);
        response = find_response(&f, before, BC_VOICE_START, 408U);
        CHECK(response != NULL && response->payload[4] == BC_REC_OK &&
              response->payload[14] == BC_REC_DELIVERED &&
              (response->payload[16] & 4U) != 0U);
        CHECK(f.capture_start_count == starts);
        replay.trigger = BC_REC_APP;
        encode_start(extra, &replay);
        before = f.sink.message_count;
        CHECK(send_request(&f, BC_VOICE_START, 409U, extra, 13U, 244U, 120U,
                           false));
        CHECK(pump(&f, 120U, 244U) != 0U);
        response = find_response(&f, before, BC_VOICE_START, 409U);
        CHECK(response != NULL && response->payload[4] == BC_REC_INVALID);
        CHECK(f.capture_start_count == starts);
    }
    fixture_destroy(&f);
}

static void test_settings_errors_and_archive_cancellation(void)
{
    fixture f;
    bc_rec_start start = start_value(0x5001U, BC_REC_PTT, 0U);
    uint8_t extra[17];
    uint8_t settings_extra[11];
    uint8_t start_extra[13];
    uint8_t frame[BC_REC_FRAME_MAX];
    const bc_voice_message *response;
    unsigned before;
    unsigned settings_calls;

    CHECK(fixture_setup(&f));
    /* Exact command lengths are part of the framing contract. */
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_SETTINGS_SET, 500U, settings_extra, 10U,
                       20U, 10U, false));
    CHECK(pump(&f, 10U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_SETTINGS_SET, 500U);
    CHECK(response != NULL && response->payload[4] == BC_REC_INVALID);
    CHECK(f.settings_count == 0U);

    encode_settings(settings_extra, BC_REC_MAX_INTERVAL + 1UL, 2222U,
                    false, false, true);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_SETTINGS_SET, 5001U, settings_extra, 11U,
                       244U, 15U, false));
    CHECK(pump(&f, 15U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_SETTINGS_SET, 5001U);
    CHECK(response != NULL && response->payload[4] == BC_REC_INVALID);
    CHECK(f.settings_count == 0U);

    encode_settings(settings_extra, 1111U, 2222U, false, false, true);
    f.settings_result = BC_REC_WRITE_ERROR;
    settings_calls = f.settings_count;
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_SETTINGS_SET, 501U, settings_extra, 11U,
                       244U, 20U, false));
    CHECK(pump(&f, 20U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_SETTINGS_SET, 501U);
    CHECK(response != NULL && response->payload[4] == BC_REC_WRITE_ERROR);
    CHECK(f.settings_count == settings_calls + 1U &&
          f.gesture.config.ptt_limit_ms == 5000U &&
          f.gesture.config.memo_limit_ms == 3000U);

    f.settings_result = BC_REC_OK;
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_SETTINGS_SET, 502U, settings_extra, 11U,
                       20U, 30U, false));
    CHECK(pump(&f, 30U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_SETTINGS_SET, 502U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    CHECK(f.gesture.config.ptt_limit_ms == 1111U &&
          f.gesture.config.memo_limit_ms == 2222U &&
          !f.gesture.config.memo_enabled);

    /* Active capture and a held gesture both block persistent settings. */
    encode_start(start_extra, &start);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_START, 503U, start_extra, 13U, 244U, 40U,
                       false));
    CHECK(pump(&f, 40U, 244U) != 0U);
    settings_calls = f.settings_count;
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_SETTINGS_SET, 504U, settings_extra, 11U,
                       244U, 50U, false));
    CHECK(pump(&f, 50U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_SETTINGS_SET, 504U);
    CHECK(response != NULL && response->payload[4] == BC_REC_BUSY &&
          f.settings_count == settings_calls);
    CHECK(bc_recording_stop(&f.recording, start.id, 60U) == BC_REC_OK);
    fill_pattern(frame, sizeof(frame), 0x2aU);
    CHECK(bc_recording_frame(&f.recording, start.id, 1U, frame,
                             sizeof(frame), 61U) == BC_REC_OK);
    CHECK(bc_recording_drained(&f.recording, start.id) == BC_REC_OK);
    f.gesture.hold_attempted = true;
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_SETTINGS_SET, 505U, settings_extra, 11U,
                       20U, 70U, false));
    CHECK(pump(&f, 70U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_SETTINGS_SET, 505U);
    CHECK(response != NULL && response->payload[4] == BC_REC_BUSY);
    f.gesture.hold_attempted = false;

    /* RESUME verification can be cancelled before the first bounded read;
     * the reader is closed and a subsequent new Start is allowed. */
    {
        bc_rec_start archive = start_value(0x5002U, BC_REC_APP, 0U);
        unsigned starts;
        unsigned i;
        CHECK(record_complete(&f, &archive, 8U, 0x70U, NULL, 80U));
        encode_resume(extra, archive.id, 0U, 0xabc002U);
        before = f.sink.message_count;
        CHECK(send_request(&f, BC_VOICE_RESUME, 506U, extra, 16U, 244U,
                           100U, false));
        CHECK(f.service.verifying && f.service.reader.open);
        bc_voice_put32(extra, 0xabc002U);
        CHECK(send_request(&f, BC_VOICE_CANCEL, 507U, extra, 4U, 20U, 101U,
                           false));
        CHECK(pump(&f, 101U, 20U) != 0U);
        CHECK(!f.service.verifying && !f.service.transferring &&
              !f.service.reader.open && !f.store.reader_active);
        response = find_response(&f, before, BC_VOICE_CANCEL, 507U);
        CHECK(response != NULL && response->payload[4] == BC_REC_OK);
        starts = f.capture_start_count;
        {
            bc_rec_start next = start_value(0x5003U, BC_REC_PTT, 0U);
            encode_start(start_extra, &next);
            before = f.sink.message_count;
            CHECK(send_request(&f, BC_VOICE_START, 508U, start_extra, 13U,
                               244U, 120U, false));
            CHECK(pump(&f, 120U, 244U) != 0U);
            response = find_response(&f, before, BC_VOICE_START, 508U);
            CHECK(response != NULL && response->payload[4] == BC_REC_OK);
            CHECK(f.capture_start_count == starts + 1U);
        }
        CHECK(bc_recording_stop(&f.recording, 0x5003U, 125U) == BC_REC_OK);
        CHECK(bc_recording_drained(&f.recording, 0x5003U) == BC_REC_EMPTY_AUDIO);
        (void)i;
    }

    /* A physical capture-start error is reported and is not retried by the
     * same persistent Start id. */
    {
        bc_rec_start failed = start_value(0x5004U, BC_REC_PTT, 0U);
        unsigned starts = f.capture_start_count;
        encode_start(start_extra, &failed);
        f.capture_start_result = BC_REC_CAPTURE_ERROR;
        before = f.sink.message_count;
        CHECK(send_request(&f, BC_VOICE_START, 509U, start_extra, 13U, 20U,
                           130U, false));
        CHECK(pump(&f, 130U, 20U) != 0U);
        response = find_response(&f, before, BC_VOICE_START, 509U);
        CHECK(response != NULL && response->payload[4] == BC_REC_CAPTURE_ERROR);
        CHECK(f.capture_start_count == starts + 1U);
        f.capture_start_result = BC_REC_OK;
        before = f.sink.message_count;
        CHECK(send_request(&f, BC_VOICE_START, 510U, start_extra, 13U, 244U,
                           140U, false));
        CHECK(pump(&f, 140U, 244U) != 0U);
        response = find_response(&f, before, BC_VOICE_START, 510U);
        CHECK(response != NULL && response->payload[4] == BC_REC_CAPTURE_ERROR);
        CHECK(f.capture_start_count == starts + 1U);
    }
    fixture_destroy(&f);
}

static void test_phone_outcome_lease_and_callback(void)
{
    fixture f;
    bc_voice_outcome_port port;
    bc_rec_start start = start_value(UINT64_C(0x6001), BC_REC_APP, 0U);
    uint8_t extra[13], frame[BC_REC_FRAME_MAX];
    const bc_voice_message *response;
    unsigned before;
    uint32_t token;
    uint32_t terminal_ms = UINT32_MAX - 70U;

    CHECK(fixture_setup(&f));

    /* The optional callback gates both advertisement and acceptance. */
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_HELLO, 800U, NULL, 0U, 244U, 1U, false));
    CHECK(pump(&f, 1U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_HELLO, 800U);
    CHECK(response != NULL &&
          (bc_voice_get32(response->payload + 5U) & BC_VOICE_CAP_PHONE_OUTCOME) == 0U);
    port.ctx = &f;
    port.set_outcome = outcome_set;
    CHECK(bc_voice_service_set_outcome_port(&f.service, &port));
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_HELLO, 801U, NULL, 0U, 244U, 2U, false));
    CHECK(pump(&f, 2U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_HELLO, 801U);
    CHECK(response != NULL &&
          (bc_voice_get32(response->payload + 5U) & BC_VOICE_CAP_PHONE_OUTCOME) != 0U);

    /* READY must be accepted while the current recording is active. The
     * terminal edge is then latched across the uint32 millisecond wrap. */
    f.service.now_ms = UINT32_MAX - 90U;
    CHECK(bc_recording_start(&f.recording, &start, UINT32_MAX - 90U) == BC_REC_OK);
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_READY, 802U, (const uint8_t[]){1U}, 1U,
                       244U, UINT32_MAX - 85U, false));
    CHECK(pump(&f, UINT32_MAX - 85U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_READY, 802U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK);
    token = response == NULL ? 0U : bc_voice_get32(response->payload + 5U);
    CHECK(token != 0U && f.service.outcome_ready_accepted);
    fill_pattern(frame, sizeof(frame), 0x21U);
    f.service.now_ms = UINT32_MAX - 80U;
    CHECK(bc_recording_frame(&f.recording, start.id, 1U, frame, sizeof(frame),
                             UINT32_MAX - 80U) == BC_REC_OK);
    f.service.now_ms = terminal_ms;
    CHECK(bc_recording_stop(&f.recording, start.id, UINT32_MAX - 75U) == BC_REC_OK);
    CHECK(bc_recording_drained(&f.recording, start.id) == BC_REC_OK);
    CHECK(f.service.outcome_terminal_ready &&
          f.service.outcome_terminal_ms == terminal_ms);

    encode_outcome(extra, start.id, token, BC_VOICE_PHONE_OUTCOME_KEYBOARD_INSERTED);
    /* A full response queue rejects the side effect before invoking the
     * callback. The client can retry the same request after draining it. */
    f.service.control_count = BC_VOICE_CONTROL_SLOTS;
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_PHONE_OUTCOME, 803U, extra, sizeof(extra),
                       244U, 20U, false));
    CHECK(f.outcome_calls == 0U &&
          find_response(&f, before, BC_VOICE_PHONE_OUTCOME, 803U) == NULL);
    f.service.control_count = 0U;
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_PHONE_OUTCOME, 803U, extra, sizeof(extra),
                       244U, 20U, false));
    CHECK(pump(&f, 20U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_PHONE_OUTCOME, 803U);
    CHECK(response != NULL && response->length == 5U &&
          response->payload[4] == BC_REC_OK);
    CHECK(f.outcome_calls == 1U && f.outcome_recording_id == start.id &&
          f.outcome_value == BC_VOICE_PHONE_OUTCOME_KEYBOARD_INSERTED);

    /* Same valid outcome is idempotent, while malformed, wrong-token and
     * wrong-outcome requests never reach the callback. */
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_PHONE_OUTCOME, 804U, extra, sizeof(extra),
                       244U, 21U, false));
    CHECK(pump(&f, 21U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_PHONE_OUTCOME, 804U);
    CHECK(response != NULL && response->payload[4] == BC_REC_OK &&
          f.outcome_calls == 1U);
    extra[8] ^= 1U;
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_PHONE_OUTCOME, 805U, extra, sizeof(extra),
                       244U, 22U, false));
    CHECK(pump(&f, 22U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_PHONE_OUTCOME, 805U);
    CHECK(response != NULL && response->payload[4] == BC_REC_WRONG_SESSION &&
          f.outcome_calls == 1U);
    extra[8] ^= 1U;
    extra[12] = 0U;
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_PHONE_OUTCOME, 806U, extra, sizeof(extra),
                       244U, 23U, false));
    CHECK(pump(&f, 23U, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_PHONE_OUTCOME, 806U);
    CHECK(response != NULL && response->payload[4] == BC_REC_INVALID &&
          f.outcome_calls == 1U);
    extra[12] = BC_VOICE_PHONE_OUTCOME_KEYBOARD_INSERTED;

    /* The strict ten-second boundary expires even an otherwise duplicate
     * request. Unsigned subtraction keeps the check wrap-safe. */
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_PHONE_OUTCOME, 807U, extra, sizeof(extra),
                       244U, terminal_ms + BC_VOICE_PHONE_OUTCOME_WINDOW_MS,
                       false));
    CHECK(pump(&f, terminal_ms + BC_VOICE_PHONE_OUTCOME_WINDOW_MS, 244U) != 0U);
    response = find_response(&f, before, BC_VOICE_PHONE_OUTCOME, 807U);
    CHECK(response != NULL && response->payload[4] == BC_REC_INVALID &&
          f.outcome_calls == 1U);

    /* A new active recording is busy and clears the prior confirmation lease;
     * after it finishes, the old ID/token cannot be replayed. */
    {
        bc_rec_start next = start_value(UINT64_C(0x6002), BC_REC_APP, 0U);
        CHECK(bc_recording_start(&f.recording, &next, 200U) == BC_REC_OK);
        before = f.sink.message_count;
        CHECK(send_request(&f, BC_VOICE_PHONE_OUTCOME, 808U, extra, sizeof(extra),
                           244U, 201U, false));
        CHECK(pump(&f, 201U, 244U) != 0U);
        response = find_response(&f, before, BC_VOICE_PHONE_OUTCOME, 808U);
        CHECK(response != NULL && response->payload[4] == BC_REC_BUSY &&
              f.outcome_calls == 1U);
    }
    fixture_destroy(&f);
}

static bool tuning_get(void *ctx, bc_voice_tuning *value, uint8_t *status)
{
    fixture *f = ctx;
    *value = f->tuning; *status = f->touch_status;
    return true;
}

static bc_rec_result tuning_set(void *ctx, const bc_voice_tuning *value)
{
    fixture *f = ctx;
    ++f->tuning_writes;
    if (f->tuning_result == BC_REC_OK) { f->tuning = *value; f->touch_status = 0; }
    return f->tuning_result;
}

static void test_tuning_contract_and_busy(void)
{
    fixture f;
    const bc_voice_message *response;
    bc_voice_tuning_port port = {&f, tuning_get, tuning_set};
    uint8_t extra[7] = {60, 56, 75, 120, 0, 24, 1}; /* 280 ms LE */
    unsigned before, i;
    CHECK(fixture_setup(&f));
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_TUNING_GET, 700U, NULL, 0U, 20U, 1U, false));
    CHECK(pump(&f, 1U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_TUNING_GET, 700U);
    CHECK(response && response->payload[4] == BC_REC_UNSUPPORTED);
    f.tuning = (bc_voice_tuning){54, 52, 100, 120, 280};
    f.touch_status = 1U;
    CHECK(bc_voice_service_set_tuning_port(&f.service, &port));
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_HELLO, 701U, NULL, 0U, 20U, 2U, false));
    CHECK(pump(&f, 2U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_HELLO, 701U);
    CHECK(response && (bc_voice_get32(response->payload + 5) & BC_VOICE_CAP_TUNING));
    f.tuning_result = BC_REC_SYNC_ERROR;
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_TUNING_SET, 702U, extra, 7U, 20U, 3U, false));
    CHECK(pump(&f, 3U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_TUNING_SET, 702U);
    CHECK(response && response->length == 13U && response->payload[4] == BC_REC_SYNC_ERROR &&
          response->payload[5] == 54U && response->payload[12] == 1U && f.tuning_writes == 1U);
    f.tuning_result = BC_REC_OK;
    before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_TUNING_SET, 703U, extra, 7U, 20U, 4U, false));
    CHECK(pump(&f, 4U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_TUNING_SET, 703U);
    CHECK(response && response->length == 13U && response->payload[4] == BC_REC_OK &&
          memcmp(response->payload + 5, extra, 7U) == 0 && response->payload[12] == 0U);
    CHECK(f.tuning_writes == 2U);
    /* Sensor readback is independent of desired-value durability. */
    for (i = 1U; i <= 2U; ++i) {
        f.touch_status = (uint8_t)i; before = f.sink.message_count;
        CHECK(send_request(&f, BC_VOICE_TUNING_GET, 704U + i, NULL, 0U, 20U, 5U, false));
        CHECK(pump(&f, 5U, 20U) != 0U);
        response = find_response(&f, before, BC_VOICE_TUNING_GET, 704U + i);
        CHECK(response && response->payload[12] == i);
    }
    f.gesture.hold_attempted = true; before = f.sink.message_count;
    CHECK(send_request(&f, BC_VOICE_TUNING_SET, 707U, extra, 7U, 20U, 6U, false));
    CHECK(pump(&f, 6U, 20U) != 0U);
    response = find_response(&f, before, BC_VOICE_TUNING_SET, 707U);
    CHECK(response && response->payload[4] == BC_REC_BUSY && f.tuning_writes == 2U);
    f.gesture.hold_attempted = false;
    {
        const bc_voice_tuning invalid[] = {
            {31,30,100,120,280}, {81,52,100,120,280}, {54,29,100,120,280},
            {54,53,100,120,280}, {54,55,100,120,280}, {54,52,0,120,280},
            {54,52,101,120,280}, {54,52,100,0,280}, {54,52,100,401,280},
            {54,52,100,121,280}, {54,52,100,120,19}, {54,52,100,120,420}
        };
        for (i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
            CHECK(!bc_voice_tuning_valid(&invalid[i]));
            extra[0] = invalid[i].touch_set; extra[1] = invalid[i].touch_clear;
            extra[2] = invalid[i].haptic_strength;
            bc_voice_put16(extra + 3, invalid[i].start_active_ms);
            bc_voice_put16(extra + 5, invalid[i].stop_active_ms);
            before = f.sink.message_count;
            CHECK(send_request(&f, BC_VOICE_TUNING_SET, 710U+i, extra, 7U, 20U, 7U, false));
            CHECK(pump(&f, 7U, 20U) != 0U);
            response = find_response(&f, before, BC_VOICE_TUNING_SET, 710U+i);
            CHECK(response && response->payload[4] == BC_REC_INVALID && f.tuning_writes == 2U);
        }
    }
    CHECK(bc_voice_tuning_valid(&(bc_voice_tuning){32,30,1,20,400}));
    CHECK(bc_voice_tuning_valid(&(bc_voice_tuning){80,78,100,400,20}));
    fixture_destroy(&f);
}

int main(void)
{
    test_handshake_queue_and_wire_rejection();
    test_start_stop_idempotency_and_recovery();
    test_pending_stop_snapshot_and_queue_reservation();
    test_empty_start_retry_query_and_remount();
    test_ready_live_ack_lease_and_disconnect();
    test_live_prefix_delayed_ready_and_backpressure();
    test_live_prefix_overflow_cancel_timeout_and_reconnect();
    test_resume_window_retry_and_crc();
    test_archive_fragment_burst_and_control_priority();
    test_archive_retired_tokens_and_active_retry();
    test_archive_delayed_ack_during_retransmit();
    test_archive_stall_expiry_and_resume();
    test_archive_only_real_progress_renews_stall();
    test_archive_verification_stall_and_progress();
    test_query_catalog_receipt_and_id_custody();
    test_settings_errors_and_archive_cancellation();
    test_phone_outcome_lease_and_callback();
    test_tuning_contract_and_busy();
    fprintf(stdout, "%u checks, %u failures\n", checks, failures);
    return failures == 0U ? 0 : 1;
}
