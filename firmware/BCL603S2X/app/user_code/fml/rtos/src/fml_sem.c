#include "fml_sem.h"


#include "fml_freertos.h"

#include "log.h"



static fml_rtos_sem_struct  sem_handler[FML_RTOS_SEM_EVENT_NUM]  = {
																																			{
																																				.sem_name = "fml enqueue sem",
																																				.sem_handler = NULL,
																																			},
																																			{
																																				.sem_name = "fml dequeue sem",
																																				.sem_handler = NULL,
																																			},
																																			
																																		};




void fml_rtos_sem_take(fml_rtos_sem_event_type  sem_event)
{
	fml_rtos_semaphore_take(sem_handler[sem_event].sem_handler);
}

void fml_rtos_sem_give(fml_rtos_sem_event_type  sem_event)
{
	fml_rtos_semaphore_give(sem_handler[sem_event].sem_handler);
}
	



void fml_sem_create(void)
{
	for(uint8_t i = 0;i < FML_RTOS_SEM_EVENT_NUM; i++)
	{
		sem_handler[i].sem_handler = fml_rtos_sem_create();
		if(sem_handler[i].sem_handler != NULL)
		{
			LOG_INFO("create %s succeed \r\n",sem_handler[i].sem_name);
		}
		else
		{
			LOG_ERROR("create  %s fail",sem_handler[i].sem_name);
		}	
	}
}





















