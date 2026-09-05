#include "app_ppg_handler.h"

#include "bc_ppg_driver_port.h"
#include "bc_delay.h"
#include "bc_sem.h"
#include "bc_rtos.h"
#include "bc_logger.h"
#include "bc_ppg.h"
#include "bc_queue.h"
#include "bc_ldo_switch.h"


#include "app_ppg_data_handler.h"
#include "app_ppg.h"
#include "bc_watchdog.h"
#include "bc_pmic.h"

//#include "bc_alg_wear_detection.h"

#include <string.h>
#include "bc_alg.h"

#if ( HARDWARE_1141_ENABLED == 1)	
#include "app_ppg_file_data_handler.h"
#endif			
	

#define   PPG_HR_RECV_TIME    4*10		// FIFO_DEPTH_AGC_PROCESS = 4		//调光时间				//250120
#define   PPG_SPO_RECV_TIME   4*10		// FIFO_DEPTH_AGC_PROCESS = 4		//250120

struct app_ppg_collection_time
{
  uint32_t collection_total_time;
  uint32_t collection_total_time_temp_count;
};

struct app_ppg_collection_time  ppg_collection_time = {0};

enum app_ppg_work_state
{
  APP_PPG_WORK_STATE_IDIE = 0,
  APP_PPG_WORK_STATE_START,
  APP_PPG_WORK_STATE_STOP
};

static enum app_ppg_work_state  ppg_work_state = APP_PPG_WORK_STATE_IDIE;

static enum app_ppg_event ppg_event = PPG_IDIE_EVENT;

extern uint16_t app_ppg_hr_data_queue_count;
uint16_t bc_ppg_io_irq_count = 0;


enum app_ppg_task
{
	PPG_TASK_TYPE_READ_SENSOR_PPG_IRQ_DATA = 0,
	PPG_TASK_TYPE_PPG_WORK_STATE,
	PPG_TASK_TYPE_NUM
};

static void app_ppg_irq_handler_thread(void *thread_handler);
static void app_ppg_work_state_handler_thread(void *thread_handler);

static bc_rtos_thread_struct task_thread[PPG_TASK_TYPE_NUM] = {
																	{
																	  .thread_name          = "app sleep handler task",
																	  .thread_stack_depth   = APP_TASK_PPG_IRQ_STACK_SIZE,
																	  .thread_priority      = APP_TASK_PPG_IRQ_PRIO,
																	  .thread_parameters    = NULL,
																	  .thread_task_code     = app_ppg_irq_handler_thread,
																	},
																	{
																	  .thread_name          = "app ppg state task",
																	  .thread_stack_depth   = APP_TASK_PPG_WORK_STATE_STACK_SIZE,
																	  .thread_priority      = APP_TASK_PPG_WORK_STATE_PRIO,
																	  .thread_parameters    = NULL,
																	  .thread_task_code     = app_ppg_work_state_handler_thread,
																	},																	
																};


static void app_ppg_collection_progress_timer_callback(void * pvParameter);

#if ( HARDWARE_413_ENABLED == 1)	
	
	static void app_ppg_poll_timer_callback(void * pvParameter);
	
#endif



static bc_rtos_timer_struct  timer_struct[APP_PPG_TIMER_NUM] = {
	{
		.timer_name = "collection progress timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 1000,
		.lock = false,
		.timer_callback_function = app_ppg_collection_progress_timer_callback,
	},
#if ( HARDWARE_413_ENABLED == 1)	
	{
		.timer_name = "poll timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 320,
		.lock = false,
		.timer_callback_function = app_ppg_poll_timer_callback,
	},
#endif	
	
};



static void app_ppg_work_state_set(enum app_ppg_work_state work_state)
{
  ppg_work_state = work_state;
}

enum app_ppg_work_state app_ppg_work_state_get(void)
{
  return ppg_work_state;
}



static bool ppg_io_irq_flag_temp = false;

static void app_ppg_int_io_irq_callback(uint8_t pin,uint8_t state)
{
	
	BC_LOG_INFO("ppg irq  %d  %d\r\n",pin,state);	
	if(state == 2 )
	{
	   bc_ppg_io_irq_handler();
#if ( HARDWARE_1141_ENABLED == 1 || HARDWARE_191_ENABLED == 1)
	   ppg_io_irq_flag_temp = true;
	   
		
#else
//	   bc_event_set(&event_struct);                        //发送事件
#endif		

//	   ppg_io_irq_flag = true;
		
	   bc_ppg_io_irq_count++;
      bc_rtos_sem_count_give_isr(BC_PPG_IRQ_SEM_COUNT);
	}
}

//void app_ppg_int_io_irq_handler_poll(void)
//{
//	if(ppg_io_irq_flag_temp)
//	{
//		
//		BC_LOG_INFO("zsbc_ppg_io_irq_count:%d\r\n",bc_ppg_io_irq_count);
//	    bc_ppg_io_irq_count--;
//		ppg_io_irq_flag_temp = false;
//		 bc_dog_feed();
//	    bc_ppg_data_handler_poll();
//	}
//}

//extern void power_manage(void);

static void ppg_irq_data_handler(void )
{

  if(ppg_event != PPG_IDIE_EVENT)
  {
	  bc_dog_feed();
	  bc_ppg_data_handler_poll();
//	  ppg_io_irq_flag = true;
//	  power_manage();
#if ( HARDWARE_413_ENABLED == 1)	
    bc_rtos_timer_start(timer_struct[APP_PPG_POLL_TIMER_EVENT].timer_handler,50);
#endif	  
//    BC_LOG_BLE("zsbc_ppg_io_irq_count:%d\r\n",bc_ppg_io_irq_count);
//	BC_LOG_INFO("zsbc_ppg_io_irq_count:%d\r\n",bc_ppg_io_irq_count);
//	bc_ppg_io_irq_count--;
//	BC_LOG_INFO("ppg data handler \r\n");
  }
}


static void app_ppg_irq_handler_thread(void *thread_handler)
{
#if (PPG_DEVIECE_TYPE == 3)  // gh3228T
     bc_ppg_init();
#endif	
	while(true)
	{
		if(bc_rtos_sem_count_take(BC_PPG_IRQ_SEM_COUNT) == true)
		{
			ppg_irq_data_handler();
		}
	}
}

static void app_ppg_work_state_stop(void)
{
	
	switch(app_ppg_event_state_get())
	{
		case PPG_COLLECTION_SPO2_EVNET:
		case PPG_AUTOMATIC_CYCLE_COLLECTION_SPO2_EVENT:
		{
			bc_ppg_spo2_unint();
			
			break;
		}
		case PPG_COLLECTION_HRM_EVNET:
		case PPG_AUTOMATIC_CYCLE_COLLECTION_HRM_EVENT:
		{
			bc_ppg_hr_unint();;
			break;
		}
        case PPG_COLLECTION_IR_EVENT:
		{
			bc_ppg_spo2_unint();
			break;
		}
		case PPG_CHECK_STATUS_EVENT:
		{
			bc_ppg_hr_unint();
			break;
		}
		case PPG_GRAY_CARD_TEST_EVENT:
		{
			bc_ppg_gray_card_uninit();
			break;
		}
		case PPG_IR_RED_GREEN_EVENT:
		{
#if (PPG_DEVIECE_TYPE == 0 || PPG_DEVIECE_TYPE == 4)   //hx 3605


#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2 || PPG_DEVIECE_TYPE == 3)  // zspd4000


	
#endif			
			

//			bc_ppg_init_spo2();			
			break;
		}		
		default:
		{
			break;
		}
	}
	
	bc_ppg_i2c_close();
	bc_ldo_ppg_power_off();
	app_ppg_data_queue_clear();
	app_ppg_event_state_set(PPG_IDIE_EVENT);
	bc_rtos_timer_stop(timer_struct[APP_PPG_COLLECTION_PROGRESS_TIMER_EVENT].timer_handler,20);
	
#if ( HARDWARE_1141_ENABLED == 1)	
	app_ppg_collection_timeout_timer_stop();
	app_ppg_file_slice_storage_timer_stop();
  app_ppg_file_timeout_timer_stop();
  app_ppg_file_close();
#endif		

	bc_pmic_set_sleepmode();
	BC_LOG_INFO("app ppg collection over!! \r\n");
}

static void app_ppg_work_state_start(void)
{
	bc_ppg_io_irq_count = 0;
	app_ppg_hr_data_queue_count = 0;
	bc_pmic_set_startmode();
	bc_ldo_ppg_power_on();
	bc_rtos_delay(100);
	bc_ppg_i2c_open();
	
	BC_LOG_INFO("ppg_model %d \r\n",app_ppg_event_state_get());
//	wear_detection_init();
	
//	bc_rtos_delay(3000);
	switch(app_ppg_event_state_get())
	{
		case PPG_COLLECTION_SPO2_EVNET:
		{
#if (PPG_DEVIECE_TYPE == 0 || PPG_DEVIECE_TYPE == 4)   //hx 3605

          bc_alg_check_wear_init();
#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2 || PPG_DEVIECE_TYPE == 3)  // zspd4000

			bc_alg_check_wear_init();
#if ( HARDWARE_413_ENABLED == 1)	
	timer_struct[APP_PPG_POLL_TIMER_EVENT].xTimerPeriodInTicks = PPG_SPO_RECV_TIME ;
#endif
			
#endif			
			
			bc_alg_check_wear_init();
			bc_ppg_init_spo2();
//			wear_detection_set_mode(PPG_MODE_SPO2);
			break;
		}
		case PPG_AUTOMATIC_CYCLE_COLLECTION_SPO2_EVENT:
		{
#if (PPG_DEVIECE_TYPE == 0 || PPG_DEVIECE_TYPE == 4)   //hx 3605

          bc_alg_check_wear_init();
#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2 || PPG_DEVIECE_TYPE == 3)  // zspd4000

			bc_alg_check_wear_init();
#if ( HARDWARE_413_ENABLED == 1)	
	timer_struct[APP_PPG_POLL_TIMER_EVENT].xTimerPeriodInTicks = PPG_SPO_RECV_TIME ;
#endif
#endif					
			bc_ppg_init_spo2();
//			wear_detection_set_mode(PPG_MODE_SPO2);
			break;
		}
		case PPG_COLLECTION_HRM_EVNET:
		{
#if (PPG_DEVIECE_TYPE == 0|| PPG_DEVIECE_TYPE == 4)   //hx 3605

          bc_alg_check_wear_init();
#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2 || PPG_DEVIECE_TYPE == 3)  // zspd4000
#if ( HARDWARE_413_ENABLED == 1)	
	timer_struct[APP_PPG_POLL_TIMER_EVENT].xTimerPeriodInTicks = PPG_HR_RECV_TIME ;
#endif
			bc_alg_check_wear_init();

#endif			

#if ( HARDWARE_451_ENABLED == 1)	

#else
			bc_alg_rri_init();

#endif	
			
			
			bc_ppg_init_hr();
//			wear_detection_set_mode(PPG_MODE_HR);
			break;
		}
		case PPG_AUTOMATIC_CYCLE_COLLECTION_HRM_EVENT:
		{
#if (PPG_DEVIECE_TYPE == 0|| PPG_DEVIECE_TYPE == 4)   //hx 3605

          bc_alg_check_wear_init();
#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2 || PPG_DEVIECE_TYPE == 3)  // zspd4000
#if ( HARDWARE_413_ENABLED == 1)	
	timer_struct[APP_PPG_POLL_TIMER_EVENT].xTimerPeriodInTicks = PPG_HR_RECV_TIME ;
#endif
			bc_alg_check_wear_init();
#endif			
			
#if ( HARDWARE_451_ENABLED == 1)	

#else
			bc_alg_rri_init();

#endif	
			bc_ppg_init_hr();
//			wear_detection_set_mode(PPG_MODE_HR);
			break;
		}
        case PPG_COLLECTION_IR_EVENT:
		{
#if (PPG_DEVIECE_TYPE == 0|| PPG_DEVIECE_TYPE == 4)   //hx 3605

          bc_alg_check_wear_init();
#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2 || PPG_DEVIECE_TYPE == 3)  // zspd4000
#if ( HARDWARE_413_ENABLED == 1)	
	timer_struct[APP_PPG_POLL_TIMER_EVENT].xTimerPeriodInTicks =  PPG_SPO_RECV_TIME;
#endif
			bc_alg_check_wear_init();
#endif	
			bc_ppg_init_ir();
			break;
		}
		case PPG_CHECK_STATUS_EVENT:
		{
#if ( HARDWARE_451_ENABLED == 1)	

#else
			bc_alg_rri_init();

#endif	
			bc_ppg_init_hr();
#if (PPG_DEVIECE_TYPE == 0|| PPG_DEVIECE_TYPE == 4)   //hx 3605

          bc_alg_check_wear_init();
#elif (PPG_DEVIECE_TYPE == 1)  // afe4403


#elif (PPG_DEVIECE_TYPE == 2 || PPG_DEVIECE_TYPE == 3)  // zspd4000
#if ( HARDWARE_413_ENABLED == 1)	
	timer_struct[APP_PPG_POLL_TIMER_EVENT].xTimerPeriodInTicks = PPG_HR_RECV_TIME;
#endif	
			bc_alg_check_wear_init();
//			wear_detection_set_mode(PPG_MODE_HR);
#endif				
			break;
		}
		case PPG_GRAY_CARD_TEST_EVENT:
		{
//			bc_ppg_gray_card_init();
			

#if ( HARDWARE_156_ENABLED == 1)
    int32_t green_data = 10000*800+536;	int32_t red_data = 10000*830+896; int32_t ir_data= 10000*850+458;
	app_ppg_gary_card_data_callback((void*)&green_data,(void*)&red_data,(void*)&ir_data);
#else	
			bc_ppg_gray_card_init();
#endif			
			
#if ( HARDWARE_413_ENABLED == 1)	
	timer_struct[APP_PPG_POLL_TIMER_EVENT].xTimerPeriodInTicks = 50;
#endif			
			#if (PPG_DEVIECE_TYPE == 0 || PPG_DEVIECE_TYPE == 4)   //hx 3605

				return;

			#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



			#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

			#endif	
			break;
		}
		case PPG_IR_RED_GREEN_EVENT:
		{
#if (PPG_DEVIECE_TYPE == 0 || PPG_DEVIECE_TYPE == 4)   //hx 3605


#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2 || PPG_DEVIECE_TYPE == 3)  // zspd4000


	
#endif			
			

//			bc_ppg_init_spo2();			
			break;
		}			
		default:
		{
			bc_ppg_i2c_close();
			bc_ldo_ppg_power_off();
			return;
		}
	}	
  if(ppg_collection_time.collection_total_time != 0)
	{
   
#if ( HARDWARE_1141_ENABLED == 1)	
#else
	 bc_rtos_timer_start(timer_struct[APP_PPG_COLLECTION_PROGRESS_TIMER_EVENT].timer_handler,50);	
#endif		
		
	}

}

static void app_ppg_work_state_handler_thread(void *thread_handler)
{
  while(true)
  {
    BC_LOG_INFO("app_ppg_work_state_get:%d \r\n",app_ppg_work_state_get());
    switch(app_ppg_work_state_get())
    {
      case APP_PPG_WORK_STATE_IDIE:
      {
        bc_rtos_thread_suspend(task_thread[PPG_TASK_TYPE_PPG_WORK_STATE].thread_handler);
        break;
      }
      case APP_PPG_WORK_STATE_START:
      {
        app_ppg_work_state_start();
        app_ppg_work_state_set(APP_PPG_WORK_STATE_IDIE);
        break;
      }
      case APP_PPG_WORK_STATE_STOP:
      {
        app_ppg_work_state_stop();
        memset((uint8_t*)&ppg_collection_time,0,sizeof(ppg_collection_time));
        app_ppg_work_state_set(APP_PPG_WORK_STATE_IDIE);
        break;
      }
    }  
  }
}



static void app_ppg_collection_progress_timer_callback(void * pvParameter)
{
  
  ppg_collection_time.collection_total_time_temp_count++;
//  BC_LOG_INFO("ppg_collection_time.collection_total_time_temp_count:%d,ppg_collection_time.collection_total_time;%d \r\n",ppg_collection_time.collection_total_time_temp_count,ppg_collection_time.collection_total_time);
  if(ppg_collection_time.collection_total_time_temp_count >= ppg_collection_time.collection_total_time)
  {
    app_ppg_collection_progress_update(false);
  }
  else
  {
    app_ppg_collection_progress_update(true);
  }
}



void app_ppg_event_state_set(enum app_ppg_event ppg_event_id)
{
	
	ppg_event = ppg_event_id;
	printf("ppg event:%d \r\n",ppg_event);
}

/*******************************************************************************
 * Function Name     : app_ppg_event_state_get
 * Description       : 获取ppg当前状态
 * Input             : 
 * Output            : 
 * Return            : app_ppg_event是枚举值，ppg当前状态
 *******************************************************************************/
enum app_ppg_event app_ppg_event_state_get(void)
{	return ppg_event ;
}

void app_ppg_stop(void)
{	
  app_ppg_work_state_set(APP_PPG_WORK_STATE_STOP);
  bc_rtos_thread_resume(task_thread[PPG_TASK_TYPE_PPG_WORK_STATE].thread_handler);
}
	
void app_ppg_start(uint32_t collection_timer,enum app_ppg_event ppg_model)
{
  ppg_collection_time.collection_total_time = collection_timer;
//  BC_LOG_INFO("ppg_collection_time.collection_total_time:%d,collection_timer:%d\r\n",ppg_collection_time.collection_total_time,collection_timer);
	app_ppg_event_state_set(ppg_model);
  app_ppg_work_state_set(APP_PPG_WORK_STATE_START);
  bc_rtos_thread_resume(task_thread[PPG_TASK_TYPE_PPG_WORK_STATE].thread_handler);
}


static void app_ppg_time_create(void)
{
	for(uint8_t i = 0;i < APP_PPG_TIMER_NUM; i++)
	{
		timer_struct[i].timer_handler = bc_rtos_timer_create(timer_struct[i].timer_name,
														  timer_struct[i].xTimerPeriodInTicks,
														  timer_struct[i].uxAutoReload, 
														   (void *)timer_struct[i].timer_id,
															timer_struct[i].timer_callback_function);
		if(timer_struct[i].timer_handler != NULL)
		{
			BC_LOG_INFO("create %s succeed\r\n",timer_struct[i].timer_name);
		}
		else
		{
			BC_LOG_ERROR("create %s fail\r\n",timer_struct[i].timer_name);
		}			
	}
	
}


void app_ppg_init(void)
{

	bc_ppg_io_irq_register_callback(app_ppg_int_io_irq_callback);
	
	bc_ppg_hr_result_callback_regdister(app_ppa_collection_hrm_result);
	bc_ppg_hrv_result_callback_regdister(app_ppa_collection_hrv_result);
	bc_ppg_spo2_result_callback_regdister(app_ppa_collection_spo2_result);
	
	
	
#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	
	bc_ppg_spo2_hr_data_callback_regdister(app_ppg_spo2_hr_data_queue);
#else
     bc_ppg_spo2_data_callback_regdister(app_ppg_spo2_data_queue);

#endif	
	
	bc_ppg_spo2_signal_check_callback_regdister(app_ppa_spo2_signal_check_callback);

	
	

	bc_ppg_hr_signal_check_callback_regdister(app_ppa_hr_signal_check_callback);
	
	bc_ppg_gary_card_callback_regdister(app_ppg_gary_card_data_callback);
	
#if (PPG_DEVIECE_TYPE == 0 || PPG_DEVIECE_TYPE == 4)   //hx 3605

	bc_ppg_hr_data_callback_regdister(app_ppg_hr_32bit_data_queue);

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	bc_ppg_hr_data_callback_regdister(app_ppg_hr_data_queue);
#elif (PPG_DEVIECE_TYPE == 3)  // gh3228T
    bc_ppg_hr_data_callback_regdister(app_ppg_hr_32bit_data_queue);
//    bc_ppg_ecg_data_callback_regdister(app_ppg_ecg_32bit_data_queue);
//    bc_ppg_pwtt_data_callback_regdister(app_ppg_pwtt_32bit_data_queue);	
#endif	
	
	app_ppg_time_create();
	
	bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < PPG_TASK_TYPE_NUM; i++)
	{
		x_return  = bc_rtos_thread_create((TaskFunction_t )task_thread[i].thread_task_code,     	
                                     (const char*    )task_thread[i].thread_name,   	
                                     (uint16_t       )task_thread[i].thread_stack_depth, 
                                     (void*          )&task_thread[i].thread_parameters,				
                                     (UBaseType_t    )task_thread[i].thread_priority,	
                                     (TaskHandle_t*  )&task_thread[i].thread_handler); 
		if(x_return != NULL)
		{
			BC_LOG_INFO("create %s succeed \r\n",task_thread[i].thread_name);
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",task_thread[i].thread_name);
		}	
	}
}
























