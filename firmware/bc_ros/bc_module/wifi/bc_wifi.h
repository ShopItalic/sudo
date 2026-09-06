#ifndef __BC_WIFI_H__
#define __BC_WIFI_H__



#include "stdint.h"




void  bc_wifi_open(void);

void  bc_wifi_close(void);

void bc_wifi_send(uint8_t *data,uint16_t length);
























#endif




