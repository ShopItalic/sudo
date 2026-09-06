#include "user.h"

#include "bc_module.h"
#include "app.h"


#include "bc_rtos.h"
#include "bc_logger.h"
#include "test.h"



enum app_user_task
{
	USER_TASK_TYPE_INIT = 0,
	USER_TASK_TYPE_NUM
};

static void app_user_handler_thread(void *thread_handler);

static bc_rtos_thread_struct app_user_thread[USER_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "app test handler task",
                                                                      .thread_stack_depth   = APP_TASK_USER_STACK_SIZE ,
                                                                      .thread_priority      = APP_TASK_USER_PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = app_user_handler_thread,
                                                                    },																	
                                                                  };

static void app_user_handler_thread(void *thread_handler)
{
  while(true)
  {
    bc_rtos_taskENTER_CRITICAL();           //进入临界区
    bc_module_init();
//    app_init();
    test_timer_create();
    bc_rtos_thread_delete(app_user_thread[USER_TASK_TYPE_INIT].thread_handler);

    bc_rtos_taskEXIT_CRITICAL();            //退出临界区
  }
}

void app_user_handler_init(void)
{
	bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < USER_TASK_TYPE_NUM; i++)
	{
		x_return  = bc_rtos_thread_create((TaskFunction_t )app_user_thread[i].thread_task_code,     	
                                     (const char*    )app_user_thread[i].thread_name,   	
                                     (uint16_t       )app_user_thread[i].thread_stack_depth, 
                                     (void*          )&app_user_thread[i].thread_parameters,				
                                     (UBaseType_t    )app_user_thread[i].thread_priority,	
                                     (TaskHandle_t*  )&app_user_thread[i].thread_handler); 
		if(x_return == bc_pdPASS)
		{
			BC_LOG_INFO("create %s succeed \r\n",app_user_thread[i].thread_name);
      bc_rtos_thread_start_scheduler();
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",app_user_thread[i].thread_name);
		}	
	}
}
