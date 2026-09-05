#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "q_device.h"
#include "nrf_spim.h"
#include "bc_sem.h"
#include "bc_spi_flash_port.h"
#include "bc_spi_flash.h"
#include <sfud.h>

static q_device_t *registered_device;
static nrfx_spim_evt_handler_t spi_handler;
static void *spi_context;
static nrfx_err_t mocked_init_result = NRFX_SUCCESS;
static bool mocked_complete = true;
static bool mocked_irq_enabled;
static bool mocked_write_enabled;
static unsigned init_calls;
static unsigned uninit_calls;
static unsigned xfer_calls;
static unsigned q_open_calls;
static unsigned q_close_calls;
static unsigned q_write_calls;
static unsigned q_control_calls;
static unsigned power_on_calls;
static unsigned power_off_calls;
static unsigned delay_ms_calls;
static unsigned gpio_low_calls;
static unsigned gpio_high_calls;
static uint8_t last_command;
static uint8_t max_tx_length;
static uint8_t max_rx_length;
static unsigned tx_255_calls;
static unsigned tx_5_calls;

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
    if(cmd == GPIO_OUTPUT_LOW)
        ++gpio_low_calls;
    else if(cmd == GPIO_OUTPUT_HIGH)
        ++gpio_high_calls;
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
}

void nrf_gpio_pin_set(uint32_t pin)
{
    (void)pin;
}

void nrf_gpio_pin_clear(uint32_t pin)
{
    (void)pin;
}

void test_nrf_delay_us(uint32_t microseconds)
{
    (void)microseconds;
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
    return true;
}

bool bc_rtos_sem_give(bc_rtos_sem_event_type sem_event)
{
    (void)sem_event;
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
    if(mocked_init_result == NRFX_SUCCESS)
    {
        spi_handler = handler;
        spi_context = context;
        mocked_irq_enabled = true;
    }
    return mocked_init_result;
}

void nrfx_spim_uninit(nrfx_spim_t const *instance)
{
    (void)instance;
    ++uninit_calls;
    mocked_irq_enabled = false;
}

nrfx_err_t nrfx_spim_xfer(nrfx_spim_t const *instance,
                          nrfx_spim_xfer_desc_t const *desc,
                          uint32_t flags)
{
    nrfx_spim_evt_t event = {0};
    size_t i;
    (void)instance;
    (void)flags;
    ++xfer_calls;
    if(desc->tx_length > max_tx_length)
        max_tx_length = (uint8_t)desc->tx_length;
    if(desc->rx_length > max_rx_length)
        max_rx_length = (uint8_t)desc->rx_length;
    if(desc->tx_length == UINT8_MAX)
        ++tx_255_calls;
    if(desc->tx_length == 5)
        ++tx_5_calls;
    if(desc->tx_length != 0 && desc->p_tx_buffer != NULL)
    {
        last_command = desc->p_tx_buffer[0];
        if(last_command == 0x06)
            mocked_write_enabled = true;
        else if(last_command == 0x04)
            mocked_write_enabled = false;
    }
    if(!mocked_complete)
        return NRFX_SUCCESS;
    if(desc->p_rx_buffer != NULL)
    {
        for(i = 0; i < desc->rx_length; ++i)
        {
            if(last_command == 0x9F)
            {
                static const uint8_t id[] = {0xC8, 0x65, 0x18};
                desc->p_rx_buffer[i] = id[i % sizeof(id)];
            }
            else if(last_command == 0x05)
            {
                desc->p_rx_buffer[i] = mocked_write_enabled ? 0x02 : 0x00;
            }
            else
            {
                desc->p_rx_buffer[i] = 0xA5;
            }
        }
    }
    if(mocked_irq_enabled && spi_handler != NULL)
        spi_handler(&event, spi_context);
    return NRFX_SUCCESS;
}

void nrfx_spim_abort(nrfx_spim_t const *instance)
{
    (void)instance;
}

static void reset_transfer_observations(void)
{
    xfer_calls = 0;
    q_write_calls = 0;
    q_control_calls = 0;
    gpio_low_calls = 0;
    gpio_high_calls = 0;
    delay_ms_calls = 0;
    max_tx_length = 0;
    max_rx_length = 0;
    tx_255_calls = 0;
    tx_5_calls = 0;
}

static void test_sfud_large_transfers(void)
{
    const sfud_flash *flash;
    uint8_t read_buffer[4096];
    uint8_t write_buffer[4096];
    unsigned read_xfers;
    unsigned write_xfers;
    size_t i;

    CHECK(sfud_init() == SFUD_SUCCESS);
    flash = sfud_get_device_table();
    CHECK(flash != NULL && flash->init_ok);
    CHECK(flash != NULL && flash->chip.mf_id == 0xC8);
    CHECK(flash != NULL && flash->chip.type_id == 0x65);
    CHECK(flash != NULL && flash->chip.capacity_id == 0x18);
    CHECK(flash != NULL && flash->chip.capacity == 16L * 1024L * 1024L);

    memset(read_buffer, 0, sizeof(read_buffer));
    reset_transfer_observations();
    CHECK(sfud_read(flash, 0, 512, read_buffer) == SFUD_SUCCESS);
    for(i = 0; i < 512; ++i)
        CHECK(read_buffer[i] == 0xA5);
    CHECK(max_tx_length <= UINT8_MAX && max_rx_length <= UINT8_MAX);
    CHECK(q_control_calls == 4 && gpio_low_calls == 2 && gpio_high_calls == 2);

    memset(read_buffer, 0, sizeof(read_buffer));
    reset_transfer_observations();
    CHECK(sfud_read(flash, 0, sizeof(read_buffer), read_buffer) == SFUD_SUCCESS);
    for(i = 0; i < sizeof(read_buffer); ++i)
        CHECK(read_buffer[i] == 0xA5);
    CHECK(max_tx_length <= UINT8_MAX && max_rx_length <= UINT8_MAX);
    CHECK(q_control_calls == 4 && gpio_low_calls == 2 && gpio_high_calls == 2);

    memset(write_buffer, 0x5A, sizeof(write_buffer));
    reset_transfer_observations();
    CHECK(sfud_write(flash, 0, sizeof(write_buffer), write_buffer) == SFUD_SUCCESS);
    write_xfers = xfer_calls;
    CHECK(write_xfers == 115);
    CHECK(max_tx_length <= UINT8_MAX && max_rx_length <= UINT8_MAX);
    CHECK(tx_255_calls == 16 && tx_5_calls == 16);
    CHECK(q_control_calls == 2 * (16 * 4 + 2));

    /* Verify a later read still starts after the long page-write sequence. */
    reset_transfer_observations();
    CHECK(sfud_read(flash, 0, 512, read_buffer) == SFUD_SUCCESS);
    read_xfers = xfer_calls;
    CHECK(read_xfers == 6);
    CHECK(read_xfers < write_xfers);

    bc_spi_flash_device_close();
    CHECK(power_off_calls == 1);
    CHECK(q_close_calls == 1);
    CHECK(uninit_calls == 1);
}

int main(void)
{
    CHECK(registered_device != NULL);
    bc_spi_flash_device_find();
    CHECK(bc_spi_flash_device_open_checked());
    CHECK(power_on_calls == 1 && power_off_calls == 0);
    reset_transfer_observations();
    test_sfud_large_transfers();

    if(failures != 0)
    {
        fprintf(stderr, "FAIL: %u of %u checks\n", failures, checks);
        return 1;
    }
    printf("PASS: %u checks (actual SFUD large transfer chunking)\n", checks);
    return 0;
}
