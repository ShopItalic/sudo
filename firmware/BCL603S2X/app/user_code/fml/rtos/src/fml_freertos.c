#include "fml_freertos.h"


void fml_rtos_thread_task_suspend(fml_rtos_thread_struct * thread_task)
{
	fml_rtos_thread_suspend(thread_task->thread_handler);
}


void fml_rtos_thread_task_resume(fml_rtos_thread_struct * thread_task)
{
	fml_rtos_thread_resume(thread_task->thread_handler);
}

