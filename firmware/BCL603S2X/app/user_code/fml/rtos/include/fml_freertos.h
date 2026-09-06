#ifndef __FML_FREERTOS_H__
#define __FML_FREERTOS_H__

/* FreeRTOSÍ·ÎÄ¼þ */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

#include "stdbool.h"
#include "string.h"
#include "stdio.h"

#include "user_rtos_config.h"

#define fml_base_type_t  BaseType_t
#define fml_pdPASS       pdPASS			
#define fml_pdFAIL       pdFAIL			

/* Type definitions. */
#define fml_portCHAR          char
#define fml_portFLOAT         float
#define fml_portDOUBLE        double
#define fml_portLONG          long
#define fml_portSHORT         short
#define fml_portSTACK_TYPE    uint32_t
#define fml_portBASE_TYPE     long
	
#define fml_pdFALSE                                  ( ( BaseType_t ) 0 )
#define fml_pdTRUE                                   ( ( BaseType_t ) 1 )

#define fml_portYIELD_FROM_ISR( x )                     portEND_SWITCHING_ISR( x )

//time
#define fml_rtos_delay(ms)  vTaskDelay(ms)


// queue
typedef struct
{

	uint8_t        queue_count;
	uint8_t        queue_depth;
	char           queue_name[30];
	uint8_t        *queue_buff;
	uint16_t       queue_buff_length;
	QueueHandle_t  queue_handler;
}fml_rtos_queue_struct;

#define fml_rtos_queue_send(xQueue, pvItemToQueue) xQueueGenericSend( ( xQueue ), ( pvItemToQueue ),portMAX_DELAY, queueSEND_TO_BACK )
#define fml_rtos_queue_receive(xQueue,pvBuffer)  xQueueReceive(( xQueue ),( pvBuffer ),portMAX_DELAY)
#define fml_rtos_queue_create(uxQueueLength, uxItemSize)   xQueueCreate( uxQueueLength, uxItemSize )
#define fml_rtos_queue_isr_enqueue(xQueue, pvItemToQueue, pxHigherPriorityTaskWoken)   xQueueGenericSendFromISR( ( xQueue ), ( pvItemToQueue ), ( pxHigherPriorityTaskWoken ), queueSEND_TO_BACK )

//task
typedef struct
{
	char         thread_name[40];
	uint16_t     thread_stack_depth;
	TaskHandle_t thread_handler;
	UBaseType_t  thread_priority;
	void *       thread_parameters;
	void *       thread_task_code;
}fml_rtos_thread_struct;

#define fml_rtos_thread_create(pxTaskCode, pcName,usStackDepth,pvParameters,uxPriority,pxCreatedTask)   xTaskCreate( (pxTaskCode),( pcName),(usStackDepth),(pvParameters),(uxPriority),(pxCreatedTask))
#define fml_rtos_thread_suspend(xTaskToSuspend)   vTaskSuspend(xTaskToSuspend)
#define fml_rtos_thread_resume(xTaskToResume)   vTaskResume(xTaskToResume)
#define fml_rtos_thread_delete(xTaskToDelete)   vTaskDelete(xTaskToDelete)


void fml_rtos_thread_task_suspend(fml_rtos_thread_struct * thread_task);
void fml_rtos_thread_task_resume(fml_rtos_thread_struct * thread_task);


// timer
typedef struct 
{
	TimerHandle_t  timer_handler;
	char           timer_name[20];
	UBaseType_t    uxAutoReload;
	TickType_t     xTimerPeriodInTicks;
	uint32_t       timer_id;
	TimerCallbackFunction_t timer_callback_function;
}fml_rtos_timer_struct;

#define fml_rtos_timer_create(pcTimerName, xTimerPeriodInTicks,uxAutoReload,pvTimerID,pxCallbackFunction)  xTimerCreate( (pcTimerName),( xTimerPeriodInTicks),(uxAutoReload),(pvTimerID),(pxCallbackFunction))

#define fml_rtos_timer_start( xTimer, xTicksToWait ) xTimerGenericCommand( ( xTimer ), tmrCOMMAND_START, ( xTaskGetTickCount() ), NULL, ( xTicksToWait ) )

#define fml_rtos_timer_stop( xTimer, xTicksToWait ) xTimerGenericCommand( ( xTimer ), tmrCOMMAND_STOP, 0U, NULL, ( xTicksToWait ) )


//sem
typedef struct 
{
	char           sem_name[20];
	SemaphoreHandle_t  sem_handler;
}fml_rtos_sem_struct;

#define fml_rtos_sem_create()                xSemaphoreCreateMutex();
#define fml_rtos_semaphore_take(xSemaphore)  xSemaphoreTake( xSemaphore, portMAX_DELAY)
#define fml_rtos_semaphore_give(xSemaphore)  xSemaphoreGive( xSemaphore )

#endif


