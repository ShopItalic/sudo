#include "app_g_sensor_handler.h"

#include "app_model_handler.h"

#include "bc_rtos.h"
#include "bc_gsensor.h"
#include "bc_logger.h"
#include "bc_device_info.h"
#include "bc_ppg.h"

#include <string.h>

enum app_g_sensor_task
{
	G_SENSOR_TASK_TYPE_STORAGE = 0,
	G_SENSOR_TASK_TYPE_NUM
};

#define APP_G_SENSOR_DATA_BUFF_MAX   25*2

static uint16_t sport_step = 0;
static uint16_t sport_step_count = 0;

static uint32_t temp_count = 0;

struct app_g_sensor_data
{
	uint16_t acc[3];
};

struct app_g_sensor_data_struct
{
	struct app_g_sensor_data g_sensor_data[APP_G_SENSOR_DATA_BUFF_MAX];
	uint16_t g_sensor_data_index;
};

#if (HARDWARE_1141_ENABLED == 1 )			

struct app_g_sensor_gyro_data
{
	uint16_t gyro[3];
};

struct app_g_sensor_gyro_data_struct
{
	struct app_g_sensor_gyro_data g_sensor_gyro_data[APP_G_SENSOR_DATA_BUFF_MAX];
	uint16_t g_sensor_gyro_data_index;
};

struct app_g_sensor_gyro_data_struct g_sensor_gyro_data_struct = {0};

#endif	

struct app_g_sensor_data_struct g_sensor_data_struct = {0};

static void app_g_sensor_storage_handler_thread(void *thread_handler);



static bc_rtos_thread_struct task_thread[G_SENSOR_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "app g_sensor handler task",
                                                                      .thread_stack_depth   = APP_TASK_G_SENSOR_STACK_SIZE,
                                                                      .thread_priority      = APP_TASK_G_SENSOR_PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = app_g_sensor_storage_handler_thread,
                                                                    },																	
                                                                  };

static void app_gsensor_sample_timer_callback(void * pvParameter);

static bc_rtos_timer_struct  timer_struct[G_SENSOR_TIMER_TYPE_NUM] = {
    {
        .timer_name = "g_sensor data get timer",
        .uxAutoReload = true,
#if (HARDWARE_451_ENABLED == 1 )	
         .xTimerPeriodInTicks = 38,
		
#elif (HARDWARE_1141_ENABLED == 1 )			
//		.xTimerPeriodInTicks = 9,
      .xTimerPeriodInTicks = 40,
#else
		 .xTimerPeriodInTicks = 40,

#endif	
        .lock = false,
        .timer_callback_function = app_gsensor_sample_timer_callback,
    },
};

static uint16_t app_g_sensor_sport_step_count_init(void)
{
	sport_step = bc_device_info_get_sport_count();
	sport_step_count = sport_step;
    return 0;
}

static void app_g_sensor_int_irq_callback(void)
{
//	app_enter_silence_model_timer_update();
}






static void app_gsensor_sample_timer_callback(void * pvParameter)
{
     if(g_sensor_data_struct.g_sensor_data_index >= APP_G_SENSOR_DATA_BUFF_MAX)
	 {
		 return;
	 }
				
#if (G_SENSOR_DEVIECE_TYPE == 0 || G_SENSOR_DEVIECE_TYPE == 3)   //QMA6100/QMA6100P
	int32_t temp[3] = {0};
	bc_gsensor_dataRead(temp);
	g_sensor_data_struct.g_sensor_data[g_sensor_data_struct.g_sensor_data_index].acc[0] = (int16_t)temp[0];
	g_sensor_data_struct.g_sensor_data[g_sensor_data_struct.g_sensor_data_index].acc[1] = (int16_t)temp[1];
	g_sensor_data_struct.g_sensor_data[g_sensor_data_struct.g_sensor_data_index].acc[2] = (int16_t)temp[2];

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688
//	bc_gsensor_dataRead((int*)&g_sensor_data_struct.g_sensor_data[g_sensor_data_struct.g_sensor_data_index]);
//	g_sensor_data_struct.g_sensor_data[g_sensor_data_struct.g_sensor_data_index].acc[0] *= 2;
//	g_sensor_data_struct.g_sensor_data[g_sensor_data_struct.g_sensor_data_index].acc[1] *= 2;
//	g_sensor_data_struct.g_sensor_data[g_sensor_data_struct.g_sensor_data_index].acc[2] *= 2;
	 
#if (HARDWARE_1141_ENABLED == 1 )
	 if(g_sensor_gyro_data_struct.g_sensor_gyro_data_index < APP_G_SENSOR_DATA_BUFF_MAX)
	 {
		 bc_gsensor_RawData_dataRead(g_sensor_data_struct.g_sensor_data[g_sensor_data_struct.g_sensor_data_index].acc,g_sensor_gyro_data_struct.g_sensor_gyro_data[g_sensor_gyro_data_struct.g_sensor_gyro_data_index].gyro);
		 g_sensor_data_struct.g_sensor_data[g_sensor_data_struct.g_sensor_data_index].acc[0] *= 2;
		 g_sensor_data_struct.g_sensor_data[g_sensor_data_struct.g_sensor_data_index].acc[1] *= 2;
		 g_sensor_data_struct.g_sensor_data[g_sensor_data_struct.g_sensor_data_index].acc[2] *= 2;
		 g_sensor_gyro_data_struct.g_sensor_gyro_data_index++;	
	 }
	 else
	 {
		 g_sensor_gyro_data_struct.g_sensor_gyro_data_index = 0;	
	 }
//	 if(g_sensor_gyro_data_struct.g_sensor_gyro_data_index < APP_G_SENSOR_DATA_BUFF_MAX)
//	 {
//		bc_gsensor_Gyroscope_dataRead((int*)g_sensor_gyro_data_struct.g_sensor_gyro_data[g_sensor_gyro_data_struct.g_sensor_gyro_data_index].gyro);
//		 g_sensor_gyro_data_struct.g_sensor_gyro_data_index++;	
//	 }
	
#endif	 

#endif
	 
	 g_sensor_data_struct.g_sensor_data_index++;    
	
}

static void app_gsensor_sample_timer_start(void)
{
#if (HARDWARE_451_ENABLED == 1 )	
     bc_gsensor_init();
	
#elif (HARDWARE_1141_ENABLED == 1 )	
//     bc_g_sensor_acc_and_gyro_config(100);
//  bc_g_sensor_acc_and_gyro_config(25);

#endif	
//	memset((uint8_t*)&g_sensor_data_struct,0,sizeof(g_sensor_data_struct));
//	timer_struct[G_SENSOR_DATA_GET].lock = true;
//	bc_rtos_timer_start(timer_struct[G_SENSOR_DATA_GET].timer_handler,50);
}

static void app_gsensor_sample_timer_stop(void)
{
	if(timer_struct[G_SENSOR_DATA_GET].lock)
	{
		timer_struct[G_SENSOR_DATA_GET].lock = false;
		bc_rtos_timer_stop(timer_struct[G_SENSOR_DATA_GET].timer_handler,50);
#if (HARDWARE_451_ENABLED == 1 )	
     bc_gsensor_init();
	
#elif (HARDWARE_1141_ENABLED == 1 )	
    bc_g_sensor_acc_and_gyro_config(25);

#endif	
	}
}

static bool app_gsensor_data_read_callback(uint8_t *data_buff,uint16_t *data_count)
{
	if(data_buff == NULL || g_sensor_data_struct.g_sensor_data_index == 0)
	{
		*data_count = 0;
		return false;
	}

	memcpy(data_buff,(uint8_t*)g_sensor_data_struct.g_sensor_data,(6 * g_sensor_data_struct.g_sensor_data_index));
	*data_count = g_sensor_data_struct.g_sensor_data_index;
	
	memset((uint8_t*)&g_sensor_data_struct,0,sizeof(g_sensor_data_struct));
	
	return true;
}

#if (HARDWARE_1141_ENABLED == 1 )			

bool app_gsensor_gyro_data_read_callback(uint8_t *data_buff,uint16_t *data_count)
{
	if(data_buff == NULL || g_sensor_gyro_data_struct.g_sensor_gyro_data_index == 0)
	{
		*data_count = 0;
		return false;
	}

	memcpy(data_buff,(uint8_t*)g_sensor_gyro_data_struct.g_sensor_gyro_data,(6 * g_sensor_gyro_data_struct.g_sensor_gyro_data_index));
	*data_count = g_sensor_gyro_data_struct.g_sensor_gyro_data_index;
	
	memset((uint8_t*)&g_sensor_gyro_data_struct,0,sizeof(g_sensor_gyro_data_struct));
	
	return true;
}

#endif

static void app_g_sensor_storage_handler_thread(void *thread_handler)
{
  bc_gsensor_init();
	app_g_sensor_sport_step_count_init();
	bc_g_sensor_int_irq_register_callback(app_g_sensor_int_irq_callback);
#if(PPG_ENABLED)   
	bc_ppg_g_sensor_callback_regdister(app_gsensor_sample_timer_start,app_gsensor_sample_timer_stop,app_gsensor_data_read_callback);
#endif  
	while(true)
	{
#if ( HARDWARE_451_ENABLED == 1)	

    temp_count++;
    if(temp_count >= 60)
    {
      uint16_t step_count = bc_gsensor_getStep();
      bc_device_info_set_sport_count(sport_step);
      sport_step+=step_count;
      bc_gsensor_clearSteps();
      temp_count = 0;
    }
#else
    bc_device_info_set_sport_count(sport_step);
#endif	
		bc_rtos_thread_suspend(task_thread[APP_G_SENSOR_SPORT_STORAGE_TASK_EVENT].thread_handler);

	}
}


uint16_t app_g_sensor_sport_step_count_get(void)
{
#if ( HARDWARE_451_ENABLED == 1)	

	uint16_t step_count = bc_gsensor_getStep();
//	bc_gsensor_clearSteps();
	bc_rtos_thread_resume(task_thread[APP_G_SENSOR_SPORT_STORAGE_TASK_EVENT].thread_handler);
//	sport_step+=step_count;
	return sport_step + step_count;
#else

	uint16_t step_count = bc_gsensor_getStep();
//	bc_gsensor_clearSteps();
	bc_rtos_thread_resume(task_thread[APP_G_SENSOR_SPORT_STORAGE_TASK_EVENT].thread_handler);
	sport_step = sport_step_count + step_count;
	return sport_step_count + step_count;
#endif	  
return 0;
}

void app_g_sensor_sport_step_count_clear(void)
{
	sport_step = 0;
	sport_step_count = 0;
	bc_device_info_set_sport_count(sport_step);
    
	bc_gsensor_clearSteps();
}




 void app_g_sensor_time_create(void)
{
	
	bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < APP_G_SENSOR_EVENT_NUM; i++)
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
	
	for(uint8_t i = 0;i < G_SENSOR_TIMER_TYPE_NUM; i++)
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
	
//	bc_gsensor_init();
//	app_g_sensor_sport_step_count_init();
//	bc_g_sensor_int_irq_register_callback(app_g_sensor_int_irq_callback);
//	bc_ppg_g_sensor_callback_regdister(app_gsensor_sample_timer_start,app_gsensor_sample_timer_stop,app_gsensor_data_read_callback);
}
















