#include "fml_queue.h"

#include "fml_freertos.h"
#include "fml_sem.h"

#include "log.h"
//#include "util.h"

#include "stdio.h"
#include "string.h"

//#include "hal_uart.h"
//#include "hal_can.h"


//static hal_uart_package uart_recv_queue_package;
//static hal_uart_package uart_send_queue_package;

//static hal_uart_package uart_rs485_recv_queue_package;
//static hal_uart_package uart_rs485_send_queue_package;

//static hal_can_package can_recv_package;
//static hal_can_package can_send_package;

static fml_rtos_queue_struct fml_ble_queue[FML_QUEUE_TYPE_NUM] ;//= {
//																																		{
//																																			.queue_name = "fml uart debug queue recv",
//																																			.queue_depth = 5,
//																																			.queue_buff = (uint8_t*)&uart_recv_queue_package,
//																																			.queue_buff_length = sizeof(uart_recv_queue_package),
//																																		},
//																																		{
//																																			.queue_name = "fml uart queue send",
//																																			.queue_depth = 5,
//																																			.queue_buff = (uint8_t*)&uart_send_queue_package,
//																																			.queue_buff_length = sizeof(uart_send_queue_package),
//																																		},
//																																		{
//																																			.queue_name = "fml uart rs485 queue recv",
//																																			.queue_depth = 5,
//																																			.queue_buff = (uint8_t*)&uart_rs485_recv_queue_package,
//																																			.queue_buff_length = sizeof(uart_rs485_recv_queue_package),
//																																		},
//																																		{
//																																			.queue_name = "fml uart rs485 queue send",
//																																			.queue_depth = 5,
//																																			.queue_buff = (uint8_t*)&uart_rs485_send_queue_package,
//																																			.queue_buff_length = sizeof(uart_rs485_send_queue_package),
//																																		},
//																																		{
//																																			.queue_name = "fml can recv queue",
//																																			.queue_depth = 5,
//																																			.queue_buff = (uint8_t*)&can_recv_package,
//																																			.queue_buff_length = sizeof(can_recv_package),
//																																		},																																		
//																															      {
//																																			.queue_name = "fml can send queue",
//																																			.queue_depth = 5,
//																																			.queue_buff = (uint8_t*)&can_send_package,
//																																			.queue_buff_length = sizeof(can_send_package),
//																																		},

//																																	};




/*******************************************************************************
 * Function Name     : fml_queue_isr_enqueue
 * Description       : 入队
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
static bool fml_queue_isr_enqueue(fml_queue_type queue_type,void *  enqueue_data)
{
	fml_base_type_t xReturn = fml_pdPASS;
	static signed fml_portBASE_TYPE  xHigherPriorityTaskWoken;	
	if(fml_ble_queue[queue_type].queue_count >= fml_ble_queue[queue_type].queue_depth)
	{
		return false;
	}
	xHigherPriorityTaskWoken = fml_pdFALSE;
	xReturn =fml_rtos_queue_isr_enqueue(fml_ble_queue[queue_type].queue_handler,(void *)enqueue_data,&xHigherPriorityTaskWoken);
	if(xReturn != fml_pdTRUE)
	{
		LOG_ERROR("%s fial\r\n",fml_ble_queue[queue_type].queue_name);
		return false;
	}
	fml_ble_queue[queue_type].queue_count++;
	fml_portYIELD_FROM_ISR( xHigherPriorityTaskWoken);
	return true;		
}

//static void fml_urart_recv_enqueue_callback_event_handler(hal_uart_package  * const uart_recv_package, uint32_t queue_event)
//{
//  
//	fml_queue_isr_enqueue((fml_queue_type)queue_event,uart_recv_package);
//}

//static void fml_can_recv_enqueue_callback_event_handler(hal_can_package  * const can_recv_package, uint32_t queue_event)
//{
//  
//	fml_queue_isr_enqueue((fml_queue_type)queue_event,can_recv_package);
//}


/*******************************************************************************
 * Function Name     : fml_queue_enqueue
 * Description       : 入队
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
bool fml_queue_enqueue(fml_queue_type queue_type,void *  enqueue_data)
{
	fml_rtos_sem_take(FML_ENUQUE_SEM );
	fml_base_type_t xReturn = fml_pdPASS;
	
	if(fml_ble_queue[queue_type].queue_count >= fml_ble_queue[queue_type].queue_depth)
	{
		fml_rtos_sem_give(FML_ENUQUE_SEM);
		return false;
	}

	xReturn = fml_rtos_queue_send(fml_ble_queue[queue_type].queue_handler,(void *)enqueue_data);
	if(xReturn != fml_pdPASS)
	{
		fml_rtos_sem_give(FML_ENUQUE_SEM);
		return false;
	}
	fml_ble_queue[queue_type].queue_count++;
	fml_rtos_sem_give(FML_ENUQUE_SEM);
	return true;		
}


/*******************************************************************************
 * Function Name     : fml_queue_dequeue
 * Description       : 出队
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
 bool fml_queue_dequeue(fml_queue_type queue_type,void * const pvBuffer)
{
	fml_base_type_t xReturn = fml_rtos_queue_receive(fml_ble_queue[queue_type].queue_handler,pvBuffer);
	if(xReturn != fml_pdPASS)
	{
		return false;
	}
	fml_ble_queue[queue_type].queue_count--;

//	LOG_DEBUG("queue dequeue ok\r\n");
	return true;
}






void fml_queue_init(void)
{
	for(uint8_t i=0;i<FML_QUEUE_TYPE_NUM;i++)
	{
		fml_ble_queue[i].queue_handler = fml_rtos_queue_create(fml_ble_queue[i].queue_depth,fml_ble_queue[i].queue_buff_length);
		if(fml_ble_queue[i].queue_handler != NULL)
		{
			LOG_INFO("create  %s succeed! \r\n",fml_ble_queue[i].queue_name);
		}
		else
		{
			LOG_ERROR("create %s fail!\r\n",fml_ble_queue[i].queue_name);
		}
	}
	
//	hal_uart_register_recv_enqueue_event_callback(HAL_DEBUG_UART_SERIAL,FML_QUEUE_TYPE_DEBUG_UART_RECV,fml_urart_recv_enqueue_callback_event_handler);
//	hal_uart_register_recv_enqueue_event_callback(HAL_RS485_UART_SERIAL,FML_QUEUE_TYPE_RS485_UART_RECV,fml_urart_recv_enqueue_callback_event_handler);

//	hal_can_register_recv_enqueue_event_callback(HAL_CAN_1_SERIAL,FML_QUEUE_TYPE_CAN_RECV,fml_can_recv_enqueue_callback_event_handler);
}





