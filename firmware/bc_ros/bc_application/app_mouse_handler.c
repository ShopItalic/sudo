#include "app_mouse_handler.h"

#include "app_ble_handler.h"
#include "app_package.h"

#include "bc_mouse.h"
#include "bc_delay.h"
#include "bc_event.h"
#include "bc_timer.h"
#include "bc_logger.h"
#include "bc_ldo_switch.h"

#include <string.h>

typedef struct
{
  	unsigned char motion;//bit7	
	int8_t dx;
	int8_t dy;
	unsigned char dir;//bit3~0	
	unsigned char touch;//bit7,4
	unsigned char click;//bit5,4
} app_mouse_xyd_t;
static app_mouse_xyd_t *mouse_xyd_data = NULL;

enum app_mouse_task_event
{
	APP_MOUSE_DATA_HANDLER_POLL_TASK_EVENT = 0,
	MOUSE_TASK_TYPE_NUM,
};

enum app_mouse_timer_event
{
	APP_MOUSE_TIMER_EVENT = 0,
	APP_MOUSE_TIMER_NUM
};

static void mouse_data_handler_event_callback(void * p_context);


static bc_event_struct event_struct[MOUSE_TASK_TYPE_NUM] = {	
	{
		.event_name = "mouse data event",
		.event_callback_function = mouse_data_handler_event_callback,
	},
};


static void app_mouse_timer_callback(void * pvParameter);

static bc_timer_struct  timer_struct[APP_MOUSE_TIMER_NUM] = {
	{
		.timer_name = "mouse timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 8,
		.lock = false,
		.timer_callback_function = app_mouse_timer_callback,
	},
};

static void app_mouse_int_io_irq_callback(uint8_t pin,uint8_t state)
{	
	BC_LOG_INFO("mouse irq  %d  %d\r\n",pin,state);	
//	bc_event_set(&event_struct);
}

static void app_mouse_timer_callback(void * pvParameter)
{
	bc_event_set(&event_struct[APP_MOUSE_DATA_HANDLER_POLL_TASK_EVENT]);
}

static void mouse_data_handler_event_callback(void * p_context)
{
	bc_mouse_ir_handler();
//	BC_LOG_BLE("mouse irq  \r\n");
}

static void app_mouse_x_ycallback(int8_t x,int8_t y)
{
	app_ble_mouse_x_y_movement((int16_t)x, (int16_t)y);
}

static void app_mouse_event_data_callback(uint8_t* data,uint8_t length)
{
	if (length >= sizeof(app_mouse_xyd_t))
    {
        // 将 data 强制转换为 app_mouse_xyd_t 类型并赋值
//        memcpy(mouse_xyd_data, data, sizeof(app_mouse_xyd_t));
//		app_package_mouse_event_up(data,length);
		if(data[5] == 0x10)
		{
			app_ble_mouse_left_button();
			BC_LOG_BLE("left_button \r\n");
		}
		else if(data[5] == 20)
		{
			app_ble_mouse_left_button();
//			bc_delay_ms(100);
//			app_ble_mouse_left_button();
		}
    }
    else
    {
        // 数据长度不足，处理错误
        printf("Received data length is too short\n");
    }
}


void app_mouse_start(void)
{
    bc_ldo_mouse_power_on();
	bc_delay_ms(100);
    bc_mouse_init(app_mouse_int_io_irq_callback);	
	
	bc_timer_start(&timer_struct[APP_MOUSE_TIMER_EVENT]);	
}

void app_mouse_stop(void)
{
	bc_ldo_mouse_power_off();
	bc_timer_stop(&timer_struct[APP_MOUSE_TIMER_EVENT]);	
}

void app_mouse_init(void)
{
	bc_mouse_x_y_callback_register_callback(app_mouse_x_ycallback);
	bc_mouse_event_data_callback_register_callback(app_mouse_event_data_callback);
	
	for(uint8_t i = 0;i < APP_MOUSE_TIMER_NUM; i++)
	{
		if(!bc_timer_create(&timer_struct[i]))
		{
			BC_LOG_INFO("create %s fial!! \r\n",timer_struct[i].timer_name);
		}
		else
		{
			BC_LOG_INFO("create %s success!! \r\n",timer_struct[i].timer_name);
		}		
	}
	for(uint8_t i = 0; i <MOUSE_TASK_TYPE_NUM; i++)
	{

		if(!bc_event_create(&event_struct[i]))                         //创建事件
		{
			BC_LOG_WARN("create %s fial!! \r\n",event_struct[i].event_name);
		}
		else
		{
			BC_LOG_INFO("create %s success!! \r\n",event_struct[i].event_name);
		}		
   }
}












