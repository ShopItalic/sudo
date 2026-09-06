#if defined(SUDO_VOICE_ONLY)
#include "app_error.h"
#endif
#include "bc_watchdog.h"



#include "bc_logger.h"
#include "bc_rtos.h"

#include "q_device.h"



static q_device_t *dog_handler;


enum bc_wdg_task
{
	WDG_TASK = 0,
	WDG_TASK_TYPE_NUM
};





static void bc_wdg_handler_thread(void *thread_handler);


static bc_rtos_thread_struct thread_struct[WDG_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "wdg handler",
                                                                      .thread_stack_depth   = BC_TASK_WDG_STACK_SIZE,
                                                                      .thread_priority      = APP_TASK_MIC_SED_PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = bc_wdg_handler_thread,
                                                                    },	                                                                     
                                                                                                                                       
                                                                 };










static void bc_wdg_handler_thread(void *thread_handler)
{
  while(true)
  {
    q_device_ctrl(dog_handler,WDT_FEED_DOG,0);	
    bc_rtos_delay(1000*30);
  }
}

static void bc_dog_task_create(void)
{

  bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < WDG_TASK_TYPE_NUM; i++)
	{
		x_return  = bc_rtos_thread_create((TaskFunction_t )thread_struct[i].thread_task_code,     	
                                     (const char*    )thread_struct[i].thread_name,   	
                                     (uint16_t       )thread_struct[i].thread_stack_depth, 
                                     (void*          )&thread_struct[i].thread_parameters,				
                                     (UBaseType_t    )thread_struct[i].thread_priority,	
                                     (TaskHandle_t*  )&thread_struct[i].thread_handler); 
		if(x_return == bc_pdPASS)
		{
			BC_LOG_INFO("create %s succeed \r\n",thread_struct[i].thread_name);
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",thread_struct[i].thread_name);
#if defined(SUDO_VOICE_ONLY)
            APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
#endif
		}	
	}
	
}

void bc_dog_feed(void)
{
	q_device_ctrl(dog_handler,WDT_FEED_DOG,0);	
}


void bc_dog_open(void)
{
	bc_dog_task_create();
	q_device_init(dog_handler);
	q_device_ctrl(dog_handler,WDT_FEED_DOG,0);	
}


void bc_dog_device_find(void)
{
	dog_handler = q_device_find("watchdog");
	q_device_assert(dog_handler);
	
	bc_dog_open();
}


