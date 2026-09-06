#ifndef SUDO_BATTERY_POWER_TEST_BC_RTOS_H
#define SUDO_BATTERY_POWER_TEST_BC_RTOS_H

#include <stdint.h>

/* The production board runs FreeRTOS at 1024 Hz; the host test uses 1 kHz so
 * millisecond boundary cases remain direct and readable. */
#ifndef configTICK_RATE_HZ
#define configTICK_RATE_HZ 1000U
#endif

extern uint32_t test_ticks;

/* This translation unit provides the task-context clock used by bc_power.c. */
#define bc_rtos_task_get_tick_count() test_ticks

void test_task_enter(void);
void test_task_exit(void);

#define bc_rtos_taskENTER_CRITICAL() test_task_enter()
#define bc_rtos_taskEXIT_CRITICAL() test_task_exit()

#endif
