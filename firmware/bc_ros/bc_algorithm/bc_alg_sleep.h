/*
* 勇芯半导体(北京)有限公司 版权所有
* 当前版本： 1.0
* 作 者： 王超
* 完成日期： 2023年2月18日
*/
#ifndef _BC_ALG_SLEEP_H_
#define _BC_ALG_SLEEP_H_

#include <stdint.h>

uint8_t bc_alg_sleep(uint32_t time,
                    uint16_t step,
                    uint8_t sport_num,
                    uint8_t time_hour,
                    uint8_t hr);

#endif


