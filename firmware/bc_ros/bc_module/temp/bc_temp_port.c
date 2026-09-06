#include "bc_temp_port.h"


#include "q_device.h"

#if (defined(HANDWARE_1_5_6) )

#include "bc_device_i2c_bus_handler.h" 

#endif

static q_device_t *i2c_dev = NULL;                                     //设备描述

static struct i2c_package i2c_pack = {
										 .slave_addr = 0x12,        //i2c从机地址
										 .write_length = 1,
										 .read_length = 1,
									 };





bool bc_temper_i2c_write(uint8_t slave_addr,uint8_t reg_addr,uint8_t *write_data,uint8_t write_length)
{
	i2c_pack.reg_addr = reg_addr;
	i2c_pack.slave_addr = slave_addr ;
	if(write_data != NULL)
	{
//		memcpy(i2c_pack.write_buff,write_data,write_length);
		i2c_pack.write_buff = write_data;
	}
	i2c_pack.write_length = write_length;
//    disable_irq();	
	if(q_device_write(i2c_dev,0,&i2c_pack,0) != RESULT_OK)
	{
//		enable_irq();
		return false;
	}
//    enable_irq();	
    return true;
}

bool bc_temper_i2c_read(uint8_t slave_addr,uint8_t reg_addr,uint8_t *read_data,uint8_t read_length)
{
	i2c_pack.reg_addr = reg_addr;
	i2c_pack.slave_addr = slave_addr ;
	i2c_pack.read_length = read_length;
	i2c_pack.read_buff = read_data;
	
//	disable_irq();
	if(q_device_read(i2c_dev,0,&i2c_pack,0) != RESULT_OK)
	{
//		enable_irq();
		return false;
	}

	if(read_data != NULL)
	{
//		memcpy(read_data,i2c_pack.read_buff,i2c_pack.read_length);
	}
//	enable_irq();
	return true;
}

void bc_temper_i2c_bus_open(void)
{
	q_device_open(i2c_dev);  
	BC_LOG_INFO("bc temper bus open \r\n");
}


void bc_temper_i2c_bus_close(void)
{
	q_device_close(i2c_dev);  
	BC_LOG_INFO("bc temper bus close \r\n");
}	

void bc_temper_device_i2c_open(void)
{

#if (defined(HANDWARE_1_5_6) )
    bc_i2c_bus_device_open(DEVICE_BUS_TYPE_TEMPER);
#else	
	q_device_open(i2c_dev);  
#endif	
	
	                                                                      //打开设备

}

void bc_temper_device_i2c_close(void)
{
	
#if (defined(HANDWARE_1_5_6) )
    bc_i2c_bus_device_close(DEVICE_BUS_TYPE_TEMPER);
#else	
	q_device_close(i2c_dev);                                                                        //关闭设备 
#endif		
	
}


void bc_temper_device_i2c_find(void)
{

#if (HARDWARE_156_ENABLED == 1 )	

	if(i2c_dev == NULL)
	{
		i2c_dev = q_device_find("i2c_1");
		q_device_assert(i2c_dev);
	}
#elif (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
	if(i2c_dev == NULL)
	{
		i2c_dev = q_device_find("i2c_1");
		q_device_assert(i2c_dev);
	}
#else
	if(i2c_dev == NULL)
	{
		i2c_dev = q_device_find("i2c_1");
		q_device_assert(i2c_dev);
	}

#endif		
	
}





















