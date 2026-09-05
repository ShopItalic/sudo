#ifndef __BC_NFC_READ_ST25R100_PORT_H__
#define __BC_NFC_READ_ST25R100_PORT_H__


#include <stdint.h>
#include <stdbool.h>




void bc_nfc_read_spi_flash_device_find(void);



void bc_nfc_read_spi_flash_cs_high(void);

void bc_nfc_read_spi_flash_cs_low(void);

uint8_t bc_nfc_read_int_io_status_get(uint8_t port,uint8_t pin);

void bc_nfc_reset_output_low(uint8_t port,uint8_t pin,uint8_t status);

void bc_nfc_reset_output_high(uint8_t port,uint8_t pin,uint8_t status);

void bc_nfc_reset_output_toggle(uint8_t port,uint8_t pin);

bool bc_nfc_read_spi_flash_write_and_read(const uint8_t *write_buff,uint8_t *read_buff,uint16_t length);















#endif





