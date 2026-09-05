#include "bsp.h"

#include "q_init.h"
#include "q_device.h"

void bsp_init(void)
{
	disable_irq();
	do_init_call();
	enable_irq();
	do_app_init_call();
	
}	







