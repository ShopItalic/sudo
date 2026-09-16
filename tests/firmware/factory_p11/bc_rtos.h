#ifndef P11_TEST_RTOS_H
#define P11_TEST_RTOS_H
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#define pdTRUE 1
#define portMAX_DELAY UINT32_MAX
#define pdMS_TO_TICKS(ms) ((TickType_t)(((uint64_t)(ms) * configTICK_RATE_HZ + 999) / 1000))
#define configUSE_MALLOC_FAILED_HOOK 0
size_t xPortGetFreeHeapSize(void);
TaskHandle_t xTaskGetCurrentTaskHandle(void);
unsigned ulTaskNotifyTake(BaseType_t clear, TickType_t wait);
void xTaskNotifyGive(TaskHandle_t task);
void vTaskDelay(TickType_t ticks);
#endif
