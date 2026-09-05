#ifndef BC_BLE_TX_H
#define BC_BLE_TX_H

#include <stdbool.h>
#include <stdint.h>

/* ATT/GATT writes in this adapter are limited to one 244-byte packet. */
#define BC_BLE_TX_MAX_LENGTH UINT16_C(244)

/*
 * A timeout is deliberately limited to INT32_MAX ticks.  This leaves the
 * unsigned tick subtraction in bc_ble_tx_write unambiguous across one wrap.
 */
#define BC_BLE_TX_MAX_TICKS UINT32_C(0x7fffffff)

typedef enum bc_ble_tx_attempt_result {
    BC_BLE_TX_ATTEMPT_ACCEPTED = 0,
    BC_BLE_TX_ATTEMPT_RETRY = 1,
    BC_BLE_TX_ATTEMPT_FATAL = 2
} bc_ble_tx_attempt_result;

typedef enum bc_ble_tx_result {
    /* Accepted by the local BLE stack; this does not acknowledge phone receipt. */
    BC_BLE_TX_ACCEPTED = 0,
    BC_BLE_TX_CANCELLED = 1,
    BC_BLE_TX_TIMEOUT = 2,
    BC_BLE_TX_INVALID = 3,
    BC_BLE_TX_FATAL = 4
} bc_ble_tx_result;

typedef bc_ble_tx_attempt_result (*bc_ble_tx_attempt_fn)(
    void *ctx,
    const uint8_t *data,
    uint16_t length);
typedef uint32_t (*bc_ble_tx_now_fn)(void *ctx);
typedef void (*bc_ble_tx_wait_fn)(void *ctx, uint32_t ticks);
typedef bool (*bc_ble_tx_session_current_fn)(void *ctx, uint32_t expected_session);

/*
 * Platform callbacks for one synchronous write operation.  The implementation
 * never takes ownership of data and never allocates storage.
 */
typedef struct bc_ble_tx_port {
    void *ctx;
    bc_ble_tx_attempt_fn attempt;
    bc_ble_tx_now_fn now;
    bc_ble_tx_wait_fn wait;
    bc_ble_tx_session_current_fn session_current;
} bc_ble_tx_port;

/*
 * Attempt data until the local BLE stack accepts it, the session changes, or
 * the fixed timeout expires.  A retry always waits before the next attempt;
 * the wait is capped by both poll_ticks and the remaining deadline.
 */
bc_ble_tx_result bc_ble_tx_write(
    const bc_ble_tx_port *port,
    const uint8_t *data,
    uint16_t length,
    uint32_t expected_session,
    uint32_t timeout_ticks,
    uint32_t poll_ticks);

#endif /* BC_BLE_TX_H */
