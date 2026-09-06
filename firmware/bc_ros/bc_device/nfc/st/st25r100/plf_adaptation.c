/******************************************************************************
  * @attention
  *
  * COPYRIGHT 2020 STMicroelectronics, all rights reserved
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied,
  * AND SPECIFICALLY DISCLAIMING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
******************************************************************************/



#include "rfal_platform.h"

//#include "demo.h"
#include "timers.h"
#include "task.h"
#include "semphr.h"
#include "st25r200_irq.h"
#include "bc_rtos.h"

/* Private typedef -----------------------------------------------------------*/

 /*! typedef holding platform timer structure */
typedef struct {
  TimerHandle_t      handle;        /*!< Timer Handle                  */
  const char * const name;          /*!< Timer Name                    */
  StaticTimer_t      timerBuf;      /*!< Timer Buffer                  */
  bool               isUsed;        /*!< Timer in use flag             */
} platformTimer;

/* Private define ------------------------------------------------------------*/
#define PLF_TIMER_MAX 10U          /*!< Timers pool size               */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/** @defgroup PTD_Main_Private_Variables
 * @{
 */
/*! NFC Timers pool               */


enum nfc_task
{
	NFC_TASK_TYPE_IRQ = 0,
  NFC_TASK_TYPE_DEFAULT,
	NFC_TASK_TYPE_NUM
};

static void nfc_irq_handler_thread(void *thread_handler);
static void nfc_default_handler_thread(void *thread_handler);

static bc_rtos_thread_struct thread_struct[NFC_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "nfc irq handler task",
                                                                      .thread_stack_depth   = APP_TASK_RTC_STACK_SIZE ,
                                                                      .thread_priority      = APP_TASK_RTC_PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = nfc_irq_handler_thread,
                                                                    },
                                                                    {
                                                                      .thread_name          = "nfc default handler task",
                                                                      .thread_stack_depth   = APP_TASK_RTC_STACK_SIZE ,
                                                                      .thread_priority      = APP_TASK_RTC_PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = nfc_default_handler_thread,
                                                                    },                                                                    
                                                                  };


void plfErrorHandler(void);





void platformTimerCallback(TimerHandle_t xTimer );
static bc_rtos_timer_struct  timer_struct[PLF_TIMER_MAX] = {
  {
		.timer_name = "0",
		.uxAutoReload = false,
		.xTimerPeriodInTicks = 5,
		.lock = false,
		.timer_callback_function = platformTimerCallback,
	},
  {
		.timer_name = "1",
		.uxAutoReload = false,
		.xTimerPeriodInTicks = 5,
		.lock = false,
		.timer_callback_function = platformTimerCallback,
	},
  {
		.timer_name = "2",
		.uxAutoReload = false,
		.xTimerPeriodInTicks = 5,
		.lock = false,
		.timer_callback_function = platformTimerCallback,
	},
  {
		.timer_name = "3",
		.uxAutoReload = false,
		.xTimerPeriodInTicks = 5,
		.lock = false,
		.timer_callback_function = platformTimerCallback,
	},
  {
		.timer_name = "4",
		.uxAutoReload = false,
		.xTimerPeriodInTicks = 5,
		.lock = false,
		.timer_callback_function = platformTimerCallback,
	},
  {
		.timer_name = "5",
		.uxAutoReload = false,
		.xTimerPeriodInTicks = 5,
		.lock = false,
		.timer_callback_function = platformTimerCallback,
	},
  {
		.timer_name = "6",
		.uxAutoReload = false,
		.xTimerPeriodInTicks = 5,
		.lock = false,
		.timer_callback_function = platformTimerCallback,
	},
  {
		.timer_name = "7",
		.uxAutoReload = false,
		.xTimerPeriodInTicks = 5,
		.lock = false,
		.timer_callback_function = platformTimerCallback,
	},
  {
		.timer_name = "8",
		.uxAutoReload = false,
		.xTimerPeriodInTicks = 5,
		.lock = false,
		.timer_callback_function = platformTimerCallback,
	},
  {
		.timer_name = "9",
		.uxAutoReload = false,
		.xTimerPeriodInTicks = 5,
		.lock = false,
		.timer_callback_function = platformTimerCallback,
	},
};



SemaphoreHandle_t platformSem;      /*!< Default task wakeup Semaphore */

/**
  * @brief  Function implementing the Timer expiry callback
  * @param  argument: Elapsed timer handle
  * @retval None
  */
void platformTimerCallback(TimerHandle_t xTimer )
{
    xSemaphoreGive(platformSem);
}

/**
  * @brief  Function implementing Timer creation
  * @param  timer: timer value
  * @retval index (from 1 to PLF_TIMER_MAX) in the timer array.
  */
uint32_t plfTimerCreate(uint32_t time)
{
  uint32_t timer = UINT32_MAX;
  uint32_t i;
  BaseType_t rc;

  for( i = 0; i < PLF_TIMER_MAX; i++)
  {
    if( !timer_struct[i].lock )
    {
      timer = i;
      break;
    }
  }
  if( timer >= PLF_TIMER_MAX )
  {
    platformLog("platformTimerCreate: timer creation failed\r\n");
    plfErrorHandler();
    return 0;
  }

  rc = xTimerChangePeriod(timer_struct[timer].timer_handler, time + 1U, (TickType_t)0);
  if( rc == pdFAIL )
  {
    platformLog("platformTimerCreate: osTimerStart failed (rc = %d)\r\n", rc);
    plfErrorHandler();
    return 0;
  }
  timer_struct[i].lock = true;
  return timer + 1U;
}

/**
  * @brief  Function implementing Timer expiry check
  * @param  timer: timer index
  * @retval true: timer has expired, false otherwise
  */
bool plfTimerIsExpired(uint32_t timer)
{
  if( timer == 0U )
  {
    return true;
  }
  timer--;
  if( timer >= PLF_TIMER_MAX )
  {
    platformLog("platformTimerIsExpired: invalid timer\r\n");
    plfErrorHandler();
    return true;
  }
  return (xTimerIsTimerActive(timer_struct[timer].timer_handler) == pdFALSE);
}

/**
  * @brief  Function returning remaining time until expiration
  * @param  timer: timer index
  * @retval remaining time
  */
uint16_t plfTimerGetRemaining(uint32_t timer)
{
    if( timer == 0U )
    {
      return 0U;
    }
    timer--;
    if( (timer >= PLF_TIMER_MAX ) || !timer_struct[timer].lock )
    {
      platformLog("plateformTimerGetRemaining: invalid timer\r\n");
      plfErrorHandler();
      return 0U;
    }
    return ((uint16_t)(xTimerGetExpiryTime( timer_struct[timer].timer_handler ) - xTaskGetTickCount()));
}
/**
  * @brief  Function implementing Timer destruction
  * @param  timer: timer index
  * @retval none
  */
void plfTimerDestroy(uint32_t timer)
{
  if( timer == 0U )
  {
    return;
  }
  timer--;
  if( timer >= PLF_TIMER_MAX )
  {
    platformLog("platformTimerDestroy: invalid timer\r\n");
    plfErrorHandler();
    return;
  }
  xTimerStop(timer_struct[timer].timer_handler, (TickType_t)0);
  timer_struct[timer].lock = false;
}

/**
  * @brief  Function implementing Error handler
  * @param  none
  * @retval none
  */
void plfErrorHandler(void)
{
    while(1)
    {
    }
}

/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
static void nfc_default_handler_thread(void *thread_handler)
{
  bool status;
  uint32_t i = 6;
  platformLog("Welcome to FreeRTOS demo.\r\n");

  status = demoIni();
  platformLog("Initialization %s.\r\n", status ? "succeeded": "failed");
  while( (!status) || (status && (i-- > 0)) )
  {
    platformDelay(status ? 200U : 100U);
  }

  /* Infinite loop */
  for(;;)
  {
    demoCycle();

    xSemaphoreTake(platformSem, 10/portTICK_PERIOD_MS);
  }
}

/**
* @brief Function implementing the nfcIsrTask thread.
* @param argument: Not used
* @retval None
*/
static void nfc_irq_handler_thread(void *thread_handler)
{
  /* Infinite loop */
  for(;;)
  {
      ulTaskNotifyTake( pdTRUE, portMAX_DELAY  );             /* Check if IRQ happen execute immediately, otherwise other execute Task meanwhile */
      st25r200Isr();                                         /* ISR called from main task avoid calling vTaskSuspendAll from ISR (SPI) - NFC IRQ pin is anyhow checked */
      xSemaphoreGive(platformSem);
  }
}

/**
  * @brief  Function implementing the ST25R200 IRQ callback
  * @param  none
  * @retval none
  */
void BSP_SPI1_IRQ_Callback(void)
{
  static BaseType_t xHigherPriorityTaskWoken;

  xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(thread_struct[NFC_TASK_TYPE_IRQ].thread_handler, &xHigherPriorityTaskWoken );     /* Notify NFC Task to execute */
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );                            /* Request a context switch as soon as possible */
}

/**
  * @brief  Function implementing platform initialization
  * @param  argument: timer index
  * @retval none
  */
void plfInit(void)
{
	uint32_t i;

	/* Initialize default task wakeup semaphore */
	platformSem = xSemaphoreCreateCounting( 10, 0 );
	if( platformSem == NULL )
	{
		platformLog("Platform semaphore initialization failed\r\n");
		plfErrorHandler();
	}

//	/* Initialize timers */
//	for( i = 0; i < PLF_TIMER_MAX; i++ )
//	{
//		platformTimers[i].handle = xTimerCreateStatic(platformTimers[i].name, (TickType_t)1, pdFALSE, (void*) 0, platformTimerCallback, &platformTimers[i].timerBuf);
//		if( platformTimers[i].handle == NULL )
//		{
//			platformLog("Platform timers initialization failed\r\n");
//			plfErrorHandler();
//		}
//	}
  for(uint8_t i = 0;i <  PLF_TIMER_MAX; i++)
	{
		timer_struct[i].timer_handler = bc_rtos_timer_create(timer_struct[i].timer_name,
														  timer_struct[i].xTimerPeriodInTicks,
														  timer_struct[i].uxAutoReload, 
														   (void *)timer_struct[i].timer_id,
															timer_struct[i].timer_callback_function);
		if(timer_struct[i].timer_handler != NULL)
		{
			BC_LOG_INFO("create %s succeed\r\n",timer_struct[i].timer_name);
		}
		else
		{
			BC_LOG_ERROR("create %s fail\r\n",timer_struct[i].timer_name);
		}			
	}
}


