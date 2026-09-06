#include "app_ble_speed_handler.h"

#include "app_package.h"
#include "bc_logger.h"
#include "bc_rtos.h"
#include "bc_delay.h"
#include "app_ble_handler.h"
#include "bc_watchdog.h"


#if ( HARDWARE_1191_ENABLED == 1)	

#include "bc_wifi_port.h"

#endif	



static uint32_t seq = 0,length = 0;

enum app_speed_mode
{
  SPEED_MODE_IDIE = 0,
  SPEED_MODE_BLE,
  SPEED_MODE_TCP
};


enum app_speed_mode speed_mode = SPEED_MODE_IDIE;

enum app_ble_speed_task
{
	BLE_SPEED_TASK_SEND = 0,
	BLE_SPEED_TASK_TYPE_NUM,
};


static void app_ble_speed_send_handler_thread(void *thread_handler);

static bc_rtos_thread_struct thread_struct[BLE_SPEED_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "app ble speed task",
                                                                      .thread_stack_depth   = APP_TASK_BLE_SPEED_STACK_SIZE,
                                                                      .thread_priority      = APP_TASK_BLE_SPEED_PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = app_ble_speed_send_handler_thread,
                                                                    },																	
                                                                  };


void app_speed_mode_set(enum app_speed_mode mode)
{
  speed_mode = mode;
}

enum app_speed_mode app_speed_mode_get(void)
{
  return speed_mode;
}

static void app_ble_speed_send_handler_thread(void *thread_handler)
{
  while(true)
  {
    switch(speed_mode)
    {
      case SPEED_MODE_IDIE:
      {
        bc_rtos_thread_suspend(thread_struct[BLE_SPEED_TASK_SEND].thread_handler);
        break;
      }
      case SPEED_MODE_BLE:
      {
        app_package_speed_test_up(seq,length);
        seq++;
        break;
      }
      case SPEED_MODE_TCP:
      {
#if ( HARDWARE_1191_ENABLED == 1)	

        seq++;
        bc_wifi_spi_write_test(seq,length);
#endif	        
        
        break;
      }
    }
    bc_dog_feed();
  }
}

void app_ble_speed_test_start(uint8_t leng)
{
  app_speed_mode_set(SPEED_MODE_BLE);
	bc_rtos_thread_resume(thread_struct[BLE_SPEED_TASK_SEND].thread_handler);
	length = leng ;
}

void app_ble_speed_test_stop(void)
{
	app_speed_mode_set(SPEED_MODE_IDIE);
	seq = 0,length = 0;
}

void app_wifi_speed_test_start(uint8_t leng)
{
  
  app_speed_mode_set(SPEED_MODE_TCP);
	bc_rtos_thread_resume(thread_struct[BLE_SPEED_TASK_SEND].thread_handler);
	length = leng ;
}

void app_wifi_speed_test_stop(void)
{
	app_speed_mode_set(SPEED_MODE_IDIE);
	seq = 0,length = 0;
}


/*******************************************************************************
 * Function Name     : app_ble_time_create
 * Description       : ble相关定时器创建
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
void app_ble_speed_time_create(void)
{
	bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < BLE_SPEED_TASK_TYPE_NUM; i++)
	{
		x_return  = bc_rtos_thread_create((TaskFunction_t )thread_struct[i].thread_task_code,     	
                                     (const char*    )thread_struct[i].thread_name,   	
                                     (uint16_t       )thread_struct[i].thread_stack_depth, 
                                     (void*          )&thread_struct[i].thread_parameters,				
                                     (UBaseType_t    )thread_struct[i].thread_priority,	
                                     (TaskHandle_t*  )&thread_struct[i].thread_handler); 
		if(x_return != NULL)
		{
			BC_LOG_INFO("create %s succeed \r\n",thread_struct[i].thread_name);
      bc_rtos_thread_suspend(thread_struct[i].thread_handler);
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",thread_struct[i].thread_name);
		}	
	}
	
}


















