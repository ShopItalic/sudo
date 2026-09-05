/*
* 勇芯半导体(北京)有限公司 版权所有
* 当前版本： 1.0
* 作 者： 王超
* 完成日期： 2023年2月18日
*/
#ifndef _YHM2712_H
#define _YHM2712_H

#include "sdk_common.h"

void YHM2710_read(void);
ret_code_t YHM2710_init(void);
void YHM2710_set_shipmode(void);
uint8_t YHM2710_read_id(void);
uint8_t YHM2710_read_charge_status(void);
void YHM2710_read_all(uint8_t *pdata);

void YHM2710_set_sleepmode(void);

void YHM2710_set_startmode(void);

#endif    
