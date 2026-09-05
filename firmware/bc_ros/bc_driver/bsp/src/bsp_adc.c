
/*******************************************************************************
此为bsp adc文件，通过宏定义来兼容nordic、phy6222硬件平台
接口遵循q_device规则
日  期：2024年1月17日
编写人：邱成凯
 *******************************************************************************/

#include "q_device.h"

#include <string.h>


#if (HARDWARE_ARCH_TYPE_NORDIC == 1)
#include "nrf_drv_saadc.h"

struct  BSP_ADC
{
	const char   *name;
	bool          lock;
  nrf_saadc_input_t adc_ch;
	nrf_saadc_channel_config_t adc_channel_config;
	q_device_t dev;
};

static struct BSP_ADC bsp_list[] = 
{

#if defined(HANDWARE_1_5_3)

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN5,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
	
	{
	    .name = "temper_adc_1",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN4,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
	{
	    .name = "temper_adc_2",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN0,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},

	{
	    .name = "ntc_bat",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN3,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
	
#elif defined(HANDWARE_1_8_1)

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN5,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
#elif defined(HANDWARE_1_9_1)

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN5,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
		
	
#elif (defined(HANDWARE_4_1_1) || defined(HANDWARE_4_1_2) || defined(RONG_WEI_Z2X))
	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN1,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
	
	{
	    .name = "temper_adc_1",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN0,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
	{
	    .name = "temper_adc_2",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN0,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},

	{
	    .name = "ntc_bat",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN3,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
#elif defined(HANDWARE_4_0_2) 

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN2,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
	
	{
	    .name = "temper_adc_1",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN1,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
	
#elif defined(HANDWARE_4_4_1) 

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN1,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
	
	{
	    .name = "temper_adc_1",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN0,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},	
#elif defined(HANDWARE_4_1_3) 

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN1,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
	
	{
	    .name = "temper_adc_1",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN2,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},	
#elif defined(HANDWARE_4_5_1) 

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN2,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
	
	{
	    .name = "temper_adc_1",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN1,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},	
#elif (defined(HANDWARE_BCL601_151))

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN1,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
	
#elif defined(HANDWARE_1_12_1)

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN5,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
	
	{
	    .name = "temper_adc_1",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN4,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
#elif defined(HANDWARE_1_5_8)

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN5,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
	
	{
	    .name = "ts2323a_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN6,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
		
#elif defined(HANDWARE_1_5_6)

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN5,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
	
	{
	    .name = "ts2323a_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN6,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},	
#elif defined(HANDWARE_1_14_1)

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN5,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},	
#elif defined(HANDWARE_1_17_1)

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN7,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},	 
#elif defined(HANDWARE_1_18_1)

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN5,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},
#elif defined(HANDWARE_1_19_1)

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN5,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},  
#elif defined(HANDWARE_1_23_2)

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN6,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},  
#elif defined(HANDWARE_1_23_1)

	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = NRF_SAADC_INPUT_AIN5,
		.adc_channel_config = {
														.resistor_p = NRF_SAADC_RESISTOR_DISABLED,      
														.resistor_n = NRF_SAADC_RESISTOR_DISABLED,      
														.gain       = NRF_SAADC_GAIN1_6,                
														.reference  = NRF_SAADC_REFERENCE_INTERNAL,     
														.acq_time   = NRF_SAADC_ACQTIME_10US,           
														.mode       = NRF_SAADC_MODE_SINGLE_ENDED,      
														.burst      = NRF_SAADC_BURST_DISABLED,         
														.pin_n      = NRF_SAADC_INPUT_DISABLED,        
		                       },
		.dev = {0},
	},  
  
#endif		
	
	
	
};

/*******************************************************************************
 * Function Name     : bsp_adc_init
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_adc_init(void)
{
	ret_code_t err_code;
	nrf_drv_saadc_config_t saadc_config = NRF_DRV_SAADC_DEFAULT_CONFIG;
	saadc_config.resolution = NRF_SAADC_RESOLUTION_12BIT;
	err_code = nrf_drv_saadc_init(&saadc_config, NULL);
	APP_ERROR_CHECK(err_code);
	q_device_delay_ms(5);
}

/*******************************************************************************
 * Function Name     : bsp_adc_unint
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_adc_uninit(void)
{
	nrfx_saadc_uninit();
}

/*******************************************************************************
 * Function Name     : bsp_adc_open
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_adc_open(q_device_t*dev)
{
	ret_code_t err_code;
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].lock)
			{
				return RESULT_OK;
			}
//			for (uint8_t j = 0; j < array_size(bsp_list); j++) 
//			{
//				if(bsp_list[j].lock && i!=j)
//				{
//					j=array_size(bsp_list);
//				}
//				else if(j == array_size(bsp_list)-1 && !bsp_list[j].lock)
//				{
//					NRF_LOG_INFO("open %s adc init",bsp_list[i].name);
//					bsp_adc_init();
//				}
//			}
			if( i !=0 && i != array_size(bsp_list) - 1)
			{  
				for(uint8_t j = 0; j < i; j++ )
				{
					if(bsp_list[j].lock)
					{
						goto int_adc;
					}
				}
				for(uint8_t j = i +1; j < array_size(bsp_list); j++ )
				{
					if(bsp_list[j].lock)
					{
						goto int_adc;
					}					
				}
				if(bsp_list[i].lock)
				{
					goto int_adc;
				}
				else
				{
//					Q_DEVICE_LOG_INFO("open %s adc init  \r\n",bsp_list[i].name);
					bsp_adc_init();					
				}
			}
			else if(i == 0 )
			{
				for(uint8_t j = i +1; j < array_size(bsp_list); j++ )
				{
					if(bsp_list[j].lock)
					{
						goto int_adc;
					}					
				}	
				if(bsp_list[i].lock)
				{
					goto int_adc;
				}
				else
				{
//					Q_DEVICE_LOG_INFO("open %s adc init  \r\n",bsp_list[i].name);
					bsp_adc_init();					
				}				
			}
			else if( i == array_size(bsp_list) - 1)
			{
				for(uint8_t j =0; j < array_size(bsp_list) - 1; j++ )
				{
					if(bsp_list[j].lock)
					{
						goto int_adc;
					}					
				}	
				if(bsp_list[i].lock)
				{
					goto int_adc;
				}
				else
				{
					
					bsp_adc_init();					
				}				
			}
			
			int_adc:
			
			//Q_DEVICE_LOG_INFO("open %s adc init  \r\n",bsp_list[i].name);
			bsp_list[i].adc_channel_config.pin_p = bsp_list[i].adc_ch;
            err_code = nrf_drv_saadc_channel_init(i, &bsp_list[i].adc_channel_config);
			bsp_list[i].lock = true;
			APP_ERROR_CHECK(err_code);
			q_device_delay_ms(2);
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
}


/*******************************************************************************
 * Function Name     : bsp_adc_close
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_adc_close(q_device_t *dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_OK;
			}
//			for (uint8_t j = 0; j < array_size(bsp_list); j++) 
//			{
//				if(bsp_list[j].lock && i!=j)
//				{
//					j = array_size(bsp_list);
//				}
//				else if((j = array_size(bsp_list) -1))
//				{
//					for (uint8_t b = j+1; b < array_size(bsp_list); b++) 
//					{
//						if(bsp_list[b].lock)
//						{
//							j = array_size(bsp_list);
//						}
//					}
//					if(j < array_size(bsp_list))
//					{
//						NRF_LOG_INFO("close %s adc init",bsp_list[i].name);
//						bsp_adc_uninit();						
//					}
//				}
//			}
			if( i !=0 && i != array_size(bsp_list) - 1)
			{  
				for(uint8_t j = 0; j < i; j++ )
				{
					if(bsp_list[j].lock)
					{
						goto unint_adc;
					}
				}
				for(uint8_t j = i +1; j < array_size(bsp_list); j++ )
				{
					if(bsp_list[j].lock)
					{
						goto unint_adc;
					}					
				}
				if(bsp_list[i].lock)
				{
//					Q_DEVICE_LOG_INFO("close %s adc init  \r\n",bsp_list[i].name);
					bsp_adc_uninit();
				}
				else
				{
				  goto unint_adc;
				}
			}
			else if(i == 0 )
			{
				for(uint8_t j = i +1; j < array_size(bsp_list); j++ )
				{
					if(bsp_list[j].lock)
					{
						goto unint_adc;
					}					
				}	
				if(bsp_list[i].lock)
				{
//					Q_DEVICE_LOG_INFO("close %s adc init  \r\n",bsp_list[i].name);
					bsp_adc_uninit();
				}
				else
				{
				  goto unint_adc;
				}				
			}
			else if( i == array_size(bsp_list) - 1)
			{
				for(uint8_t j =0; j < array_size(bsp_list) - 1; j++ )
				{
					if(bsp_list[j].lock)
					{
						goto unint_adc;
					}					
				}	
				if(bsp_list[i].lock)
				{
					
					bsp_adc_uninit();
				}
				else
				{
				  goto unint_adc;
				}				
			}
			
			unint_adc:
			//Q_DEVICE_LOG_INFO("close %s adc init  \r\n",bsp_list[i].name);
			nrf_drv_saadc_channel_uninit(bsp_list[i].adc_ch);
//			nrf_gpio_cfg_input(dev->bsp_io_pin, NRF_GPIO_PIN_NOPULL);
			bsp_list[i].lock = false;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
}


/*******************************************************************************
 * Function Name     : bsp_adc_read
 * Description       : 
 * Input             : 
 * Output            : uint16_t类型数值
 * Return            : 
 *******************************************************************************/
static int bsp_adc_read(q_device_t *dev, int pos,const void *buffer, int size)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_I2C_DEV_NULL_ERR;
			}

			nrf_saadc_value_t nrf_saadc_value;
#if defined(SUDO_VOICE_ONLY)
			int result = nrfx_saadc_sample_convert(i, &nrf_saadc_value);
			if(result != RESULT_OK)
				return result;
#else
			nrfx_saadc_sample_convert(i, &nrf_saadc_value);
#endif
			
			*((uint16_t*)buffer) = nrf_saadc_value;
			return RESULT_OK;
		}
	}
  return RESULT_I2C_DEV_NULL_ERR;
}





#endif



#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)


#include "adc.h"
#include "osal.h"
#include <math.h>

#define MAX_SAMPLE_POINT    64
uint16_t adc_debug[MAX_SAMPLE_POINT];

struct  BSP_ADC
{
	const char   *name;
	bool          lock;
	bool          adc_mode;
	float         adc_value;     //true 查分模式，false  单端模式
    adc_CH_t     adc_ch;
	adc_Cfg_t adc_channel_config;
	q_device_t dev;
};

static struct BSP_ADC bsp_list[] = 
{
	{
	    .name = "vbat_adc",
		.lock = false,
        .adc_ch = ADC_CH1N_P11,
		.adc_mode = false,
		.adc_channel_config = {   
			                    .channel = ADC_BIT(ADC_CH1N_P11),  //adc通道
			                    .is_continue_mode = FALSE,  //连续测量关闭
			                    .is_differential_mode = 0x00, //单端模式
			                    .is_high_resolution = 0x7f,  //bypass模式
		                       },
		.dev = {0},
	},
	{
	    .name = "temper_adc_0",
		.lock = false,
        .adc_ch = ADC_CH1P_P23,
		.adc_mode = false,
		.adc_channel_config = {   
			                    .channel = ADC_BIT(ADC_CH1P_P23),  //adc通道
			                    .is_continue_mode = FALSE,  //连续测量关闭
			                    .is_differential_mode = 0x00, //单端模式
			                    .is_high_resolution = 0x7f,  //bypass模式
		                       },
		.dev = {0},
	},
	{
	    .name = "temp_adc_1",
		.lock = false,
        .adc_ch = ADC_CH2P_P14,
		.adc_mode = false,
		.adc_channel_config = {   
			                    .channel = ADC_BIT(ADC_CH2P_P14),  //adc通道
			                    .is_continue_mode = FALSE,  //连续测量关闭
			                    .is_differential_mode = 0x00, //单端模式
			                    .is_high_resolution = 0x7f,  //bypass模式
		                       },
		.dev = {0},
	},
};


/*******************************************************************************
 * Function Name     : dc_evt 
 * Description       : adc 测量回调
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void adc_evt(adc_Evt_t* pev)
{
    bool is_high_resolution = FALSE;
    bool is_differential_mode = FALSE;

    float adc_value = 0;
	
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if(pev->ch == bsp_list[i].adc_ch)
		{
			if(bsp_list[i].adc_mode)
			{
				 osal_memcpy(adc_debug,pev->data,2*(pev->size));    
				is_high_resolution   = TRUE;
				is_differential_mode = TRUE;
				adc_value = hal_adc_value_cal((adc_CH_t)1,adc_debug, pev->size, is_high_resolution,is_differential_mode);
				bsp_list[i].adc_value = fabsf(adc_value);
				Q_DEVICE_LOG_INFO("P%d %d %d mv ",1,(int)(adc_value*1000),(int)(bsp_list[i].adc_value*1000));
			}
			else
			{
				is_high_resolution = (bsp_list[i].adc_channel_config.is_high_resolution & BIT(pev->ch))?TRUE:FALSE;
				is_differential_mode = (bsp_list[i].adc_channel_config.is_differential_mode & BIT(pev->ch))?TRUE:FALSE;
				bsp_list[i].adc_value = hal_adc_value_cal(pev->ch,pev->data, pev->size, is_high_resolution,is_differential_mode);
			}
		}
	}
}

/*******************************************************************************
 * Function Name     : bsp_adc_open
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_adc_open(q_device_t*dev)
{
	uint8_t batt_ch = ADC_CH1N_P11;
	GPIO_Pin_e pin;
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].adc_mode)
			{
				hal_adc_config_channel(bsp_list[i].adc_channel_config, adc_evt);
			}
			else
			{
			  pin = s_pinmap[batt_ch];
			  hal_gpio_cfg_analog_io(pin,Bit_DISABLE);
			  hal_adc_config_channel(bsp_list[i].adc_channel_config, adc_evt);
			  hal_gpio_cfg_analog_io(pin,Bit_DISABLE);  			  
			}
           bsp_list[i].lock = true;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
}

/*******************************************************************************
 * Function Name     : bsp_adc_read
 * Description       : 
 * Input             : 
 * Output            : uint16_t类型数值
 * Return            : 
 *******************************************************************************/
static int bsp_adc_read(q_device_t *dev, int pos,const void *buffer, int size)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_I2C_DEV_NULL_ERR;
			}

             hal_adc_start(INTERRUPT_MODE);
			 q_device_delay_ms(20);
			Q_DEVICE_LOG_INFO("llllllllllll  %f ",bsp_list[i].adc_value);
			*((float*)buffer) = bsp_list[i].adc_value;
			return RESULT_OK;
		}
	}
  return RESULT_I2C_DEV_NULL_ERR;
}


/*******************************************************************************
 * Function Name     : bsp_adc_open
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_adc_close(q_device_t*dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_I2C_DEV_NULL_ERR;
			}
			bsp_list[i].lock = false;
			return RESULT_OK;
		}
	}	
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
}

#endif


/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static struct q_device_ops ops =
{
	.read = bsp_adc_read,
	.open = bsp_adc_open,
	.close = bsp_adc_close,

};

/*******************************************************************************
 * Function Name     : bsp_adc_register
 * Description       : 设备注册
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_adc_register(void)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		bsp_list[i].dev.name = bsp_list[i].name;
		bsp_list[i].dev.dops  = &ops;
		q_device_register(&bsp_list[i].dev);		
	}
}

device_initcall(bsp_adc_register);
