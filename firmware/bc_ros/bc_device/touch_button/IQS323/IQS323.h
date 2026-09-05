#ifndef __IQS323_H__
#define __IQS323_H__

#include <stdbool.h>
#include <stdint.h>

#define UNUSED(X)  ((void)(X))

#define IQS323_ADDR               0x44

bool touch_i2c_write(uint8_t reg_add ,uint8_t *data,uint8_t length);
bool touch_i2c_read(uint8_t reg_add ,uint8_t *data,uint8_t length);
uint8_t touch_io_irq_status(void);

void IQS323_Init(void);
void IQS323_Stop_Bit_Disabled(void);
void IQS323_Stop_I2C_Comm_Window(void);
void Process_IQS323_Events(void);

void IQS323_unint(void);

void config_flag_set(bool flag);

bool config_flag_get(void);


bool gesture_event_flick_positive_register_callback(void *callback);

bool gesture_event_swipe_positive_register_callback(void *callback);

bool CH0_in_touch_register_callback(void *callback);

bool CH1_in_touch_register_callback(void *callback);

bool CH2_in_touch_register_callback(void *callback);

bool error_register_callback(void *callback);

bool gesture_event_hold_register_callback(void *callback);

bool gesture_event_flick_negative_register_callback(void *callback);

bool event_rawdata_callback_register_callback(void *callback);

bool check_callback_register_callback(void *callback);

#endif
