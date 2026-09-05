#include "bc_rtos.h"

void bc_rtos_thread_task_suspend(bc_rtos_thread_struct * thread_task)
{
	bc_rtos_thread_suspend(thread_task->thread_handler);
}


void bc_rtos_thread_task_resume(bc_rtos_thread_struct * thread_task)
{
	bc_rtos_thread_resume(thread_task->thread_handler);
}



void bc_rtos_thread_notify_give_from_isr(bc_rtos_thread_struct * thread_task,BaseType_t  TaskWoken)
{
    BaseType_t xHigherPriorityTaskWoken = TaskWoken;
   
    static uint32_t timerInterruptCount = 0;
    timerInterruptCount++;
    
    // 在中断中发送任务通知给显示任务
    vTaskNotifyGiveFromISR(thread_task->thread_handler, &xHigherPriorityTaskWoken);
      
    // 如果需要的话进行任务切换
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}















