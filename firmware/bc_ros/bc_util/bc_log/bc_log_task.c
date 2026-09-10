#include "bc_log_task.h"
#include "nrf.h"
#include "sdk_config.h"
#include "SEGGER_RTT.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* No heap allocation or queue wait, including before scheduler startup.
 * The selected RTT channel must remain nonblocking. */
#if SEGGER_RTT_CONFIG_DEFAULT_MODE != 0
#error "Sudo task logging requires RTT nonblocking skip mode"
#endif

enum { BC_LOG_BYTES = 240, BC_LOG_HEX_BYTES = 48 };
static char log_buffer[BC_LOG_BYTES];

static void log_write(size_t length)
{
#if NRF_LOG_ENABLED
    /* The vendor RTT API initializes and locks its buffer. Mode 0 above
     * drops a record when there is no room instead of waiting for a host. */
    (void)SEGGER_RTT_Write(0, log_buffer, (unsigned)length);
#else
    (void)length;
#endif
}

void bc_log_task_printf(const char *format, ...)
{
    va_list args;
    int length;
    if (__get_IPSR() != 0U || format == NULL)
        return;

    vTaskSuspendAll();
    va_start(args, format);
    length = vsnprintf(log_buffer, sizeof(log_buffer), format, args);
    va_end(args);
    if (length > 0) {
        size_t used = (size_t)length;
        if (used >= sizeof(log_buffer))
            used = sizeof(log_buffer) - 1U;
        log_write(used);
    }
    (void)xTaskResumeAll();
}

void bc_log_task_hex(const char *label, const uint8_t *data, size_t length)
{
    static const char hex[] = "0123456789abcdef";
    size_t used = 0, count, i;
    if (__get_IPSR() != 0U || label == NULL || (data == NULL && length != 0U))
        return;

    vTaskSuspendAll();
    /* Keep room for a bounded prefix, 48 bytes, a truncation mark and CRLF. */
    while (used < 64U && label[used] != '\0') {
        log_buffer[used] = label[used];
        ++used;
    }
    count = length < BC_LOG_HEX_BYTES ? length : BC_LOG_HEX_BYTES;
    for (i = 0; i < count; ++i) {
        log_buffer[used++] = hex[data[i] >> 4];
        log_buffer[used++] = hex[data[i] & 15U];
        log_buffer[used++] = ' ';
    }
    if (count < length) {
        log_buffer[used++] = '.';
        log_buffer[used++] = '.';
        log_buffer[used++] = '.';
    }
    log_buffer[used++] = '\r';
    log_buffer[used++] = '\n';
    log_write(used);
    (void)xTaskResumeAll();
}
