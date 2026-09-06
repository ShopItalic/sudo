#ifndef TEST_MOTOR_BC_RTOS_H
#define TEST_MOTOR_BC_RTOS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef void *TaskHandle_t;
typedef void *TimerHandle_t;
typedef unsigned int UBaseType_t;
typedef int BaseType_t;
typedef void (*TimerCallbackFunction_t)(void *);
typedef void (*TaskFunction_t)(void *);
typedef BaseType_t bc_base_type_t;

typedef struct
{
    char thread_name[40];
    uint16_t thread_stack_depth;
    TaskHandle_t thread_handler;
    UBaseType_t thread_priority;
    void *thread_parameters;
    void *thread_task_code;
    bool thread_status_lock;
} bc_rtos_thread_struct;

typedef struct
{
    char timer_name[40];
    UBaseType_t uxAutoReload;
    unsigned int xTimerPeriodInTicks;
    uint32_t timer_id;
    TimerCallbackFunction_t timer_callback_function;
    TimerHandle_t timer_handler;
    bool lock;
} bc_rtos_timer_struct;

#define APP_LINEAR_MOTOR_STACK_SIZE 128
#define APP_LINEAR_MOTOR_PRIO 8
#define bc_pdPASS 1
#ifndef configTICK_RATE_HZ
#define configTICK_RATE_HZ 1000U
#endif

extern uint32_t test_ticks;
extern unsigned test_critical_depth;
#define bc_rtos_task_get_tick_count() test_ticks
#define xTaskGetTickCountFromISR() test_ticks
#define bc_rtos_taskENTER_CRITICAL() (++test_critical_depth)
#define bc_rtos_taskEXIT_CRITICAL() (--test_critical_depth)
#define taskENTER_CRITICAL_FROM_ISR() ((UBaseType_t)(++test_critical_depth))
#define taskEXIT_CRITICAL_FROM_ISR(saved) \
    ((void)(saved), (void)(--test_critical_depth))
#define bc_rtos_thread_resume(handle) ((void)(handle))
#define bc_rtos_thread_suspend(handle) ((void)(handle))
#define bc_rtos_timer_start(handle, wait) ((void)(handle), (void)(wait), 1)
#define bc_rtos_timer_stop(handle, wait) ((void)(handle), (void)(wait), 1)
#define bc_rtos_timer_create(...) ((TimerHandle_t)0)
#define bc_rtos_thread_create(...) (1)

#endif
