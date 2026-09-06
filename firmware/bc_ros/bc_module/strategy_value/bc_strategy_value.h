#ifndef __STRATEGY_H__
#define __STRATEGY_H__

#include "stdint.h"
#include "stdbool.h"



typedef enum
{
	BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_TIME = 0,
	BUSINESS_STRATEGY_PPG_HR_AUTOMATIC_COLLECTION_TIME,
	BUSINESS_STRATEGY_PPG_SPO2_AUTOMATIC_COLLECTION_TIME,
	BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_HR_TO_SPO2,
	BUSINESS_STRATEGY_CH0_TOUCH_THRESHOLD,
	BUSINESS_STRATEGY_CH1_TOUCH_THRESHOLD,
	BUSINESS_STRATEGY_CH2_TOUCH_THRESHOLD,
	BUSINESS_STRATEGY_TEST,
	BUSINESS_STRATEGY_TEST1,
	BUSINESS_STRATEGY_CRC,
	BUSINESS_STRATEGY_MAX
}business_strategy;


void bc_business_strategy_init(void);
bool bc_get_business_strategy_value_all(uint8_t *data_value,uint8_t *data_len);
bool bc_set_business_strategy_value_all(uint8_t *data,uint8_t len);

bool bc_set_business_strategy_value(business_strategy business_strategy_id,uint32_t value);
uint32_t bc_get_business_strategy_value(business_strategy business_strategy_id);

void bc_business_strategy_reset(void);

#endif
