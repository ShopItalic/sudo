#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "bc_spi_flash_port.h"
#include "lfs.h"
#include "lfs_port.h"
#include <sfud.h>
#include "q_device.h"
#include "nrf_spim.h"
#include "bc_sem.h"

extern const struct lfs_config cfg;
sfud_err sfud_spi_port_init(sfud_flash *flash);

static q_device_t *registered_device;
static nrfx_spim_evt_handler_t spi_handler;
static void *spi_context;
static nrfx_err_t mock_init_result;
static nrfx_err_t mock_xfer_script[8];
static size_t mock_xfer_script_length;
static size_t mock_xfer_script_index;
static bool mock_complete_success;
static bool mock_transfer_active;
static bool mock_irq_enabled;
static bool mock_end_event_pending;
static bool callback_on_next_delay;
static unsigned init_calls;
static unsigned uninit_calls;
static unsigned xfer_calls;
static unsigned abort_calls;
static unsigned irq_disable_calls;
static unsigned end_event_clear_calls;
static unsigned late_callback_attempts;
static unsigned late_callback_deliveries;
static unsigned delay_us_calls;
static unsigned delay_ms_calls;
static unsigned gpio_set_calls;
static unsigned gpio_clear_calls;
static unsigned gpio_default_calls;
static unsigned q_open_calls;
static unsigned q_close_calls;
static unsigned q_write_calls;
static unsigned q_control_calls;
static unsigned power_on_calls;
static unsigned power_off_calls;
static unsigned sem_take_calls;
static unsigned sem_give_calls;
static sfud_flash mocked_flash;
static sfud_err mocked_sfud_init_result;
static sfud_err mocked_sfud_read_result;
static sfud_err mocked_sfud_write_result;
static sfud_err mocked_sfud_erase_result;
static unsigned sfud_init_calls;
static unsigned sfud_read_calls;
static unsigned sfud_write_calls;
static unsigned sfud_erase_calls;
static unsigned mount_calls;
static unsigned format_calls;
static int mocked_mount_result;
static int mocked_format_result;
static const struct lfs_config *mounted_config;

static unsigned checks;
static unsigned failures;

#define CHECK(condition) do { \
    ++checks; \
    if(!(condition)) { \
        ++failures; \
        fprintf(stderr, "FAIL line %u: %s\n", __LINE__, #condition); \
    } \
} while(0)

int q_device_register(q_device_t *dev)
{
    registered_device = dev;
    return RESULT_OK;
}

q_device_t *q_device_find(const char *name)
{
    if(name != NULL && strcmp(name, "spi_2") == 0)
        return registered_device;
    return NULL;
}

int q_device_open(q_device_t *dev)
{
    if(dev == NULL || dev->dops == NULL || dev->dops->open == NULL)
        return RESULT_DEV_NULL_ERR;
    ++q_open_calls;
    return dev->dops->open(dev);
}

int q_device_close(q_device_t *dev)
{
    if(dev == NULL || dev->dops == NULL || dev->dops->close == NULL)
        return RESULT_DEV_NULL_ERR;
    ++q_close_calls;
    return dev->dops->close(dev);
}

int q_device_write(q_device_t *dev, int pos, const void *buffer, int size)
{
    if(dev == NULL || dev->dops == NULL || dev->dops->write == NULL)
        return RESULT_DEV_NULL_ERR;
    ++q_write_calls;
    return dev->dops->write(dev, pos, buffer, size);
}

int q_device_read(q_device_t *dev, int pos, const void *buffer, int size)
{
    if(dev == NULL || dev->dops == NULL || dev->dops->read == NULL)
        return RESULT_DEV_NULL_ERR;
    return dev->dops->read(dev, pos, buffer, size);
}

int q_device_ctrl(q_device_t *dev, int cmd, void *args)
{
    if(dev == NULL || dev->dops == NULL || dev->dops->control == NULL)
        return RESULT_DEV_NULL_ERR;
    ++q_control_calls;
    return dev->dops->control(dev, cmd, args);
}

int q_device_cfg(q_device_t *dev, void *args, void *var)
{
    if(dev == NULL || dev->dops == NULL || dev->dops->config == NULL)
        return RESULT_DEV_NULL_ERR;
    return dev->dops->config(dev, args, var);
}

int q_device_reg_callback(q_device_t *dev, int pos, void *callback)
{
    if(dev == NULL || dev->dops == NULL || dev->dops->register_callback == NULL)
        return RESULT_DEV_NULL_ERR;
    return dev->dops->register_callback(dev, pos, callback);
}

void nrf_gpio_cfg_output(uint32_t pin)
{
    (void)pin;
}

void nrf_gpio_cfg_default(uint32_t pin)
{
    (void)pin;
    ++gpio_default_calls;
}

void nrf_gpio_pin_set(uint32_t pin)
{
    (void)pin;
    ++gpio_set_calls;
}

void nrf_gpio_pin_clear(uint32_t pin)
{
    (void)pin;
    ++gpio_clear_calls;
}

void test_nrf_delay_us(uint32_t microseconds)
{
    (void)microseconds;
    ++delay_us_calls;
    if(callback_on_next_delay && mock_irq_enabled && !mock_end_event_pending &&
       spi_handler != NULL)
    {
        nrfx_spim_evt_t event = {0};
        callback_on_next_delay = false;
        spi_handler(&event, spi_context);
    }
}

void test_nrf_delay_ms(uint32_t milliseconds)
{
    (void)milliseconds;
}

void bc_delay_us(uint32_t microseconds)
{
    test_nrf_delay_us(microseconds);
}

void bc_delay_ms(uint32_t milliseconds)
{
    (void)milliseconds;
    ++delay_ms_calls;
}

void bc_ldo_flash_power_on(void)
{
    ++power_on_calls;
}

void bc_ldo_flash_power_off(void)
{
    ++power_off_calls;
}

bool bc_rtos_sem_take(bc_rtos_sem_event_type sem_event)
{
    (void)sem_event;
    ++sem_take_calls;
    return true;
}

bool bc_rtos_sem_give(bc_rtos_sem_event_type sem_event)
{
    (void)sem_event;
    ++sem_give_calls;
    return true;
}

nrfx_err_t nrfx_spim_init(nrfx_spim_t const *instance,
                          nrfx_spim_config_t const *config,
                          nrfx_spim_evt_handler_t handler,
                          void *context)
{
    (void)instance;
    (void)config;
    ++init_calls;
    if(mock_init_result == NRFX_SUCCESS)
    {
        spi_handler = handler;
        spi_context = context;
        mock_irq_enabled = true;
    }
    return mock_init_result;
}

void nrfx_spim_uninit(nrfx_spim_t const *instance)
{
    (void)instance;
    ++uninit_calls;
    mock_irq_enabled = false;
    ++irq_disable_calls;
    mock_transfer_active = false;
}

nrfx_err_t nrfx_spim_xfer(nrfx_spim_t const *instance,
                          nrfx_spim_xfer_desc_t const *desc,
                          uint32_t flags)
{
    nrfx_err_t result;
    (void)instance;
    (void)flags;
    ++xfer_calls;
    if(mock_xfer_script_index < mock_xfer_script_length)
        result = mock_xfer_script[mock_xfer_script_index++];
    else
        result = NRFX_SUCCESS;
    if(result == NRFX_SUCCESS)
    {
        /* nrfx_spim's transfer setup clears the END event before START. */
        if(mock_end_event_pending)
        {
            mock_end_event_pending = false;
            ++end_event_clear_calls;
        }
        mock_transfer_active = true;
        if(desc->p_rx_buffer != NULL)
            memset(desc->p_rx_buffer, 0xa5, desc->rx_length);
        if(mock_complete_success && spi_handler != NULL)
        {
            nrfx_spim_evt_t event = {0};
            mock_transfer_active = false;
            spi_handler(&event, spi_context);
        }
    }
    return result;
}

void nrfx_spim_abort(nrfx_spim_t const *instance)
{
    (void)instance;
    ++abort_calls;
    /* The SDK abort does not clear END or disable its IRQ. */
    mock_end_event_pending = true;
    mock_transfer_active = false;
}

static void invoke_late_callback(void)
{
    nrfx_spim_evt_t event = {0};
    ++late_callback_attempts;
    CHECK(spi_handler != NULL);
    if(spi_handler != NULL && mock_irq_enabled && !mock_end_event_pending)
    {
        ++late_callback_deliveries;
        spi_handler(&event, spi_context);
    }
}

sfud_err sfud_init(void)
{
    ++sfud_init_calls;
    if(mocked_sfud_init_result != SFUD_SUCCESS)
        return mocked_sfud_init_result;
    memset(&mocked_flash, 0, sizeof(mocked_flash));
    mocked_flash.index = SFUD_GD25Q32E_DEVICE_INDEX;
    mocked_flash.init_ok = true;
    mocked_flash.chip.capacity = (uint32_t)cfg.block_size * cfg.block_count;
    mocked_flash.chip.mf_id = 0xc8;
    mocked_flash.chip.type_id = 0x40;
    mocked_flash.chip.capacity_id = 0x18;
    return sfud_spi_port_init(&mocked_flash);
}

const sfud_flash *sfud_get_device_table(void)
{
    return &mocked_flash;
}

sfud_err sfud_read(const sfud_flash *flash, uint32_t address, size_t size,
                   uint8_t *data)
{
    uint8_t command = 0x03;
    sfud_err result;
    (void)address;
    ++sfud_read_calls;
    if(mocked_sfud_read_result != SFUD_SUCCESS)
        return mocked_sfud_read_result;
    result = flash->spi.wr(&flash->spi, &command, 1, data, size);
    return result;
}

sfud_err sfud_write(const sfud_flash *flash, uint32_t address, size_t size,
                    const uint8_t *data)
{
    sfud_err result;
    ++sfud_write_calls;
    (void)address;
    if(data == NULL && size != 0)
        return SFUD_ERR_WRITE;
    if(mocked_sfud_write_result != SFUD_SUCCESS)
        return mocked_sfud_write_result;
    result = flash->spi.wr(&flash->spi, (uint8_t *)data, size, NULL, 0);
    return result;
}

sfud_err sfud_erase(const sfud_flash *flash, uint32_t address, size_t size)
{
    uint8_t command = 0x20;
    ++sfud_erase_calls;
    (void)address;
    (void)size;
    if(mocked_sfud_erase_result != SFUD_SUCCESS)
        return mocked_sfud_erase_result;
    return flash->spi.wr(&flash->spi, &command, 1, NULL, 0);
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

static void set_xfer_script(const nrfx_err_t *script, size_t length,
                            bool complete_success)
{
    size_t i;
    mock_xfer_script_length = length;
    mock_xfer_script_index = 0;
    mock_complete_success = complete_success;
    for(i = 0; i < length && i < sizeof(mock_xfer_script) / sizeof(mock_xfer_script[0]); ++i)
        mock_xfer_script[i] = script[i];
}

static void reset_observations(void)
{
    mock_init_result = NRFX_SUCCESS;
    mock_xfer_script_length = 0;
    mock_xfer_script_index = 0;
    mock_complete_success = true;
    mock_transfer_active = false;
    mock_end_event_pending = false;
    callback_on_next_delay = false;
    init_calls = 0;
    uninit_calls = 0;
    xfer_calls = 0;
    abort_calls = 0;
    irq_disable_calls = 0;
    end_event_clear_calls = 0;
    late_callback_attempts = 0;
    late_callback_deliveries = 0;
    delay_us_calls = 0;
    delay_ms_calls = 0;
    gpio_set_calls = 0;
    gpio_clear_calls = 0;
    gpio_default_calls = 0;
    q_open_calls = 0;
    q_close_calls = 0;
    q_write_calls = 0;
    q_control_calls = 0;
    power_on_calls = 0;
    power_off_calls = 0;
    sem_take_calls = 0;
    sem_give_calls = 0;
    mocked_sfud_init_result = SFUD_SUCCESS;
    mocked_sfud_read_result = SFUD_SUCCESS;
    mocked_sfud_write_result = SFUD_SUCCESS;
    mocked_sfud_erase_result = SFUD_SUCCESS;
    sfud_init_calls = 0;
    sfud_read_calls = 0;
    sfud_write_calls = 0;
    sfud_erase_calls = 0;
    mount_calls = 0;
    format_calls = 0;
    mocked_mount_result = LFS_ERR_OK;
    mocked_format_result = LFS_ERR_OK;
    mounted_config = NULL;
}

static bool open_port(void)
{
    return bc_spi_flash_device_open_checked();
}

static void close_port(void)
{
    bc_spi_flash_device_close();
}

static void test_open_checked(void)
{
    q_device_t *saved_device = registered_device;
    const nrfx_err_t wake_error[] = {NRFX_ERROR_INVALID_PARAM};

    reset_observations();
    registered_device = NULL;
    bc_spi_flash_device_find();
    CHECK(!open_port());
    CHECK(power_on_calls == 0 && power_off_calls == 0 && q_open_calls == 0);

    registered_device = saved_device;
    bc_spi_flash_device_find();
    CHECK(open_port());
    CHECK(power_on_calls == 1 && power_off_calls == 0);
    CHECK(q_open_calls == 1 && init_calls == 1 && q_write_calls == 1);
    CHECK(q_control_calls == 2 && delay_ms_calls == 4);
    CHECK(open_port());
    CHECK(power_on_calls == 1 && q_open_calls == 1 && q_write_calls == 1);
    close_port();
    CHECK(q_close_calls == 1 && uninit_calls == 1 && q_write_calls == 2);
    CHECK(power_off_calls == 1);
    close_port();
    CHECK(power_off_calls == 1 && q_close_calls == 1);

    reset_observations();
    mock_init_result = 77;
    CHECK(!open_port());
    CHECK(power_on_calls == 1 && power_off_calls == 1);
    CHECK(q_open_calls == 1 && init_calls == 1 && q_write_calls == 0);
    CHECK(!bc_spi_flash_write_and_read(NULL, 0, NULL, 0));
    CHECK(q_write_calls == 0);

    reset_observations();
    set_xfer_script(wake_error, 1, true);
    CHECK(!open_port());
    CHECK(power_on_calls == 1 && power_off_calls == 1);
    CHECK(q_open_calls == 1 && q_write_calls == 1 && q_close_calls == 1);
    CHECK(q_control_calls == 2);
}

static void test_transfer_faults(void)
{
    uint8_t command = 0x9f;
    uint8_t data[4] = {0};
    const nrfx_err_t submission_error[] = {NRFX_ERROR_INVALID_PARAM};
    const nrfx_err_t busy_then_success[] = {
        NRFX_ERROR_BUSY, NRFX_ERROR_BUSY, NRFX_SUCCESS,
    };
    const nrfx_err_t timeout_script[] = {NRFX_SUCCESS};

    reset_observations();
    CHECK(open_port());
    reset_observations();
    set_xfer_script(submission_error, 1, true);
    CHECK(!bc_spi_flash_write_and_read(&command, 1, data, 0));
    CHECK(xfer_calls == 1 && abort_calls == 0);

    set_xfer_script(busy_then_success, 3, true);
    CHECK(bc_spi_flash_write_and_read(&command, 1, data, 0));
    CHECK(xfer_calls == 4 && abort_calls == 0);

    set_xfer_script(timeout_script, 1, false);
    CHECK(!bc_spi_flash_write_and_read(&command, 1, data, 0));
    CHECK(xfer_calls == 5 && abort_calls == 1);
    CHECK(!mock_transfer_active);
    CHECK(uninit_calls == 1 && init_calls == 1 && irq_disable_calls == 1);
    CHECK(mock_end_event_pending);
    invoke_late_callback();
    CHECK(late_callback_attempts == 1 && late_callback_deliveries == 0);
    set_xfer_script(timeout_script, 1, true);
    CHECK(bc_spi_flash_write_and_read(&command, 1, data, 0));
    CHECK(xfer_calls == 6 && abort_calls == 1);
    CHECK(end_event_clear_calls == 1);
    close_port();
}

static void test_sfud_transaction_errors(void)
{
    uint8_t command = 0x03;
    uint8_t data[2] = {0};
    const nrfx_err_t write_error[] = {NRFX_ERROR_INVALID_PARAM};
    const nrfx_err_t success_twice[] = {NRFX_SUCCESS, NRFX_SUCCESS};
    const nrfx_err_t read_error[] = {NRFX_SUCCESS, NRFX_ERROR_INVALID_PARAM};
    unsigned set_before;
    unsigned clear_before;

    reset_observations();
    CHECK(open_port());
    reset_observations();
    CHECK(sfud_init() == SFUD_SUCCESS);

    set_before = gpio_set_calls;
    clear_before = gpio_clear_calls;
    set_xfer_script(write_error, 1, true);
    CHECK(mocked_flash.spi.wr(&mocked_flash.spi, &command, 1, data, sizeof(data)) ==
          SFUD_ERR_WRITE);
    CHECK(xfer_calls == 1);
    CHECK(gpio_clear_calls == clear_before + 1 && gpio_set_calls == set_before + 1);

    set_xfer_script(success_twice, 2, true);
    CHECK(mocked_flash.spi.wr(&mocked_flash.spi, &command, 1, data, sizeof(data)) ==
          SFUD_SUCCESS);
    CHECK(xfer_calls == 3 && data[0] == 0xa5 && data[1] == 0xa5);

    set_xfer_script(read_error, 2, true);
    CHECK(mocked_flash.spi.wr(&mocked_flash.spi, &command, 1, data, sizeof(data)) ==
          SFUD_ERR_READ);
    CHECK(xfer_calls == 5);
    close_port();

    set_xfer_script(success_twice, 2, true);
    CHECK(mocked_flash.spi.wr(&mocked_flash.spi, &command, 1, data, sizeof(data)) ==
          SFUD_ERR_WRITE);
    CHECK(xfer_calls == 6);
}

static void test_littlefs_error_propagation(void)
{
    lfs_t filesystem = {0};
    uint8_t data[2] = {0};
    const nrfx_err_t write_error[] = {NRFX_ERROR_INVALID_PARAM};
    const nrfx_err_t read_success[] = {NRFX_SUCCESS, NRFX_SUCCESS};

    reset_observations();
    CHECK(open_port());
    reset_observations();
    CHECK(lfs_sfud_init(&filesystem) == LFS_ERR_OK);
    CHECK(mount_calls == 1 && format_calls == 0 && mounted_config == &cfg);

    set_xfer_script(write_error, 1, true);
    CHECK(cfg.read(&cfg, 0, 0, data, sizeof(data)) == LFS_ERR_IO);
    CHECK(sfud_read_calls == 1 && xfer_calls == 1);

    set_xfer_script(read_success, 2, true);
    CHECK(cfg.read(&cfg, 0, 0, data, sizeof(data)) == LFS_ERR_OK);
    CHECK(xfer_calls == 3 && data[0] == 0xa5 && data[1] == 0xa5);

    close_port();
    CHECK(cfg.read(&cfg, 0, 0, data, sizeof(data)) == LFS_ERR_IO);
    CHECK(xfer_calls == 4);
}

int main(void)
{
    CHECK(registered_device != NULL);
    reset_observations();
    test_open_checked();
    test_transfer_faults();
    test_sfud_transaction_errors();
    test_littlefs_error_propagation();

    if(failures != 0)
    {
        fprintf(stderr, "FAIL: %u of %u checks\n", failures, checks);
        return 1;
    }
    printf("PASS: %u checks (SUDO SPI2 SFUD LittleFS fault path)\n", checks);
    return 0;
}
