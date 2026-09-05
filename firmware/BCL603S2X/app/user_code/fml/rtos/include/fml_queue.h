#ifndef __FML_QUEUE_H__
#define __FML_QUEUE_H__

#include "stdbool.h"
#include "stdint.h"

//#include "fml_ble_master.h"




typedef enum
{
	FML_QUEUE_TYPE_DEBUG_UART_RECV = 0,
  FML_QUEUE_TYPE_UART_SEND,
	FML_QUEUE_TYPE_RS485_UART_RECV,
	FML_QUEUE_TYPE_RS485_UART_SEND,
	FML_QUEUE_TYPE_CAN_RECV,
	FML_QUEUE_TYPE_CAN_SEND,
	FML_QUEUE_TYPE_NUM
}fml_queue_type;

typedef enum
{
	FML_QUEUE_UART_RECV_ENQUEUE_ISR = 0,
	FML_QUEUE_ENQUEUE_EVENT_NUM
}fml_queue_enqueue_event;


void fml_queue_init(void);
bool fml_queue_dequeue(fml_queue_type queue_type,void * const pvBuffer);

bool fml_queue_enqueue(fml_queue_type queue_type,void *  enqueue_data);



#endif
