/*******************************************************************************
button库，是为按键提供库支持各种按键触发
c99标准。
版本：v0.0.2
作者：邱成凯
https://gitee.com/feiniao-qiu/q_-multi_-button
2025.12.8：增加3击检测
 *******************************************************************************/

#ifndef _MULTI_BUTTON_H_
#define _MULTI_BUTTON_H_

#include <stdint.h>
#include <string.h>

//根据需要修改常量
#define TICKS_INTERVAL    30	//ms
#define DEBOUNCE_TICKS    3	//MAX 7 (0 ~ 7)
#define SHORT_TICKS       (300 /TICKS_INTERVAL)
#define LONG_TICKS        (3000 /TICKS_INTERVAL)

#define BUTTON_MODE       1  //0--轮询模式,由应用层手动赋予时基，1--外部中断处理模式（需要注册配置定时器接口）


typedef void (*btn_callback)(void*);

typedef enum {
	PRESS_DOWN = 0,
	PRESS_UP,
	PRESS_REPEAT,
	SINGLE_CLICK,
	DOUBLE_CLICK,
  THREE_CLICK,
	LONG_PRESS_START,
	LONG_PRESS_HOLD,
  LONG_PRESS_STOP,
	number_of_event,
	NONE_PRESS
}press_event;

typedef struct q_button {
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
}q_button;


//外部中断结合定时器使用，定时器提供时基，外部中断触发，提高效率，降低资源占用
typedef struct q_button_timer_and_io_irq {
	void (*q_button_poll_timer_start)(void*);
	void (*q_button_poll_timer_stop)(void*);
	void (*q_button_poll_timer_reset)(void*);
	void (*q_button_poll_timer_callback)(void*);
	void (*q_button_io_irq_handler_callback)(void*);
}q_button_timer_and_io_irq;

#ifdef __cplusplus
extern "C" {
#endif

void q_button_init(struct q_button* handle, uint8_t(*pin_level)(uint8_t), uint8_t active_level, uint8_t button_id);
int q_button_timer_and_io_irq_init(struct q_button_timer_and_io_irq* handle);
void q_button_attach(struct q_button* handle,press_event event, btn_callback cb);
press_event q_get_button_event(struct q_button* handle);
int  q_button_start(struct q_button* handle);
void q_button_stop(struct q_button* handle);
void q_button_ticks(void);


#ifdef __cplusplus
}
#endif

#endif
