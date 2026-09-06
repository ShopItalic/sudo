#ifndef _Cellwise_CW221X_Driver_h_
#define _Cellwise_CW221X_Driver_h_

#include "stdint.h"

int cw221x_get_chip_id(int *chip_id);
int cw221x_sleep();
int cw221x_get_vol(unsigned int *lp_vol);
int cw221x_get_capacity(int *lp_uisoc);
int cw221x_get_temp(int *lp_temp);
int cw221x_get_current(long *lp_current);
int cw221x_get_cycle_count(int *lp_count);
int cw221x_get_soh(int *lp_soh);
int cw221x_dump_register(void);
int cw221x_bat_init(uint8_t vbat_type);
#endif



