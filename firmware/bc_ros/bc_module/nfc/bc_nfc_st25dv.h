#ifndef __BC_NFC_ST25DV_H__
#define __BC_NFC_ST25DV_H__


#include <stdint.h>
#include <stdbool.h>


uint8_t bc_nfc_st25dv_device_chip_id_get(void);

bool bc_nfc_st25dv_device_chip_id_check(void);

void bc_nfc_st25dv_send_process(uint8_t charing_status,uint8_t vbat_percen);

void bc_nfc_st25dv_device_init(void);





#endif


