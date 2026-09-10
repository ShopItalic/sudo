#ifndef BC_LOG_TASK_H
#define BC_LOG_TASK_H

#include <stddef.h>
#include <stdint.h>
#include "bc_rtos.h"
#if DEBUG_INFO == 1
#include "nrf.h"
#endif

/* The Sudo target never formats a log or evaluates log arguments in an ISR.
 * Task output is bounded and serialized without masking radio interrupts. */
void bc_log_task_printf(const char *format, ...);
void bc_log_task_hex(const char *label, const uint8_t *data, size_t length);

#define BC_LOG_TASK_CALL(...) do { \
    if (__get_IPSR() == 0U) { bc_log_task_printf(__VA_ARGS__); } \
} while (0)

#if DEBUG_INFO == 1
#define BC_LOG_INFO(format, ...) BC_LOG_TASK_CALL( \
    "\r\n[INFO:%lu;%s(%d)] " format, \
    (unsigned long)bc_rtos_task_get_tick_count(), __MODULE__, __LINE__, ##__VA_ARGS__)
#define BC_LOG_DEBUG(format, ...) BC_LOG_TASK_CALL( \
    "\r\n[DEBUG:%lu;%s:%d]: " format, \
    (unsigned long)bc_rtos_task_get_tick_count(), __MODULE__, __LINE__, ##__VA_ARGS__)
#define BC_LOG_ERROR(format, ...) BC_LOG_TASK_CALL( \
    "\r\n[ERROR:%lu;%s:%d]: " format, \
    (unsigned long)bc_rtos_task_get_tick_count(), __MODULE__, __LINE__, ##__VA_ARGS__)
#define BC_LOG_WARN(format, ...) BC_LOG_TASK_CALL( \
    "\r\n[WARN :%lu;%s:%d]: " format, \
    (unsigned long)bc_rtos_task_get_tick_count(), __MODULE__, __LINE__, ##__VA_ARGS__)
#define BC_LOG_PRINTF(...) BC_LOG_TASK_CALL(__VA_ARGS__)
#define BC_LOG_HEX(label, data, length) do { \
    if (__get_IPSR() == 0U) { bc_log_task_hex(label, data, length); } \
} while (0)
#else
#define BC_LOG_INFO(...) do { } while (0)
#define BC_LOG_DEBUG(...) do { } while (0)
#define BC_LOG_ERROR(...) do { } while (0)
#define BC_LOG_WARN(...) do { } while (0)
#define BC_LOG_PRINTF(...) do { } while (0)
#define BC_LOG_HEX(...) do { } while (0)
#endif

#endif
