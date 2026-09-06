#ifndef __FML_BLE_H__
#define __FML_BLE_H__









#include "stdint.h"
#include "stdbool.h"






void bc_ble_send(uint8_t *send_data,uint16_t send_length);
void bc_ble_send_session(uint8_t *data, uint16_t length, uint32_t session);
uint32_t bc_ble_session_id(void);
void bc_ble_tx_wake(void);

void bc_ble_init(void);

bool bc_ble_connect_status(void);
bool bc_ble_pm_connect_status(void);

void bc_ble_disconnect(void);
void bc_ble_mac_get(uint8_t *ble_mac);
void bc_ble_mac_set(uint8_t *ble_mac);



#endif


