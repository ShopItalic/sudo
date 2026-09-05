#include "bc_ble_tx.h"

static bool bc_ble_tx_valid(const bc_ble_tx_port *port,
                            const uint8_t *data,
                            uint16_t length,
                            uint32_t timeout_ticks,
                            uint32_t poll_ticks)
{
    if (port == (const bc_ble_tx_port *)0 ||
        port->attempt == (bc_ble_tx_attempt_fn)0 ||
        port->now == (bc_ble_tx_now_fn)0 ||
        port->wait == (bc_ble_tx_wait_fn)0 ||
        port->session_current == (bc_ble_tx_session_current_fn)0) {
        return false;
    }

    if (data == (const uint8_t *)0 ||
        length == 0U ||
        length > BC_BLE_TX_MAX_LENGTH) {
        return false;
    }

    if (timeout_ticks == 0U ||
        timeout_ticks > BC_BLE_TX_MAX_TICKS ||
        poll_ticks == 0U ||
        poll_ticks > BC_BLE_TX_MAX_TICKS) {
        return false;
    }

    return true;
}

bc_ble_tx_result bc_ble_tx_write(const bc_ble_tx_port *port,
                                 const uint8_t *data,
                                 uint16_t length,
                                 uint32_t expected_session,
                                 uint32_t timeout_ticks,
                                 uint32_t poll_ticks)
{
    uint32_t start_ticks;

    if (!bc_ble_tx_valid(port, data, length, timeout_ticks, poll_ticks)) {
        return BC_BLE_TX_INVALID;
    }

    start_ticks = port->now(port->ctx);

    for (;;) {
        uint32_t current_ticks;
        uint32_t elapsed_ticks;
        bc_ble_tx_attempt_result attempt_result;

        /* Keep this immediately before every attempt, including after wait. */
        if (!port->session_current(port->ctx, expected_session)) {
            return BC_BLE_TX_CANCELLED;
        }

        current_ticks = port->now(port->ctx);
        elapsed_ticks = (uint32_t)(current_ticks - start_ticks);
        if (elapsed_ticks >= timeout_ticks) {
            return BC_BLE_TX_TIMEOUT;
        }

        attempt_result = port->attempt(port->ctx, data, length);
        if (attempt_result == BC_BLE_TX_ATTEMPT_ACCEPTED) {
            return BC_BLE_TX_ACCEPTED;
        }
        if (attempt_result == BC_BLE_TX_ATTEMPT_FATAL) {
            return BC_BLE_TX_FATAL;
        }
        if (attempt_result != BC_BLE_TX_ATTEMPT_RETRY) {
            return BC_BLE_TX_FATAL;
        }

        /*
         * Re-read the clock after the attempt.  The deadline is anchored at
         * start_ticks, so callback wakeups cannot extend it.  If the attempt
         * consumed the last tick, return without a zero-length busy wait.
         */
        current_ticks = port->now(port->ctx);
        elapsed_ticks = (uint32_t)(current_ticks - start_ticks);
        if (elapsed_ticks >= timeout_ticks) {
            return BC_BLE_TX_TIMEOUT;
        }

        {
            uint32_t remaining_ticks = timeout_ticks - elapsed_ticks;
            uint32_t wait_ticks = poll_ticks;

            if (wait_ticks > remaining_ticks) {
                wait_ticks = remaining_ticks;
            }
            port->wait(port->ctx, wait_ticks);
        }
    }
}
