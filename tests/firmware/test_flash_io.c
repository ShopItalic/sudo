#include "lfs_port.h"

#include <sfud.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern const struct lfs_config cfg;

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

static sfud_flash mocked_flash;
static sfud_err mocked_init_result;
static sfud_err mocked_read_result;
static sfud_err mocked_write_result;
static sfud_err mocked_erase_result;
static int mocked_mount_result;
static int mocked_format_result;

static unsigned open_calls;
static unsigned close_calls;
static unsigned sfud_init_calls;
static unsigned read_calls;
static unsigned write_calls;
static unsigned erase_calls;
static unsigned mount_calls;
static unsigned format_calls;
static uint32_t last_read_address;
static uint32_t last_write_address;
static uint32_t last_erase_address;
static size_t last_read_size;
static size_t last_write_size;
static size_t last_erase_size;
static const struct lfs_config *mounted_config;

void bc_spi_flash_device_open(void)
{
    ++open_calls;
}

void bc_spi_flash_device_close(void)
{
    ++close_calls;
}

sfud_err sfud_init(void)
{
    ++sfud_init_calls;
    return mocked_init_result;
}

const sfud_flash *sfud_get_device_table(void)
{
    return &mocked_flash;
}

sfud_err sfud_read(const sfud_flash *flash, uint32_t address, size_t size,
                   uint8_t *data)
{
    ++read_calls;
    CHECK(flash == &mocked_flash);
    last_read_address = address;
    last_read_size = size;
    if (mocked_read_result == SFUD_SUCCESS && data != NULL)
        memset(data, 0xa5, size);
    return mocked_read_result;
}

sfud_err sfud_write(const sfud_flash *flash, uint32_t address, size_t size,
                    const uint8_t *data)
{
    ++write_calls;
    CHECK(flash == &mocked_flash);
    CHECK(data != NULL);
    last_write_address = address;
    last_write_size = size;
    return mocked_write_result;
}

sfud_err sfud_erase(const sfud_flash *flash, uint32_t address, size_t size)
{
    ++erase_calls;
    CHECK(flash == &mocked_flash);
    last_erase_address = address;
    last_erase_size = size;
    return mocked_erase_result;
}

int lfs_mount(lfs_t *lfs, const struct lfs_config *config)
{
    ++mount_calls;
    CHECK(lfs != NULL);
    mounted_config = config;
    return mocked_mount_result;
}

int lfs_format(lfs_t *lfs, const struct lfs_config *config)
{
    ++format_calls;
    CHECK(lfs != NULL);
    CHECK(config == &cfg);
    return mocked_format_result;
}

static void reset_fixture(void)
{
    memset(&mocked_flash, 0, sizeof(mocked_flash));
    mocked_init_result = SFUD_SUCCESS;
    mocked_read_result = SFUD_SUCCESS;
    mocked_write_result = SFUD_SUCCESS;
    mocked_erase_result = SFUD_SUCCESS;
    mocked_mount_result = LFS_ERR_OK;
    mocked_format_result = LFS_ERR_OK;
    open_calls = 0;
    close_calls = 0;
    sfud_init_calls = 0;
    read_calls = 0;
    write_calls = 0;
    erase_calls = 0;
    mount_calls = 0;
    format_calls = 0;
    last_read_address = 0;
    last_write_address = 0;
    last_erase_address = 0;
    last_read_size = 0;
    last_write_size = 0;
    last_erase_size = 0;
    mounted_config = NULL;
}

static size_t selected_filesystem_size(void)
{
    return (size_t)cfg.block_size * cfg.block_count;
}

static bool init_valid_filesystem(lfs_t *lfs)
{
    reset_fixture();
    mocked_flash.init_ok = true;
    mocked_flash.chip.capacity = selected_filesystem_size();
    return lfs_sfud_init(lfs) == LFS_ERR_OK;
}

static void test_initialization_gates(void)
{
    lfs_t lfs;

    memset(&lfs, 0, sizeof(lfs));
    reset_fixture();
    CHECK(lfs_sfud_init(NULL) == LFS_ERR_INVAL);
    CHECK(open_calls == 0);
    CHECK(close_calls == 0);
    CHECK(sfud_init_calls == 0);
    CHECK(mount_calls == 0);

    reset_fixture();
    mocked_flash.init_ok = true;
    mocked_flash.chip.capacity = selected_filesystem_size();
    mocked_init_result = SFUD_ERR_READ;
    CHECK(lfs_sfud_init(&lfs) == LFS_ERR_IO);
    CHECK(open_calls == 0);
    CHECK(close_calls == 0);
    CHECK(sfud_init_calls == 1);
    CHECK(mount_calls == 0);
    CHECK(format_calls == 0);

    reset_fixture();
    mocked_flash.init_ok = false;
    mocked_flash.chip.capacity = selected_filesystem_size();
    CHECK(lfs_sfud_init(&lfs) == LFS_ERR_IO);
    CHECK(open_calls == 0);
    CHECK(close_calls == 0);
    CHECK(sfud_init_calls == 1);
    CHECK(mount_calls == 0);

    reset_fixture();
    mocked_flash.init_ok = true;
    mocked_flash.chip.capacity = selected_filesystem_size() - 1;
    CHECK(lfs_sfud_init(&lfs) == LFS_ERR_INVAL);
    CHECK(open_calls == 0);
    CHECK(close_calls == 0);
    CHECK(sfud_init_calls == 1);
    CHECK(mount_calls == 0);

    reset_fixture();
    mocked_flash.init_ok = true;
    mocked_flash.chip.capacity = selected_filesystem_size();
    mocked_mount_result = LFS_ERR_CORRUPT;
    CHECK(lfs_sfud_init(&lfs) == LFS_ERR_CORRUPT);
    CHECK(mount_calls == 1);
    CHECK(format_calls == 0);
    CHECK(open_calls == 0);
    CHECK(close_calls == 0);
}

static void test_jedec_metadata(void)
{
    lfs_t lfs;
    uint32_t expected;
    unsigned open_before;
    unsigned close_before;
    unsigned init_before;
    unsigned read_before;
    unsigned write_before;
    unsigned erase_before;
    unsigned mount_before;
    unsigned format_before;

    memset(&lfs, 0, sizeof(lfs));
    reset_fixture();
    mocked_init_result = SFUD_ERR_READ;
    CHECK(lfs_sfud_init(&lfs) == LFS_ERR_IO);
    CHECK(lfs_sfud_jedec_id() == 0U);

    reset_fixture();
    mocked_flash.init_ok = true;
    mocked_flash.chip.capacity = selected_filesystem_size();
    mocked_flash.chip.mf_id = 0xefU;
    mocked_flash.chip.type_id = 0x40U;
    mocked_flash.chip.capacity_id = 0x16U;
    expected = ((uint32_t)mocked_flash.chip.mf_id << 16) |
               ((uint32_t)mocked_flash.chip.type_id << 8) |
               (uint32_t)mocked_flash.chip.capacity_id;
    CHECK(lfs_sfud_init(&lfs) == LFS_ERR_OK);
    CHECK(open_calls == 0);
    CHECK(close_calls == 0);
    CHECK(lfs_sfud_jedec_id() == expected);

    open_before = open_calls;
    close_before = close_calls;
    init_before = sfud_init_calls;
    read_before = read_calls;
    write_before = write_calls;
    erase_before = erase_calls;
    mount_before = mount_calls;
    format_before = format_calls;
    CHECK(lfs_sfud_jedec_id() == expected);
    CHECK(open_calls == open_before);
    CHECK(close_calls == close_before);
    CHECK(sfud_init_calls == init_before);
    CHECK(read_calls == read_before);
    CHECK(write_calls == write_before);
    CHECK(erase_calls == erase_before);
    CHECK(mount_calls == mount_before);
    CHECK(format_calls == format_before);

    reset_fixture();
    mocked_flash.init_ok = true;
    CHECK(lfs_sfud_jedec_id() == 0U);
}

static void test_format_is_denied(void)
{
    lfs_t lfs;

    memset(&lfs, 0, sizeof(lfs));
    reset_fixture();
    CHECK(lfs_sfud_format(&lfs) == LFS_ERR_INVAL);
    CHECK(open_calls == 0);
    CHECK(close_calls == 0);
    CHECK(mount_calls == 0);
    CHECK(format_calls == 0);
}

static void test_block_operations(void)
{
    lfs_t lfs;
    uint8_t data[8] = {0};
    uint64_t filesystem_size;
    uint32_t last_address;
    unsigned calls_before;

    memset(&lfs, 0, sizeof(lfs));
    CHECK(init_valid_filesystem(&lfs));
    CHECK(mounted_config == &cfg);
    CHECK(mounted_config != NULL);
    if (mounted_config == NULL)
        return;

    filesystem_size = (uint64_t)cfg.block_size * cfg.block_count;
    last_address = (uint32_t)(filesystem_size - 1U);

    mocked_read_result = SFUD_ERR_READ;
    CHECK(mounted_config->read(mounted_config, 0, 0, data, sizeof(data)) ==
          LFS_ERR_IO);
    CHECK(read_calls == 1);

    mocked_read_result = SFUD_SUCCESS;
    CHECK(mounted_config->read(mounted_config, cfg.block_count - 1,
                               cfg.block_size - 1, data, 1) == LFS_ERR_OK);
    CHECK(read_calls == 2);
    CHECK(last_read_address == last_address);
    CHECK(last_read_size == 1);
    CHECK(data[0] == 0xa5);

    mocked_write_result = SFUD_ERR_WRITE;
    CHECK(mounted_config->prog(mounted_config, 0, 0, data, sizeof(data)) ==
          LFS_ERR_IO);
    CHECK(write_calls == 1);

    mocked_write_result = SFUD_SUCCESS;
    CHECK(mounted_config->prog(mounted_config, cfg.block_count - 1,
                               cfg.block_size - sizeof(data), data,
                               sizeof(data)) == LFS_ERR_OK);
    CHECK(write_calls == 2);
    CHECK(last_write_address == (uint32_t)(filesystem_size - sizeof(data)));
    CHECK(last_write_size == sizeof(data));

    mocked_erase_result = SFUD_ERR_TIMEOUT;
    CHECK(mounted_config->erase(mounted_config, 0) == LFS_ERR_IO);
    CHECK(erase_calls == 1);

    mocked_erase_result = SFUD_SUCCESS;
    CHECK(mounted_config->erase(mounted_config, cfg.block_count - 1) ==
          LFS_ERR_OK);
    CHECK(erase_calls == 2);
    CHECK(last_erase_address == (uint32_t)(filesystem_size - cfg.block_size));
    CHECK(last_erase_size == cfg.block_size);

    CHECK(mounted_config->sync(mounted_config) == LFS_ERR_OK);

    calls_before = read_calls;
    CHECK(mounted_config->read(mounted_config, cfg.block_count, 0, data, 1) ==
          LFS_ERR_INVAL);
    CHECK(read_calls == calls_before);
    CHECK(mounted_config->read(mounted_config, 0, cfg.block_size + 1,
                               data, 1) == LFS_ERR_INVAL);
    CHECK(read_calls == calls_before);
    CHECK(mounted_config->read(mounted_config, 0, cfg.block_size - 1,
                               data, 2) == LFS_ERR_INVAL);
    CHECK(read_calls == calls_before);
    CHECK(mounted_config->read(mounted_config, 0, 0, NULL, 1) ==
          LFS_ERR_INVAL);
    CHECK(read_calls == calls_before);
    CHECK(mounted_config->read(NULL, 0, 0, data, 1) == LFS_ERR_INVAL);
    CHECK(read_calls == calls_before);

    calls_before = write_calls;
    CHECK(mounted_config->prog(mounted_config, cfg.block_count, 0, data, 1) ==
          LFS_ERR_INVAL);
    CHECK(write_calls == calls_before);
    CHECK(mounted_config->prog(mounted_config, 0, cfg.block_size - 1, data, 2) ==
          LFS_ERR_INVAL);
    CHECK(write_calls == calls_before);

    calls_before = erase_calls;
    CHECK(mounted_config->erase(mounted_config, cfg.block_count) ==
          LFS_ERR_INVAL);
    CHECK(erase_calls == calls_before);

    mocked_flash.init_ok = false;
    calls_before = read_calls;
    CHECK(mounted_config->read(mounted_config, 0, 0, data, 1) == LFS_ERR_IO);
    CHECK(read_calls == calls_before);
}

int main(void)
{
    test_initialization_gates();
    test_jedec_metadata();
    test_format_is_denied();
    test_block_operations();

    if (failures != 0)
    {
        fprintf(stderr, "FAIL: %u of %u checks\n", failures, checks);
        return 1;
    }

    printf("PASS: %u checks (SUDO LittleFS SFUD gates and I/O errors)\n",
           checks);
    return 0;
}
