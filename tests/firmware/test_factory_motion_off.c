/* Actual ST register driver + production shutdown; only board IO is modeled. */
#include "app_factory_motion_off.h"
#include "ring_config.h"
#include "lsm6dso_reg.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t registers[3][256];
static uint8_t bank;
static unsigned calls, writes, fail_at, opens, closes, checks;
static bool opened, dropped_write;

void bc_g_sensor_i2c_open(void) { assert(!opened); opened = true; ++opens; }
void bc_g_sensor_i2c_close(void) { assert(opened); opened = false; ++closes; }
bool bc_g_sensor_i2c_read(uint8_t address, uint8_t reg, uint8_t *data, uint8_t length)
{
    assert(opened && address == LSM6DSO_I2C_ADD_L && length && reg + length <= 256);
    if (++calls == fail_at) return false;
    for (unsigned i = 0; i < length; ++i)
        data[i] = registers[(reg + i == LSM6DSO_FUNC_CFG_ACCESS) ? 0 : bank][reg + i];
    return true;
}
bool bc_g_sensor_i2c_write(uint8_t address, uint8_t reg, uint8_t *data, uint8_t length)
{
    assert(opened && address == LSM6DSO_I2C_ADD_L && length && reg + length <= 256);
    if (++calls == fail_at) return false;
    ++writes;
    for (unsigned i = 0; i < length; ++i) {
        if (bank == 0 && (reg + i == LSM6DSO_CTRL1_XL || reg + i == LSM6DSO_CTRL2_G))
            assert((data[i] & 0xf0U) == 0); /* Never activate either sensor. */
        if (dropped_write && bank == 0 && reg + i == LSM6DSO_CTRL1_XL) continue;
        registers[(reg + i == LSM6DSO_FUNC_CFG_ACCESS) ? 0 : bank][reg + i] = data[i];
        if (reg + i == LSM6DSO_FUNC_CFG_ACCESS)
            bank = data[i] & 0x80U ? 1 : (data[i] & 0x40U ? 2 : 0);
    }
    return true;
}
static void seed(unsigned value, unsigned selected_bank)
{
    for (unsigned b = 0; b < 3; ++b) for (unsigned r = 0; r < 256; ++r)
        registers[b][r] = (uint8_t)(value * 17U + r * 7U + b);
    registers[0][LSM6DSO_WHO_AM_I] = LSM6DSO_ID;
    registers[0][LSM6DSO_FUNC_CFG_ACCESS] = selected_bank == 1 ? 0x80 : (selected_bank == 2 ? 0x40 : 0);
    bank = selected_bank;
    calls = writes = fail_at = opens = closes = 0;
    opened = dropped_write = false;
    app_factory_motion_status = FACTORY_MOTION_NOT_CHECKED;
}
static void balanced(void) { assert(!opened && opens == closes); ++checks; }
int main(void)
{
    for (unsigned b = 0; b < 3; ++b) for (unsigned value = 0; value < 32; ++value) {
        seed(value, b); app_factory_motion_off(); balanced();
        assert(app_factory_motion_status == FACTORY_MOTION_OFF && bank == 0);
        assert((registers[0][LSM6DSO_CTRL1_XL] & 0xf0) == 0);
        assert((registers[0][LSM6DSO_CTRL2_G] & 0xf0) == 0);
        unsigned successful_calls = calls;
        assert(app_factory_motion_id() == LSM6DSO_ID); balanced();
        for (unsigned fault = 1; fault <= successful_calls; ++fault) {
            seed(value, b); fail_at = fault; app_factory_motion_off(); balanced();
            assert(app_factory_motion_status != FACTORY_MOTION_OFF);
        }
    }
    seed(3, 0); registers[0][LSM6DSO_WHO_AM_I] = 0;
    app_factory_motion_off(); balanced();
    assert(app_factory_motion_status == FACTORY_MOTION_WRONG_DEVICE && writes == 1);
    seed(3, 0); dropped_write = true; registers[0][LSM6DSO_CTRL1_XL] = 0x40;
    app_factory_motion_off(); balanced();
    assert(app_factory_motion_status == FACTORY_MOTION_IO_FAILED);
    seed(3, 0); fail_at = 1;
    assert(app_factory_motion_id() == 0); balanced();
    printf("Motion-off real-driver tests: %u checked scenarios passed\n", checks);
    return 0;
}
