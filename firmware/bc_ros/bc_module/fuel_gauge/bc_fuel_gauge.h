#ifndef __BC_FUEL_GAUGE_H__
#define __BC_FUEL_GAUGE_H__

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#if 0
typedef enum{
    BATT_TYPE_JWLN_12MAH = 0,
    BATT_TYPE_JWLN_14MAH,
    BATT_TYPE_JWLN_18MAH,
    BATT_TYPE_NUM
} batt_type_t;
#else
typedef enum{
    BATT_TYPE_JWLN_155522_12MAH = 0,
    BATT_TYPE_JWLN_155525_15MAH, // 1
    BATT_TYPE_JWLN_155528_18MAH, // 2
    BATT_TYPE_JWLN_155522_14MAH, // 3
    BATT_TYPE_JWLN_155525_16MAH, // 4
    BATT_TYPE_JWLN_155528_19MAH, // 5
    BATT_TYPE_HL_155625_14MAH,   // 6
    BATT_TYPE_HL_155628_16MAH,   // 7
    BATT_TYPE_HL_155634_20MAH,   // 8
    BATT_TYPE_JWLN_155021_6_12MAH, // 9
    BATT_TYPE_JWLN_155021_7_12MAH, // 10
    BATT_TYPE_JWLN_155026_8_16MAH, // 11
    BATT_TYPE_JWLN_155026_9_16MAH, // 12
    BATT_TYPE_JWLN_155031_10_20MAH, // 13
    BATT_TYPE_JWLN_155031_11_20MAH, // 14
    BATT_TYPE_JWLN_155031_12_20MAH, // 15
    BATT_TYPE_JWLN_155031_13_20MAH, // 16
    BATT_TYPE_JWLN_150722_18MAH, // 17
    BATT_TYPE_JWLN_150728_25MAH, // 18
    BATT_TYPE_JWLN_150732_30MAH, // 19
    BATT_TYPE_JWLN_150722_14MAH, // 20
    BATT_TYPE_NUM
} batt_type_t;
#endif

typedef struct {
    int batt_per;
    unsigned int batt_vol;
    long batt_cur;
    int batt_temp;
    int batt_cycle_cnt;
    int batt_health;
} fuel_parameter_t;

bool bc_fuel_gauge_init(void);
uint8_t bc_fuel_gauge_getId(void);
bool bc_fuel_gauge_checkId(void);

uint8_t bc_fuel_gauge_getBattPer(void);

bool bc_fuel_gauge_i2c_write(uint8_t slave_addr,uint8_t reg_add ,uint8_t *data,uint8_t length);
bool bc_fuel_gauge_i2c_read(uint8_t slave_addr,uint8_t reg_add ,uint8_t *data,uint8_t length);
void bc_fuel_gauge_getParameter(uint8_t *data);

#endif
