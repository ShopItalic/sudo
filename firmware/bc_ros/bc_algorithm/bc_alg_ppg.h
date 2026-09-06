/*
* 勇芯半导体(北京)有限公司 版权所有
* 当前版本： 1.0
* 作 者： 王超
* 完成日期： 2023年2月18日
*/
#ifndef _BC_ALG_ECG_H_
#define _BC_ALG_ECG_H_

#include <stdint.h>

void bc_alg_rri_init(void);
void bc_alg_rri_append(int32_t hrs_data);
void bc_alg_rri_process(uint8_t hr,uint16_t *pbuffer,uint8_t *len);

void bc_alg_check_wear_init(void);
void bc_alg_check_wear_ir_append(int32_t wave);
void bc_alg_check_wear_temp_append(uint16_t temp);
uint8_t bc_alg_get_wear_flag(void);
uint32_t bc_alg_get_real_vpp(void);

#endif


