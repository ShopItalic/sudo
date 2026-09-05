#ifndef SUDO_BATTERY_POWER_TEST_BC_RTOS_H
#define SUDO_BATTERY_POWER_TEST_BC_RTOS_H

void test_task_enter(void);
void test_task_exit(void);

#define bc_rtos_taskENTER_CRITICAL() test_task_enter()
#define bc_rtos_taskEXIT_CRITICAL() test_task_exit()

#endif
