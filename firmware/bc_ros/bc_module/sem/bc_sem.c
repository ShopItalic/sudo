#include "bc_sem.h"
#include "bc_rtos.h"

#include "bc_logger.h"



static bc_rtos_sem_struct  sem_handler[BC_RTOS_SEM_EVENT_NUM]  = {
    {
        .sem_name = " enqueue sem",
        .sem_handler = NULL,
    },
    {
        .sem_name = " dequeue sem",
        .sem_handler = NULL,
    },
    {
        .sem_name = " ppg stop sem",
        .sem_handler = NULL,
    },
    {
        .sem_name = " sfud sem",
        .sem_handler = NULL,
    },
    {
        .sem_name = " hardlint tsdb sem",
        .sem_handler = NULL,
    },
    {
        .sem_name = " logger sem",
        .sem_handler = NULL,
    },
};


static bc_rtos_sem_count_struct  sem_count_struct[BC_RTOS_SEM_COUNT_EVENT_NUM] = {
	{
		.sem_name = " ppg irq sem",
    .sem_handler = NULL,
		.max_count = 10,
		.initial_count = 0,
	},
  {
		.sem_name = " mic irq sem",
    .sem_handler = NULL,
		.max_count = 20,
		.initial_count = 0,
	},
  {
		.sem_name = " touch irq sem",
    .sem_handler = NULL,
		.max_count = 10,
		.initial_count = 0,
	},
};

bool bc_rtos_sem_take(bc_rtos_sem_event_type  sem_event)
{
	return bc_rtos_semaphore_take(sem_handler[sem_event].sem_handler);
}

bool bc_rtos_sem_give(bc_rtos_sem_event_type  sem_event)
{
	return bc_rtos_semaphore_give(sem_handler[sem_event].sem_handler);
}

bool bc_rtos_sem_give_isr(bc_rtos_sem_event_type  sem_event)
{
    BaseType_t xHigherPriorityTaskWoken = pdTRUE;
    
	BaseType_t xreturn = xSemaphoreGiveFromISR(sem_handler[sem_event].sem_handler, &xHigherPriorityTaskWoken);
    // 释放信号量通知任务
    if( xreturn == pdPASS)
    {
        // 信号量释放成功
    }
    
    // 如果有更高优先级任务被唤醒，请求上下文切换
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	return  xreturn;
}
	
bool bc_rtos_sem_count_take(bc_rtos_sem_count_event_type  sem_event)
{
	return bc_rtos_semaphore_count_take(sem_count_struct[sem_event].sem_handler);
}

bool bc_rtos_sem_count_give(bc_rtos_sem_count_event_type  sem_event)
{
	return bc_rtos_semaphore_count_give(sem_count_struct[sem_event].sem_handler);
}

bool bc_rtos_sem_count_give_isr(bc_rtos_sem_count_event_type  sem_event)
{
    BaseType_t xHigherPriorityTaskWoken = pdTRUE;
    
	BaseType_t xreturn  = xSemaphoreGiveFromISR(sem_count_struct[sem_event].sem_handler, &xHigherPriorityTaskWoken);
    // 释放信号量通知任务
    if(xreturn == pdPASS)
    {
        // 信号量释放成功
    }
    
    // 如果有更高优先级任务被唤醒，请求上下文切换
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	return  xreturn;
}


void bc_sem_create(void)
{
	for(uint8_t i = 0;i < BC_RTOS_SEM_EVENT_NUM; i++)
	{
		sem_handler[i].sem_handler = bc_rtos_sem_create();
		if(sem_handler[i].sem_handler != NULL)
		{
			BC_LOG_INFO("create %s succeed \r\n",sem_handler[i].sem_name);
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",sem_handler[i].sem_name);
		}	
	}
	
	for(uint8_t i = 0;i < BC_RTOS_SEM_COUNT_EVENT_NUM; i++)
	{
		sem_count_struct[i].sem_handler = bc_rtos_semaphore_create_counting(sem_count_struct[i].max_count,sem_count_struct[i].initial_count);
		if(sem_count_struct[i].sem_handler != NULL)
		{
			BC_LOG_INFO("create %s succeed \r\n",sem_count_struct[i].sem_name);
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",sem_count_struct[i].sem_name);
		}	
	}
}




















