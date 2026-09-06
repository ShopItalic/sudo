#ifndef BC_TOUCH_TUNING_H
#define BC_TOUCH_TUNING_H

#include <stdbool.h>
#include <stdint.h>
#include "bc_voice_inputs.h"

/* The SUDO IQS7211E profile uses only hold, double-tap and triple-tap gesture bits.  This
 * value is also the fallback used if a configuration request arrives before
 * the device adapter has performed its one-time initialization. */
#ifndef BC_TOUCH_TUNING_DEFAULT_GESTURE_MASK
#if defined(SUDO_VOICE_ONLY)
#define BC_TOUCH_TUNING_DEFAULT_GESTURE_MASK 0x0008U
#else
#define BC_TOUCH_TUNING_DEFAULT_GESTURE_MASK 0x0F0FU
#endif
#endif

#define BC_TOUCH_TUNING_DEFAULT_SET 54U
#define BC_TOUCH_TUNING_DEFAULT_CLEAR 52U
#define BC_TOUCH_TUNING_MIN_SET 32U
#define BC_TOUCH_TUNING_MAX_SET 80U
#define BC_TOUCH_TUNING_MIN_CLEAR 30U
#define BC_TOUCH_TUNING_MIN_HYSTERESIS 2U

#define BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG 0x38U
#define BC_TOUCH_TUNING_GESTURE_ENABLE_REG 0x4BU
#define BC_TOUCH_TUNING_HOLD_TIME_REG 0x4FU
#define BC_TOUCH_TUNING_GESTURE_DOUBLE 0x0002U
#define BC_TOUCH_TUNING_GESTURE_TRIPLE 0x0004U
#define BC_TOUCH_TUNING_GESTURE_HOLD 0x0008U

typedef enum {
    BC_TOUCH_TUNING_PENDING = 0,
    BC_TOUCH_TUNING_APPLIED = 1,
    BC_TOUCH_TUNING_IO_ERROR = 2
} bc_touch_tuning_status;

typedef struct {
    uint8_t touch_set;
    uint8_t touch_clear;
    bool memo_enabled; /* Compatibility mirror of the triple-tap enable bit. */
    uint16_t gesture_mask, hold_ms;
    bc_touch_tuning_status status;
    uint32_t generation;
} bc_touch_tuning_snapshot;

/* These callbacks are called only by bc_touch_tuning_on_sample after the IQS
 * adapter has opened its existing event communication window.  They must not
 * call back into the tuning module or wait for another worker. */
typedef bool (*bc_touch_tuning_write_fn)(void *ctx, uint8_t reg,
                                         uint8_t *data, uint8_t length);
typedef bool (*bc_touch_tuning_read_fn)(void *ctx, uint8_t reg,
                                        uint8_t *data, uint8_t length);

/* Initializes the desired values from the adapter's compile-time gesture
 * mask.  Repeated calls preserve an already-posted request. */
void bc_touch_tuning_init(uint16_t initial_gesture_mask);

/* Posts a desired configuration without doing I2C. */
bool bc_touch_tuning_request(uint8_t touch_set, uint8_t touch_clear,
                             bool memo_enabled);

bool bc_touch_tuning_request_inputs(uint8_t touch_set, uint8_t touch_clear,
                                    const bc_voice_inputs *inputs);

/* Copies the desired configuration and current state under a FreeRTOS
 * critical section. */
bool bc_touch_tuning_snapshot_get(bc_touch_tuning_snapshot *snapshot);
bc_touch_tuning_status bc_touch_tuning_status_get(void);

/* The IQS adapter calls this when it observes a sensor reset.  The desired
 * values are retained, while the hardware is considered unapplied. */
void bc_touch_tuning_sensor_reset(void);

/* Called from the IQS event window after a status report has been decoded.
 * Only a valid, explicit no-contact sample may perform I2C.  A reset sample
 * only invalidates the hardware state and never performs I2C. */
void bc_touch_tuning_on_sample(bool valid, bool contact, bool reset,
                               bc_touch_tuning_write_fn write,
                               bc_touch_tuning_read_fn read, void *ctx);

#endif
