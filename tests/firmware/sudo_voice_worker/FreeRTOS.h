#ifndef SUDO_VOICE_WORKER_FREERTOS_H
#define SUDO_VOICE_WORKER_FREERTOS_H

#include <stdint.h>

typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef uint32_t TickType_t;

#define configTICK_RATE_HZ 1024U
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define pdFALSE ((BaseType_t)0)
#define pdTRUE ((BaseType_t)1)
#define pdPASS ((BaseType_t)1)
#define pdFAIL ((BaseType_t)0)
#define pdMS_TO_TICKS(milliseconds) \
    ((TickType_t)(((uint64_t)(milliseconds) * configTICK_RATE_HZ) / 1000U))

extern unsigned test_critical_depth;
void test_task_enter(void);
void test_task_exit(void);

#define taskENTER_CRITICAL() test_task_enter()
#define taskEXIT_CRITICAL() test_task_exit()

#endif
