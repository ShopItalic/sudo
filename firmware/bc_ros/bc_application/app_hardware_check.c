#include "app_hardware_check.h"


#include "bc_rtos.h"

#include "bc_logger.h"
#include "bc_ppg.h"
#include "bc_temp.h"
#include "bc_pmic.h"
#include "bc_gsensor.h"
//#include "bc_buf.h"
#include "bc_touch_button.h"
//#include "bc_led.h"

#include "app_model_handler.h"
//#include "app_touch_button_handler.h"
#include "app_pmic_handler.h"
#include "app_touch_button_handler.h"
#include "bc_watchdog.h"
#include "app_pdm_handler.h"

#if (HARDWARE_153_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED )

#include "bc_ic_led.h"
						
#endif	

#if (HARDWARE_441_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	

#include "bc_spi_flash.h"
	
#endif

struct __attribute__((__packed__)) hardware_error_info
{
	unsigned int ppg_error : 1;
	unsigned int g_sensor_error : 1;
	unsigned int pmic_error : 1;
	unsigned int vabt_error : 1;
	unsigned int temper_error : 1;
	unsigned int ppg_led_error : 1;
	unsigned int sensor_error : 1;
	unsigned int puf_error : 1;
	unsigned int touch_error : 1;
	unsigned int flash_error : 1;
	unsigned int :6;
};


static struct hardware_error_info error_info ={0};



enum app_hardware_check_task
{
	HARDWARE_CHECK_HANDLER_TASK = 0,
	HARDWARE_CHECK_TASK_TYPE_NUM
};


static void app_hardware_check_handler_thread(void *thread_handler);

static bc_rtos_thread_struct app_rtc_thread[HARDWARE_CHECK_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "app rtc irq handler task",
                                                                      .thread_stack_depth   = APP_TASK_RTC_STACK_SIZE ,
                                                                      .thread_priority      = APP_TASK_RTC_PRIO+5,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = app_hardware_check_handler_thread,
                                                                    },																	
                                                                  };



static bool app_hardware_check(void)
{
	bool temp = false;
	*(unsigned int*)&error_info = 0x00;
	
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_153_ENABLED == 1  || HARDWARE_441_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 || \
   	 HARDWARE_402_ENABLED == 1 || HARDWARE_4132_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_191_ENABLED == 1 || \
     HARDWARE_1181_ENABLED == 1 )		
	if(!bc_ppg_hardware_id_check())
	{
		BC_LOG_ERROR("ppg hardware check error!!  \r\n");
		error_info.ppg_error = 1;
	}
	if(!bc_temp_temperature_check())
	{
		BC_LOG_ERROR("temper hardware check error!!  \r\n");
		error_info.temper_error = 1;		
	}
	
	if(!bc_pmic_check_vbat())
	{
		BC_LOG_ERROR("vbat hardware check error!!  \r\n");
		error_info.vabt_error = 1;		
	}
	
	if(!bc_pmic_id_hardware_check())
	{
		BC_LOG_ERROR("pmic hardware check error!!  \r\n");
		error_info.pmic_error = 1;	
	}
	
#endif	

#if (HARDWARE_181_ENABLED == 1 || HARDWARE_182_ENABLED == 1 )		
	
	if(!bc_pmic_check_vbat())
	{
		BC_LOG_ERROR("vbat hardware check error!!  \r\n");
		error_info.vabt_error = 1;		
	}
	
	if(!bc_pmic_id_hardware_check())
	{
		BC_LOG_ERROR("pmic hardware check error!!  \r\n");
		error_info.pmic_error = 1;	
	}
	
#endif
	
//	if(!bc_gsensor_id_hardware_check())
//	{
//		BC_LOG_ERROR("g_sensor hardware check error!!  \r\n");
//		error_info.g_sensor_error = 1;		
//	}
//	if(!bc_buf_chip_id_hardware_check())
//	{
//		BC_LOG_ERROR("puf hardware check error!!  \r\n");
//		error_info.puf_error = 1;	
//	}
#if (TOUCH_ENABLED)	

	if(!bc_touch_button_chip_id_hardware_check())
	{
		BC_LOG_ERROR("touch hardware check error!!  \r\n");
		error_info.touch_error = 1;	
	}
	
#endif	
	
#if (HARDWARE_441_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	

	if(!spi_flash_device_check_id())
	{
		BC_LOG_ERROR("flash hardware check error!!  \r\n");
		error_info.flash_error = 1;	
	}
//	
#endif		
    if(*(unsigned int*)&error_info != 0)
	{
		return false;
	}		
	return true;
}	


static void app_hardware_check_handler_thread(void * p_context)
{
  while(true)
  {
      
      bc_rtos_delay(2*1000);
      //bc_rtos_delay(10*1000);
      BC_LOG_INFO("app hardware check ok3333333333!!  \r\n");
    	if(app_hardware_check())
      {
        BC_LOG_INFO("app hardware check ok!!  \r\n");
       
    //		bc_led_hardware_check_error_hint_flash_stop();
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED )
        bc_id_led_clear();
        app_model_state_set(APP_MODEL_WORKING_STATE);
#elif (HARDWARE_181_ENABLED == 1 || HARDWARE_182_ENABLED == 1 || HARDWARE_1141_ENABLED == 1  || HARDWARE_1181_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1191_ENABLED || HARDWARE_1231_ENABLED)		
        app_pmic_handler_timer_start();            //启动定时查询电量与pmic状态	
#if(HARDWARE_1231_ENABLED)    
        //app_touch_init_event();
#if defined(HANDWARE_1_23_2_ONE_SEC)
        /* 上电自动开启离线录音 */
        BC_LOG_INFO("power on auto start recording\r\n");
        app_timer_record_start();
#endif
#endif        
        
#else
//        app_model_state_set(APP_MODEL_WORKING_STATE);
#endif			

         bc_rtos_thread_suspend(app_rtc_thread[HARDWARE_CHECK_HANDLER_TASK].thread_handler);
         bc_rtos_thread_delete(app_rtc_thread[HARDWARE_CHECK_HANDLER_TASK].thread_handler);
        
      }
      else
      {
        //error hardware handler
    #if (HARDWARE_153_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED)
        bc_id_led_clear();
                
    #endif	
        BC_LOG_ERROR("app hardware check ERROR!!  error_info:%d  \r\n",*(uint8_t*)&error_info);
        
    //		bc_led_hardware_check_error_hint_flash_start();
          
        
        
      }
  }
  

}




uint16_t app_hardware_check_all(void)
{
	*(uint16_t *)&error_info = 0;

#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_153_ENABLED == 1  || HARDWARE_441_ENABLED == 1 || HARDWARE_402_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)		
	if(!bc_ppg_hardware_id_check())
	{
		BC_LOG_ERROR("ppg hardware check error!!  \r\n");
		error_info.ppg_error = 1;
	}
	if(!bc_temp_temperature_check())
	{
		BC_LOG_ERROR("temper hardware check error!!  \r\n");
		error_info.temper_error = 1;		
	}
	
	if(!bc_pmic_check_vbat())
	{
		BC_LOG_ERROR("vbat hardware check error!!  \r\n");
		error_info.vabt_error = 1;		
	}
	
	if(!bc_pmic_id_hardware_check())
	{
		BC_LOG_ERROR("pmic hardware check error!!  \r\n");
		error_info.pmic_error = 1;	
	}
	if(!bc_gsensor_id_hardware_check())
	{
		BC_LOG_ERROR("g_sensor hardware check error!!  \r\n");
		error_info.g_sensor_error = 1;		
	}
	
#endif		


#if (HARDWARE_181_ENABLED == 1 || HARDWARE_182_ENABLED == 1 )		
	
	if(!bc_pmic_check_vbat())
	{
		BC_LOG_ERROR("vbat hardware check error!!  \r\n");
		error_info.vabt_error = 1;		
	}
	
	if(!bc_pmic_id_hardware_check())
	{
		BC_LOG_ERROR("pmic hardware check error!!  \r\n");
		error_info.pmic_error = 1;	
	}
	
#endif

	
	
//	if(!bc_buf_chip_id_hardware_check())
//	{
//		BC_LOG_ERROR("puf hardware check error!!  \r\n");
//		error_info.puf_error = 1;	
//	}
	
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_153_ENABLED == 1 || HARDWARE_413_ENABLED == 1 )	

	if(!bc_touch_button_chip_id_hardware_check())
	{
		BC_LOG_ERROR("touch hardware check error!!  \r\n");
		error_info.touch_error = 1;	
	}
	else
	{
//		app_touch_init_event();
	}
	
#endif	
	
	if(!bc_gsensor_hardware_check())
	{
		error_info.sensor_error = 1;
	}

	
	return *(uint16_t *)&error_info;
}

void app_hardware_check_model(void)
{
//	app_model_state_set(APP_MODEL_HARDWARE_CHECK_STATE);
}




/*******************************************************************************
 * Function Name     : app_test_timer_create
 * Description       : test定时器
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
void app_hardware_check_task_create(void)
{
	
	bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < HARDWARE_CHECK_TASK_TYPE_NUM; i++)
	{
		x_return  = bc_rtos_thread_create((TaskFunction_t )app_rtc_thread[i].thread_task_code,     	
                                     (const char*    )app_rtc_thread[i].thread_name,   	
                                     (uint16_t       )app_rtc_thread[i].thread_stack_depth, 
                                     (void*          )&app_rtc_thread[i].thread_parameters,				
                                     (UBaseType_t    )app_rtc_thread[i].thread_priority,	
                                     (TaskHandle_t*  )&app_rtc_thread[i].thread_handler); 
		if(x_return != NULL)
		{
			BC_LOG_INFO("create %s succeed \r\n",app_rtc_thread[i].thread_name);
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",app_rtc_thread[i].thread_name);
		}	
	}
//	fml_temper_adc_hardware_error_register_callback(app_hardware_check_model);
//	fml_pmic_event_handler(PMIC_POWER_ADC_HARDWARE_ERROR_REGISTER_CALLBACK_EVENT,&app_hardware_check_model);
//	fml_pmic_event_handler(PMIC_HARDWARE_ERROR_REGISTER_CALLBACK_EVENT,&app_hardware_check_model);
	
}
























