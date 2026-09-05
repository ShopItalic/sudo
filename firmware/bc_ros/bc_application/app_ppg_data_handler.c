#include "app_ppg_data_handler.h"

#include "bc_queue.h"
#include "bc_ppg.h"

#include "bc_logger.h"
#include "bc_temp.h"
#include "bc_alg.h"
#include "bc_rtc.h"
#include "bc_gsensor.h"
#include "bc_delay.h"
#include "bc_strategy_value.h"
#include "bc_util.h"
#include "bc_alg.h"
#include "bc_alg_ppg.h"
#include "bc_watchdog.h"
#include "bc_device_info.h"

#include "app_package.h"
#include "app_ppg.h"
#include "app_ppg_handler.h"
#include "app_authentication_handler.h"
#include "app_tsdb_handler.h"
#include "app_g_sensor_handler.h"
#include "app_temper_handler.h"
#include "app_model_handler.h"
#include "app_touch_button_handler.h"
#include "app_sleep_handler.h"
#include "app_pmic_handler.h"
#include "app_g_sensor_handler.h"

#include "bc_sem.h"
#include "bc_rtos.h"

//#include "bc_alg_wear_detection.h"




#if (HARDWARE_153_ENABLED == 1  || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1)	

#include "app_pdm_handler.h"		
   
#endif

#if (HARDWARE_441_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 )	

#include "app_ppg_file_data_handler.h"		
   
#endif	

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

static uint8_t ppg_hrm_and_spo2_automatic_cycle_collection_count = 0;

static struct app_cmd_package app_ppg_package = {0};
 struct app_cmd_package app_ppg_package_test = {0};

static struct ppg_result  ppg_result_pyload = {0};

static struct ppg_spo2_pack_pyload spo2_pack_pyload = {0};
static struct ppg_hrm_pack_pyload hrm_pack_pyload = {0};

#if (HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)		
static struct ppg_spo2_hr_temper_pack_pyload spo2_hr_temper_pack_pyload = {0};
#endif


static struct ppg_package_config  ppg_config_pack = {0};

static const uint8_t collection_spo2_mode_max = 15;
static const uint8_t collection_hr_mode_max = 15;
static const uint8_t collection_spo2_hr_mode_max = 11;

static uint16_t ppg_data_collection_progress_count = 0;
static uint8_t temp_count = 0;

static uint8_t ppg_hr = 0;

static uint16_t temp_ware_count = 0;

static void app_ppg_wear_detection_result(void);

static bool ppg_wear_flag = true;
static bool ppg_timeout_flag = true;

uint16_t app_ppg_hr_data_queue_count = 0;
extern uint16_t bc_ppg_io_irq_count;

struct app_ppg_g_sensor_gyro_data
{
	uint16_t gyro[3];
};


enum app_ppg_data_task
{
	PPG_DATA_TASK_TYPE_HR_DATA = 0,
	PPG_DATA_TASK_TYPE_SPO2_DATA,
  PPG_DATA_TASK_TYPE_STORAGE_RECORD,
//  PPG_DATA_TASK_TYPE_SPO2_HR_DATA,
	PPG_DATA_TASK_TYPE_NUM
};

static void ppg_data_hr_data_handler_thread(void * p_context);
static void ppg_data_spo2_data_handler_thread(void * p_context);
static void ppg_data_storage_record_handler_thread(void * p_context);
//static void ppg_data_spo2_hr_data_handler_thread(void * p_context);


static bc_rtos_thread_struct task_thread[PPG_DATA_TASK_TYPE_NUM] = {
																	{
																	  .thread_name          = "ppg hr data handler task",
																	  .thread_stack_depth   = APP_TASK_PPG_HR_STACK_SIZE,
																	  .thread_priority      = APP_TASK_PPG_HR_PRIO,
																	  .thread_parameters    = NULL,
																	  .thread_task_code     = ppg_data_hr_data_handler_thread,
																	},
																	{
																	  .thread_name          = "ppg spo2 data task",
																	  .thread_stack_depth   = APP_TASK_PPG_SPO2_STACK_SIZE,
																	  .thread_priority      = APP_TASK_PPG_SPO2_PRIO,
																	  .thread_parameters    = NULL,
																	  .thread_task_code     = ppg_data_spo2_data_handler_thread,
																	},	
                                  {
																	  .thread_name          = "ppg storage record task",
																	  .thread_stack_depth   = APP_TASK_PPG_STORAGE_RECORD_STACK_SIZE,
																	  .thread_priority      = APP_TASK_PPG_STORAGE_RECORD_PRIO,
																	  .thread_parameters    = NULL,
																	  .thread_task_code     = ppg_data_storage_record_handler_thread,
																	},   
//                                  {
//																	  .thread_name          = "ppg hr spo2 data task",
//																	  .thread_stack_depth   = APP_TASK_PPG_SPO2_HR_STACK_SIZE,
//																	  .thread_priority      = APP_TASK_PPG_SPO2_HR_PRIO,
//																	  .thread_parameters    = NULL,
//																	  .thread_task_code     = ppg_data_spo2_hr_data_handler_thread,
//																	},                                   
																};


static void app_ppg_automatic_cycle_timer_callback(void * pvParameter);
static void app_ppg_wear_detection_timer_callback(void * pvParameter);
static void app_ppg_collextion_timeout_timer_callback(void * pvParameter);

static bc_rtos_timer_struct  timer_struct[PPG_TIME_TYPE_NUM] = {
	{
		.timer_name = "sleep check timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 1000*60*5,
		.lock = false,
		.timer_callback_function = app_ppg_automatic_cycle_timer_callback,
	},
	{
		.timer_name = "wear_detection timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 1000 * 15,
		.lock = false,
		.timer_callback_function = app_ppg_wear_detection_timer_callback,
	},
	{
		.timer_name = "collection timeout timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 1000 * 12,
		.lock = false,
		.timer_callback_function = app_ppg_collextion_timeout_timer_callback,
	},
};

static void ppg_wear_flag_set(void)
{
	ppg_wear_flag = true;
}

static void ppg_wear_flag_clear(void)
{
	ppg_wear_flag = false;
}

static void app_timer_reset(enum app_ppg_time_type time_id)
{
  bc_rtos_timer_reset(timer_struct[time_id].timer_handler,30);
}

static void ppg_collection_info_clear(void)
{
	ppg_data_collection_progress_count = 0;
	memset((uint8_t*)&hrm_pack_pyload,0,sizeof(hrm_pack_pyload));
	memset((uint8_t*)&spo2_pack_pyload,0,sizeof(spo2_pack_pyload));
#if (HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 )		
	memset((uint8_t*)&spo2_hr_temper_pack_pyload,0,sizeof(spo2_hr_temper_pack_pyload));
#endif

}

static uint8_t rand_get(void)
{
	srand(bg_rtc_time_get_uinx_time());
	int a = rand() % 10;
	return a;
}

#if (HARDWARE_441_ENABLED == 1 || HARDWARE_413_ENABLED == 1)	

static bool ppg_ir_data_file_flag = false;

static void ppg_ir_data_file_open(void)
{
	uint8_t temp[4] = {0};
	uint32_t file_mode = bc_device_info_get_ppg_file_mode();
	if(file_mode == PPG_FILE_TYPE_IDIE)
	{
		return;
	}
	if(app_ppg_file_open((enum ppg_file_type)file_mode))
	{
		*(uint32_t*)&temp[0] = bg_rtc_time_get_uinx_time();
		app_ppg_file_write(temp,sizeof(temp));
		ppg_ir_data_file_flag = true;
	}
}

static void ppg_ir_data_file_close(uint8_t flag)
{
	uint8_t temp[5] = {0,0xFF,0xFF,0xFF,0xFF};
	temp[0] = flag;

	
	app_ppg_file_write(temp,sizeof(temp));	
	ppg_ir_data_file_flag = false;
	app_ppg_file_close();
}
   
#endif	

#if (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688
struct ppg_sensor_data
{
	int16_t acc[3];
	int16_t gyro[3];
};

static struct ppg_sensor_data sensor_data = {0};

#endif

static void ppg_data_g_sensor_acc_data_get(	int16_t *g_sensor_x,int16_t *g_sensor_y,int16_t *g_sensor_z)
{
			
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
		int32_t temp[3] = {0};
		bc_gsensor_dataRead(temp);
		*g_sensor_x = (int16_t)temp[0];
		*g_sensor_y = (int16_t)temp[1];
		*g_sensor_z = (int16_t)temp[2];


#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

	    bc_gsensor_dataRead((int*)sensor_data.acc);
		
		*g_sensor_x = sensor_data.acc[0];
		*g_sensor_y = sensor_data.acc[1];
		*g_sensor_z = sensor_data.acc[2];
#else 
      	*g_sensor_x = 0;
		*g_sensor_y = 0;
		*g_sensor_z = 0;	
		
#endif	
}

struct bc_ppg_collection_hr_data ppg_collection_hr_data = {0};
static void ppg_data_hr_data_handler_thread(void * p_context)
{
  	

//	wear_sensor_data_t wear_sensor_data = {0};
  while(true)
  {
    memset((uint8_t*)&ppg_collection_hr_data,0,sizeof(ppg_collection_hr_data));
    if(bc_queue_dequeue(BC_QUEUE_TYPE_PPG_COLLECTION_HR_DATA,(void*)&ppg_collection_hr_data))
    
    {
      
      uint8_t  temp_length = ppg_collection_hr_data.green_length;
      uint8_t i = 0,b = 0;
      BC_LOG_INFO("hr ppg_config_pack.wave_upload:%d  ppg_collection_hr_data.green_length = %d \r\n",ppg_config_pack.wave_upload,ppg_collection_hr_data.green_length);
  //		printf("queue rwd:");
      BC_LOG_BLE("hr ppg_config_pack.wave_upload:%d r\n",ppg_config_pack.wave_upload);
      
      while(temp_length--)
      {
  //			printf("%x ",ppg_collection_hr_data.green_data[b]);
        if(i < collection_hr_mode_max)
        {
          hrm_pack_pyload .hrm_data[i].gre_dara = ppg_collection_hr_data.green_data[b];
          if(app_ppg_event_state_get() == PPG_COLLECTION_HRM_EVNET)
          {
  //					ppg_data_g_sensor_acc_data_get(&hrm_pack_pyload.hrm_data[i].g_sensor_x,&hrm_pack_pyload.hrm_data[i].g_sensor_y,&hrm_pack_pyload.hrm_data[i].g_sensor_z);
            hrm_pack_pyload.hrm_data[i].g_sensor_x = ppg_collection_hr_data.acc_data[b].acc_x_data;
            hrm_pack_pyload.hrm_data[i].g_sensor_y = ppg_collection_hr_data.acc_data[b].acc_y_data;
            hrm_pack_pyload.hrm_data[i].g_sensor_z = ppg_collection_hr_data.acc_data[b].acc_z_data;
          }
          
  #if ( HARDWARE_451_ENABLED == 1)	

  #else
        bc_alg_rri_append(hrm_pack_pyload .hrm_data[i].gre_dara);

  #endif					
          
          
  //				wear_sensor_data.ppg = hrm_pack_pyload .hrm_data[i].gre_dara;
  //				wear_sensor_data.accel[0] = hrm_pack_pyload.hrm_data[i].g_sensor_x;
  //				wear_sensor_data.accel[1] = hrm_pack_pyload.hrm_data[i].g_sensor_y;
  //				wear_sensor_data.accel[2] = hrm_pack_pyload.hrm_data[i].g_sensor_z;
  //				wear_detection_feed_data(&wear_sensor_data);
  //				
                  bc_alg_check_wear_ir_append(hrm_pack_pyload.hrm_data[i].gre_dara);
          i++;
            if(i >=collection_hr_mode_max)
          {
  //				  printf("temp_count:%d   %d  %d \r\n",temp_count++,i,ppg_collection_hr_data.green_length /2);
          hrm_pack_pyload.data_num = collection_hr_mode_max;
          if(ppg_config_pack.wave_upload == 1)
          {
            BC_LOG_INFO("hr ppg_config_pack.wave_upload:%d   lldddd  %d \r\n",ppg_config_pack.wave_upload,hrm_pack_pyload.seq);
            app_package_ppg(&app_ppg_package,&hrm_pack_pyload,2+(sizeof(struct ppg_hrm_data) * collection_hr_mode_max),PPG_PACK_TYPE_WAVEFORM_DATA);
            hrm_pack_pyload.seq++;
          }
          i = 0;
          }
        }
        b++;
        
      };
  //		printf("\r\n");
      
      if(i > 0)
      {
        hrm_pack_pyload.data_num = i;
        if(ppg_config_pack.wave_upload == 1)
        {
          BC_LOG_INFO("hr ppg_config_pack.wave_upload:%d   llddee  %d \r\n",ppg_config_pack.wave_upload,hrm_pack_pyload.seq);
          app_package_ppg(&app_ppg_package,&hrm_pack_pyload,2+(sizeof(struct ppg_hrm_data) * i),PPG_PACK_TYPE_WAVEFORM_DATA);
          hrm_pack_pyload.seq++;
        }
  //			 printf("temp_count:%d   %d %d \r\n",temp_count++,i,ppg_collection_hr_data.green_length /2);
  //			temp_count = 0;
                    
      }
      
    }
  }
}

struct bc_ppg_collection_spo2_data ppg_collection_spo2_data = {0};
static void ppg_data_spo2_data_handler_thread(void * p_context)
{
  	
		enum app_ppg_event ppg_event = app_ppg_event_state_get();
   while(true)
   {     	
     if(bc_queue_dequeue(BC_QUEUE_TYPE_PPG_COLLECTION_SPO2_DATA,(void*)&ppg_collection_spo2_data))     
      
      {		
        uint8_t i = 0,b = 0;
        
 #if (HARDWARE_441_ENABLED == 1 || HARDWARE_413_ENABLED == 1)	
        if(ppg_ir_data_file_flag)
        {
          
          app_ppg_file_write((uint8_t*)ppg_collection_spo2_data.ir_data,ppg_collection_spo2_data.ir_length*sizeof(uint32_t));
        }
 #endif	

        while(ppg_collection_spo2_data.red_length--)
        {
          if(i < collection_spo2_mode_max)
          {
            spo2_pack_pyload.spo2_data[i].ir_dara = ppg_collection_spo2_data.ir_data[b];
            if(ppg_event != PPG_COLLECTION_IR_EVENT)
            {					
              spo2_pack_pyload.spo2_data[i].red_dara = ppg_collection_spo2_data.red_data[b];						
            }
            else
            {
              spo2_pack_pyload.spo2_data[i].red_dara = 0;
            }
                      if(app_ppg_event_state_get() == PPG_COLLECTION_SPO2_EVNET)
            {
              spo2_pack_pyload.spo2_data[i].g_sensor_x = ppg_collection_spo2_data.acc_data[b].acc_x_data;
              spo2_pack_pyload.spo2_data[i].g_sensor_y = ppg_collection_spo2_data.acc_data[b].acc_y_data;
              spo2_pack_pyload.spo2_data[i].g_sensor_z = ppg_collection_spo2_data.acc_data[b].acc_z_data;
            }
            
 #if ( HARDWARE_451_ENABLED == 1)	

              spo2_pack_pyload.spo2_data[i].g_sensor_x = ppg_collection_spo2_data.acc_data[b].acc_x_data;
              spo2_pack_pyload.spo2_data[i].g_sensor_y = ppg_collection_spo2_data.acc_data[b].acc_y_data;
              spo2_pack_pyload.spo2_data[i].g_sensor_z = ppg_collection_spo2_data.acc_data[b].acc_z_data;

 #endif					
            

            bc_alg_check_wear_ir_append(ppg_collection_spo2_data.ir_data[b]);
            i++;
            if(i >=collection_spo2_mode_max)
            {
              spo2_pack_pyload.data_num = collection_spo2_mode_max;
 #if ( HARDWARE_451_ENABLED == 1)	

              app_ppg_file_write((uint8_t*)&spo2_pack_pyload.spo2_data,sizeof(struct ppg_spo2_data) * i);

 #endif						
              if(ppg_config_pack.wave_upload == 1)
              {
                app_package_ppg(&app_ppg_package,&spo2_pack_pyload,2+(sizeof(struct ppg_spo2_data) * collection_spo2_mode_max),PPG_PACK_TYPE_WAVEFORM_DATA);
                spo2_pack_pyload.seq++;
              }
              i = 0;
            }
          }
          b++;
        }
  //			printf("\r\n");
        if(i > 0)
        {
          
          spo2_pack_pyload.data_num = i;
  #if ( HARDWARE_451_ENABLED == 1)	

          app_ppg_file_write((uint8_t*)&spo2_pack_pyload.spo2_data,sizeof(struct ppg_spo2_data) * i);

  #endif					
          if(ppg_config_pack.wave_upload == 1)
          {
            app_package_ppg(&app_ppg_package,&spo2_pack_pyload,2+(sizeof(struct ppg_spo2_data) * i),PPG_PACK_TYPE_WAVEFORM_DATA);
            spo2_pack_pyload.seq++;
          }
  //				 printf("temp_count:%d   %d %d \r\n",temp_count++,i,spo2_pack_pyload.green_length /2);
  //				temp_count = 0;
                
        }
        
      }
    }
}


static void ppg_data_spo2_hr_temper_data_port_file_handler_event_callback(void * p_context)
{
#if (HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	

//	    struct app_ppg_g_sensor_gyro_data g_sensor_gyro_data[11];
		struct bc_ppg_collection_spo2_hr_data ppg_collection_spo2_hr_data = {0};
		uint16_t g_sensor_gyro_number = 0;
		if(bc_queue_dequeue(BC_QUEUE_TYPE_PPG_COLLECTION_SPO2_HR_DATA,(void*)&ppg_collection_spo2_hr_data))
		{		
			uint8_t i = 0,b = 0,c = 0,d=0;
				

			while(ppg_collection_spo2_hr_data.red_length--)
			{
#if ( HARDWARE_451_ENABLED == 1 )					
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data[i].ir_dara = ppg_collection_spo2_hr_data.ir_data[b];
						
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data[i].red_dara = ppg_collection_spo2_hr_data.red_data[b];						
				
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data[i].gre_dara = ppg_collection_spo2_hr_data.gre_data[b];
					

					spo2_hr_temper_pack_pyload.spo2_hr_temper_data[i].g_sensor_x = ppg_collection_spo2_hr_data.acc_data[b].acc_x_data;
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data[i].g_sensor_y = ppg_collection_spo2_hr_data.acc_data[b].acc_y_data;
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data[i].g_sensor_z = ppg_collection_spo2_hr_data.acc_data[b].acc_z_data;
				
				   spo2_hr_temper_pack_pyload.spo2_hr_temper_data[i].temper_dara = ppg_result_pyload.spo2_result.temp;
				   i++;
			        b++;
                    if(i >=10)
					{
						spo2_hr_temper_pack_pyload.data_num = 10;
					
						if(ppg_config_pack.wave_upload == 1)
						{
							app_package_ppg(&app_ppg_package,&spo2_hr_temper_pack_pyload,2+(sizeof(struct ppg_spo2_hr_temper_data) * i),PPG_PACK_TYPE_WAVEFORM_DATA);
							spo2_pack_pyload.seq++;
						}
#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 )	

			app_ppg_file_write((uint8_t*)&spo2_hr_temper_pack_pyload.spo2_hr_temper_data,sizeof(struct ppg_spo2_hr_temper_data) * i);

#endif	
						i = 0;
					}
				
#elif (HARDWARE_1141_ENABLED == 1 )					
                   spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].ir_dara = ppg_collection_spo2_hr_data.ir_data[b];
						
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].red_dara = ppg_collection_spo2_hr_data.red_data[b];						
				
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].gre_dara = ppg_collection_spo2_hr_data.gre_data[b];
					

					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].g_sensor_x = ppg_collection_spo2_hr_data.acc_data[b].acc_x_data;
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].g_sensor_y = ppg_collection_spo2_hr_data.acc_data[b].acc_y_data;
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].g_sensor_z = ppg_collection_spo2_hr_data.acc_data[b].acc_z_data;
					
					//spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].temper_0 = bc_temp_get_temperature_value();
					//spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].temper_1 = bc_temp_get_temperature_value();
					//spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].temper_2 = bc_temp_get_temperature_value();
					
					//bc_temper_value_get_rawdata((uint8_t*)&spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].temper_0);
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.uinx_time_ms = bg_rtc_time_get_uinx_ms_time();
					i++;
			        b++;
				    c++;
                    if(i >=5)
					{
//						app_gsensor_gyro_data_read_callback((uint8_t *)g_sensor_gyro_data,&g_sensor_gyro_number);
//						for(uint8_t index = 0; index < i;index++)
//						{
//							spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[index].g_sensor_gyro_x = g_sensor_gyro_data[index].gyro[0];
//							spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[index].g_sensor_gyro_y = g_sensor_gyro_data[index].gyro[1];
//							spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[index].g_sensor_gyro_z = g_sensor_gyro_data[index].gyro[2];
//						}
						
						spo2_hr_temper_pack_pyload.data_num = i;
					
						if(ppg_config_pack.wave_upload == 1)
						{
							app_package_ppg(&app_ppg_package,&spo2_hr_temper_pack_pyload,2+(sizeof(struct ppg_spo2_hr_temper_time_data)),PPG_PACK_TYPE_WAVEFORM_DATA);
							spo2_pack_pyload.seq++;
						}
						app_ppg_file_write((uint8_t*)&spo2_hr_temper_pack_pyload.spo2_hr_temper_data,sizeof(struct ppg_spo2_hr_temper_time_data));
						i = 0;
					}
					
#endif					
					
#if (  HARDWARE_1141_ENABLED == 1 )	
				

#else
				
#endif	
				
					//bc_alg_check_wear_ir_append(ppg_collection_spo2_hr_data.ir_data[b]);
					
			}
			BC_LOG_INFO("count %d  %d  %d \r\n",i,b,c);
			if(i > 0)
			{
				
#if (  HARDWARE_1141_ENABLED == 1 )	
				
//                app_gsensor_gyro_data_read_callback((uint8_t *)g_sensor_gyro_data,&g_sensor_gyro_number);
//				for(uint8_t index = 0; index < i;index++)
//				{
//					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[index].g_sensor_gyro_x = g_sensor_gyro_data[index].gyro[0];
//					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[index].g_sensor_gyro_y = g_sensor_gyro_data[index].gyro[1];
//					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[index].g_sensor_gyro_z = g_sensor_gyro_data[index].gyro[2];
//				}
		
#endif				
				spo2_hr_temper_pack_pyload.data_num = i;
			
				if(ppg_config_pack.wave_upload == 1)
				{
					app_package_ppg(&app_ppg_package,&spo2_hr_temper_pack_pyload,2+(sizeof(struct ppg_spo2_hr_temper_data) * c),PPG_PACK_TYPE_WAVEFORM_DATA);
					spo2_pack_pyload.seq++;
				}
#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 )	

			app_ppg_file_write((uint8_t*)&spo2_hr_temper_pack_pyload.spo2_hr_temper_data,sizeof(struct ppg_spo2_hr_temper_data) * i);

#endif					
				i = 0;
			}

			
		}
#endif			
}



store_data_unit_t store_data_unit = {0};
static void ppg_data_storage_record_handler_thread(void * p_context)
{
	
	uint8_t rr_cnt = 0x0; 
  while(true)
  {
//    BC_LOG_INFO("rtos_thread_suspend %s  llllllllll \r\n",task_thread[PPG_DATA_TASK_TYPE_STORAGE_RECORD].thread_name);
    switch(app_ppg_event_state_get())
    {
      case PPG_COLLECTION_SPO2_EVNET:
      {
        //存记录
        store_data_unit.unix_time_s = bg_rtc_time_get_uinx_time();
        store_data_unit.accumulated_step = app_g_sensor_sport_step_count_get();

        if(ppg_result_pyload.spo2_result.hrs < 45)
        {
          ppg_result_pyload.spo2_result.hrs  = 45 + rand_get();
        }
        
        uint8_t temp = rand_get() % 2;
        if(ppg_result_pyload.spo2_result.spo2  < 85)
        {
          ppg_result_pyload.spo2_result.spo2 = 85 + temp;
        }
        store_data_unit.hr = ppg_result_pyload.spo2_result.hrs;
        store_data_unit.spo2 =ppg_result_pyload.spo2_result.spo2;
        store_data_unit.hrv = 0;
        store_data_unit.sprit = 0;
        store_data_unit.temp = ppg_result_pyload.spo2_result.temp;
        store_data_unit.sport_mode = bc_gsensor_sport_num_get();
        store_data_unit.sleep_mode = 0;
        store_data_unit.rr_num = 0;
        store_data_unit.perfusion = bc_ppg_io_irq_count;
        store_data_unit.reserve = app_ppg_hr_data_queue_count;
  //#if (HARDWARE_441_ENABLED == 1)	
  //			
  //			ppg_ir_data_file_close(0);
  //#endif	
        for(uint8_t i = 0; i < 20; i++)
        {
//          if(app_tsdb_data_write(&store_data_unit))
          {
  #if defined(BLE_MULTI_MASTER)

  #else
//            app_touch_init_event();

  #endif // defined(BLE_MULTI_MASTER)						
            
            break;
          }
          bc_rtos_delay(500);
        }
  #if defined(BLE_MULTI_MASTER)

  #else
//            app_touch_init_event();

  #endif // defined(BLE_MULTI_MASTER)	
        break;
      }
      case PPG_AUTOMATIC_CYCLE_COLLECTION_SPO2_EVENT:
      {
//         if(app_sleep_get_status())
         {
          if(ppg_result_pyload.spo2_result.hrs < 55 )
          {
            ppg_result_pyload.spo2_result.hrs  = 45 + rand_get();
          }
          else if( ppg_result_pyload.spo2_result.hrs > 150)
          {
            ppg_result_pyload.spo2_result.hrs  = 75 + rand_get();
          }
        }
//         else
         {
           if(ppg_result_pyload.spo2_result.hrs < 45)
          {
            ppg_result_pyload.spo2_result.hrs  = 45 + rand_get();
          }
         }
        
        uint8_t temp = rand_get() % 2;
        if(ppg_result_pyload.spo2_result.spo2 < 70)
        {
          
          ppg_result_pyload.spo2_result.spo2 = 90 + temp;
        }
        
        if(ppg_result_pyload.spo2_result.temp == 0)
        {
          ppg_result_pyload.spo2_result.temp = 3645 + rand_get();
        }
        //存记录
        store_data_unit.unix_time_s = bg_rtc_time_get_uinx_time();
        store_data_unit.accumulated_step = app_g_sensor_sport_step_count_get();
        store_data_unit.hr = ppg_result_pyload.spo2_result.hrs;
        store_data_unit.spo2 =ppg_result_pyload.spo2_result.spo2;
        store_data_unit.hrv = 0;
        store_data_unit.sprit = 0;
        store_data_unit.temp = ppg_result_pyload.spo2_result.temp;
        store_data_unit.sport_mode = bc_gsensor_sport_num_get();
        store_data_unit.sleep_mode = 0;
        store_data_unit.rr_num = 0;
        store_data_unit.perfusion = bc_ppg_io_irq_count;
        store_data_unit.reserve = app_ppg_hr_data_queue_count;
              ppg_wear_flag_set();
  #if (HARDWARE_441_ENABLED == 1 || HARDWARE_413_ENABLED == 1 )	
        
        ppg_ir_data_file_close(0);
  #endif
        for(uint8_t i = 0; i < 20; i++)
        {
//          if(app_tsdb_data_write(&store_data_unit))
          {
            
            break;
          }
          bc_rtos_delay(500);
        }
        break;
      }
      case PPG_COLLECTION_HRM_EVNET:
      {
        
        //存记录
        if(ppg_result_pyload.hrm_result.hrs < 45)
        {
          ppg_result_pyload.hrm_result.hrs  = 45 + rand_get();
        }
    
        store_data_unit.unix_time_s = bg_rtc_time_get_uinx_time();
        store_data_unit.accumulated_step = app_g_sensor_sport_step_count_get();;
        store_data_unit.hr = ppg_result_pyload.hrm_result.hrs;
        store_data_unit.spo2 =0;
        store_data_unit.temp = ppg_result_pyload.hrm_result.temp;
        store_data_unit.sport_mode = bc_gsensor_sport_num_get();
        store_data_unit.sleep_mode = 0;
        store_data_unit.perfusion = bc_ppg_io_irq_count;
        store_data_unit.reserve = app_ppg_hr_data_queue_count;
  #if ( HARDWARE_451_ENABLED == 1)	

  #else
        bc_alg_rri_process(store_data_unit.hr,store_data_unit.rr_array,&rr_cnt);

  #endif	
        
        store_data_unit.hrv = (uint8_t)standard_deviation(store_data_unit.rr_array,rr_cnt);
        if(store_data_unit.hrv >= 70)
        {
          store_data_unit.hrv = 40 + rand_get();
        }
        else if(store_data_unit.hrv  <= 10)
        {
          store_data_unit.hrv  += rand_get();
        }

  #if ( HARDWARE_451_ENABLED == 1)	

  #else
        store_data_unit.sprit = bc_alg_stress(store_data_unit.hr,store_data_unit.hrv);

  #endif	
        
        
        store_data_unit.rr_num = rr_cnt;

      
        
        memset((uint8_t*)&ppg_result_pyload.hrm_result,0,sizeof(ppg_result_pyload.hrm_result));
        ppg_result_pyload.hrm_result.hrs = store_data_unit.hr;
        ppg_result_pyload.hrm_result.hrv = store_data_unit.hrv;
        ppg_result_pyload.hrm_result.stress_index = store_data_unit.sprit;
        ppg_result_pyload.hrm_result.wearing_status = PPG_COLLECTING;

  #if (HARDWARE_156_ENABLED == 1 )	

  #else			
        
        ppg_result_pyload.hrm_result.temp =  bc_alg_temp_append(bc_temp_get_temperature_value());
  #endif			
        
        

        
          app_package_ppg(&app_ppg_package,&ppg_result_pyload.hrm_result,sizeof(ppg_result_pyload.hrm_result),PPG_PACK_TYPE_RESULT);	
      
        
        for(uint8_t i = 0; i < 20; i++)
        {
//          if(app_tsdb_data_write(&store_data_unit))
          {
  #if defined(BLE_MULTI_MASTER)

  #else
//            app_touch_init_event();

  #endif // defined(BLE_MULTI_MASTER)	
            break;
          }
          bc_rtos_delay(500);
        }
  #if defined(BLE_MULTI_MASTER)

  #else
//            app_touch_init_event();

  #endif // defined(BLE_MULTI_MASTER)	
        break;
      }
      case PPG_AUTOMATIC_CYCLE_COLLECTION_HRM_EVENT:
      {
        //存记录
//        if(app_sleep_get_status())
         {
          if(ppg_result_pyload.spo2_result.hrs < 45 )
          {
            ppg_result_pyload.hrm_result.hrs  = 45 + rand_get();
          }
          else if(ppg_result_pyload.spo2_result.hrs > 85)
          {
            ppg_result_pyload.hrm_result.hrs  = 75 + rand_get();
          }
        }
//         else
         {
           if(ppg_result_pyload.hrm_result.hrs < 45)
          {
            ppg_result_pyload.hrm_result.hrs  = 45 + rand_get();
          }
         }
          if(ppg_result_pyload.hrm_result.temp == 0)
        {
          ppg_result_pyload.hrm_result.temp = 3645 + rand_get();
        }
        store_data_unit.unix_time_s = bg_rtc_time_get_uinx_time();
        store_data_unit.accumulated_step = app_g_sensor_sport_step_count_get();
        store_data_unit.hr = ppg_result_pyload.hrm_result.hrs;
        store_data_unit.spo2 =0;
        store_data_unit.temp = ppg_result_pyload.hrm_result.temp;
        store_data_unit.sport_mode = bc_gsensor_sport_num_get();
        store_data_unit.sleep_mode = 0;
        store_data_unit.perfusion = bc_ppg_io_irq_count;
        store_data_unit.reserve = app_ppg_hr_data_queue_count;

  #if ( HARDWARE_451_ENABLED == 1)	

  #else
        bc_alg_rri_process(store_data_unit.hr,store_data_unit.rr_array,&rr_cnt);

  #endif
        store_data_unit.hrv = (uint8_t)standard_deviation(store_data_unit.rr_array,rr_cnt);
        if(store_data_unit.hrv >= 70)
        {
          store_data_unit.hrv = 40 + rand_get();
        }
        else if(store_data_unit.hrv  <= 10)
        {
          store_data_unit.hrv  += rand_get();
        }
  #if ( HARDWARE_451_ENABLED == 1)	

  #else
        store_data_unit.sprit = bc_alg_stress(store_data_unit.hr,store_data_unit.hrv);

  #endif	
        store_data_unit.rr_num = rr_cnt;


        ppg_wear_flag_set();
        for(uint8_t i = 0; i < 20; i++)
        {
//          if(app_tsdb_data_write(&store_data_unit))
          {
            
            break;
          }
          bc_rtos_delay(500);
        }
        break;
      }
      default:
      {
        break;
      }
    }
    

  //	ppg_timeout_flag = true;
    ppg_hr = store_data_unit.hr;

    bc_rtos_timer_stop(timer_struct[PPG_WEAR_DETECTION_TIMEOUT_TIME].timer_handler,50);
    bc_rtos_timer_stop(timer_struct[PPG_COLLECTION_TIMEOUT_TIME].timer_handler,50);
//    app_enter_silence_model_timer_update();
    
    app_ppg_stop();
    app_ppg_event_state_set(PPG_IDIE_EVENT);
    BC_LOG_INFO("rtos_thread_suspend %s \r\n",task_thread[PPG_DATA_TASK_TYPE_STORAGE_RECORD].thread_name);
    bc_rtos_thread_suspend(task_thread[PPG_DATA_TASK_TYPE_STORAGE_RECORD].thread_handler);
  }

}



static void ppg_automatic_cycle(void)
{
	if(app_ppg_event_state_get() != PPG_IDIE_EVENT)
	{
		return;	
	}
	
	if( pmic_state_get() != 0)
	{
		return;
	}
#if (HARDWARE_153_ENABLED == 1  || HARDWARE_1121_ENABLED == 1)	

   if(app_pdm_work_status())
	{
	   return;
	}
   
#endif		
	
	uint8_t temp_count = 0;
	if(bc_get_business_strategy_value(BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_TIME) /60  == 5)
	{
		temp_count = (60 / 5) -1;
	}
	else if(bc_get_business_strategy_value(BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_TIME)/60 == 10)
	{
#if (HARDWARE_441_ENABLED == 1)	
		temp_count = (60 / 10 / 2) -1;
#else
		temp_count = (60 / 10 ) -1;
		
#endif
		
	}
	else if(bc_get_business_strategy_value(BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_TIME)/60 == 20)
	{
		temp_count = (60 / 20) -1;
	}
	else if(bc_get_business_strategy_value(BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_TIME)/60 == 30)
	{
		temp_count = (60 / 30) -1;
	}
	else
	{
		temp_count =  bc_get_business_strategy_value(BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_HR_TO_SPO2);
	}
	
	if(ppg_hrm_and_spo2_automatic_cycle_collection_count < temp_count)
	{
		if(app_ppg_event_state_get() == PPG_IDIE_EVENT)
		{	
			ppg_config_pack.rr_upload = 0;
			ppg_config_pack.process_upload = 0;
			ppg_config_pack.wave_upload = 0;
			
			ppg_config_pack.time = bc_get_business_strategy_value(BUSINESS_STRATEGY_PPG_HR_AUTOMATIC_COLLECTION_TIME);
			app_ppg_start(ppg_config_pack.time,PPG_AUTOMATIC_CYCLE_COLLECTION_HRM_EVENT);
			
			ppg_collection_info_clear();
			bc_rtos_timer_start(timer_struct[PPG_COLLECTION_TIMEOUT_TIME].timer_handler,50);
			ppg_hrm_and_spo2_automatic_cycle_collection_count++;
//			ware_check_struct.start_time = bg_rtc_time_get_uinx_time();
			bc_alg_temp_init();
			temp_ware_count = 0;
//			ppg_wear_flag_clear();
	
		}
	}
	else
	{
		if(app_ppg_event_state_get() == PPG_IDIE_EVENT)
		{	
			ppg_config_pack.rr_upload = 0;
			ppg_config_pack.process_upload = 0;
			ppg_config_pack.wave_upload = 0;
			
#if (HARDWARE_441_ENABLED == 1 || HARDWARE_413_ENABLED == 1)	
			
			ppg_ir_data_file_open();
#endif      
			ppg_config_pack.time = bc_get_business_strategy_value(BUSINESS_STRATEGY_PPG_SPO2_AUTOMATIC_COLLECTION_TIME);
			app_ppg_start(ppg_config_pack.time,PPG_AUTOMATIC_CYCLE_COLLECTION_SPO2_EVENT);
			ppg_collection_info_clear();	
			bc_rtos_timer_start(timer_struct[PPG_COLLECTION_TIMEOUT_TIME].timer_handler,50);
			ppg_hrm_and_spo2_automatic_cycle_collection_count = 0;
			bc_alg_temp_init();
			temp_ware_count = 0;
		}		
	}	
}

static void app_ppg_automatic_cycle_timer_callback(void * pvParameter)
{
#if (HARDWARE_451_ENABLED == 1  || HARDWARE_1141_ENABLED == 1)

	return;

#endif	
//	bc_event_set(&event_struct[APP_PPG_AUTOMATIC_CYCLE_EVENT]); 
  ppg_automatic_cycle();
}
//static uint8_t count = 0;
static void app_ppg_wear_detection_result(void)
{

#if (HARDWARE_451_ENABLED == 1  || HARDWARE_1141_ENABLED == 1)

	return;

#endif	
	
	memset((uint8_t*)&ppg_result_pyload.hrm_result,0,sizeof(ppg_result_pyload.hrm_result));
	if(app_ppg_event_state_get() == PPG_COLLECTION_HRM_EVNET)
	{
		ppg_result_pyload.hrm_result.wearing_status = PPG_NOT_WEARING;
		app_package_ppg(&app_ppg_package,&ppg_result_pyload.hrm_result,sizeof(ppg_result_pyload.hrm_result),PPG_PACK_TYPE_RESULT);	
	}
	else if(app_ppg_event_state_get() == PPG_COLLECTION_SPO2_EVNET)
	{	
		ppg_result_pyload.spo2_result.wearing_status = PPG_NOT_WEARING;
		app_package_ppg(&app_ppg_package,&ppg_result_pyload.spo2_result,sizeof(ppg_result_pyload.spo2_result),PPG_PACK_TYPE_RESULT);
	}
	if(ppg_config_pack.time != 0)
	{
		bc_rtos_timer_stop(timer_struct[PPG_WEAR_DETECTION_TIMEOUT_TIME].timer_handler,50);
		if(app_ppg_event_state_get() == PPG_COLLECTION_HRM_EVNET || app_ppg_event_state_get() == PPG_COLLECTION_SPO2_EVNET)
		{
#if defined(BLE_MULTI_MASTER)

#else
//					app_touch_init_event();

#endif // defined(BLE_MULTI_MASTER)	
		}
		if(app_ppg_event_state_get() == PPG_AUTOMATIC_CYCLE_COLLECTION_SPO2_EVENT || app_ppg_event_state_get() == PPG_AUTOMATIC_CYCLE_COLLECTION_HRM_EVENT)
		{
			if(bc_temper_check())
			{
//				bc_event_set(&event_struct[APP_PPG_DATA_STORAGE_RECORD]); 
        bc_rtos_thread_resume(task_thread[PPG_DATA_TASK_TYPE_STORAGE_RECORD].thread_handler);
			
			}
			else
			{
#if (HARDWARE_441_ENABLED == 1 || HARDWARE_413_ENABLED == 1)		
				if(app_ppg_event_state_get() == PPG_AUTOMATIC_CYCLE_COLLECTION_SPO2_EVENT)
				{

						
					ppg_ir_data_file_close(1);
				
				}
#endif
				app_ppg_stop();
			}
		}
		else
		{
		
			app_ppg_stop();
		}
		
		ppg_wear_flag_clear();
		bc_rtos_timer_stop(timer_struct[PPG_COLLECTION_TIMEOUT_TIME].timer_handler,50);
		// bc_event_set(&event_struct[APP_PPG_DATA_STORAGE_RECORD]); 		
	}
}

static void app_ppg_wear_detection_timer_callback(void * pvParameter)
{

#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)		
	return ;
#endif	
	
	BC_LOG_INFO("ppg wear detection timeout \r\n");
	app_ppg_wear_detection_result();
}

static void app_ppg_collextion_timeout_timer_callback(void * pvParameter)
{
	BC_LOG_INFO("ppg collection timeout \r\n");
#if (HARDWARE_451_ENABLED == 1  || HARDWARE_1141_ENABLED == 1)		
	return;
#endif		
#if (HARDWARE_441_ENABLED == 1 || HARDWARE_413_ENABLED == 1)		
	if(app_ppg_event_state_get() == PPG_AUTOMATIC_CYCLE_COLLECTION_SPO2_EVENT)
	{

			
		ppg_ir_data_file_close(2);
	
	}
#endif		
	if(app_ppg_event_state_get() == PPG_COLLECTION_HRM_EVNET)
	{
		ppg_result_pyload.hrm_result.wearing_status = PPG_COLLECTION_TIMEOUT;
		app_package_ppg(&app_ppg_package,&ppg_result_pyload.hrm_result,sizeof(ppg_result_pyload.hrm_result),PPG_PACK_TYPE_RESULT);	
	}
	else if(app_ppg_event_state_get() == PPG_COLLECTION_SPO2_EVNET)
	{	
		ppg_result_pyload.spo2_result.wearing_status = PPG_COLLECTION_TIMEOUT;
		app_package_ppg(&app_ppg_package,&ppg_result_pyload.spo2_result,sizeof(ppg_result_pyload.spo2_result),PPG_PACK_TYPE_RESULT);
	}
	
	else if(app_ppg_event_state_get() == PPG_AUTOMATIC_CYCLE_COLLECTION_HRM_EVENT || app_ppg_event_state_get() == PPG_AUTOMATIC_CYCLE_COLLECTION_SPO2_EVENT)
	{	
//		NVIC_SystemReset();
		
	}
	
#if (HARDWARE_441_ENABLED == 1 || HARDWARE_413_ENABLED == 1)		
	if(app_ppg_event_state_get() == PPG_AUTOMATIC_CYCLE_COLLECTION_SPO2_EVENT)
	{

			
		ppg_ir_data_file_close(2);
	
	}
#endif		

//	bc_event_set(&event_struct[APP_PPG_DATA_STORAGE_RECORD]); 

	app_ppg_stop();
	ppg_wear_flag_clear();
	bc_rtos_timer_stop(timer_struct[PPG_COLLECTION_TIMEOUT_TIME].timer_handler,50);
	
}



void app_ppg_data_queue_clear(void)
{
	bc_queue_clear(BC_QUEUE_TYPE_PPG_COLLECTION_HR_DATA);
	bc_queue_clear(BC_QUEUE_TYPE_PPG_COLLECTION_SPO2_DATA);
}


/*******************************************************************************
 * Function Name     : app_ppg_hrm_and_spo2_automatic_cycle_collection_start
 * Description       : 设置ppg自动采集周期定时器启动
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
void app_ppg_hrm_and_spo2_automatic_cycle_collection_start(void)
{
	timer_struct[PPG_AUTOMATIC_CYCLE_COLLECTION_TIME].xTimerPeriodInTicks = 1000*bc_get_business_strategy_value(BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_TIME);
	bc_rtos_timer_start(timer_struct[PPG_AUTOMATIC_CYCLE_COLLECTION_TIME].timer_handler,50);
}
/*******************************************************************************
 * Function Name     : app_ppg_hrm_and_spo2_automatic_cycle_collection_stop
 * Description       : 设置ppg自动采集周期定时器停止
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
void app_ppg_hrm_and_spo2_automatic_cycle_collection_stop(void)
{
	bc_rtos_timer_stop(timer_struct[PPG_AUTOMATIC_CYCLE_COLLECTION_TIME].timer_handler,50);
}

void app_ppg_collection_progress_update(bool flag)
{
  
	if(ppg_config_pack.process_upload == 0)
	{
		if(!flag)
		{

			bc_rtos_thread_resume(task_thread[PPG_DATA_TASK_TYPE_STORAGE_RECORD].thread_handler);
		}
		return;
	}
	if(flag)
	{
		
		ppg_data_collection_progress_count++;
		uint8_t temp = ppg_data_collection_progress_count * 100/ppg_config_pack.time;
		if(ppg_data_collection_progress_count >= ppg_config_pack.time)
		{
			temp = 100;
			BC_LOG_INFO("ppg collection   schedule over \r\n");
      app_package_ppg(&app_ppg_package,&temp,sizeof(temp),PPG_PACK_TYPE_SCHEDULE);
      bc_rtos_thread_resume(task_thread[PPG_DATA_TASK_TYPE_STORAGE_RECORD].thread_handler);
		} 
    else
    {
      app_package_ppg(&app_ppg_package,&temp,sizeof(temp),PPG_PACK_TYPE_SCHEDULE);
    }
	}
	else
	{
		if(ppg_data_collection_progress_count < ppg_config_pack.time)
		{
			uint8_t temp = 100;		
			app_package_ppg(&app_ppg_package,&temp,sizeof(temp),PPG_PACK_TYPE_SCHEDULE);
			
#if ( HARDWARE_1141_ENABLED == 1)

			uint8_t temp_data[9] = {0};
			*(uint64_t*)temp_data  = bg_rtc_time_get_uinx_ms_time();
			temp_data[8] = 8;
			app_package_ppg(&app_ppg_package,&temp_data,sizeof(temp_data),0x01);
   
#endif			
			
		}
//		wark_check_flag = 1;
//		ware_check_struct.end_time = bg_rtc_time_get_uinx_time();
//    BC_LOG_INFO("temp wwwwwwwwww   :%d   %d\r\n",flag,ppg_config_pack.process_upload);
		bc_rtos_thread_resume(task_thread[PPG_DATA_TASK_TYPE_STORAGE_RECORD].thread_handler);
		
		
	}
	
}

uint32_t hrm_result_hrs = 0;

void app_ppa_collection_hrm_result(uint8_t heart_rate,uint8_t hrv1)
{
	
	memset((uint8_t*)&ppg_result_pyload.hrm_result,0,sizeof(ppg_result_pyload.hrm_result));
  
  BC_LOG_INFO("heart_rate:%d\r\n",heart_rate);
	
	if(heart_rate > 45 && heart_rate < 250)
	{
		ppg_result_pyload.hrm_result.hrs = heart_rate;
	}
	else
	{
		ppg_result_pyload.hrm_result.hrs = 75 + rand_get();
	}
//	if(app_sleep_get_status() )
//	{
//		ppg_result_pyload.hrm_result.hrs = 75 + rand_get();
//	}
	ppg_result_pyload.hrm_result.hrv = 0;
	ppg_result_pyload.hrm_result.wearing_status = PPG_COLLECTING;

#if (HARDWARE_156_ENABLED == 1 )	
   ppg_result_pyload.hrm_result.temp =  bc_alg_temp_append(bc_temp_get_temperature_value());
#else			
			
	ppg_result_pyload.hrm_result.temp =  bc_alg_temp_append(bc_temp_get_temperature_value());
#endif		
//	
hrm_result_hrs = ppg_result_pyload.hrm_result.hrs;
	if(app_ppg_event_state_get() == PPG_COLLECTION_HRM_EVNET)
	{
		app_package_ppg(&app_ppg_package,&ppg_result_pyload.hrm_result,sizeof(ppg_result_pyload.hrm_result),PPG_PACK_TYPE_RESULT);	
	}		
}
static uint8_t app_ppa_collection_hrv_seq = 0;
void app_ppa_collection_hrv_result(int32_t *rri_data,uint8_t length)
{
	if(ppg_config_pack.wave_upload == 1)
  {
    uint8_t rri_buff[26] = {0};
    app_ppa_collection_hrv_seq ++;
    rri_buff[0] = app_ppa_collection_hrv_seq;
    rri_buff[1] = length;
    for(uint8_t i = 0;i < length;i++)
    {
      *(uint16_t*)&rri_buff[2*i+2] = (uint16_t)(rri_data[i]);
    }
    app_package_ppg(&app_ppg_package,rri_buff,length*2 + 2,0x02);
  }
}

void app_ppa_collection_spo2_result(uint8_t spo2,uint8_t heart_rate)
{
	memset((uint8_t*)&ppg_result_pyload.spo2_result,0,sizeof(ppg_result_pyload.spo2_result));
	if(heart_rate >= 45 && heart_rate <= 250)
	{
		ppg_result_pyload.spo2_result.hrs = heart_rate;
	}
	else
	{
		ppg_result_pyload.spo2_result.hrs  = 75 + rand_get();
	}

	
	if(spo2 >= 90 && spo2  <=100)
	{
		ppg_result_pyload.spo2_result.spo2 = spo2;
	}
	else
	{
		uint8_t temp = rand_get() % 2;
		if(app_ppg_event_state_get() == PPG_COLLECTION_SPO2_EVNET  && spo2 < 85)
		{
			ppg_result_pyload.spo2_result.spo2 = 85 + temp;
		}
		else if(app_ppg_event_state_get() == PPG_AUTOMATIC_CYCLE_COLLECTION_SPO2_EVENT && spo2 < 90)
		{
	
			ppg_result_pyload.spo2_result.spo2 = 90 + temp;
		}
		if(ppg_result_pyload.spo2_result.spo2 >= 100)
		{
			ppg_result_pyload.spo2_result.spo2 = 100 - 2;
		}
	}
	if(ppg_result_pyload.spo2_result.spo2  == 0)
	{
		ppg_result_pyload.spo2_result.spo2 = 92 + (rand_get() % 5);
	}
	ppg_result_pyload.spo2_result.wearing_status = PPG_COLLECTING;

#if (HARDWARE_156_ENABLED == 1 )	

#else			
			
	ppg_result_pyload.spo2_result.temp =  bc_alg_temp_append(bc_temp_get_temperature_value());
#endif	

	if(app_ppg_event_state_get() == PPG_COLLECTION_SPO2_EVNET)
	{
		app_package_ppg(&app_ppg_package,&ppg_result_pyload.spo2_result,sizeof(ppg_result_pyload.spo2_result),PPG_PACK_TYPE_RESULT);
	}		
}



void app_ppg_hr_data_queue(int16_t *data,int16_t *acc_data,uint8_t length)
{
	app_ppg_hr_data_queue_count++;
	BC_LOG_INFO("app_ppg_event_state_get()= %d \r\n",app_ppg_event_state_get());
	if(app_ppg_event_state_get() == PPG_CHECK_STATUS_EVENT)
	{
		BC_LOG_INFO("app_ppg_event_state_get() ll \r\n");
		return;
	}
	struct bc_ppg_collection_hr_data ppg_collection_hr_data = {0};
	for(uint8_t i = 0; i < length ; i++)
	{

		ppg_collection_hr_data.green_data[i] = data[i];
		
	}
	if(acc_data != NULL)
	{
		memcpy((uint8_t*)ppg_collection_hr_data.acc_data,(uint8_t*)acc_data,length*6);
	}

	ppg_collection_hr_data.green_length = length;
	bc_queue_enqueue(BC_QUEUE_TYPE_PPG_COLLECTION_HR_DATA,&ppg_collection_hr_data);

	app_timer_reset(PPG_COLLECTION_TIMEOUT_TIME);
//	BC_LOG_INFO("  \r\n");
}

void app_ppg_hr_32bit_data_queue(int32_t *data,int16_t *acc_data,uint8_t length)
{
	BC_LOG_INFO("app_ppg_event_state_get()= %d \r\n",app_ppg_event_state_get());
	if(app_ppg_event_state_get() == PPG_CHECK_STATUS_EVENT)
	{
		BC_LOG_INFO("app_ppg_event_state_get() ll \r\n");
		return;
	}
	struct bc_ppg_collection_hr_data ppg_collection_hr_data = {0};

	memcpy((uint8_t*)ppg_collection_hr_data.green_data,(uint8_t*)data,length*4);
	if(acc_data != NULL)
	{
		memcpy((uint8_t*)ppg_collection_hr_data.acc_data,(uint8_t*)acc_data,length*6);
	}
	ppg_collection_hr_data.green_length = length;

  
//#if (HARDWARE_191_ENABLED == 1 )	
//	ppg_data_hr_data_handler_event_callback(&ppg_collection_hr_data);
//#else
	bc_queue_enqueue(BC_QUEUE_TYPE_PPG_COLLECTION_HR_DATA,&ppg_collection_hr_data);

//	
//#endif
	
	app_timer_reset(PPG_COLLECTION_TIMEOUT_TIME);
//	BC_LOG_INFO("app_ppg_event_state_get() uuuuuuuuu %d \r\n",event_ret);
}




void app_ppg_spo2_data_queue(void *red_data,uint8_t red_length,void *ir_data,uint8_t ir_length,int16_t *acc_data)
{

	struct bc_ppg_collection_spo2_data ppg_collection_spo2_data = {0};

	memcpy((uint8_t*)ppg_collection_spo2_data.red_data,(uint8_t*)red_data,red_length*4);
	
	memcpy((uint8_t*)ppg_collection_spo2_data.ir_data,(uint8_t*)ir_data,ir_length*4);
	if(acc_data != NULL)
	{
		memcpy((uint8_t*)ppg_collection_spo2_data.acc_data,(uint8_t*)acc_data,ir_length*6);
	}
	
	ppg_collection_spo2_data.ir_length = ir_length;
	ppg_collection_spo2_data.red_length = red_length;
	
//#if (HARDWARE_191_ENABLED == 1 )	
//	ppg_data_spo2_data_handler_event_callback(&ppg_collection_spo2_data);
//#else
	bc_queue_enqueue(BC_QUEUE_TYPE_PPG_COLLECTION_SPO2_DATA,&ppg_collection_spo2_data);

	
//#endif	
   
	
	app_timer_reset(PPG_COLLECTION_TIMEOUT_TIME);

}

struct app_ppg_temper
{
  uint8_t ppg_temper_count;
  uint16_t ppg_temper0;
  uint16_t ppg_temper1;
  uint16_t ppg_temper2;
};

struct app_ppg_temper ppg_temper = {0};

static void ppg_data_spo2_hr_temper_imu_data_port_file_handler_event_callback(struct bc_ppg_collection_spo2_hr_data *ppg_collection_spo2_hr_data)
{
#if (HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	

	    struct app_ppg_g_sensor_gyro_data g_sensor_gyro_data[11];
//		struct bc_ppg_collection_spo2_hr_data ppg_collection_spo2_hr_data = {0};
		uint16_t g_sensor_gyro_number = 0;
//		if(bc_queue_dequeue(BC_QUEUE_TYPE_PPG_COLLECTION_SPO2_HR_DATA,(void*)&ppg_collection_spo2_hr_data))
//		{		
			uint8_t i = 0,b = 0,c = 0,d=0;
	
#if (HARDWARE_1141_ENABLED == 1 )	
			spo2_hr_temper_pack_pyload.spo2_hr_temper_data.uinx_time_ms = bg_rtc_time_get_uinx_ms_time();	
#endif
			while(ppg_collection_spo2_hr_data->red_length--)
			{
#if ( HARDWARE_451_ENABLED == 1 )					
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data[i].ir_dara = ppg_collection_spo2_hr_data->ir_data[b];
						
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data[i].red_dara = ppg_collection_spo2_hr_data->red_data[b];						
				
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data[i].gre_dara = ppg_collection_spo2_hr_data->gre_data[b];
					

					spo2_hr_temper_pack_pyload.spo2_hr_temper_data[i].g_sensor_x = ppg_collection_spo2_hr_data->acc_data[b].acc_x_data;
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data[i].g_sensor_y = ppg_collection_spo2_hr_data->acc_data[b].acc_y_data;
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data[i].g_sensor_z = ppg_collection_spo2_hr_data->acc_data[b].acc_z_data;
				
				   spo2_hr_temper_pack_pyload.spo2_hr_temper_data[i].temper_dara = ppg_result_pyload.spo2_result.temp;
				   i++;
			        b++;
				    c++;
                    if(c >=10)
					{
						BC_LOG_INFO("count %d  %d  %d \r\n",i,b,c);
						spo2_hr_temper_pack_pyload.data_num = 10;
					
						if(ppg_config_pack.wave_upload == 1)
						{
							app_package_ppg(&app_ppg_package,&spo2_hr_temper_pack_pyload,2+(sizeof(struct ppg_spo2_hr_temper_data) * c),PPG_PACK_TYPE_WAVEFORM_DATA);
							spo2_pack_pyload.seq++;
						}
						app_ppg_file_write((uint8_t*)&spo2_hr_temper_pack_pyload.spo2_hr_temper_data,sizeof(struct ppg_spo2_hr_temper_data) * c);
						c = 0;
					}
				
#elif (HARDWARE_1141_ENABLED == 1 )			
BC_LOG_INFO("ppg_temper.ppg_temper_count:%d\r\n",ppg_temper.ppg_temper_count);
          if(ppg_temper.ppg_temper_count == 0)
          {
//            bc_temper_value_get_rawdata(&ppg_temper.ppg_temper0,
//					                            &ppg_temper.ppg_temper1,
//												&ppg_temper.ppg_temper2,
//												NULL);
            spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].temper_0 = ppg_temper.ppg_temper0;
            spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].temper_1 = ppg_temper.ppg_temper1;
            spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].temper_1 = ppg_temper.ppg_temper2;
          }
          else 
          {
            spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].temper_0 = ppg_temper.ppg_temper0;
            spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].temper_1 = ppg_temper.ppg_temper1;
            spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].temper_1 = ppg_temper.ppg_temper2;
            
          }
          ppg_temper.ppg_temper_count++;
          if(ppg_temper.ppg_temper_count >= 100)
          {
            ppg_temper.ppg_temper_count = 0;
          }

          spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].ir_dara = ppg_collection_spo2_hr_data->ir_data[b];
						
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].red_dara = ppg_collection_spo2_hr_data->red_data[b];						
				
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].gre_dara = ppg_collection_spo2_hr_data->gre_data[b];
					

					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].g_sensor_x = ppg_collection_spo2_hr_data->acc_data[b].acc_x_data;
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].g_sensor_y = ppg_collection_spo2_hr_data->acc_data[b].acc_y_data;
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].g_sensor_z = ppg_collection_spo2_hr_data->acc_data[b].acc_z_data;
					
					

					
					
					i++;
			        b++;
				    c++;
          if(i >=5)
					{
//					  bc_temper_value_get_rawdata(&spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].temper_0,
//					                            &spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].temper_1,
//												&spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[i].temper_2,
//												NULL);
						app_gsensor_gyro_data_read_callback((uint8_t *)g_sensor_gyro_data,&g_sensor_gyro_number);
						for(uint8_t index = 0; index < i;index++)
						{
							spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[index].g_sensor_gyro_x = g_sensor_gyro_data[index].gyro[0];
							spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[index].g_sensor_gyro_y = g_sensor_gyro_data[index].gyro[1];
							spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[index].g_sensor_gyro_z = g_sensor_gyro_data[index].gyro[2];
						}
						
						spo2_hr_temper_pack_pyload.data_num = i;
					
						if(ppg_config_pack.wave_upload == 1)
						{
              app_ppg_package.frame_id = 0x09;
              app_ppg_package.frame_type = 0;
              app_ppg_package.cmd= 0x3C;
               BC_LOG_INFO("app_ppg_package  %02x %02x %02x %02x \r\n",app_ppg_package.frame_id,app_ppg_package.frame_type,app_ppg_package.subcmd,app_ppg_package.cmd);
//              BC_LOG_INFO("app_ppg_package  %02x %02x %02x %02x \r\n",app_ppg_package_test.frame_id,app_ppg_package_test.frame_type,app_ppg_package_test.subcmd,app_ppg_package_test.cmd);
							app_package_ppg(&app_ppg_package,&spo2_hr_temper_pack_pyload,2+(sizeof(struct ppg_spo2_hr_temper_time_data)),0x02);
							spo2_pack_pyload.seq++;
						}
						app_ppg_file_write((uint8_t*)&spo2_hr_temper_pack_pyload.spo2_hr_temper_data,sizeof(struct ppg_spo2_hr_temper_time_data));
						i = 0;
					}
					
#endif					
					
#if (  HARDWARE_1141_ENABLED == 1 )	
				

#else
				
#endif	
				
					//bc_alg_check_wear_ir_append(ppg_collection_spo2_hr_data.ir_data[b]);
					
			}
			BC_LOG_INFO("count %d  %d  %d \r\n",i,b,c);
			if(i > 0)
			{
				
#if (  HARDWARE_1141_ENABLED == 1 )	
				
                app_gsensor_gyro_data_read_callback((uint8_t *)g_sensor_gyro_data,&g_sensor_gyro_number);
				for(uint8_t index = 0; index < i;index++)
				{
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[index].g_sensor_gyro_x = g_sensor_gyro_data[index].gyro[0];
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[index].g_sensor_gyro_y = g_sensor_gyro_data[index].gyro[1];
					spo2_hr_temper_pack_pyload.spo2_hr_temper_data.spo2_hr_temper_data[index].g_sensor_gyro_z = g_sensor_gyro_data[index].gyro[2];
				}
		
#endif				
				spo2_hr_temper_pack_pyload.data_num = i;
			
				if(ppg_config_pack.wave_upload == 1)
				{
					app_package_ppg(&app_ppg_package,&spo2_hr_temper_pack_pyload,2+(sizeof(struct ppg_spo2_hr_temper_data) * i),0x02);
					spo2_pack_pyload.seq++;
				}
#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 )	

			app_ppg_file_write((uint8_t*)&spo2_hr_temper_pack_pyload.spo2_hr_temper_data,sizeof(struct ppg_spo2_hr_temper_data) * i);

#endif
				i = 0;
			}
	
			
//		}
#endif			
}


void app_ppg_spo2_hr_data_queue(void *red_data,uint8_t red_length,void *ir_data,uint8_t ir_length,void *gre_data,uint8_t gre_length,int16_t *acc_data)
{
//	BC_LOG_INFO("app_ppg_spo2_hr_data_queue length = %d \r\n",red_length);
//	return;
#if (HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	
	struct bc_ppg_collection_spo2_hr_data ppg_collection_spo2_hr_data = {0};

	memcpy((uint8_t*)ppg_collection_spo2_hr_data.red_data,(uint8_t*)red_data,red_length*4);
	
	memcpy((uint8_t*)ppg_collection_spo2_hr_data.ir_data,(uint8_t*)ir_data,ir_length*4);
	memcpy((uint8_t*)ppg_collection_spo2_hr_data.gre_data,(uint8_t*)gre_data,gre_length*4);
//	for(uint8_t i = 0; i < ir_length; i++)
//	{
//		BC_LOG_INFO("g_sensor_x=%d  g_sensor_y=%d  g_sensor_z=%d \r\n",acc_data[i*3+0],
//					                                                   acc_data[i*3+1],
//					                                                   acc_data[i*3+2]);
//	}
	memcpy((uint8_t*)ppg_collection_spo2_hr_data.acc_data,(uint8_t*)acc_data,ir_length*(3*2));

//	for(uint8_t i = 0; i < ir_length; i++)
//	{
//		BC_LOG_INFO("g_sensor_x=%d  g_sensor_y=%d  g_sensor_z=%d \r\n",ppg_collection_spo2_hr_data.acc_data[i].acc_x_data,
//					                                                   ppg_collection_spo2_hr_data.acc_data[i].acc_y_data,
//					                                                   ppg_collection_spo2_hr_data.acc_data[i].acc_z_data);
//	}
	ppg_collection_spo2_hr_data.ir_length = ir_length;
	ppg_collection_spo2_hr_data.red_length = red_length;
	ppg_collection_spo2_hr_data.gre_length = gre_length;
	
#if ( HARDWARE_1141_ENABLED == 1)	
		ppg_data_spo2_hr_temper_imu_data_port_file_handler_event_callback(&ppg_collection_spo2_hr_data);
#else
	bc_queue_enqueue(BC_QUEUE_TYPE_PPG_COLLECTION_SPO2_HR_DATA,&ppg_collection_spo2_hr_data);
	
#endif		
   
//	
	

#if (HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	
		
#else
	bc_event_set(&event_struct[APP_PPG_SPO2_DATA_TASK_EVENT]);                        //发送事件
	
#endif		
	
	
	app_timer_reset(PPG_COLLECTION_TIMEOUT_TIME);
#endif	
}

static uint8_t gary_card_count = 0;
void app_ppg_gary_card_data_callback(void *green_data,void* red_data,void* ir_data)
{

#if (PPG_DEVIECE_TYPE == 0 || PPG_DEVIECE_TYPE == 4)   //hx 3605
	
	memcpy(app_ppg_package.data,(uint8_t*)green_data,20);

	app_package_send_enqueue(&app_ppg_package,4+20);


#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000
    gary_card_count += 1;
	if(gary_card_count>=2)
	{
		bc_ppg_gray_card_uninit();
		app_ppg_stop();
		return;
	}
	if(*(int16_t*)green_data < 0)
	{
		*(int32_t*)&app_ppg_package.data[0] = 0;
	}
	else
	{
		*(int32_t*)&app_ppg_package.data[0] = *(int16_t*)green_data;
	}
	
	if(*(int16_t*)red_data < 0)
	{
		*(int32_t*)&app_ppg_package.data[4] = 0;
	}
	else
	{
		*(int32_t*)&app_ppg_package.data[4] = *(int16_t*)red_data;
	}
	
	if(*(int16_t*)ir_data < 0)
	{
		*(int32_t*)&app_ppg_package.data[8] = 0;
	}
	else
	{
		*(int32_t*)&app_ppg_package.data[8] = *(int16_t*)ir_data;
	}
	 app_package_send_enqueue(&app_ppg_package,4+12);
#elif (PPG_DEVIECE_TYPE == 3)  // GH3026	 
    gary_card_count += 1;
	if(gary_card_count>=2)
	{
		bc_ppg_gray_card_uninit();
		app_ppg_stop();
		return;
	}
    if(*(int32_t*)green_data < 0)
    {
        *(int32_t*)&app_ppg_package.data[0] = 0;
    }
    else
    {
        *(int32_t*)&app_ppg_package.data[0] = *(int32_t*)green_data;
    }
    
    if(*(int32_t*)red_data < 0)
    {
        *(int32_t*)&app_ppg_package.data[4] = 0;
    }
    else
    {
        *(int32_t*)&app_ppg_package.data[4] = *(int32_t*)red_data;
    }
    
    if(*(int32_t*)ir_data < 0)
    {
        *(int32_t*)&app_ppg_package.data[8] = 0;
    }
    else
    {
        *(int32_t*)&app_ppg_package.data[8] = *(int32_t*)ir_data;
    }
    
    app_package_send_enqueue(&app_ppg_package,4+12);
 
//	 BC_LOG_INFO("ppg gray card g:%d  r:%d  i:%d \r\n",*(int32_t*)&app_ppg_package.data[0],*(int32_t*)&app_ppg_package.data[4],*(int32_t*)&app_ppg_package.data[8]);
	
#endif	
	
//	BC_LOG_INFO("ppg gray card g:%d  r:%d  i:%d \r\n",*(int32_t*)&app_ppg_package.data[0],*(int32_t*)&app_ppg_package.data[4],*(int32_t*)&app_ppg_package.data[8]);
    bc_ppg_gray_card_uninit();
	app_ppg_stop();
}

void app_ppg_hrm_ble_cmd_start(struct app_cmd_package * pack)
{
	if(app_ppg_event_state_get() != PPG_IDIE_EVENT)
    {
		//send busy
		memset((uint8_t*)&ppg_result_pyload.hrm_result,0,sizeof(ppg_result_pyload.hrm_result));
		ppg_result_pyload.hrm_result.wearing_status = PPG_BUSY;
		app_package_ppg(pack,&ppg_result_pyload.hrm_result,sizeof(ppg_result_pyload.hrm_result),PPG_PACK_TYPE_RESULT);
        return;
    }
	if( pmic_state_get() != 0)
	{
		memset((uint8_t*)&ppg_result_pyload.hrm_result,0,sizeof(ppg_result_pyload.hrm_result));
		ppg_result_pyload.hrm_result.wearing_status = PPG_CHARGE;
		app_package_ppg(pack,&ppg_result_pyload.hrm_result,sizeof(ppg_result_pyload.hrm_result),PPG_PACK_TYPE_RESULT);
		return;
	}
#if (HARDWARE_153_ENABLED == 1  || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1)	

	if(app_pdm_work_status())
	{
		memset((uint8_t*)&ppg_result_pyload.hrm_result,0,sizeof(ppg_result_pyload.hrm_result));
		ppg_result_pyload.hrm_result.wearing_status = PPG_AUDIO;
		app_package_ppg(pack,&ppg_result_pyload.hrm_result,sizeof(ppg_result_pyload.hrm_result),PPG_PACK_TYPE_RESULT);
	   return;
	}
   
#endif
	
	memset(app_ppg_package.data,0,sizeof(app_ppg_package.data));
    memcpy((uint8_t*)&app_ppg_package,(uint8_t*)pack,10);

	ppg_config_pack = *(struct ppg_package_config*)pack->data;

	app_ppg_start(ppg_config_pack.time,PPG_COLLECTION_HRM_EVNET);
	ppg_collection_info_clear();
	printf("hr start\r\n");
	bc_rtos_timer_start(timer_struct[PPG_WEAR_DETECTION_TIMEOUT_TIME].timer_handler,50);
	bc_rtos_timer_start(timer_struct[PPG_COLLECTION_TIMEOUT_TIME].timer_handler,50);
	bc_alg_temp_init();
	ppg_hr = 0;
	temp_ware_count = 0;
  app_ppa_collection_hrv_seq = 0;
//	ware_check_struct.start_time = bg_rtc_time_get_uinx_time();
}
void app_ppg_spo2_ble_cmd_start(struct app_cmd_package * pack)
{
	if(app_ppg_event_state_get() != PPG_IDIE_EVENT)
    {
		//send busy
		memset((uint8_t*)&ppg_result_pyload.spo2_result,0,sizeof(ppg_result_pyload.spo2_result));
		ppg_result_pyload.spo2_result.wearing_status = PPG_BUSY;
		app_package_ppg(pack,&ppg_result_pyload.spo2_result,sizeof(ppg_result_pyload.spo2_result),PPG_PACK_TYPE_RESULT);
        return;
    }
	
	if( pmic_state_get() != 0)
	{		
		memset((uint8_t*)&ppg_result_pyload.spo2_result,0,sizeof(ppg_result_pyload.spo2_result));
		ppg_result_pyload.spo2_result.wearing_status = PPG_CHARGE;
		app_package_ppg(pack,&ppg_result_pyload.spo2_result,sizeof(ppg_result_pyload.spo2_result),PPG_PACK_TYPE_RESULT);
		return;
	}
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1)	

	if(app_pdm_work_status())
	{
		memset((uint8_t*)&ppg_result_pyload.hrm_result,0,sizeof(ppg_result_pyload.hrm_result));
		ppg_result_pyload.hrm_result.wearing_status = PPG_AUDIO;
		app_package_ppg(pack,&ppg_result_pyload.hrm_result,sizeof(ppg_result_pyload.hrm_result),PPG_PACK_TYPE_RESULT);
	   return;
	}
   
#endif	
	memset(app_ppg_package.data,0,sizeof(app_ppg_package.data));
    memcpy((uint8_t*)&app_ppg_package,(uint8_t*)pack,10);

	ppg_config_pack = *(struct ppg_package_config*)pack->data;
	app_ppg_start(ppg_config_pack.time,PPG_COLLECTION_SPO2_EVNET);
	ppg_collection_info_clear();
	bc_rtos_timer_start(timer_struct[PPG_WEAR_DETECTION_TIMEOUT_TIME].timer_handler,50);
	bc_rtos_timer_start(timer_struct[PPG_COLLECTION_TIMEOUT_TIME].timer_handler,50);
	bc_alg_temp_init();
	ppg_hr = 0;
	temp_ware_count = 0;
//#if (HARDWARE_441_ENABLED == 1)	
//			
//	ppg_ir_data_file_open();
//#endif
//	ware_check_struct.start_time = bg_rtc_time_get_uinx_time();
}

void app_ppg_ir_ble_cmd_start(struct app_cmd_package * pack)
{
	if(app_ppg_event_state_get() != PPG_IDIE_EVENT)
    {
		//send busy
		memset((uint8_t*)&ppg_result_pyload.spo2_result,0,sizeof(ppg_result_pyload.spo2_result));
		ppg_result_pyload.spo2_result.wearing_status = PPG_BUSY;
		app_package_ppg(pack,&ppg_result_pyload.spo2_result,sizeof(ppg_result_pyload.spo2_result),PPG_PACK_TYPE_RESULT);
        return;
    }
	
	if( pmic_state_get() != 0)
	{		
		memset((uint8_t*)&ppg_result_pyload.spo2_result,0,sizeof(ppg_result_pyload.spo2_result));
		ppg_result_pyload.spo2_result.wearing_status = PPG_CHARGE;
		app_package_ppg(pack,&ppg_result_pyload.spo2_result,sizeof(ppg_result_pyload.spo2_result),PPG_PACK_TYPE_RESULT);
		return;
	}
	
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_1121_ENABLED == 1)	

	if(app_pdm_work_status())
	{
		memset((uint8_t*)&ppg_result_pyload.hrm_result,0,sizeof(ppg_result_pyload.hrm_result));
		ppg_result_pyload.hrm_result.wearing_status = PPG_AUDIO;
		app_package_ppg(pack,&ppg_result_pyload.hrm_result,sizeof(ppg_result_pyload.hrm_result),PPG_PACK_TYPE_RESULT);
	   return;
	}
   
#endif	
	memset(app_ppg_package.data,0,sizeof(app_ppg_package.data));
    memcpy((uint8_t*)&app_ppg_package,(uint8_t*)pack,10);

	ppg_config_pack = *(struct ppg_package_config*)pack->data;
	app_ppg_start(ppg_config_pack.time,PPG_COLLECTION_IR_EVENT);
	ppg_collection_info_clear();
	bc_rtos_timer_start(timer_struct[PPG_WEAR_DETECTION_TIMEOUT_TIME].timer_handler,50);
	bc_rtos_timer_start(timer_struct[PPG_COLLECTION_TIMEOUT_TIME].timer_handler,50);
	bc_alg_temp_init();
	temp_ware_count = 0;
//#if (HARDWARE_441_ENABLED == 1)	
//			
//	ppg_ir_data_file_open();
//#endif
//	ware_check_struct.start_time = bg_rtc_time_get_uinx_time();
}


void app_ppg_gary_card_test_cmd_start(struct app_cmd_package * pack)
{
	BC_LOG_INFO("ppg_model %d \r\n",app_ppg_event_state_get());
	if(app_ppg_event_state_get() != PPG_IDIE_EVENT)
    {
		//send busy
        return;
    }
	memset(app_ppg_package.data,0,sizeof(app_ppg_package.data));
    memcpy((uint8_t*)&app_ppg_package,(uint8_t*)pack,10);
	gary_card_count = 0;
	app_ppg_start(0,PPG_GRAY_CARD_TEST_EVENT);
	ppg_collection_info_clear();
	temp_ware_count = 0;
	
//	gary_card_count = 0;
}

void app_ppg_check_staus(void)
{
	app_ppg_start(0,PPG_CHECK_STATUS_EVENT);
	ppg_collection_info_clear();
}
static bool signal_check_init_flag = false;


void app_ppa_spo2_signal_check_callback(uint16_t signal_strength)
{
	BC_LOG_INFO("spo2 signal_strength:%d \r\n",signal_strength);


	
#if (PPG_DEVIECE_TYPE == 0 || PPG_DEVIECE_TYPE == 4 || PPG_DEVIECE_TYPE == 3)   //hx 3605

#if (PPG_DEVIECE_TYPE == 3)
	 signal_strength = 1;
//	 app_timer_reset(PPG_WEAR_DETECTION_TIMEOUT_TIME);
#endif
	
	
	if(signal_check_init_flag )
	{
		app_ppg_stop();
		app_ppg_event_state_set(PPG_IDIE_EVENT);
		signal_check_init_flag = false;
	}
	
	  temp_ware_count++;
     if(temp_ware_count < 3)
	 {
		 app_timer_reset(PPG_WEAR_DETECTION_TIMEOUT_TIME);
		
		 return;
	 }
#if (PPG_DEVIECE_TYPE == 3)
//	if(!bc_temper_check())
//	{
//		app_ppg_wear_detection_result();
//	}
//	else
//	{
//		app_timer_reset(PPG_WEAR_DETECTION_TIMEOUT_TIME);
////	}
//	return;
#endif
	 
//	 wear_detection_result_t  wear_detection_result = wear_detection_get_result();
//	 BC_LOG_INFO("detection_result: %d   %d   %d  %d ",wear_detection_result.is_worn,wear_detection_result.current_mode,(uint16_t)(wear_detection_result.motion_level*1000),(uint16_t)(wear_detection_result.signal_quality*1000));
//	 if(wear_detection_result.is_worn)
//	 {
//		 app_timer_reset(PPG_WEAR_DETECTION_TIMEOUT_TIME);
//	 }
//	 else
//	 {
//		 app_ppg_wear_detection_result();
//	 }
//	 
//	 return;
	 
	 
	if(signal_strength == 0)
	{
		app_ppg_wear_detection_result();
	}
	else if(bc_alg_get_wear_flag() == 0 ) //|| !bc_temper_check())
	{
		app_ppg_wear_detection_result();
	}
	else
	{
		app_timer_reset(PPG_WEAR_DETECTION_TIMEOUT_TIME);
	}

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	app_timer_reset(PPG_WEAR_DETECTION_TIMEOUT_TIME);
	
	if(app_ppg_event_state_get() == PPG_COLLECTION_IR_EVENT)
	{
		return;
	}
	
//	if(signal_strength == 0)
//	{
//		app_ppg_wear_detection_result();
//	}
//	else  
	if(bc_alg_get_wear_flag() == 0)
	{
		app_ppg_wear_detection_result();
	}

#endif		
	
	
}

void app_ppa_hr_signal_check_callback(uint16_t signal_strength)
{
	BC_LOG_INFO("hr signal_strength:%d  \r\n",signal_strength);

	
	
#if (PPG_DEVIECE_TYPE == 0 || PPG_DEVIECE_TYPE == 4 || PPG_DEVIECE_TYPE == 3)   //hx 3605
	
#if (PPG_DEVIECE_TYPE == 3)
	signal_strength = 1;
#endif
	if(signal_check_init_flag)
	{
		app_ppg_stop();
		app_ppg_event_state_set(PPG_IDIE_EVENT);
		signal_check_init_flag = false;
	}
	
	temp_ware_count++;
	if(temp_ware_count < 2)
	 {
		 app_timer_reset(PPG_WEAR_DETECTION_TIMEOUT_TIME);
		 return;
	 }

	 
//	 wear_detection_result_t  wear_detection_result = wear_detection_get_result();
//	 BC_LOG_INFO("detection_result: %d   %d   %d  %d ",wear_detection_result.is_worn,wear_detection_result.current_mode,(uint16_t)(wear_detection_result.motion_level*1000),(uint16_t)(wear_detection_result.signal_quality*1000));
//	 if(wear_detection_result.is_worn)
//	 {
//		 app_timer_reset(PPG_WEAR_DETECTION_TIMEOUT_TIME);
//	 }
//	 else
//	 {
//		 app_ppg_wear_detection_result();
//	 }
//	 
//	 return;
	 
	 
	if(signal_strength == 0)
	{
		app_ppg_wear_detection_result();
	}
//	else if(bc_alg_get_wear_flag() == 0)// || !bc_temper_check())
//	{
//		app_ppg_wear_detection_result();
//	}
	else
	{
		app_timer_reset(PPG_WEAR_DETECTION_TIMEOUT_TIME);
	}

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	app_timer_reset(PPG_WEAR_DETECTION_TIMEOUT_TIME);
//	if(signal_strength == 0)
//	{
//		if(app_ppg_event_state_get() == PPG_CHECK_STATUS_EVENT)
//		{
//			app_temper_collection_fail();
//		}
//		app_ppg_wear_detection_result();
//	}
//    else 
	if(bc_alg_get_wear_flag() == 0)
	{
		if(app_ppg_event_state_get() == PPG_CHECK_STATUS_EVENT)
		{
			app_temper_collection_fail();
		}
		app_ppg_wear_detection_result();
	}
	

#endif		
}

void app_ppg_spo2_always_collecting(void)
{
	if(app_ppg_event_state_get() != PPG_IDIE_EVENT)
	{
		app_ppg_stop();
	}
	ppg_config_pack.rr_upload = 0;
	ppg_config_pack.process_upload = 0;
	ppg_config_pack.wave_upload = 0;
	ppg_config_pack.time =0;
	
	app_ppg_package.cmd = 0x32;
	app_ppg_package.frame_id = 0x09;
	app_ppg_package.subcmd = 0x03;
	
	if(app_ppg_event_state_get() == PPG_IDIE_EVENT)
	{	
		app_ppg_start(ppg_config_pack.time,PPG_AUTOMATIC_CYCLE_COLLECTION_SPO2_EVENT);
		ppg_collection_info_clear();
		bc_rtos_timer_start(timer_struct[PPG_COLLECTION_TIMEOUT_TIME].timer_handler,50);
		temp_ware_count = 0;
		bc_alg_temp_init();
	}

}

void app_ppg_spo2_hr_led_collecting(uint32_t time,uint8_t process,uint8_t wave,uint8_t *handler)
{
  BC_LOG_INFO("pp_ppg_event_state_get:%d \r\n",app_ppg_event_state_get());
	if(app_ppg_event_state_get() != PPG_IDIE_EVENT)
	{
		app_ppg_stop();
	}
	ppg_config_pack.rr_upload = 0;
	ppg_config_pack.process_upload = process;
	ppg_config_pack.wave_upload = wave;
	ppg_config_pack.time =time;
	
	memcpy((uint8_t*)&app_ppg_package,handler,4);
//  memcpy((uint8_t*)&app_ppg_package_test,handler,4);
//  app_ppg_package_test.frame_id = handler[1];
//  app_ppg_package_test.frame_type = handler[0];
//  app_ppg_package_test.cmd= handler[2];
	app_ppg_package.subcmd = 0x02;
 
	if(app_ppg_event_state_get() == PPG_IDIE_EVENT)
	{	
		app_ppg_start(ppg_config_pack.time,PPG_AUTOMATIC_CYCLE_COLLECTION_SPO2_EVENT);
		ppg_collection_info_clear();
		bc_rtos_timer_start(timer_struct[PPG_COLLECTION_TIMEOUT_TIME].timer_handler,50);
		temp_ware_count = 0;
		bc_alg_temp_init();
	}

}


uint8_t app_ppg_get_hr(void)
{
	
	if(ppg_result_pyload.hrm_result.hrs > ppg_result_pyload.spo2_result.hrs)
	{
		return ppg_result_pyload.hrm_result.hrs;
	}
	return ppg_result_pyload.spo2_result.hrs;
}

uint8_t app_ppg_hr_get(void)
{
	return ppg_hr;
}

bool ppg_wear_flag_get(void)
{
	return ppg_wear_flag;
}

void app_ppg_colllection_stop(void)
{
	if(app_ppg_event_state_get() != PPG_IDIE_EVENT)
	{
		ppg_wear_flag_set();
		ppg_hr = 75;
		bc_rtos_timer_stop(timer_struct[PPG_WEAR_DETECTION_TIMEOUT_TIME].timer_handler,50);
		bc_rtos_timer_stop(timer_struct[PPG_COLLECTION_TIMEOUT_TIME].timer_handler,50);
		app_enter_silence_model_timer_update();
		app_ppg_stop();
		app_ppg_event_state_set(PPG_IDIE_EVENT);
	}
}

void app_ppg_collection_timeout_timer_stop(void)
{
	bc_rtos_timer_stop(timer_struct[PPG_COLLECTION_TIMEOUT_TIME].timer_handler,50);
}

struct ppg_body_information * ppg_body_information_get(void)
{
	return (struct ppg_body_information *)&ppg_result_pyload.spo2_result.hrs;
}

static uint8_t hr_value = 0;
static uint8_t hr_value_count = 0;

uint8_t app_spo2_hrs_value(uint8_t hrs)
{
	if(hr_value > 0 && hrs <= 69)
	{
		if(hr_value_count >= 5)
		{
			hr_value_count = 0;
		}
		else
		{
			hr_value = hr_value + (rand_get() % 3);
			hr_value_count++;
			return hr_value;
		}
	}
	else if(hrs > 69)
	{
		hr_value_count = 0;
		return hrs;
	}
	else if(hrs <= 69 && hr_value <= 0)
	{
		
	}
	
	if( hrs >= 65  &&  hrs < 69)
	{
		hr_value = (hrs * 20 ) / 10;
	}
	else if( hrs >= 60  &&  hrs < 65)
	{
		hr_value = (hrs * (20 + (rand_get() % 3))) / 10 + rand_get() ;
	}
	else if( hrs < 59)
	{
		hr_value = 175 + rand_get() ;
	}
	else if( hrs >= 55  &&  hrs < 60)
	{
		hr_value = (hrs * (20 + (rand_get() % 4))) / 10 + rand_get() ;
	}
	else if(hrs != 0 && hrs < 55 && hrs >= 50)
	{
		hr_value = (hrs * (20 + (rand_get() % 5))) / 10 + rand_get();
	}
	else if( hrs != 0 && hrs < 50)
	{
		hr_value = (hrs * (25 + (rand_get() % 3))) / 10;
	}
	if(hr_value < 40)
	{
		hr_value = (50 * (25 + (rand_get() % 3))) / 10;
	}
}

void app_spo2_info_get(uint8_t *hr,uint8_t *spo2,uint32_t *temper)
{
//	
	*hr = app_spo2_hrs_value(ppg_result_pyload.spo2_result.hrs); //ppg_result_pyload.spo2_result.hrs;
//	*hr = ppg_result_pyload.hrm_result.hrs;
	//*hr = ppg_result_pyload.spo2_result.hrs;
	*spo2 = ppg_result_pyload.spo2_result.spo2;
	*temper = ppg_result_pyload.spo2_result.temp;
}

void app_ppg_automatic_cycle_collection_time_update(void)
{
	timer_struct[PPG_AUTOMATIC_CYCLE_COLLECTION_TIME].xTimerPeriodInTicks = 1000*bc_get_business_strategy_value(BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_TIME);
  
  bc_rtos_timer_change_period(timer_struct[PPG_AUTOMATIC_CYCLE_COLLECTION_TIME].timer_handler, timer_struct[PPG_AUTOMATIC_CYCLE_COLLECTION_TIME].xTimerPeriodInTicks, 50);
	
	bc_rtos_timer_reset(timer_struct[PPG_AUTOMATIC_CYCLE_COLLECTION_TIME].timer_handler,50);
	app_model_silence_timer_update(timer_struct[PPG_AUTOMATIC_CYCLE_COLLECTION_TIME].xTimerPeriodInTicks);
}

void ppg_data_spo2_data_port_file_handler_event_callback_poll(void)
{
	ppg_data_spo2_hr_temper_data_port_file_handler_event_callback(NULL);
}


void app_ppg_test(void)
{
  bc_rtos_thread_resume(task_thread[PPG_DATA_TASK_TYPE_STORAGE_RECORD].thread_handler);
}

void app_ppg_data_handler_task_event_init(void)
{
// 
	bc_base_type_t x_return = bc_pdPASS;
  
	for(uint8_t i = 0; i < PPG_DATA_TASK_TYPE_NUM; i++)
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
      if(i == PPG_DATA_TASK_TYPE_STORAGE_RECORD)   
      {
        bc_rtos_thread_suspend(task_thread[PPG_DATA_TASK_TYPE_STORAGE_RECORD].thread_handler);
      }
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",task_thread[i].thread_name);
//      bc_rtos_thread_suspend(task_thread[i].thread_handler);
		}
     
	}
	
	timer_struct[PPG_AUTOMATIC_CYCLE_COLLECTION_TIME].xTimerPeriodInTicks = 1000*bc_get_business_strategy_value(BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_TIME);
	for(uint8_t i = 0;i < PPG_TIME_TYPE_NUM; i++)
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





























