/*
* 勇芯半导体(北京)有限公司 版权所有
* 当前版本： 1.0
* 作 者： 王超
* 完成日期： 2023年2月18日
*/
#ifndef _BC_ALG_ACC_H_
#define _BC_ALG_ACC_H_

#include <stdint.h>

void bc_alg_pinch_init(void);
void bc_alg_pinch_append(int16_t acc_x,int16_t acc_y,int16_t acc_z);
uint8_t bc_alg_get_pinch_result(void);

#endif


