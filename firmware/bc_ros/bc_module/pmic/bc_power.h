
#include <stdint.h>
#include <stdbool.h>

#if defined(SUDO_VOICE_ONLY)
/* Returned when the SUDO ADC transaction cannot produce a checked sample. */
#define BC_POWER_ADC_ERROR UINT16_MAX
#define BC_POWER_PERCENT_UNKNOWN ((uint8_t)255U)
#endif

uint16_t bc_power_get_adc_value(void);

uint8_t bc_power_get_vbat_percen(void);

bool bc_power_check_vbat(void);

bool bc_power_adc_hardware_error_register_callback(const void *error_callback);

void bc_power_vbat_adc_find(void);










