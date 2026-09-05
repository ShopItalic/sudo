
#include <stdint.h>
#include <stdbool.h>


#include "ring_config.h"


enum pmic_charge_status
{
	PMIC_CHARGED_NOT = 0,
	PMIC_CHARGED_ING,
	PMIC_CHARGED_OVER,
};

/*

*/
bool bc_pmic_get_id(uint8_t *data);

/*

*/
enum pmic_charge_status bc_pmic_get_charge_status(void);

/*

*/
void bc_pmic_set_shipmode(void);

/*

*/
void bc_pmic_feeddog(void);

/*

*/
void bc_pmic_read_all(uint8_t *data);

/*

*/
void bc_pmic_init(void);



void bc_pmic_device_find(void);

uint16_t bc_pmic_get_adc_value(void);

uint8_t bc_pmic_get_vbat_percen(void);

bool bc_pmic_check_vbat(void);

bool bc_pmic_id_hardware_check(void);

void bc_pmic_set_sleepmode(void);

void bc_pmic_set_startmode(void);

