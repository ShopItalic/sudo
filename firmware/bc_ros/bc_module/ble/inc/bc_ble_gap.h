#ifndef __FML_BLE_GAP_H__
#define __FML_BLE_GAP_H__



#include <stdbool.h>
#include "bc_ble_modu_interface.h"




bool bc_conn_params_change(enum bc_ble_conn_params conn_params);


void gap_params_init(void);








#endif
