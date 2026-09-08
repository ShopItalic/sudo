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

#include <stddef.h>

extern unsigned test_critical_depth;
void test_task_enter(void);
void test_task_exit(void);

/* One-time codec allocations come from the RTOS heap. The shim counts them
 * and can fail the next request to prove Start reports UNSUPPORTED. */
void *pvPortMalloc(size_t size);
extern unsigned test_heap_allocations;
extern size_t test_heap_bytes;
extern unsigned test_heap_fail_remaining;

#define taskENTER_CRITICAL() test_task_enter()
#define taskEXIT_CRITICAL() test_task_exit()

#endif
