#ifndef __BC_PMIC_PORT_H__
#define __BC_PMIC_PORT_H__

#include <stdint.h>

typedef int (*pmic_i2c_read_t)(uint8_t reg, uint8_t *pData, uint8_t size); 
typedef int (*pmic_i2c_write_t)(uint8_t reg, uint8_t *pData, uint8_t size);
typedef void (*pmic_i2c_init_t)(void);
typedef void (*pmic_i2c_open_t)(void);
typedef void (*pmic_i2c_close_t)(void);

typedef struct
{
    pmic_i2c_read_t i2c_read;
    pmic_i2c_write_t i2c_write;
    pmic_i2c_init_t i2c_init;
    pmic_i2c_open_t i2c_open;
    pmic_i2c_close_t i2c_close;
}   pmic_i2c_t;

void pmic_i2c_init(void);
void pmic_i2c_open(void);
void pmic_i2c_colse(void);
int pmic_i2c_read(uint8_t reg, uint8_t *pData, uint8_t size);
int pmic_i2c_write(uint8_t reg, uint8_t *pData, uint8_t size);

#endif