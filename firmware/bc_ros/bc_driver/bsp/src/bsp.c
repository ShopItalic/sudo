
/*******************************************************************************
此为bsp初始化文件，必须在初始化后才可以调用q_device的api对bsp外设进行操控

日  期：2024年1月17日
编写人：邱成凯
 *******************************************************************************/


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







