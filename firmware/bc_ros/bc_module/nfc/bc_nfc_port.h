#ifndef  __BC_NFC_PORT_H__
#define  __BC_NFC_PORT_H__



#include <stdint.h>


extern uint8_t globalCommProtectCnt;


void bc_nfc_exit_irq_disable(void);

void bc_nfc_exit_irq_enable(void);


uint32_t bc_nfc_get_tick(void);

void bc_nfc_delay(uint32_t ms);


uint8_t bc_nfc_exit_irq_io_state(void);

void bc_nfc_error_handler(char * file, int line);

int32_t bc_nfc_spi_write_and_recv(const uint8_t * const write_data, uint8_t * const recv_data, uint16_t length);

void bc_nfc_open_spi(void);

void bc_nfc_close_spi(void);

void bc_nfc_spi_cs_high(void);

void bc_nfc_spi_cs_low(void);

void bc_nfc_io_irq_reg_callback(void *lo_to_hi_irq_callback,void *hi_to_lo_irq_callback);
void bc_nfc_irq_state_handler(void);
int bc_bfc_log(const char* format, ...);
void bc_nfc_device_find(void);

#endif





