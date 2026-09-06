#ifndef __WEAR_CHECK_H
#define __WEAR_CHECK_H

#include "zsbm_algo.h"

#define WEAR_NONE				1			//穿戴状态：未穿戴
#define WEAR_UNDETECTED	2			//未检测
#define WEAR_POORLY			4			//穿戴状态：穿戴异常
#define WEAR_NICE				8			//穿戴状态：正常穿戴

//芯片型号枚举
typedef enum {    
	NULL_TYPE = 0,
  ZSPD_TYPE, 
  ZSBM_TYPE
} ZS_SERIES_TYPE;
//DC检测输出结果的结构体
typedef struct {
  int32_t  	dc_average;      							// 本次穿戴校验的平均DC值
  uint8_t		current_wearing_condition;    // 本次校验的穿戴状态
} ZS_WEAR_STSTUE_RET;

//脱腕检测输出结果的结构体		
typedef struct {												
  int32_t  	dc_average;      							// 本次穿戴校验的平均DC值
  uint8_t		dc_wearing_condition;    			// DC穿戴检测结果
	uint8_t		ac_wearing_condition;    			// AC穿戴检测结果
	uint8_t		result_wear_statue;   				// 本次穿戴判定结果
} ZS_WEAR_STSTUE_FULL_RET;




/******************************以下为设置开关选项******************************************/
#define DC_CHECK_INIT_STATUE								WEAR_NICE					//DC穿戴状态的初始化配置  可选项：WEAR_NONE(未穿戴),WEAR_NICE(已穿戴),WEAR_UNDETECTED(未检测)
#define AC_CHECK_INIT_STATUE								WEAR_UNDETECTED		//AC穿戴状态的初始化配置	可选项：WEAR_NONE(未穿戴),WEAR_NICE(已穿戴),WEAR_UNDETECTED(未检测)

#define WEAR_CHECK_THRESHOLD								3000			//判定未穿戴的阈值：数值0~32767，数值越大，穿戴判定条件越苛刻，数值越小穿戴判定条件越随和，默认值3000
#define HUMAN_AC_SIGNEL_THRESHOLD						12				//信号AC强度评分阈值，HUMAN_AC_SIGNEL_THRESHOLD数值越大穿戴判定条件越苛刻，HUMAN_AC_SIGNEL_THRESHOLD数值越小穿戴判定条件越随和，默认值15，输入范围1~255

#define WEAR_NONE_CNT_TIME									10				//未穿戴检测时间，单位100mS，配置范围1~255，对应时间0.1秒~25.5秒
#define WEAR_NICE_CNT_TIME									20				//穿戴良好检测时间，单位100mS，配置范围1~255，对应时间0.1秒~25.5秒

#define WEAR_CHECK_STEP2_WE_CHECK_NUM				3					//算法检测已佩戴次数，默认值3，输入范围0~255  每个值约1秒 默认3则代表3秒检测
#define WEAR_CHECK_STEP2_NG_CHECK_NUM				3					//算法检测未佩戴次数，默认值3，输入范围0~255  每个值约1秒 默认3则代表3秒检测


ZS_WEAR_STSTUE_RET wear_check__process_step_1(uint8_t* wear_data_temp,uint16_t wear_data_num,uint8_t wear_data_bit_width,uint8_t repeat_tsx,uint8_t Equipment_type);		//穿戴检测初级验证程序  /**无需引用该程序**/
ZS_WEAR_STSTUE_FULL_RET wear_check__process(ZSBM_ALGO_INIT_PARAMETERS *algo_init,ZSBM_ALGO_INPUT_DATA *algo_in ,ZSBM_ALGO_OUTPUT_DATA *algo_out,uint8_t repeat_tsx,uint8_t Equipment_type);	//佩戴检测程序  /**该函数应在主函数中引用**/




#endif









