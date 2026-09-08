#include "bc_voice_legacy_archive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FLASH_BLOCK_SIZE 256U
#define FLASH_BLOCK_COUNT 64U
#define FLASH_SIZE (FLASH_BLOCK_SIZE * FLASH_BLOCK_COUNT)
#define TEST_CACHE_SIZE 64U
#define TEST_LOOKAHEAD_SIZE 8U
#define MAX_PACKETS 32U

struct ram_nor {
    uint8_t bytes[FLASH_SIZE];
    unsigned read_calls;
    unsigned prog_calls;
    unsigned erase_calls;
    unsigned sync_calls;
    unsigned fail_read_call;
};

struct test_fs {
    lfs_t lfs;
    struct lfs_config config;
    uint8_t read_cache[TEST_CACHE_SIZE];
    uint8_t prog_cache[TEST_CACHE_SIZE];
    uint8_t lookahead[TEST_LOOKAHEAD_SIZE];
};

struct send_sink {
    bool accept;
    unsigned attempts;
    unsigned accepted;
    uint8_t packets[MAX_PACKETS][BC_VOICE_LEGACY_UPLOAD_PACKET_MAX];
    uint16_t lengths[MAX_PACKETS];
    uint32_t epochs[MAX_PACKETS];
    uint8_t last_attempt[BC_VOICE_LEGACY_UPLOAD_PACKET_MAX];
    uint16_t last_attempt_length;
};

static unsigned checks;
static unsigned failures;

static void check_result(bool condition, const char *expression, unsigned line)
{
    ++checks;
    if (!condition) {
        ++failures;
        fprintf(stderr, "FAIL line %u: %s\n", line, expression);
    }
}

#define CHECK(condition) check_result((condition), #condition, __LINE__)

static int ram_read(const struct lfs_config *config, lfs_block_t block,
                    lfs_off_t off, void *buffer, lfs_size_t size)
{
    struct ram_nor *ram = (struct ram_nor *)config->context;
    uint64_t address = (uint64_t)block * config->block_size + off;

    ++ram->read_calls;
    if (ram->fail_read_call != 0U && ram->read_calls == ram->fail_read_call) {
        ram->fail_read_call = 0U;
        return LFS_ERR_IO;
    }
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
    struct ram_nor *ram = (struct ram_nor *)config->context;
    const uint8_t *source = (const uint8_t *)buffer;
    uint64_t address = (uint64_t)block * config->block_size + off;
    lfs_size_t i;

    ++ram->prog_calls;
    if (buffer == NULL || block >= FLASH_BLOCK_COUNT ||
        off > config->block_size || size > config->block_size - off ||
        address + size > FLASH_SIZE || off % config->prog_size != 0U ||
        size % config->prog_size != 0U)
        return LFS_ERR_INVAL;
    for (i = 0; i < size; ++i) {
        if ((ram->bytes[address + i] & source[i]) != source[i])
            return LFS_ERR_CORRUPT;
    }
    for (i = 0; i < size; ++i)
        ram->bytes[address + i] &= source[i];
    return LFS_ERR_OK;
}

static int ram_erase(const struct lfs_config *config, lfs_block_t block)
{
    struct ram_nor *ram = (struct ram_nor *)config->context;

    ++ram->erase_calls;
    if (block >= FLASH_BLOCK_COUNT)
        return LFS_ERR_INVAL;
    memset(ram->bytes + (size_t)block * config->block_size, 0xff,
           config->block_size);
    return LFS_ERR_OK;
}

static int ram_sync(const struct lfs_config *config)
{
    struct ram_nor *ram = (struct ram_nor *)config->context;
    ++ram->sync_calls;
    return LFS_ERR_OK;
}

static void fs_configure(struct test_fs *fs, struct ram_nor *ram)
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

static bool fs_format_mount(struct test_fs *fs, struct ram_nor *ram)
{
    int error;

    fs_configure(fs, ram);
    error = lfs_format(&fs->lfs, &fs->config);
    if (error != LFS_ERR_OK)
        return false;
    memset(&fs->lfs, 0, sizeof(fs->lfs));
    return lfs_mount(&fs->lfs, &fs->config) == LFS_ERR_OK;
}

static void make_name(uint8_t name[BC_VOICE_LEGACY_NAME_SIZE], uint8_t type,
                      uint8_t seed)
{
    unsigned i;

    for (i = 0; i < BC_VOICE_LEGACY_NAME_SIZE; ++i)
        name[i] = (uint8_t)('a' + (uint8_t)((seed + i) % 26U));
    name[33] = type;
}

static bool name_for_start(void *ctx, const bc_rec_start *start,
                           char name[BC_REC_NAME_SIZE])
{
    uint8_t fixed[BC_VOICE_LEGACY_NAME_SIZE];

    (void)ctx;
    make_name(fixed, (uint8_t)'B', (uint8_t)start->id);
    memcpy(name, fixed, sizeof(fixed));
    name[BC_VOICE_LEGACY_NAME_SIZE] = '\0';
    name[BC_VOICE_LEGACY_NAME_SIZE + 1U] = '\0';
    return true;
}

static void fill_pattern(uint8_t *data, size_t size, uint8_t seed)
{
    size_t i;
    for (i = 0; i < size; ++i)
        data[i] = (uint8_t)(seed + (uint8_t)i);
}

static bool write_old_file(struct test_fs *fs,
                           const uint8_t name[BC_VOICE_LEGACY_NAME_SIZE],
                           const uint8_t *data, uint32_t length)
{
    lfs_file_t file;
    struct lfs_file_config config;
    uint8_t cache[BC_VOICE_LEGACY_FILE_CACHE_SIZE];
    char path[BC_VOICE_LEGACY_PATH_SIZE];
    lfs_ssize_t written;
    int error;

    memset(&config, 0, sizeof(config));
    config.buffer = cache;
    path[0] = '/';
    memcpy(path + 1U, name, BC_VOICE_LEGACY_NAME_SIZE);
    path[1U + BC_VOICE_LEGACY_NAME_SIZE] = '\0';
    error = lfs_file_opencfg(&fs->lfs, &file, path,
                             LFS_O_WRONLY | LFS_O_CREAT, &config);
    if (error != LFS_ERR_OK)
        return false;
    written = lfs_file_write(&fs->lfs, &file, data, length);
    error = lfs_file_close(&fs->lfs, &file);
    return written == (lfs_ssize_t)length && error == LFS_ERR_OK;
}

static bool create_record(bc_rec_store *store, uint64_t id,
                          const uint8_t *data, uint16_t length)
{
    bc_rec_start start;
    bc_rec_file file;
    uint16_t offset = 0U;

    start.id = id;
    start.trigger = BC_REC_PTT;
    start.duration_limit_ms = 30000U;
    if (bc_rec_store_open(store, &start, &file) != BC_REC_OK)
        return false;
    while (offset < length) {
        uint16_t chunk = (uint16_t)(length - offset);
        if (chunk > BC_REC_FRAME_MAX)
            chunk = BC_REC_FRAME_MAX;
        if (bc_rec_store_append(store, data + offset, chunk) != BC_REC_OK)
            return false;
        offset = (uint16_t)(offset + chunk);
    }
    return bc_rec_store_finish(store, true, &file) == BC_REC_OK;
}

static bool sink_send(void *context, const uint8_t *packet, uint16_t length,
                      uint32_t epoch)
{
    struct send_sink *sink = (struct send_sink *)context;

    ++sink->attempts;
    sink->last_attempt_length = length;
    memcpy(sink->last_attempt, packet, length);
    if (!sink->accept || sink->accepted >= MAX_PACKETS)
        return false;
    memcpy(sink->packets[sink->accepted], packet, length);
    sink->lengths[sink->accepted] = length;
    sink->epochs[sink->accepted] = epoch;
    ++sink->accepted;
    return true;
}

static bc_rec_result drain(bc_voice_legacy_archive *archive, unsigned limit)
{
    bc_rec_result result = BC_REC_OK;
    unsigned i;

    for (i = 0; i < limit && bc_voice_legacy_archive_active(archive); ++i) {
        result = bc_voice_legacy_archive_poll(archive);
        if (result != BC_REC_OK && result != BC_REC_BUSY)
            break;
    }
    CHECK(!bc_voice_legacy_archive_active(archive));
    return result;
}

static void put_le32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static uint32_t get_le32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static uint16_t list_response_name_offset(void)
{
    return (uint16_t)(BC_VOICE_LEGACY_HEADER_SIZE +
                      BC_VOICE_LEGACY_LIST_FIELDS_SIZE);
}

static void test_list_and_old_upload(struct test_fs *fs,
                                     bc_rec_store *store,
                                     struct send_sink *sink,
                                     const uint8_t old_name[38],
                                     const uint8_t *old_data,
                                     uint32_t old_size)
{
    bc_voice_legacy_archive archive;
    uint8_t request[BC_VOICE_LEGACY_HEADER_SIZE + 38U];
    unsigned i;
    unsigned old_seen = 0U;
    unsigned new_seen = 0U;

    memset(&archive, 0, sizeof(archive));
    CHECK(bc_voice_legacy_archive_init(&archive, &fs->lfs, store, sink_send,
                                       sink));
    memset(sink, 0, sizeof(*sink));
    sink->accept = true;
    request[0] = 0x11U;
    request[1] = 0x22U;
    request[2] = BC_VOICE_LEGACY_COMMAND;
    request[3] = BC_VOICE_LEGACY_SUB_LIST;
    CHECK(bc_voice_legacy_archive_request(&archive, request, 4U, 17U, 241U) ==
          BC_REC_OK);
    CHECK(drain(&archive, 300U) == BC_REC_OK);
    CHECK(sink->accepted == 2U);
    for (i = 0; i < sink->accepted; ++i) {
        CHECK(sink->lengths[i] == BC_VOICE_LEGACY_LIST_PACKET_MAX);
        CHECK(sink->packets[i][0] == request[0] &&
              sink->packets[i][1] == request[1] &&
              sink->packets[i][2] == request[2] &&
              sink->packets[i][3] == request[3]);
        CHECK(get_le32(sink->packets[i] + 4U) == 2U);
        CHECK(get_le32(sink->packets[i] + 8U) == i + 1U);
        if (memcmp(sink->packets[i] + list_response_name_offset(), old_name,
                   38U) == 0) {
            ++old_seen;
            CHECK(get_le32(sink->packets[i] + 12U) == old_size);
        } else {
            ++new_seen;
            CHECK(get_le32(sink->packets[i] + 12U) == 450U);
        }
    }
    CHECK(old_seen == 1U && new_seen == 1U);

    memset(sink, 0, sizeof(*sink));
    sink->accept = false;
    memcpy(request + 4U, old_name, 38U);
    request[3] = BC_VOICE_LEGACY_SUB_UPLOAD;
    CHECK(bc_voice_legacy_archive_request(&archive, request, sizeof(request),
                                          18U, 241U) == BC_REC_OK);
    /* Reach the first buffered packet, then exercise an identical retry. */
    for (i = 0; i < 100U && bc_voice_legacy_archive_active(&archive) &&
         sink->attempts == 0U; ++i) {
        bc_rec_result step = bc_voice_legacy_archive_poll(&archive);
        CHECK(step == BC_REC_OK || step == BC_REC_BUSY);
    }
    CHECK(sink->attempts == 1U && sink->last_attempt_length == 241U);
    {
        uint8_t first_attempt[BC_VOICE_LEGACY_UPLOAD_PACKET_MAX];
        uint16_t first_length = sink->last_attempt_length;
        memcpy(first_attempt, sink->last_attempt, first_length);
        CHECK(bc_voice_legacy_archive_poll(&archive) == BC_REC_BUSY);
        CHECK(sink->attempts == 2U && sink->last_attempt_length == first_length &&
              memcmp(sink->last_attempt, first_attempt, first_length) == 0);
    }
    sink->accept = true;
    CHECK(drain(&archive, 300U) == BC_REC_OK);
    CHECK(sink->accepted == 3U);
    for (i = 0; i < sink->accepted; ++i) {
        uint32_t remaining = get_le32(sink->packets[i] + 5U);
        uint32_t count = get_le32(sink->packets[i] + 9U);
        uint32_t sequence = get_le32(sink->packets[i] + 13U);
        uint32_t chunk = get_le32(sink->packets[i] + 17U);
        uint32_t offset = old_size - remaining;
        CHECK(sink->lengths[i] == 4U + 17U + chunk);
        CHECK(sink->packets[i][4] == 1U && count == 3U - i &&
              sequence == i + 1U && chunk <= 220U);
        CHECK(memcmp(sink->packets[i] + 21U, old_data + offset, chunk) == 0);
    }
}

static void test_new_resume_and_cancel(struct test_fs *fs,
                                       bc_rec_store *store,
                                       struct send_sink *sink,
                                       const uint8_t new_name[38],
                                       const uint8_t *new_data,
                                       uint32_t new_size)
{
    bc_voice_legacy_archive archive;
    uint8_t request[46];

    memset(&archive, 0, sizeof(archive));
    CHECK(bc_voice_legacy_archive_init(&archive, &fs->lfs, store, sink_send,
                                       sink));
    memset(sink, 0, sizeof(*sink));
    sink->accept = true;
    request[0] = 0x31U;
    request[1] = 0x41U;
    request[2] = BC_VOICE_LEGACY_COMMAND;
    request[3] = BC_VOICE_LEGACY_SUB_RESUME;
    put_le32(request + 4U, 220U);
    memcpy(request + 8U, new_name, 38U);
    CHECK(bc_voice_legacy_archive_request(&archive, request, sizeof(request),
                                          19U, 241U) == BC_REC_OK);
    CHECK(drain(&archive, 400U) == BC_REC_OK);
    CHECK(sink->accepted == 2U);
    CHECK(get_le32(sink->packets[0] + 5U) == new_size - 220U);
    CHECK(get_le32(sink->packets[0] + 9U) == 2U);
    CHECK(get_le32(sink->packets[0] + 13U) == 1U);
    CHECK(get_le32(sink->packets[0] + 17U) == 220U);
    CHECK(memcmp(sink->packets[0] + 21U, new_data + 220U, 220U) == 0);
    CHECK(get_le32(sink->packets[1] + 5U) == 10U);
    CHECK(get_le32(sink->packets[1] + 9U) == 1U);
    CHECK(get_le32(sink->packets[1] + 13U) == 2U);
    CHECK(get_le32(sink->packets[1] + 17U) == 10U);
    CHECK(memcmp(sink->packets[1] + 21U, new_data + 440U, 10U) == 0);

    memset(sink, 0, sizeof(*sink));
    sink->accept = false;
    request[3] = BC_VOICE_LEGACY_SUB_UPLOAD;
    memcpy(request + 4U, new_name, 38U);
    CHECK(bc_voice_legacy_archive_request(&archive, request, 42U, 20U, 241U) ==
          BC_REC_OK);
    CHECK(bc_voice_legacy_archive_poll(&archive) == BC_REC_OK);
    CHECK(bc_voice_legacy_archive_cancel(&archive) == BC_REC_OK);
    CHECK(!bc_voice_legacy_archive_active(&archive));
    CHECK(sink->accepted == 0U);
}

static void test_rejects_and_errors(struct test_fs *fs, bc_rec_store *store,
                                    struct send_sink *sink,
                                    const uint8_t old_name[38])
{
    bc_voice_legacy_archive archive;
    uint8_t request[46];
    uint8_t bad_name[38];
    bc_rec_result result;

    memset(&archive, 0, sizeof(archive));
    CHECK(bc_voice_legacy_archive_init(&archive, &fs->lfs, store, sink_send,
                                       sink));
    memset(sink, 0, sizeof(*sink));
    sink->accept = true;
    memset(request, 0, sizeof(request));
    request[2] = BC_VOICE_LEGACY_COMMAND;
    request[3] = BC_VOICE_LEGACY_SUB_UPLOAD;
    memcpy(request + 4U, old_name, 38U);
    CHECK(bc_voice_legacy_archive_request(&archive, request, 42U, 21U, 240U) ==
          BC_REC_INVALID);
    memcpy(bad_name, old_name, sizeof(bad_name));
    bad_name[0] = '/';
    memcpy(request + 4U, bad_name, sizeof(bad_name));
    CHECK(bc_voice_legacy_archive_request(&archive, request, 42U, 21U, 241U) ==
          BC_REC_INVALID);
    memcpy(bad_name, old_name, sizeof(bad_name));
    bad_name[33] = 'C';
    memcpy(request + 4U, bad_name, sizeof(bad_name));
    CHECK(bc_voice_legacy_archive_request(&archive, request, 42U, 21U, 241U) ==
          BC_REC_INVALID);

    memcpy(request + 4U, old_name, 38U);
    request[3] = BC_VOICE_LEGACY_SUB_DELETE;
    CHECK(bc_voice_legacy_archive_request(&archive, request, 42U, 22U, 241U) ==
          BC_REC_UNSUPPORTED);
    CHECK(bc_voice_legacy_archive_poll(&archive) == BC_REC_OK);
    CHECK(sink->accepted == 1U && sink->lengths[0] == 5U &&
          sink->packets[0][4] == 0U);
    if (bc_voice_legacy_archive_active(&archive))
        CHECK(bc_voice_legacy_archive_cancel(&archive) == BC_REC_OK);

    memset(sink, 0, sizeof(*sink));
    sink->accept = true;
    request[3] = BC_VOICE_LEGACY_SUB_BATCH;
    request[4] = 7U; /* Existing one-click requests carry a file index. */
    CHECK(bc_voice_legacy_archive_request(&archive, request, 5U, 23U, 241U) ==
          BC_REC_UNSUPPORTED);
    CHECK(bc_voice_legacy_archive_poll(&archive) == BC_REC_OK);
    CHECK(sink->accepted == 1U && sink->lengths[0] == 5U &&
          sink->packets[0][4] == 0U);

    memset(sink, 0, sizeof(*sink));
    sink->accept = true;
    request[3] = BC_VOICE_LEGACY_SUB_SPACE;
    CHECK(bc_voice_legacy_archive_request(&archive, request, 4U, 23U, 241U) ==
          BC_REC_OK);
    CHECK(drain(&archive, 20U) == BC_REC_OK);
    CHECK(sink->accepted == 1U && sink->lengths[0] == 16U);
    CHECK(get_le32(sink->packets[0] + 4U) == FLASH_SIZE);
    CHECK(get_le32(sink->packets[0] + 8U) < FLASH_SIZE);
    CHECK(get_le32(sink->packets[0] + 12U) ==
          FLASH_SIZE - get_le32(sink->packets[0] + 8U));

    memset(sink, 0, sizeof(*sink));
    sink->accept = true;
    request[3] = BC_VOICE_LEGACY_SUB_UPLOAD;
    memcpy(request + 4U, old_name, 38U);
    ((struct ram_nor *)fs->config.context)->fail_read_call =
        ((struct ram_nor *)fs->config.context)->read_calls + 1U;
    CHECK(bc_voice_legacy_archive_request(&archive, request, 42U, 24U, 241U) ==
          BC_REC_OK);
    do {
        result = bc_voice_legacy_archive_poll(&archive);
    } while (result == BC_REC_OK || result == BC_REC_BUSY);
    CHECK(result != BC_REC_OK && !bc_voice_legacy_archive_active(&archive));
}

int main(void)
{
    struct ram_nor ram;
    struct test_fs fs;
    bc_rec_store store;
    uint8_t old_name[BC_VOICE_LEGACY_NAME_SIZE];
    uint8_t new_name[BC_VOICE_LEGACY_NAME_SIZE];
    uint8_t old_data[500];
    uint8_t new_data[450];

    memset(&ram, 0, sizeof(ram));
    memset(ram.bytes, 0xff, sizeof(ram.bytes));
    CHECK(fs_format_mount(&fs, &ram));
    make_name(old_name, (uint8_t)'8', 3U);
    make_name(new_name, (uint8_t)'B', 1U);
    fill_pattern(old_data, sizeof(old_data), 0x10U);
    fill_pattern(new_data, sizeof(new_data), 0x70U);
    if (failures == 0U) {
        CHECK(write_old_file(&fs, old_name, old_data, sizeof(old_data)));
        CHECK(bc_rec_store_init(&store, &fs.lfs, name_for_start, NULL));
        {
            bc_audio_format legacy;
            bc_audio_format_legacy_adpcm(&legacy);
            CHECK(bc_rec_store_set_format(&store, &legacy));
        }
        CHECK(create_record(&store, 1U, new_data, sizeof(new_data)));
        {
            struct send_sink sink;
            test_list_and_old_upload(&fs, &store, &sink, old_name, old_data,
                                     sizeof(old_data));
            test_new_resume_and_cancel(&fs, &store, &sink, new_name, new_data,
                                       sizeof(new_data));
            test_rejects_and_errors(&fs, &store, &sink, old_name);
        }
    }
    if (fs.lfs.cfg != NULL)
        CHECK(lfs_unmount(&fs.lfs) == LFS_ERR_OK);
    fprintf(stdout, "%u checks, %u failures\n", checks, failures);
    return failures == 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
