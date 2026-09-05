#ifndef SUDO_CAPTURE_TEST_FREERTOS_H
#define SUDO_CAPTURE_TEST_FREERTOS_H

#include <stdint.h>

typedef int BaseType_t;
typedef uint32_t TickType_t;
#define configTICK_RATE_HZ 1024U

#ifndef pdFALSE
#define pdFALSE ((BaseType_t)0)
#endif

/* Matches the priority relationship checked by the production adapter. */
#ifndef configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#endif

extern unsigned test_critical_depth;
void test_task_enter(void);
void test_task_exit(void);

#define taskENTER_CRITICAL() test_task_enter()
#define taskEXIT_CRITICAL() test_task_exit()

#endif
