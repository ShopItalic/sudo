#ifndef __BC_TOUCH_BUTTON_DEVICE_PORT_H__
#define __BC_TOUCH_BUTTON_DEVICE_PORT_H__



#include <stdint.h>
#include <stdbool.h>




int touch_i2c_open(void);

int touch_i2c_close(void);	
	
bool touch_i2c_write(uint8_t reg_add ,uint8_t *data,uint8_t length);
																 
bool touch_i2c_read(uint8_t reg_add ,uint8_t *data,uint8_t length);

void bc_touch_button_device_find(void);

void touch_rdy_out_high(void);

void touch_rdy_out_low(void);

void touch_io_irq_enable(void);

void touch_io_irq_disable(void);

bool touch_irq_register_callback(void *callback);

uint8_t touch_io_irq_status(void);

uint8_t touch_io_irq_status_get(uint8_t button_id);

void touch_io_irq_disnable(void);

void touch_rdy_out_open(void);

void touch_rdy_out_close(void);

#endif

