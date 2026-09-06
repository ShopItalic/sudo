#ifndef SUDO_CAPTURE_TEST_TASK_H
#define SUDO_CAPTURE_TEST_TASK_H

#include <stddef.h>

#include "FreeRTOS.h"

/* A task handle is opaque to the adapter. The test uses a non-NULL token. */
typedef void *TaskHandle_t;

extern unsigned test_notify_calls;
extern unsigned test_yield_calls;
extern TickType_t test_ticks;
static inline TickType_t xTaskGetTickCount(void) { return test_ticks; }
static inline TickType_t xTaskGetTickCountFromISR(void) { return test_ticks; }

static inline void vTaskNotifyGiveFromISR(TaskHandle_t task,
                                          BaseType_t *higher_priority_task_woken)
{
    (void)task;
    ++test_notify_calls;
    if (higher_priority_task_woken != NULL)
        *higher_priority_task_woken = pdFALSE;
}

#define portYIELD_FROM_ISR(woken) \
    do { ++test_yield_calls; (void)(woken); } while (0)

#endif
