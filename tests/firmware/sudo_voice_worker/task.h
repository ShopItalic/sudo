#ifndef SUDO_VOICE_WORKER_TASK_H
#define SUDO_VOICE_WORKER_TASK_H

#include "FreeRTOS.h"

typedef void *TaskHandle_t;
typedef void (*TaskFunction_t)(void *);

extern TickType_t test_ticks;
extern unsigned test_notify_calls;
extern unsigned test_yield_calls;

BaseType_t xTaskCreate(TaskFunction_t task, const char *name,
                       uint16_t stack_depth, void *parameters,
                       UBaseType_t priority, TaskHandle_t *created);
TickType_t xTaskGetTickCount(void);
TickType_t xTaskGetTickCountFromISR(void);
BaseType_t xTaskNotifyGive(TaskHandle_t task);
void vTaskNotifyGiveFromISR(TaskHandle_t task,
                            BaseType_t *higher_priority_task_woken);
uint32_t ulTaskNotifyTake(BaseType_t clear_count_on_exit,
                          TickType_t ticks_to_wait);

#define portYIELD_FROM_ISR(woken) \
    do { ++test_yield_calls; (void)(woken); } while (0)

#endif
