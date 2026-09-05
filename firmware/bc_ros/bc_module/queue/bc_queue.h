#ifndef __BC_QUEUE_H__
#define __BC_QUEUE_H__




#include "stdint.h"
#include "stdbool.h"


#include "ring_config.h"

typedef enum
{
	BC_QUEUE_TYPE_BLE_RECV = 0,
	BC_QUEUE_TYPE_BLE_SEND,
	BC_QUEUE_TYPE_PPG_COLLECTION_HR_DATA,
	BC_QUEUE_TYPE_PPG_COLLECTION_SPO2_DATA,
	
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED == 1 || HARDWARE_1121_ENABLED || HARDWARE_158_ENABLED || HARDWARE_1181_ENABLED || HARDWARE_1171_ENABLED || HARDWARE_1191_ENABLED\
     || HARDWARE_1231_ENABLED )	
	BC_QUEUE_TYPE_PDM_COLLECTION_DATA,	
#endif
#if !defined(HANDWARE_1_23_4)
	BC_QUEUE_TYPE_IMU_SEND_DATA,
#endif
#if (HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)
    BC_QUEUE_TYPE_PPG_COLLECTION_SPO2_HR_DATA,
#endif

#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
    BC_QUEUE_TYPE_IIC1_HANDLE_DATA,
#endif	
	
	BC_QUEUE_TYPE_NUM
}bc_queue_type;









bool bc_queue_isr_enqueue(bc_queue_type queue_type,void *  enqueue_data);
bool bc_queue_isr_enqueue_not_yield(bc_queue_type queue_type,void *  enqueue_data);
bool bc_queue_enqueue(bc_queue_type queue_type,void *  enqueue_data);

bool bc_queue_dequeue(bc_queue_type queue_type, void *  const pvBuffer);

uint32_t bc_queue_get_count(bc_queue_type queue_type);

void bc_queue_clear(bc_queue_type queue_type);

void bc_queue_init(void);


#endif


