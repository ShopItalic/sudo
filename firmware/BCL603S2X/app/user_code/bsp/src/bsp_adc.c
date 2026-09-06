#include "q_device.h"

#include <string.h>

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
	  .name = "temper_adc",
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
	  .name = "ntc_adc",
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
	
};

/*******************************************************************************
 * Function Name     : bsp_adc_init
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
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
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
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
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
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
					Q_DEVICE_LOG_INFO("open %s adc init",bsp_list[i].name);
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
					Q_DEVICE_LOG_INFO("open %s adc init",bsp_list[i].name);
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
					Q_DEVICE_LOG_INFO("open %s adc init",bsp_list[i].name);
					bsp_adc_init();					
				}				
			}
			
			int_adc:
			
			
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
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
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
					Q_DEVICE_LOG_INFO("close %s adc init",bsp_list[i].name);
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
					Q_DEVICE_LOG_INFO("close %s adc init",bsp_list[i].name);
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
					Q_DEVICE_LOG_INFO("close %s adc init",bsp_list[i].name);
					bsp_adc_uninit();
				}
				else
				{
				  goto unint_adc;
				}				
			}
			
			unint_adc:
			nrf_drv_saadc_channel_uninit(bsp_list[i].adc_ch);
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
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
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
			nrfx_saadc_sample_convert(i, &nrf_saadc_value);
			
			*((uint16_t*)buffer) = nrf_saadc_value;
			return RESULT_OK;
		}
	}
  return RESULT_I2C_DEV_NULL_ERR;
}



/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
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
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
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

