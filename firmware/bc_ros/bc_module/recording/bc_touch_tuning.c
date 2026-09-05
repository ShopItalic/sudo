#include "bc_touch_tuning.h"

#include "FreeRTOS.h"
#include "task.h"

typedef struct {
    bool initialized;
    uint16_t initial_gesture_mask;
    uint8_t touch_set;
    uint8_t touch_clear;
    bool memo_enabled;
    uint32_t generation;
    uint32_t applied_generation;
    bool hardware_applied;
    bc_touch_tuning_status status;
} bc_touch_tuning_state;

static bc_touch_tuning_state tuning;

static void generation_advance(void)
{
    ++tuning.generation;
    /* Generation zero is reserved for an uninitialized snapshot. */
    if (tuning.generation == 0U)
        tuning.generation = 1U;
}

static void ensure_initialized(uint16_t initial_gesture_mask)
{
    if (tuning.initialized)
        return;

    tuning.initialized = true;
    tuning.initial_gesture_mask = initial_gesture_mask;
    tuning.touch_set = BC_TOUCH_TUNING_DEFAULT_SET;
    tuning.touch_clear = BC_TOUCH_TUNING_DEFAULT_CLEAR;
    tuning.memo_enabled = true;
    tuning.generation = 1U;
    tuning.applied_generation = tuning.generation;
    tuning.hardware_applied = true;
    tuning.status = BC_TOUCH_TUNING_APPLIED;
}

static void publish_io_error(uint32_t generation)
{
    taskENTER_CRITICAL();
    if (tuning.generation == generation) {
        tuning.status = BC_TOUCH_TUNING_IO_ERROR;
        tuning.hardware_applied = false;
        tuning.applied_generation = 0U;
    }
    taskEXIT_CRITICAL();
}

void bc_touch_tuning_init(uint16_t initial_gesture_mask)
{
    taskENTER_CRITICAL();
    ensure_initialized(initial_gesture_mask);
    taskEXIT_CRITICAL();
}

bool bc_touch_tuning_request(uint8_t touch_set, uint8_t touch_clear,
                             bool memo_enabled)
{
    if (touch_set < BC_TOUCH_TUNING_MIN_SET ||
        touch_set > BC_TOUCH_TUNING_MAX_SET ||
        touch_clear < BC_TOUCH_TUNING_MIN_CLEAR ||
        touch_clear >= touch_set ||
        (uint8_t)(touch_set - touch_clear) < BC_TOUCH_TUNING_MIN_HYSTERESIS)
        return false;

    taskENTER_CRITICAL();
    ensure_initialized(BC_TOUCH_TUNING_DEFAULT_GESTURE_MASK);
    tuning.touch_set = touch_set;
    tuning.touch_clear = touch_clear;
    tuning.memo_enabled = memo_enabled;
    generation_advance();
    tuning.status = BC_TOUCH_TUNING_PENDING;
    tuning.hardware_applied = false;
    tuning.applied_generation = 0U;
    taskEXIT_CRITICAL();
    return true;
}

bool bc_touch_tuning_snapshot_get(bc_touch_tuning_snapshot *snapshot)
{
    if (snapshot == 0)
        return false;

    taskENTER_CRITICAL();
    ensure_initialized(BC_TOUCH_TUNING_DEFAULT_GESTURE_MASK);
    snapshot->touch_set = tuning.touch_set;
    snapshot->touch_clear = tuning.touch_clear;
    snapshot->memo_enabled = tuning.memo_enabled;
    snapshot->status = tuning.status;
    snapshot->generation = tuning.generation;
    taskEXIT_CRITICAL();
    return true;
}

bc_touch_tuning_status bc_touch_tuning_status_get(void)
{
    bc_touch_tuning_status status;

    taskENTER_CRITICAL();
    ensure_initialized(BC_TOUCH_TUNING_DEFAULT_GESTURE_MASK);
    status = tuning.status;
    taskEXIT_CRITICAL();
    return status;
}

void bc_touch_tuning_sensor_reset(void)
{
    taskENTER_CRITICAL();
    ensure_initialized(BC_TOUCH_TUNING_DEFAULT_GESTURE_MASK);
    generation_advance();
    tuning.status = BC_TOUCH_TUNING_PENDING;
    tuning.hardware_applied = false;
    tuning.applied_generation = 0U;
    taskEXIT_CRITICAL();
}

void bc_touch_tuning_on_sample(bool valid, bool contact, bool reset,
                               bc_touch_tuning_write_fn write,
                               bc_touch_tuning_read_fn read, void *ctx)
{
    uint8_t touch_thresholds[2];
    uint8_t gesture_mask[2];
    uint8_t readback[2];
    uint8_t touch_set;
    uint8_t touch_clear;
    bool memo_enabled;
    uint16_t initial_gesture_mask;
    uint32_t generation;
    uint16_t mask;
    bool needs_apply;

    if (reset) {
        bc_touch_tuning_sensor_reset();
        return;
    }

    /* A callback is supplied only by the device adapter while its I2C
     * communication window is open.  A bad or touching report is never
     * allowed to consume that callback. */
    if (!valid || contact || write == 0 || read == 0)
        return;

    taskENTER_CRITICAL();
    ensure_initialized(BC_TOUCH_TUNING_DEFAULT_GESTURE_MASK);
    needs_apply = !(tuning.status == BC_TOUCH_TUNING_APPLIED &&
                    tuning.hardware_applied &&
                    tuning.applied_generation == tuning.generation);
    if (needs_apply) {
        touch_set = tuning.touch_set;
        touch_clear = tuning.touch_clear;
        memo_enabled = tuning.memo_enabled;
        initial_gesture_mask = tuning.initial_gesture_mask;
        generation = tuning.generation;
    }
    taskEXIT_CRITICAL();

    if (!needs_apply)
        return;

    touch_thresholds[0] = touch_set;
    touch_thresholds[1] = touch_clear;
    if (!write(ctx, BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG,
               touch_thresholds, sizeof(touch_thresholds))) {
        publish_io_error(generation);
        return;
    }

    if (!read(ctx, BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG, readback,
              sizeof(readback)) || readback[0] != touch_set ||
        readback[1] != touch_clear) {
        publish_io_error(generation);
        return;
    }

    /* The SUDO profile deliberately enables only hold and optional double
     * tap.  A non-SUDO caller can retain its other compile-time gesture bits;
     * the IQS adapter does not route this service in that profile. */
#if defined(SUDO_VOICE_ONLY)
    (void)initial_gesture_mask;
    mask = BC_TOUCH_TUNING_GESTURE_HOLD;
#else
    mask = (uint16_t)(initial_gesture_mask &
                      (uint16_t)~(BC_TOUCH_TUNING_GESTURE_HOLD |
                                  BC_TOUCH_TUNING_GESTURE_DOUBLE));
    mask = (uint16_t)(mask | BC_TOUCH_TUNING_GESTURE_HOLD);
#endif
    if (memo_enabled)
        mask = (uint16_t)(mask | BC_TOUCH_TUNING_GESTURE_DOUBLE);
    gesture_mask[0] = (uint8_t)(mask & 0xFFU);
    gesture_mask[1] = (uint8_t)(mask >> 8);

    if (!write(ctx, BC_TOUCH_TUNING_GESTURE_ENABLE_REG,
               gesture_mask, sizeof(gesture_mask))) {
        publish_io_error(generation);
        return;
    }

    if (!read(ctx, BC_TOUCH_TUNING_GESTURE_ENABLE_REG, readback,
              sizeof(readback)) || readback[0] != gesture_mask[0] ||
        readback[1] != gesture_mask[1]) {
        publish_io_error(generation);
        return;
    }

    taskENTER_CRITICAL();
    if (tuning.generation == generation) {
        tuning.status = BC_TOUCH_TUNING_APPLIED;
        tuning.hardware_applied = true;
        tuning.applied_generation = generation;
    } else {
        /* A request or reset arrived while I2C was in progress.  The old
         * writes may have reached the sensor, but they do not acknowledge the
         * newer desired generation. */
        tuning.status = BC_TOUCH_TUNING_PENDING;
        tuning.hardware_applied = false;
        tuning.applied_generation = 0U;
    }
    taskEXIT_CRITICAL();
}
