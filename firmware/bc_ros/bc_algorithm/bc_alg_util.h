/*
* 勇芯半导体(北京)有限公司 版权所有
* 当前版本： 1.0
* 作 者： 王超
* 完成日期： 2023年2月18日
*/
#ifndef _BC_ALG_UTIL_H
#define _BC_ALG_UTIL_H

#include <stdint.h>

int32_t min_value_i32(int32_t *value, uint32_t length);
int32_t max_value_i32(int32_t *value, uint32_t length);
uint32_t sum_value_u16(uint16_t *value, uint32_t length);
float standard_deviation(uint16_t* data, uint32_t length); 


#endif


