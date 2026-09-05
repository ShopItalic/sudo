#ifndef __BC_NFC_H__
#define __BC_NFC_H__

#include <stdbool.h>
#include <stdint.h>

bool bc_nfc_recv_register_callback(void *callback);

bool bc_nfc_write(uint8_t *data,uint16_t length);

void bc_nfc_init_start(void);

bool bc_nfc_url_set(uint8_t *url_data,uint8_t url_length);


void bc_nfc_init_stop(void);


#endif




