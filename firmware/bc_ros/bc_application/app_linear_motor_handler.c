#if defined(SUDO_VOICE_ONLY)
#include "app_error.h"
#endif
#include "app_linear_motor_handler.h"

#include "bc_logger.h"
#include "bc_delay.h"
#include "bc_linear_motor.h"
#include "bc_device_info.h"
#include "q_device.h"
#include "bc_ldo_switch.h"

#include "app_touch_button_handler.h"

#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4 ))
#include "bc_linear_motor_ic.h"
#endif


enum app_linear_motor_type
{
	LINEAR_MOTOR_TYPE_IDIE = 0,
	LINEAR_MOTOR_TYPE_STRONG_VIBRATION,
	LINEAR_MOTOR_TYPE_CONTINUOUS_VIBRATION,
	LINEAR_MOTOR_TYPE_GRADUAL_VIBRATION,
};

enum app_linear_motor_timer_type
{
	LINEAR_MOTOR_TIMEOUT_TIMER = 0,
	LINEAR_MOTOR_TIMER_TYPE_NUM
};

static enum app_linear_motor_type linear_motor_type = LINEAR_MOTOR_TYPE_IDIE;

static void app_linear_motor_timer_callback(void * pvParameter);


static bc_rtos_timer_struct  timer_struct[LINEAR_MOTOR_TIMER_TYPE_NUM] = {
	{
		.timer_name = "motor idie timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 3500,
		.lock = false,
		.timer_callback_function = app_linear_motor_timer_callback,
	}
};

enum app_linear_motor_task_event
{
	LINEAR_MOTOR_TASK_TYPE_START = 0,
	LINEAR_MOTOR_TASK_TYPE_NUM
};


static void app_linear_motor_handler_thread(void *thread_handler);

static bc_rtos_thread_struct thread_struct[LINEAR_MOTOR_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "motor start task",
                                                                      .thread_stack_depth   = APP_LINEAR_MOTOR_STACK_SIZE ,
                                                                      .thread_priority      = APP_LINEAR_MOTOR_PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = app_linear_motor_handler_thread,
                                                                    },																	
                                                                  };

static void app_linear_motor_type_set(enum app_linear_motor_type motor_type)
{
	linear_motor_type = motor_type;
}

static enum app_linear_motor_type app_linear_motor_type_get(void)
{
	return linear_motor_type;
}

/*******************************************************************************
 * 震动模式接口 - 前向声明与变量定义
 *******************************************************************************/

/* 震动模式参数配置 */
typedef struct
{
    uint16_t repeats;        /* 单次震动时长（重复次数） */
    uint16_t interval_ms;    /* 多次震动间隔（毫秒） */
    uint16_t pwm_value;      /* PWM幅值 */
} vibrate_param_t;

static const vibrate_param_t s_vibrate_params[VIBRATE_MODE_MAX] =
{
    /* VIBRATE_MODE_VERY_SHORT  极短振 */
    {
        .repeats = 5,
        .interval_ms = 200,
        .pwm_value = 3200,
    },
    /* VIBRATE_MODE_SHORT  短振（默认） */
    {
        .repeats = 13,
        .interval_ms = 300,
        .pwm_value = 3200,
    },
    /* VIBRATE_MODE_LONG  长振 */
    {
        .repeats = 40,
        .interval_ms = 500,
        .pwm_value = 3200,
    },
};

#if !(defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4 ))
/* PWM驱动版本：全局变量 */
static uint16_t s_vibrate_pwm_seq[4] = {0};
static struct pwm_config s_vibrate_pwm_config = {0};
static vibrate_mode_t s_vibrate_mode = VIBRATE_MODE_MAX;
#endif


static void app_linear_motor_timer_callback(void * pvParameter)
{
	bc_device_hid_info *hid_info = NULL;
	hid_info = bc_device_info_get_hid_info();
	if(hid_info->device_hid_touch_mode == 0xFF)
	{
#if defined(BLE_MULTI_MASTER)

#else
					

#endif // defined(BLE_MULTI_MASTER)	
	}
  bc_rtos_thread_resume(thread_struct[LINEAR_MOTOR_TASK_TYPE_START].thread_handler);
	bc_rtos_timer_stop(timer_struct[LINEAR_MOTOR_TIMEOUT_TIMER].timer_handler,100);
   	
//	BC_LOG_INFO("");
}


static void app_linear_motor_handler_thread(void *thread_handler)
{
  while(true)
  {
    BC_LOG_INFO("app_linear_motor_type_get():%d \r\n",app_linear_motor_type_get());
    switch(app_linear_motor_type_get())
    {
      case LINEAR_MOTOR_TYPE_STRONG_VIBRATION:
      {
        BC_LOG_INFO("LINEAR_MOTOR_TYPE_STRONG_VIBRATION \r\n");
        bc_linear_motor_strong_vibration_start();
        break;
      }
      case LINEAR_MOTOR_TYPE_CONTINUOUS_VIBRATION:
      {
        BC_LOG_INFO("LINEAR_MOTOR_TYPE_CONTINUOUS_VIBRATION \r\n");
        bc_linear_motor_continuous_vibration_start();
        break;
      }
      case LINEAR_MOTOR_TYPE_GRADUAL_VIBRATION:
      {
        break;
      }
      default:
      {
        break;
      }
    }
    bc_rtos_thread_suspend(thread_struct[LINEAR_MOTOR_TASK_TYPE_START].thread_handler);
  }
}

static void app_linear_motor_idie_callback(void)
{
	app_linear_motor_type_set(LINEAR_MOTOR_TYPE_IDIE);
}

void app_linear_motor_set(uint32_t time,uint8_t type)
{
	timer_struct[LINEAR_MOTOR_TIMEOUT_TIMER].xTimerPeriodInTicks = 1000*time;
	app_linear_motor_type_set((enum app_linear_motor_type)type);
	bc_rtos_timer_start(timer_struct[LINEAR_MOTOR_TIMEOUT_TIMER].timer_handler,100);
}

void app_linear_motor_start(uint8_t type)
{  
  BC_LOG_INFO("linear_motor_start:%d \r\n",type);
	app_linear_motor_type_set((enum app_linear_motor_type)type);
	bc_rtos_thread_resume(thread_struct[LINEAR_MOTOR_TASK_TYPE_START].thread_handler);
}

void app_linear_motor_stop(void)
{
	if(app_linear_motor_type_get() != LINEAR_MOTOR_TYPE_IDIE)
	{
		bc_linear_motor_stop();
		bc_device_hid_info *hid_info = NULL;
		hid_info = bc_device_info_get_hid_info();
		if(hid_info->device_hid_touch_mode == 0xFF)
		{
			
		}
	}
}

/*******************************************************************************
 * Function Name     : app_linear_motor_time_create
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
void app_linear_motor_time_create(void)
{
  
  for(uint8_t i = 0;i <  LINEAR_MOTOR_TIMER_TYPE_NUM; i++)
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
	
	bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < LINEAR_MOTOR_TASK_TYPE_NUM; i++)
	{
		x_return  = bc_rtos_thread_create((TaskFunction_t )thread_struct[i].thread_task_code,     	
                                     (const char*    )thread_struct[i].thread_name,   	
                                     (uint16_t       )thread_struct[i].thread_stack_depth, 
                                     (void*          )&thread_struct[i].thread_parameters,				
                                     (UBaseType_t    )thread_struct[i].thread_priority,	
                                     (TaskHandle_t*  )&thread_struct[i].thread_handler); 
		if(x_return == bc_pdPASS)
		{
			BC_LOG_INFO("create %s succeed \r\n",thread_struct[i].thread_name);
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",thread_struct[i].thread_name);
#if defined(SUDO_VOICE_ONLY)
            APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
#endif
		}	
	}
	bc_linear_motor_pwm_idie_register_callback(app_linear_motor_idie_callback);
}

/*******************************************************************************
 * 震动模式接口函数实现
 *******************************************************************************/

#if defined(HANDWARE_1_23_3)
/* IC驱动版本：使用 app_linear_motor_ic_start_config */
uint8_t app_vibrate_start(vibrate_mode_t mode, uint8_t count)
{
    bc_device_linear_motor_info config;
    
    if(mode >= VIBRATE_MODE_MAX)
    {
        BC_LOG_ERROR("vibrate start: invalid mode %d\r\n", mode);
        return 1;
    }
    
    const vibrate_param_t *param = &s_vibrate_params[mode];
    
    /* 转换为IC驱动参数
       duration_ms: 单次震动时长（约 repeats * 0.1ms，粗略估算）
       interval_ms: 震动间隔
       vib_count: 震动次数
    */
    config.duration_ms = param->repeats * 2;   /* 粗略转换 */
    config.vib_count = count;
    config.vib_gain = 0x90;
    config.interval_ms = param->interval_ms;
    
    BC_LOG_INFO("vibrate start: mode=%d, count=%d\r\n", mode, count);
    
    return app_linear_motor_ic_start_config(&config);
}

void app_vibrate_stop(void)
{
    app_linear_motor_ic_stop();
}

#else
/* PWM驱动版本：函数实现
   仿照 bc_linear_motor_start(LINEAR_MOTOR_MIC_START) 的流程：
   1. 开启马达电源 (bc_ldo_motor_power_on)
   2. 延迟20ms
   3. 配置PWM参数并启动
   震动次数通过 playback_count 直接控制（与LINEAR_MOTOR_MIC_STOP方式一致）
*/
#ifndef HANDWARE_1_23_4
uint8_t app_vibrate_start(vibrate_mode_t mode, uint8_t count)
{
    if(mode >= VIBRATE_MODE_MAX)
    {
        BC_LOG_ERROR("vibrate start: invalid mode %d\r\n", mode);
        return 1;
    }
    
    const vibrate_param_t *param = &s_vibrate_params[mode];
    
    BC_LOG_INFO("vibrate start: mode=%d, count=%d\r\n", mode, count);
    
    s_vibrate_mode = mode;
    
#if defined(SUDO_VOICE_ONLY)
    /* Invalidate and close any prior PWM before taking the new power lease. */
    bc_linear_motor_stop();
    /* Publish the legacy pre-LDO and settle interval to the battery ADC. */
    bc_linear_motor_activity_begin();
#endif

    /* 开启马达电源（与bc_linear_motor_start流程一致） */
    bc_ldo_motor_power_on();
    bc_delay_ms(20);
    
    /* PWM序列：前两个周期振动，第三个归零（与LINEAR_MOTOR_MIC_START一致） */
    s_vibrate_pwm_seq[0] = param->pwm_value;
    s_vibrate_pwm_seq[1] = param->pwm_value;
    s_vibrate_pwm_seq[2] = 10000;  /* top_value，归零 */
    
    s_vibrate_pwm_config.pwm_aisle0_enable_status = true;
    s_vibrate_pwm_config.pwm_aisle1_enable_status = false;
    s_vibrate_pwm_config.pwm_aisle2_enable_status = false;
    s_vibrate_pwm_config.pwm_aisle3_enable_status = false;
    s_vibrate_pwm_config.pwm_parameter_config.length = 3;
    s_vibrate_pwm_config.pwm_parameter_config.p_common = s_vibrate_pwm_seq;
    s_vibrate_pwm_config.pwm_parameter_config.top_value = 10000;
    s_vibrate_pwm_config.pwm_parameter_config.repeats = param->repeats;
    
    if(count == 0)
    {
        /* count=0: 无限循环模式 */
        s_vibrate_pwm_config.pwm_parameter_config.playback_count = 1;
        s_vibrate_pwm_config.pwm_parameter_config.flags = 2;  /* PWM_FLAG_LOOP */
    }
    else
    {
        /* count>0: 震动count次，通过playback_count控制 */
        s_vibrate_pwm_config.pwm_parameter_config.playback_count = count;
#if defined(SUDO_VOICE_ONLY)
        s_vibrate_pwm_config.pwm_parameter_config.flags = PWM_FLAG_STOP;
#else
        s_vibrate_pwm_config.pwm_parameter_config.flags = 0;  /* PWM_FLAG_STOP */
#endif
    }
    
    bc_linear_motor_pwm_out(&s_vibrate_pwm_config);
    
    return 0;
}

void app_vibrate_stop(void)
{
    s_vibrate_mode = VIBRATE_MODE_MAX;
    bc_linear_motor_stop();
}
#endif
#endif /* PWM驱动版本 */








