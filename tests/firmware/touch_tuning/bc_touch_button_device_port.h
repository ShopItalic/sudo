#ifndef TEST_TOUCH_TUNING_BC_TOUCH_BUTTON_DEVICE_PORT_H
#define TEST_TOUCH_TUNING_BC_TOUCH_BUTTON_DEVICE_PORT_H

#include <stdbool.h>
#include <stdint.h>

int touch_i2c_open(void);
int touch_i2c_close(void);
bool touch_i2c_write(uint8_t reg, uint8_t *data, uint8_t length);
bool touch_i2c_read(uint8_t reg, uint8_t *data, uint8_t length);
uint8_t touch_io_irq_status(void);

#endif
