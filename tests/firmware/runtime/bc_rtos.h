#ifndef TEST_RUNTIME_RTOS_H
#define TEST_RUNTIME_RTOS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef int BaseType_t;
typedef unsigned UBaseType_t;
typedef void *TimerHandle_t;
typedef void (*TimerCallbackFunction_t)(void *);
typedef struct {
    TimerHandle_t timer_handler;
    char timer_name[40];
    UBaseType_t uxAutoReload;
    uint32_t xTimerPeriodInTicks;
    uint32_t timer_id;
    TimerCallbackFunction_t timer_callback_function;
    bool lock;
} bc_rtos_timer_struct;
#define pdFALSE 0
#define pdTRUE 1
#define pdPASS 1
#define pdFAIL 0
uint32_t test_task_ticks(void);
#define bc_rtos_task_get_tick_count() test_task_ticks()
void vTaskSuspendAll(void);
BaseType_t xTaskResumeAll(void);
#endif
