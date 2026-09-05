#include "app_ble_handler.h"


#include "bc_ble_modu_interface.h"
#include "bc_ble.h"
#include "bc_queue.h"

#include "app_touch_button_handler.h"
#include "app_package.h"
#include "app_ppg_handler.h"
#include "app_hid_handler.h"

#if (HARDWARE_153_ENABLED == 1 || HARDWARE_1121_ENABLED || HARDWARE_158_ENABLED == 1 || HARDWARE_1191_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

#include "app_pdm_handler.h"		
   
#endif	

 #if ( HARDWARE_1121_ENABLED == 1)	

#include "app_mouse_handler.h"
   
#endif

#if ( HARDWARE_1191_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

#include "bc_led_pwm.h"

#endif	

#if (HARDWARE_1231_ENABLED == 1)	

#include "bc_ic_led.h"

#endif	

#include "bc_rtos.h"
#include "bc_queue.h"
#include "bc_logger.h"
#include "bc_led.h"

#include "string.h"
#include "app_six_axis_sensor_handler.h"

static struct bc_ble_calss ble_calss = {0};

#define BLE_CONNECT_NOTIFY_INHIBIT_MS   3000
static uint32_t ble_connect_tick = 0;
static bool ble_notify_inhibit = false;

static void app_ble_recv_handler_thread(void *thread_handler);
static void app_ble_send_handler_thread(void *thread_handler);

static enum app_ble_hid_touch_mode ble_hid_mode = BLE_HID_TOUCH_VIDEO_MODE;

static bc_rtos_thread_struct app_ble_thread[BLE_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "ble recv handler task",
                                                                      .thread_stack_depth   = APP_TASK_BLE_RECV_STACK_SIZE ,
                                                                      .thread_priority      = APP_TASK_BLE_RECV_PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = app_ble_recv_handler_thread,
                                                                    },
                                                                    {
                                                                      .thread_name          = " ble send handler task",
                                                                      .thread_stack_depth   = APP_TASK_BLE_SEND_STACK_SIZE ,
                                                                      .thread_priority      = APP_TASK_BLE_SEND_PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = app_ble_send_handler_thread,
                                                                    }																		
                                                                  };


static void app_ble_connect_idie_timeout_timer_callback(void * pvParameter);

static bc_rtos_timer_struct  ble_timer[BLE_TIMER_TYPE_NUM] = {
                                                               {
                                                                 .timer_name = "ble connect idie timer",
                                                                 .uxAutoReload = bc_pdFALSE,
                                                                 .xTimerPeriodInTicks = 1000*60*30,
                                                                 .timer_id = APP_BLE_CONNECT_IDIE_TIMEOUT_TIMER_ID ,
                                                                 .timer_callback_function = app_ble_connect_idie_timeout_timer_callback,   
                                                                 .lock = false,
                                                               },
                                                              };													   
                       															  
static void app_ble_adv_light_start(void)
{
  if(!app_ble_connect_status())
  {
#if (defined(HANDWARE_1_19_1))    
    bc_led_white_breathe_start(LED_WHITE_FlLASH_CYCLE_300ms_4700ms);
#endif    
  }
}
                                                              
/*******************************************************************************
 * Function Name     : app_ble_connect_idie_timeout_timer_callback
 * Description       : ble连接空闲超时回调
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static void app_ble_connect_idie_timeout_timer_callback(void * pvParameter)
{
	if(ble_calss.ble_connect_status())
	{
//		ble_calss.ble_disconnect();
//		ble_calss.ble_connect_params_update(BLE_CONN_PARAMS_SLOW);
	}
}
/*******************************************************************************
 * Function Name     : app_ble_connect_callback
 * Description       : ble连接回调
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static void app_ble_connect_callback(void)
{
    //BC_LOG_INFO("app ble connect callback\r\n");
    ble_connect_tick = xTaskGetTickCount();
    ble_notify_inhibit = true;

#if ( HARDWARE_1191_ENABLED == 1)	

  bc_led_white_breathe_start(LED_WHITE_LONG_LIGHT_3S);
  
#elif (HARDWARE_1231_ENABLED == 1)	 
  //bc_ic_led_ble_connect();  
#if defined(SUDO_VOICE_ONLY)
    if (!app_pdm_work_status()) bc_ic_led_ble_connect_from_isr();
#else
    bc_ic_led_ble_connect_from_isr();
#endif
#if defined(HANDWARE_1_23_2 )
    //app_pdm_mode_change_to_online();
#if !defined(SUDO_VOICE_ONLY)
    app_package_mic_recording_stop_isr();
#endif
#endif
#if defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_2_ONE_SEC)
    // 蓝牙重连后延迟PDM BLE发送，等GATT就绪避免SoftDevice断言
    app_pdm_ble_connect_set_tx_delay();
#endif
#else

 

#endif	

#if ( HARDWARE_1191_ENABLED == 1)	

  app_package_active_upload_check();

#endif	  
  
//	app_touch_init_event();
//	app_six_axis_sensor_stop();
#if ( HARDWARE_451_ENABLED == 1)	

#else
  
#if(PPG_ENABLED) 
   app_ppg_stop();
#endif
  
#endif	
//	app_ble_mouse_x_movement(-127);
//    app_hid_photograth_start();
	//BC_LOG_INFO("app ble connect callback\r\n");
	//app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);
    app_connect_idie_timer_start_from_isr(BLE_CONNECT_IDIE_TIMEOUT_TIMER);
}

/*******************************************************************************
 * Function Name     : app_ble_disconnect_callback
 * Description       : ble断开连接回调
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static void app_ble_disconnect_callback(void)
{
    ble_notify_inhibit = false;
	bc_log_ble_disenable();
#if ( HARDWARE_1191_ENABLED == 1)	

  bc_led_white_breathe_start(LED_WHITE_FlLASH_3S);
#elif (HARDWARE_1231_ENABLED == 1)	 
#if defined(HANDWARE_1_23_4)
    /* HANDWARE_1_23_4: 判断是否在录音 */
    if(app_pdm_work_status())
    {
      /* 正在录音：不闪烁蓝灯，直接停止录音并关闭所有LED */
      /* 清除暂停标志 */
      app_pdm_recording_resume();
      /* 停止PDM录音 */
      app_pdm_ble_stop();
      /* 关闭所有录音相关LED */
      bc_ic_led_mic_offline_recording_off();
      bc_ic_led_mic_online_recording_off();
      bc_ic_led_mic_offline_recording_capture_off();
      bc_ic_led_mic_online_recording_capture_off();
      /* 关闭暂停呼吸灯 */
      bc_ic_led_recording_pause_off();
    }
    else
    {
      /* 没有录音：正常闪烁蓝灯 */
  #if defined(SUDO_VOICE_ONLY)
    if (!app_pdm_work_status()) bc_ic_led_ble_disconnect_from_isr();
#else
    bc_ic_led_ble_disconnect_from_isr();
#endif
    }
    app_package_pdm_key_flag_clear();
#else
  //bc_ic_led_ble_disconnect();  
#if defined(SUDO_VOICE_ONLY)
    if (!app_pdm_work_status()) bc_ic_led_ble_disconnect_from_isr();
#else
    bc_ic_led_ble_disconnect_from_isr();
#endif
#if defined(HANDWARE_1_23_2)
    //app_package_pdm_switch_online_to_offline();
    app_package_pdm_key_flag_clear();
#else
    #ifndef HANDWARE_1_23_3
    app_package_pdm_key_flag_clear();
    #endif
#endif
#endif
#else
  
  

#endif	
//	app_touch_uninit_event();
  
 #if ( HARDWARE_451_ENABLED == 1)	

#else
  
#if(PPG_ENABLED)  
   app_ppg_stop();
#endif
  
#endif		
	
 #if (HARDWARE_153_ENABLED == 1  || HARDWARE_BCL601_151_ENABLED || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_1191_ENABLED == 1)	

//   if(app_touch_pdm_audio_status_get())
//  {
//    app_touch_pdm_audio_stop();
//  }
//  else
//  {
//    app_pdm_audio_discooenct_stop();
//  app_pdm_recording_stop();
//  }
   
#endif	 
  
	//BC_LOG_INFO("app ble disconnect callback\r\n");
}

/*******************************************************************************
 * Function Name     : app_ble_pm_connect_callback
 * Description       : ble配对回调
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static void app_ble_pm_connect_callback(void)
{
 #if ( HARDWARE_1121_ENABLED == 1)	

  app_mouse_start();
   
#endif	 
  
}

/*******************************************************************************
 * Function Name     : app_ble_pm_disconnect_callback
 * Description       : ble断开配对回调
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static void app_ble_pm_disconnect_callback(void)
{

 #if ( HARDWARE_1121_ENABLED == 1)	
	
   app_mouse_stop();
	
#endif	 
  
}

#if defined(HANDWARE_1_23_4)
/*******************************************************************************
 * Function Name     : app_ble_connect_guard_callback
 * Description       : BLE连接守卫回调，在BLE协议栈层阻止连接
 *                     离线录音中返回true拒绝连接，避免在app层断开导致后续事件崩溃
 * Input             : 无
 * Output            : 无
 * Return            : true-拒绝连接 false-允许连接
 * Author            : 
 *******************************************************************************/
static bool app_ble_connect_guard_callback(void)
{
    if(app_pdm_work_status() && app_pdm_mode_get() == PDM_MODE_OFFLINE)
    {
        BC_LOG_INFO("ble connect guard: reject - offline recording\r\n");
        return true;
    }
    return false;
}
#endif


/*******************************************************************************
 * Function Name     : app_ble_recv_handler_thread
 * Description       : ble接受处理
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
struct bc_ble_data_package ble_recv_msg = {0};
static void app_ble_recv_handler_thread(void *thread_handler)
{
	
  app_ble_adv_light_start();
	while(true)
	{
		if(bc_queue_dequeue(BC_QUEUE_TYPE_BLE_RECV,(void*)&ble_recv_msg))
		{
#if defined(SUDO_VOICE_ONLY)
            if (ble_recv_msg.session_id != bc_ble_session_id()) continue;
#endif
			BC_LOG_INFO("recv length:%d\r\n",ble_recv_msg.data_length);
			BC_LOG_BLE("recv length:%d\r\n",ble_recv_msg.data_length);
			BC_LOG_HEX_P("recv data:",ble_recv_msg.data,ble_recv_msg.data_length);
	//		bc_queue_enqueue(BC_QUEUE_TYPE_BLE_SEND,&ble_msg);
			app_cmd_package_parse(ble_recv_msg.data,ble_recv_msg.data_length);
			app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);
			ble_calss.ble_connect_params_update(BLE_CONN_PARAMS_FAST);
		}	
    /* Queue receive blocks when idle; drain commands without a fixed delay. */
	}
}
/*******************************************************************************
 * Function Name     : app_ble_send_handler_thread
 * Description       : ble发送处理
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint16_t temp = 0;
struct bc_ble_data_package ble_send_msg = {0};
static void app_ble_send_handler_thread(void *thread_handler)
{
  while(true)
	{
		if(bc_queue_dequeue(BC_QUEUE_TYPE_BLE_SEND,(void*)&ble_send_msg))
		{

			if(ble_calss.ble_connect_status() &&
               ble_send_msg.session_id == bc_ble_session_id())
			{
                bc_ble_send_session(ble_send_msg.data, ble_send_msg.data_length,
                                    ble_send_msg.session_id);
	//			BC_LOG_INFO("send temp:%d",temp++);
			}
            /* Per-packet UART/hex logging throttles audio transfers. */
			memset((uint8_t*)&ble_send_msg,0,sizeof(struct bc_ble_data_package));
			app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_1121_ENABLED == 1)	

//			if(!app_pdm_work_status())
//			{
//			   ble_calss.ble_connect_params_update(BLE_CONN_PARAMS_FAST);
//			}
#else
               ble_calss.ble_connect_params_update(BLE_CONN_PARAMS_FAST); 			
   
#endif
		}	
	}	
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
static void app_ble_time_create(void)
{
	for(uint8_t i = 0;i < BLE_TIMER_TYPE_NUM; i++)
	{
		ble_timer[i].timer_handler = bc_rtos_timer_create(ble_timer[i].timer_name,
														   ble_timer[i].xTimerPeriodInTicks,
														   ble_timer[i].uxAutoReload, 
														   (void *)ble_timer[i].timer_id,
															ble_timer[i].timer_callback_function);
		if(ble_timer[i].timer_handler != NULL)
		{
			BC_LOG_INFO("create %s succeed\r\n",ble_timer[i].timer_name);
		}
		else
		{
			BC_LOG_ERROR("create %s fail\r\n",ble_timer[i].timer_name);
		}		
	}
	
}
/*******************************************************************************
 * Function Name     : app_connect_idie_timer_start
 * Description       : ble连接空闲超时回调
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/										               
void app_connect_idie_timer_start(enum app_ble_timer_type time_id)
{
  bc_rtos_timer_start(ble_timer[time_id].timer_handler,50);
}

void app_connect_idie_timer_start_from_isr(enum app_ble_timer_type time_id)
{
    BaseType_t yieldReq = pdFALSE;
    xTimerStartFromISR(ble_timer[time_id].timer_handler, &yieldReq);
}

bool app_ble_connect_status(void)
{
	return ble_calss.ble_connect_status();
}

bool app_ble_notify_allowed(void)
{
    if(!ble_calss.ble_connect_status())
    {
        return false;
    }
    if(ble_notify_inhibit)
    {
        uint32_t elapsed = xTaskGetTickCount() - ble_connect_tick;
        if(elapsed >= pdMS_TO_TICKS(BLE_CONNECT_NOTIFY_INHIBIT_MS))
        {
            ble_notify_inhibit = false;
        }
        else
        {
            return false;
        }
    }
    return true;
}

void app_ble_send(uint8_t *send_data,uint8_t send_length)
{
    struct bc_ble_data_package packet = {0};
    if (!send_data || !send_length || send_length > sizeof(packet.data) ||
        !ble_calss.ble_connect_status())
        return;
    memcpy(packet.data, send_data, send_length);
    packet.data_length = send_length;
    /* Do not let concurrent IMU/command producers call the radio directly. */
    if (!bc_queue_ble_send(&packet, bc_ble_session_id(), pdMS_TO_TICKS(100)))
        BC_LOG_WARN("BLE TX queue full\r\n");
}


void app_ble_mouse_left_button(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_mouse_button_control(MOUSE_LEFT_BUTTON_HOLD);
	}
}

void app_ble_mouse_right_button(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_mouse_button_control(MOUSE_RIGHT_BUTTON_HOLD);
	}
}

void app_ble_mouse_cancel_button(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_mouse_button_control(MOUSE_BUTTON_CANCL);
	}
}

void app_ble_mouse_slide_up(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_mouse_button_control(MOUSE_SLIDE_UP);
	}
}

void app_ble_mouse_slide_down(void)
{
	if(ble_calss.ble_connect_status())
	{
		ble_calss.ble_mouse_button_control(MOUSE_SLIDE_DOWN);
	}
}


void app_ble_mouse_pulley_up(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_mouse_button_control(MOUSE_PULLEY_UP);
//		app_ble_mouse_y_movement(-50);
//		app_ble_mouse_cancel_button();
	}
}

void app_ble_mouse_pulley_down(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_mouse_button_control(MOUSE_PULLEY_DOWN);
//		app_ble_mouse_cancel_button();
	}
}

void app_ble_mouse_android_pulley_up(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_mouse_button_control(MOUSE_ANDROID_PULLEY_UP);
	}
}

void app_ble_mouse_android_pulley_down(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_mouse_button_control(MOUSE_ANDROID_PULLEY_DOWN);
	}
}

void app_ble_mouse_ios_pulley_up(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_mouse_button_control(MOUSE_IOS_PULLEY_UP);
	}
}

void app_ble_mouse_ios_pulley_down(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_mouse_button_control(MOUSE_IOS_PULLEY_DOWN);
	}
}


void app_ble_mouse_x_movement(int16_t movement_data)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_mouse_movement(movement_data,0);
	}
}

void app_ble_mouse_y_movement(int16_t movement_data)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_mouse_movement(0,movement_data);
	}
}

void app_ble_mouse_x_y_movement(int16_t x_movement_data,int16_t y_movement_data)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_mouse_movement(x_movement_data,y_movement_data);
	}
}
void app_ble_mouse_movement_origin(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_mouse_movement(1,1);
	}
}

void app_ble_hid_volume_up(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_hid_send_cmd(BLE_HID_VOLUSE_UP);
	}
}

void app_ble_hid_volume_down(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_hid_send_cmd(BLE_HID_VOLUSE_DOWN);
	}
}

void app_ble_hid_previous_music(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_hid_send_cmd(BLE_HID_PREVIOUS_MUSIC);
	}
}

void app_ble_hid_next_music(void)
{
	if(ble_calss.ble_connect_status() && ble_calss.ble_pm_connect_status())
	{
		ble_calss.ble_hid_send_cmd(BLE_HID_NEXT_MUSIC);
	}
}

void app_ble_hid_touch_mode_set(enum app_ble_hid_touch_mode hid_mode)
{
	ble_hid_mode = hid_mode;
}

enum app_ble_hid_touch_mode app_ble_hid_mode_get(void)
{
	return ble_hid_mode ;
}

void app_adv_data_update(uint8_t *data,uint8_t length)
{
	if(!ble_calss.ble_connect_status())
	{
		ble_calss.ble_adv_pyload_update(data,length);
	}
}

void app_ble_mac_get(uint8_t *mac_buff)
{
	ble_calss.ble_mac_get(mac_buff);
}

void app_ble_mac_set(uint8_t *mac_buff)
{
	ble_calss.ble_mac_set(mac_buff);
}

void app_ble_conn_time_audio_set(void)
{
//	if(ble_calss.ble_connect_status())
//	{
//		ble_calss.ble_connect_params_update(BLE_CONN_PARAMS_AUDIO);
//	}
}

void app_ble_conn_time_audio_reset(void)
{
//	if(ble_calss.ble_connect_status())
//	{
//		ble_calss.ble_connect_params_update(BLE_CONN_PARAMS_FAST);
//	}
}

void app_ble_hid_phone_screen_set(uint32_t phone_screen_high,uint32_t phone_screen_width,char *phone_type_name,uint8_t phone_type_name_length)
{
	if(ble_calss.ble_connect_status())
	{
		ble_calss.ble_hid_phone_screen_set(phone_screen_high,phone_screen_width,phone_type_name,phone_type_name_length);
	}
}

void app_ble_hid_phone_screen_get(uint32_t *phone_screen_high,uint32_t *phone_screen_width,char *phone_type_name,uint8_t *phone_type_name_length)
{
	if(ble_calss.ble_connect_status())
	{
		ble_calss.ble_hid_phone_screen_get(phone_screen_high,phone_screen_width,phone_type_name,phone_type_name_length);
	}
}

void app_ble_adv_start(void)
{
	ble_calss.ble_adv_start();
}

void app_ble_adv_stop(void)
{
	ble_calss.ble_adv_stop();
}

/*******************************************************************************
 * Function Name     : app_ble_time_create
 * Description       : ble相关线程创建
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
void app_ble_handler_thread_create(void)
{	
	bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < BLE_TASK_TYPE_NUM; i++)
	{
		x_return  = bc_rtos_thread_create((TaskFunction_t )app_ble_thread[i].thread_task_code,     	
                                     (const char*    )app_ble_thread[i].thread_name,   	
                                     (uint16_t       )app_ble_thread[i].thread_stack_depth, 
                                     (void*          )&app_ble_thread[i].thread_parameters,				
                                     (UBaseType_t    )app_ble_thread[i].thread_priority,	
                                     (TaskHandle_t*  )&app_ble_thread[i].thread_handler); 
		if(x_return != NULL)
		{
			BC_LOG_INFO("create %s succeed \r\n",app_ble_thread[i].thread_name);
//      bc_rtos_thread_suspend(app_ble_thread[i].thread_handler);
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",app_ble_thread[i].thread_name);
		}	
	}
  app_ble_time_create();
  
  bc_ble_connect_callabck_register(app_ble_connect_callback);
  bc_ble_disconnect_callabck_register(app_ble_disconnect_callback);
  bc_ble_pm_connect_callabck_register(app_ble_pm_connect_callback);
  bc_ble_pm_disconnect_callabck_register(app_ble_pm_disconnect_callback);

#if defined(HANDWARE_1_23_4)
  bc_ble_connect_guard_register(app_ble_connect_guard_callback);
#endif

#if (defined(HANDWARE_1_19_1))  
  bc_led_pwm_stop_register_callback(app_ble_adv_light_start);
#endif  
//
  ble_calss = bc_ble_new();
}
















