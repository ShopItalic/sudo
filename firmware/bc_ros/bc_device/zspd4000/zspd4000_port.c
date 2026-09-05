#include "zspd4000_port.h"

#include "bc_ppg_driver_port.h"
#include "bc_delay.h"

#define ZSPD4XXX7BITADDR			(0x5B<<1)

// used to read out raw data from ZSPD4000 fifo, max 128 bytes
int16_t zapd_raw_dat[256] = {0} ;

// big_endian to little_endian
static uint16_t Bswap_16 (uint16_t n)
{
	return (((uint16_t)(n) & 0x00ff ) << 8 ) | \
				(((uint16_t)(n) & 0xff00 ) >> 8 ) ;
}

/**
*function:millisecond delay, used for power up reset delay
*parameter:ms, time in millisecond
*return:none
*date:2022/01/23
**/
void ZSPD4000_DelayMs(uint32_t ms)
{
		bc_delay_ms(ms);		
}

/**
*function:Read multy word (16bit register) from ZSPD4000
*parameter:regaddr, register address
*parameter:rcvbuf, read data first address
*parameter:length, data length
*return: data length
*date:2022/10/23
**/
uint8_t ZSPD4000_ReadMultyWord (uint8_t regaddr , uint16_t *rcvbuf, uint8_t length)
{
	uint8_t tmplen = length;
	bc_ppg_2c_read(ZSPD4XXX7BITADDR,regaddr,(uint8_t *)rcvbuf, tmplen*2);
//	while(tmplen--)
//	{
//		*rcvbuf = Bswap_16(*rcvbuf);
//		rcvbuf++ ;
//	}
	for(uint8_t i = 0; i < tmplen ;i++)
	{
		rcvbuf[i] = Bswap_16(rcvbuf[i]);
	}
	return length;	
}

/**
*function:Write multy word (16bit register) to ZSPD4000
*parameter:regaddr, register address
*parameter:regdat, reg values to be write
*parameter:length, data length
*return: data length
*date:2022/10/23
**/
uint8_t ZSPD4000_WriteMultyWord (uint8_t regaddr , uint16_t *regdat, uint8_t length)
{
	// Max 38 regs once
	uint8_t i, tmp[128];
	
	tmp[0] = regaddr ; 
	
	for(i=0; i< length; i++)
	{
		tmp[2*i+1] = *regdat >> 8 ;
		tmp[2*i + 2] = *regdat & 0xff ;		
		regdat++ ;
	}
	bc_ppg_i2c_write(ZSPD4XXX7BITADDR,regaddr,&tmp[1],length*2);
	return length;	
}

/**
*function:Clear host CPU pending GPIO interrupt then enable GPIO interrupt
*parameter:none
*return: none
*date:2022/10/23
**/
void ZSPD4000_GpioIntEnable(void)
{
	bc_ppg_int_io_irq_enable();
//	zspd_int_flag = 1;
}

/**
*function:Disable host CPU GPIO interrupt
*parameter:none
*return: none
*date:2022/10/23
**/
void ZSPD4000_GpioIntDisable(void)
{
  bc_ppg_int_io_irq_disable();
}
