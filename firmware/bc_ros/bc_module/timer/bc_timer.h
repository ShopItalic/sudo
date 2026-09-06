#ifndef __BC_TIMER_H__
#define __BC_TIMER_H__


#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"
#include "ring_config.h"


#if (HARDWARE_ARCH_TYPE_NORDIC == 1)



#define  BC_TIMER_MAX   45

// 用于将两个标识符连接成一个新的标识符
#define CONCAT(a, b) a ## b







#endif

#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)

#define  BC_TIMER_GROUP_MAX   3

#endif
	
typedef void(*bc_timer_callback)(void* contex);

struct bc_timer_info
{
	bool timer_lock;
	uint8_t task_id;
	uint16_t timer_id;
	bc_timer_callback timer_callback;
};

// timer
typedef struct 
{
	char            timer_name[30];
	bool            uxAutoReload;
	bool            lock;
	void           *timer_callback_function;
	uint32_t        xTimerPeriodInTicks;
	uint32_t       timer_id;
	struct bc_timer_info  timer_handler;	
}bc_timer_struct;



void bc_timer_init(uint8_t task_group_number);

bool bc_timer_create(bc_timer_struct *timer_struct);


bool bc_timer_start(bc_timer_struct *timer_struct);

bool bc_timer_stop(bc_timer_struct *timer_struct);


bool bc_rtos_timer_delete(bc_timer_struct *timer_struct);

uint16_t bc_timer_wrist_porc_event_fun0( uint8_t task_id, uint16_t events );
uint16_t bc_timer_wrist_porc_event_fun1( uint8_t task_id, uint16_t events );
uint16_t bc_timer_wrist_porc_event_fun2( uint8_t task_id, uint16_t events );



#endif




