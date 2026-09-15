#ifndef TEST_FACTORY_PTT_RTOS_H
#define TEST_FACTORY_PTT_RTOS_H
#include <stdint.h>
#include <stddef.h>
typedef uintptr_t TaskHandle_t;
extern uint32_t fixture_ticks;
extern TaskHandle_t fixture_task;
void fixture_enter(void);
void fixture_leave(void);
void fixture_notify(TaskHandle_t task);
unsigned fixture_wait(unsigned clear, unsigned ticks);
#define configTICK_RATE_HZ 1024U
#define pdMS_TO_TICKS(ms) ((uint32_t)(ms) * configTICK_RATE_HZ / 1000U)
#define pdTRUE 1
#define xTaskGetTickCount() fixture_ticks
#define xTaskGetCurrentTaskHandle() fixture_task
#define taskENTER_CRITICAL() fixture_enter()
#define taskEXIT_CRITICAL() fixture_leave()
#define xTaskNotifyGive(task) fixture_notify(task)
#define ulTaskNotifyTake(clear, ticks) fixture_wait(clear, ticks)
#endif
