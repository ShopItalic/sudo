
//引用的C库头文件
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
//Log需要引用的头文件
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
//APP定时器需要引用的头文件
#include "app_timer.h"



//广播需要引用的头文件
#include "ble_advdata.h"
#include "ble_advertising.h"
//电源管理需要引用的头文件
#include "nrf_pwr_mgmt.h"
//SoftDevice handler configuration需要引用的头文件
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
//排序写入模块需要引用的头文件
#include "nrf_ble_qwr.h"
//GATT需要引用的头文件
#include "nrf_ble_gatt.h"
//连接参数协商需要引用的头文件
#include "ble_conn_params.h"
#include "nrf.h"

//DFU需要引用的头文件
#include "nrf_dfu_ble_svci_bond_sharing.h"
#include "nrf_svci_async_function.h"
#include "nrf_svci_async_handler.h"
#include "nrf_power.h"
#include "ble_dfu.h"
#include "nrf_bootloader_info.h"
#include "nrf_drv_clock.h"



#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "bsp.h"
#include "bc_module.h"
#include "app.h"
#include "user.h"

#include "test.h"

//用于stack dump的错误代码，可以用于栈回退时确定堆栈位置
#define DEAD_BEEF                       0xDEADBEEF     
               
#if NRF_LOG_ENABLED
static TaskHandle_t m_logger_thread;                                /**< Definition of Logger thread. */
#endif


//初始化电源管理模块
static void power_management_init(void)
{
    ret_code_t err_code;
	  //初始化电源管理
    err_code = nrf_pwr_mgmt_init();
	  //检查函数返回的错误代码
    APP_ERROR_CHECK(err_code);
}


#define FPU_EXCEPTION_MASK               0x0000009F                      //!< FPU exception mask used to clear exceptions in FPSCR register.

void power_manage(void)
{

    /* Clear exceptions and PendingIRQ from the FPU unit */

    __set_FPSCR(__get_FPSCR()  & ~(FPU_EXCEPTION_MASK));

    (void) __get_FPSCR();

    NVIC_ClearPendingIRQ(FPU_IRQn);

    uint32_t err_code = sd_app_evt_wait();

    APP_ERROR_CHECK(err_code);

}

//初始化APP定时器模块
static void timers_init(void)
{
    //初始化APP定时器模块
    ret_code_t err_code = app_timer_init();
	  //检查返回值
    APP_ERROR_CHECK(err_code);

    //加入创建用户定时任务的代码，创建用户定时任务。 

}
static void log_init(void)
{
    //初始化log程序模块
	  ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    //设置log输出终端（根据sdk_config.h中的配置设置输出终端为UART或者RTT）
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


#if NRF_LOG_ENABLED
/**@brief Thread for handling the logger.
 *
 * @details This thread is responsible for processing log entries if logs are deferred.
 *          Thread flushes all log entries and suspends. It is resumed by idle task hook.
 *
 * @param[in]   arg   Pointer used for passing some arbitrary information (context) from the
 *                    osThreadCreate() call to the thread.
 */
static void logger_thread(void * arg)
{
    UNUSED_PARAMETER(arg);

    while (1)
    {
        NRF_LOG_FLUSH();

        vTaskSuspend(NULL); // Suspend myself
    }
}
#endif //NRF_LOG_ENABLED

/**@brief A function which is hooked to idle task.
 * @note Idle hook must be enabled in FreeRTOS configuration (configUSE_IDLE_HOOK).
 */
#if defined(SUDO_VOICE_ONLY)
/* Debugger-visible, in-session diagnostics. These fields are not a persisted
 * crash record and are cleared by startup after reset. Never log from the
 * overflow hook: the affected task may already have exhausted its stack. */
volatile uint32_t sudo_rtos_malloc_failed;
volatile uint32_t sudo_rtos_stack_overflow;
volatile uintptr_t sudo_rtos_overflow_task;
volatile uintptr_t sudo_rtos_overflow_name;

void vApplicationMallocFailedHook(void)
{
    /* Preserve callers' existing allocation-failure handling and fallbacks. */
    sudo_rtos_malloc_failed = 1U;
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    sudo_rtos_stack_overflow = 1U;
    sudo_rtos_overflow_task = (uintptr_t)task;
    sudo_rtos_overflow_name = (uintptr_t)name;
    __disable_irq();
    if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0U)
        __BKPT(0);
    NVIC_SystemReset();
    for (;;) { }
}
#endif

void vApplicationIdleHook( void )
{
#if NRF_LOG_ENABLED
//	 vTaskResume(m_logger_thread);
#endif
}

/**@brief Function for initializing the clock.
 */
static void clock_init(void)
{
    ret_code_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
}

//主函数
int main(void)
{
	//nreset_disable_as_gpio();
    //disable_reset_pin();
	//初始化log程序模块
	log_init();
  clock_init();
     
#if NRF_LOG_ENABLED
    // Start execution.
  if (pdPASS != xTaskCreate(logger_thread, "LOGGER", 256, NULL, 1, &m_logger_thread))
  {
      APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
  }
#endif  
  
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

	//初始化APP定时器
	timers_init();

	power_management_init();
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

	bsp_init();
  bc_module_init();
  app_init();
	//test_timer_create();


 vTaskStartScheduler();
  //主循环
	while(true)
	{
		 APP_ERROR_HANDLER(NRF_ERROR_FORBIDDEN);
	}
}


