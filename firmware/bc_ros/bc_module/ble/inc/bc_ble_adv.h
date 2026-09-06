#ifndef __FML_BLE_ADV_H__
#define __FML_BLE_ADV_H__


//广播需要引用的头文件
#include "ble_advdata.h"
#include "ble_advertising.h"

//#define             ADVERTISING_UPDATE

void fml_ble_advertising_config_get(ble_adv_modes_config_t * p_config);
void fml_ble_advertising_config_set(ble_adv_modes_config_t * p_config);

void ble_adv_data_update(uint8_t *update_data,uint8_t update_length);

void advertising_start(void);
void advertising_stop(void);
void advertising_init(void);
 
void bc_ble_adv_data_update(uint8_t *update_data,uint8_t update_length);






#endif


