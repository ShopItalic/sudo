#ifndef __FML_BLE_ADV_H__
#define __FML_BLE_ADV_H__


//广播需要引用的头文件
#include "ble_advdata.h"
#include "ble_advertising.h"

void fml_ble_advertising_config_get(ble_adv_modes_config_t * p_config);
void fml_ble_advertising_config_set(ble_adv_modes_config_t * p_config);


 void advertising_start(void);
 void advertising_init(void);


#endif


