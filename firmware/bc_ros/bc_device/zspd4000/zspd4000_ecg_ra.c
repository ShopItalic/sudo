#include <stdio.h>
#include "zspd4000_drv.h"
#include "zspd4000_port.h"
#include "zspd4000_regcfg.h"
#include "ecg_fifo.h"

#include <string.h>


//#define PPG_TS_ENABLE

fifo_t ecgfifo;
uint8_t	readbuff[600];//300*2

volatile uint8_t ecg_start = 0;
extern uint8_t rawforalgin[600];
extern uint16_t zspd_clk_frq ;

extern int32_t TEST_BIN_ADDR[];
extern uint32_t TEST_BIN_SIZE;

void ZSPD4000_EcgPreInit(void)
{
	uint16_t tmp;
	// soft reset
	tmp = 0x0001 ;
	ZSPD_WRITE_REG16 (0x00, &tmp , 1) ;
	
	tmp = 0x0080 ;
	ZSPD_WRITE_REG16 (0x02, &tmp , 1) ;
	tmp = zspd_clk_frq / 600 ;//0x0035 ;
	ZSPD_WRITE_REG16 (0x04, &tmp , 1) ;
	
	tmp = 0x0001 ;		// ECG use timeslot A
	ZSPD_WRITE_REG16 (0x0F, &tmp , 1) ;
	tmp = 0x00FF ;
	ZSPD_WRITE_REG16 (0x17, &tmp , 1) ;	
	tmp = 0x00FF ;
	ZSPD_WRITE_REG16 (0x18, &tmp , 1) ;	
#ifdef USE_INTERNAL_PD
	tmp = 0x00AA ;
	ZSPD_WRITE_REG16 (0x19, &tmp , 1) ;
	tmp = 0x0022 ;
	ZSPD_WRITE_REG16 (0x1A, &tmp , 1) ;	
#else
	tmp = 0xAA00 ;
	ZSPD_WRITE_REG16 (0x19, &tmp , 1) ;
	tmp = 0x2200 ;
	ZSPD_WRITE_REG16 (0x1A, &tmp , 1) ;	
#endif	
	
	tmp = 0x0500 ;
	ZSPD_WRITE_REG16 (0x1F, &tmp , 1) ;		
	
	tmp = 0x0010 ;
	ZSPD_WRITE_REG16 (0x00, &tmp , 1) ;		/*clear fifo*/
	tmp = 0x0091 ;	//0x0099
	ZSPD_WRITE_REG16 (0x02, &tmp , 1) ;		
}

void ZSPD4000_EcgInit(void)
{
	uint16_t tmp;
	
	ZSPD4000_EcgPreInit();
	ZSPD4000_DelayMs(500);
	
	// soft reset
	tmp = 0x0001 ;
	ZSPD_WRITE_REG16 (0x00, &tmp , 1) ;
	
	tmp = 0x0080 ;
	ZSPD_WRITE_REG16 (0x02, &tmp , 1) ;	// Stop TS
	tmp = zspd_clk_frq / 300 ;//0x006A ;
	ZSPD_WRITE_REG16 (0x04, &tmp , 1) ;	// Set sample rate
	tmp = 0x0001 ;
	ZSPD_WRITE_REG16 (0x06, &tmp , 1) ;	// Config GPIO
	
	tmp = 0x0060 ;
	ZSPD_WRITE_REG16 (0x0E, &tmp , 1) ;//dis_tia_ovr

	tmp = 0x0007 ;	// Sel time slot A,B,C
	ZSPD_WRITE_REG16 (0x0F, &tmp , 1) ;

	tmp = 0x0500 ;
	ZSPD_WRITE_REG16 (0x1F, &tmp , 1) ;//VC_VREF
	
	tmp = 0x1041 ;
	ZSPD_WRITE_REG16 (0x1B, &tmp , 1) ;//TIA_200K	
			
	tmp = 0x0046 ;
	ZSPD_WRITE_REG16 (0x1C, &tmp , 1) ;//INTEG_EN,BPF_BYP,INTG_INPUT_RES=340K	
	
	tmp = 0x0003 ;
	ZSPD_WRITE_REG16 (0x1E, &tmp , 1) ;//INTEG_WIDTH=3uS	

	tmp = 0x000D ;
	ZSPD_WRITE_REG16 (0x16, &tmp , 1) ;//LED_offset=13uS	

	tmp = 0x0000 ;
	ZSPD_WRITE_REG16 (0x17, &tmp , 1) ;//LED_WIDTH=0uS

	tmp = 0x1209 ;
	ZSPD_WRITE_REG16 (0x1D, &tmp , 1) ;//INTEG_offset=LED_offset+LED_WIDTH-INTEG_WIDTH=250 ns
	
	tmp = 0x0102 ;
	ZSPD_WRITE_REG16 (0x21, &tmp , 1) ;//MOD_TYPE=float modulation MOD_WIDTH=2uS

//	delay_ms(10);
	
	tmp = 0x0001 ;	// Sel time slot A
	ZSPD_WRITE_REG16 (0x0F, &tmp , 1) ;

	tmp = 0x0000 ;
	ZSPD_WRITE_REG16 (0x18, &tmp , 1) ;
	
#ifdef USE_INTERNAL_PD
	tmp = 0x1140 ;
	ZSPD_WRITE_REG16 (0x19, &tmp , 1) ;//INT2
	
	tmp = 0x1140 ;
	ZSPD_WRITE_REG16 (0x1A, &tmp , 1) ;//INT3
#else
	tmp = 0x4011 ;
	ZSPD_WRITE_REG16 (0x19, &tmp , 1) ;//INT0
	
	tmp = 0x4011 ;
	ZSPD_WRITE_REG16 (0x1A, &tmp , 1) ;//INT1
#endif	
	tmp = 0x0102 ;
	ZSPD_WRITE_REG16 (0x21, &tmp , 1) ;//MOD_TYPE=float modulation MOD_WIDTH=2uS

//	delay_ms(10);
				
	tmp = 0x0002 ;	// Sel time slot B
	ZSPD_WRITE_REG16 (0x0F, &tmp , 1) ;
	
	tmp = 0x0A00 ;
	ZSPD_WRITE_REG16 (0x1F, &tmp , 1) ;//VC_VREF+250MV
	
	tmp = 0x0000 ;//00-OK	
	ZSPD_WRITE_REG16 (0x18, &tmp , 1) ;
	
#ifdef USE_INTERNAL_PD
	tmp = 0x1121 ;//	
	ZSPD_WRITE_REG16 (0x19, &tmp , 1) ;
	
	tmp = 0x1181 ;//	
	ZSPD_WRITE_REG16 (0x1A, &tmp , 1) ;	
#else
	tmp = 0x2111 ;//	
	ZSPD_WRITE_REG16 (0x19, &tmp , 1) ;
	
	tmp = 0x8111 ;//	
	ZSPD_WRITE_REG16 (0x1A, &tmp , 1) ;	
#endif
	tmp = 0x5000 ;	// 6
	ZSPD_WRITE_REG16 (0x23, &tmp , 1) ;	
	
//	delay_ms(10);

	tmp = 0x0004 ;	// Sel time slot C
	ZSPD_WRITE_REG16 (0x0F, &tmp , 1) ;
	
	tmp = 0x0500 ;//	
	ZSPD_WRITE_REG16 (0x1F, &tmp , 1) ;	//VC_VREF

	tmp = 0x0032 ;//	
	ZSPD_WRITE_REG16 (0x18, &tmp , 1) ;
#ifdef USE_INTERNAL_PD	
	tmp = 0x1188 ;//	
	ZSPD_WRITE_REG16 (0x19, &tmp , 1) ;//INT2

	tmp = 0x1122 ;//00-OK	
	ZSPD_WRITE_REG16 (0x1A, &tmp , 1) ;//INT3
#else
	tmp = 0x8811 ;//	
	ZSPD_WRITE_REG16 (0x19, &tmp , 1) ;//INT0

	tmp = 0x2211 ;//00-OK	
	ZSPD_WRITE_REG16 (0x1A, &tmp , 1) ;//INT1
#endif
	tmp = 0x5000 ;	// 6
	ZSPD_WRITE_REG16 (0x23, &tmp , 1) ;	
	
//	delay_ms(10);	

#ifdef PPG_TS_ENABLE
	tmp = 0x0008 ;	// Sel time slot D
	ZSPD_WRITE_REG16 (0x0F, &tmp , 1) ;
	
	tmp = 0x0050 ;	// Green LED current
	ZSPD_WRITE_REG16 (0x13, &tmp , 1) ;
	tmp = 0x3811 ;
	ZSPD_WRITE_REG16 (0x1B, &tmp , 1) ;	
	tmp = 0x0200 ;
	ZSPD_WRITE_REG16 (0x1F, &tmp , 1) ;	
	
#ifdef USE_INTERNAL_PD
	tmp = 0x4207 ;
	ZSPD_WRITE_REG16 (0x18, &tmp , 1) ;		
	tmp = 0x1111 ;
	ZSPD_WRITE_REG16 (0x19, &tmp , 1) ;	
	tmp = 0x1111 ;
	ZSPD_WRITE_REG16 (0x1A, &tmp , 1) ;	
#else
	tmp = 0x0007 ;
	ZSPD_WRITE_REG16 (0x18, &tmp , 1) ;	
	tmp = 0x1142 ;
	ZSPD_WRITE_REG16 (0x19, &tmp , 1) ;	
	tmp = 0x1112 ;
	ZSPD_WRITE_REG16 (0x1A, &tmp , 1) ;	
#endif	//USE_INTERNAL_PD
	tmp = 0x5000 ;	// 6
	ZSPD_WRITE_REG16 (0x23, &tmp , 1) ;	
#endif	// PPG_TS_ENABLE

	tmp = 0x0010 ;
	ZSPD_WRITE_REG16 (0x00, &tmp , 1) ;		/*clear fifo*/
//	ZSPD4000_DumpRegister();
	zspd_satus = ECG_NORMAL ;
	ZSPD4000_GpioIntConfig(1);
	
#ifdef PPG_TS_ENABLE
	tmp = 0x0091 ;	//0x009F
	ZSPD_WRITE_REG16 (0x02, &tmp , 1) ;	
#else
	tmp = 0x0097 ;	//0x0097
	ZSPD_WRITE_REG16 (0x02, &tmp , 1) ;			
#endif
	
	fifo_init(&ecgfifo, readbuff );
	fifo_free(&ecgfifo);	
	
	HAL_TIM_Base_Start_IT(&htim2);

	ZSPD_PRINTF ("ECG_MODE INIT.\n");
}

#if 1
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/*
	static uint16_t timeslot5 ;
	printf("TIM2 int %d\n", HAL_GetTick() - timeslot5) ;
	timeslot5 =  HAL_GetTick() ;
	*/
	uint8_t i, bytehl[2], data_size;
	uint16_t num;
	int16_t tmp; 
	static uint32_t size=2500;
		
	if(htim == &htim2)	//100 ms
	{
		data_size = ZSPD4000_ReadFifoCount() ;
		
		data_size = data_size/8 * 8 ;	// PPG_TS_ENABLE, Divide by 6
		
//		ZSPD_PRINTF("data_size = %d\n", data_size);
		// read out data to clear ZSPD interrupt flag
		if(data_size > 0 && data_size <= ZSPD4000_FIFO_MAX_DEPTH/2){
			ZSPD4000_ReadFifoData (data_size, zapd_raw_dat); 
			
			// PUSH data to ring
			for(i=0;i<data_size;i++)
			{
				if(i%8 == 1)	// Slot B, RA data
				{
					printf("%d\n", zapd_raw_dat[i]);		
					continue ;
				}
				
				if(i%8 == 2)	// Slot C, RA data
				{
//					printf("%d\n", zapd_raw_dat[i]);		
					continue ;
				}

//				if(i%9 == 3)	// PPG data
//				{
////					printf("%d\n", zapd_raw_dat[i]);		
//					continue ;
//				}
#if 0		// use test data
				tmp = TEST_BIN_ADDR[size];
				size ++ ;
				if(size>(TEST_BIN_SIZE/4))
					size = 0 ;
				memcpy(bytehl, (uint8_t *)&tmp, 2);
#else					
				memcpy(bytehl, (uint8_t *)&zapd_raw_dat[i], 2);
#endif
				fifo_put(&ecgfifo, bytehl[0]) ;
				fifo_put(&ecgfifo, bytehl[1]) ;
			}
			// get ringbuf data size
			num = fifo_avail(&ecgfifo);		
			if((num>=ECG_ALG_IN_SIZE*2) && (ecg_start == 0)) // no pending data for alg
			{
				for(i=0;i<ECG_ALG_IN_SIZE;i++)	// POP data from ring and copy to ecg_in buf
				{
					fifo_get(&ecgfifo, &bytehl[0]) ;
					fifo_get(&ecgfifo, &bytehl[1]) ;

//					tmp = *(int16_t *)&bytehl[0] ;
//					ecg_in[i] = tmp ;
						rawforalgin[2*i] = bytehl[0] ;
						rawforalgin[2*i + 1] = bytehl[1] ;
				}		
				ecg_start = 1;
			}			
		}

		else
		{
			ZSPD_PRINTF("FIFO read length out of range\n");			
			return;
		}		
	}
}
#endif
