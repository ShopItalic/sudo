#include "bc_nfc_rs2323_port.h"



#include "q_device.h"

#include <string.h>
#include "bc_logger.h"



static q_device_t *rs2323_vdd_sw_dev = NULL;                              //设备描述

static q_device_t *rs2323_sw_dev = NULL;                                  //设备描述


void bc_nfc_nordic_on(void)
{
//	q_device_open(rs2323_vdd_sw_dev);
	q_device_ctrl(rs2323_vdd_sw_dev,GPIO_OUTPUT_HIGH,0);
	
//	q_device_open(rs2323_sw_dev);
	q_device_ctrl(rs2323_sw_dev,GPIO_OUTPUT_LOW,0);
	
}

void bc_nfc_exit_FM11RF08_on(void)
{
//	q_device_open(rs2323_vdd_sw_dev);
	q_device_ctrl(rs2323_vdd_sw_dev,GPIO_OUTPUT_HIGH,0);
	
//	q_device_open(rs2323_sw_dev);	
	q_device_ctrl(rs2323_sw_dev,GPIO_OUTPUT_HIGH,0);
	
}



void bc_nfc_rs2323_device_find(void)
{


	
	if(rs2323_vdd_sw_dev == NULL)
	{
		rs2323_vdd_sw_dev = q_device_find("rs2323_vdd_sw");
		q_device_assert(rs2323_vdd_sw_dev);
		q_device_open(rs2323_vdd_sw_dev);
//		q_device_ctrl(rs2323_vdd_sw_dev,GPIO_OUTPUT_LOW,0);
	}
	if(rs2323_sw_dev == NULL)
	{
		rs2323_sw_dev = q_device_find("rs2323_sw");
		q_device_assert(rs2323_sw_dev);
		q_device_open(rs2323_sw_dev);
//		q_device_ctrl(rs2323_sw_dev,GPIO_OUTPUT_LOW,0);
	}


	
}







