#ifndef __FML_SEM_H__
#define __FML_SEM_H__



typedef enum
{
	FML_ENUQUE_SEM = 0,
	FML_DEQUEUE_SEM,
	FML_RTOS_SEM_EVENT_NUM
}fml_rtos_sem_event_type;






void fml_rtos_sem_take(fml_rtos_sem_event_type  sem_event);
void fml_rtos_sem_give(fml_rtos_sem_event_type  sem_event);


void fml_sem_create(void);









#endif


