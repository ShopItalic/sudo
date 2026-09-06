#include "bc_nfc_st25dv.h"

#include "nfc04a1_nfctag.h"

#include "bc_nfc_st25dv_port.h"
#include "bc_logger.h"
#include "bc_delay.h"

uint8_t bc_nfc_st25dv_device_chip_id_get(void)
{
	uint8_t temp =0;
	bc_nfc_st25dv_i2c_open();
	bc_delay_ms(50);
	temp = MX_NFC_readID();
	bc_nfc_st25dv_i2c_close();
	BC_LOG_INFO("bc_nfc_st25dv_device_chip_id:%02x \r\n",temp);
	return temp;
}	

bool bc_nfc_st25dv_device_chip_id_check(void)
{
	uint8_t id = bc_nfc_st25dv_device_chip_id_get();
	if( id == 0x50)
	{
		return true;
	}
	return false;
}	

void bc_nfc_st25dv_device_init(void)
{
	bc_nfc_st25dv_i2c_open();
	bc_delay_ms(50);
	MX_NFC_Init();
	bc_nfc_st25dv_i2c_close();
}	



void bc_nfc_st25dv_send_process(uint8_t charing_status,uint8_t vbat_percen)
{
	uint8_t temp =0;
	bc_nfc_st25dv_i2c_open();
	bc_delay_ms(50);
	temp = MX_NFC_readID();
	if( temp == 0x50)
	{
		MX_NFC4_MAILBOX_Process(charing_status,vbat_percen);
	}
	bc_nfc_st25dv_i2c_close();
	BC_LOG_INFO("bc_nfc_st25dv_device_chip_id:%02x \r\n",temp);
}















