#include "test.h"

#include "nrf_gpio.h"
#include "bc_rtos.h"
#include "bc_delay.h"

#include "q_device.h"

#include "app_ppg_data_handler.h"

#include "app_package.h"

#if ( HARDWARE_1191_ENABLED == 1)	

#include "bc_wifi_port.h"
#include "app_pdm_handler.h"

#endif	
#if ( HARDWARE_1191_ENABLED == 1)	

#include "bc_led_pwm.h"

#endif	


#if ( HARDWARE_1231_ENABLED == 1)	

#include "bc_touch_button.h"
#include "app_touch_button_handler.h"
#include "bc_linear_motor.h"
#include "tx1812n5.h"
#include "bc_ldo_switch.h"
#include "bc_ic_led.h"

#include "bc_led.h"
#include "bc_led_pwm.h"
#endif	

#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
#include "bc_linear_motor_ic.h"
#include "bc_linear_motor_ic_port.h"
#include "bc_fuel_gauge.h"
#include "bc_fuel_gauge_port.h"
#endif

static  uint32_t temp_count = 0;


static void test_timer_callback (void * pvParameter);

static bc_rtos_timer_struct  timer_struct = {
																						 .timer_name = "test timer",
																						 .uxAutoReload = pdTRUE,
																						 .xTimerPeriodInTicks = 800,
																						 .timer_id = TEST_TIMER_ID,
																						 .timer_callback_function = test_timer_callback,                                                  
											                  	  };

                                            
enum app_test_task
{
	TEST_TASK_TYPE_INIT = 0,
	TEST_TASK_TYPE_NUM
};

static void app_test_handler_thread(void *thread_handler);

static bc_rtos_thread_struct app_user_thread[TEST_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "app user handler task",
                                                                      .thread_stack_depth   = 512 ,
                                                                      .thread_priority      = 5,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = app_test_handler_thread,
                                                                    },																	
                                                                  };

static uint8_t ifalg = 0;                                                                  
static void app_test_handler_thread(void *thread_handler)
{
    #if defined(HANDWARE_1_23_3)
//    bc_linear_motor_device_i2c_find();
//    bc_cw221x_device_find();
    //bc_linear_motor_i2c_open();
    #endif
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(0,10));
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(0,9));
  while(true)
  {

    bc_delay_ms(1000*1);
      nrf_gpio_pin_toggle(NRF_GPIO_PIN_MAP(0,9));
      nrf_gpio_pin_toggle(NRF_GPIO_PIN_MAP(0,10));
      if(!ifalg) {
          //ifalg = 1;
          BC_LOG_INFO("ifalg\r\n");
#if defined(HANDWARE_1_23_3)
          //bc_linear_motor_device_i2c_find();
//            bc_linear_motor_ic_device_init();
#endif
      }
    BC_LOG_INFO("test  %d \r\n",temp_count++);

   
  }
}



/*******************************************************************************
 * Function Name     : app_test_timer_callback
 * Description       : test定时器回调
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
static void test_timer_callback (void * pvParameter)
{

	//BC_LOG_INFO("test  %d \r\n",temp_count++);
//  app_ppg_test();
//  app_package_authentication_test_recv();
//  bc_wifi_spi_write_test(0x31);
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
void test_timer_create(void)
{
	

  	timer_struct.timer_handler = bc_rtos_timer_create( timer_struct.timer_name,
													   timer_struct.xTimerPeriodInTicks,
													 timer_struct.uxAutoReload, 
													 (void *)timer_struct.timer_id,
													timer_struct.timer_callback_function);
	if(timer_struct.timer_handler != NULL)
	{
		bc_rtos_timer_start(timer_struct.timer_handler,100);
		BC_LOG_INFO("create %s succeed\r\n",timer_struct.timer_name);
	}
    else
	{
		BC_LOG_ERROR("create %s fail\r\n",timer_struct.timer_name);
	}	
  
  
  	bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < TEST_TASK_TYPE_NUM; i++)
	{
		x_return  = bc_rtos_thread_create((TaskFunction_t )app_user_thread[i].thread_task_code,     	
                                     (const char*    )app_user_thread[i].thread_name,   	
                                     (uint16_t       )app_user_thread[i].thread_stack_depth, 
                                     (void*          )&app_user_thread[i].thread_parameters,				
                                     (UBaseType_t    )app_user_thread[i].thread_priority,	
                                     (TaskHandle_t*  )&app_user_thread[i].thread_handler); 
		if(x_return == bc_pdPASS)
		{
			BC_LOG_INFO("create %s succeed \r\n",app_user_thread[i].thread_name);
//      bc_rtos_thread_start_scheduler();
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",app_user_thread[i].thread_name);
		}	
	}
}


