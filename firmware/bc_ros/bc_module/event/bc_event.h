#ifndef __BC_EVENT_H__
#define __BC_EVENT_H__


#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"
#include "ring_config.h"


#if (HARDWARE_ARCH_TYPE_NORDIC == 1)


#define  BC_EVENT_MAX   45

#endif

#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)

#define  BC_EVENT_GROUP_MAX   3

#endif

typedef void(*bc_event_callback)(void* contex);

struct bc_event_info
{
	bool event_lock;
	uint8_t task_id;
	uint16_t event_id;
	uint16_t stack_depth;
	uint16_t priority;
	void* parameters;
	bc_event_callback event_callback;
};


// timer
typedef struct 
{
	char            event_name[30];
	bool            lock;
	void           *event_callback_function;
	struct bc_event_info  event_handler;	
}bc_event_struct;







void bc_event_init(uint8_t task_group_number);

bool bc_event_create(bc_event_struct *event_struct);

bool bc_event_set(bc_event_struct *event_struct);

bool bc_event_clear(bc_event_struct *event_struct);

uint16_t bc_wrist_porc_event_fun0( uint8_t task_id, uint16_t events );

uint16_t bc_wrist_porc_event_fun1( uint8_t task_id, uint16_t events );

uint16_t bc_wrist_porc_event_fun2( uint8_t task_id, uint16_t events );

void bc_event_poll(void);
















#endif



