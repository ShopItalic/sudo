/* Application-only retirement for standard 603V1.23.2. No task/IRQ/timer. */
#include "app_factory_motion_off.h"
#include "ring_config.h"
#include "bc_g_sensor_device_port.h"
#include "lsm6dso_reg.h"
#include <string.h>
#if !defined(HANDWARE_1_23_2) || G_SENSOR_DEVIECE_TYPE != 4
#error "Motion retirement is qualified only for the standard 1.23.2 I2C IMU"
#endif

volatile unsigned app_factory_motion_status;

static int32_t motion_read(void *handle, uint8_t reg, uint8_t *data, uint16_t length)
{
    (void)handle;
    if (length > 255U) return -1;
    /* The supplier board port returns true on success. */
    if (bc_g_sensor_i2c_read(LSM6DSO_I2C_ADD_L, reg, data, (uint8_t)length)) return 0;
    /* Some pinned ST getters inspect their buffer even after an IO failure. */
    memset(data, 0, length);
    return -1;
}

static int32_t motion_write(void *handle, uint8_t reg, uint8_t *data, uint16_t length)
{
    (void)handle;
    if (length > 255U) return -1;
    return bc_g_sensor_i2c_write(LSM6DSO_I2C_ADD_L, reg, data, (uint8_t)length) ? 0 : -1;
}

static bool bytes_zero(const void *data, unsigned length)
{
    const uint8_t *p = data;
    while (length--) if (*p++) return false;
    return true;
}

static bool odrs_off(stmdev_ctx_t *ctx)
{
    lsm6dso_ctrl1_xl_t xl = {0};
    lsm6dso_ctrl2_g_t gy = {0};
    bool ok = true;
    /* The general ST rate setter can impose a nonzero FSM minimum even when
     * OFF is requested. Set only these documented ODR fields, never enabling
     * sampling even if a subsequent embedded-function shutdown fails. */
    if (lsm6dso_read_reg(ctx, LSM6DSO_CTRL1_XL, (uint8_t *)&xl, 1) == 0) {
        xl.odr_xl = LSM6DSO_XL_ODR_OFF;
        ok = lsm6dso_write_reg(ctx, LSM6DSO_CTRL1_XL, (uint8_t *)&xl, 1) == 0;
    } else ok = false;
    if (lsm6dso_read_reg(ctx, LSM6DSO_CTRL2_G, (uint8_t *)&gy, 1) == 0) {
        gy.odr_g = LSM6DSO_GY_ODR_OFF;
        ok = (lsm6dso_write_reg(ctx, LSM6DSO_CTRL2_G, (uint8_t *)&gy, 1) == 0) && ok;
    } else ok = false;
    return ok;
}

uint8_t app_factory_motion_id(void)
{
    uint8_t id = 0;
    bc_g_sensor_i2c_open();
    if (motion_read(NULL, LSM6DSO_WHO_AM_I, &id, 1U) != 0) id = 0;
    bc_g_sensor_i2c_close();
    return id;
}

void app_factory_motion_off(void)
{
    stmdev_ctx_t ctx = {0};
    lsm6dso_pin_int1_route_t int1 = {0};
    lsm6dso_pin_int2_route_t int2 = {0};
    lsm6dso_ctrl1_xl_t xl = {0};
    lsm6dso_ctrl2_g_t gy = {0};
    lsm6dso_emb_sens_t embedded = {0};
    lsm6dso_emb_fsm_enable_t fsm = {0};
    lsm6dso_bdr_xl_t batch_xl = LSM6DSO_XL_NOT_BATCHED;
    lsm6dso_bdr_gy_t batch_gy = LSM6DSO_GY_NOT_BATCHED;
    lsm6dso_fifo_mode_t fifo = LSM6DSO_BYPASS_MODE;
    uint8_t id = 0;
    bool ok = true;
    ctx.read_reg = motion_read; ctx.write_reg = motion_write;
    app_factory_motion_status = FACTORY_MOTION_IO_FAILED;
    /* Called in app_init, after device discovery and before the scheduler.
     * No sensor-enable operation, soft reset, retry loop or shared-bus retuning. */
    bc_g_sensor_i2c_open();
    if (lsm6dso_mem_bank_set(&ctx, LSM6DSO_USER_BANK) != 0) goto done;
    if (motion_read(NULL, LSM6DSO_WHO_AM_I, &id, 1U) != 0) goto done;
    if (id != LSM6DSO_ID) {
        app_factory_motion_status = FACTORY_MOTION_WRONG_DEVICE;
        goto done;
    }
    /* Explicit ODR writes also handle warm boots with a previously active IMU.
     * Continue independent shutdown writes after an error; never claim success. */
    ok = odrs_off(&ctx);
    ok = (lsm6dso_fifo_xl_batch_set(&ctx, LSM6DSO_XL_NOT_BATCHED) == 0) && ok;
    ok = (lsm6dso_fifo_gy_batch_set(&ctx, LSM6DSO_GY_NOT_BATCHED) == 0) && ok;
    ok = (lsm6dso_fifo_mode_set(&ctx, LSM6DSO_BYPASS_MODE) == 0) && ok;
    ok = (lsm6dso_embedded_sens_off(&ctx) == 0) && ok;
    if (lsm6dso_mem_bank_set(&ctx, LSM6DSO_USER_BANK) != 0) goto done;
    ok = (lsm6dso_fsm_enable_set(&ctx, &fsm) == 0) && ok;
    if (lsm6dso_mem_bank_set(&ctx, LSM6DSO_USER_BANK) != 0) goto done;
    ok = (lsm6dso_pin_int1_route_set(&ctx, int1) == 0) && ok;
    /* A failed banked operation can leave the device in a non-user bank. */
    if (lsm6dso_mem_bank_set(&ctx, LSM6DSO_USER_BANK) != 0) goto done;
    ok = (lsm6dso_pin_int2_route_set(&ctx, NULL, int2) == 0) && ok;
    if (lsm6dso_mem_bank_set(&ctx, LSM6DSO_USER_BANK) != 0) goto done;
    ok = (lsm6dso_read_reg(&ctx, LSM6DSO_CTRL1_XL, (uint8_t *)&xl, 1) == 0) && ok;
    ok = (lsm6dso_read_reg(&ctx, LSM6DSO_CTRL2_G, (uint8_t *)&gy, 1) == 0) && ok;
    ok = (lsm6dso_fifo_xl_batch_get(&ctx, &batch_xl) == 0) && ok;
    ok = (lsm6dso_fifo_gy_batch_get(&ctx, &batch_gy) == 0) && ok;
    ok = (lsm6dso_fifo_mode_get(&ctx, &fifo) == 0) && ok;
    memset(&int1, 0, sizeof(int1)); memset(&int2, 0, sizeof(int2));
    ok = (lsm6dso_pin_int1_route_get(&ctx, &int1) == 0) && ok;
    if (lsm6dso_mem_bank_set(&ctx, LSM6DSO_USER_BANK) != 0) goto done;
    ok = (lsm6dso_pin_int2_route_get(&ctx, NULL, &int2) == 0) && ok;
    if (lsm6dso_mem_bank_set(&ctx, LSM6DSO_USER_BANK) != 0) goto done;
    ok = (lsm6dso_embedded_sens_get(&ctx, &embedded) == 0) && ok;
    if (lsm6dso_mem_bank_set(&ctx, LSM6DSO_USER_BANK) != 0) goto done;
    ok = (lsm6dso_fsm_enable_get(&ctx, &fsm) == 0) && ok;
    ok = (lsm6dso_mem_bank_set(&ctx, LSM6DSO_USER_BANK) == 0) && ok;
    if (ok && xl.odr_xl == LSM6DSO_XL_ODR_OFF && gy.odr_g == LSM6DSO_GY_ODR_OFF &&
        batch_xl == LSM6DSO_XL_NOT_BATCHED && batch_gy == LSM6DSO_GY_NOT_BATCHED &&
        fifo == LSM6DSO_BYPASS_MODE && bytes_zero(&int1, sizeof(int1)) &&
        bytes_zero(&int2, sizeof(int2)) && bytes_zero(&embedded, sizeof(embedded)) &&
        bytes_zero(&fsm, sizeof(fsm))) app_factory_motion_status = FACTORY_MOTION_OFF;
done:
    bc_g_sensor_i2c_close();
}
