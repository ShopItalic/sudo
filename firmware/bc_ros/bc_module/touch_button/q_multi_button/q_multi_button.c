
/*******************************************************************************
button库，是为按键提供库支持各种按键触发
c99标准。
版本：v0.0.1
作者：邱成凯
https://gitee.com/feiniao-qiu/q_-multi_-button
 *******************************************************************************/

#include "q_multi_button.h"

#define EVENT_CB(ev)   if(handle->cb[ev])handle->cb[ev]((void*)handle)
#define PRESS_REPEAT_MAX_NUM  15  /*!< 重复计数器的最大值 */

//button句柄
static struct q_button* head_handle = NULL;


static void q_button_poll_ticks(void * pvParameter);
static void q_button_io_irq_handler(void * pvParameter);

static struct q_button_timer_and_io_irq timer_and_io_irq = {NULL};

/*******************************************************************************
 * Function Name     :q_button_timer_start
 * Description       :启动定时器
 * Input             : 无
 * Output            : 无
 * Author            : 邱成凯
 * Modified Date:    : 2024年10月6日
 *******************************************************************************/
static void q_button_timer_start(void)
{
	if(BUTTON_MODE == 1 && timer_and_io_irq.q_button_poll_timer_start != NULL)
	{
		timer_and_io_irq.q_button_poll_timer_start(NULL);
	}
}

/*******************************************************************************
 * Function Name     :q_button_timer_stop
 * Description       :停止定时器
 * Input             : 无
 * Output            : 无
 * Author            : 邱成凯
 * Modified Date:    : 2024年10月6日
 *******************************************************************************/
static void q_button_timer_stop(void)
{
	if(BUTTON_MODE == 1 && timer_and_io_irq.q_button_poll_timer_stop != NULL)
	{
		timer_and_io_irq.q_button_poll_timer_stop(NULL);
	}
}

/*******************************************************************************
 * Function Name     :q_button_timer_reset
 * Description       :复位定时器
 * Input             : 无
 * Output            : 无
 * Author            : 邱成凯
 * Modified Date:    : 2024年10月6日
 *******************************************************************************/
static void q_button_timer_reset(void)
{
	if(BUTTON_MODE == 1 && timer_and_io_irq.q_button_poll_timer_reset != NULL)
	{
		timer_and_io_irq.q_button_poll_timer_reset(NULL);
	}
}

/*******************************************************************************
 * Function Name     : q_button_handler
 * Description       : 按键驱动核心函数，驱动状态机。
 * Input             : handle：button句柄结构
 * Output            : 无
 * Return            : 无
 * Author            : 邱成凯
 * Modified Date:    : 2024年10月6日
 *******************************************************************************/
static void q_button_handler(struct q_button* handle)
{
	uint8_t read_gpio_level = handle->hal_button_Level(handle->button_id);

	//计数器计数
	if((handle->state) > 0)
	{
		handle->ticks++;
	}

	/*------------按键去抖动处理---------------*/
	if(read_gpio_level != handle->button_level)
	{ //不等于前一个
		//读 3 次相同的电平更改
		if(++(handle->debounce_cnt) >= DEBOUNCE_TICKS)
		{
			handle->button_level = read_gpio_level;
			handle->debounce_cnt = 0;
		}
	}
	else
	{ //电平不变 计数重置
		handle->debounce_cnt = 0;
	}

	/*-----------------状态机-------------------*/
	switch (handle->state)
	{
		case 0:
			if(handle->button_level == handle->active_level)
			{	//按下按键
				handle->event = (uint8_t)PRESS_DOWN;
				EVENT_CB(PRESS_DOWN);
				handle->ticks = 0;
				handle->repeat = 1;
				handle->state = 1;
			}
			else
			{
				handle->event = (uint8_t)NONE_PRESS;
			}
			break;

		case 1:
			if(handle->button_level != handle->active_level)
			{ //释放按键
				handle->event = (uint8_t)PRESS_UP;
				EVENT_CB(PRESS_UP);
				handle->ticks = 0;
				handle->state = 2;
			}
			else if(handle->ticks > LONG_TICKS)
			{
				handle->event = (uint8_t)LONG_PRESS_START;
				EVENT_CB(LONG_PRESS_START);
				handle->state = 5;
			}
			break;

		case 2:
			if(handle->button_level == handle->active_level)
			{ //再次按下
				handle->event = (uint8_t)PRESS_DOWN;
				EVENT_CB(PRESS_DOWN);
				if(handle->repeat != PRESS_REPEAT_MAX_NUM)
				{
					handle->repeat++;
				}
				EVENT_CB(PRESS_REPEAT); // 重复按下
				handle->ticks = 0;
				handle->state = 3;
			}
			else if(handle->ticks > SHORT_TICKS)
			{ //释放超时
				if(handle->repeat == 1)
				{
					handle->event = (uint8_t)SINGLE_CLICK;
					EVENT_CB(SINGLE_CLICK);  // 单击
					q_button_timer_stop();
				} 
				else if(handle->repeat == 2)
				{
					handle->event = (uint8_t)DOUBLE_CLICK;
					EVENT_CB(DOUBLE_CLICK); // 双击
					q_button_timer_stop();
				}
        else if(handle->repeat == 3)
				{
					handle->event = (uint8_t)THREE_CLICK;
					EVENT_CB(THREE_CLICK); // 三击
					q_button_timer_stop();
				}
				handle->state = 0;
			}
			break;

		case 3:
			if(handle->button_level != handle->active_level)
			{ //释放按键
				handle->event = (uint8_t)PRESS_UP;
				EVENT_CB(PRESS_UP);
				if(handle->ticks < SHORT_TICKS)
				{
					handle->ticks = 0;
					handle->state = 2; // 重复按下
				}
				else
				{
					handle->state = 0;
				}
			}
			else if(handle->ticks > SHORT_TICKS)
			{ // SHORT_TICKS < press down hold time < LONG_TICKS
				handle->state = 1;
			}
			break;

		case 5:
			if(handle->button_level == handle->active_level)
			{
				//长按
				handle->event = (uint8_t)LONG_PRESS_HOLD;
				EVENT_CB(LONG_PRESS_HOLD);
			}
			else
			{ //释放
				handle->event = (uint8_t)PRESS_UP;
				EVENT_CB(PRESS_UP);
        handle->event = (uint8_t)LONG_PRESS_STOP;
				EVENT_CB(LONG_PRESS_STOP);
				handle->state = 0; //复位状态机状态
				q_button_timer_stop();
			}
			break;
		default:
			handle->state = 0;  //复位状态机状态
			break;
	}
}

/*******************************************************************************
 * Function Name     :q_button_poll_ticks_callback
 * Description       :按键时基定时器处理
 * Input             : 无
 * Output            : 无
 * Author            : 邱成凯
 * Modified Date:    : 2024年10月6日
 *******************************************************************************/
static void q_button_poll_ticks(void * pvParameter)
{
	struct q_button* target;
	for(target=head_handle; target; target=target->next)
	{
		q_button_handler(target);
	}
}


/*******************************************************************************
 * Function Name     :q_button_io_irq_handler
 * Description       :外部中断处理
 * Input             : 无
 * Output            : 无
 * Author            : 邱成凯
 * Modified Date:    : 2024年10月6日
 *******************************************************************************/
static void q_button_io_irq_handler(void * pvParameter)
{
	q_button_timer_start();
}

/*******************************************************************************
 * Function Name     : q_button_init
 * Description       : 初始化button结构句柄
 * Input             : handle：button句柄结构
                       pin_level：读取io电平的接口（传入函数指针地址）
					   active_level：按下的 GPIO 电平状态
					   button_id：按键id
 * Output            : 无
 * Return            : 无
 * Author            : 邱成凯
 * Modified Date:    : 2024年10月6日
 *******************************************************************************/
void q_button_init(struct q_button* handle, uint8_t(*pin_level)(uint8_t), uint8_t active_level, uint8_t button_id)
{
	memset(handle, 0, sizeof(struct q_button));
	handle->event = (uint8_t)NONE_PRESS;
	handle->hal_button_Level = pin_level;
	handle->button_level = !active_level;
	handle->active_level = active_level;
	handle->button_id = button_id;
}

/*******************************************************************************
 * Function Name     : q_button_attach
 * Description       : 注册按键事件回调函数
 * Input             : handle：button句柄结构
                       event：触发事件类型
					   cb：回调函数
 * Output            : 无
 * Return            : 无
 * Author            : 邱成凯
 * Modified Date:    : 2024年10月4日
 *******************************************************************************/
void q_button_attach(struct q_button* handle, press_event event, btn_callback cb)
{
	handle->cb[event] = cb;
}

/*******************************************************************************
 * Function Name     : q_get_button_event
 * Description       : 查询按键事件发生
 * Input             : handle：button句柄结构
 * Output            : 无
 * Return            : press_event 按键事件
 * Author            : 邱成凯
 * Modified Date:    : 2024年10月6日
 *******************************************************************************/
press_event q_get_button_event(struct q_button* handle)
{
	return (press_event)(handle->event);
}




/*******************************************************************************
 * Function Name     : q_button_start
 * Description       : 启动按键，将句柄添加到工作列表中
 * Input             : handle：button句柄结构
 * Output            : 无
 * Return            : 0：成功。-1：已经存在。
 * Author            : 邱成凯
 * Modified Date:    : 2024年10月6日
 *******************************************************************************/
int q_button_start(struct q_button* handle)
{
	struct q_button* target = head_handle;
	while(target)
	{
		if(target == handle)
		{
			return -1;	//已经存在
		}
		target = target->next;
	}
	handle->next = head_handle;
	head_handle = handle;
	return 0;
}

/*******************************************************************************
 * Function Name     :q_button_stop
 * Description       :  停止按键，从工作列表中移除句柄
 * Input             : handle：button句柄结构
 * Output            : 无
 * Author            : 邱成凯
 * Modified Date:    : 2024年10月6日
 *******************************************************************************/
void q_button_stop(struct q_button* handle)
{
	struct q_button** curr;
	for(curr = &head_handle; *curr; )
	{
		struct q_button* entry = *curr;
		if(entry == handle)
		{
			*curr = entry->next;
//			free(entry);
			return;
		}
		else
		{
			curr = &entry->next;
		}
	}
}

/*******************************************************************************
 * Function Name     :q_button_ticks
 * Description       :滴答 定时器重复调用间隔 5ms
 * Input             : 无
 * Output            : 无
 * Author            : 邱成凯
 * Modified Date:    : 2024年10月6日
 *******************************************************************************/
void q_button_ticks(void)
{
	struct q_button* target;
	for(target=head_handle; target; target=target->next)
	{
		q_button_handler(target);
	}
}

/*******************************************************************************
 * Function Name     :q_button_timer_and_io_irq_init
 * Description       :初始化定时器接口与中断处理接口
 * Input             : handle q_button_timer_and_io_irq结构体句柄
 * Output            : 无
 * Return            : 0：成功。-1：传入的句柄是NULL
 * Author            : 邱成凯
 * Modified Date:    : 2024年10月6日
 *******************************************************************************/

int q_button_timer_and_io_irq_init(struct q_button_timer_and_io_irq* handle)
{
	if (handle != NULL) 
	{
		handle->q_button_poll_timer_callback = q_button_poll_ticks;
		handle->q_button_io_irq_handler_callback = q_button_io_irq_handler;
        timer_and_io_irq = *handle;
		return 0;
    }
	return -1;
}

