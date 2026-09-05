#ifndef __BC_KEY_H__
#define __BC_KEY_H__



#include <stdbool.h>
#include <stdint.h>

void bc_key_io_irq_enable(void);

void bc_key_io_irq_disable(void);

bool bc_key_io_irq_register_callback(void *callback);

void bc_key_device_find(void);

uint8_t bc_key_io_key_status_get(uint8_t button_id);







#endif








