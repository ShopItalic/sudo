#include "bc_g_sensor_device_port.h"


#include "q_device.h"

#include <string.h>
#include "bc_logger.h"

#if (defined(HANDWARE_4_5_1) )

#include "bc_device_i2c_bus_handler.h"

#endif


static q_device_t *i2c_dev = NULL;                                     //设备描述

static q_device_t *int_io_dev = NULL;                                  //设备描述



#if (HARDWARE_181_ENABLED == 1 )	

static q_device_t *cs_io_dev = NULL;                                   //设备描述
static q_device_t *sdo_io_dev = NULL;                                  //设备描述

#endif

static void *int_io_irq_callback = NULL;

static struct i2c_package i2c_pack = {
										 .slave_addr = 0x12,        //i2c从机地址
										 .write_length = 1,
										 .read_length = 1,
									 };





bool bc_g_sensor_i2c_write(uint8_t slave_addr,uint8_t reg_addr,uint8_t *write_data,uint8_t write_length)
{
	i2c_pack.reg_addr = reg_addr;
	i2c_pack.slave_addr = slave_addr ;
	if(write_data != NULL)
	{
//		memcpy(i2c_pack.write_buff,write_data,write_length);
		i2c_pack.write_buff = write_data;
	}
	i2c_pack.write_length = write_length;
	if(q_device_write(i2c_dev,0,&i2c_pack,0) != RESULT_OK)
	{
		return false;
	}		
    return true;
}

bool bc_g_sensor_i2c_read(uint8_t slave_addr,uint8_t reg_addr,uint8_t *read_data,uint8_t read_length)
{
	i2c_pack.reg_addr = reg_addr;
	i2c_pack.slave_addr = slave_addr ;
	i2c_pack.read_length = read_length;
	i2c_pack.read_buff = read_data;
	if(q_device_read(i2c_dev,0,&i2c_pack,0) != RESULT_OK)
	{
		return false;
	}

	if(read_data != NULL)
	{
//		memcpy(read_data,i2c_pack.read_buff,i2c_pack.read_length);
	}
	return true;
}


void bc_g_sensor_int_io_irq_enable(void)
{

	q_device_open(int_io_dev);
	q_device_reg_callback(int_io_dev,GPIOT_CONFIG_POLARITY_HiToLo,int_io_irq_callback);	  //注册回调

}

void bc_g_sensor_int_io_irq_disable(void)
{
		q_device_close(int_io_dev);
}

void bc_g_sensor_i2c_bus_open(void)
{
	q_device_open(i2c_dev);                                                                        //打开设备
//     BC_LOG_INFO("bc g-sensor bus open \r\n");
}

void bc_g_sensor_i2c_bus_close(void)
{
	q_device_close(i2c_dev);                                                                        //关闭设备
//	BC_LOG_INFO("bc g-sensor bus close \r\n");
}

void bc_g_sensor_i2c_open(void)
{
#if (defined(HANDWARE_4_5_1) )
    bc_i2c_bus_device_open(DEVICE_BUS_TYPE_G_SENSOR);
#else	
	q_device_open(i2c_dev); 
#endif	
	                                                                       //打开设备
//     BC_LOG_INFO("bc g-sensor open \r\n");
}

void bc_g_sensor_i2c_close(void)
{

#if (defined(HANDWARE_4_5_1) )
    bc_i2c_bus_device_close(DEVICE_BUS_TYPE_G_SENSOR);
#else	
	q_device_close(i2c_dev);
#endif	
	                                                                        //关闭设备
//	BC_LOG_INFO("bc g-sensor close \r\n");
}



bool bc_g_sensor_io_irq_register_callback(void *callback)
{
	if(callback != NULL)
	{
		int_io_irq_callback = callback;
		return true;
	}
	return false;
}

/*******************************************************************************
 * Function Name     : test_io_output_led_device_find
 * Description       : 查找led设备
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
void bc_g_sensor_device_i2c_find(void)
{

#if (HARDWARE_413_ENABLED == 1 )	
	if(i2c_dev == NULL)
	{
		i2c_dev = q_device_find("i2c_0");
		q_device_assert(i2c_dev);
	}
#elif (HARDWARE_1121_ENABLED == 1 || HARDWARE_191_ENABLED == 1 || HARDWARE_1171_ENABLED == 1)	
	if(i2c_dev == NULL)
	{
		i2c_dev = q_device_find("i2c_2");
		q_device_assert(i2c_dev);
	}
#elif (HARDWARE_1231_ENABLED == 1)	


#if defined(HANDWARE_1_23_2)  
  
  if(i2c_dev == NULL)
	{
		i2c_dev = q_device_find("i2c_2");
		q_device_assert(i2c_dev);
	} 
  
#else
  if(i2c_dev == NULL)
	{
		i2c_dev = q_device_find("i2c_1");
		q_device_assert(i2c_dev);
	} 

#endif
  
	 
#elif (HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1  || HARDWARE_1141_ENABLED == 1  || HARDWARE_1181_ENABLED == 1 )	
	if(i2c_dev == NULL)
	{
		i2c_dev = q_device_find("i2c_2");
		q_device_assert(i2c_dev);
	}	
#elif (HARDWARE_451_ENABLED == 1 )	
	if(i2c_dev == NULL)
	{
		i2c_dev = q_device_find("i2c_4");
		q_device_assert(i2c_dev);
	}	
#else
	if(i2c_dev == NULL)
	{
		i2c_dev = q_device_find("i2c_0");
		q_device_assert(i2c_dev);
	}
#endif	

#if (HARDWARE_181_ENABLED == 1 )	

	
	if(cs_io_dev == NULL)
	{
		cs_io_dev = q_device_find("imu_cs");
		q_device_assert(cs_io_dev);
		q_device_open(cs_io_dev);
		q_device_ctrl(cs_io_dev,GPIO_OUTPUT_HIGH,0);
	}
	if(sdo_io_dev == NULL)
	{
		sdo_io_dev = q_device_find("imu_sdo");
		q_device_assert(sdo_io_dev);
		q_device_open(sdo_io_dev);
		q_device_ctrl(sdo_io_dev,GPIO_OUTPUT_LOW,0);
	}

#endif	
	
}








