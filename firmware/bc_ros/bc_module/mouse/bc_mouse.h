#ifndef __BC_MOUSE_H__
#define __BC_MOUSE_H__





#include <stdint.h>
#include <stdbool.h>

uint8_t bc_mouse_id_get(void);


void bc_mouse_init(void *register_callback);
void bc_mouse_ir_handler(void);

bool bc_mouse_x_y_callback_register_callback(void *callback);

bool bc_mouse_event_data_callback_register_callback(void *callback);


#endif








