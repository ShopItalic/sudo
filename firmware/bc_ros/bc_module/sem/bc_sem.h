#ifndef __BC_SEM_H__
#define __BC_SEM_H__

#include <stdbool.h>

typedef enum
{
	BC_ENUQUE_SEM = 0,
	BC_DEQUEUE_SEM,
	BC_PPG_STOP_SEM,
  BC_SFUD_SEM,
  BC_HARDLINE_TSDB_SEM,
    BC_LOGGER_SEM,
	BC_RTOS_SEM_EVENT_NUM
}bc_rtos_sem_event_type;


typedef enum
{
	BC_PPG_IRQ_SEM_COUNT = 0,
  BC_MIC_ISR_SEM_COUNT,
  BC_TOUCH_ISR_SEM_COUNT,
	BC_RTOS_SEM_COUNT_EVENT_NUM
}bc_rtos_sem_count_event_type;



bool bc_rtos_sem_take(bc_rtos_sem_event_type  sem_event);
bool bc_rtos_sem_give(bc_rtos_sem_event_type  sem_event);
bool bc_rtos_sem_give_isr(bc_rtos_sem_event_type  sem_event);

bool bc_rtos_sem_count_take(bc_rtos_sem_count_event_type  sem_event);
bool bc_rtos_sem_count_give(bc_rtos_sem_count_event_type  sem_event);
bool bc_rtos_sem_count_give_isr(bc_rtos_sem_count_event_type  sem_event);

void bc_sem_create(void);





#endif


