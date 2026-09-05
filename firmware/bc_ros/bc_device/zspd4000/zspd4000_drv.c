#include <stdio.h>
#include <string.h>
#include "zspd4000_drv.h"
#include "zsbm_algo.h"
#include "bc_logger.h"
#include "bc_delay.h"

#include "wear_check.h"

#ifndef ABS
#define ABS(a) ((a) >= 0 ? (a) : -(a))
#endif

#define DELTA_EVA_COEF (5)

ZSBM_ALGO_INPUT_DATA in;
ZSBM_ALGO_OUTPUT_DATA out;

bool ppg_ir_flag = false;

int16_t wave_out[50];
uint8_t zspd_algo_buffer[PPG_SPO2_ALGO_BUFFER_BYTES_100HZ] = {0};

uint8_t zspd_agc_comp_flg = 0;			//AGC调光完成标志，用于调整调光时的定时器配置  //2025年1月20日

uint8_t* zspd_algo_buffer_addr(void)
{
	return zspd_algo_buffer;
}

static ZSBM_ALGO_INIT_PARAMETERS *zspd_algo_init_para = NULL;
static ZS_WEAR_STSTUE_FULL_RET zspd_ware_status = {0};



static int32_t data_process_temp[64] = {0};
static int16_t write_point = 0;
static uint8_t	first_flg = 1;



ZSPD_SENSOR_STATUS_T zspd_satus = RESET_STATUS ;
WORK_MODE_T workmode ;

volatile uint8_t zspd_int_flag = 0;
uint16_t zspd_clk_frq = 32000;

WORK_MODE_T workmode ;
static const uint16_t gains[8] = {TIA_12K5_CAP_8P86,TIA_25K_CAP_6P32,TIA_50K_CAP_4P74,TIA_100K_CAP_3P16,TIA_200K_CAP_1P58,TIA_400K_CAP_960F,TIA_800K_CAP_480F,TIA_1M6_CAP_320F};

extern void set_io(void);

//uint8_t rawforalgin[600];
volatile uint16_t ppgdatready = 0;

extern int32_t TEST_BIN_ADDR[];
extern uint32_t TEST_BIN_SIZE;

extern uint8_t	readbuff[];//300*2

ZSPD_LED_CFG_T led_cfg[3] = { \
															{AGC_IR_LED_CURREN_MAX,  IR_GAIN_MAX_LV - 1, 0}, \
															{AGC_GREEN_LED_CURREN_MAX,  GREEN_GAIN_MAX_LV - 1, 0}, \
															{AGC_RED_LED_CURREN_MAX,  RED_GAIN_MAX_LV - 1, 0} \
														} ;

typedef void (*ppg_green_data_callback)(int16_t *data,int16_t *acc_data,uint8_t length); 		
typedef void (*ppg_red_and_ir_data_callback)(void *red_data,uint8_t red_length,void *ir_data,uint8_t ir_length,,int16_t *acc_data); 		
typedef void (*ppg_hr_result_callback)(uint8_t heart_rate,uint8_t hrv1); 
typedef void (*ppg_spo2_result_callback)(uint8_t spo2,uint8_t heart_rate);

typedef void (*ppg_signal_check_callback)(uint16_t signal_strength);
typedef void (*ppg_signal_check_callback)(uint16_t signal_strength); 
typedef void (*ppg_gary_card_callback)(void *green_data,void* red_data,void* ir_data);													

static ppg_green_data_callback	     green_data_callback = NULL;											
static ppg_red_and_ir_data_callback  red_and_ir_data_callback = NULL;	
static ppg_hr_result_callback	     hr_result_callback = NULL;	
static ppg_spo2_result_callback		 spo2_result_callback = NULL;
														
static ppg_signal_check_callback     hr_signal_check_callback = NULL;
static ppg_signal_check_callback     spo2_signal_check_callback = NULL;			
static ppg_gary_card_callback		 gary_card_callback = NULL;												
 
void ZSPD4000_SoftReset(void)
{
	uint16_t tmp = SOFT_RESET ;
	ZSPD_WRITE_REG16(0x00, &tmp, 1) ;
}

/*clear fifo*/
void ZSPD4000_ClearFifo(void)
{
	int16_t data_size = 0;
	int16_t raw_dat[64] = {0} ;
	data_size = ZSPD4000_ReadFifoCount() ;	//读fifo深度
	if(data_size > 0 && data_size <= ZSPD4000_FIFO_MAX_DEPTH/2)
		ZSPD4000_ReadFifoData (data_size, raw_dat); //读取FIFO值
	
	data_size = 0;
	
	bc_delay_ms(5);			//延时用于缓解板子差模信号不平衡时造成的电压波动
	
//	uint16_t tmp = FIFO_CLR ;
//	ZSPD_WRITE_REG16(0x00, &tmp, 1);
}

ZSPD_ERROR_CODE_T ZSPD4000_IDCheck(void)
{
    uint8_t retry = 5 ; 
		uint16_t  reg_value[2] ;
    while(retry--)
		{
			ZSPD_READ_REG16(0, reg_value, 2) ;
			if (reg_value[0] == 0x400) 
			{
				ZSPD_PRINTF("Chip Version = %4x!\n", reg_value[1]) ;
				return ZSPD_OK;			
			}	
		}
		ZSPD_PRINTF("Chip ID check fail %4x!\n", reg_value[0]) ;
    return ZSPD_ID_FAIL;
}

uint16_t ZSPD4000_ID_get(void)
{
	uint16_t  reg_value[2] ;
	ZSPD_READ_REG16(0, reg_value, 2) ;
	return reg_value[0];
}

/**
*function:Enable 32K OSC and Set sample rate
*parameter: sample rate;
*return:none;
*date:2022/04/11
**/
void ZSPD4000_SetSampleRate(uint16_t samplerate) 
{
	uint16_t tmp[3];
	
	/*open OSC32K and stop all timeslots */
	tmp[0] = OSC32K_EN ;		// reg 0x02
	
	ZSPD_PRINTF("zspd_clk_frq = %d\n", zspd_clk_frq);

	/*set time slot cycle according to sample rate */
	tmp[1] = (zspd_clk_frq / samplerate ) >> 16 ;			// reg 0x03
	tmp[2] = (zspd_clk_frq / samplerate ) & 0xFFFF ;		// reg 0x04
	ZSPD_WRITE_REG16(0x02, tmp, 3) ;
	return ;
}

/**
*function:Set ZSPD FIFO mode/depth
*parameter: depth;
*return:none;
*date:2022/04/11
**/
void ZSPD4000_SetFifoDepth(uint16_t depth)
{
	uint16_t tmp;
	
	/*auto clear fifo after reading, overwrite old data when the fifo full*/
	tmp = INT_FIFO_THRHD_EN | INT_ACLEAR_FIFO_EN|FIFO_OF_CONTINUE_WR_NEW ;
	ZSPD_WRITE_REG16(0x06, &tmp, 1);

	/*config FIFO interupt threshold*/
	tmp = (depth & 0x003F) -1 ;
	ZSPD_WRITE_REG16(0x0C, &tmp, 1);
}

/**
*function:Time slot sel
*parameter: timeslot;
*return:none;
*date:2022/04/11
**/
void ZSPD4000_SelectTimeSlot( uint8_t timeslot)
{
	uint16_t tmp;
	tmp = timeslot ;
	
	ZSPD_WRITE_REG16(0x0F, &tmp, 1);
}

/**
*function:config fifo word size
*parameter: depth;
*return:none;
*date:2022/04/11
**/
void ZSPD4000_SetFifoWordSize(uint16_t fifowordsize)
{
	uint16_t tmp;
	
	tmp = fifowordsize ;
	ZSPD_WRITE_REG16(0x10, &tmp, 1);
}

/**
*function:config number of repeat tsx
*parameter: repeattsx;
*return:none;
*date:2022/04/11
**/
void ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx(uint8_t subperiod, uint8_t numinttsx, uint8_t numberrepeattsx)
{
	uint16_t tmp;
	tmp = ((subperiod & 0xf ) << 12) ;
	tmp |= ((numinttsx & 0xf ) << 8 );
	tmp |= (numberrepeattsx & 0xff);
	ZSPD_WRITE_REG16(0x23, &tmp, 1);
}
/**
*function:Set ZSPD TIA_GAIN
*parameter: depth;
*return:none;
*date:2022/04/11
**/
void ZSPD4000_SetTIAGain(uint16_t tiagaingap)
{
	uint16_t tmp;
	
	tmp = (tiagaingap & 0xFF70) | TIA_VREF | TIA_EN;
	ZSPD_WRITE_REG16(0x1B, &tmp, 1);
}


/**
*function:Set ZSPD TIA_VREF
*parameter: depth;
*return:none;
*date:2022/04/11
**/
void ZSPD4000_SetVcSel(uint16_t vcsel)
{
	uint16_t tmp;
	
	tmp = vcsel ;
	ZSPD_WRITE_REG16(0x1F, &tmp, 1);
}

/**
*function:Enable time slot and start Cycle
*parameter: timeslot;
*return:none;
*date:2022/04/11
**/
void ZSPD4000_TimerSlotStart( uint16_t timeslot)
{
	uint16_t tmp;
	tmp = (timeslot & 0x000F) | OP_EN | OSC32K_EN;

	/*enable time slot, go!*/
	ZSPD_WRITE_REG16(0x02, &tmp, 1);
}

/**
*function:Config ZSPD GPIO Interrupt output
*parameter: gpiointoutputmode;
*return:none;
*date:2022/04/11
**/
void ZSPD4000_ConfigIntOutputPin( uint16_t gpiointoutputmode)
{
	uint16_t tmp;
	
	/*GPIO0 set push-pull output, interrupt output, active low, low drive strength*/
//	tmp = GPIO0_CFG_OUT_PP | GPIO0_POL_NEG | GPIO0_DS_12MA | GPIO0_OUT_SOURCE_INT;
	tmp = gpiointoutputmode;	
	ZSPD_WRITE_REG16(0x0B, &tmp, 1);
}

/**
*function:Config ZSPD PD Connection
*parameter: PDconnect;
*return:none;
*date:2022/04/11
**/
void ZSPD4000_ConfigPDConnection( ZSPD_PD_CONN_T *PDconnect)
{
	uint16_t *tmp;
	tmp = (uint16_t *)PDconnect ;
	ZSPD_WRITE_REG16(0x18, tmp, 3);
}

/**
*function:Set LED current
*parameter: led channel , led current;
Note: Green always set to timeslot A
			Red always set to timeslot B
			IR always set to timeslot C
*return:none;
*date:2022/04/11
**/
void ZSPD4000_SetLedCurrent( ZSPD_LED_T ledchannel, uint16_t ledcurrent)
{
	uint16_t tmp;

	switch(ledchannel)
	{
		case Z_GREEN_CH :
			tmp = TSA_REG_SEL ;
			ZSPD_WRITE_REG16(0x0f, &tmp, 1);
			tmp = ledcurrent & 0x007F;
			ZSPD_WRITE_REG16(LED_GREEN_CURRENT_REG, &tmp, 1);
			break;
		
		case Z_RED_CH :
			tmp = TSB_REG_SEL ;
			ZSPD_WRITE_REG16(0x0f, &tmp, 1);
			tmp = ledcurrent & 0x007F;
			ZSPD_WRITE_REG16(LED_RED_CURRENT_REG, &tmp, 1);
			break;
		
		case Z_IR_CH :
			tmp = TSC_REG_SEL ;
			ZSPD_WRITE_REG16(0x0f, &tmp, 1);
			tmp = ledcurrent & 0x007F;
			ZSPD_WRITE_REG16(LED_IR_CURRENT_REG, &tmp, 1);
			break;		
		default:
			break;
	}
}

/**
*function:Get LED current and tia setting
*parameter: led channel , led current, tia setting;
Note: Green always set to timeslot A
			Red always set to timeslot B
			IR always set to timeslot C
*return:none;
*date:2022/12/20
**/
void ZSPD4000_GetLedCurrentAndTiaSetting( ZSPD_LED_T ledchannel, uint16_t *current, uint16_t *tiagain)
{
	uint16_t tmp, tia;

	switch(ledchannel)
	{
		case Z_GREEN_CH :
			tmp = TSA_REG_SEL ;
			ZSPD_WRITE_REG16(0x0f, &tmp, 1);
			ZSPD_READ_REG16(LED_GREEN_CURRENT_REG, &tmp, 1);
			ZSPD_READ_REG16(0x1B, &tia, 1);
			break;
		
		case Z_RED_CH :
			tmp = TSB_REG_SEL ;
			ZSPD_WRITE_REG16(0x0f, &tmp, 1);
			ZSPD_READ_REG16(LED_RED_CURRENT_REG, &tmp, 1);
			ZSPD_READ_REG16(0x1B, &tia, 1);
			break;
		
		case Z_IR_CH :
			tmp = TSC_REG_SEL ;
			ZSPD_WRITE_REG16(0x0f, &tmp, 1);
			ZSPD_READ_REG16(LED_IR_CURRENT_REG, &tmp, 1);
			ZSPD_READ_REG16(0x1B, &tia, 1);
			break;	
		
		default:
			break;
	}
	ZSPD_PRINTF("CURRENT = %d, TIA = 0x%x\n", tmp, tia) ;
	
	tmp &= 0x7F ;
	if(tmp<16)
		*current = tmp ;
	else
		*current = (tmp-16) * 2 + 16 ;
	
	*tiagain = tia & 0xFF70 ;
}

/**
*function:read fifo data number
*parameter:none;
*return:fifo_num_temp:fifo data size;
*date:2022/04/11
**/
uint8_t ZSPD4000_ReadFifoCount(void) 
{
	uint16_t fifo_num_temp;
	ZSPD_READ_REG16(0x07, &fifo_num_temp, 1) ;
	return fifo_num_temp&0x003F;
}

/**
*function: read fifo data
*parameter:read_fifo_size,fifo size
*parameter:buf,read fifo data first address
*return:none
*date:2022/07/04
**/
void ZSPD4000_ReadFifoData(uint8_t read_fifo_size, int16_t *buf)
{
	uint16_t tmp;
#if 1
	ZSPD_READ_REG16(0x60, (uint16_t *)buf, read_fifo_size) ;
#else
	uint8_t i;
	uint16_t *ptr=(uint16_t *)buf;
	for(i=0;i<read_fifo_size;i++)
		ZSPD_READ_REG16(0x60, ptr++, 1);
#endif	
	ZSPD_READ_REG16(0x44, &tmp, 1);
}

/**
*function:clear fifo and interrupt_flg
*parameter:none
*return:none
*date:2022/05/26
**/
void ZSPD4000_ClearIntAndFifo(void) 
{
	int16_t tmp[64];
	ZSPD4000_ReadFifoData(ZSPD4000_ReadFifoCount(), tmp) ;
}


/**
*function:gipo interrupt config
*parameter:	en,	!=0 -> enable; 0 -> disable
*return:none
*date:2022/01/21
**/
void ZSPD4000_GpioIntConfig(uint8_t en)
{
	uint16_t tmp;
	
	// enable/disable host CPU gpio interrupt
	if(en)
		ZSPD4000_GpioIntEnable();
	else
		ZSPD4000_GpioIntDisable();

	// clear ZSPD4000 fifo count & fifo data
	tmp = INT_FIFO_THRHD_CLR ;
	ZSPD_WRITE_REG16(0x08, &tmp, 1) ;
	tmp = FIFO_CLR ;
	ZSPD_WRITE_REG16(0x00, &tmp, 1);
}

/**
*function:heart rate ppg adc offset
*parameter:none
*return:0,success
*date:2022/04/16
**/
ZSPD_ERROR_CODE_T ZSPD4000_SetPpgAdcOffset(uint8_t length, int16_t *buf)
{
	uint8_t i;
	int16_t tmp;
	// time slot A + B + C enabled
	for(i=3; i > 0; i--)
	{
		tmp = 0 - buf[length-1-i];
		ZSPD4000_SelectTimeSlot(0x1<<(3-i)) ;
		ZSPD_WRITE_REG16(0x22, (uint16_t *)&tmp, 1);
		return ZSPD_OK;
	}
	return ZSPD_AGC_OFFSET_FAIL;
}


/**
*function:heart rate ppg adc offset
*parameter:none
*return:0,success
*date:2022/04/16
**/

void ZSPD4000_CommonInit(void)
{
	ZSPD_ERROR_CODE_T ret;
	ZSPD_PD_CONN_T PDconnect ;
	
	zspd_satus = RESET_STATUS;
	// can remove this softreset
	ZSPD4000_SoftReset() ;
	bc_delay_ms(200);
	ZSPD4000_GpioIntConfig(0);	// Disable GPIO IRQ
	
	ret = ZSPD4000_IDCheck();
	if(ret != ZSPD_OK)
		ZSPD_PRINTF("ZSPD4000 ID check fail!\n") ;
	
//	ZSPD4000_SetSampleRate(SAMPLE_FS_10HZ) ;	// start with IR @ 10Hz
	ZSPD4000_ConfigIntOutputPin( GPIO0_CFG_OUT_PP | GPIO0_POL_NEG | GPIO0_DS_12MA | GPIO0_OUT_SOURCE_INT) ;
//	ZSPD4000_SetFifoDepth(FIFO_DEPTH_AGC_CAL_IR_DETECT) ;	
/*
	
	ZSPD4000_SetLedCurrent(Z_GREEN_CH, 0) ;
	ZSPD4000_SetLedCurrent(Z_RED_CH, 0) ;
	ZSPD4000_SetLedCurrent(Z_IR_CH, 0) ;
*/	
	
	ZSPD4000_SelectTimeSlot	(TSA_REG_SEL | TSB_REG_SEL  | TSC_REG_SEL  | TSD_REG_SEL) ;
	/*config fifo word size*/
//	ZSPD4000_SetFifoWordSize(FIFO_WORD_SIZE_16);
//	ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx	(9, 0, 0) ;
	
	/* PD connection config */
#ifdef USE_INTERNAL_PD
	PDconnect.REG18 = 0x4207;
//	PDconnect.REG19 = 0x1111;
//	PDconnect.REG1A = 0x1111;
#else
//	PDconnect.REG18 = 0x0007;
	PDconnect.REG18 = 0x00FF;
	PDconnect.REG19 = PD0_CONNECTION;
	PDconnect.REG1A = PD1_CONNECTION;
#endif	

	ZSPD4000_ConfigPDConnection(&PDconnect);

	/*TIA gain = TIA_GAIN, feedback cap = TIA_GAIN_CAP, TIA Vref = TIA_VREF, TIA enable*/
	ZSPD4000_SetTIAGain(TIA_GAIN_CAP);
	
	/*VC = TIA_VREF + 250mV*/
	ZSPD4000_SetVcSel(VC_SEL_TIA_VREF_250);	

	ZSPD_PRINTF("ZSPD4000_CommonInit Done!\n") ;


/*
	ZSPD4000_ClearFifo();
	ZSPD4000_TimerSlotStart(TSA_EN | TSB_EN | TSC_EN);	
	
	zspd_satus = AGC_OFFSET_CAL ;
*/
	ZSPD4000_GpioIntConfig(1);	// Enable GPIO IRQ
}

void ZSPD_IRDeteLowFs(void)
{
//	ZSPD4000_SetSampleRate(SAMPLE_FS_100HZ) ;	// start with IR @ 5Hz
//	ZSPD4000_SetFifoDepth(12) ;	
//	
//	ZSPD4000_SetLedCurrent(Z_IR_CH, 0);
////	ZSPD4000_SetTIAGain(AGC_CAL_IR_TIA_GAIN_CAP);
//	ZSPD4000_SetFifoWordSize(FIFO_WORD_SIZE_16);
////	ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx	(0, 0, 0) ;

//	ZSPD4000_SetLedCurrent(Z_GREEN_CH, 5) ;
//	ZSPD4000_SetTIAGain(AGC_CAL_IR_TIA_GAIN_CAP);
//	ZSPD4000_SetFifoWordSize(FIFO_WORD_SIZE_16);
////	ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx	(0, 0, 0) ;

//	ZSPD4000_SetLedCurrent(Z_RED_CH, 0) ;
//	ZSPD4000_SetFifoWordSize(FIFO_WORD_SIZE_16);
////	ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx	(0, 0, 0) ;
//	
//	ZSPD4000_ClearFifo();

//	ZSPD4000_TimerSlotStart(TSA_EN );	
	
	
	ZSPD4000_SetSampleRate(SAMPLE_FS_10HZ) ;	// start with IR @ 5Hz
	ZSPD4000_SetFifoDepth(FIFO_DEPTH_AGC_CAL_IR_DETECT) ;	
	
	ZSPD4000_SetLedCurrent(Z_IR_CH, AGC_CAL_IR_INIT_CURR) ;
	ZSPD4000_SetTIAGain(AGC_CAL_IR_TIA_GAIN_CAP);
	ZSPD4000_SetFifoWordSize(FIFO_WORD_SIZE_16);
	ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx	(0, 0, 0) ;

	ZSPD4000_SetLedCurrent(Z_GREEN_CH, 0) ;
	ZSPD4000_SetFifoWordSize(FIFO_WORD_SIZE_16);
	ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx	(0, 0, 0) ;

	ZSPD4000_SetLedCurrent(Z_RED_CH, 0) ;
	ZSPD4000_SetFifoWordSize(FIFO_WORD_SIZE_16);
	ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx	(0, 0, 0) ;
	
	ZSPD4000_ClearFifo();

	ZSPD4000_TimerSlotStart(TSC_EN );	
}

void ZSPD_GreenNormal(uint16_t samplerate)
{
	uint16_t cur, tia ;
	ZSPD4000_SetSampleRate(samplerate) ;	// 
	ZSPD4000_SetFifoDepth	(FIFO_DEPTH_HRS_HRV) ;					// fifo depth set to 25
	ZSPD4000_SetLedCurrent(Z_GREEN_CH, led_cfg[1].led_cur) ;
	ZSPD4000_SetTIAGain(gains[led_cfg[1].tia_cap_group]);
	
	/*
	// Multy pos
	ZSPD4000_SelectTimeSlot(TSA_REG_SEL) ;
	ZSPD4000_SetFifoWordSize(FIFO_WORD_SIZE_32);
	ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx	(0, 0, 7) ;
	*/
//	ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx	(0, 0, 0) ;
	ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx	(0, 0, 0) ;

	ZSPD4000_GetLedCurrentAndTiaSetting(Z_GREEN_CH, &cur, &tia);
	
	ZSPD4000_ClearFifo();

	ZSPD4000_TimerSlotStart(TSA_EN );	
}

static uint16_t tmp = 0x03;
void ZSPD_IrRedNormal(uint16_t samplerate)
{
	uint16_t cur, tia ;
	ZSPD4000_SetSampleRate(samplerate) ;	// 
	ZSPD4000_SetFifoDepth	(FIFO_DEPTH_SPO2) ;					// fifo depth set to 40
	if(!ppg_ir_flag)
	{		
		ZSPD4000_SetLedCurrent(Z_RED_CH, led_cfg[2].led_cur) ;
		ZSPD4000_SetTIAGain(gains[led_cfg[2].tia_cap_group]);	
	}
	ZSPD4000_SetLedCurrent(Z_IR_CH, led_cfg[0].led_cur );
	ZSPD4000_SetTIAGain(gains[led_cfg[0].tia_cap_group]);

	// Multy pos
	ZSPD4000_SelectTimeSlot(TSB_REG_SEL | TSC_REG_SEL) ;
	ZSPD4000_SetFifoWordSize(FIFO_WORD_SIZE_32);
	ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx	(0, 0, 15) ;
   if(!ppg_ir_flag)
   {
		ZSPD4000_GetLedCurrentAndTiaSetting(Z_RED_CH, &cur, &tia);
   }
	ZSPD4000_GetLedCurrentAndTiaSetting(Z_IR_CH, &cur, &tia);
   
//   tmp = 0x000F;
//   ZSPD_WRITE_REG16(0x0F, &tmp, 1);
////   tmp = 0x7F;
////   ZSPD_WRITE_REG16(0x18, &tmp, 1);
//   
////	 tmp = 0x000F;
////   ZSPD_WRITE_REG16(0x0F, &tmp, 1);
//	 
   tmp = 0x0301;
   ZSPD_WRITE_REG16(0x06, &tmp, 1);
   
//    tmp = 0x0000;
//   ZSPD_WRITE_REG16(0x40, &tmp, 1);
   
    tmp = 0x8408;
   ZSPD_WRITE_REG16(0x42, &tmp, 1);
 
	ZSPD4000_ClearFifo();
	ZSPD4000_TimerSlotStart(TSB_EN | TSC_EN );	
}

ZSPD_ERROR_CODE_T ZSPD_IRDetecProcess(uint8_t data_size, int16_t *buf)
{
	uint8_t i;
//	for(i=1; i< 4; i++){
//		if(buf[data_size - i] < IR_DETEC_THRESHOLD_MIN)
//			return ZSPD_IR_DETEC_CONTINUE ;
//	}
	
	for(i=1; i<= data_size; i++){
		if((buf[data_size - i] < IR_DETEC_THRESHOLD_MIN)&&(buf[data_size - i]>=-32765))
			return ZSPD_IR_DETEC_CONTINUE ;
	}
	// IR value OK, turn off IR  
	// HRS/HRV turn Green LED, SPO2 turn on IR+RED
	return ZSPD_OK;
}

ZSPD_ERROR_CODE_T	ZSPD_AgcCalSearchCurrentSetting(uint8_t timeslot, ZSPD_LED_CFG_T *led)	// Timeslot B green
{

	if(timeslot & TSA_REG_SEL)
	{
		ZSPD4000_SetLedCurrent(Z_GREEN_CH, led[1].led_cur) ;
		ZSPD4000_SetTIAGain(gains[led[1].tia_cap_group]);		
	}

	if(timeslot & TSB_REG_SEL)
	{
		ZSPD4000_SetLedCurrent(Z_RED_CH, led[2].led_cur) ;
		ZSPD4000_SetTIAGain(gains[led[2].tia_cap_group]);		
	}

	if(timeslot & TSC_REG_SEL)
	{
		ZSPD4000_SetLedCurrent(Z_IR_CH, led[0].led_cur) ;
		ZSPD4000_SetTIAGain(gains[led[0].tia_cap_group]);		
	}
	
	ZSPD4000_ClearFifo();
	ZSPD4000_TimerSlotStart(timeslot & 0x7 );	
	return ZSPD_OK;
}

#define HIGH_PERFORMANCE
#ifdef HIGH_PERFORMANCE
ZSPD_ERROR_CODE_T ZSPD_AgcCalProcess(int16_t latest_dat, uint8_t timeslot, ZSPD_LED_CFG_T *led)
{
	uint8_t currentmin, currentmax, tiagainmax ;
	if(timeslot&0x4)	
	{
		currentmin = AGC_IR_LED_CURREN_MIN;
		currentmax = AGC_IR_LED_CURREN_MAX;
		tiagainmax = IR_GAIN_MAX_LV - 1;
	}
	if(timeslot&0x1)	
	{
		currentmin = AGC_GREEN_LED_CURREN_MIN;
		currentmax = AGC_GREEN_LED_CURREN_MAX;
		tiagainmax = GREEN_GAIN_MAX_LV - 1;
	}
	if(timeslot&0x2)	
	{
		currentmin = AGC_RED_LED_CURREN_MIN;
		currentmax = AGC_RED_LED_CURREN_MAX;
		tiagainmax = RED_GAIN_MAX_LV - 1;
	}
	
	// The latest data
	if((led->status & 0x1) != 1)	// no proper tia 
	{
		if((latest_dat < AGC_PPG_THRESHOLD_LV3) && (latest_dat >= -32765))
//		if(latest_dat < AGC_PPG_THRESHOLD_LV3)			//0120
		{
				if(	led->tia_cap_group < (tiagainmax - 1) )
				{
					// TIA 不是最高档，超过PPG上限，调高一档TIA
					led->tia_cap_group ++ ;
				}
				else
				{
					// TIA 已是最高档，固定在最高档，结束TIA调整，准备调整电流
					ZSPD_PRINTF("ppg = %d, Current = %d, TIA = 0x%x\n", latest_dat, led->led_cur, gains[led->tia_cap_group] ) ;
					return ZSPD_AGC_REACH_GAIN_MIN;
				}
		}
		else
		{
				led->status |= 0x1;
				ZSPD_PRINTF("TIA CAL Done!\n");
//				return ZSPD_AGC_REACH_GAIN_MAX;				
		}
	}
	else		// TIA OK, fine tune current
	{
		ZSPD_PRINTF("Current CAL start... %d\n", latest_dat);
		if((latest_dat > AGC_PPG_THRESHOLD_LV2) && (latest_dat < AGC_PPG_THRESHOLD_LV3))
		{
				led->status |= 0x2 ;
				return ZSPD_OK ;			
		}

		if((latest_dat > AGC_PPG_THRESHOLD_LV3) || (latest_dat < -32765) )//0120
//		if((latest_dat > AGC_PPG_THRESHOLD_LV3)
		{
			if(led->led_cur > currentmin )
				led->led_cur -=2 ;
			else
			{
				ZSPD_PRINTF("ppg = %d, Current = %d, TIA = 0x%x\n", latest_dat, led->led_cur, gains[led->tia_cap_group] ) ;
				
				BC_LOG_INFO("current min \n");
				return ZSPD_AGC_REACH_CURRENT_MIN ;	
				
			}		
		}
		if((latest_dat < AGC_PPG_THRESHOLD_LV2) && (latest_dat > -32765) )//0120
//		if(latest_dat < AGC_PPG_THRESHOLD_LV2) 
		{
			if(led->led_cur < currentmax)
				led->led_cur +=2 ;
			else
			{
				ZSPD_PRINTF("ppg = %d, Current = %d, TIA = 0x%x\n", latest_dat, led->led_cur, gains[led->tia_cap_group] ) ;
				BC_LOG_INFO("current max \n");
				return ZSPD_AGC_REACH_CURRENT_MAX ;	
			}				
		}		
	}

	ZSPD_PRINTF("Next Current = %d, TIA = 0x%x\n", led->led_cur, gains[led->tia_cap_group] ) ;
	return ZSPD_AGC_CAL_CONTINUE ;
}
#else
ZSPD_ERROR_CODE_T ZSPD_AgcCalProcess(int16_t latest_dat, uint8_t timeslot, ZSPD_LED_CFG_T *led)
{
	uint8_t currentmin, currentmax, tiagainmax ;
	if(timeslot&0x1)	
	{
		currentmin = AGC_IR_LED_CURREN_MIN;
		currentmax = AGC_IR_LED_CURREN_MAX;
		tiagainmax = IR_GAIN_MAX_LV - 1;
	}
	if(timeslot&0x2)	
	{
		currentmin = AGC_GREEN_LED_CURREN_MIN;
		currentmax = AGC_GREEN_LED_CURREN_MAX;
		tiagainmax = GREEN_GAIN_MAX_LV -1 ;
	}
	if(timeslot&0x4)	
	{
		currentmin = AGC_RED_LED_CURREN_MIN;
		currentmax = AGC_RED_LED_CURREN_MAX;
		tiagainmax = RED_GAIN_MAX_LV -1 ;
	}
	
	// The latest data
	if((led->status & 0x1) != 1)	// no proper tia 
	{
		if(latest_dat == AGC_PPG_THRESHOLD_LV4)
		{
				if(	led->tia_cap_group > 0 )
				{
					// TIA 不是最低档，饱和，调低一档TIA
					led->tia_cap_group -- ;
				}
				else
				{
					// TIA 已是最低档，固定在最低档，结束TIA调整，准备调整电流
					led->tia_cap_group = 0;
					led->status |= 0x1;
					ZSPD_PRINTF("TIA CAL Done\n" ) ;
					ZSPD_PRINTF("ppg = %d, Current = %d, TIA = 0x%x\n", latest_dat, led->led_cur, gains[led->tia_cap_group] ) ;
//					return ZSPD_AGC_REACH_GAIN_MIN;
				}
		}
		else
		{
			//	如果在当前TIA档不饱和，且TIA档不是最高档，调高一档，并结束TIA调整
			if(led->tia_cap_group < tiagainmax)
			{
				led->tia_cap_group++ ;
				led->status |= 0x1;
				ZSPD_PRINTF("TIA CAL Done!\n");
			}
			else
					return ZSPD_AGC_REACH_GAIN_MAX;				
		}
	}
	else		// TIA OK, fine tune current
	{
		ZSPD_PRINTF("Current CAL start... %d\n", latest_dat);
		if((latest_dat > AGC_PPG_THRESHOLD_LV2) && (latest_dat < AGC_PPG_THRESHOLD_LV3))
		{
				led->status |= 0x2 ;
				return ZSPD_OK ;			
		}

		if(latest_dat > AGC_PPG_THRESHOLD_LV3)
		{
			if(led->led_cur > currentmin )
				led->led_cur -=2 ;
			else
			{
				ZSPD_PRINTF("ppg = %d, Current = %d, TIA = 0x%x\n", latest_dat, led->led_cur, gains[led->tia_cap_group] ) ;
				return ZSPD_AGC_REACH_CURRENT_MIN ;	
			}		
		}
		if(latest_dat < AGC_PPG_THRESHOLD_LV2) 
		{
			if(led->led_cur < currentmax)
				led->led_cur +=2 ;
			else
			{
				ZSPD_PRINTF("ppg = %d, Current = %d, TIA = 0x%x\n", latest_dat, led->led_cur, gains[led->tia_cap_group] ) ;
				return ZSPD_AGC_REACH_CURRENT_MAX ;	
			}				
		}		
	}

	ZSPD_PRINTF("Next Current = %d, TIA = 0x%x\n", led->led_cur, gains[led->tia_cap_group] ) ;
	return ZSPD_AGC_CAL_CONTINUE ;
}
#endif

ZSPD_ERROR_CODE_T ZSPD_GreenAgcCalProcess(uint8_t data_size, int16_t *buf)
{
	ZSPD_ERROR_CODE_T ret;
	
	// the latest green PPG data
	int16_t rawdat = buf[data_size-1];
	ret = ZSPD_AgcCalProcess(rawdat, TSA_REG_SEL, &led_cfg[1]) ;
	if(ret != ZSPD_OK)
		ZSPD_AgcCalSearchCurrentSetting(TSA_REG_SEL,led_cfg) ;
//	else
//	{
//		BC_LOG_INFO("agcOK tia=%d ,led=%d \n",led_cfg[1].tia_cap_group,led_cfg[1].tia_cap_group);
//	}
	return ret;
}

ZSPD_ERROR_CODE_T ZSPD_IrRedAgcCalProcess(uint8_t data_size, int16_t *buf)
{
	ZSPD_ERROR_CODE_T retr,reti;
	// the latest IR data
	int16_t rawdat = buf[data_size-1];
	if(led_cfg[0].status != 3)
	{
		ZSPD_PRINTF("ZSPD_AgcCalProcess for IR\n");		
		reti = ZSPD_AgcCalProcess(rawdat, TSC_REG_SEL, &led_cfg[0]) ;
	}
	else
		reti = ZSPD_OK;
	ZSPD_PRINTF("IR AGC...reti = %d\n", reti);
	
	// the latest RED PPG data
	rawdat = buf[data_size-2];
	if(led_cfg[2].status != 3)
	{
		ZSPD_PRINTF("ZSPD_AgcCalProcess for RED\n");		
		retr = ZSPD_AgcCalProcess(rawdat, TSB_REG_SEL, &led_cfg[2]) ;
	}
	else
		retr = ZSPD_OK;
	ZSPD_PRINTF("RED AGC...retr = %d\n", retr);
	
	if((reti == ZSPD_OK)  && (retr == ZSPD_OK))	return ZSPD_OK;	
	
	if( ((reti == ZSPD_AGC_CAL_CONTINUE) && (retr == ZSPD_OK)) || \
		((retr == ZSPD_AGC_CAL_CONTINUE) && (reti == ZSPD_OK)) || \
		((retr == ZSPD_AGC_CAL_CONTINUE) && (reti == ZSPD_AGC_CAL_CONTINUE)) )
	{
		ZSPD_AgcCalSearchCurrentSetting(TSB_REG_SEL | TSC_REG_SEL, led_cfg) ;
		return ZSPD_AGC_CAL_CONTINUE ;
	}
	
	if((retr != ZSPD_AGC_CAL_CONTINUE) && (retr != ZSPD_OK))	return retr;	// red led agc fail
	if((reti != ZSPD_AGC_CAL_CONTINUE) && (reti != ZSPD_OK))	return reti;	// ir agc fail	
	
	// add to remove compile errors
	return ZSPD_OK ;
}

extern uint64_t ttime;
extern void power_manage(void);

/**
*function:zspd4000 data processing
*parameter:none
*return:none
*date:2022/05/20
**/
static uint32_t temp_count = 0;
bool ZSPD4000_DataHandle(void)
{
	uint8_t data_size,bytehl[2] = {0};
	uint8_t i, retry = 3;
	static uint8_t firstin=0;
	int32_t *p;
	
	static uint32_t tslot1, tslot2=0;
	
	ZSPD_ERROR_CODE_T ret;

	uint16_t num;
	int16_t tmp; 
	static uint32_t size=2500;
	if(zspd_int_flag)
	{	
		zspd_int_flag = 0;	
		data_size = 0;
//		while(retry--)
//		{
//			data_size = ZSPD4000_ReadFifoCount() ;
//			if(data_size)
//				break ;
//		}
		
		data_size = ZSPD4000_ReadFifoCount() ;
		
		if(data_size <= 2)
			return false;

		if( IR_RED_NORMAL == zspd_satus)
			data_size = (data_size / 4 * 4 ) ; // SPO2, must 4 aligned
		
		if(zspd_satus == IR_RED_AGC_CAL)	//2025年1月20日
		{
			if(data_size < FIFO_DEPTH_AGC_PROCESS)
			{
				BC_LOG_BLE("data size <= 4\n");
				return false;
			}
			else
				data_size = FIFO_DEPTH_AGC_PROCESS;
		}
		
		
//		printf("data_size = %d\n", data_size);
		// read out data to clear ZSPD interrupt flag
		if(data_size > 2 && data_size <= ZSPD4000_FIFO_MAX_DEPTH/2){
			ZSPD4000_ReadFifoData (data_size, zapd_raw_dat); 
						
//			BC_LOG_BLE("zspd_satus:%d  data_size:%d\r\n",zspd_satus,data_size);
//			BC_LOG_INFO("\n zspd FIFOFIFOFIFO\n\n");
//			for(i=0;i<data_size;i++)
//			{
//				printf("%6x ", zapd_raw_dat[i]);
//			}
//			BC_LOG_HEX_P("zspd FIFOFIFOFIFO", (uint8_t*)zapd_raw_dat ,data_size*2);
		}

		else
		{
			printf("FIFO read length out of range\n");
			BC_LOG_BLE("FIFO read length out of range\n");			
			return false;
		}
		printf("zspd_satus:%d\r\n",zspd_satus);
		BC_LOG_BLE("zspd_satus:%d  data_size:%d\r\n",zspd_satus,data_size);
		
//		return true;
		
		switch(zspd_satus)
		{
			/*
			case AGC_OFFSET_CAL:
				ZSPD4000_SetPpgAdcOffset(data_size, zapd_raw_dat);
				ZSPD_IRDeteLowFs() ;
				zspd_satus = IR_DETEC_LOW_FS;
				ZSPD_PRINTF("IR_DETEC_LOW_FS\n");
				break;
			*/
			
			case IR_DETEC_LOW_FS:
				zspd_agc_comp_flg =0;					//2025年1月20日
				BC_LOG_BLE("IR detec LOWFS IRDATA:%d \r\n",zapd_raw_dat[0]);
				if(ZSPD_OK == ZSPD_IRDetecProcess(data_size, zapd_raw_dat))//2025年1月20日
//				if(1)
				{
					
					if((HR_MODE == workmode) | (HRV_MODE == workmode))
					{
#ifdef HIGH_PERFORMANCE
						led_cfg[1].led_cur = AGC_GREEN_LED_CURREN_MAX;
						led_cfg[1].tia_cap_group = 1 - 1 ;
						led_cfg[1].status = 0;
#else
						led_cfg[1].led_cur = AGC_GREEN_LED_CURREN_MAX;
						led_cfg[1].tia_cap_group = GREEN_GAIN_MAX_LV - 1 ;
						led_cfg[1].status = 0;
#endif						
						ZSPD4000_CommonInit();
						ZSPD4000_SetSampleRate(DEFAULT_AGC_CAL_SAMPLE_FS) ;	// start with IR @ 100Hz
						ZSPD4000_SetFifoDepth	(FIFO_DEPTH_AGC_PROCESS) ;	// 4 aligned
						ZSPD_AgcCalSearchCurrentSetting(TSA_REG_SEL, led_cfg);	// Timeslot A green
						zspd_satus = GREEN_AGC_CAL;
					}
					if(SPO2_MODE == workmode)
					{
#ifdef HIGH_PERFORMANCE
						led_cfg[0].led_cur = AGC_IR_LED_CURREN_MAX;
						led_cfg[0].tia_cap_group = 1 - 1 ;
						led_cfg[0].status = 0;
						led_cfg[2].led_cur = AGC_RED_LED_CURREN_MAX;
						led_cfg[2].tia_cap_group = 1 - 1 ;
						led_cfg[2].status = 0;
#else						
						led_cfg[0].led_cur = AGC_IR_LED_CURREN_MAX;
						led_cfg[0].tia_cap_group = IR_GAIN_MAX_LV - 1 ;
						led_cfg[0].status = 0;
						led_cfg[2].led_cur = AGC_RED_LED_CURREN_MAX;
						led_cfg[2].tia_cap_group = RED_GAIN_MAX_LV - 1 ;
						led_cfg[2].status = 0;
#endif
						ZSPD4000_CommonInit();
						ZSPD4000_SetSampleRate(DEFAULT_AGC_CAL_SAMPLE_FS) ;	// start with IR @ 100Hz
						ZSPD4000_SetFifoDepth	(FIFO_DEPTH_AGC_PROCESS) ;	// 4 aligned
						ZSPD_AgcCalSearchCurrentSetting(TSB_REG_SEL | TSC_REG_SEL, led_cfg);	// timeslot A+C
						zspd_satus = IR_RED_AGC_CAL;
						
						BC_LOG_BLE("IR RED AGC BEGAIN\r\n",zspd_satus,data_size);
					}
				}
				break;
				
			case GREEN_AGC_CAL:	
				ret = ZSPD_GreenAgcCalProcess(data_size, zapd_raw_dat);
				ZSPD_PRINTF("ZSPD_GreenAgcCalProcess = %d\n", ret);
			
				if( ZSPD_OK == ret){
					if(HR_MODE == workmode)
					{
						ZSPD4000_CommonInit();
						ZSPD_GreenNormal(SAMPLE_FS_25HZ);	// HRS 25Hz
						
					}
					else if(HRV_MODE == workmode)
					{
						ZSPD4000_CommonInit();
						ZSPD_GreenNormal(SAMPLE_FS_100HZ);	// HRS 100Hz
					}				
					zspd_agc_comp_flg =1;							
					zspd_satus = GREEN_NORMAL;	
					BC_LOG_INFO("agcOK tia=%d ,led=%d \n",led_cfg[1].tia_cap_group,led_cfg[1].tia_cap_group);					
				}						
				// no wear, turn off green, turn on IR, then goto low FS IR detec mode
				else if(ZSPD_AGC_CAL_CONTINUE != ret)
				{
					ZSPD4000_CommonInit();
					ZSPD_IRDeteLowFs() ;
					zspd_satus = IR_DETEC_LOW_FS;
				}
				break;
				
			case IR_RED_AGC_CAL:	
				ret = ZSPD_IrRedAgcCalProcess(data_size, zapd_raw_dat);
				ZSPD_PRINTF("ZSPD_IrRedAgcCalProcess = %d\n", ret);
			
		  	
			
				if( ZSPD_OK == ret){
					ZSPD4000_CommonInit();
					ZSPD_IrRedNormal(SAMPLE_FS_100HZ);
					zspd_satus = IR_RED_NORMAL;		
					zspd_agc_comp_flg =1;							
				}						
				// no wear, turn off green, turn on IR, then goto low FS IR detec mode
				else if(ZSPD_AGC_CAL_CONTINUE != ret)
				{
					ZSPD4000_CommonInit();
					ZSPD_IRDeteLowFs() ;
					zspd_satus = IR_DETEC_LOW_FS;
//					BC_LOG_BLE("IR RED AGC IRDATA:%d  REDDATA:%d\r\n",zapd_raw_dat[3],zapd_raw_dat[2]);
//					//0120
//					ZSPD4000_CommonInit();							
//					ZSPD_IrRedNormal(SAMPLE_FS_100HZ);
//					zspd_satus = IR_RED_NORMAL;	
//					zspd_agc_comp_flg =1;		
					break;
					
				}
				BC_LOG_BLE("IR RED AGC continue IRDATA:%d  REDDATA:%d\r\n",zapd_raw_dat[3],zapd_raw_dat[2]);
				break;				
				
			case GREEN_NORMAL:				
				ppgdatready = data_size;	// fresh new data length
				  temp_count++;
				BC_LOG_INFO("data_size = %d  emp_count = %d\r\n",data_size,temp_count);
				in.data_length = Hr_Dhr_data_process(zapd_raw_dat,ppgdatready,0);
				in.data_length *=2;
				if(temp_count >= 2)
				{
					if(green_data_callback != NULL)
					{
						BC_LOG_INFO("data_size = %d \r\n",data_size);
						green_data_callback(zapd_raw_dat,NULL,data_size);
					}
				}
//				in.data_length = ppgdatready*2;		//0120
				in.green_data = zapd_raw_dat;
//				in.pd_nums=1;
				in.ptr_x = 0x01;
				
				out.waveform = wave_out;
				
				ZSBM_AlgoUpdateData(zspd_algo_buffer,&in,&out);
				if(out.heart_rate_refreshed)
				{
					uint16_t hr_result = out.heart_rate;
//					printf("\r\n----->hr_result:%d <--------------\r\n",hr_result);
					if(hr_result_callback  != NULL)
					{
						hr_result_callback(out.heart_rate,out.hrv1);
					};
				}
				BC_LOG_INFO("ppg_signal_strength = %d\tstrenth_refresh = %d\tlevel = %d\n",\
				out.ppg_signal_strength,out.ppg_signal_strength_refreshed,out.ppg_signal_level);
				uint16_t temp_tsx = 0;
//				ZSPD_READ_REG16(0x23, &temp_tsx, 1) ;
//				zspd_ware_status = wear_check__process(zspd_algo_init_para,&in,&out,1,ZSPD_TYPE);
				BC_LOG_INFO("zspd_ware_status = %d \r\n",zspd_ware_status.result_wear_statue);
				if(hr_signal_check_callback != NULL)
				{
//					if(zspd_ware_status.result_wear_statue == WEAR_NONE )
//					{
//						hr_signal_check_callback(0);
//					}
//					else if(zspd_ware_status.result_wear_statue == WEAR_NICE)
//					{
						hr_signal_check_callback(1);
//					}	
				}
//				if(hr_signal_check_callback != NULL && out.ppg_signal_strength_refreshed == 1)
//				{
//					hr_signal_check_callback(out.ppg_signal_strength);
//				}
				
				break; 
			
			case IR_RED_NORMAL:				
				ppgdatready = data_size;	// fresh new data length
			    in.data_length = ppgdatready*2;
			    in.ptr_x = 6;
			
			    zsbm_convert_red_ir_data_format(zapd_raw_dat, &in.data_length, 32, &in.red_data, &in.ir_data);
//				BC_LOG_HEX_P("in.red_data in",in.red_data ,in.data_length);
//				BC_LOG_HEX_P("in.ir_data  in",in.ir_data ,in.data_length);
			
			    in.data_length = SPO2_data_processingV2(in.red_data, in.data_length,in.ir_data, in.data_length,0);  			    
//				BC_LOG_HEX_P("in.red_data out",in.red_data ,in.data_length);
//				BC_LOG_HEX_P("in.ir_data  out",in.ir_data ,in.data_length);
		    
				out.waveform = wave_out;
				temp_count++;
				BC_LOG_INFO("in.data_length = %d  emp_count = %d\r\n",in.data_length,temp_count);
				if(temp_count > 2)
				{
					if(red_and_ir_data_callback != NULL)
					{
						BC_LOG_INFO("in.data_lengthlllllll = %d  emp_count = %d\r\n",in.data_length,temp_count);
						bc_delay_ms(20);
						red_and_ir_data_callback(in.red_data,in.data_length/4,in.ir_data,in.data_length/4,NULL);
					}
					ZSBM_AlgoUpdateData(zspd_algo_buffer,&in,&out);
				}
				BC_LOG_INFO("ppg_signal_strength = %d\tstrenth_refresh = %d\tlevel = %d\n",\
				out.ppg_signal_strength,out.ppg_signal_strength_refreshed,out.ppg_signal_level);
				if(out.spo2_refreshed)
				{
					uint16_t spo2_result = out.spo2;
//					printf("\r\n----->spo2_result:%d <--------------\r\n",spo2_result);
					if(spo2_result_callback  != NULL)
					{
						spo2_result_callback(out.spo2,out.heart_rate);
					}

				}
//				ZSPD_READ_REG16(0x23, &temp_tsx, 1) ;
//				zspd_ware_status = wear_check__process(zspd_algo_init_para,&in,&out,32,ZSPD_TYPE);
				BC_LOG_INFO("zspd_ware_status = %d \r\n",zspd_ware_status.result_wear_statue);
				if(spo2_signal_check_callback != NULL )
				{
//					if(zspd_ware_status.result_wear_statue == WEAR_NONE)
//					{
//						spo2_signal_check_callback(0);
//					}
//					else if(zspd_ware_status.result_wear_statue == WEAR_NICE)
//					{
						spo2_signal_check_callback(1);
//					}
					
				}
//				if(spo2_signal_check_callback != NULL && out.ppg_signal_strength_refreshed == 1)
//				{
//					spo2_signal_check_callback(out.ppg_signal_strength);
//				}
				break;
			case FT_GREEN_RED_IR_NORMAL:
				for(i=0; i<data_size/3; i++)
				{
					//BC_LOG_INFO("%d %d %d\n", (uint32_t)zapd_raw_dat[3*i+0], (uint32_t)zapd_raw_dat[3*i+1], (uint32_t)zapd_raw_dat[3*i+2]) ;// 绿光、红光、红外数据
					if(gary_card_callback != NULL)
					{
						gary_card_callback(&zapd_raw_dat[3*i+0],&zapd_raw_dat[3*i+1],&zapd_raw_dat[3*i+2]);
					}
				}
				break; 
			
			case ECG_NORMAL:
				break; 
				
			default:
				break ;
		}
	}
	return true;
}

void ZSPD_TestGreenRedIr(void)
{
	ZSPD4000_SetSampleRate(SAMPLE_FS_25HZ) ;	// start with IR @ 25Hz
	ZSPD4000_SetFifoDepth(FIFO_DEPTH_HRS_HRV) ;	

	ZSPD4000_SetLedCurrent(Z_GREEN_CH, 15) ;
	ZSPD4000_SetTIAGain(TIA_50K_CAP_4P74);
	ZSPD4000_SetFifoWordSize(FIFO_WORD_SIZE_16);
	ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx	(0, 0, 0) ;


	ZSPD4000_SetLedCurrent(Z_RED_CH, 15) ;
	ZSPD4000_SetTIAGain(TIA_50K_CAP_4P74);
	ZSPD4000_SetFifoWordSize(FIFO_WORD_SIZE_16);
	ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx	(0, 0, 0) ;
	

	ZSPD4000_SetLedCurrent(Z_IR_CH, 15) ;
	ZSPD4000_SetTIAGain(TIA_50K_CAP_4P74);
	ZSPD4000_SetFifoWordSize(FIFO_WORD_SIZE_16);
	ZSPD4000_SetSubPeriod_NumIntTsx_NumRepeatTsx	(0, 0, 0) ;

	ZSPD4000_ClearFifo();

	ZSPD4000_TimerSlotStart(TSA_EN | TSB_EN  | TSC_EN );
}

ZSPD_ERROR_CODE_T ZSPD4000_Init(WORK_MODE_T mode)
{
	workmode = mode;
	if(NULL_MODE == workmode){
		ZSPD4000_GpioIntConfig(0);
		ZSPD4000_SoftReset();	
		return ZSPD_OK;
	}
	
	if(ECG_MODE == workmode)
	{
//		ZSPD4000_EcgInit();
		return ZSPD_OK;
	}
	// 绿+红光+红外，15mA电流，50K增益，25Hz，三TS
	if(FT_GRI_MODE == workmode)
	{
		ZSPD4000_CommonInit();
		zspd_satus = FT_GREEN_RED_IR_NORMAL ;
		ZSPD_TestGreenRedIr() ;
		return ZSPD_OK;
	}
	
	ZSPD4000_CommonInit();
	zspd_satus = IR_DETEC_LOW_FS ;
	ZSPD4000_ClearFifo();				//202550121添加
	ZSPD_IRDeteLowFs() ;
	return ZSPD_OK;
}

void ZSPD4000_DumpRegister( void)
{
	uint16_t regs[0x25] ;
	uint8_t i;
	ZSPD_PRINTF("Dump register:\r\n" );
	ZSPD4000_ReadMultyWord(0, &regs[0], 0x24);
	for(i=0;i<0x25;i++)
	{
		ZSPD_PRINTF("regs[%x] = 0x%x \n", i, regs[i]);
	}
	ZSPD_PRINTF("Dump register done!\r\n" );
}






bool ZSPD4000_algo_init_para(void *init_para)
{
	if(init_para == NULL)
	{
		return false;
	}
	zspd_algo_init_para = init_para;
	return true;
}

void ZSPD4000_unint(void)
{
	ZSBM_AlgoFree();
    ZSPD4000_Init(NULL_MODE);
	temp_count = 0;
}

void ppg_ir_flag_set(bool flag)
{
	ppg_ir_flag = flag;
}

bool ZSPD4000_hr_data_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	green_data_callback = (ppg_green_data_callback)function_callback;
	return true;
}

bool ZSPD4000_spo2_data_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	red_and_ir_data_callback = (ppg_red_and_ir_data_callback)function_callback;
	return true;
}



bool ZSPD4000_hr_result_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	hr_result_callback = (ppg_hr_result_callback)function_callback;
	return true;
}

bool ZSPD4000_spo2_result_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	spo2_result_callback = (ppg_spo2_result_callback)function_callback;
	return true;
}

bool ZSPD4000_spo2_signal_check_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	spo2_signal_check_callback = (ppg_signal_check_callback)function_callback;
	return true;
}

bool ZSPD4000_hr_signal_check_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	hr_signal_check_callback = (ppg_signal_check_callback)function_callback;
	return true;
}

bool ZSPD4000_gary_card_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	gary_card_callback = (ppg_gary_card_callback)function_callback;
	return true;
}

bool zspd400_agc_comp_flg(void)
{
	return zspd_agc_comp_flg;
}

/***************************************************************************
* 函数名: uint16_t SPO2_data_processing(int32_t * fifo_data, uint16_t len,int32_t ** ts_b_data,int32_t ** ts_c_data,uint8_t clear_first_in)
* 说明功能: 将数据滤波处理后存储到原FIFO指针地址
* 配置说明:
						int32_t * a							ZSPD4000FIFO数据输入首地址
						uint16_t len						数据长度，数据长度应是数个TS的数据，比如2个TS应该含有2个红光点位和两个红外点位，函数不能够处理单个奇数个点位的数据，比如2个红光点位+1个红外点位
						int32_t * ts_b_data			指向数据转换后的红光数据存储地址
						int32_t * ts_c_data			指向数据转换后红外数据存储地址
						uint8_t clear_first_in	是否清除首次处理标志   写1则主动清除标志，函数会自动存储收到的第一组数据，从第二组数据开始按串口发送
 * fifo_data
 *    |
 *    V
 * {red_1, ir_1, red_2, ir_2 ... ... red_n, ir_n}  to
 * out_red                 out_ir
 *    |                       |
 *    V                       V
 * {red_1, red_2 ... red_n, ir_1, ir_2 ... ir_n}

* 返回值: 红外数据的点位长度，数据位宽32bit,数据长度位宽16bit
***************************************************************************/
uint16_t SPO2_data_processing(int32_t * fifo_data, uint16_t len,void ** ts_b_data,void ** ts_c_data,uint8_t clear_first_in)
{
	uint16_t i=0,j = 0,n=0,leng_data=0;
	int32_t data_process_temp_sent_data[64] = {0},data_process_temp_sent_data_rew[64] = {0};


	
	
	if(clear_first_in)
	{
		first_flg = 1;
		write_point = 0;
	}
	
	if(first_flg)			//第一次读数据
	{
//		memcpy((uint8_t *)&data_process_temp[write_point],(uint8_t *) fifo_data,len*4);
//		write_point += len;
		
		for(i=0;i<len;i++)
		{
			data_process_temp[write_point] = fifo_data[i];
			write_point++;
		}
		first_flg = 0;
		return 0;
	}
	else
	{
//		memcpy((uint8_t *)&data_process_temp[write_point],(uint8_t *) fifo_data,len*4);
//		write_point += len;
		for(i=0;i<len;i++)				//fifo原数据存储
		{
			data_process_temp[write_point] = fifo_data[i];
			write_point++;
		}
		
		
//		memcpy((uint8_t *)&data_process_temp_sent_data[write_point],(uint8_t *) data_process_temp,len*4);
		for(i=0;i<write_point;i++)			//FIFO数据提取
		{
			data_process_temp_sent_data[i] =data_process_temp[i];
		}
		
		leng_data = write_point*4;
		
		zsbm_convert_red_ir_data_format(data_process_temp_sent_data, &leng_data, 32, (void**)ts_b_data, (void**)ts_c_data);				//转换ZSPD的RED和IR的数据到rawforalgin数组中
		BC_LOG_HEX_P("in.red_data before",*ts_b_data ,leng_data);
		BC_LOG_HEX_P("in.ir_data before", *ts_c_data ,leng_data);
		
		
		zsbm_algo_fix_jump_point_v1(*ts_b_data, write_point/2, 100) ;			//按32位进行数据处理
		
		zsbm_algo_fix_jump_point_v1(*ts_c_data, write_point/2, 100) ;			//按32位进行数据处理
		
		BC_LOG_HEX_P("in.red_data after",*ts_b_data ,leng_data);
		BC_LOG_HEX_P("in.ir_data  after", *ts_c_data ,leng_data);
		
		
//		zsbm_algo_fix_jump_point_v1(*ts_b_data, write_point/2, 100) ;			//按32位进行数据处理
//		
//		zsbm_algo_fix_jump_point_v1(*ts_c_data, write_point/2, 100) ;			//按32位进行数据处理
//		
//		BC_LOG_HEX_P("in.red_data after2",*ts_b_data ,leng_data);
//		BC_LOG_HEX_P("in.ir_data  after2", *ts_c_data ,leng_data);
		
		
//		memcpy((uint8_t *)fifo_data,(uint8_t *) data_process_temp_sent_data,len*2);		//转移红光数据到当前FIFO
//		memcpy((uint8_t *)fifo_data[len*2],(uint8_t *) data_process_temp_sent_data[len*2],len*2);		//转移红光数据到当前FIFO
		
		for(i =0;i<len/2;i++)						//替换当前FIFO数据
		{
			fifo_data[i] = data_process_temp_sent_data[i];
			fifo_data[i+len/2] = data_process_temp_sent_data[i+write_point/2];
			
		}
		
		*ts_b_data = fifo_data;
		*ts_c_data = fifo_data+(len/2);
		
		
//		memcpy((uint8_t *)data_process_temp,(uint8_t *) data_process_temp[len*2],( write_point - len)*4);		//转移红光数据到当前FIFO
		
		for(i = len;i< write_point ;i++)
		{
			data_process_temp[i-len] = data_process_temp[i];
		}
		
		write_point = write_point - len;
		
		return len/2;
	}
	
}


uint16_t SPO2_data_processingV2(uint8_t * red_data, uint16_t len_red,uint8_t * ir_data, uint16_t len_ir,uint8_t clear_first_in)
{
	
	static uint8_t first_flg =1;
	uint16_t i=0,j = 0,n=0,leng_data=0;
	int32_t data_process_temp_sent_data[64] = {0};
	static int32_t data_temp_convert_red[32] = {0};
	static int32_t data_temp_convert_ir[32] = {0};

	static uint16_t write_pointer_red = 0;
	static uint16_t write_pointer_ir = 0;
	
	
	if(clear_first_in)
	{
		first_flg = 1;
		write_pointer_ir = 0;
		write_pointer_red = 0;
	}
	
	memcpy((uint8_t *)& data_temp_convert_red[write_pointer_red],(uint8_t *) red_data,len_red);
	write_pointer_red += len_red/4;
	
	memcpy((uint8_t *)& data_temp_convert_ir[write_pointer_ir],(uint8_t *) ir_data,len_ir);
	write_pointer_ir += len_ir/4;
	
	
	if(first_flg)			//第一次读数据			//存放首组数据
	{
		first_flg = 0;
		return 0;
	}
	else					//非第一次读取
	{
//		BC_LOG_HEX_P("in.red_data before",(uint8_t *)data_temp_convert_red ,write_pointer_red*4);
//		BC_LOG_HEX_P("in.ir_data before", (uint8_t *)data_temp_convert_ir ,write_pointer_ir*4);
		
		
		zsbm_algo_fix_jump_point_v1(data_temp_convert_red, write_pointer_red, 100) ;			//按32位进行数据处理
		
		zsbm_algo_fix_jump_point_v1(data_temp_convert_ir, write_pointer_ir, 100) ;			//按32位进行数据处理
		
//		BC_LOG_HEX_P("in.red_data before",(uint8_t *)data_temp_convert_red ,write_pointer_red*4);
//		BC_LOG_HEX_P("in.ir_data before", (uint8_t *)data_temp_convert_ir ,write_pointer_ir*4);
		
			
		memcpy((uint8_t *) red_data,(uint8_t *)data_temp_convert_red,len_red);
		memcpy((uint8_t *) ir_data,(uint8_t *)data_temp_convert_ir,len_ir);
		
		
		
		
		memcpy((uint8_t *)data_temp_convert_red,(uint8_t *) &data_temp_convert_red[len_red/4],( write_pointer_red*4 - len_red));		//推FIFO
		memcpy((uint8_t *)data_temp_convert_ir,(uint8_t *) &data_temp_convert_ir[len_ir/4],( write_pointer_ir*4 - len_ir));		//推FIFO

		write_pointer_red = write_pointer_red - len_red/4;
		write_pointer_ir = write_pointer_ir - len_ir/4;
		
		return len_ir;
	}
	
}




/***************************************************************************
* 函数名: uint16_t Hr_Dhr_data_process(int16_t * fifo_data, uint16_t len,uint8_t clear_first_in)
* 说明功能: 将数据滤波处理后存储到原FIFO指针地址
* 配置说明:
						int16_t * a							ZSPD4000FIFO数据输入首地址 数据位宽16bit
						uint16_t len						数据长度，数据长度应是数个TS的数据，比如2个TS应该含有2个红光点位和两个红外点位，函数不能够处理单个奇数个点位的数据，比如2个红光点位+1个红外点位
						uint8_t clear_first_in	是否清除首次处理标志   写1则主动清除标志，函数会自动存储收到的第一组数据，从第二组数据开始按串口发送
 * fifo_data
 *    |
 *    V
 * {red_1, ir_1, red_2, ir_2 ... ... red_n, ir_n}  to
 * out_red                 out_ir
 *    |                       |
 *    V                       V
 * {red_1, red_2 ... red_n, ir_1, ir_2 ... ir_n}

* 返回值: 处理好的数据长度 数据位宽16bit 数据长度位宽 16bit
***************************************************************************/
uint16_t Hr_Dhr_data_process(int16_t * fifo_data, uint16_t len,uint8_t clear_first_in)
{
	uint16_t i=0,j = 0,n=0,leng_data=0;
	static int16_t data_process_hr_temp[50] = {0};
	static int16_t hr_write_point = 0;
	static uint8_t	hr_first_flg = 1;
	
	if(clear_first_in)
	{
		hr_first_flg = 1;
		hr_write_point = 0;
	}
	
	memcpy((uint8_t *)& data_process_hr_temp[hr_write_point],(uint8_t *) fifo_data,len*2);
	hr_write_point += len;
	
	
	if(hr_first_flg)			//第一次读数据
	{
		hr_first_flg = 0;
		return 0;
	}
	else
	{

		zsbm_algo_fix_jump_point_v1_16bit(data_process_hr_temp, hr_write_point, 20) ;			//按16位进行数据处理
		
		memcpy((uint8_t *) fifo_data,(uint8_t *)data_process_hr_temp,len*2);
		

		memcpy((uint8_t *)data_process_hr_temp,(uint8_t *) &data_process_hr_temp[len],( hr_write_point - len)*2);		//推FIFO

		
		hr_write_point = hr_write_point - len;
		
		return len;
	}



}


//去单点噪声滤波算法 32bit
void zsbm_algo_fix_jump_point_v1(int32_t *data, int32_t length, int32_t thres) {
  if (length < 3)
    return;

  int32_t d0 = 0, d1 = 0;
  int32_t i = 1;
  for (; i < length - 1; i++) {
    d0 = ABS(data[i] - data[i - 1]);
    d1 = ABS(data[i + 1] - data[i - 1]);
    if (d0 > thres) {
      if (d1 > (thres << 1) && d1 > d0) {
        // data[i] = data[i - 1];
        continue;
        // } else if (d1 > thres) {
        //   // data[i - 1] = data[i];
        //   data[i - 1] = data[i] + (data[i - 1] > data[i] ? 1 : (-1)) * (thres / DELTA_EVA_COEF);
      } else {
        data[i] = (data[i - 1] + data[i + 1]) >> 1;
      }
    }
  }
  // i = length - 1;
  // d0 = ABS(data[i] - data[i - 1]);
  // if (d0 > thres) {
  //   // data[i] = data[i - 1];
  //   data[i] = data[i - 1] + (data[i] > data[i - 1] ? 1 : (-1)) * (thres / DELTA_EVA_COEF);
  // }
  return;
}

//去单点噪声滤波算法 16bit
void zsbm_algo_fix_jump_point_v1_16bit(int16_t *data, int16_t length, int16_t thres) {
  if (length < 3)
    return;

  int16_t d0 = 0, d1 = 0;
  int16_t i = 1;
 for (; i < length - 1; i++) {
    d0 = ABS(data[i] - data[i - 1]);
    d1 = ABS(data[i + 1] - data[i - 1]);
    if (d0 > thres) {
      if (d1 > (thres << 1) && d1 > d0) {
        // data[i] = data[i - 1];
        continue;
        // } else if (d1 > thres) {
        //   // data[i - 1] = data[i];
        //   data[i - 1] = data[i] + (data[i - 1] > data[i] ? 1 : (-1)) * (thres / DELTA_EVA_COEF);
      } else {
        data[i] = (data[i - 1] + data[i + 1]) >> 1;
      }
    }
  }
  // i = length - 1;
  // d0 = ABS(data[i] - data[i - 1]);
  // if (d0 > thres) {
  //   // data[i] = data[i - 1];
  //   data[i] = data[i - 1] + (data[i] > data[i - 1] ? 1 : (-1)) * (thres / DELTA_EVA_COEF);
  // }
  return;
}


