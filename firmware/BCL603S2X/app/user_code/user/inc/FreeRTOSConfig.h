/*
 * FreeRTOS Kernel V10.0.0
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */


#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#ifdef SOFTDEVICE_PRESENT
#include "nrf_soc.h"
#endif
#include "app_util_platform.h"

/*-----------------------------------------------------------
 * Possible configurations for system timer
 */
#define FREERTOS_USE_RTC      0 /**< Use real time clock for the system */
#define FREERTOS_USE_SYSTICK  1 /**< Use SysTick timer for system */

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#define configTICK_SOURCE FREERTOS_USE_RTC

/* 置1：RTOS使用抢占式调度器；置0：RTOS使用协作式调度器（时间片）
 * 
 * 注：在多任务管理机制上，操作系统可以分为抢占式和协作式两种。
 * 协作式操作系统是任务主动释放CPU后，切换到下一个任务。
 * 任务切换的时机完全取决于正在运行的任务。
 */
#define configUSE_PREEMPTION 1

#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configUSE_TICKLESS_IDLE 1
#define configUSE_TICKLESS_IDLE_SIMPLE_DEBUG                                      1 /* See into vPortSuppressTicksAndSleep source code for explanation */
#define configCPU_CLOCK_HZ                                                        ( SystemCoreClock )
#define configTICK_RATE_HZ                                                        1024

//可使用的最大优先级
#define configMAX_PRIORITIES                                                      ( 32 )

//空闲任务使用的堆栈大小
#define configMINIMAL_STACK_SIZE                                                  ( 60 )

//系统所有总的堆大小
#define configTOTAL_HEAP_SIZE                                                     ( 1024*30 )

//任务名字字符串长度
#define configMAX_TASK_NAME_LEN                                                   ( 40 )

 //系统节拍计数器变量数据类型，1表示为16位无符号整形，0表示为32位无符号整形
#define configUSE_16_BIT_TICKS                                                    0

//空闲任务放弃CPU使用权给其他同优先级的用户任务
#define configIDLE_SHOULD_YIELD                                                   1

//使用互斥信号量
#define configUSE_MUTEXES                                                         1

//使用递归互斥信号量
#define configUSE_RECURSIVE_MUTEXES                                               1

//为1时使用计数信号量
#define configUSE_COUNTING_SEMAPHORES                                             1
#define configUSE_ALTERNATIVE_API                                                 0    /* Deprecated! */

/* 设置可以注册的信号量和消息队列个数 */
#define configQUEUE_REGISTRY_SIZE                                                 20

//启用队列
#define configUSE_QUEUE_SETS                                                      1

//1使能时间片调度(默认式使能的)
#define configUSE_TIME_SLICING                                                    1
#define configUSE_NEWLIB_REENTRANT                                                0
#define configENABLE_BACKWARD_COMPATIBILITY                                       1

/* Hook function related definitions. */
/* 置1：使用空闲钩子（Idle Hook类似于回调函数）；置0：忽略空闲钩子
 * 
 * 空闲任务钩子是一个函数，这个函数由用户来实现，
 * FreeRTOS规定了函数的名字和参数：void vApplicationIdleHook(void )，
 * 这个函数在每个空闲任务周期都会被调用
 * 对于已经删除的RTOS任务，空闲任务可以释放分配给它们的堆栈内存。
 * 因此必须保证空闲任务可以被CPU执行
 * 使用空闲钩子函数设置CPU进入省电模式是很常见的
 * 不可以调用会引起空闲任务阻塞的API函数
 */
#define configUSE_IDLE_HOOK 1
/* 置1：使用时间片钩子（Tick Hook）；置0：忽略时间片钩子
 * 
 * 
 * 时间片钩子是一个函数，这个函数由用户来实现，
 * FreeRTOS规定了函数的名字和参数：void vApplicationTickHook(void )
 * 时间片中断可以周期性的调用
 * 函数必须非常短小，不能大量使用堆栈，
 * 不能调用以”FromISR" 或 "FROM_ISR”结尾的API函数
 */
 /*xTaskIncrementTick函数是在xPortSysTickHandler中断函数中被调用的。因此，vApplicationTickHook()函数执行的时间必须很短才行*/
#define configUSE_TICK_HOOK                                                       0

//使用内存申请失败钩子函数
#define configCHECK_FOR_STACK_OVERFLOW                                            0

/*
 * 大于0时启用堆栈溢出检测功能，如果使用此功能 
 * 用户必须提供一个栈溢出钩子函数，如果使用的话
 * 此值可以为1或者2，因为有两种栈溢出检测方法 */
#define configUSE_MALLOC_FAILED_HOOK                                              0

/* Run time and task stats gathering related definitions. */
//启用运行时间统计功能
#define configGENERATE_RUN_TIME_STATS                                             0

 //启用可视化跟踪调试
#define configUSE_TRACE_FACILITY                                                  0

/* 与宏configUSE_TRACE_FACILITY同时为1时会编译下面3个函数
 * prvWriteNameToBuffer()
 * vTaskList(),
 * vTaskGetRunTimeStats()
*/
#define configUSE_STATS_FORMATTING_FUNCTIONS                                      0

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES                                                     0
#define configMAX_CO_ROUTINE_PRIORITIES                                           ( 2 )

/* Software timer definitions. */

 //启用软件定时器
#define configUSE_TIMERS 1

//软件定时器优先级
#define configTIMER_TASK_PRIORITY                                                 (2 ) //(configMAX_PRIORITIES-1)  //( 2 )

//软件定时器队列长度
#define configTIMER_QUEUE_LENGTH                                                  32

//软件定时器任务堆栈大小
#define configTIMER_TASK_STACK_DEPTH                                             1024// (configMINIMAL_STACK_SIZE*2)  //( 80 )

/* Tickless Idle configuration. */
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP                                     2

/* Tickless idle/low power functionality. */


/* Define to trap errors during development. */
#if defined(DEBUG_NRF) || defined(DEBUG_NRF_USER)
#define configASSERT( x )                                                         ASSERT(x)
#endif

/* FreeRTOS MPU specific definitions. */
#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS                    1

/* Optional functions - most linkers will remove unused functions anyway. */
#define INCLUDE_vTaskPrioritySet                                                  1
#define INCLUDE_uxTaskPriorityGet                                                 1
#define INCLUDE_vTaskDelete                                                       1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_xResumeFromISR                                                    1
#define INCLUDE_vTaskDelayUntil                                                   1
#define INCLUDE_vTaskDelay                                                        1
#define INCLUDE_xTaskGetSchedulerState                                            1
#define INCLUDE_xTaskGetCurrentTaskHandle                                         1
#define INCLUDE_uxTaskGetStackHighWaterMark                                       1
#define INCLUDE_xTaskGetIdleTaskHandle                                            1
#define INCLUDE_xTimerGetTimerDaemonTaskHandle                                    1
#define INCLUDE_pcTaskGetTaskName                                                 1
#define INCLUDE_eTaskGetState                                                     1
#define INCLUDE_xEventGroupSetBitFromISR                                          1
#define INCLUDE_xTimerPendFunctionCall                                            1

/* The lowest interrupt priority that can be used in a call to a "set priority"
function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         0xf

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY _PRIO_APP_HIGH


/* Interrupt priorities used by the kernel port layer itself.  These are generic
to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY                 configLIBRARY_LOWEST_INTERRUPT_PRIORITY
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY            configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names - or at least those used in the unmodified vector table. */

#define vPortSVCHandler                                                           SVC_Handler
#define xPortPendSVHandler                                                        PendSV_Handler


/*-----------------------------------------------------------
 * Settings that are generated automatically
 * basing on the settings above
 */
#if (configTICK_SOURCE == FREERTOS_USE_SYSTICK)
    // do not define configSYSTICK_CLOCK_HZ for SysTick to be configured automatically
    // to CPU clock source
    #define xPortSysTickHandler     SysTick_Handler
#elif (configTICK_SOURCE == FREERTOS_USE_RTC)
    #define configSYSTICK_CLOCK_HZ  ( 32768UL )
    #define xPortSysTickHandler     RTC1_IRQHandler
#else
    #error  Unsupported configTICK_SOURCE value
#endif

/* Code below should be only used by the compiler, and not the assembler. */
#if !(defined(__ASSEMBLY__) || defined(__ASSEMBLER__))
    #include "nrf.h"
    #include "nrf_assert.h"

    /* This part of definitions may be problematic in assembly - it uses definitions from files that are not assembly compatible. */
    /* Cortex-M specific definitions. */
    #ifdef __NVIC_PRIO_BITS
        /* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
        #define configPRIO_BITS             __NVIC_PRIO_BITS
    #else
        #error "This port requires __NVIC_PRIO_BITS to be defined"
    #endif

    /* Access to current system core clock is required only if we are ticking the system by systimer */
    #if (configTICK_SOURCE == FREERTOS_USE_SYSTICK)
        #include <stdint.h>
        extern uint32_t SystemCoreClock;
    #endif
#endif /* !assembler */

/** Implementation note:  Use this with caution and set this to 1 ONLY for debugging
 * ----------------------------------------------------------
     * Set the value of configUSE_DISABLE_TICK_AUTO_CORRECTION_DEBUG to below for enabling or disabling RTOS tick auto correction:
     * 0. This is default. If the RTC tick interrupt is masked for more than 1 tick by higher priority interrupts, then most likely
     *    one or more RTC ticks are lost. The tick interrupt inside RTOS will detect this and make a correction needed. This is needed
     *    for the RTOS internal timers to be more accurate.
     * 1. The auto correction for RTOS tick is disabled even though few RTC tick interrupts were lost. This feature is desirable when debugging
     *    the RTOS application and stepping though the code. After stepping when the application is continued in debug mode, the auto-corrections of
     *    RTOS tick might cause asserts. Setting configUSE_DISABLE_TICK_AUTO_CORRECTION_DEBUG to 1 will make RTC and RTOS go out of sync but could be
     *    convenient for debugging.
     */
#define configUSE_DISABLE_TICK_AUTO_CORRECTION_DEBUG     0

#endif /* FREERTOS_CONFIG_H */
