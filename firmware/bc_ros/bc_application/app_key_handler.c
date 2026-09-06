#include "app_key_handler.h"

#include "app_ble_handler.h"
#include "app_package.h"

#include "bc_logger.h"
#include "bc_key.h"
#include "bc_rtos.h"

#include "q_multi_button.h"
#include "stdint.h"
#if ( HARDWARE_1191_ENABLED == 1)	

#include "bc_wifi_port.h"


#endif	

#if (HARDWARE_1191_ENABLED == 1 )	

#include "app_hardline_tsdb_handler.h"
#include "app_pdm_handler.h"
#endif

#if ( HARDWARE_1191_ENABLED == 1)	

#include "bc_led_pwm.h"

#endif	

#include "bc_touch_button_device_port.h"

enum app_key_timer_type
{
	KEY_MULTIBUTTON_POLL_TIMER = 0,
	KEY_TIMER_TYPE_NUM
};

enum button_id {
	btn1_id,
};

static struct q_button btn1;

static void app_key_timer_start(void * pvParameter);
static void app_key_timer_stop(void * pvParameter);
static void app_key_timer_reset(void * pvParameter);

static struct q_button_timer_and_io_irq button_timer_and_io_irq = {
	.q_button_poll_timer_start = app_key_timer_start,
	.q_button_poll_timer_stop = app_key_timer_stop,
	.q_button_poll_timer_reset = app_key_timer_reset,
};


static bc_rtos_timer_struct  timer_struct[KEY_TIMER_TYPE_NUM] = {
	{
		.timer_name = "key poll timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 30,
		.lock = false,
		.timer_callback_function = NULL,
	}
};

static void app_key_timer_start(void * pvParameter)
{
	if(!timer_struct[KEY_MULTIBUTTON_POLL_TIMER].lock)
	{
		bc_rtos_timer_start(timer_struct[KEY_MULTIBUTTON_POLL_TIMER].timer_handler,100);
		timer_struct[KEY_MULTIBUTTON_POLL_TIMER].lock = true;
	}
}

static void app_key_timer_stop(void * pvParameter)
{
	if(timer_struct[KEY_MULTIBUTTON_POLL_TIMER].lock)
	{
		bc_rtos_timer_stop(timer_struct[KEY_MULTIBUTTON_POLL_TIMER].timer_handler,100);
		timer_struct[KEY_MULTIBUTTON_POLL_TIMER].lock = false;

	}
}

static void app_key_timer_reset(void * pvParameter)
{
	if(timer_struct[KEY_MULTIBUTTON_POLL_TIMER].lock)
	{
		bc_rtos_timer_reset(timer_struct[KEY_MULTIBUTTON_POLL_TIMER].timer_handler,50);
	}
}


static void app_btn1_single_click_handler(void *event)
{
	BC_LOG_INFO("key single click \r\n");
#if (HARDWARE_1191_ENABLED == 1 )	

  if(app_pdm_mode_get() == PDM_MODE_OFFLINE)
  {
    app_hardline_mark();
  }
  
#endif
}

#if ( HARDWARE_1191_ENABLED == 1)	
static bool app_pdm_key_flag = true;
#endif
static void app_btn1_double_click_handler(void *event)
{
	BC_LOG_INFO("key double click \r\n");

#if ( HARDWARE_1191_ENABLED == 1)	

  if(app_pdm_key_flag)
  {
    if(app_pdm_mode_get() == PDM_MODE_IDIE)
    {
      app_package_mic_recording_start();
      app_pdm_key_flag = false;
    }
    
  }
  else
  {
    if(app_pdm_mode_get() == PDM_MODE_OFFLINE)
    {
      app_package_mic_recording_stop();
      app_pdm_key_flag = true;
    }
  }

#endif	
  
}

static void app_btn1_three_click_handler(void *event)
{
	BC_LOG_INFO("key three click \r\n");
  
}

static void app_btn1_long_press_start_handler(void *event)
{
	BC_LOG_INFO("key long press start \r\n");
#if (HARDWARE_1191_ENABLED == 1 )	  
  if(app_pdm_mode_get() == PDM_MODE_IDIE)
  {
    app_package_mic_capture_recording_start();
  }
#endif  
}

static void app_btn1_long_press_stop_handler(void *event)
{
	BC_LOG_INFO("key long press stop \r\n");
#if (HARDWARE_1191_ENABLED == 1 )	  
  if(app_pdm_mode_get() == PDM_MODE_KEY_OFFLINE)
  {
    app_package_mic_capture_recording_stop();
  }
#endif  
}


static void app_btn1_long_press_hold_handler(void *event)
{
//	BC_LOG_INFO("key long press hold \r\n");
}
static void app_key_io_irq_callback(void)
{
	
	BC_LOG_INFO("key irq \r\n");
	if(button_timer_and_io_irq.q_button_io_irq_handler_callback != NULL)
	{
		button_timer_and_io_irq.q_button_io_irq_handler_callback(NULL);
	}
}

void app_touch_key_io_irq_callback(void)
{
	
	BC_LOG_INFO("key irq \r\n");
	if(button_timer_and_io_irq.q_button_io_irq_handler_callback != NULL)
	{
		button_timer_and_io_irq.q_button_io_irq_handler_callback(NULL);
	}
}


void app_key_handler_init(void)
{
	q_button_init(&btn1, bc_key_io_key_status_get, 0, btn1_id);
  
	q_button_timer_and_io_irq_init(&button_timer_and_io_irq);
	
	q_button_attach(&btn1, SINGLE_CLICK,     app_btn1_single_click_handler);
	q_button_attach(&btn1, DOUBLE_CLICK,     app_btn1_double_click_handler);
  q_button_attach(&btn1, THREE_CLICK,      app_btn1_three_click_handler);
	q_button_attach(&btn1, LONG_PRESS_START, app_btn1_long_press_start_handler);
	q_button_attach(&btn1, LONG_PRESS_HOLD,  app_btn1_long_press_hold_handler);
  q_button_attach(&btn1, LONG_PRESS_STOP,  app_btn1_long_press_stop_handler);
  
  
	q_button_start(&btn1);
	
	
	timer_struct[KEY_MULTIBUTTON_POLL_TIMER].timer_callback_function = button_timer_and_io_irq.q_button_poll_timer_callback;
	
  
  for(uint8_t i = 0;i <  KEY_TIMER_TYPE_NUM; i++)
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
	
	bc_key_io_irq_register_callback(app_key_io_irq_callback);
	bc_key_io_irq_enable();
}











