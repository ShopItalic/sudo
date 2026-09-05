/*******************************************************************************
此为bc队列管理文件，通过宏定义来兼容nordic、phy6222硬件平台，遵循q_queue api接口规则

日  期：2024年1月24日
编写人：邱成凯
 *******************************************************************************/

#include "bc_queue.h"


#include "bc_ble_modu_interface.h"
#include "bc_logger.h"
#include "bc_ppg.h"
#include "bc_pdm.h"

#include <stdlib.h>

#if !defined(HANDWARE_1_23_4)
#include "app_six_axis_sensor_handler.h"
#endif

#include "bc_rtos.h"
#include "bc_sem.h"

#if (defined(HANDWARE_1_23_3 ) || defined(HANDWARE_1_23_4))
#include "bc_linear_motor_ic.h"
#endif


static bc_rtos_queue_struct bc_queue[BC_QUEUE_TYPE_NUM] = {
                                                            {
                                                              .queue_name = "ble queue recv",
                                                              .queue_depth = 5,
                                                              .queue_count = 0,
                                                              .queue_buff_length = sizeof(struct bc_ble_data_package),
																												  	},
                                                            {
                                                              .queue_name = "ble queue send",
                                                              .queue_depth = 10,
                                                              .queue_count = 0,
                                                              .queue_buff_length = sizeof(struct bc_ble_data_package) ,
																												  	},                                                         
                                                            {
                                                              .queue_name = "ppg collection hr data",
                                                              .queue_count = 0,
#if ( HARDWARE_1191_ENABLED || HARDWARE_1231_ENABLED)                                                              
                                                              .queue_depth = 1,
                                                              .queue_buff_length = sizeof(struct  bc_ppg_collection_hr_data) ,
#else
                                                              .queue_depth = 10,
                                                              .queue_buff_length = sizeof(struct  bc_ppg_collection_hr_data) ,
#endif                                                                
                                                            },
                                                            {
                                                              .queue_name = "ppg collection spo2 data",
                                                              .queue_count = 0,
#if ( HARDWARE_1191_ENABLED || HARDWARE_1171_ENABLED || HARDWARE_1231_ENABLED)                                                              
                                                              .queue_depth = 1,
                                                              .queue_buff_length = sizeof(struct  bc_ppg_collection_hr_data),
#else
                                                              .queue_depth = 15,
                                                              .queue_buff_length = sizeof(struct  bc_ppg_collection_hr_data) ,
#endif
                                                            },
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED || HARDWARE_1121_ENABLED || HARDWARE_158_ENABLED == 1 || HARDWARE_1181_ENABLED || HARDWARE_1171_ENABLED || HARDWARE_1191_ENABLED\
     || HARDWARE_1231_ENABLED)	
                                                            {
                                                              .queue_name = "pdm data",
                                                              .queue_count = 0,
                                                              
#if ( HARDWARE_1191_ENABLED)                                                              
                                                              .queue_depth =100,
                                                              .queue_buff_length = sizeof(struct bc_adpcm_package ),
#elif ( HARDWARE_1231_ENABLED)    

                                                              .queue_depth =100,
                                                              .queue_buff_length = sizeof(struct bc_adpcm_package ),
                                                              
#elif ( HARDWARE_1171_ENABLED)      
                                                              .queue_depth =20*20,
                                                              .queue_buff_length = sizeof(struct bc_adpcm_package ),
#else
                                                              .queue_depth =5,
                                                              .queue_buff_length = sizeof(struct bc_adpcm_package ),
#endif                                                              
                                                              
                                                            },

#endif														
#if !defined(HANDWARE_1_23_4)
                                                            {
                                                              .queue_name = "imu data",
                                                              .queue_count = 0,
                                                              .queue_depth = 10,
                                                              .queue_buff_length = sizeof(struct imu_sensor_package),
                                                            },
#endif
#if (HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 )
                                                            {
                                                              .queue_name = "ppg collection spo2 hr data",
                                                              .queue_count = 0,
                                                              .queue_depth = sizeof(ppg_collection_spo2_hr_data)/sizeof(struct bc_ppg_collection_spo2_data ),
                                                              .queue_buff = (uint8_t*)&ppg_collection_spo2_hr_data,
                                                              .queue_buff_length = sizeof(ppg_collection_spo2_hr_data),
                                                            },
 #endif		
#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
                                                            {
                                                              .queue_name = "app iic1 handle data",
                                                              .queue_count = 0,
                                                              .queue_depth = 10,
                                                              .queue_buff_length = sizeof(STR_IIC1_DATA),
                                                            },
 #endif		
														
};

													 

													 

/*******************************************************************************
 * Function Name     : bc_queue_isr_enqueue
 * Description       : 入队
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
 bool bc_queue_isr_enqueue(bc_queue_type queue_type,void *  enqueue_data)
{
	bc_base_type_t xReturn = bc_pdPASS;
	signed bc_long  xHigherPriorityTaskWoken;	
	if(bc_queue[queue_type].queue_count >= bc_queue[queue_type].queue_depth)
	{
        //BC_LOG_ERROR("queue enqueue fial, queue type:%d  queue_count:%d \r\n",queue_type,bc_queue[queue_type].queue_count);
		return false;
	}
    //BC_LOG_INFO("queue enqueue isr ok, queue type:%d  queue_count:%d enqueue_count:%d\r\n",queue_type,bc_queue[queue_type].queue_count,bc_queue[queue_type].enqueue_count);
	xHigherPriorityTaskWoken = bc_pdFALSE;
	xReturn =bc_rtos_queue_isr_enqueue(bc_queue[queue_type].queue_handler,(void *)enqueue_data,&xHigherPriorityTaskWoken);
	if(xReturn != bc_pdTRUE)
	{
		//BC_LOG_ERROR("%s fial\r\n",bc_queue[queue_type].queue_name);
		return false;
	}
    UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    if(bc_queue[queue_type].queue_count < 0xff)
        bc_queue[queue_type].queue_count++;
  bc_queue[queue_type].enqueue_count++;
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
  //printf("queue enqueue ok, queue type:%d   count:%d enqueue_count:%d\r\n",queue_type,bc_queue[queue_type].queue_count,bc_queue[queue_type].enqueue_count);
	bc_portYIELD_FROM_ISR( xHigherPriorityTaskWoken);
	return true;		
}

bool bc_queue_isr_enqueue_not_yield(bc_queue_type queue_type,void *  enqueue_data)
{
	bc_base_type_t xReturn = bc_pdPASS;
	signed bc_long  xHigherPriorityTaskWoken;	
	if(bc_queue[queue_type].queue_count >= bc_queue[queue_type].queue_depth)
	{
        //BC_LOG_ERROR("queue enqueue fial, queue type:%d  queue_count:%d \r\n",queue_type,bc_queue[queue_type].queue_count);
		return false;
	}
    //BC_LOG_INFO("queue enqueue isr ok, queue type:%d  queue_count:%d enqueue_count:%d\r\n",queue_type,bc_queue[queue_type].queue_count,bc_queue[queue_type].enqueue_count);
	xHigherPriorityTaskWoken = bc_pdFALSE;
	xReturn =bc_rtos_queue_isr_enqueue(bc_queue[queue_type].queue_handler,(void *)enqueue_data,&xHigherPriorityTaskWoken);
	if(xReturn != bc_pdTRUE)
	{
		//BC_LOG_ERROR("%s fial\r\n",bc_queue[queue_type].queue_name);
		return false;
	}
    UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    if(bc_queue[queue_type].queue_count < 0xff)
        bc_queue[queue_type].queue_count++;
  bc_queue[queue_type].enqueue_count++;
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
  //printf("queue enqueue ok, queue type:%d   count:%d enqueue_count:%d\r\n",queue_type,bc_queue[queue_type].queue_count,bc_queue[queue_type].enqueue_count);
	//bc_portYIELD_FROM_ISR( xHigherPriorityTaskWoken);
	return true;		
}

/*******************************************************************************
 * Function Name     : bc_queue_enqueue
 * Description       : 入队
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
bool bc_queue_enqueue(bc_queue_type queue_type,void *  enqueue_data)
{
//	bc_rtos_sem_take(BC_ENUQUE_SEM );
	bc_base_type_t xReturn = bc_pdPASS;

	
	if(bc_queue[queue_type].queue_count >= bc_queue[queue_type].queue_depth)
	{

//		bc_rtos_sem_give(BC_ENUQUE_SEM);
        BC_LOG_ERROR("queue enqueue fial, queue type:%d  queue_count:%d \r\n",queue_type,bc_queue[queue_type].queue_count);
		return false;
	}
    BC_LOG_INFO("queue enqueue ok, queue type:%d  queue_count:%d enqueue_count:%d\r\n",queue_type,bc_queue[queue_type].queue_count,bc_queue[queue_type].enqueue_count);
	xReturn = bc_rtos_queue_send(bc_queue[queue_type].queue_handler,(void *)enqueue_data);
	if(xReturn != bc_pdPASS)
	{
//		bc_rtos_sem_give(BC_ENUQUE_SEM);

		return false;
	}
  //printf("queue enqueue ok, queue type:%d   count:%d enqueue_count:%d\r\n",queue_type,bc_queue[queue_type].queue_count,bc_queue[queue_type].enqueue_count);
    bc_rtos_taskENTER_CRITICAL();
    if(bc_queue[queue_type].queue_count < 0xff)
        bc_queue[queue_type].queue_count++;
  bc_queue[queue_type].enqueue_count++;
    bc_rtos_taskEXIT_CRITICAL();
//	bc_rtos_sem_give(BC_ENUQUE_SEM);
	return true;		
}

/*******************************************************************************
 * Function Name     : bc_queue_dequeue
 * Description       : 出队
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
bool bc_queue_dequeue(bc_queue_type queue_type, void *  const pvBuffer)
{
	bc_base_type_t xReturn = bc_rtos_queue_receive(bc_queue[queue_type].queue_handler,pvBuffer);
	if(xReturn != bc_pdPASS)
	{
        BC_LOG_ERROR("queue dequeue fial, queue type:%d  queue_count:%d \r\n",queue_type,bc_queue[queue_type].queue_count);
		return false;
	}
    
    BC_LOG_INFO("queue dequeue ok, queue type:%d  queue_count:%d dequeue_conut:%d\r\n",queue_type,bc_queue[queue_type].queue_count,bc_queue[queue_type].dequeue_conut);
    bc_rtos_taskENTER_CRITICAL();
    if(bc_queue[queue_type].queue_count)
        bc_queue[queue_type].queue_count--;
  bc_queue[queue_type].dequeue_conut++;
    bc_rtos_taskEXIT_CRITICAL();
  
//	BC_LOG_DEBUG("queue dequeue ok\r\n");
	return true;	
}

/*******************************************************************************
 * Function Name     : bc_queue_dequeue
 * Description       : 获取队列条数
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
uint32_t bc_queue_get_count(bc_queue_type queue_type)
{
	return bc_rtos_queue_messages_waiting(bc_queue[queue_type].queue_handler);
}

/*******************************************************************************
 * Function Name     : bc_queue_dequeue
 * Description       : 清空队列
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
void bc_queue_clear(bc_queue_type queue_type)
{
	bc_rtos_queue_reset(bc_queue[queue_type].queue_handler);
	bc_queue[queue_type].queue_count = 0;
  bc_queue[queue_type].dequeue_conut = 0;
  bc_queue[queue_type].enqueue_count = 0;
}


/*******************************************************************************
 * Function Name     : bc_queue_dequeue
 * Description       : 初始化队列
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
void bc_queue_init(void)
{
  for(uint8_t i=0;i<BC_QUEUE_TYPE_NUM;i++)
	{
		bc_queue[i].queue_handler = bc_rtos_queue_create(bc_queue[i].queue_depth,bc_queue[i].queue_buff_length);
    bc_queue[i].dequeue_conut = 0;
    bc_queue[i].enqueue_count = 0;
		if(bc_queue[i].queue_handler != NULL)
		{
			BC_LOG_INFO("create  %s queue succeed! \r\n",bc_queue[i].queue_name);
		}
		else
		{
			BC_LOG_ERROR("create %s queue fail!\r\n",bc_queue[i].queue_name);
		}
	}
	

}











