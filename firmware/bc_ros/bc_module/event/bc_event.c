/*******************************************************************************
此为bc事件文件，通过宏定义来兼容nordic、phy6222硬件平台

日  期：2024年1月17日
编写人：邱成凯
 *******************************************************************************/
#include "bc_event.h"

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)

#include "q_queue.h"
#include "stdio.h"


QUEUE_HandleTypeDef event_queue_handler = {0};
static QUEUE_DATA_T event_queue_buff[BC_EVENT_MAX];

static struct bc_event_info event_info[BC_EVENT_MAX];


static bool event_check(struct bc_event_info *event)
{
	for(uint8_t i = 0; i < BC_EVENT_MAX;i++)
	{
		if(!event_info[i].event_lock)
		{
			event_info[i].event_lock = true;
			event->event_id = i;
			event->event_lock = event_info[i].event_lock;
			return true;
		}
		
	}
	return false;
}

/*******************************************************************************
 * Function Name     : bc_event_init
 * Description       : 初始化事件
 * Input             : task_group_number  写0
 * Output            : 
 * Return            : 
 *******************************************************************************/
void bc_event_init(uint8_t task_group_number)
{
	q_queue_init(&event_queue_handler, event_queue_buff, BC_EVENT_MAX);
	return;
}
/*******************************************************************************
 * Function Name     : bc_event_create
 * Description       : 创建事件
 * Input             : event_struct  事件结构体指针
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
bool bc_event_create(bc_event_struct *event_struct)
{
	if(event_struct->event_callback_function == NULL)
	{
		return false;
	}
	if(!event_check(&event_struct->event_handler))
	{		
		return false;
	}
	event_info[event_struct->event_handler.event_id].event_callback = (bc_event_callback)event_struct->event_callback_function;
	return true;
}
/*******************************************************************************
 * Function Name     : bc_event_set
 * Description       : 发送事件
 * Input             : event_struct  事件结构体指针
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
bool bc_event_set(bc_event_struct *event_struct)
{
	if(!event_struct->event_handler.event_lock || !event_info[event_struct->event_handler.event_id].event_lock)
	{
		return false;
	}
	__disable_irq();
	q_queue_push(&event_queue_handler, event_struct->event_handler.event_id);
	__enable_irq();
	// send event
	return true;
}
/*******************************************************************************
 * Function Name     : bc_event_clear
 * Description       : 清除事件
 * Input             : event_struct  事件结构体指针
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
bool bc_event_clear(bc_event_struct *event_struct)
{
	if(!event_struct->event_handler.event_lock || !event_info[event_struct->event_handler.event_id].event_lock)
	{
		return false;
	}
	event_info[event_struct->event_handler.event_id].event_lock = false;
	event_info[event_struct->event_handler.event_id].event_callback = NULL;
	event_struct->event_handler.event_id = 0;
	event_struct->event_handler.event_lock = false;
	return true;
}


void bc_event_poll(void)
{
	
	while(q_queue_count(&event_queue_handler) != 0)
	{
		
		uint8_t event_index = 0;
		__disable_irq();
		if(q_queue_pop(&event_queue_handler, &event_index) == QUEUE_OK)
		{
			
			__enable_irq();
			if(event_info[event_index].event_lock && event_info[event_index].event_callback != NULL)
			{
			
				event_info[event_index].event_callback(NULL);
			}
		}
		__enable_irq();
//		printf("event_index:%d\r\n",event_index);
	}
	
}

#endif





#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)

#include "OSAL.h"

struct bc_event_buff
{
	struct bc_event_info bc_event[15];
};

static struct bc_event_buff event_group[BC_EVENT_GROUP_MAX] = {0};


static bool event_check(struct bc_event_info *event_info)
{
	for(uint8_t i = 0; i < BC_EVENT_GROUP_MAX;i++)
	{
		for(uint8_t j = 0; j < 15; j++)
		{
			if(!event_group[i].bc_event[j].event_lock)
			{
				event_group[i].bc_event[j].event_lock = true;
				event_info->task_id = i;
				event_info->event_id = j;
				event_info->event_lock = event_group[i].bc_event[j].event_lock;
				return true;
			}
		}
	}
	return false;
}




static uint16_t event_callback_handler(uint8_t task_id,uint16_t event_id)
{
	uint8_t task_id_index = 0;
	for(uint8_t i = 0; i < BC_EVENT_GROUP_MAX;i++)
	{
		if(event_group[i].bc_event[0].task_id == task_id)
		{
			task_id_index = i;
		}
	}

	for(uint8_t i = 0; i < 15;i++)
	{

		if((event_id & event_group[task_id_index].bc_event[i].event_id) && event_group[task_id_index].bc_event[i].event_lock)
		{
			if(event_group[task_id_index].bc_event[i].event_callback != NULL)
			{
				event_group[task_id_index].bc_event[i].event_callback(NULL);
				return (event_id ^ event_group[task_id_index].bc_event[i].event_id);
			}
		}
		
	}
	return 0;
}

/*******************************************************************************
 * Function Name     : bc_event_init
 * Description       : 初始化事件
 * Input             : task_group_number  task id
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
void bc_event_init(uint8_t task_group_number)
{
	
	for(uint8_t i = 0; i < BC_EVENT_GROUP_MAX;i++)
	{
		event_group[i].bc_event[0].event_id = 1;
		event_group[i].bc_event[0].task_id = task_group_number+ i;
		for(uint8_t j = 1; j < 15; j++)
		{
			event_group[i].bc_event[j].event_id = event_group[i].bc_event[j-1].event_id *2;
			event_group[i].bc_event[j].task_id = task_group_number + i;
		}
	}
}
/*******************************************************************************
 * Function Name     : bc_event_create
 * Description       : 创建事件
 * Input             : event_struct  事件结构体指针
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
bool bc_event_create(bc_event_struct *event_struct)
{
	if(event_struct->event_callback_function == NULL )
	{
		return false;
	}
	if(!event_check(&event_struct->event_handler))
	{		
		return false;
	}
	event_group[event_struct->event_handler.task_id].bc_event[event_struct->event_handler.event_id].event_callback = (bc_event_callback)event_struct->event_callback_function;
	return true;
}

/*******************************************************************************
 * Function Name     : bc_event_set
 * Description       : 发送事件
 * Input             : event_struct  事件结构体指针
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
bool bc_event_set(bc_event_struct *event_struct)
{
	if(!event_struct->event_handler.event_lock || !event_group[event_struct->event_handler.task_id].bc_event[event_struct->event_handler.event_id].event_lock)
	{
		return false;
	}
	osal_set_event(event_group[event_struct->event_handler.task_id].bc_event[event_struct->event_handler.event_id].task_id,
	                event_group[event_struct->event_handler.task_id].bc_event[event_struct->event_handler.event_id].event_id);
	return true;
}
/*******************************************************************************
 * Function Name     : bc_event_clear
 * Description       : 清除事件
 * Input             : event_struct  事件结构体指针
 * Output            : 
 * Return            : true 成功；false 失败
 *******************************************************************************/
bool bc_event_clear(bc_event_struct *event_struct)
{
	if(!event_struct->event_handler.event_lock || !event_group[event_struct->event_handler.task_id].bc_event[event_struct->event_handler.event_id].event_lock)
	{
		return false;
	}
	osal_clear_event(event_group[event_struct->event_handler.task_id].bc_event[event_struct->event_handler.event_id].task_id,event_group[event_struct->event_handler.task_id].bc_event[event_struct->event_handler.event_id].event_id);
	event_group[event_struct->event_handler.task_id].bc_event[event_struct->event_handler.event_id].event_lock = false;
	event_group[event_struct->event_handler.task_id].bc_event[event_struct->event_handler.event_id].event_callback = NULL;
	return true;
}


uint16_t bc_wrist_porc_event_fun0( uint8_t task_id, uint16_t events )
{
	return event_callback_handler(task_id,events);
}

uint16_t bc_wrist_porc_event_fun1( uint8_t task_id, uint16_t events )
{
	return event_callback_handler(task_id,events);
}

uint16_t bc_wrist_porc_event_fun2( uint8_t task_id, uint16_t events )
{
	return event_callback_handler(task_id,events);
}




#endif









