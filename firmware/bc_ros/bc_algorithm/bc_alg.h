/*
* 勇芯半导体(北京)有限公司 版权所有
* 当前版本： 1.0
* 作 者： 王超
* 完成日期： 2023年2月18日
*/

#include <stdint.h>
#include "ring_config.h"
#include "bc_alg_ppg.h"

/********************************************************* bc_alg_ppg *************************************************/
#ifndef _BC_ALG_ECG_H_
#define _BC_ALG_ECG_H_

/**************************************
    @brief   ppg算法参数初始化.

    @remark  

    @param   none.

    @return  none
***************************************/
void bc_alg_rri_init(void);

/**************************************
    @brief   数据填充.

    @remark  

    @param   hrs_data：心率数据.

    @return  none
***************************************/
void bc_alg_rri_append(int32_t hrs_data);

/**************************************
    @brief   计算rri.

    @remark  

    @param   hr：心率数据.
             *pbuffer：rri结果数组
             *len：rri数据量

    @return  none
***************************************/
void bc_alg_rri_process(uint8_t hr,uint16_t *pbuffer,uint8_t *len);

/**************************************
    @brief   佩戴检测算法参数初始化.

    @remark  

    @param   mode：ppg工作模式.

    @return  none
***************************************/
#if (PPG_DEVIECE_TYPE == 0 || PPG_DEVIECE_TYPE == 4)   //hx 3605

        void bc_alg_check_wear_init(void);
#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2 || PPG_DEVIECE_TYPE == 3)  // zspd4000

     void bc_alg_check_wear_init(void);
#endif


/**************************************
    @brief   佩戴检测算法数据填充.

    @remark  

    @param   data：ppg数据.

    @return  none
***************************************/
void bc_alg_check_wear_append(int32_t data);

/**************************************
    @brief   佩戴检测算法结果获取.

    @remark  

    @param   none.

    @return  1：佩戴
             0：未佩戴
***************************************/
uint8_t bc_alg_get_wear_flag(void);

uint8_t bc_alg_pi_filter(uint8_t data);

uint8_t bc_alg_stress(uint8_t hr,uint8_t hrv);

uint32_t bc_alg_get_real_vpp(void);
uint32_t bc_alg_get_min_value(void);
uint32_t bc_alg_get_max_value(void);
uint8_t bc_alg_get_size(void);

#endif

/********************************************************* bc_alg_sleep *************************************************/
#ifndef _BC_ALG_SLEEP_H_
#define _BC_ALG_SLEEP_H_


void sleepClassification_active(unsigned char reason);
/**************************************
    @brief   睡眠计算.

    @remark  

    @param   time：时间戳（s）
             step：步数
             sport_num：五分钟内运动次数
             time_hour：当前小时
             hr：心率

    @return  0：无效
             1：清醒
             2：浅睡
             3：深睡
             4：眼动
***************************************/
unsigned char sleepClassification(unsigned int time,
                            unsigned short int step,
                            unsigned char sport_num,
                            unsigned char time_hour,
                            unsigned char hr);
                            
void sleep_analysis_callback(int timediff, int sleep_state);
void sleep_calculate();

#endif

/********************************************************* bc_alg_temp *************************************************/
#ifndef _BC_ALG_TEMP_H
#define _BC_ALG_TEMP_H

void bc_alg_temp_init(void);
uint16_t bc_alg_temp_append(uint16_t temp_data);


#endif

/********************************************************* bc_alg_util *************************************************/
#ifndef _BC_ALG_UTIL_H
#define _BC_ALG_UTIL_H

int32_t min_value_i32(int32_t *value, uint32_t length);
int32_t max_value_i32(int32_t *value, uint32_t length);
uint32_t sum_value_u16(uint16_t *value, uint32_t length);
float standard_deviation(uint16_t* data, uint32_t length); 
float standard_deviation_i32(int32_t* data, uint32_t length);


#endif



