#ifndef __BC_RTOS_H__
#define __BC_RTOS_H__

/* FreeRTOS头文件 */
#include "FreeRTOS.h"
#include "task.h"       // 任务管理
#include "queue.h"      // 队列管理
#include "timers.h"     // 软件定时器
#include "semphr.h"     // 信号量
#include "event_groups.h" //事件

#include "stdbool.h"    // 布尔类型
#include "string.h"     // 字符串操作
#include "stdio.h"      // 标准输入输出

#include "user_rtos_config.h"  // 用户自定义FreeRTOS配置

/* 基础类型重定义 */
#define bc_event_bits  EventBits_t
#define bc_base_type_t  BaseType_t  // FreeRTOS基础类型
#define bc_pdPASS       pdPASS      // 操作成功返回值
#define bc_pdFAIL       pdFAIL      // 操作失败返回值

/* 类型定义 */
#define bc_char          char       // 字符类型
#define bc_float         float      // 单精度浮点
#define bc_double        double     // 双精度浮点
#define bc_long          long       // 长整型
#define bc_short         short      // 短整型
#define bc_uint32        uint32_t   // 32位无符号整型

/* 布尔值定义 */
#define bc_pdFALSE       ( ( BaseType_t ) 0 )  // 假值
#define bc_pdTRUE        ( ( BaseType_t ) 1 )  // 真值

#define bc_rtos_taskENTER_CRITICAL()    taskENTER_CRITICAL();           //进入临界区
#define bc_rtos_taskEXIT_CRITICAL()    taskEXIT_CRITICAL();            //退出临界区

/* 中断中切换任务 */
#define bc_portYIELD_FROM_ISR( x )  portEND_SWITCHING_ISR( x )

/* 时间相关 */
#define bc_rtos_delay(ms)  vTaskDelay(ms)  // 任务延时(毫秒)
#define bc_rtos_task_get_tick_count()   xTaskGetTickCount()   // 获取系统节拍数
#define bc_rtos_max_delay portMAX_DELAY


// 在调试版本中，分配失败时打印错误信息
#if (0)
    #define bc_rtos_malloc(size)                                 \
    ({                                                      \
        void *__ptr = pvPortMalloc(size);                   \
        if (__ptr == NULL && (size) != 0) {                 \
            printf("[MALLOC FAIL] File:%s, Line:%d\n",      \
                   __FILE__, __LINE__);                     \
        }                                                   \
        __ptr;                                              \
    })

    #define bc_rtos_free(ptr)        \
    do {                        \
        vPortFree(ptr);         \
        (ptr) = NULL;           \
         printf("malloc free fail\r\n"); \
         bc_rtos_delay(1000);\
    } while(0)
#else
    // 非调试版本使用简单定义
    #define bc_rtos_malloc(size)    pvPortMalloc(size)
    #define bc_rtos_free(ptr)       vPortFree(ptr)
#endif

/* 队列相关结构体 */
typedef struct
{
    uint8_t        queue_count;        // 队列当前消息数量
    char           queue_name[30];     // 队列名称
    uint16_t       queue_depth;        // 队列深度(最大消息数)
    uint8_t        *queue_buff;        // 队列缓冲区指针
    uint32_t       queue_buff_length;  // 队列缓冲区长度
    uint32_t       enqueue_count;      // 队列入队次数
    uint32_t       dequeue_conut;      // 队列出队次数
    QueueHandle_t  queue_handler;      // FreeRTOS队列句柄
} bc_rtos_queue_struct;

/* 队列操作宏 */
#define bc_rtos_queue_send(xQueue, pvItemToQueue) xQueueGenericSend( ( xQueue ), ( pvItemToQueue ), portMAX_DELAY, queueSEND_TO_BACK )  // 发送消息到队列尾部
#define bc_rtos_queue_receive(xQueue,pvBuffer)  xQueueReceive(( xQueue ),( pvBuffer ),portMAX_DELAY)  // 从队列接收消息
#define bc_rtos_queue_create(uxQueueLength, uxItemSize) xQueueCreate( uxQueueLength, uxItemSize )  // 创建队列
#define bc_rtos_queue_isr_enqueue(xQueue, pvItemToQueue, pxHigherPriorityTaskWoken) xQueueGenericSendFromISR( ( xQueue ), ( pvItemToQueue ), ( pxHigherPriorityTaskWoken ), queueSEND_TO_BACK )  // 中断中发送消息
#define bc_rtos_queue_delete( xQueue ) vQueueDelete( xQueue )  // 删除队列
#define bc_rtos_queue_reset( xQueue ) xQueueReset( xQueue )    // 重置队列
#define bc_rtos_queue_messages_waiting( xQueue ) uxQueueMessagesWaiting( ( QueueHandle_t ) ( xQueue) )  // 获取队列中等待的消息数

/* 任务相关结构体 */
typedef struct
{
    char         thread_name[40];       // 任务名称
    uint16_t     thread_stack_depth;    // 任务堆栈深度
    TaskHandle_t thread_handler;        // 任务句柄
    UBaseType_t  thread_priority;       // 任务优先级
    void *       thread_parameters;     // 任务参数
    void *       thread_task_code;      // 任务函数指针
    bool         thread_status_lock;    // 任务锁定状态
} bc_rtos_thread_struct;

/* 任务操作宏 */
#define bc_rtos_thread_create(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask) \
    xTaskCreate( (pxTaskCode), (pcName), (usStackDepth), (pvParameters), (uxPriority), (pxCreatedTask))  // 创建任务

#define bc_rtos_thread_suspend(xTaskToSuspend) vTaskSuspend(xTaskToSuspend)  // 挂起任务
#define bc_rtos_thread_resume(xTaskToResume) vTaskResume(xTaskToResume)      // 恢复任务
#define bc_rtos_thread_resume_from_isr(xTaskToResume) xTaskResumeFromISR(xTaskToResume)  // 在中断中恢复任务
#define bc_rtos_thread_delete(xTaskToDelete) vTaskDelete(xTaskToDelete)     // 删除任务
#define bc_rtos_thread_start_scheduler() vTaskStartScheduler();   /* 启动任务，开启调度 */
#define bc_rtos_thread_notify_take(xClearCountOnExit, xTicksToWait)   ulTaskNotifyTake( xClearCountOnExit, xTicksToWait) //获取任务通知
#define bc_rtos_thread_notify_give( xTaskToNotify)  xTaskNotifyGive( xTaskToNotify )   //发送通知
void bc_rtos_thread_notify_give_from_isr(bc_rtos_thread_struct * thread_task,BaseType_t  TaskWoken); //中断内发送通知

/* 任务操作函数声明 */
void bc_rtos_thread_task_suspend(bc_rtos_thread_struct * thread_task);  // 挂起指定任务
void bc_rtos_thread_task_resume(bc_rtos_thread_struct * thread_task);   // 恢复指定任务


/* 事件结构体 */
typedef struct
{
    char               event_name[40];       // 事件名称
    EventGroupHandle_t event_handler;        // 事件句柄
    BaseType_t         event_clear_on_exit;
    BaseType_t         event_wait_for_all_bits;
    
} bc_rtos_event_struct;



#define bc_rtos_event_group_wait_bits(xEventGroup,uxBitsToWaitFor, xClearOnExit, xWaitForAllBits,xTicksToWait ) \
          xEventGroupWaitBits(xEventGroup,uxBitsToWaitFor,xClearOnExit,xWaitForAllBits,xTicksToWait )

#define bc_rtos_event_group_set_bits( xEventGroup,uxBitsToSet ) xEventGroupSetBits(xEventGroup,uxBitsToSet )

#define bc_rtos_event_group_create() xEventGroupCreate()


/* 定时器相关结构体 */
typedef struct 
{
    TimerHandle_t  timer_handler;           // 定时器句柄
    char           timer_name[40];          // 定时器名称
    UBaseType_t    uxAutoReload;            // 是否自动重载
    TickType_t     xTimerPeriodInTicks;     // 定时周期(节拍数)
    uint32_t       timer_id;                // 定时器ID
    TimerCallbackFunction_t timer_callback_function;  // 定时器回调函数
    bool           lock;                    // 定时器锁定状态
} bc_rtos_timer_struct;

/* 定时器操作宏 */
#define bc_rtos_timer_create(pcTimerName, xTimerPeriodInTicks, uxAutoReload, pvTimerID, pxCallbackFunction) \
    xTimerCreate( (pcTimerName), (xTimerPeriodInTicks), (uxAutoReload), (pvTimerID), (pxCallbackFunction))  // 创建定时器

#define bc_rtos_timer_start(xTimer, xTicksToWait) \
    xTimerGenericCommand( (xTimer), tmrCOMMAND_START, (xTaskGetTickCount()), NULL, (xTicksToWait))  // 启动定时器

#define bc_rtos_timer_stop(xTimer, xTicksToWait) \
    xTimerGenericCommand( (xTimer), tmrCOMMAND_STOP, 0U, NULL, (xTicksToWait))  // 停止定时器

#define bc_rtos_timer_change_period(xTimer, xNewPeriod, xTicksToWait) \
    xTimerGenericCommand( (xTimer), tmrCOMMAND_CHANGE_PERIOD, (pdMS_TO_TICKS(xNewPeriod)), NULL, (xTicksToWait))  // 修改定时器周期

#define bc_rtos_timer_reset(xTimer, xTicksToWait) \
    xTimerGenericCommand( (xTimer), tmrCOMMAND_RESET, (xTaskGetTickCount()), NULL, (xTicksToWait))  // 重置定时器

/* 互斥信号量相关结构体 */
typedef struct 
{
    char           sem_name[20];          // 信号量名称
    SemaphoreHandle_t  sem_handler;       // 信号量句柄
} bc_rtos_sem_struct;

/* 互斥信号量操作宏 */
#define bc_rtos_sem_create() xSemaphoreCreateMutex()  // 创建互斥信号量
#define bc_rtos_semaphore_take(xSemaphore) xSemaphoreTake(xSemaphore, portMAX_DELAY)  // 获取信号量
#define bc_rtos_semaphore_give(xSemaphore) xSemaphoreGive(xSemaphore)  // 释放信号量

/* 计数信号量相关结构体 */
typedef struct 
{
    char           sem_name[20];          // 信号量名称
    uint32_t       max_count;             // 最大计数值
    uint32_t       initial_count;         // 初始计数值
    SemaphoreHandle_t  sem_handler;       // 信号量句柄
} bc_rtos_sem_count_struct;

/* 计数信号量操作宏 */
#define bc_rtos_semaphore_create_counting(uxMaxCount, uxInitialCount) \
    xSemaphoreCreateCounting(uxMaxCount, uxInitialCount)  // 创建计数信号量

#define bc_rtos_semaphore_count_take(xSemaphore) \
    xSemaphoreTake(xSemaphore, portMAX_DELAY)  // 获取计数信号量

#define bc_rtos_semaphore_count_give(xSemaphore) \
    xSemaphoreGive(xSemaphore)  // 释放计数信号量

#endif  // __BC_RTOS_H__




