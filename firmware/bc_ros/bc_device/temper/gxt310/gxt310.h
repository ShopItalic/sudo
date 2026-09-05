#ifndef __GXT310_H__
#define __GXT310_H__





#include "stdint.h"
#include "stdbool.h"


void gxt310_temper_get_config_all_device(void);
void gxt310_temper_set_frequency_reg(uint8_t number);


float gxt310x0_temper_get(void);

float gxt310x1_temper_get(void);

float gxt310x2_temper_get(void);

float gxt310x3_temper_get(void);



uint8_t gxt310x0_temper_get_id(void);
uint8_t gxt310x1_temper_get_id(void);
uint8_t gxt310x2_temper_get_id(void);
uint8_t gxt310x3_temper_get_id(void);


//模式切换 0关断，1工作
void gxt310x0_switch_mode(bool mode);
//模式切换 0关断，1工作
void gxt310x1_switch_mode(bool mode);
//模式切换 0关断，1工作
void gxt310x2_switch_mode(bool mode);
//模式切换 0关断，1工作
void gxt310x3_switch_mode(bool mode);

#endif

