#include "bc_ic_led.h"



#include "tx1812n5.h"
	



#include "bc_logger.h"
#include "bc_delay.h"
#include "bc_rtos.h"
#include "bc_ldo_switch.h"
#include "bc_device_info.h"

struct rgb_struct rgb_breathing_config = {0,0,0};

#if defined(HANDWARE_1_23_4) || defined(HANDWARE_1_23_2_ONE_SEC)
/* 呼吸灯状态 */
static bool s_breathing_running = false;
static enum bc_ic_led_breathing_mode s_breathing_mode = LED_BREATHING_FAST;
static uint8_t s_breathing_brightness = 0;   /* 当前亮度 0-255 */
static int8_t s_breathing_direction = 1;     /* 1=渐亮, -1=渐暗 */
static uint8_t s_breathing_r = 0;            /* 目标红色 */
static uint8_t s_breathing_g = 0;            /* 目标绿色 */
static uint8_t s_breathing_b = 0;            /* 目标蓝色 */
static uint8_t s_breathing_total_count = 0;  /* 总呼吸次数（0=无限循环） */
static uint8_t s_breathing_remain_count = 0; /* 剩余呼吸次数 */

/* 呼吸灯定时器 */
static void bc_ic_led_breathing_timer_callback(void *pvParameter);

static bc_rtos_timer_struct s_breathing_timer =
{
    .timer_name = "led breathing timer",
    .uxAutoReload = true,
    .xTimerPeriodInTicks = 30,   /* 30ms 步进 */
    .lock = false,
    .timer_callback_function = bc_ic_led_breathing_timer_callback,
};
#endif

#if defined(HANDWARE_1_23_4)
/* 电量指示颜色 */
static uint8_t s_battery_indication_g = 0;
static uint8_t s_battery_indication_r = 0;
static uint8_t s_battery_indication_b = 0;

/* 电量指示定时器（亮2秒后熄灭） */
static void bc_ic_led_battery_indication_timer_callback(void *pvParameter);

static bc_rtos_timer_struct s_battery_indication_timer =
{
    .timer_name = "battery indication timer",
    .uxAutoReload = false,      /* 一次性定时器 */
    .xTimerPeriodInTicks = 2000, /* 2秒 */
    .lock = false,
    .timer_callback_function = bc_ic_led_battery_indication_timer_callback,
};

/* 在线录音LED亮度渐变定时器（2秒后亮度降为30%） */
static void bc_ic_led_online_recording_dim_timer_callback(void *pvParameter);

static bc_rtos_timer_struct s_online_recording_dim_timer =
{
    .timer_name = "online recording dim timer",
    .uxAutoReload = false,      /* 一次性定时器 */
    .xTimerPeriodInTicks = 2000, /* 2秒 */
    .lock = false,
    .timer_callback_function = bc_ic_led_online_recording_dim_timer_callback,
};
#endif

#if defined(HANDWARE_1_23_2_ONE_SEC)
/* 文件同步完成后绿灯关闭定时器（亮1秒后熄灭） */
static void bc_ic_led_file_sync_green_off_timer_callback(void *pvParameter);

static bc_rtos_timer_struct s_file_sync_green_off_timer =
{
    .timer_name = "file sync green off timer",
    .uxAutoReload = false,      /* 一次性定时器 */
    .xTimerPeriodInTicks = 1000, /* 1秒 */
    .lock = false,
    .timer_callback_function = bc_ic_led_file_sync_green_off_timer_callback,
};
#endif

enum led_timer_event
{
	LED_BREATHING_TIMER_EVENT = 0,
	LED_BLE_IDIE_TIMER_EVENT,
	LED_POWER_LOW_TIMER_EVENT,
	LED_CONNECT_OFF_TIMER_EVENT,
	LED_TIMER_NUM
};


enum bc_ic_led_flag
{
	IC_LED_DISENABLE =0,
	IC_LED_ENABLE =1,
};
struct __attribute__((__packed__)) ic_led_status
{
	unsigned int ble_idie : 1;
	unsigned int ble_connect : 1;
	unsigned int power_low : 1;
	unsigned int chargeing : 1;
	unsigned int charge_over: 1;
	unsigned int ble_disconnect : 1;
	unsigned int mic_offline_reocrding : 1;
  unsigned int mic_online_reocrding : 1;
  unsigned int mic_offline_reocrding_capture : 1;
  unsigned int mic_online_reocrding_capture : 1;
#if defined(HANDWARE_1_23_4)
  unsigned int hold_recording : 1;       /* 长按录音状态 */
  unsigned int hold_recording_online : 1; /* 长按录音是否在线 */
  unsigned int battery_indication : 1;    /* 电量指示状态 */
  unsigned int online_recording_dim : 1;   /* 在线录音LED是否已变暗（30%亮度） */
  unsigned int recording_pause : 1;          /* 录音暂停状态 */
#else
  unsigned int : 5;
#endif
};


static struct ic_led_status led_status ={0};

#if defined(SUDO_VOICE_ONLY)
/* Latest synchronous recording indicator request; the LED task owns hardware writes. */
static volatile bool sudo_recording_indicator_desired = false;
/* Aligned word publication prevents a torn GRB request across tasks. */
static volatile uint32_t sudo_idle_rgb_desired;
#endif

enum BC_IC_LED_EVENT
{
#if defined(SUDO_VOICE_ONLY)
  IC_LED_SUDO_IDLE_COLOR_EVENT = (0x00000001 << 23),
#endif
  IC_LED_BLE_CONNECT_EVENT   = (0x00000001 << 0),
  IC_LED_BLE_DISCONNECT_EVENT = (0x00000001 << 1),
  IC_LED_MIC_OFFLINE_RECORDING_ON_EVENT = (0x00000001 << 2),
  IC_LED_MIC_ONLINE_RECORDING_ON_EVENT = (0x00000001 << 3),
  IC_LED_MIC_ONLINE_RECORDING_CAPTURE_ON_EVENT = (0x00000001 << 4),
  IC_LED_MIC_OFFLINE_RECORDING_CAPTURE_ON_EVENT = (0x00000001 << 5),
  IC_LED_MIC_OFFLINE_RECORDING_OFF_EVENT = (0x00000001 << 6),
  IC_LED_MIC_ONLINE_RECORDING_OFF_EVENT = (0x00000001 << 7),
  IC_LED_MIC_ONLINE_RECORDING_CAPTURE_OFF_EVENT = (0x00000001 << 8),
  IC_LED_MIC_OFFLINE_RECORDING_CAPTURE_OFF_EVENT = (0x00000001 << 9),
#if defined(HANDWARE_1_23_4) || defined(HANDWARE_1_23_2_ONE_SEC)
  IC_LED_BREATHING_UPDATE_EVENT = (0x00000001 << 17),
  IC_LED_BREATHING_START_EVENT = (0x00000001 << 18),
  IC_LED_BREATHING_STOP_EVENT = (0x00000001 << 19),
#endif
#if defined(HANDWARE_1_23_2_ONE_SEC)
  IC_LED_FILE_SYNC_ON_EVENT = (0x00000001 << 20),
  IC_LED_FILE_SYNC_STOP_EVENT = (0x00000001 << 21),
  IC_LED_FILE_SYNC_GREEN_OFF_EVENT = (0x00000001 << 22),
#endif
#if defined(HANDWARE_1_23_4)
  IC_LED_HOLD_RECORDING_ON_EVENT = (0x00000001 << 10),
  IC_LED_HOLD_RECORDING_OFF_EVENT = (0x00000001 << 11),
  IC_LED_BATTERY_INDICATION_ON_EVENT = (0x00000001 << 12),
  IC_LED_BATTERY_INDICATION_OFF_EVENT = (0x00000001 << 13),
  IC_LED_ONLINE_RECORDING_DIM_EVENT = (0x00000001 << 14),
  IC_LED_RECORDING_PAUSE_ON_EVENT = (0x00000001 << 15),
  IC_LED_RECORDING_PAUSE_OFF_EVENT = (0x00000001 << 16),
#endif
};

static bc_rtos_event_struct event_struct = {

  .event_name              = "ic led event",
  .event_clear_on_exit     = bc_pdTRUE,
  .event_wait_for_all_bits = bc_pdFALSE,
};


enum bc_ic_led_task_type
{
  IC_LED_TASK_TYPE = 0,
	IC_LED_TASK_TYPE_NUM
};

static void bc_ic_led_handler_thread(void *thread_handler);

static bc_rtos_thread_struct thread_struct[IC_LED_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "ic led task",
                                                                      .thread_stack_depth   = BC_IC_LED_STACK_SIZE ,
                                                                      .thread_priority      = BC_IC_LED__PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = bc_ic_led_handler_thread,
                                                                    },	                                                                    
                                                                  };

#if defined(HANDWARE_1_23_2)
/*******************************************************************************
 * Function Name     : bc_ic_led_color_value_get
 * Description       : 根据配置颜色值获取RGB分量
 * Input             : color - 0x01蓝, 0x02红, 0x03绿
 * Output            : g, r, b
 * Return            :
 *******************************************************************************/
static void bc_ic_led_color_value_get(uint8_t color, uint8_t *g, uint8_t *r, uint8_t *b)
{
    *g = 0;
    *r = 0;
    *b = 0;
    switch(color)
    {
        case 0x01:  /* 蓝 */
        {
            *b = 20;
            break;
        }
        case 0x02:  /* 红 */
        {
            *r = 20;
            break;
        }
        case 0x03:  /* 绿 */
        {
            *g = 20;
            break;
        }
        default:
        {
            *b = 20;  /* 默认蓝色 */
            break;
        }
    }
}
#endif

static void bc_ic_led_rgb_set(uint8_t rgb_g,uint8_t rgb_r,uint8_t rgb_b,uint8_t num)
{
  struct rgb_struct rgb_config = {.rgb_g = 0,.rgb_r = 0,.rgb_b =0};
  bc_ldo_rgb_power_on();
  #if 0 // liukun 20260518
	bc_delay_ms(10);
  #else
    bc_delay_ms(20);
  #endif
  rgb_config.rgb_b = rgb_b;
  rgb_config.rgb_g = rgb_g;
  rgb_config.rgb_r = rgb_r;
  tx1812n5_RGB(&rgb_config ,1);
}

static void bc_ic_led_rgb_clear(void)
{
  struct rgb_struct rgb_config = {.rgb_g = 0,.rgb_r = 0,.rgb_b =0}; 
  tx1812n5_RGB(&rgb_config ,1);
  bc_delay_ms(20);
  bc_ldo_rgb_power_off();
}  
                                                                  
#if defined(HANDWARE_1_23_2_ONE_SEC)
static void bc_ic_led_file_sync_green_off_timer_callback(void *pvParameter)
{
    /* 发送绿灯关闭事件，由LED线程统一处理 */
    bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_FILE_SYNC_GREEN_OFF_EVENT);
}
#endif

static void bc_ic_led_handler_thread(void *thread_handler)
{
  bc_event_bits event_bits = 0;
  tx1812n5_rgb_init();
  bc_ic_led_rgb_set(0,0,0,1);
  bc_ic_led_rgb_clear();
  while(true)
  {
    event_bits = bc_rtos_event_group_wait_bits(event_struct.event_handler,
                                                IC_LED_BLE_CONNECT_EVENT | IC_LED_BLE_DISCONNECT_EVENT | \
                                                IC_LED_MIC_OFFLINE_RECORDING_ON_EVENT | IC_LED_MIC_ONLINE_RECORDING_ON_EVENT | \
                                                IC_LED_MIC_ONLINE_RECORDING_CAPTURE_ON_EVENT | IC_LED_MIC_OFFLINE_RECORDING_CAPTURE_ON_EVENT | \
                                                IC_LED_MIC_OFFLINE_RECORDING_OFF_EVENT | IC_LED_MIC_ONLINE_RECORDING_OFF_EVENT | \
                                                IC_LED_MIC_ONLINE_RECORDING_CAPTURE_OFF_EVENT | IC_LED_MIC_OFFLINE_RECORDING_CAPTURE_OFF_EVENT
#if defined(SUDO_VOICE_ONLY)
                                                 | IC_LED_SUDO_IDLE_COLOR_EVENT
#endif
#if defined(HANDWARE_1_23_4)
                                                 | IC_LED_HOLD_RECORDING_ON_EVENT | IC_LED_HOLD_RECORDING_OFF_EVENT
                                                 | IC_LED_BATTERY_INDICATION_ON_EVENT | IC_LED_BATTERY_INDICATION_OFF_EVENT
                                                 | IC_LED_ONLINE_RECORDING_DIM_EVENT
                                                 | IC_LED_RECORDING_PAUSE_ON_EVENT | IC_LED_RECORDING_PAUSE_OFF_EVENT
#endif
#if defined(HANDWARE_1_23_4) || defined(HANDWARE_1_23_2_ONE_SEC)
                                                 | IC_LED_BREATHING_UPDATE_EVENT
                                                 | IC_LED_BREATHING_START_EVENT | IC_LED_BREATHING_STOP_EVENT
#endif
#if defined(HANDWARE_1_23_2_ONE_SEC)
                                                 | IC_LED_FILE_SYNC_ON_EVENT | IC_LED_FILE_SYNC_STOP_EVENT
                                                 | IC_LED_FILE_SYNC_GREEN_OFF_EVENT
#endif
                                                ,
                                                event_struct.event_clear_on_exit,event_struct.event_wait_for_all_bits,bc_rtos_max_delay);
#if defined(SUDO_VOICE_ONLY)
    /* Only this task drives the LED. A queued legacy/off/online event cannot
     * override the latest recording indicator, or replay an old idle color
     * after Stop. Candidate connection notifications are deliberately quiet. */
    if (event_bits & (IC_LED_MIC_OFFLINE_RECORDING_ON_EVENT |
                     IC_LED_MIC_OFFLINE_RECORDING_OFF_EVENT |
                     IC_LED_SUDO_IDLE_COLOR_EVENT))
    {
      led_status.mic_offline_reocrding = sudo_recording_indicator_desired;
      if (sudo_recording_indicator_desired) bc_ic_led_rgb_set(5, 0, 0, 1);
      else if (event_bits & IC_LED_MIC_OFFLINE_RECORDING_OFF_EVENT) bc_ic_led_rgb_clear();
      else if (event_bits & IC_LED_SUDO_IDLE_COLOR_EVENT)
      {
        uint32_t color = sudo_idle_rgb_desired;
        if (color == 0) bc_ic_led_rgb_clear();
        else bc_ic_led_rgb_set((uint8_t)color, (uint8_t)(color >> 8), (uint8_t)(color >> 16), 1);
      }
    }
    continue;
#endif
    if((event_bits & IC_LED_BLE_CONNECT_EVENT) == IC_LED_BLE_CONNECT_EVENT)
    {
      led_status.ble_connect = IC_LED_ENABLE;
#if defined(HANDWARE_1_23_2)
      /* HANDWARE_1_23_2: 根据配置颜色显示蓝牙连接提示，常亮2秒 */
      {
        bc_device_led_motor_mode_info info = {0};
        bc_device_info_led_motor_mode_info_get(&info);
        uint8_t g = 0, r = 0, b = 0;
        bc_ic_led_color_value_get(info.ble_connect_color, &g, &r, &b);
        bc_ic_led_rgb_set(g, r, b, 1);
        bc_delay_ms(1000 * 2);
        bc_ic_led_rgb_clear();
      }
#else
      bc_ic_led_rgb_set(0,0,20,1);
      bc_delay_ms(1000*3);
      bc_ic_led_rgb_clear();
#endif
      led_status.ble_connect = IC_LED_DISENABLE;


#if defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_2_ONE_SEC)
      // 蓝牙连接提示结束后，如果正在在线录音，恢复紫色常亮
      if(led_status.mic_online_reocrding)
      {
        bc_ic_led_rgb_set(0,20,20,1);
      }
      else if(led_status.mic_offline_reocrding)
      {
#if defined(HANDWARE_1_23_2_ONE_SEC)
        // 蓝牙连接提示结束后，如果正在离线录音，恢复红灯慢闪
        bc_ic_led_breathing_start(LED_BREATHING_SLOW, 0, 0, 20, 0);
#else
        // 蓝牙连接提示结束后，如果正在离线录音，恢复绿色常亮
        bc_ic_led_rgb_set(5,0,0,1);
#endif
      }
#elif defined(HANDWARE_1_23_2)
      // 蓝牙连接提示结束后，如果正在录音，恢复配置颜色常亮
      if(led_status.mic_online_reocrding || led_status.mic_offline_reocrding)
      {
        bc_device_led_motor_mode_info info = {0};
        bc_device_info_led_motor_mode_info_get(&info);
        uint8_t g = 0, r = 0, b = 0;
        bc_ic_led_color_value_get(info.recording_color, &g, &r, &b);
        bc_ic_led_rgb_set(g, r, b, 1);
      }
#endif

      BC_LOG_INFO("IC_LED_BLE_CONNECT_EVENT \r\n");
    }
    if((event_bits & IC_LED_BLE_DISCONNECT_EVENT) == IC_LED_BLE_DISCONNECT_EVENT)
    {
      led_status.ble_disconnect = IC_LED_ENABLE;
#if defined(HANDWARE_1_23_2)
      /* HANDWARE_1_23_2: 根据配置颜色显示蓝牙断开提示，闪烁2次 */
      {
        bc_device_led_motor_mode_info info = {0};
        bc_device_info_led_motor_mode_info_get(&info);
        uint8_t g = 0, r = 0, b = 0;
        bc_ic_led_color_value_get(info.ble_disconnect_color, &g, &r, &b);
        for(uint8_t i = 0; i < 2; i++)
        {
          bc_ic_led_rgb_set(g, r, b, 1);
          bc_delay_ms(500 * 1);
          bc_ic_led_rgb_clear();
          bc_delay_ms(500 * 1);
        }
      }
#else
      for(uint8_t i = 0;i<3;i++)
      {
        bc_ic_led_rgb_set(0,0,20,1);
        bc_delay_ms(500*1);
        bc_ic_led_rgb_clear();
        bc_delay_ms(500*1);
      }
#endif
      //bc_ic_led_rgb_clear();
      led_status.ble_disconnect = IC_LED_DISENABLE; 
      if(led_status.mic_offline_reocrding)
      {
        
      }

#if defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_2_ONE_SEC)
      // 蓝牙断开提示结束后，如果正在在线录音，恢复紫色常亮
      if(led_status.mic_online_reocrding)
      {
        bc_ic_led_rgb_set(0,20,20,1);
      }
      else if (led_status.mic_offline_reocrding)
      {
#if defined(HANDWARE_1_23_2_ONE_SEC)
          bc_ic_led_breathing_start(LED_BREATHING_SLOW, 0, 0, 20, 0);
#else
          bc_ic_led_rgb_set(5,0,0,1);
#endif
      }
#elif defined(HANDWARE_1_23_2)
      // 蓝牙断开提示结束后，如果正在录音，恢复配置颜色常亮
      if(led_status.mic_online_reocrding || led_status.mic_offline_reocrding)
      {
        bc_device_led_motor_mode_info info = {0};
        bc_device_info_led_motor_mode_info_get(&info);
        uint8_t g = 0, r = 0, b = 0;
        bc_ic_led_color_value_get(info.recording_color, &g, &r, &b);
        bc_ic_led_rgb_set(g, r, b, 1);
      }
#endif
      BC_LOG_INFO("IC_LED_BLE_DISCONNECT_EVENT \r\n");
    }
    if((event_bits & IC_LED_MIC_OFFLINE_RECORDING_ON_EVENT) == IC_LED_MIC_OFFLINE_RECORDING_ON_EVENT)
    {
      BC_LOG_INFO("IC_LED_MIC_OFFLINE_RECORDING_ON_EVENT \r\n");
      led_status.mic_offline_reocrding = IC_LED_ENABLE;
#if defined(SUDO_VOICE_ONLY)
      if (sudo_recording_indicator_desired)
      {
        led_status.mic_offline_reocrding = IC_LED_ENABLE;
        /* SUDO recording indicator is always green (GRB 5,0,0). */
        bc_ic_led_rgb_set(5, 0, 0, 1);
      }
      else
      {
        led_status.mic_offline_reocrding = IC_LED_DISENABLE;
      }
#elif defined(HANDWARE_1_23_2)
      #if defined(HANDWARE_1_23_2_ONE_SEC)
        /* HANDWARE_1_23_2_ONE_SEC: 红灯慢闪替代绿灯常亮 */
        bc_ic_led_breathing_start(LED_BREATHING_SLOW, 0, 0, 20, 0);
      #else
      /* HANDWARE_1_23_2: 根据配置颜色显示录音LED */
      {
        bc_device_led_motor_mode_info info = {0};
        bc_device_info_led_motor_mode_info_get(&info);
        uint8_t g = 0, r = 0, b = 0;
        bc_ic_led_color_value_get(info.recording_color, &g, &r, &b);
        bc_ic_led_rgb_set(g, r, b, 1);
      }
    #endif
#else
#if defined(HANDWARE_1_23_4)
      /* HANDWARE_1_23_4: 长按录音期间，普通录音LED不改变显示（长按录音优先级更高） */
      if(!led_status.hold_recording)
#endif
      {
        bc_ic_led_rgb_set(5,0,0,1);
#if defined(HANDWARE_1_23_4)
        /* HANDWARE_1_23_4: 清除dim状态，启动2秒定时器，时间到后亮度降为30% */
        led_status.online_recording_dim = IC_LED_DISENABLE;
        if(s_online_recording_dim_timer.timer_handler == NULL)
        {
            s_online_recording_dim_timer.timer_handler = bc_rtos_timer_create(
                s_online_recording_dim_timer.timer_name,
                s_online_recording_dim_timer.xTimerPeriodInTicks,
                s_online_recording_dim_timer.uxAutoReload,
                (void *)0,
                s_online_recording_dim_timer.timer_callback_function);
        }
        if(s_online_recording_dim_timer.timer_handler != NULL)
        {
            bc_rtos_timer_start(s_online_recording_dim_timer.timer_handler, 100);
        }
#endif
      }
#endif
    }
    if((event_bits & IC_LED_MIC_ONLINE_RECORDING_ON_EVENT) == IC_LED_MIC_ONLINE_RECORDING_ON_EVENT)
    {
      BC_LOG_INFO("IC_LED_MIC_ONLINE_RECORDING_ON_EVENT \r\n");
      led_status.mic_online_reocrding = IC_LED_ENABLE;
#if (defined(HANDWARE_1_23_2) && !defined(HANDWARE_1_23_2_ONE_SEC))
      /* HANDWARE_1_23_2: 根据配置颜色显示录音LED */
      {
        bc_device_led_motor_mode_info info = {0};
        bc_device_info_led_motor_mode_info_get(&info);
        uint8_t g = 0, r = 0, b = 0;
        bc_ic_led_color_value_get(info.recording_color, &g, &r, &b);
        bc_ic_led_rgb_set(g, r, b, 1);
      }
#elif defined(HANDWARE_1_23_4)
      /* HANDWARE_1_23_4: 长按录音期间，普通录音LED不改变显示（长按录音优先级更高） */
      if(!led_status.hold_recording)
      {
        bc_ic_led_rgb_set(0,20,20,1);
        /* HANDWARE_1_23_4: 清除dim状态，启动2秒定时器，时间到后亮度降为30% */
        led_status.online_recording_dim = IC_LED_DISENABLE;
        if(s_online_recording_dim_timer.timer_handler == NULL)
        {
            s_online_recording_dim_timer.timer_handler = bc_rtos_timer_create(
                s_online_recording_dim_timer.timer_name,
                s_online_recording_dim_timer.xTimerPeriodInTicks,
                s_online_recording_dim_timer.uxAutoReload,
                (void *)0,
                s_online_recording_dim_timer.timer_callback_function);
        }
        if(s_online_recording_dim_timer.timer_handler != NULL)
        {
            bc_rtos_timer_start(s_online_recording_dim_timer.timer_handler, 100);
        }
      }
#else
      {
        bc_ic_led_rgb_set(0,20,20,1);
      }
#endif
    }
    if((event_bits & IC_LED_MIC_ONLINE_RECORDING_CAPTURE_ON_EVENT ) == IC_LED_MIC_ONLINE_RECORDING_CAPTURE_ON_EVENT )
    {
      BC_LOG_INFO("IC_LED_MIC_ONLINE_RECORDING_CAPTURE_ON_EVENT  \r\n");
      led_status.mic_online_reocrding_capture = IC_LED_ENABLE;
#if (defined(HANDWARE_1_23_2) && !defined(HANDWARE_1_23_2_ONE_SEC))
      /* HANDWARE_1_23_2: 根据配置颜色显示录音LED */
      {
        bc_device_led_motor_mode_info info = {0};
        bc_device_info_led_motor_mode_info_get(&info);
        uint8_t g = 0, r = 0, b = 0;
        bc_ic_led_color_value_get(info.recording_color, &g, &r, &b);
        bc_ic_led_rgb_set(g, r, b, 1);
      }
#elif defined(HANDWARE_1_23_4)
      /* HANDWARE_1_23_4: 长按录音期间，普通录音LED不改变显示（长按录音优先级更高） */
      if(!led_status.hold_recording)
      {
        bc_ic_led_rgb_set(5,0,0,1);
        /* HANDWARE_1_23_4: 清除dim状态，启动2秒定时器，时间到后亮度降为30% */
        led_status.online_recording_dim = IC_LED_DISENABLE;
        if(s_online_recording_dim_timer.timer_handler == NULL)
        {
            s_online_recording_dim_timer.timer_handler = bc_rtos_timer_create(
                s_online_recording_dim_timer.timer_name,
                s_online_recording_dim_timer.xTimerPeriodInTicks,
                s_online_recording_dim_timer.uxAutoReload,
                (void *)0,
                s_online_recording_dim_timer.timer_callback_function);
        }
        if(s_online_recording_dim_timer.timer_handler != NULL)
        {
            bc_rtos_timer_start(s_online_recording_dim_timer.timer_handler, 100);
        }
      }
#else
      {
        bc_ic_led_rgb_set(5,0,0,1);
      }
#endif
    }
    if((event_bits & IC_LED_MIC_OFFLINE_RECORDING_CAPTURE_ON_EVENT) == IC_LED_MIC_OFFLINE_RECORDING_CAPTURE_ON_EVENT)
    {
      BC_LOG_INFO("IC_LED_MIC_OFFLINE_RECORDING_CAPTURE_ON_EVENT \r\n");
    }  
    if((event_bits & IC_LED_MIC_OFFLINE_RECORDING_OFF_EVENT) == IC_LED_MIC_OFFLINE_RECORDING_OFF_EVENT)
    {
      BC_LOG_INFO("IC_LED_MIC_OFFLINE_RECORDING_OFF_EVENT \r\n");
      led_status.mic_offline_reocrding = IC_LED_DISENABLE;
#if defined(SUDO_VOICE_ONLY)
      led_status.mic_offline_reocrding = sudo_recording_indicator_desired ? IC_LED_ENABLE : IC_LED_DISENABLE;
      if (!sudo_recording_indicator_desired)
      {
        /* Clear only when the latest synchronous request is stop. */
        bc_ic_led_rgb_clear();
      }
#elif (defined(HANDWARE_1_23_2) && !defined(HANDWARE_1_23_2_ONE_SEC))
      /* HANDWARE_1_23_2: 录音LED为常亮模式，直接清除 */
      bc_ic_led_rgb_clear();
#elif defined(HANDWARE_1_23_2_ONE_SEC)
      /* HANDWARE_1_23_2_ONE_SEC: 停止红灯慢闪 */
      bc_ic_led_breathing_stop();
#else
      bc_ic_led_rgb_clear();
#if defined(HANDWARE_1_23_4)
      /* HANDWARE_1_23_4: 清除dim状态，停止亮度渐变定时器 */
      led_status.online_recording_dim = IC_LED_DISENABLE;
      if(s_online_recording_dim_timer.timer_handler != NULL)
      {
          bc_rtos_timer_stop(s_online_recording_dim_timer.timer_handler, 100);
      }
      /* HANDWARE_1_23_4: 长按录音期间，不清除LED（长按录音LED继续显示） */
      if(!led_status.hold_recording)
#endif
      {
        bc_ic_led_rgb_clear();
      }
#endif
    }
    if((event_bits & IC_LED_MIC_ONLINE_RECORDING_OFF_EVENT) == IC_LED_MIC_ONLINE_RECORDING_OFF_EVENT)
    {
      BC_LOG_INFO("IC_LED_MIC_ONLINE_RECORDING_OFF_EVENT \r\n");
      bc_ic_led_rgb_clear();
      led_status.mic_online_reocrding = IC_LED_DISENABLE; 
#if defined(HANDWARE_1_23_4)
      /* HANDWARE_1_23_4: 清除dim状态，停止亮度渐变定时器 */
      led_status.online_recording_dim = IC_LED_DISENABLE;
      if(s_online_recording_dim_timer.timer_handler != NULL)
      {
          bc_rtos_timer_stop(s_online_recording_dim_timer.timer_handler, 100);
      }
      /* HANDWARE_1_23_4: 长按录音期间，不清除LED（长按录音LED继续显示） */
      if(!led_status.hold_recording)
#endif
      {
        bc_ic_led_rgb_clear();
      }
    }
    if((event_bits & IC_LED_MIC_ONLINE_RECORDING_CAPTURE_OFF_EVENT ) == IC_LED_MIC_ONLINE_RECORDING_CAPTURE_OFF_EVENT )
    {
      BC_LOG_INFO("IC_LED_MIC_ONLINE_RECORDING_CAPTURE_OFF_EVENT  \r\n");
      bc_ic_led_rgb_clear();
      led_status.mic_online_reocrding_capture = IC_LED_DISENABLE; 
#if defined(HANDWARE_1_23_4)
      /* HANDWARE_1_23_4: 清除dim状态，停止亮度渐变定时器 */
      led_status.online_recording_dim = IC_LED_DISENABLE;
      if(s_online_recording_dim_timer.timer_handler != NULL)
      {
          bc_rtos_timer_stop(s_online_recording_dim_timer.timer_handler, 100);
      }
      /* HANDWARE_1_23_4: 长按录音期间，不清除LED（长按录音LED继续显示） */
      if(!led_status.hold_recording)
#endif
      {
        bc_ic_led_rgb_clear();
      }
    }
    if((event_bits & IC_LED_MIC_OFFLINE_RECORDING_CAPTURE_OFF_EVENT) == IC_LED_MIC_OFFLINE_RECORDING_CAPTURE_OFF_EVENT)
    {
      BC_LOG_INFO("IC_LED_MIC_OFFLINE_RECORDING_CAPTURE_OFF_EVENT \r\n");
      led_status.mic_offline_reocrding_capture = IC_LED_DISENABLE; 
#if defined(HANDWARE_1_23_4)
      /* HANDWARE_1_23_4: 长按录音期间，不清除LED（长按录音LED继续显示） */
      if(!led_status.hold_recording)
#endif
      {
        bc_ic_led_rgb_clear();
      }
    }
#if defined(HANDWARE_1_23_4)
    if((event_bits & IC_LED_HOLD_RECORDING_ON_EVENT) == IC_LED_HOLD_RECORDING_ON_EVENT)
    {
      BC_LOG_INFO("IC_LED_HOLD_RECORDING_ON_EVENT, online=%d\r\n", led_status.hold_recording_online);
      led_status.hold_recording = IC_LED_ENABLE;
      if(led_status.hold_recording_online)
      {
        /* 在线长按录音：绿灯 */
        bc_ic_led_rgb_set(20, 0, 0, 1);  // G=20, R=0, B=0
      }
      else
      {
        /* 离线长按录音：紫灯 */
        bc_ic_led_rgb_set(0, 20, 20, 1);  // G=0, R=20, B=20
      }
    }
    if((event_bits & IC_LED_HOLD_RECORDING_OFF_EVENT) == IC_LED_HOLD_RECORDING_OFF_EVENT)
    {
      bool was_online = led_status.hold_recording_online;
      BC_LOG_INFO("IC_LED_HOLD_RECORDING_OFF_EVENT, was_online=%d\r\n", was_online);
      led_status.hold_recording = IC_LED_DISENABLE;
      /* 长按录音结束，恢复其他LED状态 */
      if(led_status.mic_online_reocrding_capture)
      {
        /* 根据dim状态决定恢复的亮度 */
        if(led_status.online_recording_dim)
        {
          bc_ic_led_rgb_set(2, 0, 0, 1);  // 30%亮度（绿灯）
        }
        else
        {
          bc_ic_led_rgb_set(5, 0, 0, 1);  // 100%亮度（绿灯）
        }
      }
      else if(led_status.mic_online_reocrding)
      {
        /* 如果是在线长按录音结束，不恢复在线录音LED（录音会停止），避免闪一下 */
        if(!was_online)
        {
          /* 根据dim状态决定恢复的亮度 */
          if(led_status.online_recording_dim)
          {
            bc_ic_led_rgb_set(0, 6, 6, 1);  // 30%亮度
          }
          else
          {
            bc_ic_led_rgb_set(0, 20, 20, 1);  // 100%亮度
          }
        }
        else
        {
          bc_ic_led_rgb_clear();
        }
      }
      else if(led_status.mic_offline_reocrding)
      {
        /* 如果是离线长按录音结束，不恢复离线录音LED（录音会停止），避免闪一下绿灯 */
        if(was_online)
        {
          /* 根据dim状态决定恢复的亮度 */
          if(led_status.online_recording_dim)
          {
            bc_ic_led_rgb_set(2, 0, 0, 1);  // 30%亮度（绿灯）
          }
          else
          {
            bc_ic_led_rgb_set(5, 0, 0, 1);  // 100%亮度（绿灯）
          }
        }
        else
        {
          bc_ic_led_rgb_clear();
        }
      }
      else
      {
        bc_ic_led_rgb_clear();
      }
    }
    if((event_bits & IC_LED_BATTERY_INDICATION_ON_EVENT) == IC_LED_BATTERY_INDICATION_ON_EVENT)
    {
      BC_LOG_INFO("IC_LED_BATTERY_INDICATION_ON_EVENT, g=%d r=%d b=%d\r\n",
                  s_battery_indication_g, s_battery_indication_r, s_battery_indication_b);
      led_status.battery_indication = IC_LED_ENABLE;
      /* 点亮对应颜色的LED */
      bc_ic_led_rgb_set(s_battery_indication_g, s_battery_indication_r, s_battery_indication_b, 1);
        /* 初始化定时器（如果还没创建） */
        if(s_battery_indication_timer.timer_handler == NULL)
        {
            s_battery_indication_timer.timer_handler = bc_rtos_timer_create(
                s_battery_indication_timer.timer_name,
                s_battery_indication_timer.xTimerPeriodInTicks,
                s_battery_indication_timer.uxAutoReload,
                (void *)0,
                s_battery_indication_timer.timer_callback_function);
        }
        else
        {
            bc_rtos_timer_reset(s_battery_indication_timer.timer_handler, 50);
        }
    }
    if((event_bits & IC_LED_BATTERY_INDICATION_OFF_EVENT) == IC_LED_BATTERY_INDICATION_OFF_EVENT)
    {
      BC_LOG_INFO("IC_LED_BATTERY_INDICATION_OFF_EVENT\r\n");
      led_status.battery_indication = IC_LED_DISENABLE;
      /* 停止定时器 */
      if(s_battery_indication_timer.timer_handler != NULL)
      {
        bc_rtos_timer_stop(s_battery_indication_timer.timer_handler, 50);
      }
      /* 电量指示结束，恢复其他LED状态 */
      if(led_status.hold_recording)
      {
        if(led_status.hold_recording_online)
        {
          bc_ic_led_rgb_set(20, 0, 0, 1);  // 在线长按录音：绿灯
        }
        else
        {
          bc_ic_led_rgb_set(0, 20, 20, 1);  // 离线长按录音：紫灯
        }
      }
      else if(led_status.mic_online_reocrding_capture)
      {
        /* 根据dim状态决定恢复的亮度 */
        if(led_status.online_recording_dim)
        {
          bc_ic_led_rgb_set(2, 0, 0, 1);  // 30%亮度（绿灯）
        }
        else
        {
          bc_ic_led_rgb_set(5, 0, 0, 1);  // 100%亮度（绿灯）
        }
      }
      else if(led_status.mic_online_reocrding)
      {
        /* 根据dim状态决定恢复的亮度 */
        if(led_status.online_recording_dim)
        {
          bc_ic_led_rgb_set(0, 6, 6, 1);  // 30%亮度
        }
        else
        {
          bc_ic_led_rgb_set(0, 20, 20, 1);  // 100%亮度
        }
      }
      else if(led_status.mic_offline_reocrding)
      {
        /* 根据dim状态决定恢复的亮度 */
        if(led_status.online_recording_dim)
        {
          bc_ic_led_rgb_set(2, 0, 0, 1);  // 30%亮度（绿灯）
        }
        else
        {
          bc_ic_led_rgb_set(5, 0, 0, 1);  // 100%亮度（绿灯）
        }
      }
      else
      {
        bc_ic_led_rgb_clear();
      }
    }
    if((event_bits & IC_LED_ONLINE_RECORDING_DIM_EVENT) == IC_LED_ONLINE_RECORDING_DIM_EVENT)
    {
      BC_LOG_INFO("IC_LED_ONLINE_RECORDING_DIM_EVENT\r\n");
      /* 录音2秒后，亮度降为30% */
      led_status.online_recording_dim = IC_LED_ENABLE;
      /* 暂停状态下不改变LED颜色，保持黄灯呼吸 */
      if(!led_status.recording_pause && !led_status.hold_recording && !led_status.battery_indication)
      {
        if(led_status.mic_online_reocrding_capture)
        {
          /* capture在线录音（绿灯）：G=5 * 30% ≈ 2 */
          bc_ic_led_rgb_set(2, 0, 0, 1);
        }
        else if(led_status.mic_online_reocrding)
        {
          /* 普通在线录音（紫灯）：20 * 30% = 6 */
          bc_ic_led_rgb_set(0, 6, 6, 1);
        }
        else if(led_status.mic_offline_reocrding)
        {
          /* 离线录音（绿灯）：G=5 * 30% ≈ 2 */
          bc_ic_led_rgb_set(2, 0, 0, 1);
        }
      }
    }
    if((event_bits & IC_LED_RECORDING_PAUSE_ON_EVENT) == IC_LED_RECORDING_PAUSE_ON_EVENT)
    {
      BC_LOG_INFO("IC_LED_RECORDING_PAUSE_ON_EVENT\r\n");
      led_status.recording_pause = IC_LED_ENABLE;
      /* 停止亮度渐变定时器，避免暂停期间触发导致颜色跳动 */
      if(s_online_recording_dim_timer.timer_handler != NULL)
      {
          bc_rtos_timer_stop(s_online_recording_dim_timer.timer_handler, 100);
      }
      /* 暂停结束后直接恢复30%亮度，所以设置dim状态为已启用 */
      led_status.online_recording_dim = IC_LED_ENABLE;
      /* 启动黄灯慢闪呼吸灯效果（无限循环） */
      bc_ic_led_breathing_start(LED_BREATHING_SLOW, 0, 20, 20, 0);
    }
    if((event_bits & IC_LED_RECORDING_PAUSE_OFF_EVENT) == IC_LED_RECORDING_PAUSE_OFF_EVENT)
    {
      BC_LOG_INFO("IC_LED_RECORDING_PAUSE_OFF_EVENT\r\n");
      led_status.recording_pause = IC_LED_DISENABLE;
      /* 停止呼吸灯 */
      bc_ic_led_breathing_stop();
      /* 恢复30%亮度的绿灯 */
      if(led_status.mic_online_reocrding_capture)
      {
        /* capture在线录音（绿灯）：30%亮度 */
        bc_ic_led_rgb_set(2, 0, 0, 1);
      }
      else if(led_status.mic_online_reocrding)
      {
        /* 普通在线录音（紫灯）：30%亮度 */
        bc_ic_led_rgb_set(0, 6, 6, 1);
      }
      else if(led_status.mic_offline_reocrding)
      {
        /* 离线录音（绿灯）：30%亮度 */
        bc_ic_led_rgb_set(2, 0, 0, 1);
      }
    }
#endif /* HANDWARE_1_23_4 */
#if defined(HANDWARE_1_23_4) || defined(HANDWARE_1_23_2_ONE_SEC)
    if((event_bits & IC_LED_BREATHING_START_EVENT) == IC_LED_BREATHING_START_EVENT)
    {
      BC_LOG_INFO("IC_LED_BREATHING_START_EVENT\r\n");
      /* 呼吸灯启动：在LED线程中统一处理，避免并发访问导致数据错误 */

      /* 如果已在运行，先停止定时器 */
      if(s_breathing_running)
      {
          if(s_breathing_timer.timer_handler != NULL)
          {
              bc_rtos_timer_stop(s_breathing_timer.timer_handler, 50);
          }
      }

      /* 重置亮度和方向 */
      s_breathing_brightness = 0;
      s_breathing_direction = 1;

      /* 初始化定时器（如果还没创建） */
      if(s_breathing_timer.timer_handler == NULL)
      {
          s_breathing_timer.timer_handler = bc_rtos_timer_create(
              s_breathing_timer.timer_name,
              s_breathing_timer.xTimerPeriodInTicks,
              s_breathing_timer.uxAutoReload,
              (void *)0,
              s_breathing_timer.timer_callback_function);
      }

      if(s_breathing_timer.timer_handler != NULL)
      {
          s_breathing_running = true;
          /* 开启RGB电源并等待稳定 */
          bc_ldo_rgb_power_on();
          bc_delay_ms(20);
          bc_rtos_timer_start(s_breathing_timer.timer_handler, 50);
      }
      else
      {
          BC_LOG_ERROR("breathing start: timer create fail\r\n");
      }
    }
    if((event_bits & IC_LED_BREATHING_STOP_EVENT) == IC_LED_BREATHING_STOP_EVENT)
    {
      BC_LOG_INFO("IC_LED_BREATHING_STOP_EVENT\r\n");
      /* 呼吸灯停止：在LED线程中统一处理，避免并发访问导致数据错误 */

      if(!s_breathing_running)
      {
          /* 已经停止，直接返回 */
      }
      else
      {
          /* 停止定时器 */
          if(s_breathing_timer.timer_handler != NULL)
          {
              bc_rtos_timer_stop(s_breathing_timer.timer_handler, 50);
          }

          /* 关闭LED */
          struct rgb_struct rgb_config = {.rgb_g = 0, .rgb_r = 0, .rgb_b = 0};
          tx1812n5_RGB(&rgb_config, 1);
          bc_ldo_rgb_power_off();

          /* 重置状态 */
          s_breathing_running = false;
          s_breathing_brightness = 0;
          s_breathing_direction = 1;
          s_breathing_total_count = 0;
          s_breathing_remain_count = 0;
      }
    }
    if((event_bits & IC_LED_BREATHING_UPDATE_EVENT) == IC_LED_BREATHING_UPDATE_EVENT)
    {
      //BC_LOG_INFO("IC_LED_BREATHING_UPDATE_EVENT\r\n");
      /* 呼吸灯亮度更新：在LED线程中统一处理，避免并发访问导致数据错误 */
      uint8_t step;
      uint8_t g, r, b;

      /* 根据模式计算步长 */
      if(s_breathing_mode == LED_BREATHING_FAST)
      {
          step = 31;  /* 快闪：约0.5秒一个周期（30ms步进） */
      }
      else
      {
          step = 8;   /* 慢闪：约2秒一个周期（30ms步进） */
      }

      /* 更新亮度 */
      if(s_breathing_direction == 1)
      {
          /* 渐亮 */
          if(s_breathing_brightness + step >= 255)
          {
              s_breathing_brightness = 255;
              s_breathing_direction = -1;
          }
          else
          {
              s_breathing_brightness += step;
          }
      }
      else
      {
          /* 渐暗 */
          if(s_breathing_brightness <= step)
          {
              s_breathing_brightness = 0;
              s_breathing_direction = 1;

              /* 一个完整呼吸循环结束（0→255→0） */
              if(s_breathing_total_count > 0)
              {
                  s_breathing_remain_count--;
                  if(s_breathing_remain_count == 0)
                  {
                      /* 达到设定次数，停止呼吸灯 */
                      bc_rtos_timer_stop(s_breathing_timer.timer_handler, 50);
                      /* 关闭LED */
                      struct rgb_struct rgb_stop_config = {.rgb_g = 0, .rgb_r = 0, .rgb_b = 0};
                      tx1812n5_RGB(&rgb_stop_config, 1);
                      bc_ldo_rgb_power_off();
                      s_breathing_running = false;
                      s_breathing_brightness = 0;
                      s_breathing_direction = 1;
                      s_breathing_total_count = 0;
                      return;
                  }
              }
          }
          else
          {
              s_breathing_brightness -= step;
          }
      }

      /* 按亮度比例计算RGB值 */
      g = (uint8_t)((uint16_t)s_breathing_g * s_breathing_brightness / 255);
      r = (uint8_t)((uint16_t)s_breathing_r * s_breathing_brightness / 255);
      b = (uint8_t)((uint16_t)s_breathing_b * s_breathing_brightness / 255);

      /* 设置LED（呼吸灯运行期间电源保持常亮，避免频繁开关导致颜色跳变） */
      struct rgb_struct rgb_config = {.rgb_g = g, .rgb_r = r, .rgb_b = b};
      tx1812n5_RGB(&rgb_config, 1);
    }
#if defined(HANDWARE_1_23_2_ONE_SEC)
    if((event_bits & IC_LED_FILE_SYNC_ON_EVENT) == IC_LED_FILE_SYNC_ON_EVENT)
    {
      BC_LOG_INFO("IC_LED_FILE_SYNC_ON_EVENT\r\n");
      /* 文件同步开始：蓝色慢闪 */
      bc_ic_led_breathing_start(LED_BREATHING_SLOW, 0, 0, 0, 20);
    }
    if((event_bits & IC_LED_FILE_SYNC_STOP_EVENT) == IC_LED_FILE_SYNC_STOP_EVENT)
    {
      BC_LOG_INFO("IC_LED_FILE_SYNC_STOP_EVENT\r\n");
      /* 文件同步完成：停止蓝色呼吸灯 */
      if(s_breathing_running)
      {
        if(s_breathing_timer.timer_handler != NULL)
        {
            bc_rtos_timer_stop(s_breathing_timer.timer_handler, 50);
        }
        s_breathing_running = false;
        s_breathing_brightness = 0;
        s_breathing_direction = 1;
        s_breathing_total_count = 0;
        s_breathing_remain_count = 0;
      }
      /* 绿灯亮1秒 */
      bc_ic_led_rgb_set(20, 0, 0, 1);
      if(s_file_sync_green_off_timer.timer_handler == NULL)
      {
          s_file_sync_green_off_timer.timer_handler = bc_rtos_timer_create(
              s_file_sync_green_off_timer.timer_name,
              s_file_sync_green_off_timer.xTimerPeriodInTicks,
              s_file_sync_green_off_timer.uxAutoReload,
              (void *)0,
              s_file_sync_green_off_timer.timer_callback_function);
      }
      if(s_file_sync_green_off_timer.timer_handler != NULL)
      {
          bc_rtos_timer_start(s_file_sync_green_off_timer.timer_handler, 100);
      }
    }
    if((event_bits & IC_LED_FILE_SYNC_GREEN_OFF_EVENT) == IC_LED_FILE_SYNC_GREEN_OFF_EVENT)
    {
      BC_LOG_INFO("IC_LED_FILE_SYNC_GREEN_OFF_EVENT\r\n");
      /* 如果呼吸灯已在运行（如用户在此期间开启了录音），跳过关闭LED */
      if(!s_breathing_running)
      {
        bc_ic_led_rgb_clear();
      }
    }
#endif
#endif
    
  }
}  


void bc_ic_led_ble_connect_from_isr(void)
{
#if defined(SUDO_VOICE_ONLY)
    return;
#else
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(event_struct.event_handler, IC_LED_BLE_CONNECT_EVENT, &xHigherPriorityTaskWoken);
#endif
}

void bc_ic_led_ble_connect(void)
{
#if defined(SUDO_VOICE_ONLY)
  return;
#else
  bc_rtos_event_group_set_bits(event_struct.event_handler,IC_LED_BLE_CONNECT_EVENT );
#endif
}

void bc_ic_led_ble_disconnect(void)
{
#if defined(SUDO_VOICE_ONLY)
  return;
#else
  bc_rtos_event_group_set_bits(event_struct.event_handler,IC_LED_BLE_DISCONNECT_EVENT );
#endif
}

void bc_ic_led_ble_disconnect_from_isr(void)
{
#if defined(SUDO_VOICE_ONLY)
  return;
#else
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xEventGroupSetBitsFromISR(event_struct.event_handler,IC_LED_BLE_DISCONNECT_EVENT , &xHigherPriorityTaskWoken);
#endif
}

void bc_ic_led_mic_offline_recording_on(void)
{
#if defined(SUDO_VOICE_ONLY)
  sudo_recording_indicator_desired = true;
#endif
  bc_rtos_event_group_set_bits(event_struct.event_handler,IC_LED_MIC_OFFLINE_RECORDING_ON_EVENT);
}

void bc_ic_led_mic_online_recording_on(void)
{
  bc_rtos_event_group_set_bits(event_struct.event_handler,IC_LED_MIC_ONLINE_RECORDING_ON_EVENT );
}


void bc_ic_led_mic_offline_recording_capture_on(void)
{
  bc_rtos_event_group_set_bits(event_struct.event_handler,IC_LED_MIC_OFFLINE_RECORDING_CAPTURE_ON_EVENT);
}

void bc_ic_led_mic_online_recording_capture_on(void)
{
  bc_rtos_event_group_set_bits(event_struct.event_handler,IC_LED_MIC_ONLINE_RECORDING_CAPTURE_ON_EVENT );
}


void bc_ic_led_mic_offline_recording_off(void)
{
#if defined(SUDO_VOICE_ONLY)
  sudo_recording_indicator_desired = false;
#endif
  bc_rtos_event_group_set_bits(event_struct.event_handler,IC_LED_MIC_OFFLINE_RECORDING_OFF_EVENT);
}

void bc_ic_led_mic_online_recording_off(void)
{
  bc_rtos_event_group_set_bits(event_struct.event_handler,IC_LED_MIC_ONLINE_RECORDING_OFF_EVENT );
}


void bc_ic_led_mic_offline_recording_capture_off(void)
{
  bc_rtos_event_group_set_bits(event_struct.event_handler,IC_LED_MIC_OFFLINE_RECORDING_CAPTURE_OFF_EVENT);
}

void bc_ic_led_mic_online_recording_capture_off(void)
{
  bc_rtos_event_group_set_bits(event_struct.event_handler,IC_LED_MIC_ONLINE_RECORDING_CAPTURE_OFF_EVENT );
}





			


void bc_ic_led_stop(void)
{
#if defined(SUDO_VOICE_ONLY)
  if (sudo_recording_indicator_desired) return;
  sudo_idle_rgb_desired = 0;
  bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_SUDO_IDLE_COLOR_EVENT);
#else
	struct rgb_struct rgb_config = {.rgb_g = 0,.rgb_r = 0,.rgb_b =0};

	tx1812n5_RGB(&rgb_config ,1);
	bc_ldo_rgb_power_off();
//	tx1812n5_reset();
#endif
}


void bc_ic_led_set(uint8_t* rgb_data)
{
#if defined(SUDO_VOICE_ONLY)
  if (!rgb_data || sudo_recording_indicator_desired) return;
  sudo_idle_rgb_desired = (uint32_t)rgb_data[0] | ((uint32_t)rgb_data[1] << 8) |
                           ((uint32_t)rgb_data[2] << 16);
  bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_SUDO_IDLE_COLOR_EVENT);
#else
	bc_ldo_rgb_power_on();
	bc_delay_ms(20);
	struct rgb_struct rgb_config  = *(struct rgb_struct*)rgb_data;
  if(rgb_config.rgb_g == 0 && rgb_config.rgb_b == 0 && rgb_config.rgb_r == 0)
	{
		bc_ldo_rgb_power_off();
	}else{
	tx1812n5_RGB(&rgb_config ,1);
  }
#endif
}



void bc_ic_led_breathing_light_start(void)
{

}

void bc_ic_led_breathing_light_stop(void)
{


}

#if defined(HANDWARE_1_23_4) || defined(HANDWARE_1_23_2_ONE_SEC)
/*******************************************************************************
 * Function Name     : bc_ic_led_breathing_timer_callback
 * Description       : 呼吸灯定时器回调，发送更新事件到LED线程
 *                     所有LED操作统一在LED线程中处理，避免并发访问导致数据错误
 * Input             : pvParameter - 定时器参数
 * Output            : 无
 * Return            : 无
 * Author            : liukun
 *******************************************************************************/
static void bc_ic_led_breathing_timer_callback(void *pvParameter)
{
    /* 只发送事件，LED更新在LED线程中统一处理 */
    bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_BREATHING_UPDATE_EVENT);
}

/*******************************************************************************
 * Function Name     : bc_ic_led_breathing_start
 * Description       : 启动呼吸灯（发送事件到LED线程处理）
 * Input             : mode - 呼吸灯模式（快闪/慢闪）
 *                     count - 呼吸循环次数（0=无限循环，1~N=呼吸N次后自动停止）
 *                     g - 绿色亮度(0-255)
 *                     r - 红色亮度(0-255)
 *                     b - 蓝色亮度(0-255)
 * Output            : 无
 * Return            : 无
 * Author            : liukun
 *******************************************************************************/
void bc_ic_led_breathing_start(enum bc_ic_led_breathing_mode mode, uint8_t count, uint8_t g, uint8_t r, uint8_t b)
{
    if(mode >= LED_BREATHING_MODE_MAX)
    {
        BC_LOG_ERROR("breathing start: invalid mode %d\r\n", mode);
        return;
    }

    BC_LOG_INFO("breathing start: mode=%d, count=%d, g=%d, r=%d, b=%d\r\n", mode, count, g, r, b);

    /* 保存配置（LED线程中读取使用） */
    s_breathing_mode = mode;
    s_breathing_total_count = count;
    s_breathing_remain_count = count;
    s_breathing_g = g;
    s_breathing_r = r;
    s_breathing_b = b;
    s_breathing_brightness = 0;
    s_breathing_direction = 1;

    /* 发送启动事件，由LED线程统一处理 */
    bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_BREATHING_START_EVENT);
}

/*******************************************************************************
 * Function Name     : bc_ic_led_breathing_stop
 * Description       : 停止呼吸灯（发送事件到LED线程处理）
 * Input             : 无
 * Output            : 无
 * Return            : 无
 * Author            : liukun
 *******************************************************************************/
void bc_ic_led_breathing_stop(void)
{
    BC_LOG_INFO("breathing stop\r\n");

    /* 发送停止事件，由LED线程统一处理 */
    bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_BREATHING_STOP_EVENT);
}

#if defined(HANDWARE_1_23_2_ONE_SEC)
void bc_ic_led_file_sync_start(void)
{
    bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_FILE_SYNC_ON_EVENT);
}

void bc_ic_led_file_sync_stop(void)
{
    bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_FILE_SYNC_STOP_EVENT);
}
#endif /* HANDWARE_1_23_2_ONE_SEC */

#endif /* HANDWARE_1_23_4 || HANDWARE_1_23_2_ONE_SEC */

#if defined(HANDWARE_1_23_4)
/*******************************************************************************
 * Function Name     : bc_ic_led_hold_recording_online_on
 * Description       : 长按在线录音LED指示 - 绿灯长亮
 * Input             : 无
 * Output            : 无
 * Return            : 无
 * Author            : liukun
 *******************************************************************************/
void bc_ic_led_hold_recording_online_on(void)
{
    led_status.hold_recording_online = 1;
    bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_HOLD_RECORDING_ON_EVENT);
}

/*******************************************************************************
 * Function Name     : bc_ic_led_hold_recording_offline_on
 * Description       : 长按离线录音LED指示 - 紫灯长亮
 * Input             : 无
 * Output            : 无
 * Return            : 无
 * Author            : liukun
 *******************************************************************************/
void bc_ic_led_hold_recording_offline_on(void)
{
    led_status.hold_recording_online = 0;
    bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_HOLD_RECORDING_ON_EVENT);
}

/*******************************************************************************
 * Function Name     : bc_ic_led_hold_recording_off
 * Description       : 长按录音结束关闭LED
 * Input             : 无
 * Output            : 无
 * Return            : 无
 * Author            : liukun
 *******************************************************************************/
void bc_ic_led_hold_recording_off(void)
{
    bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_HOLD_RECORDING_OFF_EVENT);
}

/*******************************************************************************
 * Function Name     : bc_ic_led_battery_indication_timer_callback
 * Description       : 电量指示定时器回调，2秒后发送熄灭事件
 * Input             : pvParameter - 定时器参数
 * Output            : 无
 * Return            : 无
 * Author            : liukun
 *******************************************************************************/
static void bc_ic_led_battery_indication_timer_callback(void *pvParameter)
{
    bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_BATTERY_INDICATION_OFF_EVENT);
}

/*******************************************************************************
 * Function Name     : bc_ic_led_battery_indication
 * Description       : 电量LED指示（未充电时，亮2秒后熄灭）
 *                     percent >= 30 : 绿灯
 *                     10 <= percent < 30 : 黄灯
 *                     percent < 10 : 红灯
 * Input             : percent - 电量百分比(0-100)
 * Output            : 无
 * Return            : 无
 * Author            : liukun
 *******************************************************************************/
void bc_ic_led_battery_indication(uint8_t percent)
{
    /* 根据电量设置颜色 */
    if(percent >= 30)
    {
        /* 绿灯 */
        s_battery_indication_g = 20;
        s_battery_indication_r = 0;
        s_battery_indication_b = 0;
    }
    else if(percent >= 10)
    {
        /* 黄灯（绿+红） */
        s_battery_indication_g = 20;
        s_battery_indication_r = 20;
        s_battery_indication_b = 0;
    }
    else
    {
        /* 红灯 */
        s_battery_indication_g = 0;
        s_battery_indication_r = 20;
        s_battery_indication_b = 0;
    }

    BC_LOG_INFO("battery indication: percent=%d, g=%d r=%d b=%d\r\n",
                percent, s_battery_indication_g, s_battery_indication_r, s_battery_indication_b);

    /* 发送点亮事件 */
    bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_BATTERY_INDICATION_ON_EVENT);
}

/*******************************************************************************
 * Function Name     : bc_ic_led_online_recording_dim_timer_callback
 * Description       : 在线录音亮度渐变定时器回调，2秒后发送亮度降低事件
 * Input             : pvParameter - 定时器参数
 * Output            : 无
 * Return            : 无
 * Author            : liukun
 *******************************************************************************/
static void bc_ic_led_online_recording_dim_timer_callback(void *pvParameter)
{
    bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_ONLINE_RECORDING_DIM_EVENT);
}

/*******************************************************************************
 * Function Name     : bc_ic_led_recording_pause_on
 * Description       : 录音暂停LED指示 - 黄灯慢闪
 * Input             : 无
 * Output            : 无
 * Return            : 无
 * Author            : liukun
 *******************************************************************************/
void bc_ic_led_recording_pause_on(void)
{
    bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_RECORDING_PAUSE_ON_EVENT);
}

/*******************************************************************************
 * Function Name     : bc_ic_led_recording_pause_off
 * Description       : 恢复录音LED指示 - 恢复30%绿灯
 * Input             : 无
 * Output            : 无
 * Return            : 无
 * Author            : liukun
 *******************************************************************************/
void bc_ic_led_recording_pause_off(void)
{
    bc_rtos_event_group_set_bits(event_struct.event_handler, IC_LED_RECORDING_PAUSE_OFF_EVENT);
}
#endif

void bc_ic_led_pdm_on(void)
{

}
void bc_ic_led_pdm_off(void)
{

}

void bc_id_led_clear(void)
{
	bc_ldo_rgb_power_on();
	struct rgb_struct rgb_config = {.rgb_g = 0,.rgb_r = 0,.rgb_b =0};

	tx1812n5_RGB(&rgb_config ,1);
	bc_ldo_rgb_power_off();
}

void bc_ic_led_test_cmd(uint8_t g,uint8_t r,uint8_t b)
{
	bc_ldo_rgb_power_on();
    bc_delay_ms(20);
	struct rgb_struct rgb_config = {.rgb_g = 0,.rgb_r = 0,.rgb_b =0};
#if 1 // by liukun 20260512
	if(g)
	{
		rgb_config.rgb_g = g;
	}
	if(r)
	{
		rgb_config.rgb_r = r;
	}
	if(b)
	{
		rgb_config.rgb_b = b;
	}
#else
    if(g == 1)
	{
		rgb_config.rgb_g = 50;
	}
	if(r == 1)
	{
		rgb_config.rgb_r = 50;
	}
	if(b == 1)
	{
		rgb_config.rgb_b = 50;
	}
#endif
	tx1812n5_RGB(&rgb_config ,1);
	if(g == 0 && r == 0 && b == 0)
	{
		bc_ldo_rgb_power_off();
	}
}


void bc_ic_led_init(void)
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
	for(uint8_t i = 0; i < IC_LED_TASK_TYPE_NUM; i++)
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
	
}





































