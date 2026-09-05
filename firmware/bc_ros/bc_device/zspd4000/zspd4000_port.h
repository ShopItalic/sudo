#ifndef _ZSPD4000_PORT_H_
#define _ZSPD4000_PORT_H_

#include <stdio.h>


#include "zspd4000_regcfg.h"
#include "stdint.h"

#define ZSPD_DEBUG	1

#ifdef ZSPD_DEBUG
//#define  ZSPD_PRINTF(x, args ...) printf(" [%s()]\n"  x, __func__, ## args)
#define  ZSPD_PRINTF(...)     printf(__VA_ARGS__)
#else
#define	 ZSPD_PRINTF(x, args ...)  
#endif

#define BSWAP_16(n)		(uint16_t)(((uint16_t)(n) & 0x00ff << 8 ) | \
																(((uint16_t)(n) & 0xff00 >> 8 )))
typedef enum {    
		NULL_MODE,
    HR_MODE, 
    HRV_MODE,
    SPO2_MODE,	
		ECG_MODE,
    WEAR_MODE,		
    FT_G_MODE,	 
    FT_R_MODE,
		FT_GR_MODE,
    FT_GRI_MODE,
		FT_4PD_MODE,
} WORK_MODE_T;;

typedef struct {    
		int16_t *ppgi;
		int16_t *ppgg;
		int16_t *ppgr;
} ZSPD_SPO2_DATA_T;

extern int16_t zapd_raw_dat[] ;
extern volatile uint8_t zspd_int_flag;

extern void ZSPD4000_DelayMs(uint32_t ms);
extern uint8_t ZSPD4000_ReadMultyWord (uint8_t regaddr , uint16_t *rcvbuf, uint8_t length) ;
extern uint8_t ZSPD4000_WriteMultyWord (uint8_t regaddr , uint16_t *regdat, uint8_t length) ;
extern void ZSPD4000_GpioIntEnable(void) ;
extern void ZSPD4000_GpioIntDisable(void) ;

#endif
