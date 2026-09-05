#ifndef __BC_PMIC_DEVICE_PORT_H__
#define __BC_PMIC_DEVICE_PORT_H__


#include "stdint.h"
#include "stdbool.h"


void pmic_i2c_init(void);

void pmic_int_chg_open(void);

void pmic_int_chg_close(void);

void pmic_i2c_open(void);

void pmic_i2c_close(void);	
	
bool pmic_i2c_write(uint8_t reg_add ,uint8_t data,uint8_t length);
																 
bool pmic_i2c_read(uint8_t reg_add ,uint8_t *data,uint8_t length);

void pmic_delay(uint16_t ms);

void pmic_ship_mode_en(void);


void bc_pmic_stacmd_wirte(uint8_t reg_addr,uint8_t length,uint8_t *data);

void bc_pmic_stacmd_read(uint8_t reg_addr,uint8_t length,uint8_t *data);

void bc_pmic_device_stacmd_find(void);

void pmic_i2c_bus_open(void);

void pmic_i2c_bus_close(void);

bool pmic_irq_register_callback(void *callback);
void pmic_io_irq_enable(void);
void pmic_io_irq_disnable(void);
uint8_t pmic_io_irq_status(void);

#endif
























