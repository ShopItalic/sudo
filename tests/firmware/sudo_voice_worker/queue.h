#ifndef SUDO_VOICE_WORKER_QUEUE_H
#define SUDO_VOICE_WORKER_QUEUE_H

#include "FreeRTOS.h"

typedef void *QueueHandle_t;

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t item_size);
BaseType_t xQueueSend(QueueHandle_t queue, const void *item,
                      TickType_t ticks_to_wait);
BaseType_t xQueueReceive(QueueHandle_t queue, void *item,
                         TickType_t ticks_to_wait);
UBaseType_t uxQueueMessagesWaiting(QueueHandle_t queue);
void vQueueDelete(QueueHandle_t queue);

#endif
