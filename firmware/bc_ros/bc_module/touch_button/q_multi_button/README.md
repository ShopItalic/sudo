# q_multi_button

## 简介
q_multi_button 是一个事件驱动型按键驱动模块，可无限量扩展按键，按键事件的回调异步处理方式可以简化你的程序结构，参考了multibutton 


## 使用方法
1.先申请一个按键结构

```c
struct q_button button1;
```
2.初始化按键对象，绑定按键的GPIO电平读取接口** key_io_key_status_get()** ，后一个参数设置有效触发电平

```c
q_button_init(&button1,  key_io_key_status_get 0, 0);
```
3.注册按键事件

```c
q_button_attach(&button1, SINGLE_CLICK,     app_btn1_single_click_handler);
q_button_attach(&button1, DOUBLE_CLICK,     app_btn1_double_click_handler);
...
```
4.启动按键

```c
q_button_start(&button1);
```
5.设置一个5ms间隔的定时器循环调用后台处理函数

```c
while(1) {
    ...
    if(timer_ticks == 5) {
        timer_ticks = 0;

        q_button_ticks();
    }
}
```

## 特性

q_multi_button 使用C语言实现，基于面向对象方式设计思路，每个按键对象单独用一份数据结构管理：

```c
struct q_button {
	uint16_t ticks;
	uint8_t  repeat : 4;
	uint8_t  event : 4;
	uint8_t  state : 3;
	uint8_t  debounce_cnt : 3;
	uint8_t  active_level : 1;
	uint8_t  button_level : 1;
	uint8_t  button_id;
	uint8_t  (*hal_button_Level)(uint8_t button_id_);
	btn_callback  cb[number_of_event];
	struct q_button* next;
};
```
这样每个按键使用单向链表相连，依次进入 q_button_handler(struct q_button* handle) 状态机处理，所以每个按键的状态彼此独立。


## 按键事件

事件 | 说明
---|---
PRESS_DOWN | 按键按下，每次按下都触发
PRESS_UP | 按键弹起，每次松开都触发
PRESS_REPEAT | 重复按下触发，变量repeat计数连击次数
SINGLE_CLICK | 单击按键事件
DOUBLE_CLICK | 双击按键事件
LONG_PRESS_START | 达到长按时间阈值时触发一次
LONG_PRESS_HOLD | 长按期间一直触发


## Examples

```c

#include "logger.h"
#include "timer.h"
#include "q_multi_button.h"
#include "stdint.h"

enum app_key_timer_type
{
	KEY_MULTIBUTTON_POLL_TIMER = 0,
	KEY_MULTIBUTTON_HOLD_STOP_CHECK_TIMER,
	KEY_TIMER_TYPE_NUM
};

enum button_id {
	btn1_id,
};

static struct q_button btn1;

static void app_key_poll_timer_callback(void * pvParameter);
static void app_key_hold_stop_check_timer_callback(void * pvParameter);

static bc_timer_struct  timer_struct[KEY_TIMER_TYPE_NUM] = {
	{
		.timer_name = "key poll timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 5,
		.lock = false,
		.timer_callback_function = app_key_poll_timer_callback,
	},
	{
		.timer_name = "key hold stop check timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 50,
		.lock = false,
		.timer_callback_function = app_key_hold_stop_check_timer_callback,
	}
};	

static void app_key_timer_start(enum app_key_timer_type  timer_type)
{
	if(!timer_struct[timer_type].lock)
	{
		timer_start(&timer_struct[timer_type]);
		timer_struct[timer_type].lock = true;
	}
}

static void app_key_timer_stop(enum app_key_timer_type  timer_type)
{
	if(timer_struct[timer_type].lock)
	{
		timer_stop(&timer_struct[timer_type]);
		timer_struct[timer_type].lock = false;
	}
}

static void app_key_timer_reset(enum app_key_timer_type  timer_type)
{
	if(timer_struct[timer_type].lock)
	{
		timer_stop(&timer_struct[timer_type]);
		timer_start(&timer_struct[timer_type]);
	}
}

static void app_key_poll_timer_callback(void * pvParameter)
{
	q_button_ticks();
}

static void app_key_hold_stop_check_timer_callback(void * pvParameter)
{
	app_key_timer_stop(KEY_MULTIBUTTON_POLL_TIMER);
	app_key_timer_stop(KEY_MULTIBUTTON_HOLD_STOP_CHECK_TIMER);
}

static void app_btn1_single_click_handler(void *event)
{
	LOG_INFO("key single click \r\n");
	app_key_timer_stop(KEY_MULTIBUTTON_POLL_TIMER);
}

static void app_btn1_double_click_handler(void *event)
{
	LOG_INFO("key double click \r\n");
	app_key_timer_stop(KEY_MULTIBUTTON_POLL_TIMER);
}

static void app_btn1_long_press_start_handler(void *event)
{
	LOG_INFO("key long press start \r\n");
	app_key_timer_start(KEY_MULTIBUTTON_HOLD_STOP_CHECK_TIMER);
}

static void app_btn1_long_press_hold_handler(void *event)
{
	LOG_INFO("key long press hold \r\n");
	app_key_timer_reset(KEY_MULTIBUTTON_HOLD_STOP_CHECK_TIMER);
}
static void app_key_io_irq_callback(void)
{
	
	LOG_INFO("key irq \r\n");
	
	app_key_timer_start(KEY_MULTIBUTTON_POLL_TIMER);
	
}

void app_key_handler_init(void)
{
	q_button_init(&btn1, key_io_key_status_get, 0, btn1_id);
	
	q_button_attach(&btn1, SINGLE_CLICK,     app_btn1_single_click_handler);
	q_button_attach(&btn1, DOUBLE_CLICK,     app_btn1_double_click_handler);
	q_button_attach(&btn1, LONG_PRESS_START, app_btn1_long_press_start_handler);
	q_button_attach(&btn1, LONG_PRESS_HOLD,  app_btn1_long_press_hold_handler);
	q_button_start(&btn1);
	
	for(uint8_t i = 0;i < KEY_TIMER_TYPE_NUM; i++)
	{
		if(!timer_create(&timer_struct[i]))
		{
			BC_LOG_INFO("create %s fial!! \r\n",timer_struct[i].timer_name);
		}
		else
		{
			BC_LOG_INFO("create %s success!! \r\n",timer_struct[i].timer_name);
		}		
	}
}

```
