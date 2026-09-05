#ifndef _ZSPD4000_REGCFG_H
#define _ZSPD4000_REGCFG_H

  //reg 0x00
  #define SOFT_RESET						0x0001
  #define FIFO_CLR							0x0010 
  
  //reg 0x02
  #define TSA_EN							0x0001
  #define TSB_EN							0x0002
  #define TSC_EN							0x0004
  #define TSD_EN							0x0008
  #define OP_EN								0x0010
  #define OP_DIS							0x0000
  #define EXT_SYNC_EN						0x0020
  #define EXT_SYNC_GPIO0					0x0000
  #define EXT_SYNC_GPIO1					0x0040
  #define OSC32K_EN							0x0080
  #define OSC24M_CAL_EN						0x0100
  
  //reg 0x03
  #define TIMESLOT_PERIOD_H_50HZ			0x0000	
  #define TIMESLOT_PERIOD_H(N) 				(N & 0x00FF)
  #define CLK_SEL(N) 						((N & 0x0003) << 8)
  #define CLK_SEL_GPIO0 					0x0000
  #define CLK_SEL_GPIO1 					0x0400
  
  //reg 0x04
  #define TIMESLOT_PERIOD_L_50HZ			0x0280
  #define TIMESLOT_PERIOD_L(N) 				(N & 0xFFFF)	
  
  //reg 0x05
  #define OSC32K_CAL_DEFAULT				0x0168
  #define OSC32K_CAL(N) 					((N & 0x03FF))
  
  //reg 0x06
  #define INT_FIFO_THRHD_EN					0x0001
  #define INT_FIFO_UFLOW_EN					0x0002
  #define INT_FIFO_OFLOW_EN					0x0004
  #define INT_ADC_OV_EN						0x0008
  #define INT_TIA_OV_EN						0x0010
  #define INT_ACLEAR_FIFO_DIS				0x0040
  #define INT_ACLEAR_FIFO_EN				0x0000
  #define FIFO_OF_CONTINUE_WR_OLD			0x0080
  #define FIFO_OF_CONTINUE_WR_NEW			0x0000
  
  //reg 0x07 W1C
  #define INT_HLEV_TSA						0x0100
  #define INT_HLEV_TSB						0x0200
  #define INT_HLEV_TSC						0x0400
  #define INT_HLEV_TSD						0x0800
  #define INT_LLEV_TSA						0x1000
  #define INT_LLEV_TSB						0x2000
  #define INT_LLEV_TSC						0x4000
  #define INT_LLEV_TSD						0x8000
  
  //reg 0x08 W1C
  #define INT_FIFO_THRHD_CLR				0x0001
  #define INT_FIFO_OFLOW_CLR				0x0002
  #define INT_FIFO_UFLOW_CLR				0x0004
  #define INT_ADC_OV_TSA_CLR				0x0010
  #define INT_ADC_OV_TSB_CLR				0x0020
  #define INT_ADC_OV_TSC_CLR				0x0040
  #define INT_ADC_OV_TSD_CLR				0x0080
  #define INT_TIA_OV_TSA_CLR				0x0100
  #define INT_TIA_OV_TSB_CLR				0x0200
  #define INT_TIA_OV_TSC_CLR				0x0400
  #define INT_TIA_OV_TSD_CLR				0x0800
  
  //reg 0x09
  #define INT_LHLEV_TS_IO0					0x0000
  #define INT_HLEV_TSA_IO1					0x0100
  #define INT_HLEV_TSB_IO1					0x0200
  #define INT_HLEV_TSC_IO1					0x0400
  #define INT_HLEV_TSD_IO1					0x0800
  #define INT_LLEV_TSA_IO1					0x1000
  #define INT_LLEV_TSB_IO1					0x2000
  #define INT_LLEV_TSC_IO1					0x4000
  #define INT_LLEV_TSD_IO1					0x8000
  
  //reg 0x0A
  #define INT_FIFO_THRHD_IO0				0x0000
  #define INT_FIFO_THRHD_IO1				0x0001
  #define INT_FIFO_OFLOW_IO0				0x0000
  #define INT_FIFO_OFLOW_IO1				0x0002
  #define INT_FIFO_UFLOW_IO0				0x0000
  #define INT_FIFO_UFLOW_IO1				0x0004
  #define INT_ADC_OV_TS_IO0					0x0000
  #define INT_ADC_OV_TSA_IO1				0x0010
  #define INT_ADC_OV_TSB_IO1				0x0020
  #define INT_ADC_OV_TSC_IO1				0x0040
  #define INT_ADC_OV_TSD_IO1				0x0080
  #define INT_TIA_OV_TS_IO0					0x0000
  #define INT_TIA_OV_TSA_IO1				0x0100
  #define INT_TIA_OV_TSB_IO1				0x0200
  #define INT_TIA_OV_TSC_IO1				0x0400
  #define INT_TIA_OV_TSD_IO1				0x0800
  
  //reg 0x0B
  #define GPIO0_CFG_DIS						0x0000
  #define GPIO0_CFG_OUT_PP					0x0001
  #define GPIO0_CFG_OUT_OD					0x0002
  #define GPIO0_CFG_INTO					0x0003
  #define GPIO0_POL_NEG						0x0000
  #define GPIO0_POL_REVERSAL				0x0004
  #define GPIO0_DS_12MA						0x0000
  #define GPIO0_DS_16MA						0x0008
  #define GPIO0_OUT_SOURCE_LOW				0x0000
  #define GPIO0_OUT_SOURCE_HIGH				0x0010
  #define GPIO0_OUT_SOURCE_LOW_CLK			0x0020
  #define GPIO0_OUT_SOURCE_HIGH_CLK			0x0030
  #define GPIO0_OUT_SOURCE_INT				0x0040
  #define GPIO1_CFG_DIS						0x0000
  #define GPIO1_CFG_OUT_PP					0x0100
  #define GPIO1_CFG_OUT_OD					0x0200
  #define GPIO1_CFG_INTO					0x0300
  #define GPIO1_POL_NEG						0x0000
  #define GPIO1_POL_REVERSAL				0x0400
  #define GPIO1_DS_12MA						0x0000
  #define GPIO1_DS_16MA						0x0800
  #define GPIO1_OUT_SOURCE_LOW				0x0000
  #define GPIO1_OUT_SOURCE_HIGH				0x1000
  #define GPIO1_OUT_SOURCE_LOW_CLK			0x2000
  #define GPIO1_OUT_SOURCE_HIGH_CLK			0x3000
  #define GPIO1_OUT_SOURCE_INT				0x4000
  
  //reg 0x0C
  #define FIFO_THRESHOLD_DEFAULT			0x000B
  #define FIFO_THRESHOLD(N) 				((N & 0x003F))
  
  //reg 0x0D
  #define EXTCLK_DIV_RATIO_DEFAULT			0x0176
  #define EXTCLK_DIV_RATIO(N) 				((N & 0x0FFF))
  
  //reg 0x0E
  #define INP01_SLEEP_CON_FLOAT				0x0000
  #define SLEEP_PD0_CON_VC_PD1_FLOAT		0x0001
  #define SLEEP_PD1_CON_VC_PD0_FLOAT		0x0002
  #define SLEEP_PD01_CON_VC					0x0003
  #define SLEEP_PD01_CON_FLOAT				0x0004
  #define INTPS_SLEEP_CON_FLOAT				0x0000
  #define INTPS_SLEEP_CON_VC				0x0008
  #define VC_SLEEP_AVDD						(0x0000 << 3)
  #define VC_SLEEP_GND						(0x0001 << 3)
  #define VC_SLEEP_FLOAT					(0x0002 << 3)
  #define ADC_OV_WFIFO_EN					0x0040
  #define TIA_OV_WFIFO_EN					0x0080
  #define INP23_SLEEP_CON_FLOAT				0x0000
  #define SLEEP_PD2_CON_VC_PD3_FLOAT		0x0100
  #define SLEEP_PD3_CON_VC_PD2_FLOAT		0x0200
  #define SLEEP_PD23_CON_VC					0x0300
  #define SLEEP_PD23_CON_FLOAT				0x0400
  
  //reg 0x0F
  #define TS_REG_NOT_SEL					0x0000
  #define TSA_REG_SEL						0x0001
  #define TSB_REG_SEL						0x0002
  #define TSC_REG_SEL						0x0004
  #define TSD_REG_SEL						0x0008
  
  //reg 0x10
  #define FIFO_WORD_SIZE_16					0x0000
  #define FIFO_WORD_SIZE_32					0x0001
  #define INT_LLEV_EN						0x0002
  #define INT_HLEV_EN						0x0004
  #define LLEV_SHFT_CNT(N) 					((N & 0x000F) << 4)
  #define HLEV_SHFT_CNT(N) 					((N & 0x000F) << 8)
  
  //reg 0x11
  #define LOW_LEVEL(N) 						((N & 0xFFFF))
  
  //reg 0x12
  #define HIGH_LEVEL(N) 					((N & 0xFFFF))
  
  //reg 0x13 - 0x15
  #define LED_CURRENT_OFF					00
  #define LED_CURRENT(N) 					((N & 0x007F))
  #define LED_CURRENT_10F					10
  #define LED_CURRENT_25F					20
  #define LED_CURRENT_50F					32
  #define LED_CURRENT_75F					45
  #define LED_CURRENT_100F					57
  #define LED_CURRENT_200F					110
  
  //reg 0x16
  #define LED_OFFSET_DEFAULT 				0x0017
  #define LED_OFFSET(N) 					((N & 0x00FF))
  #define LED_MASK_DEFAULT 					0x0000
  #define LED_MASK(N) 						((N & 0x000F) << 8)
  
  //reg 0x17
  #define LED_WIDTH_DEFAULT 				0x0003
  #define LED_WIDTH(N) 						((N & 0x00FF))
  
  //reg 0x18
  #define PRECON_WIDTH_DEFAULT 				0x0007
  #define PRECON_WIDTH(N) 					((N & 0x00FF))
  #define INPD_PRE_CON_FLOAT				0x0000
  #define INPD_PRE_CON_VC					0x0100
  #define INPD_PRE_CON_TIA_VREF				0x0200
  #define INPD_PRE_CON_VCOM					0x0300
  
  #define INPD_ACT_CON_FLOAT				0x0000
  #define INPD_ACT_CON_VC					0x1000
  #define INPD_ACT_CON_TIA_VREF				0x2000
  #define INPD_ACT_CON_VCOM					0x3000
  
  
  //reg 0x19 - 0x1A
  #define INP02_PRE_ACT_CON_DEFAULT 		0x0042
  #define INP13_PRE_ACT_CON_DEFAULT 		0x0011
  #define INP01_PRE_CON_FLOAT				0x0000
  #define INP01_PRE_CON_VC					0x0001
  #define INP01_PRE_CON_TIA_VREF			0x0002
  #define INP01_PRE_CON_VCOM				0x0003
  #define INP0_PRE_CON_TIAN					0x0004
  #define INP1_PRE_CON_TIAP					0x0004
  #define INP0_PRE_CON_INP1					0x0008
  #define INP1_PRE_CON_TIAN					0x0008
  
  #define INP01_ACT_CON_FLOAT				0x0000
  #define INP01_ACT_CON_VC					0x0010
  #define INP01_ACT_CON_TIA_VREF			0x0020
  #define INP01_ACT_CON_VCOM				0x0030
  #define INP0_ACT_CON_TIAN					0x0040
  #define INP1_ACT_CON_TIAP					0x0040
  #define INP0_ACT_CON_INP1					0x0080
  #define INP1_ACT_CON_TIAN					0x0080
  
  
  #define INP23_PRE_CON_FLOAT				0x0000
  #define INP23_PRE_CON_VC					0x0100
  #define INP23_PRE_CON_TIA_VREF			0x0200
  #define INP23_PRE_CON_VCOM				0x0300
  #define INP2_PRE_CON_TIAN					0x0400
  #define INP3_PRE_CON_TIAP					0x0400
  #define INP2_PRE_CON_INP3					0x0800
  #define INP3_PRE_CON_TIAN					0x0800
  
  #define INP23_ACT_CON_FLOAT				0x0000
  #define INP23_ACT_CON_VC					0x1000
  #define INP23_ACT_CON_TIA_VREF			0x2000
  #define INP23_ACT_CON_VCOM				0x3000
  #define INP2_ACT_CON_TIAN					0x4000
  #define INP3_ACT_CON_TIAP					0x4000
  #define INP2_ACT_CON_INP3					0x8000
  #define INP3_ACT_CON_TIAN					0x8000
  
  //reg 0x1B
  #define TIA_EN							0x0001
  #define TIA_VREF_0P64V					0x0000
  #define TIA_VREF_0P85V					0x0002
  #define TIA_VREF_1P06V					0x0004
  #define TIA_VREF_1P26V					0x0006
  #define TIA_12K5_CAP_8P86					0x6600
  #define TIA_25K_CAP_6P32					0x3810
  #define TIA_50K_CAP_4P74					0x4020
  #define TIA_100K_CAP_3P16					0x2030
  #define TIA_200K_CAP_1P58					0x1040
  #define TIA_400K_CAP_960F					0x0650
  #define TIA_800K_CAP_480F					0x0360
  #define TIA_1M6_CAP_320F					0x0270
  
  //reg 0x1C
  #define ACL_EN							0x0001
  #define ACL_BYP_ON						0x0000
  #define ACL_BYP_OFF						0x0002
  #define INTG_EN							0x0004
  #define ACL_INTG_BYP_ON					0x0000
  #define ACL_INTG_BYP_OFF					0x0008
  #define INTG_AS_BUFF_ON					0x0010
  #define BUFF_GAIN_NEG3DB					0x0020
  #define INTG_INPUT_RES_680K				0x0000
  #define INTG_INPUT_RES_340K				0x0040
  #define INTG_INPUT_RES_170K				0x0080
  #define INTG_3DB							0x0100
  
  //reg 0x1D
  #define INTG_OFFSET_DEFAULT				0x0016
  #define INTG_OFFSET(N)					((N & 0x00FF))
  #define INTG_FINE_DEFAULT					0x0600
  #define INTG_FINE_OFFSET(N)				((N & 0x000F) << 8)
  
  //reg 0x1E
  #define INTG_WIDTH_DEFAULT				0x0004
  #define INTG_WIDTH(N)						((N & 0x00FF))
  
  //reg 0x1F
  #define REVERSE_INTG(N)					((N & 0x000F))
  #define SUBTRACTION(N)					((N & 0x00F0))
  
  #define VC_SEL_AVDD						0x0000
  #define VC_SEL_TIA_VREF					0x0100
  #define VC_SEL_TIA_VREF_250				0x0200
  #define VC_SEL_GND						0x0300
  
  #define VC_SEL_MOD_AVDD					0x0000
  #define VC_SEL_MOD_TIA_VREF				0x0400
  #define VC_SEL_MOD_TIA_VREF_250			0x0800
  #define VC_SEL_MOD_GND					0x0C00
  
  //reg 0x20
  #define MOD_OFFSET_DEFAULT				0x0013
  #define MOD_OFFSET(N)						(N & 0x00FF)
  
  //reg 0x21
  #define MOD_WIDTH_DEFAULT					0x0003
  #define MOD_WIDTH(N)						(N & 0x00FF)
  #define MOD_TYPE_NOT_MODULATION			0x0000
  #define MOD_TYPE_FLOAT					0x0100
  #define MOD_TYPE_NOT_FLOAT				0x0200
  
  //reg 0x22
  #define ADC_ADJUST_DEFAULT				0x0000
  #define ADC_ADJUST(N)						(N & 0xFFFF)
  
  //reg 0x23
  #define NUM_REPEAT(N)						(N & 0x00FF)
  #define MUM_INT(N)						((N & 0x000F) << 8)
  #define SUB_PERIOD(N)						((N & 0x000F) << 12)
  
  //reg 0x24
  #define IOC_CURRENT(N)					(N & 0x01FF)
  #define IOC_P_SOURCE_EN					0x0200
  #define IOC_P_SINK_EN						0x0400
  #define IOC_N_SOURCE_EN					0x0800
  #define IOC_N_SINK_EN						0x1000
  #define IOC_DLY_SEL(N)					((N & 0x0003) << 13)
  #define IOC_EN							0x8000


#endif


