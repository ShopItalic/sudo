#ifndef __BC_NFC_ST25DV_PORT_H__
#define __BC_NFC_ST25DV_PORT_H__

#include <stdint.h>


void bc_nfc_st25dv_i2c_write(uint8_t slave_addr,uint16_t reg_addr,uint8_t *write_data,uint16_t write_length);

void bc_nfc_st25dv_i2c_read(uint8_t slave_addr,uint16_t reg_addr,uint8_t *read_data,uint16_t read_length);

void bc_nfc_st25dv_i2c_open(void);

void bc_nfc_st25dv_i2c_close(void);


void bc_nfc_st25dv_device_find(void);















#endif







