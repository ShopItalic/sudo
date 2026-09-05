#ifndef __APP_BLE_SPEED_HANDLER_H__
#define __APP_BLE_SPEED_HANDLER_H__




#include "stdint.h"


void app_ble_speed_test_start(uint8_t leng);

void app_ble_speed_test_stop(void);



void app_wifi_speed_test_start(uint8_t leng);

void app_wifi_speed_test_stop(void);

void app_ble_speed_time_create(void);


#endif



