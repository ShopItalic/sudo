#ifndef __APP_PMIC_HANDLER_H__
#define __APP_PMIC_HANDLER_H__

#include "stdint.h"

extern uint8_t precent;

void app_pmic_handler_timer_create(void);


void app_pmic_handler_timer_start(void);

void app_pmic_handler_timer_stop(void);

uint8_t pmic_state_get(void);

uint8_t app_pmic_precent_get(void);

uint8_t getvpct(void);

#endif


