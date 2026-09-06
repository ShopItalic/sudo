#include "bc_linear_motor_ic.h"

#include "bc_linear_motor_ic_port.h"
#include "bc_fuel_gauge.h"
#include "haptic_nv.h"
#include "bc_delay.h"
#include "bc_logger.h"
#include "bc_queue.h"
#include "app_cmd_handler.h"
#include "app_package.h"
#include "bc_temp.h"
#include "app_pmic_handler.h"
#include "bc_device_info.h"

static uint8_t ifalg = 0;
static uint8_t lmisok = 0;
static uint8_t batisok = 0;
uint8_t iic1_busy = 0;
STR_IIC1_DATA diic1;

/* 马达震动配置（当前生效的配置） */
static bc_device_linear_motor_info s_motor_config = {
    .duration_ms = 5,      /* 默认单次震动时长 5ms */
    .vib_count   = 1,      /* 默认震动 1 次 */
    .vib_gain    = 0x90,   /* 默认增益 */
    .interval_ms = 700,    /* 默认间隔 700ms */
};

/* 多次震动计数器 */
static uint8_t s_vib_remain_count = 0;
/* 震动运行标志 */
static uint8_t s_motor_running = 0;

enum APP_MOTOR_EVENT
{
  MOTOR_INIT_EVENT   = (0x00000001 << 0),
  MOTOR_UNINIT_EVENT = (0x00000001 << 1),
  MOTOR_START_EVENT = (0x00000001 << 2),
  MOTOR_STOP_EVENT = (0x00000001 << 3),
  MOTOR_STATUS_EVENT = (0x00000001 << 4),
//  TOUCH_SWIPE_LEFT_EVENT = (0x00000001 << 5),
//  TOUCH_SWIPE_RIGHT_EVENT = (0x00000001 << 6),
//  TOUCH_SWIPE_UP_EVENT = (0x00000001 << 7),
//  TOUCH_SWIPE_DOWN_EVENT = (0x00000001 << 8),
//  TOUCH_HOLD_EVENT = (0x00000001 << 9),
//  TOUCH_HOLD_START_EVENT = (0x00000001 << 11),
//  TOUCH_HOLD_STOP_EVENT = (0x00000001 << 12),
//  TOUCH_IRQ_EVENT = (0x00000001 << 13),
};

static bc_rtos_event_struct event_struct = {

  .event_name              = "motor event",
  .event_clear_on_exit     = bc_pdTRUE,
  .event_wait_for_all_bits = bc_pdFALSE,
};



enum app_test_task
{
	MOTOR_TASK_TYPE_INIT = 0,
	MOTOR_TASK_TYPE_NUM
};

enum bc_linear_motor_ic_timer_type
{
	BC_LINEAR_MOTOR_IC_MODE_TIMER = 0,
	BC_LINEAR_MOTOR_IC_MODE_TIMER_TYPE_NUM
};

extern void timer_mode_callback(void *htim);

static bc_rtos_timer_struct  timer_struct[BC_LINEAR_MOTOR_IC_MODE_TIMER_TYPE_NUM] = {
	{
		.timer_name = "bc motor mode timer",
		.uxAutoReload = false,
		.xTimerPeriodInTicks = 700,
		.lock = false,
		.timer_callback_function = timer_mode_callback,
	}
};


static void app_linear_motor_handler_thread(void *thread_handler);
uint8_t app_linear_motor_ic_start_config(bc_device_linear_motor_info *config);

static bc_rtos_thread_struct app_linear_motor_thread[MOTOR_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "linear motor handler",
                                                                      .thread_stack_depth   = 512 ,
                                                                      .thread_priority      = 11,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = app_linear_motor_handler_thread,
                                                                    },																	
                                                                  };

uint8_t app_linear_motor_ic_start_test(void)
{
    return app_linear_motor_ic_start_config(&s_motor_config);
}

uint8_t app_linear_motor_ic_start_timer_vib(uint32_t delay, uint16_t type_vib)
{
    if(type_vib) {
        bc_device_linear_motor_info config = s_motor_config;
        config.duration_ms = delay*10;
        return app_linear_motor_ic_start_config(&config);
    }
    return 0;
}

uint8_t app_linear_motor_ic_start(uint8_t count)
{
    /* count: 震动次数
       count = 0: 无限循环（需手动调用 stop 停止）
       count > 0: 震动 count 次
    */
    bc_device_linear_motor_info config = s_motor_config;
    config.vib_count = count;
    return app_linear_motor_ic_start_config(&config);
}

uint8_t app_linear_motor_ic_start_config(bc_device_linear_motor_info *config)
{
    STR_IIC1_DATA idc1 = {0};

    if (config == NULL) {
        return 1;
    }

    /* 参数校验 */
    if (config->duration_ms == 0) {
        config->duration_ms = 5;
    }
    if (config->vib_gain == 0) {
        config->vib_gain = 0x80;
    }

    /* 保存配置到队列数据中（通过 data 数组传递配置）
       data[0..1] = duration_ms (小端)
       data[2]    = vib_count
       data[3]    = vib_gain
       data[4..5] = interval_ms (小端)
    */
    idc1.cmd = CMD_MOTOR_START;
    idc1.subcmd = 1;  /* subcmd=1 表示带配置参数的启动 */
    idc1.data[0] = (uint8_t)(config->duration_ms & 0xFF);
    idc1.data[1] = (uint8_t)((config->duration_ms >> 8) & 0xFF);
    idc1.data[2] = config->vib_count;
    idc1.data[3] = config->vib_gain;
    idc1.data[4] = (uint8_t)(config->interval_ms & 0xFF);
    idc1.data[5] = (uint8_t)((config->interval_ms >> 8) & 0xFF);

    return bc_queue_enqueue(BC_QUEUE_TYPE_IIC1_HANDLE_DATA, &idc1);
}

void app_linear_motor_ic_stop(void)
{
    /* 停止循环定时器 */
    bc_rtos_timer_stop(timer_struct[BC_LINEAR_MOTOR_IC_MODE_TIMER].timer_handler, 50);
    /* 清零剩余计数和运行标志 */
    s_vib_remain_count = 0;
    s_motor_running = 0;

    /* 调用驱动停止 */
    if (lmisok) {
        bc_linear_motor_i2c_open();
        bc_delay_ms(5);
        g_func_haptic_nv->play_stop();
        bc_linear_motor_i2c_close();
    }
}

uint8_t app_linear_motor_get_id(void *params)
{    
    STR_IIC1_DATA idc1 ={0};
    //bc_rtos_event_group_set_bits(event_struct.event_handler,MOTOR_START_EVENT );
    idc1.cmd = CMD_MOTOR_GET_ID;
    memcpy(idc1.data, params, sizeof(idc1.data));
    return bc_queue_enqueue(BC_QUEUE_TYPE_IIC1_HANDLE_DATA,&idc1);
}

uint8_t app_cw221x_get_id(void *params)
{    
    STR_IIC1_DATA idc1 ={0};
    //bc_rtos_event_group_set_bits(event_struct.event_handler,MOTOR_START_EVENT );
    idc1.cmd = CMD_CW221X_GET_ID;
    memcpy(idc1.data, params, sizeof(idc1.data));
    return bc_queue_enqueue(BC_QUEUE_TYPE_IIC1_HANDLE_DATA,&idc1);
}

uint8_t app_cw221x_get_cap(void *params)
{    
    STR_IIC1_DATA idc1 ={0};
    //bc_rtos_event_group_set_bits(event_struct.event_handler,MOTOR_START_EVENT );
    idc1.cmd = CMD_CW221X_GET_CAP;
    if(params) {
        idc1.subcmd = 0;
        memcpy(idc1.data, params, sizeof(idc1.data));
    } else
        idc1.subcmd = 1;
    return bc_queue_enqueue(BC_QUEUE_TYPE_IIC1_HANDLE_DATA,&idc1);
}

uint8_t bc_temper_get_id(void * params)
{
    STR_IIC1_DATA idc1 ={0};
    //bc_rtos_event_group_set_bits(event_struct.event_handler,MOTOR_START_EVENT );
    idc1.cmd = CMD_TX1812_GET_TEMP_ID;
    memcpy(idc1.data, params, sizeof(idc1.data));
    return bc_queue_enqueue(BC_QUEUE_TYPE_IIC1_HANDLE_DATA,&idc1);
}

void timer_mode_callback(void *params)
{
    BC_LOG_INFO("timer_mode_callback remain:%d\r\n", s_vib_remain_count);

    /* 无限循环模式（vib_count=0）或还有剩余次数 */
    if (s_vib_remain_count > 0 || s_motor_config.vib_count == 0) {
        if (s_vib_remain_count > 0) {
            s_vib_remain_count--;
        }

        /* 继续下一次震动，使用当前配置 */
        STR_IIC1_DATA idc1 = {0};
        idc1.cmd = CMD_MOTOR_START;
        idc1.subcmd = 2;  /* subcmd=2 表示定时器触发的续震（使用已保存的配置） */
        idc1.data[0] = (uint8_t)(s_motor_config.duration_ms & 0xFF);
        idc1.data[1] = (uint8_t)((s_motor_config.duration_ms >> 8) & 0xFF);
        idc1.data[2] = s_vib_remain_count;
        idc1.data[3] = s_motor_config.vib_gain;
        idc1.data[4] = (uint8_t)(s_motor_config.interval_ms & 0xFF);
        idc1.data[5] = (uint8_t)((s_motor_config.interval_ms >> 8) & 0xFF);
        bc_queue_enqueue(BC_QUEUE_TYPE_IIC1_HANDLE_DATA, &idc1);
    } else {
        /* 震动次数已完成，停止 */
        s_motor_running = 0;
        BC_LOG_INFO("motor vib done\r\n");
    }
}

void bc_linear_motor_config(uint16_t pwm_seq_values,uint8_t playback_count,uint16_t repeats)
{
    if(pwm_seq_values > 0xff)
        pwm_seq_values = 0xff;
    // 参数范围限制
    if(pwm_seq_values > 0x90)
        pwm_seq_values = 0x90;
    if(repeats > 20)
        repeats = 20;

    s_motor_config.vib_gain = pwm_seq_values;
    s_motor_config.vib_count = playback_count;
    s_motor_config.duration_ms = repeats;
    s_motor_config.interval_ms = s_motor_config.duration_ms + 300;

#if defined(HANDWARE_1_23_3)
    // 保存马达配置到Flash，上电可恢复
    bc_device_info_linear_motor_config_set(s_motor_config.duration_ms,
                                           s_motor_config.vib_count,
                                           s_motor_config.vib_gain,
                                           s_motor_config.interval_ms);
#endif
}


static void bc_motor_init(void)
{
    uint8_t ret = 0;
    //BC_LOG_INFO("app_linear_motor_handler_thread start \r\n");
    
    bc_linear_motor_i2c_open();
    bc_delay_ms(20);	
    ret = haptic_nv_boot_init();
    if(ret) {
      BC_LOG_INFO("haptic_nv_boot_init return 1 fail\r\n");
        lmisok = 0;
    } else {
        BC_LOG_INFO("haptic_nv_boot_init return 0 succes\r\n");
        lmisok = 1;
    }
    bc_linear_motor_i2c_close();

#if defined(HANDWARE_1_23_3)
    // 从Flash恢复用户配置的马达参数
    uint16_t duration_ms = 0;
    uint8_t vib_count = 0;
    uint8_t vib_gain = 0;
    uint16_t interval_ms = 0;
    bc_device_info_linear_motor_config_get(&duration_ms, &vib_count, &vib_gain, &interval_ms);
    if(duration_ms > 0 && vib_gain > 0)
    {
        // 参数范围限制
        if(duration_ms > 20)
            duration_ms = 20;
        if(vib_gain > 0x90)
            vib_gain = 0x90;

        s_motor_config.duration_ms = duration_ms;
        s_motor_config.vib_count = vib_count;
        s_motor_config.vib_gain = vib_gain;
        s_motor_config.interval_ms = interval_ms;
        BC_LOG_INFO("motor config restore from flash: dur:%d cnt:%d gain:%02x intv:%d\r\n",
                    duration_ms, vib_count, vib_gain, interval_ms);
    }
#endif
}


static void app_linear_motor_handler_thread(void *thread_handler)
{
    
    //bc_event_bits event_bits = 0;
#if defined(HANDWARE_1_23_3)
    bc_motor_init();
#endif
    if(!batisok) {
        bool res = bc_fuel_gauge_init();
        if(!res)
            batisok = 1;
        else
            batisok = 0;
    }
  while(true)
  {
//      event_bits = bc_rtos_event_group_wait_bits(event_struct.event_handler,
//                                                MOTOR_INIT_EVENT | MOTOR_UNINIT_EVENT | MOTOR_START_EVENT | MOTOR_STOP_EVENT | MOTOR_STATUS_EVENT, 
//                                                event_struct.event_clear_on_exit,event_struct.event_wait_for_all_bits,100);
    
    memset((void *)&diic1, 0, sizeof(diic1));
      if(bc_queue_dequeue(BC_QUEUE_TYPE_IIC1_HANDLE_DATA,(void*)&diic1))  
    {
    //if((event_bits & MOTOR_START_EVENT) == MOTOR_START_EVENT)
    //{
        BC_LOG_INFO("BC_QUEUE_TYPE_IIC1_HANDLE_DATA cmd:%d\r\n",diic1.cmd);
        if(diic1.cmd == CMD_MOTOR_START)
        {
        #if (defined(HANDWARE_1_23_3))
            if(lmisok) {
                uint16_t duration_ms;
                uint8_t  vib_count;
                uint8_t  vib_gain;
                uint16_t interval_ms;

                /* subcmd=1: 外部调用带配置参数，解析参数 */
                if (diic1.subcmd == 1) {
                    /* 从队列数据中解析配置参数 */
                    duration_ms  = (uint16_t)diic1.data[0] | ((uint16_t)diic1.data[1] << 8);
                    vib_count    = diic1.data[2];
                    vib_gain     = diic1.data[3];
                    interval_ms  = (uint16_t)diic1.data[4] | ((uint16_t)diic1.data[5] << 8);

                    /* 保存到全局配置 */
                    s_motor_config.duration_ms = duration_ms;
                    s_motor_config.vib_count   = vib_count;
                    s_motor_config.vib_gain    = vib_gain;
                    s_motor_config.interval_ms = interval_ms;

                    /* 设置剩余次数（0表示无限循环） */
                    if (vib_count == 0) {
                        s_vib_remain_count = 0;  /* 无限循环 */
                    } else {
                        s_vib_remain_count = vib_count - 1;  /* 第一次马上执行，剩余减1 */
                    }

                    s_motor_running = 1;

                    BC_LOG_INFO("motor start: dur=%dms cnt=%d gain=0x%02x intv=%dms\r\n",
                                duration_ms, vib_count, vib_gain, interval_ms);
                }
                /* subcmd=2: 定时器触发的续震，使用已保存的配置 */
                else if (diic1.subcmd == 2) {
                    duration_ms  = s_motor_config.duration_ms;
                    vib_gain     = s_motor_config.vib_gain;
                    vib_count    = s_vib_remain_count;
                    interval_ms  = s_motor_config.interval_ms;
                }
                /* subcmd=0 或其他: 兼容旧版调用（data[0] 为震动次数 count） */
                else {
                    uint8_t count = diic1.data[0];
                    duration_ms  = s_motor_config.duration_ms;
                    vib_gain     = s_motor_config.vib_gain;
                    interval_ms  = s_motor_config.interval_ms;
                    vib_count    = count;
                    if (count == 0) {
                        s_vib_remain_count = 0;  /* 无限循环 */
                    } else {
                        s_vib_remain_count = count - 1;  /* 第一次马上执行，剩余减1 */
                    }
                    s_motor_running = 1;
                }

                /* 执行一次震动 */
                bc_linear_motor_i2c_open();
                bc_delay_ms(20);

                uint8_t ret = g_func_haptic_nv->long_vib_work(4, vib_gain, duration_ms);

                bc_linear_motor_i2c_close();

                if (AW_SUCCESS != ret) {
                    BC_LOG_INFO("long_vib_work fail!\r\n");
                }

                /* 判断是否需要启动定时器（多次震动或无限循环 */
                if (s_motor_running && (s_vib_remain_count > 0 || s_motor_config.vib_count == 0)) {
                    /* 启动间隔定时器 */
                    uint32_t timer_ticks = s_motor_config.interval_ms;  /* 假设 1 tick = 1ms */
                    BC_LOG_INFO("motor next vib, remain=%d, interval=%dms\r\n",
                                s_vib_remain_count, s_motor_config.interval_ms);
                    bc_rtos_timer_start(
                        timer_struct[BC_LINEAR_MOTOR_IC_MODE_TIMER].timer_handler,
                        timer_ticks);
                } else {
                    s_motor_running = 0;
                    BC_LOG_INFO("motor vib finished\r\n");
                }
            } else {
                BC_LOG_INFO("bc_linear_motor_ic_device_init fail\r\n");
            }
        #endif
        } else if (CMD_MOTOR_GET_ID == diic1.cmd) {
        #if (defined(HANDWARE_1_23_3))
            BC_LOG_INFO("CMD_MOTOR_GET_ID :%d,%d,%d,%d,%d\r\n", diic1.data[0],diic1.data[1],diic1.data[2],diic1.data[3],diic1.data[4]);
            if(lmisok) {
                uint32_t reg = 0;
                struct app_cmd_package * cmd_package = (struct app_cmd_package *)diic1.data;
                bc_linear_motor_i2c_open();
                bc_delay_ms(20);
                uint8_t ret = haptic_nv_read_chipid(&reg, AW_FIRST_TRY);
                if (ret != AW_SUCCESS) {
                    ret = haptic_nv_read_chipid(&reg, AW_LAST_TRY);
                    if (ret != AW_SUCCESS)
                        break;
                }
                bc_linear_motor_i2c_close();
                BC_LOG_INFO("MOTOR_ID:%04x\r\n",reg);
                cmd_package->data[0] = reg;
                app_package_send_enqueue(cmd_package,5);
            } else
                BC_LOG_INFO("bc_linear_motor_ic_device_init fail\r\n");
        #endif
        }else if (CMD_CW221X_GET_ID == diic1.cmd) {
            if(batisok) {
                struct app_cmd_package * cmd_package = (struct app_cmd_package *)diic1.data;
                uint8_t id = bc_fuel_gauge_getId();
                BC_LOG_INFO("bc_fuel_gauge_getId : %d\r\n", id);
                cmd_package->data[0] = id;
                app_package_send_enqueue(cmd_package,5);
            }
        }else if (CMD_CW221X_GET_CAP == diic1.cmd) {
            struct app_cmd_package * cmd_package = (struct app_cmd_package *)diic1.data;
            if(!batisok) {
                bool res = bc_fuel_gauge_init();
                if(!res)
                    batisok = 1;
                else
                    batisok = 0;
            }
            if(batisok) {
                uint8_t bp = bc_fuel_gauge_getBattPer();
                BC_LOG_INFO("bc_fuel_gauge_getBattPer: %d\r\n",bp);
                precent = bp;
                if(!diic1.subcmd) {
                    cmd_package->data[0] = bp;
                    app_package_send_enqueue(cmd_package,5);
                } else
                    app_package_precent_up(bp);
            }else{
                cmd_package->data[0] = 255;
                app_package_send_enqueue(cmd_package,5);
            }
        }else if (CMD_TX1812_GET_TEMP_ID == diic1.cmd) {
            #if (defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3))
            struct app_cmd_package * cmd_package = (struct app_cmd_package *)diic1.data;
            bc_temper_id_get(cmd_package->data);
            app_package_send_enqueue(cmd_package,8);
            #endif
        }
    }   
  }
}



void bc_linear_motor_ic_device_init(void)
{   
    //event_struct.event_handler = bc_rtos_event_group_create();
    bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < MOTOR_TASK_TYPE_NUM; i++)
	{
		x_return  = bc_rtos_thread_create((TaskFunction_t )app_linear_motor_thread[i].thread_task_code,     	
                                     (const char*    )app_linear_motor_thread[i].thread_name,   	
                                     (uint16_t       )app_linear_motor_thread[i].thread_stack_depth, 
                                     (void*          )&app_linear_motor_thread[i].thread_parameters,				
                                     (UBaseType_t    )app_linear_motor_thread[i].thread_priority,	
                                     (TaskHandle_t*  )&app_linear_motor_thread[i].thread_handler); 
		if(x_return == bc_pdPASS)
		{
			BC_LOG_INFO("create %s succeed \r\n",app_linear_motor_thread[i].thread_name);
//      bc_rtos_thread_start_scheduler();
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",app_linear_motor_thread[i].thread_name);
		}	
	}
    for(uint8_t i = 0;i <  BC_LINEAR_MOTOR_IC_MODE_TIMER_TYPE_NUM; i++)
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




