
#include <stdint.h>
#include <stdbool.h>

uint16_t bc_power_get_adc_value(void);

uint8_t bc_power_get_vbat_percen(void);

bool bc_power_check_vbat(void);

bool bc_power_adc_hardware_error_register_callback(const void *error_callback);

void bc_power_vbat_adc_find(void);










