#ifndef SUDO_LED_TEST_BC_RTOS_H
#define SUDO_LED_TEST_BC_RTOS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t bc_event_bits;
typedef int BaseType_t;
typedef unsigned UBaseType_t;
typedef void *EventGroupHandle_t;
typedef void *TaskHandle_t;
typedef void (*TaskFunction_t)(void *);
typedef BaseType_t bc_base_type_t;

#define bc_pdFALSE ((BaseType_t)0)
#define bc_pdTRUE ((BaseType_t)1)
#define bc_pdPASS ((bc_base_type_t)1)
#define bc_pdFAIL ((bc_base_type_t)0)
#define pdFALSE ((BaseType_t)0)
#define bc_rtos_max_delay UINT32_MAX
#define BC_IC_LED_STACK_SIZE 128U
#define BC_IC_LED__PRIO 12U

typedef struct
{
    char event_name[40];
    EventGroupHandle_t event_handler;
    BaseType_t event_clear_on_exit;
    BaseType_t event_wait_for_all_bits;
} bc_rtos_event_struct;

typedef struct
{
    char thread_name[40];
    uint16_t thread_stack_depth;
    TaskHandle_t thread_handler;
    UBaseType_t thread_priority;
    void *thread_parameters;
    TaskFunction_t thread_task_code;
    bool thread_status_lock;
} bc_rtos_thread_struct;

EventGroupHandle_t bc_rtos_event_group_create(void);
bc_event_bits bc_rtos_event_group_wait_bits(EventGroupHandle_t event_group,
                                             bc_event_bits bits_to_wait_for,
                                             BaseType_t clear_on_exit,
                                             BaseType_t wait_for_all_bits,
                                             uint32_t ticks_to_wait);
bc_event_bits bc_rtos_event_group_set_bits(EventGroupHandle_t event_group,
                                            bc_event_bits bits_to_set);
bc_base_type_t bc_rtos_thread_create(TaskFunction_t task_code,
                                     const char *name,
                                     uint16_t stack_depth,
                                     void *parameters,
                                     UBaseType_t priority,
                                     TaskHandle_t *created_task);
BaseType_t xEventGroupSetBitsFromISR(EventGroupHandle_t event_group,
                                     bc_event_bits bits_to_set,
                                     BaseType_t *higher_priority_task_woken);

#endif
