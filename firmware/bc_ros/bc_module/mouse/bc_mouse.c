#include "bc_mouse.h"

#include <stdint.h>

#include "bc_logger.h"

#include "bc_mouse_device_port.h"
#include "bc_ldo_switch.h"
#include "bc_delay.h"

#include "paw3008.h"

uint8_t bc_mouse_id_get(void)
{
	uint8_t id = 0;
	bc_ldo_mouse_power_on();
	bc_mouse_i2c_open();
	bc_delay_ms(20);
	
	bc_mouse_i2c_read(0x53,00&0x7F,&id,1);
	BC_LOG_INFO("bc mouse Id:0x%x \r\n",id);  //0x33
	bc_mouse_i2c_close();
	bc_ldo_mouse_power_off();
	return id;
}

void bc_mouse_init(void *register_callback)
{
	bc_mouse_io_irq_register_callback(register_callback);
	bc_mouse_int_io_irq_enable();
	bc_mouse_i2c_open();
	
	OFN_Init();
	bc_mouse_i2c_close();	
}

bool bc_mouse_id_check(void)
{
	uint8_t id = 0;
	bc_ldo_mouse_power_on();
	bc_mouse_i2c_open();
	bc_delay_ms(20);
	
	bc_mouse_i2c_read(0x53,00&0x7F,&id,1);
	BC_LOG_INFO("bc mouse Id:0x%x \r\n",id);  //0x33
	bc_mouse_i2c_close();
	bc_ldo_mouse_power_off();
	if(id == 0x33)
	{
		return true;
	}
	return false;
}


void bc_mouse_ir_handler(void)
{
	uint8_t id = 0;
	bc_mouse_i2c_open();
	
	OFN_ReportXY_handler();
	bc_mouse_i2c_close();
}


bool bc_mouse_x_y_callback_register_callback(void *callback)
{
	return mouse_x_y_callback_register_callback(callback);
}

bool bc_mouse_event_data_callback_register_callback(void *callback)
{
	return mouse_event_data_callback_register_callback(callback);
}


