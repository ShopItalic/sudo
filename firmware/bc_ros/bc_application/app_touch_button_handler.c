#include "app_touch_button_handler.h"

#include <string.h>

#include "bc_rtos.h"
#include "bc_sem.h"
#include "bc_logger.h"
#include "bc_delay.h"

#include "bc_touch_button.h"
#include "bc_touch_button_device_port.h"
#include "bc_device_info.h"
#include "bc_strategy_value.h"

#include "app_package.h"
#include "app_ble_handler.h"
#include "app_pdm_handler.h"
#include "app_key_handler.h"

#include "bc_ic_led.h"

#if (HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1)	

#include "app_pdm_handler.h"
#include "app_linear_motor_handler.h"
   
#endif

#if defined(HANDWARE_1_23_3 )
#include "bc_linear_motor_ic.h"
#endif
#include "bc_rtc.h"
#include "bc_ble_adv.h"
#include "app_pmic_handler.h"

#if defined(HANDWARE_1_23_2_ONE_SEC)
#include "app_ppg_file_data_handler.h"
#include "app_linear_motor_handler.h"
#endif

enum APP_TOUCH_EVENT
{
  TOUCH_INIT_EVENT   = (0x00000001 << 0),
  TOUCH_UNINIT_EVENT = (0x00000001 << 1),
  TOUCH_SINGLE_TAP_EVENT = (0x00000001 << 2),
  TOUCH_DOUBLE_TAP_EVENT = (0x00000001 << 3),
  TOUCH_TRIPLE_TAP_EVENT = (0x00000001 << 4),
  TOUCH_SWIPE_LEFT_EVENT = (0x00000001 << 5),
  TOUCH_SWIPE_RIGHT_EVENT = (0x00000001 << 6),
  TOUCH_SWIPE_UP_EVENT = (0x00000001 << 7),
  TOUCH_SWIPE_DOWN_EVENT = (0x00000001 << 8),
  TOUCH_HOLD_EVENT = (0x00000001 << 9),
  TOUCH_HOLD_START_EVENT = (0x00000001 << 11),
  TOUCH_HOLD_STOP_EVENT = (0x00000001 << 12),
  TOUCH_IRQ_EVENT = (0x00000001 << 13),
};

static bc_rtos_event_struct event_struct = {

  .event_name              = "touch event",
  .event_clear_on_exit     = bc_pdTRUE,
  .event_wait_for_all_bits = bc_pdFALSE,
};


enum app_touch_task_type
{
  TOUCH_EVNET_TASK_TYPE = 0,
	TOUCH_TASK_TYPE_NUM
};

uint32_t pdm_stop_utime = 0;

static void app_touch_event_handler_thread(void *thread_handler);

static bc_rtos_thread_struct thread_struct[TOUCH_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "touch event task",
                                                                      .thread_stack_depth   = APP_TOUCH_EVENT_STACK_SIZE ,
                                                                      .thread_priority      = APP_TOUCH_EVENT_PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = app_touch_event_handler_thread,
                                                                    },	                                                                    
                                                                  };

struct app_touch_key_hold
{
	uint32_t key_hold_cunt;
	uint32_t key_hold_cunt_temp;
	uint32_t key_hold_timeout_cunt;
};

static struct app_touch_key_hold touch_key_hold = {0};   

enum app_touch_timer_type
{
	TOUCH_HOLD_TIMEOUT_TIMER = 0,
	TOUCH_TIMER_TYPE_NUM
}; 

static void app_touch_hold_timeout_timer_callback (void * pvParameter);

static bc_rtos_timer_struct  timer_struct[TOUCH_TIMER_TYPE_NUM] = {
	{
		.timer_name = "touch hold timeout timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 500,
		.lock = false,
		.timer_callback_function = app_touch_hold_timeout_timer_callback,
	}
};

static void app_touch_single_tap_callback(void);
static void app_touch_double_tap_callback(void);
static void app_touch_triple_tap_callback(void);
static void app_touch_swipe_left_callback(void);
static void app_touch_swipe_right_callback(void);
static void app_touch_swipe_up_callback(void);
static void app_touch_swipe_down_callback(void);
static void app_touch_button_hold_callback(void);   

static void app_touch_timer_start(enum app_touch_timer_type timer_type)
{
  if(!timer_struct[timer_type].lock)
  {
    timer_struct[timer_type].lock = true;
    bc_rtos_timer_start(timer_struct[timer_type].timer_handler,50);
  }
}

static void app_touch_timer_stop(enum app_touch_timer_type timer_type)
{
  if(timer_struct[timer_type].lock)
  {
    timer_struct[timer_type].lock = false;
    bc_rtos_timer_stop(timer_struct[timer_type].timer_handler,50);
  }
}


static void app_touch_hold_timeout_timer_callback (void * pvParameter)
{

  BC_LOG_INFO("TOUCH hold stop !!!!!!!!!!!!\r\n");

  touch_key_hold.key_hold_cunt = 0;
  touch_key_hold.key_hold_cunt_temp = 0;
  touch_key_hold.key_hold_timeout_cunt = 0;
  bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_HOLD_STOP_EVENT);         
   bc_rtos_timer_stop(timer_struct[TOUCH_HOLD_TIMEOUT_TIMER].timer_handler,50);
}


#if ( HARDWARE_1231_ENABLED == 1)	
static bool app_pdm_key_flag = true;
#endif
static void app_touch_event_handler_thread(void *thread_handler)
{
  bc_event_bits event_bits = 0;
  while(true)
  {
    event_bits = bc_rtos_event_group_wait_bits(event_struct.event_handler,
                                                TOUCH_INIT_EVENT | TOUCH_UNINIT_EVENT | TOUCH_SINGLE_TAP_EVENT | TOUCH_DOUBLE_TAP_EVENT | TOUCH_TRIPLE_TAP_EVENT \
                                                | TOUCH_SWIPE_LEFT_EVENT | TOUCH_SWIPE_RIGHT_EVENT | TOUCH_SWIPE_UP_EVENT | TOUCH_SWIPE_DOWN_EVENT | TOUCH_HOLD_EVENT | TOUCH_IRQ_EVENT \
                                                | TOUCH_HOLD_START_EVENT | TOUCH_HOLD_STOP_EVENT, 
                                                event_struct.event_clear_on_exit,event_struct.event_wait_for_all_bits,bc_rtos_max_delay);
    
    if((event_bits & TOUCH_INIT_EVENT) == TOUCH_INIT_EVENT)
    {
      bc_touch_button_init();
      bc_touch_button_gesture_event_hold_register_callback(app_touch_button_hold_callback);
      bc_touch_single_tap_register_callback_register_callback(app_touch_single_tap_callback);
      bc_touch_double_tap_register_callback_register_callback(app_touch_double_tap_callback);
      bc_touch_triple_tap_register_callback_register_callback(app_touch_triple_tap_callback);
      bc_touch_swipe_left_register_callback_register_callback(app_touch_swipe_left_callback);
      bc_touch_swipe_right_register_callback_register_callback(app_touch_swipe_right_callback);
      bc_touch_swipe_up_register_callback_register_callback(app_touch_swipe_up_callback);
      bc_touch_swipe_down_register_callback_register_callback(app_touch_swipe_down_callback);
      BC_LOG_INFO("TOUCH_INIT_EVENT \r\n");
    }
    else if((event_bits & TOUCH_UNINIT_EVENT) == TOUCH_UNINIT_EVENT)
    {
      bc_touch_button_gesture_event_hold_register_callback(NULL);
      bc_touch_single_tap_register_callback_register_callback(NULL);
      bc_touch_double_tap_register_callback_register_callback(NULL);
      bc_touch_triple_tap_register_callback_register_callback(NULL);
      bc_touch_swipe_left_register_callback_register_callback(NULL);
      bc_touch_swipe_right_register_callback_register_callback(NULL);
      bc_touch_swipe_up_register_callback_register_callback(NULL);
      bc_touch_swipe_down_register_callback_register_callback(NULL);      
      bc_touch_button_uninit();
      BC_LOG_INFO("TOUCH_UNINIT_EVENT \r\n");
    }
    else if((event_bits & TOUCH_SINGLE_TAP_EVENT) == TOUCH_SINGLE_TAP_EVENT)
    {
      BC_LOG_INFO("TOUCH_SINGLE_TAP_EVENT \r\n");
      app_package_button_up(BUTTON_CLICK_PRESS);

#if defined(HANDWARE_1_23_2_ONE_SEC)
      /* 1.23.2_one_sec版本：离线录音时单击记录时间戳 */
      if(!app_ble_connect_status())
      {
        if(app_pdm_mode_get() == PDM_MODE_OFFLINE && app_pdm_work_status())
        {
          app_single_tap_record_write();
        }
      }
#endif
    }
    else if((event_bits & TOUCH_DOUBLE_TAP_EVENT) == TOUCH_DOUBLE_TAP_EVENT)
    {
       BC_LOG_INFO("TOUCH_DOUBLE_TAP_EVENT \r\n");
      
#if ( HARDWARE_1231_ENABLED == 1)	
#ifndef HANDWARE_1_23_2L
      if(app_ble_connect_status()){
        if(app_pdm_key_flag){
#if defined(HANDWARE_1_23_2_ONE_SEC)
          /* HANDWARE_1_23_2_ONE_SEC: 去掉PDM状态判断，由优先级机制控制 */
          app_pdm_touch_start();
          app_pdm_key_flag = false;
#else
          if(app_pdm_mode_get() == PDM_MODE_IDIE)
          {
            app_pdm_touch_start();
            app_pdm_key_flag = false;
          }       
#endif
        }
        else
        {
#if defined(HANDWARE_1_23_4)
          /* HANDWARE_1_23_4: 暂停状态下双击，先恢复录音 */
          if(app_pdm_recording_is_paused())
          {
            app_pdm_recording_resume();
            bc_ic_led_recording_pause_off();
          }
          else
#endif
          {
            app_pdm_touch_stop();
            app_pdm_key_flag = true;
          }
        }
      }
      else {
          enum app_pdm_mode mpdm = app_pdm_mode_get();
        if(app_pdm_key_flag){
#if defined(HANDWARE_1_23_2_ONE_SEC)
          /* HANDWARE_1_23_2_ONE_SEC: 去掉PDM状态判断，由优先级机制控制 */
          app_pdm_key_flag = false;
          app_package_mic_recording_start();
          app_pdm_key_flag = false;
#else
          if(mpdm == PDM_MODE_IDIE)
          {
              app_pdm_key_flag = false;
            app_package_mic_recording_start();
            
          }   
            app_pdm_key_flag = false;          
#endif
        }
        else
        {
#if defined(HANDWARE_1_23_2_ONE_SEC)
          /* HANDWARE_1_23_2_ONE_SEC: 去掉PDM状态判断，直接停止录音 */
          app_pdm_key_flag = true;
          app_package_mic_recording_stop();
        #ifdef ADVERTISING_UPDATE
          uint32_t reltm = bg_rtc_time_get_uinx_time();
          if(reltm) {
              //taskENTER_CRITICAL();
              pdm_stop_utime = reltm;
              //taskEXIT_CRITICAL();
          }
          bc_ble_adv_data_update((uint8_t *)&pdm_stop_utime, sizeof(pdm_stop_utime));
        #endif
          app_pdm_key_flag = true;
#else
          if(mpdm == PDM_MODE_OFFLINE)
          {
              app_pdm_key_flag = true;
            app_package_mic_recording_stop();
        #ifdef ADVERTISING_UPDATE
              uint32_t reltm = bg_rtc_time_get_uinx_time();
              if(reltm) {
                  //taskENTER_CRITICAL();
                  pdm_stop_utime = reltm;
                  //taskEXIT_CRITICAL();
              }
              bc_ble_adv_data_update((uint8_t *)&pdm_stop_utime, sizeof(pdm_stop_utime));
        #endif
          }
          app_pdm_key_flag = true;
#endif
        }
      }
#endif
#endif
     app_package_button_up(BUTTON_DOUBLE_PRESS);
    }
    else if((event_bits & TOUCH_TRIPLE_TAP_EVENT) == TOUCH_TRIPLE_TAP_EVENT)
    {
      BC_LOG_INFO("TOUCH_TRIPLE_TAP_EVENT \r\n");
      app_package_button_up(BUTTON_THREE_PRESS);
	    
    }
    else if((event_bits & TOUCH_SWIPE_LEFT_EVENT) == TOUCH_SWIPE_LEFT_EVENT)
    {
      BC_LOG_INFO("TOUCH_SWIPE_LEFT_EVEN \r\n");
      app_package_button_up(BUTTON_SWIPE_LEFT_PRESS);
  
    }
    else if((event_bits & TOUCH_SWIPE_RIGHT_EVENT) == TOUCH_SWIPE_RIGHT_EVENT)
    {
      BC_LOG_INFO("TOUCH_SWIPE_RIGHT_EVENT \r\n");
      app_package_button_up(BUTTON_SWIPE_RIGHT_PRESS);
      
    }
    else if((event_bits & TOUCH_SWIPE_UP_EVENT) == TOUCH_SWIPE_UP_EVENT)
    {
      BC_LOG_INFO("TOUCH_SWIPE_UP_EVEN \r\n");
      app_package_button_up(BUTTON_SWIPE_UP_PRESS);
    }
    else if((event_bits & TOUCH_SWIPE_DOWN_EVENT) == TOUCH_SWIPE_DOWN_EVENT)
    {
      BC_LOG_INFO("TOUCH_SWIPE_DOWN_EVENT \r\n");
      app_package_button_up(BUTTON_SWIPE_DOWN_PRESS);
    }
    else if((event_bits & TOUCH_HOLD_EVENT) == TOUCH_HOLD_EVENT)
    {
      BC_LOG_INFO("TOUCH_HOLD_EVENT \r\n");
      touch_key_hold.key_hold_cunt ++;
      if(touch_key_hold.key_hold_cunt == 1)
      {
          #if defined(HANDWARE_1_23_4)
              /* HANDWARE_1_23_4: 长按开始录音，LED指示 */
              if(app_ble_connect_status()){
                /* 在线：绿灯长亮 */
                if(app_pdm_key_flag){
                  app_pdm_touch_start();
                  app_pdm_key_flag = false;
                  bc_ic_led_hold_recording_online_on();
                }
              }
          #endif
        BC_LOG_INFO("TOUCH hold start !!!!!!!!!!!!\r\n");
        app_package_button_up(BUTTON_LONG_PRESS);
        bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_HOLD_START_EVENT);
      }
      bc_rtos_timer_reset(timer_struct[TOUCH_HOLD_TIMEOUT_TIMER].timer_handler,50);
    }
    else if((event_bits & TOUCH_HOLD_START_EVENT) == TOUCH_HOLD_START_EVENT)
    {
        BC_LOG_INFO("TOUCH_HOLD_START_EVENT app_pdm_key_flag:%d\r\n",app_pdm_key_flag);
#if ( HARDWARE_1231_ENABLED == 1)	      
#if defined(HANDWARE_1_23_2L )
//      if(app_pdm_mode_get() == PDM_MODE_OFFLINE)
//      {
//        app_package_mic_recording_stop();
//        app_pdm_key_flag = true;
//      }
//      if(app_ble_connect_status())
//      {
//        app_pdm_touch_start();
//      }     
        if(app_pdm_key_flag){
            BC_LOG_INFO("TOUCH_MOLD_START_EVENT********app_pdm_mode_get:%d \r\n",app_pdm_mode_get());
          if(app_pdm_mode_get() == PDM_MODE_IDIE)
          {
              BC_LOG_INFO("TOUCH_MOLD_START_EVENT****************************** \r\n");
            app_package_mic_recording_start();
            app_pdm_key_flag = false;
          }       
        }
//        else
//        {
//          if(app_pdm_mode_get() == PDM_MODE_OFFLINE)
//          {
//            app_package_mic_recording_stop();
//            app_pdm_key_flag = true;
//          }
//        }
#elif defined(HANDWARE_1_23_2_ONE_SEC)
#if 0
      /* HANDWARE_1_23_2_ONE_SEC: 长按开始录音，由优先级机制控制 */
      if(app_ble_connect_status()){
        if(app_pdm_key_flag){
          app_pdm_touch_start();
          app_pdm_key_flag = false;
        }
      }
      else {
        if(app_pdm_key_flag){
          app_pdm_key_flag = false;
          app_package_mic_recording_start();
          app_pdm_key_flag = false;
        }
	  }
#endif
#elif defined(HANDWARE_1_23_4)
      /* HANDWARE_1_23_4: 长按开始录音，LED指示 */
//      if(app_ble_connect_status()){
//        /* 在线：绿灯长亮 */
//        if(app_pdm_key_flag){
//          app_pdm_touch_start();
//          app_pdm_key_flag = false;
//          bc_ic_led_hold_recording_online_on();
//        }
//      }
//      else {
//        /* 离线：紫灯长亮 */
//        if(app_pdm_key_flag){
//          app_package_mic_recording_start();
//          app_pdm_key_flag = false;
//          bc_ic_led_hold_recording_offline_on();
//        }
//      }
#endif
    }

#endif    
    else if((event_bits & TOUCH_HOLD_STOP_EVENT) == TOUCH_HOLD_STOP_EVENT)
    {
        BC_LOG_INFO("TOUCH_HOLD_STOP_EVENT app_pdm_key_flag:%d\r\n",app_pdm_key_flag);
#if ( HARDWARE_1231_ENABLED == 1)	      
#if defined(HANDWARE_1_23_2L )
//      if(app_ble_connect_status())
//      {
//        app_pdm_touch_stop();
//      }
        if(!app_pdm_key_flag){
          if(app_pdm_mode_get() == PDM_MODE_OFFLINE)
          {
              BC_LOG_INFO("TOUCH_HOLD_STOP_EVENT****************************** \r\n");
            app_package_mic_recording_stop();
            app_pdm_key_flag = true;
          }
        }
#elif defined(HANDWARE_1_23_2_ONE_SEC)
#if 0
      /* HANDWARE_1_23_2_ONE_SEC: 长按松开停止录音 */
      if(app_ble_connect_status()){
        if(!app_pdm_key_flag){
          app_pdm_touch_stop();
          app_pdm_key_flag = true;
        }
      }
      else {
        if(!app_pdm_key_flag){
          app_pdm_key_flag = true;
          app_package_mic_recording_stop();
        #ifdef ADVERTISING_UPDATE
          uint32_t reltm = bg_rtc_time_get_uinx_time();
          if(reltm) {
              pdm_stop_utime = reltm;
          }
          bc_ble_adv_data_update((uint8_t *)&pdm_stop_utime, sizeof(pdm_stop_utime));
        #endif
          app_pdm_key_flag = true;
        }
      }
#endif
#elif defined(HANDWARE_1_23_4)
      /* HANDWARE_1_23_4: 长按松开停止录音，关闭LED */
      if(app_ble_connect_status()){
        if(!app_pdm_key_flag){
          app_pdm_touch_stop();
          app_pdm_key_flag = true;
          bc_ic_led_hold_recording_off();
        }
      }
      else {
        if(!app_pdm_key_flag){
          app_pdm_key_flag = true;
          app_package_mic_recording_stop();
        #ifdef ADVERTISING_UPDATE
          uint32_t reltm = bg_rtc_time_get_uinx_time();
          if(reltm) {
              pdm_stop_utime = reltm;
          }
          bc_ble_adv_data_update((uint8_t *)&pdm_stop_utime, sizeof(pdm_stop_utime));
        #endif
          app_pdm_key_flag = true;
          bc_ic_led_hold_recording_off();
        }
      }
#endif
    }
#endif    
    
    else if((event_bits & TOUCH_IRQ_EVENT) == TOUCH_IRQ_EVENT)
    {
      //BC_LOG_INFO("TOUCH_IRQ_EVENT \r\n");
        //services_print_log("TOUCH_IRQ_EVENT \r\n");
      bc_touch_button_irq_process(); 
      
    }
  }
}







static void app_touch_rawdata_event_callback(void * p_context)
{

}

static uint16_t long_key_press_count = 0;
static void app_touch_button_poll_timeout_timer_callback(void * pvParameter)
{

}

//static uint16_t long_key_press_count = 0;
static void app_touch_button_long_press_timer_callback(void * pvParameter)
{
		
	app_package_button_up(BUTTON_LONG_PRESS);
}

static void app_button_handler(void)
{
	
}

static void app_touch_button_gesture_event_flick_positive_handler(void)
{	

}

static void app_touch_button_gesture_event_flick_nositive_handler(void)
{
}

static void app_touch_button_gesture_event_hold_handler(void)
{
}


static void app_touch_button_irq_callback(uint8_t gpio_pin,uint8_t gpio_status)
{
  BC_LOG_INFO("app touch  button_irq !! !!!!!!!!!!!!!!!!!!!!\r\n");
//  bc_rtos_thread_notify_give_from_isr(&thread_struct[TOUCH_IRQ_TASK_TYPE],bc_pdTRUE); //中断内发送通知
//  bc_rtos_sem_count_give(BC_TOUCH_ISR_SEM_COUNT);
//  bc_rtos_thread_notify_give(thread_struct[TOUCH_IRQ_TASK_TYPE].thread_handler);
//  app_touch_key_io_irq_callback();
//  if(app_pdm_mode_get() == PDM_MODE_OFFLINE)
//  {
//    app_touch_timer_start(TOUCH_POLL_TIMER);
//  }
  #if 0 // liukun
  //bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_IRQ_EVENT );
  #else
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(event_struct.event_handler, TOUCH_IRQ_EVENT, &xHigherPriorityTaskWoken);
  #endif
  
}

static void app_touch_single_tap_callback(void)
{
//  BC_LOG_INFO("app touch  single tap !! \r\n");
  bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_SINGLE_TAP_EVENT );
}

static void app_touch_double_tap_callback(void)
{
//  BC_LOG_INFO("app touch  double tap !! \r\n");
  bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_DOUBLE_TAP_EVENT );
}

static void app_touch_triple_tap_callback(void)
{
//  BC_LOG_INFO("app touch  triple tap !! \r\n");
  bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_TRIPLE_TAP_EVENT );
}

static void app_touch_swipe_left_callback(void)
{
//  BC_LOG_INFO("app touch swipe_left !! \r\n");
  bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_SWIPE_LEFT_EVENT );
}

static void app_touch_swipe_right_callback(void)
{
//  BC_LOG_INFO("app touch swipe right !! \r\n");
  bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_SWIPE_RIGHT_EVENT );
}

static void app_touch_swipe_up_callback(void)
{
//  BC_LOG_INFO("app touch swipe up !! \r\n");
  bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_SWIPE_UP_EVENT );
}

static void app_touch_swipe_down_callback(void)
{
//  BC_LOG_INFO("app touch swipe down !! \r\n");
  bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_SWIPE_DOWN_EVENT );
}



static void app_touch_button_hold_callback(void)
{
	BC_LOG_INFO("app touch  hold !!!!!!!!!!!!!!!!!!!!!!!! \r\n");
  bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_HOLD_EVENT );
}

static void app_touch_button_alp_ati_error_register_callback(void)
{
  bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_INIT_EVENT );
}


void app_touch_init_event(void)
{
	bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_INIT_EVENT );
}

void app_touch_uninit_event(void)
{
	bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_UNINIT_EVENT );
//	temp_flag = false;
}

void app_touch_low_power(void)
{
	if(!app_ble_connect_status())
	{
		bc_rtos_event_group_set_bits(event_struct.event_handler,TOUCH_UNINIT_EVENT );
	}
}


static bool app_button_handware_chek_flag = false;
void app_button_rawdata_event(uint8_t *rawdata,uint8_t rawdata_length)
{
    BC_LOG_HEX("rawdata:", rawdata, rawdata_length);

    //services_print_log("0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\n",rawdata[0],rawdata[1],rawdata[2],rawdata[3],rawdata[4],rawdata[5],rawdata[6],rawdata[7]);
}

void app_touch_check_start(struct app_cmd_package * pack)
{
}

void app_button_handware_chek_start(void)
{
	app_button_handware_chek_flag = true;
	BC_LOG_INFO("app touch check start!! \r\n");
}

void app_button_handware_chek_stop(void)
{
	app_button_handware_chek_flag = false;
	BC_LOG_INFO("app touch check stop!! \r\n");
}

bool app_touch_pdm_audio_status_get(void)
{
}

void app_touch_pdm_audio_stop(void)
{
	app_touch_button_long_press_timer_callback(NULL);
}

void app_touch_pdm_key_flag_clear(void)
{
  if(app_pdm_mode_get() == PDM_MODE_ONLINE ){
    app_pdm_touch_stop();
    app_pdm_key_flag = true;
  }
}

void app_touch_pdm_key_flag_set(bool flag)
{
  app_pdm_key_flag = flag;
}

void app_touch_handler_init(void)
{
	event_struct.event_handler = bc_rtos_event_group_create();
  if(event_struct.event_handler != NULL)
  {
    BC_LOG_INFO("create %s succeed \r\n",event_struct.event_name);
  }
  else
  {
    BC_LOG_ERROR("create  %s fail",event_struct.event_name);
  }	
	
	bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < TOUCH_TASK_TYPE_NUM; i++)
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
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",thread_struct[i].thread_name);
		}	
	}
  for(uint8_t i = 0;i <  TOUCH_TIMER_TYPE_NUM; i++)
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
	
  bc_touch_alp_ati_error_register_callback(app_touch_button_alp_ati_error_register_callback);
	
	touch_irq_register_callback(app_touch_button_irq_callback);
	
	bc_touch_event_rawdata_callback_register_callback(app_button_rawdata_event);
	
	
      bc_touch_button_gesture_event_hold_register_callback(app_touch_button_hold_callback);
      bc_touch_single_tap_register_callback_register_callback(app_touch_single_tap_callback);
      bc_touch_double_tap_register_callback_register_callback(app_touch_double_tap_callback);
      bc_touch_triple_tap_register_callback_register_callback(app_touch_triple_tap_callback);
      bc_touch_swipe_left_register_callback_register_callback(app_touch_swipe_left_callback);
      bc_touch_swipe_right_register_callback_register_callback(app_touch_swipe_right_callback);
      bc_touch_swipe_up_register_callback_register_callback(app_touch_swipe_up_callback);
      bc_touch_swipe_down_register_callback_register_callback(app_touch_swipe_down_callback);

	
//	bc_touch_button_init();

}


//void app_touch_test(void)
//{
//	bc_event_set(&event_struct[APP_TOUCH_PROCESS_EVENT]);
//}





