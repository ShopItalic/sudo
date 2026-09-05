#include "bc_rec_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FLASH_BLOCK_SIZE 256U
#define FLASH_BLOCK_COUNT 64U
#define FLASH_SIZE (FLASH_BLOCK_SIZE * FLASH_BLOCK_COUNT)
#define TEST_CACHE_SIZE 64U
#define TEST_LOOKAHEAD_SIZE 8U

enum ram_cut_kind {
    RAM_CUT_NONE = 0,
    RAM_CUT_PROG,
    RAM_CUT_ERASE,
    RAM_CUT_SYNC
};

enum ram_cut_mode {
    RAM_CUT_MODE_NONE = 0,
    RAM_CUT_MODE_BEFORE,
    RAM_CUT_MODE_PARTIAL,
    RAM_CUT_MODE_AFTER
};

struct ram_nor {
    uint8_t bytes[FLASH_SIZE];
    unsigned prog_calls;
    unsigned erase_calls;
    unsigned sync_calls;
    unsigned fail_prog_call;
    unsigned fail_erase_call;
    unsigned fail_sync_call;
    enum ram_cut_kind cut_kind;
    enum ram_cut_mode cut_mode;
    unsigned cut_call;
    bool cut_armed;
    bool cut_triggered;
};

struct test_fs {
    lfs_t lfs;
    struct lfs_config config;
    uint8_t read_cache[TEST_CACHE_SIZE];
    uint8_t prog_cache[TEST_CACHE_SIZE];
    uint8_t lookahead[TEST_LOOKAHEAD_SIZE];
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

static unsigned *ram_call_counter(struct ram_nor *ram,
                                  enum ram_cut_kind kind)
{
    switch (kind) {
    case RAM_CUT_PROG:
        return &ram->prog_calls;
    case RAM_CUT_ERASE:
        return &ram->erase_calls;
    case RAM_CUT_SYNC:
        return &ram->sync_calls;
    case RAM_CUT_NONE:
    default:
        return NULL;
    }
}

/* For AFTER, the selected primitive completes and arms the next callback as
 * the simulated power cut.  The operation wrapper abandons the mounted lfs_t
 * as soon as that armed primitive returns, covering the final callback too. */
static bool ram_cut_should_fail(struct ram_nor *ram,
                                enum ram_cut_kind kind)
{
    unsigned *calls;

    if (ram->cut_mode == RAM_CUT_MODE_NONE)
        return false;
    if (ram->cut_mode == RAM_CUT_MODE_AFTER && ram->cut_armed) {
        ram->cut_armed = false;
        ram->cut_triggered = true;
        return true;
    }
    if (ram->cut_kind != kind || ram->cut_call == 0U)
        return false;
    calls = ram_call_counter(ram, kind);
    if (calls == NULL || *calls != ram->cut_call)
        return false;
    if (ram->cut_mode == RAM_CUT_MODE_AFTER) {
        ram->cut_armed = true;
        return false;
    }
    ram->cut_triggered = true;
    return true;
}

static void ram_reset_io(struct ram_nor *ram)
{
    ram->prog_calls = 0U;
    ram->erase_calls = 0U;
    ram->sync_calls = 0U;
    ram->fail_prog_call = 0U;
    ram->fail_erase_call = 0U;
    ram->fail_sync_call = 0U;
    ram->cut_kind = RAM_CUT_NONE;
    ram->cut_mode = RAM_CUT_MODE_NONE;
    ram->cut_call = 0U;
    ram->cut_armed = false;
    ram->cut_triggered = false;
}

static void ram_set_cut(struct ram_nor *ram, enum ram_cut_kind kind,
                        enum ram_cut_mode mode, unsigned call)
{
    ram_reset_io(ram);
    ram->cut_kind = kind;
    ram->cut_mode = mode;
    ram->cut_call = call;
}

static int ram_read(const struct lfs_config *config, lfs_block_t block,
                    lfs_off_t off, void *buffer, lfs_size_t size)
{
    struct ram_nor *ram = (struct ram_nor *)config->context;
    uint64_t address = (uint64_t)block * config->block_size + off;
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
    if (ram_cut_should_fail(ram, RAM_CUT_PROG)) {
        lfs_size_t partial_size;

        if (ram->cut_mode != RAM_CUT_MODE_PARTIAL)
            return LFS_ERR_IO;
        if (buffer == NULL || block >= FLASH_BLOCK_COUNT ||
            off > config->block_size || size > config->block_size - off ||
            address + size > FLASH_SIZE || off % config->prog_size != 0U ||
            size % config->prog_size != 0U)
            return LFS_ERR_INVAL;
        partial_size = size > 1U ? size / 2U : size;
        for (i = 0U; i < partial_size; ++i) {
            if ((ram->bytes[address + i] & source[i]) != source[i])
                return LFS_ERR_CORRUPT;
        }
        for (i = 0U; i < partial_size; ++i)
            ram->bytes[address + i] &= source[i];
        return LFS_ERR_IO;
    }
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
    uint64_t address = (uint64_t)block * config->block_size;
    ++ram->erase_calls;
    if (ram_cut_should_fail(ram, RAM_CUT_ERASE)) {
        if (block >= FLASH_BLOCK_COUNT || address + config->block_size > FLASH_SIZE)
            return LFS_ERR_INVAL;
        if (ram->cut_mode == RAM_CUT_MODE_PARTIAL) {
            memset(ram->bytes + address, 0xff, config->block_size / 2U);
        }
        return LFS_ERR_IO;
    }
    if (ram->fail_erase_call != 0U &&
        ram->erase_calls == ram->fail_erase_call) {
        ram->fail_erase_call = 0U;
        return LFS_ERR_IO;
    }
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
    if (ram_cut_should_fail(ram, RAM_CUT_SYNC))
        return LFS_ERR_IO;
    if (ram->fail_sync_call != 0U &&
        ram->sync_calls == ram->fail_sync_call) {
        ram->fail_sync_call = 0U;
        return LFS_ERR_IO;
    }
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

static bool fs_mount_existing(struct test_fs *fs, struct ram_nor *ram)
{
    fs_configure(fs, ram);
    return lfs_mount(&fs->lfs, &fs->config) == LFS_ERR_OK;
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

static bool name_for_start(void *ctx, const bc_rec_start *start,
                           char name[BC_REC_NAME_SIZE])
{
    (void)ctx;
    (void)start;
    memcpy(name, "capture.raw", sizeof("capture.raw"));
    return true;
}

static bool traversal_name(void *ctx, const bc_rec_start *start,
                           char name[BC_REC_NAME_SIZE])
{
    (void)start;
    memcpy(name, ctx, BC_REC_NAME_SIZE);
    return true;
}

static bc_rec_start start_for(uint64_t id)
{
    bc_rec_start start;
    start.id = id;
    start.trigger = BC_REC_PTT;
    start.duration_limit_ms = 30000U;
    return start;
}

static void fill_pattern(uint8_t *data, size_t size, uint8_t seed)
{
    size_t i;
    for (i = 0; i < size; ++i)
        data[i] = (uint8_t)(seed + (uint8_t)i);
}

static bool store_init(bc_rec_store *store, struct test_fs *fs)
{
    return bc_rec_store_init(store, &fs->lfs, name_for_start, NULL);
}

static void test_crc_duplicate_and_bounded_read(struct ram_nor *ram,
                                                struct test_fs *fs)
{
    bc_rec_store store;
    bc_rec_start start = start_for(1U);
    bc_rec_start found;
    bc_rec_file file;
    bc_rec_file duplicate;
    uint8_t data[] = "123456789";
    uint8_t output[16];
    bc_rec_store_cursor cursor;
    bc_rec_store_status status;
    lfs_file_t unrelated;
    bool saw_more = false;
    bool saw_record = false;
    unsigned i;
    int32_t count;

    CHECK(store_init(&store, fs));
    CHECK(bc_rec_store_open(&store, &start, &file) == BC_REC_OK);
    CHECK(bc_rec_store_append(&store, data, (uint16_t)(sizeof(data) - 1U)) ==
          BC_REC_OK);
    CHECK(bc_rec_store_checkpoint(&store, &file) == BC_REC_OK);
    CHECK(file.bytes == 9U && file.frames == 1U);
    CHECK(bc_rec_store_finish(&store, true, &file) == BC_REC_OK);
    CHECK(file.complete && !file.recovered && file.crc32 == 0xcbf43926UL);
    status = bc_rec_store_lookup(&store, start.id, &found, &duplicate);
    CHECK(status == BC_REC_STORE_OK);
    CHECK(found.id == start.id && found.trigger == start.trigger);
    CHECK(duplicate.complete && duplicate.bytes == 9U &&
          duplicate.crc32 == 0xcbf43926UL);

    CHECK(bc_rec_store_open(&store, &start, &duplicate) ==
          BC_REC_ALREADY_EXISTS);
    CHECK(duplicate.complete && duplicate.bytes == 9U);

    start.trigger = BC_REC_MEMO;
    CHECK(bc_rec_store_open(&store, &start, &duplicate) == BC_REC_DUPLICATE);
    start = start_for(1U);

    memset(output, 0, sizeof(output));
    count = bc_rec_store_read(&store, start.id, 3U, output, 3U);
    CHECK(count == 3 && memcmp(output, "456", 3U) == 0);
    count = bc_rec_store_read(&store, start.id, 7U, output, sizeof(output));
    CHECK(count == 2 && memcmp(output, "89", 2U) == 0);
    count = bc_rec_store_read(&store, start.id, 9U, output, sizeof(output));
    CHECK(count == 0);
    CHECK(bc_rec_store_read(&store, start.id, 0U, NULL, 1U) < 0);

    CHECK(bc_rec_store_list_begin(&store, &cursor) == BC_REC_STORE_OK);
    CHECK(bc_rec_store_list_next(&store, &cursor, &found, &duplicate) ==
          BC_REC_STORE_OK);
    CHECK(found.id == 1U && duplicate.bytes == 9U);
    CHECK(bc_rec_store_list_next(&store, &cursor, &found, &duplicate) ==
          BC_REC_STORE_END);
    CHECK(bc_rec_store_list_end(&store, &cursor) == BC_REC_STORE_OK);

    /* Metadata iteration consumes one directory entry per call.  An
     * unrelated file therefore returns MORE and cannot force a raw CRC scan. */
    CHECK(lfs_file_open(&fs->lfs, &unrelated, "/.sudo-rec/unrelated",
                        LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL) == LFS_ERR_OK);
    CHECK(lfs_file_close(&fs->lfs, &unrelated) == LFS_ERR_OK);
    CHECK(bc_rec_store_list_begin(&store, &cursor) == BC_REC_STORE_OK);
    for (i = 0U; i < 8U && !(saw_record && saw_more); ++i) {
        status = bc_rec_store_metadata_next(&store, &cursor, &found,
                                            &duplicate);
        if (status == BC_REC_STORE_MORE)
            saw_more = true;
        else if (status == BC_REC_STORE_OK)
            saw_record = true;
        else if (status == BC_REC_STORE_END)
            break;
        else
            CHECK(false);
    }
    CHECK(saw_record && saw_more);
    CHECK(bc_rec_store_list_end(&store, &cursor) == BC_REC_STORE_OK);

    CHECK(lfs_unmount(&fs->lfs) == LFS_ERR_OK);
    CHECK(fs_mount_existing(fs, ram));
}

static void test_partial_recovery(struct ram_nor *ram, struct test_fs *fs)
{
    bc_rec_store store;
    bc_rec_store_reader reader;
    bc_rec_store_cursor catalog;
    bc_rec_start start = start_for(2U);
    bc_rec_start found;
    bc_rec_file file;
    bc_rec_file recovered;
    uint8_t first[32];
    uint8_t second[24];
    uint8_t read_back[12];
    bool crc_verified;
    unsigned sync_before;

    fill_pattern(first, sizeof(first), 0x20U);
    fill_pattern(second, sizeof(second), 0x80U);
    CHECK(store_init(&store, fs));
    CHECK(bc_rec_store_open(&store, &start, &file) == BC_REC_OK);
    CHECK(bc_rec_store_append(&store, first, sizeof(first)) == BC_REC_OK);
    CHECK(bc_rec_store_checkpoint(&store, &file) == BC_REC_OK);
    CHECK(file.bytes == sizeof(first));
    CHECK(bc_rec_store_append(&store, second, sizeof(second)) == BC_REC_OK);

    sync_before = ram->sync_calls;
    ram->fail_sync_call = sync_before + 1U;
    CHECK(bc_rec_store_checkpoint(&store, &file) == BC_REC_SYNC_ERROR);
    CHECK(file.bytes == sizeof(first));
    {
        bc_rec_result finish_result = bc_rec_store_finish(&store, false, &file);
        CHECK(finish_result == BC_REC_SYNC_ERROR ||
              finish_result == BC_REC_WRITE_ERROR ||
              finish_result == BC_REC_NO_SPACE);
        CHECK(file.bytes == sizeof(first));
    }
    CHECK(lfs_unmount(&fs->lfs) == LFS_ERR_OK);
    CHECK(fs_mount_existing(fs, ram));

    CHECK(store_init(&store, fs));
    CHECK(bc_rec_store_lookup(&store, start.id, &found, &recovered) ==
          BC_REC_STORE_OK);
    CHECK(found.id == start.id && recovered.bytes == sizeof(first));
    CHECK(!recovered.complete && recovered.recovered);
    CHECK(bc_rec_store_read(&store, start.id, 0U, first, sizeof(first)) ==
          (int32_t)sizeof(first));
    CHECK(bc_rec_store_read(&store, start.id, sizeof(first), second,
                            sizeof(second)) == 0);

    CHECK(bc_rec_store_reader_begin(&store, start.id, &reader, &found, &file) ==
          BC_REC_STORE_OK);
    CHECK(!file.complete && file.recovered && file.bytes == sizeof(first));
    CHECK(bc_rec_store_open(&store, &start, &file) == BC_REC_BUSY);
    CHECK(bc_rec_store_reader_read(&reader, 0U, read_back, sizeof(read_back)) ==
          BC_REC_STORE_UNVERIFIED);
    CHECK(bc_rec_store_reader_verify_step(&reader, 8U) == BC_REC_STORE_MORE);
    CHECK(bc_rec_store_reader_verify_step(&reader, 8U) == BC_REC_STORE_MORE);
    CHECK(bc_rec_store_reader_verify_step(&reader, 8U) == BC_REC_STORE_MORE);
    CHECK(bc_rec_store_reader_verify_step(&reader, 8U) == BC_REC_STORE_OK);
    CHECK(bc_rec_store_reader_read(&reader, 4U, read_back, sizeof(read_back)) ==
          (int32_t)sizeof(read_back));
    CHECK(memcmp(read_back, first + 4U, sizeof(read_back)) == 0);
    CHECK(bc_rec_store_reader_read(&reader, sizeof(first), read_back,
                                   sizeof(read_back)) == 0);
    CHECK(bc_rec_store_reader_close(&reader) == BC_REC_STORE_OK);

    CHECK(bc_rec_store_catalog_begin(&store, &catalog, 0U) ==
          BC_REC_STORE_OK);
    CHECK(bc_rec_store_catalog_next(&store, &catalog, &found, &file,
                                    &crc_verified) == BC_REC_STORE_OK);
    CHECK(found.id == 1U && !crc_verified);
    CHECK(bc_rec_store_catalog_next(&store, &catalog, &found, &file,
                                    &crc_verified) == BC_REC_STORE_OK);
    CHECK(found.id == 2U && !crc_verified && file.recovered);
    CHECK(bc_rec_store_catalog_next(&store, &catalog, &found, &file,
                                    &crc_verified) == BC_REC_STORE_END);
    CHECK(bc_rec_store_list_end(&store, &catalog) == BC_REC_STORE_OK);
}

static void corrupt_raw_data(struct test_fs *fs, struct ram_nor *ram,
                             uint64_t id)
{
    lfs_file_t file;
    struct lfs_file_config config;
    uint8_t cache[TEST_CACHE_SIZE];
    uint8_t attr[BC_REC_STORE_METADATA_SIZE];
    struct lfs_attr attribute;
    uint8_t byte;
    lfs_block_t block;
    lfs_off_t off;
    int error;

    memset(&config, 0, sizeof(config));
    memset(&attribute, 0, sizeof(attribute));
    attribute.type = BC_REC_STORE_META_ATTR;
    attribute.buffer = attr;
    attribute.size = sizeof(attr);
    config.buffer = cache;
    config.attrs = &attribute;
    config.attr_count = 1U;
    {
        char path[BC_REC_STORE_PATH_SIZE];
        (void)snprintf(path, sizeof(path), "/.sudo-rec/%016llx.raw",
                       (unsigned long long)id);
        error = lfs_file_opencfg(&fs->lfs, &file, path, LFS_O_RDONLY,
                                 &config);
    }
    CHECK(error == LFS_ERR_OK);
    CHECK(lfs_file_read(&fs->lfs, &file, &byte, 1U) == 1);
    block = file.block;
    off = file.off - 1U;
    CHECK(block < FLASH_BLOCK_COUNT);
    CHECK(lfs_file_close(&fs->lfs, &file) == LFS_ERR_OK);
    ram->bytes[(size_t)block * FLASH_BLOCK_SIZE + off] ^= 0x01U;
}

static void test_crc_corruption_and_empty(struct ram_nor *ram,
                                          struct test_fs *fs)
{
    bc_rec_store store;
    bc_rec_start start = start_for(1U);
    bc_rec_start empty_start = start_for(3U);
    bc_rec_file file;
    bc_rec_start found;
    bc_rec_store_reader reader;
    bc_rec_store_status status;

    corrupt_raw_data(fs, ram, start.id);
    CHECK(store_init(&store, fs));
    status = bc_rec_store_stat(&store, start.id, &found, &file);
    CHECK(status == BC_REC_STORE_OK && file.complete && file.bytes == 9U);
    status = bc_rec_store_lookup(&store, start.id, &found, &file);
    CHECK(status == BC_REC_STORE_CORRUPT);
    CHECK(bc_rec_store_reader_begin(&store, start.id, &reader, &found, &file) ==
          BC_REC_STORE_OK);
    CHECK(bc_rec_store_reader_verify_step(&reader, 1024U) ==
          BC_REC_STORE_CORRUPT);
    CHECK(bc_rec_store_reader_close(&reader) == BC_REC_STORE_OK);

    CHECK(bc_rec_store_open(&store, &empty_start, &file) == BC_REC_OK);
    CHECK(bc_rec_store_finish(&store, true, &file) == BC_REC_EMPTY_AUDIO);
    CHECK(!file.complete && file.bytes == 0U);
    CHECK(bc_rec_store_lookup(&store, empty_start.id, &found, &file) ==
          BC_REC_STORE_EMPTY);
}

static void test_program_failure_preserves_prefix(struct ram_nor *ram,
                                                  struct test_fs *fs)
{
    bc_rec_store store;
    bc_rec_start start = start_for(7U);
    bc_rec_start found;
    bc_rec_file file;
    uint8_t first[32];
    uint8_t second[32];
    bc_rec_result result;

    fill_pattern(first, sizeof(first), 0x12U);
    fill_pattern(second, sizeof(second), 0xa2U);
    CHECK(store_init(&store, fs));
    CHECK(bc_rec_store_open(&store, &start, &file) == BC_REC_OK);
    CHECK(bc_rec_store_append(&store, first, sizeof(first)) == BC_REC_OK);
    CHECK(bc_rec_store_checkpoint(&store, &file) == BC_REC_OK);
    ram->fail_prog_call = ram->prog_calls + 1U;
    result = bc_rec_store_append(&store, second, sizeof(second));
    if (result == BC_REC_OK)
        result = bc_rec_store_checkpoint(&store, &file);
    CHECK(result == BC_REC_WRITE_ERROR || result == BC_REC_SYNC_ERROR ||
          result == BC_REC_NO_SPACE);
    CHECK(file.bytes == sizeof(first));
    result = bc_rec_store_finish(&store, false, &file);
    CHECK(result == BC_REC_WRITE_ERROR || result == BC_REC_SYNC_ERROR);
    CHECK(file.bytes == sizeof(first));
    CHECK(lfs_unmount(&fs->lfs) == LFS_ERR_OK);
    CHECK(fs_mount_existing(fs, ram));
    CHECK(store_init(&store, fs));
    CHECK(bc_rec_store_lookup(&store, start.id, &found, &file) ==
          BC_REC_STORE_OK);
    CHECK(!file.complete && file.recovered && file.bytes == sizeof(first));
}

static void test_metadata_corruption_and_validation(struct test_fs *fs)
{
    bc_rec_store store;
    bc_rec_start start = start_for(1U);
    bc_rec_start found;
    bc_rec_file file;
    uint8_t attr[BC_REC_STORE_METADATA_SIZE];
    int error;

    error = (int)lfs_getattr(&fs->lfs, "/.sudo-rec/0000000000000001.raw",
                             BC_REC_STORE_META_ATTR, attr, sizeof(attr));
    CHECK(error == (int)sizeof(attr));
    attr[0] ^= 1U;
    CHECK(lfs_setattr(&fs->lfs, "/.sudo-rec/0000000000000001.raw",
                      BC_REC_STORE_META_ATTR, attr, sizeof(attr)) == LFS_ERR_OK);
    CHECK(store_init(&store, fs));
    CHECK(bc_rec_store_lookup(&store, start.id, &found, &file) ==
          BC_REC_STORE_CORRUPT);
}

static void test_receipt_tombstone(struct ram_nor *ram, struct test_fs *fs)
{
    bc_rec_store store;
    bc_rec_start start = start_for(4U);
    bc_rec_start found;
    bc_rec_file file;
    uint8_t data[48];
    bc_rec_store_status receipt_status;
    bc_rec_store_status list_status;
    bc_rec_store_cursor cursor;
    bc_rec_start listed_start;
    bc_rec_file listed_file;
    unsigned seen = 0U;
    unsigned i;

    fill_pattern(data, sizeof(data), 0x40U);
    CHECK(store_init(&store, fs));
    CHECK(bc_rec_store_open(&store, &start, &file) == BC_REC_OK);
    CHECK(bc_rec_store_append(&store, data, sizeof(data)) == BC_REC_OK);
    CHECK(bc_rec_store_finish(&store, true, &file) == BC_REC_OK);
    ram->fail_sync_call = ram->sync_calls + 2U;
    receipt_status = bc_rec_store_receipt(&store, start.id, file.bytes,
                                          file.crc32);
    CHECK(receipt_status == BC_REC_STORE_SYNC_ERROR);
    CHECK(lfs_unmount(&fs->lfs) == LFS_ERR_OK);
    CHECK(fs_mount_existing(fs, ram));
    CHECK(store_init(&store, fs));
    CHECK(bc_rec_store_lookup(&store, start.id, &found, &file) ==
          BC_REC_STORE_OK);
    CHECK(!file.delivered && file.complete && file.bytes == sizeof(data));
    CHECK(bc_rec_store_receipt(&store, start.id, file.bytes, file.crc32) ==
          BC_REC_STORE_OK);
    CHECK(bc_rec_store_list_begin(&store, &cursor) == BC_REC_STORE_OK);
    for (i = 0U; i < 64U; ++i) {
        list_status = bc_rec_store_metadata_next(&store, &cursor,
                                                 &listed_start, &listed_file);
        if (list_status == BC_REC_STORE_END)
            break;
        if (list_status == BC_REC_STORE_MORE)
            continue;
        CHECK(list_status == BC_REC_STORE_OK);
        if (list_status == BC_REC_STORE_OK && listed_start.id == start.id) {
            ++seen;
            CHECK(listed_file.delivered && listed_file.bytes == sizeof(data));
        }
    }
    CHECK(seen == 1U);
    CHECK(bc_rec_store_list_end(&store, &cursor) == BC_REC_STORE_OK);
    CHECK(bc_rec_store_receipt(&store, start.id, file.bytes, file.crc32) ==
          BC_REC_STORE_OK);
    /* Custody is already persisted, so cleanup remains safe even if raw
     * storage develops a later bit error. */
    corrupt_raw_data(fs, ram, start.id);
    CHECK(bc_rec_store_delete(&store, start.id, file.bytes, file.crc32) ==
          BC_REC_STORE_OK);
    CHECK(bc_rec_store_lookup(&store, start.id, &found, &file) ==
          BC_REC_STORE_OK);
    CHECK(found.id == start.id && file.complete && file.bytes == sizeof(data));
    CHECK(bc_rec_store_read(&store, start.id, 0U, data, sizeof(data)) ==
          BC_REC_STORE_DELETED);
    CHECK(bc_rec_store_open(&store, &start, &file) == BC_REC_ALREADY_EXISTS);
    CHECK(file.complete && file.bytes == sizeof(data));
    CHECK(bc_rec_store_delete(&store, start.id, file.bytes, file.crc32) ==
          BC_REC_STORE_OK);

    CHECK(lfs_unmount(&fs->lfs) == LFS_ERR_OK);
    CHECK(fs_mount_existing(fs, ram));
}

static void test_full_storage_and_bad_name(struct test_fs *fs)
{
    bc_rec_store store;
    bc_rec_store bad_store;
    bc_rec_start start = start_for(5U);
    bc_rec_file file;
    uint8_t data[BC_REC_FRAME_MAX];
    unsigned i;
    bc_rec_result result = BC_REC_OK;
    char bad_name[BC_REC_NAME_SIZE] = "../outside.raw";

    fill_pattern(data, sizeof(data), 0x11U);
    CHECK(store_init(&store, fs));
    CHECK(bc_rec_store_open(&store, &start, &file) == BC_REC_OK);
    for (i = 0; i < 200U && result == BC_REC_OK; ++i) {
        result = bc_rec_store_append(&store, data, sizeof(data));
        if (result == BC_REC_OK && (i % 3U) == 0U)
            result = bc_rec_store_checkpoint(&store, &file);
    }
    CHECK(result == BC_REC_NO_SPACE || i == 200U);
    if (store.active)
        (void)bc_rec_store_finish(&store, false, &file);

    CHECK(bc_rec_store_init(&bad_store, &fs->lfs, traversal_name, bad_name));
    start.id = 6U;
    CHECK(bc_rec_store_open(&bad_store, &start, &file) == BC_REC_INVALID);
}

static bool matrix_cut_requested(const struct ram_nor *ram)
{
    return ram->cut_triggered || ram->cut_armed;
}

static void matrix_abandon(struct test_fs *fs)
{
    /* Deliberately do not call lfs_unmount: this is the simulated power cut. */
    memset(fs, 0, sizeof(*fs));
}

static bool matrix_make_complete_record(struct test_fs *fs, uint64_t id,
                                        const uint8_t *data, uint16_t length)
{
    bc_rec_store store;
    bc_rec_start start = start_for(id);
    bc_rec_file file;

    if (!store_init(&store, fs) ||
        bc_rec_store_open(&store, &start, &file) != BC_REC_OK ||
        bc_rec_store_append(&store, data, length) != BC_REC_OK ||
        bc_rec_store_finish(&store, true, &file) != BC_REC_OK ||
        !file.complete || file.bytes != length)
        return false;
    return true;
}

static bc_rec_result matrix_capture_sequence(struct ram_nor *ram,
                                              struct test_fs *fs,
                                              uint64_t id, bool *abandoned)
{
    bc_rec_store store;
    bc_rec_start start = start_for(id);
    bc_rec_file file;
    uint8_t first[BC_REC_FRAME_MAX];
    uint8_t second[BC_REC_FRAME_MAX];
    bc_rec_result result;

    *abandoned = false;
    fill_pattern(first, sizeof(first), 0x24U);
    fill_pattern(second, sizeof(second), 0xa4U);
    if (!store_init(&store, fs))
        return BC_REC_OPEN_ERROR;

    result = bc_rec_store_open(&store, &start, &file);
    if (matrix_cut_requested(ram)) {
        *abandoned = true;
        return result;
    }
    if (result != BC_REC_OK)
        return result;

    result = bc_rec_store_append(&store, first, sizeof(first));
    if (matrix_cut_requested(ram)) {
        *abandoned = true;
        return result;
    }
    if (result != BC_REC_OK)
        return result;

    result = bc_rec_store_checkpoint(&store, &file);
    if (matrix_cut_requested(ram)) {
        *abandoned = true;
        return result;
    }
    if (result != BC_REC_OK)
        return result;

    result = bc_rec_store_append(&store, second, sizeof(second));
    if (matrix_cut_requested(ram)) {
        *abandoned = true;
        return result;
    }
    if (result != BC_REC_OK)
        return result;

    result = bc_rec_store_finish(&store, true, &file);
    if (matrix_cut_requested(ram))
        *abandoned = true;
    return result;
}

static void matrix_check_record_state(struct ram_nor *ram, struct test_fs *fs,
                                      uint64_t durable_id, uint64_t id)
{
    bc_rec_store store;
    bc_rec_start found;
    bc_rec_start start = start_for(id);
    bc_rec_file file;
    bc_rec_file reopened;
    bc_rec_store_status status;
    bc_rec_result open_result;

    ram_reset_io(ram);
    if (!fs_mount_existing(fs, ram)) {
        CHECK(false);
        matrix_abandon(fs);
        return;
    }
    if (!store_init(&store, fs)) {
        CHECK(false);
        CHECK(lfs_unmount(&fs->lfs) == LFS_ERR_OK);
        return;
    }

    status = bc_rec_store_lookup(&store, durable_id, &found, &file);
    CHECK(status == BC_REC_STORE_OK && file.complete && file.bytes != 0U);

    status = bc_rec_store_lookup(&store, id, &found, &file);
    CHECK(status == BC_REC_STORE_OK || status == BC_REC_STORE_EMPTY ||
          status == BC_REC_STORE_NOT_FOUND || status == BC_REC_STORE_CORRUPT);
    if (status == BC_REC_STORE_OK) {
        /* lookup itself performs the full committed-prefix CRC check. */
        CHECK(file.bytes != 0U);
        CHECK(file.recovered == !file.complete);
    } else if (status == BC_REC_STORE_EMPTY) {
        CHECK(file.bytes == 0U && !file.complete);
    }

    /* A persisted id is never silently reused.  An absent id may be opened
     * again because no durable record survived this cut. */
    open_result = bc_rec_store_open(&store, &start, &reopened);
    if (status == BC_REC_STORE_OK) {
        CHECK(open_result == BC_REC_ALREADY_EXISTS);
    } else if (status == BC_REC_STORE_EMPTY) {
        CHECK(open_result == BC_REC_EMPTY_AUDIO);
    } else if (status == BC_REC_STORE_CORRUPT) {
        CHECK(open_result != BC_REC_OK);
    } else {
        if (open_result == BC_REC_OK)
            CHECK(bc_rec_store_finish(&store, false, &reopened) !=
                  BC_REC_INVALID);
        else
            CHECK(open_result != BC_REC_ALREADY_EXISTS);
    }
    CHECK(lfs_unmount(&fs->lfs) == LFS_ERR_OK);
}

static void matrix_run_capture_cut(struct ram_nor *ram, struct test_fs *fs,
                                   const uint8_t *base, uint64_t durable_id,
                                   uint64_t id, enum ram_cut_kind kind,
                                   enum ram_cut_mode mode, unsigned call)
{
    bc_rec_result result;
    bool abandoned;

    memcpy(ram->bytes, base, sizeof(ram->bytes));
    ram_reset_io(ram);
    if (!fs_mount_existing(fs, ram)) {
        CHECK(false);
        matrix_abandon(fs);
        return;
    }
    ram_set_cut(ram, kind, mode, call);
    result = matrix_capture_sequence(ram, fs, id, &abandoned);
    CHECK(matrix_cut_requested(ram));
    (void)result;
    (void)abandoned;
    matrix_abandon(fs);
    matrix_check_record_state(ram, fs, durable_id, id);
}

static void test_capture_power_cut_matrix(void)
{
    struct ram_nor ram;
    struct test_fs fs;
    uint8_t base[FLASH_SIZE];
    uint8_t durable_data[BC_REC_FRAME_MAX];
    uint8_t first_data[BC_REC_FRAME_MAX];
    uint8_t second_data[BC_REC_FRAME_MAX];
    bc_rec_store store;
    bc_rec_start durable_start = start_for(0x100U);
    bc_rec_start target_start = start_for(0x200U);
    bc_rec_file target_file;
    unsigned prog_count;
    unsigned erase_count;
    unsigned sync_count;
    unsigned call;
    enum ram_cut_kind kind;

    memset(&ram, 0, sizeof(ram));
    memset(ram.bytes, 0xff, sizeof(ram.bytes));
    CHECK(fs_format_mount(&fs, &ram));
    fill_pattern(durable_data, sizeof(durable_data), 0x10U);
    CHECK(matrix_make_complete_record(&fs, durable_start.id, durable_data,
                                      sizeof(durable_data)));
    CHECK(lfs_unmount(&fs.lfs) == LFS_ERR_OK);
    memcpy(base, ram.bytes, sizeof(base));

    /* Capture the complete operation's real primitive counts from the same
     * durable image.  Every cut case below starts from this exact image. */
    if (!fs_mount_existing(&fs, &ram)) {
        CHECK(false);
        matrix_abandon(&fs);
        return;
    }
    ram_reset_io(&ram);
    fill_pattern(first_data, sizeof(first_data), 0x24U);
    fill_pattern(second_data, sizeof(second_data), 0xa4U);
    CHECK(store_init(&store, &fs));
    CHECK(bc_rec_store_open(&store, &target_start, &target_file) == BC_REC_OK);
    CHECK(bc_rec_store_append(&store, first_data, sizeof(first_data)) ==
          BC_REC_OK);
    CHECK(bc_rec_store_checkpoint(&store, &target_file) == BC_REC_OK);
    CHECK(bc_rec_store_append(&store, second_data, sizeof(second_data)) ==
          BC_REC_OK);
    CHECK(bc_rec_store_finish(&store, true, &target_file) == BC_REC_OK);
    prog_count = ram.prog_calls;
    erase_count = ram.erase_calls;
    sync_count = ram.sync_calls;
    CHECK(prog_count != 0U && erase_count != 0U && sync_count != 0U);
    CHECK(lfs_unmount(&fs.lfs) == LFS_ERR_OK);

    for (kind = RAM_CUT_PROG; kind <= RAM_CUT_SYNC; ++kind) {
        unsigned count = kind == RAM_CUT_PROG ? prog_count :
                         kind == RAM_CUT_ERASE ? erase_count : sync_count;
        for (call = 1U; call <= count; ++call) {
            matrix_run_capture_cut(&ram, &fs, base, durable_start.id,
                                   target_start.id, kind,
                                   RAM_CUT_MODE_BEFORE, call);
            if (kind != RAM_CUT_SYNC) {
                matrix_run_capture_cut(&ram, &fs, base, durable_start.id,
                                       target_start.id, kind,
                                       RAM_CUT_MODE_PARTIAL, call);
            }
            matrix_run_capture_cut(&ram, &fs, base, durable_start.id,
                                   target_start.id, kind,
                                   RAM_CUT_MODE_AFTER, call);
        }
    }
    fprintf(stdout, "capture power-cut matrix: prog=%u erase=%u sync=%u\n",
            prog_count, erase_count, sync_count);
}

static void matrix_path_for_id(char path[BC_REC_STORE_PATH_SIZE],
                               uint64_t id, const char suffix[])
{
    static const char hex[] = "0123456789abcdef";
    size_t n = 0U;
    int i;

    memcpy(path, BC_REC_STORE_DIR, sizeof(BC_REC_STORE_DIR) - 1U);
    n = sizeof(BC_REC_STORE_DIR) - 1U;
    path[n++] = '/';
    for (i = 15; i >= 0; --i)
        path[n++] = hex[(id >> (4U * (unsigned)i)) & 0xfU];
    while (*suffix != '\0')
        path[n++] = *suffix++;
    path[n] = '\0';
}

static int matrix_path_stat(struct test_fs *fs, uint64_t id,
                            const char suffix[], struct lfs_info *info)
{
    char path[BC_REC_STORE_PATH_SIZE];

    matrix_path_for_id(path, id, suffix);
    return lfs_stat(&fs->lfs, path, info);
}

static bool matrix_make_receipt_base(struct ram_nor *ram, struct test_fs *fs,
                                     uint64_t durable_id, uint64_t id,
                                     const uint8_t *data, uint16_t length,
                                     uint8_t base[FLASH_SIZE],
                                     uint32_t *bytes, uint32_t *crc32)
{
    bc_rec_store store;
    bc_rec_start start = start_for(id);
    bc_rec_file file;

    memset(ram, 0, sizeof(*ram));
    memset(ram->bytes, 0xff, sizeof(ram->bytes));
    if (!fs_format_mount(fs, ram))
        return false;
    if (!matrix_make_complete_record(fs, durable_id, data, length))
        return false;
    if (!matrix_make_complete_record(fs, id, data, length))
        return false;
    if (lfs_unmount(&fs->lfs) != LFS_ERR_OK)
        return false;
    memcpy(base, ram->bytes, sizeof(ram->bytes));

    if (!fs_mount_existing(fs, ram))
        return false;
    ram_reset_io(ram);
    if (!store_init(&store, fs) ||
        bc_rec_store_lookup(&store, id, &start, &file) != BC_REC_STORE_OK)
        return false;
    *bytes = file.bytes;
    *crc32 = file.crc32;
    if (file.bytes != length || !file.complete)
        return false;
    if (lfs_unmount(&fs->lfs) != LFS_ERR_OK)
        return false;
    return true;
}

static bc_rec_store_status matrix_receipt_sequence(struct ram_nor *ram,
                                                    struct test_fs *fs,
                                                    uint64_t id,
                                                    uint32_t bytes,
                                                    uint32_t crc32,
                                                    bool *abandoned)
{
    bc_rec_store store;
    bc_rec_store_status result;

    *abandoned = false;
    if (!store_init(&store, fs))
        return BC_REC_STORE_OPEN_ERROR;
    result = bc_rec_store_receipt(&store, id, bytes, crc32);
    if (matrix_cut_requested(ram))
        *abandoned = true;
    return result;
}

static void matrix_check_receipt_state(struct ram_nor *ram,
                                       struct test_fs *fs,
                                       uint64_t durable_id, uint64_t id,
                                       uint32_t bytes, uint32_t crc32)
{
    bc_rec_store store;
    bc_rec_start found;
    bc_rec_file file;
    bc_rec_store_status status;
    bc_rec_store_status retry;
    struct lfs_info info;

    ram_reset_io(ram);
    if (!fs_mount_existing(fs, ram)) {
        CHECK(false);
        matrix_abandon(fs);
        return;
    }
    if (!store_init(&store, fs)) {
        CHECK(false);
        matrix_abandon(fs);
        return;
    }
    status = bc_rec_store_lookup(&store, durable_id, &found, &file);
    CHECK(status == BC_REC_STORE_OK && file.complete && file.bytes != 0U);

    /* receipt() never removes raw audio, including when the power cut lands
     * after the tombstone commit.  A damaged or uncommitted companion may
     * make the combined lookup report CORRUPT, but the raw entry must remain
     * present for a retry. */
    CHECK(matrix_path_stat(fs, id, ".raw", &info) == LFS_ERR_OK);
    status = bc_rec_store_lookup(&store, id, &found, &file);
    CHECK(status == BC_REC_STORE_OK || status == BC_REC_STORE_CORRUPT);
    if (status == BC_REC_STORE_OK) {
        CHECK(file.complete && file.bytes == bytes && file.crc32 == crc32);
        retry = bc_rec_store_receipt(&store, id, bytes, crc32);
        CHECK(retry == BC_REC_STORE_OK ||
              retry == BC_REC_STORE_CORRUPT ||
              retry == BC_REC_STORE_OPEN_ERROR ||
              retry == BC_REC_STORE_SYNC_ERROR ||
              retry == BC_REC_STORE_CLOSE_ERROR ||
              retry == BC_REC_STORE_NO_SPACE);
        if (retry == BC_REC_STORE_OK) {
            CHECK(bc_rec_store_lookup(&store, id, &found, &file) ==
                  BC_REC_STORE_OK);
            CHECK(file.complete && file.delivered && file.bytes == bytes &&
                  file.crc32 == crc32);
        }
    } else {
        retry = bc_rec_store_receipt(&store, id, bytes, crc32);
        CHECK(retry != BC_REC_STORE_OK);
    }
    CHECK(matrix_path_stat(fs, id, ".raw", &info) == LFS_ERR_OK);
    CHECK(lfs_unmount(&fs->lfs) == LFS_ERR_OK);
}

static void matrix_run_receipt_cut(struct ram_nor *ram, struct test_fs *fs,
                                    const uint8_t *base, uint64_t durable_id,
                                    uint64_t id, uint32_t bytes, uint32_t crc32,
                                    enum ram_cut_kind kind,
                                    enum ram_cut_mode mode, unsigned call)
{
    bc_rec_store_status result;
    bool abandoned;

    memcpy(ram->bytes, base, sizeof(ram->bytes));
    ram_reset_io(ram);
    if (!fs_mount_existing(fs, ram)) {
        CHECK(false);
        matrix_abandon(fs);
        return;
    }
    ram_set_cut(ram, kind, mode, call);
    result = matrix_receipt_sequence(ram, fs, id, bytes, crc32, &abandoned);
    CHECK(matrix_cut_requested(ram));
    (void)result;
    (void)abandoned;
    matrix_abandon(fs);
    matrix_check_receipt_state(ram, fs, durable_id, id, bytes, crc32);
}

static bc_rec_store_status matrix_delete_sequence(struct ram_nor *ram,
                                                   struct test_fs *fs,
                                                   uint64_t id,
                                                   uint32_t bytes,
                                                   uint32_t crc32,
                                                   bool *abandoned)
{
    bc_rec_store store;
    bc_rec_store_status result;

    *abandoned = false;
    if (!store_init(&store, fs))
        return BC_REC_STORE_OPEN_ERROR;
    result = bc_rec_store_delete(&store, id, bytes, crc32);
    if (matrix_cut_requested(ram))
        *abandoned = true;
    return result;
}

static void matrix_check_delete_state(struct ram_nor *ram, struct test_fs *fs,
                                      uint64_t durable_id, uint64_t id,
                                      uint32_t bytes, uint32_t crc32)
{
    bc_rec_store store;
    bc_rec_start found;
    bc_rec_file file;
    bc_rec_store_status status;
    bc_rec_store_status retry;
    struct lfs_info info;

    ram_reset_io(ram);
    if (!fs_mount_existing(fs, ram)) {
        CHECK(false);
        matrix_abandon(fs);
        return;
    }
    if (!store_init(&store, fs)) {
        CHECK(false);
        matrix_abandon(fs);
        return;
    }
    status = bc_rec_store_lookup(&store, durable_id, &found, &file);
    CHECK(status == BC_REC_STORE_OK && file.complete && file.bytes != 0U);

    /* The receipt image was durable before the remove cut.  The tombstone
     * must remain valid and a retry must be able to finish cleanup whether
     * the first remove reached the directory commit or not. */
    status = bc_rec_store_lookup(&store, id, &found, &file);
    CHECK(status == BC_REC_STORE_OK);
    CHECK(file.complete && file.delivered && file.bytes == bytes &&
          file.crc32 == crc32);
    CHECK(matrix_path_stat(fs, id, ".done", &info) == LFS_ERR_OK);
    retry = bc_rec_store_delete(&store, id, bytes, crc32);
    CHECK(retry == BC_REC_STORE_OK);
    CHECK(matrix_path_stat(fs, id, ".raw", &info) == LFS_ERR_NOENT);
    status = bc_rec_store_lookup(&store, id, &found, &file);
    CHECK(status == BC_REC_STORE_OK && file.delivered &&
          file.bytes == bytes && file.crc32 == crc32);
    {
        bc_rec_start start = start_for(id);
        CHECK(bc_rec_store_open(&store, &start, &file) == BC_REC_ALREADY_EXISTS);
        CHECK(file.complete && file.delivered && file.bytes == bytes &&
              file.crc32 == crc32);
    }
    CHECK(lfs_unmount(&fs->lfs) == LFS_ERR_OK);
}

static void matrix_run_delete_cut(struct ram_nor *ram, struct test_fs *fs,
                                   const uint8_t *base, uint64_t durable_id,
                                   uint64_t id, uint32_t bytes, uint32_t crc32,
                                   enum ram_cut_kind kind,
                                   enum ram_cut_mode mode, unsigned call)
{
    bc_rec_store_status result;
    bool abandoned;

    memcpy(ram->bytes, base, sizeof(ram->bytes));
    ram_reset_io(ram);
    if (!fs_mount_existing(fs, ram)) {
        CHECK(false);
        matrix_abandon(fs);
        return;
    }
    ram_set_cut(ram, kind, mode, call);
    result = matrix_delete_sequence(ram, fs, id, bytes, crc32, &abandoned);
    CHECK(matrix_cut_requested(ram));
    (void)result;
    (void)abandoned;
    matrix_abandon(fs);
    matrix_check_delete_state(ram, fs, durable_id, id, bytes, crc32);
}

static void test_receipt_delete_power_cut_matrix(void)
{
    struct ram_nor ram;
    struct test_fs fs;
    uint8_t base[FLASH_SIZE];
    uint8_t receipt_base[FLASH_SIZE];
    uint8_t delete_base[FLASH_SIZE];
    uint8_t data[BC_REC_FRAME_MAX];
    uint64_t durable_id = 0x301U;
    uint64_t id = 0x302U;
    uint32_t bytes;
    uint32_t crc32;
    unsigned receipt_prog_count;
    unsigned receipt_erase_count;
    unsigned receipt_sync_count;
    unsigned delete_prog_count;
    unsigned delete_erase_count;
    unsigned delete_sync_count;
    unsigned call;
    unsigned count;
    enum ram_cut_kind kind;

    fill_pattern(data, sizeof(data), 0x38U);
    if (!matrix_make_receipt_base(&ram, &fs, durable_id, id, data,
                                  sizeof(data), base, &bytes, &crc32)) {
        CHECK(false);
        matrix_abandon(&fs);
        return;
    }

    /* Capture the exact receipt cut points and retain the committed receipt
     * image as the delete test's setup. */
    if (!fs_mount_existing(&fs, &ram)) {
        CHECK(false);
        matrix_abandon(&fs);
        return;
    }
    ram_reset_io(&ram);
    {
        bc_rec_store store;
        CHECK(store_init(&store, &fs));
        CHECK(bc_rec_store_receipt(&store, id, bytes, crc32) ==
              BC_REC_STORE_OK);
    }
    receipt_prog_count = ram.prog_calls;
    receipt_erase_count = ram.erase_calls;
    receipt_sync_count = ram.sync_calls;
    CHECK(receipt_prog_count != 0U && receipt_erase_count != 0U &&
          receipt_sync_count != 0U);
    CHECK(lfs_unmount(&fs.lfs) == LFS_ERR_OK);
    memcpy(receipt_base, ram.bytes, sizeof(receipt_base));

    for (kind = RAM_CUT_PROG; kind <= RAM_CUT_SYNC; ++kind) {
        count = kind == RAM_CUT_PROG ? receipt_prog_count :
                kind == RAM_CUT_ERASE ? receipt_erase_count : receipt_sync_count;
        for (call = 1U; call <= count; ++call) {
            matrix_run_receipt_cut(&ram, &fs, base, durable_id, id, bytes,
                                   crc32, kind, RAM_CUT_MODE_BEFORE, call);
            if (kind != RAM_CUT_SYNC)
                matrix_run_receipt_cut(&ram, &fs, base, durable_id, id, bytes,
                                       crc32, kind, RAM_CUT_MODE_PARTIAL, call);
            matrix_run_receipt_cut(&ram, &fs, base, durable_id, id, bytes,
                                   crc32, kind, RAM_CUT_MODE_AFTER, call);
        }
    }
    fprintf(stdout, "receipt power-cut matrix: prog=%u erase=%u sync=%u\n",
            receipt_prog_count, receipt_erase_count, receipt_sync_count);

    /* One no-cut delete establishes the actual primitive count.  It is also
     * the exact durable image every delete case below starts from. */
    memcpy(ram.bytes, receipt_base, sizeof(ram.bytes));
    if (!fs_mount_existing(&fs, &ram)) {
        CHECK(false);
        matrix_abandon(&fs);
        return;
    }
    ram_reset_io(&ram);
    {
        bc_rec_store store;
        CHECK(store_init(&store, &fs));
        CHECK(bc_rec_store_delete(&store, id, bytes, crc32) ==
              BC_REC_STORE_OK);
    }
    delete_prog_count = ram.prog_calls;
    delete_erase_count = ram.erase_calls;
    delete_sync_count = ram.sync_calls;
    CHECK(delete_prog_count != 0U && delete_sync_count != 0U);
    CHECK(lfs_unmount(&fs.lfs) == LFS_ERR_OK);
    memcpy(delete_base, receipt_base, sizeof(delete_base));

    for (kind = RAM_CUT_PROG; kind <= RAM_CUT_SYNC; ++kind) {
        count = kind == RAM_CUT_PROG ? delete_prog_count :
                kind == RAM_CUT_ERASE ? delete_erase_count : delete_sync_count;
        for (call = 1U; call <= count; ++call) {
            matrix_run_delete_cut(&ram, &fs, delete_base, durable_id, id,
                                  bytes, crc32, kind, RAM_CUT_MODE_BEFORE,
                                  call);
            if (kind != RAM_CUT_SYNC)
                matrix_run_delete_cut(&ram, &fs, delete_base, durable_id, id,
                                      bytes, crc32, kind,
                                      RAM_CUT_MODE_PARTIAL, call);
            matrix_run_delete_cut(&ram, &fs, delete_base, durable_id, id,
                                  bytes, crc32, kind, RAM_CUT_MODE_AFTER, call);
        }
    }
    fprintf(stdout, "delete power-cut matrix: prog=%u erase=%u sync=%u\n",
            delete_prog_count, delete_erase_count, delete_sync_count);
}

int main(void)
{
    struct ram_nor ram;
    struct test_fs fs;

    memset(&ram, 0, sizeof(ram));
    memset(ram.bytes, 0xff, sizeof(ram.bytes));
    CHECK(fs_format_mount(&fs, &ram));
    if (failures == 0U) {
        test_crc_duplicate_and_bounded_read(&ram, &fs);
        test_partial_recovery(&ram, &fs);
        test_program_failure_preserves_prefix(&ram, &fs);
        test_receipt_tombstone(&ram, &fs);
        test_crc_corruption_and_empty(&ram, &fs);
        test_metadata_corruption_and_validation(&fs);
        test_full_storage_and_bad_name(&fs);
        test_capture_power_cut_matrix();
        test_receipt_delete_power_cut_matrix();
    }
    if (fs.lfs.cfg != NULL)
        CHECK(lfs_unmount(&fs.lfs) == LFS_ERR_OK);
    fprintf(stdout, "%u checks, %u failures\n", checks, failures);
    return failures == 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
