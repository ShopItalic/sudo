/*******************************************************************************
此为bc定时器文件，通过宏定义来兼容nordic、phy6222硬件平台

日  期：2024年1月17日
编写人：邱成凯
 *******************************************************************************/

#include "bc_timer.h"

#include "string.h"



#if (HARDWARE_ARCH_TYPE_NORDIC == 1)

#include "app_timer.h"



static uint8_t timer_num_count = 0;
struct timer_struct
{
	bool timer_lock;
	app_timer_id_t timer_handler;
};
static struct timer_struct timer_info[BC_TIMER_MAX] = {0};


/*******************************************************************************
 * Function Name     : bc_timer_init
 * Description       : 初始化定时器
 * Input             : task_group_number 写0
 * Output            : 
 * Return            : 
 *******************************************************************************/
void bc_timer_init(uint8_t task_group_number)
{

	//此处为无脑垃圾代码，由于APP_TIMER_DEF必须传入字符（char *c = "adc",char c[3],都不可以）
	APP_TIMER_DEF(CONCAT(timer, 0));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 0);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 1));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 1);
	timer_num_count++;

	APP_TIMER_DEF(CONCAT(timer, 2));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 2);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 3));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 3);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 4));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 4);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 5));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 5);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 6));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 6);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 7));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 7);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 8));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 8);
	timer_num_count++;
	
	
	APP_TIMER_DEF(CONCAT(timer, 9));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 9);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 10));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 10);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 11));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 11);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 12));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 12);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 13));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 13);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 14));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 14);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 15));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 15);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 16));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 16);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 17));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 17);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 18));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 18);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 19));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 19);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 20));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 20);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 21));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 21);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 22));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 22);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 23));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 23);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 24));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 24);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 25));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 25);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 26));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 26);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 27));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 27);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 28));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 28);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 29));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 29);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 30));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 30);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 31));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 31);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 32));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 32);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 33));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 33);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 34));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 34);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 35));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 35);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 36));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 36);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 37));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 37);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 38));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 38);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 39));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 39);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 40));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 40);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 41));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 41);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 42));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 42);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 43));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 43);
	timer_num_count++;
	
	APP_TIMER_DEF(CONCAT(timer, 44));
	timer_info[timer_num_count].timer_handler = CONCAT(timer, 44);
	timer_num_count++;
	
	timer_num_count = 0;
	


}

/*******************************************************************************
 * Function Name     : bc_rtos_timer_create
 * Description       : 创建定时器
 * Input             : timer_struct  定时器结构体指针
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
bool bc_timer_create(bc_timer_struct *timer_struct)
{
	if(timer_struct->timer_callback_function == NULL || timer_struct->xTimerPeriodInTicks < 5)
	{
		return false;
	}
	
	ret_code_t     err_code;
	for(uint8_t i = 0; i < BC_TIMER_MAX;i++)
	{
		if(!timer_info[i].timer_lock)
		{
			if(timer_struct->uxAutoReload)
			{
				err_code = app_timer_create(&timer_info[i].timer_handler,
										APP_TIMER_MODE_REPEATED,
										(app_timer_timeout_handler_t)timer_struct->timer_callback_function);
			}
			else
			{
				err_code = app_timer_create(&timer_info[i].timer_handler,
										APP_TIMER_MODE_SINGLE_SHOT,
										(app_timer_timeout_handler_t)timer_struct->timer_callback_function);
			}
			APP_ERROR_CHECK(err_code);
			timer_info[i].timer_lock = true;
			timer_struct->timer_handler.timer_id = i;
			timer_struct->timer_handler.timer_lock = timer_info[i].timer_lock;
//			printf("test lll timer_struct->timer_handler.timer_id:%d \r\n",timer_struct->timer_handler.timer_id);
			return true;
		}
	}
	return false;	
}

/*******************************************************************************
 * Function Name     : fml_rtos_timer_start
 * Description       : 启动定时器
 * Input             : timer_struct  定时器结构体指针
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
bool bc_timer_start(bc_timer_struct *timer_struct)
{
	ret_code_t err_code;
	if(!timer_info[timer_struct->timer_handler.timer_id].timer_lock)
	{
		return false;
	}
//	printf("test timer_struct->timer_handler.timer_id:%d \r\n",timer_struct->timer_handler.timer_id);
	err_code = app_timer_start(timer_info[timer_struct->timer_handler.timer_id].timer_handler,APP_TIMER_TICKS(timer_struct->xTimerPeriodInTicks),NULL);
	APP_ERROR_CHECK(err_code);
	return true;
}

/*******************************************************************************
 * Function Name     : fml_rtos_timer_stop
 * Description       : 停止定时器
 * Input             : timer_struct  定时器结构体指针
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
bool bc_timer_stop(bc_timer_struct *timer_struct)
{
	ret_code_t err_code;
	if(!timer_struct->timer_handler.timer_lock || !timer_info[timer_num_count].timer_lock)
	{
		return false;
	}	
	err_code = app_timer_stop(timer_info[timer_struct->timer_handler.timer_id].timer_handler);
    APP_ERROR_CHECK(err_code);
	return true;
	
}

/*******************************************************************************
 * Function Name     : fml_rtos_timer_delete
 * Description       : 删除定时器
 * Input             : timer_struct  定时器结构体指针
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
bool bc_timer_delete( bc_timer_struct *timer_struct)
{
	if(!timer_struct->timer_handler.timer_lock || !timer_info[timer_num_count].timer_lock)
	{
		return false;
	}
	timer_info[timer_struct->timer_handler.timer_id].timer_lock = false;
	timer_struct->timer_handler.timer_id = 0;
	timer_struct->timer_handler.timer_lock = false;
	return true;
}


#endif


#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)

#include "OSAL.h"

struct bc_timer_buff
{
	struct bc_timer_info bc_timer[15];
};

static struct bc_timer_buff timer_group[BC_TIMER_GROUP_MAX] = {0};

static bool timer_check(struct bc_timer_info *timer_info)
{
	for(uint8_t i = 0; i < BC_TIMER_GROUP_MAX;i++)
	{
		for(uint8_t j = 0; j < 15; j++)
		{
			if(!timer_group[i].bc_timer[j].timer_lock)
			{
				timer_group[i].bc_timer[j].timer_lock = true;
				timer_info->task_id = i;
				timer_info->timer_id = j;
				timer_info->timer_lock = timer_group[i].bc_timer[j].timer_lock;
				return true;
			}
		}
	}
	return false;
}

static uint16_t timer_timeout_callback_handler(uint8_t task_id,uint16_t timer_id)
{
	uint8_t task_id_index = 0;
	for(uint8_t i = 0; i < BC_TIMER_GROUP_MAX;i++)
	{
		if(timer_group[i].bc_timer[0].task_id == task_id)
		{
			task_id_index = i;
		}
	}

	for(uint8_t i = 0; i < 15;i++)
	{

		if((timer_id & timer_group[task_id_index].bc_timer[i].timer_id) && timer_group[task_id_index].bc_timer[i].timer_lock)
		{
			if(timer_group[task_id_index].bc_timer[i].timer_callback != NULL)
			{
				timer_group[task_id_index].bc_timer[i].timer_callback(NULL);
				return (timer_id ^ timer_group[task_id_index].bc_timer[i].timer_id);
			}
		}
		
	}
	return 0;
}

/*******************************************************************************
 * Function Name     : bc_timer_init
 * Description       : 初始化定时器
 * Input             : task_group_number task id
 * Output            : 
 * Return            : 
 *******************************************************************************/
void bc_timer_init(uint8_t task_group_number)
{
	
	for(uint8_t i = 0; i < BC_TIMER_GROUP_MAX;i++)
	{
		timer_group[i].bc_timer[0].timer_id = 1;
		timer_group[i].bc_timer[0].task_id = task_group_number+ i;
		for(uint8_t j = 1; j < 15; j++)
		{
			timer_group[i].bc_timer[j].timer_id = timer_group[i].bc_timer[j-1].timer_id *2;
			timer_group[i].bc_timer[j].task_id = task_group_number + i;
		}
	}
}



/*******************************************************************************
 * Function Name     : bc_rtos_timer_create
 * Description       : 创建定时器
 * Input             : timer_struct  定时器结构体指针
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
bool bc_timer_create(bc_timer_struct *timer_struct)
{
	if(timer_struct->timer_callback_function == NULL || timer_struct->xTimerPeriodInTicks < 5)
	{
		return false;
	}
	if(!timer_check(&timer_struct->timer_handler))
	{		
		return false;
	}
	timer_group[timer_struct->timer_handler.task_id].bc_timer[timer_struct->timer_handler.timer_id].timer_callback = (bc_timer_callback)timer_struct->timer_callback_function;
	return true;
}

/*******************************************************************************
 * Function Name     : bc_rtos_timer_start
 * Description       : 启动定时器
 * Input             : timer_struct  定时器结构体指针
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
bool bc_timer_start(bc_timer_struct *timer_struct)
{
	if(!timer_struct->timer_handler.timer_lock || !timer_group[timer_struct->timer_handler.task_id].bc_timer[timer_struct->timer_handler.timer_id].timer_lock)
	{
		return false;
	}
	
	if(timer_struct->uxAutoReload)
	{
		osal_start_reload_timer(timer_group[timer_struct->timer_handler.task_id].bc_timer[timer_struct->timer_handler.timer_id].task_id,
								timer_group[timer_struct->timer_handler.task_id].bc_timer[timer_struct->timer_handler.timer_id].timer_id,
								timer_struct->xTimerPeriodInTicks);
		
	}
	else
	{
		osal_start_timerEx(timer_group[timer_struct->timer_handler.task_id].bc_timer[timer_struct->timer_handler.timer_id].task_id,
							timer_group[timer_struct->timer_handler.task_id].bc_timer[timer_struct->timer_handler.timer_id].timer_id,
							timer_struct->xTimerPeriodInTicks);
	}
	return true;
}

/*******************************************************************************
 * Function Name     : bc_rtos_timer_stop
 * Description       : 停止定时器
 * Input             : timer_struct  定时器结构体指针
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
bool bc_timer_stop(bc_timer_struct *timer_struct)
{
	if(!timer_struct->timer_handler.timer_lock || !timer_group[timer_struct->timer_handler.task_id].bc_timer[timer_struct->timer_handler.timer_id].timer_lock)
	{
		return false;
	}	

	osal_stop_timerEx(timer_group[timer_struct->timer_handler.task_id].bc_timer[timer_struct->timer_handler.timer_id].task_id,
							timer_group[timer_struct->timer_handler.task_id].bc_timer[timer_struct->timer_handler.timer_id].timer_id);
	return true;
	
}


/*******************************************************************************
 * Function Name     : bc_rtos_timer_delete
 * Description       : 删除定时器
 * Input             : timer_struct  定时器结构体指针
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
bool bc_timer_delete( bc_timer_struct *timer_struct)
{
  if(!timer_group[timer_struct->timer_handler.task_id].bc_timer[timer_struct->timer_handler.timer_id].timer_lock)
  {
	  return false;
  }
  timer_group[timer_struct->timer_handler.task_id].bc_timer[timer_struct->timer_handler.timer_id].timer_lock = false;
  timer_group[timer_struct->timer_handler.task_id].bc_timer[timer_struct->timer_handler.timer_id].timer_callback = NULL;
  memset((uint8_t*)&timer_struct->timer_handler,0,sizeof(struct bc_timer_info));
  return true;


}


uint16_t bc_timer_wrist_porc_event_fun0( uint8 task_id, uint16 events )
{
	return timer_timeout_callback_handler(task_id,events);
}

uint16_t bc_timer_wrist_porc_event_fun1( uint8 task_id, uint16 events )
{
	return timer_timeout_callback_handler(task_id,events);
}

uint16_t bc_timer_wrist_porc_event_fun2( uint8 task_id, uint16 events )
{
	return timer_timeout_callback_handler(task_id,events);
}


#endif

