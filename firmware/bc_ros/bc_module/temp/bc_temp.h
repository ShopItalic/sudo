
#include <stdint.h>
#include "ring_config.h"

#include "stdint.h"
#include "stdbool.h"

uint16_t bc_temp_get_temperature_value(void);

uint16_t bc_temp_get_temper_adc_value(void);

void bc_temp_temperature_adc_find(void);

bool bc_temp_temperature_check(void);

uint16_t bc_temp_get_ntc_adc_value(void);

uint16_t bc_temp_ppg_get_temperature_value(void);

void bc_temp_ppg_get_temperature_off(void);

void bc_temp_ppg_get_temperature_on(void);

bool bc_temper_check(void);

void bc_temper_id_get(uint8_t *data);

void bc_temper_value_get(uint8_t *data);

void bc_temper_value_get_rawdata(uint16_t *temper_0,uint16_t *temper_1,uint16_t *temper_2,uint16_t *temper_3);

void bc_temper_ic_enable(void);

void bc_temper_ic_disenable(void);










