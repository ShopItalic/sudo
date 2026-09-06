#include "bc_gsensor.h"

/* HANDWARE_1_23_3 / HANDWARE_1_23_4 使用 SPI 版本 GSensor (bc_spi_gsensor.c)，
 * 此文件（I2C 版本）在这些硬件版本下不参与编译，避免符号重定义 */
#if !defined(HANDWARE_1_23_3) && !defined(HANDWARE_1_23_4)


#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
	#include "qma6100.h"

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

	#include "icm42688.h"
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267

#include "da267.h" 

#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW

#include "lsm6sdo_port.h"
  
#endif	

#include "q_device.h"


#include "bc_g_sensor_device_port.h"
#include "bc_device_info.h"
#include "bc_ldo_switch.h"
#include "bc_delay.h"
#include "bc_rtos.h"
#include <stdlib.h>

static q_device_t *g_sensor_int_device_handler;
static void sport_count_timer_callback (void * pvParameter);

typedef void (*g_sensor_int_irq_callback)(void); 

static g_sensor_int_irq_callback g_sensor_irq_callback = NULL;

static uint8_t sport_num = 0;

static bool bc_g_sensor_acc_and_gyro_status_flag = false;

//gsensor相关定时器
static bc_rtos_timer_struct  g_sensor_timer[G_SENSOR_TIMER_NUM] = {
    {
        .timer_name = "sport_count_timer",                          //定时器名字
        .uxAutoReload = true,                                      //周期定时器
        .xTimerPeriodInTicks = 1000*5,                              //定时器时间
        .timer_callback_function = sport_count_timer_callback,      //定时器回调
    },
};


//gsensor中断回调
static void bc_g_sensor_int_callback(uint8_t pin,uint8_t pin_status)
{
//    ret_code_t err_code;
	BC_LOG_INFO("bc_g_sensor_int_callback sport_num:%d \r\n",sport_num);

	q_device_close(g_sensor_int_device_handler);
  bc_rtos_timer_start(g_sensor_timer[0].timer_handler,50);
//    bc_gsensor_irqOff();
    sport_num += 1;
	if(g_sensor_irq_callback != NULL)
	{
		g_sensor_irq_callback();
	}
}



//gsensor定时器回调
void sport_count_timer_callback (void * pvParameter)
{
    BC_LOG_INFO("sport_count_timer_callback");
    
    q_device_open(g_sensor_int_device_handler);
	q_device_reg_callback(g_sensor_int_device_handler,0,bc_g_sensor_int_callback);
//	bc_timer_stop(&g_sensor_timer[0]);
  bc_rtos_timer_stop(g_sensor_timer[0].timer_handler,50);
//	bc_gsensor_irqOn();
}

//gsensor定时器创建
void gsensor_int_timer_create(void)
{
    for(uint8_t i = 0;i < G_SENSOR_TIMER_NUM;i++)
    {
      g_sensor_timer[i].timer_handler = bc_rtos_timer_create( g_sensor_timer[i].timer_name,
													  g_sensor_timer[i].xTimerPeriodInTicks,
													 g_sensor_timer[i].uxAutoReload, 
													 (void *)g_sensor_timer[i].timer_id,
													g_sensor_timer[i].timer_callback_function);
      if(g_sensor_timer[i].timer_handler != NULL)
      {
    //		bc_rtos_timer_start(timer_struct.timer_handler,100);
        BC_LOG_INFO("create %s succeed\r\n",g_sensor_timer[i].timer_name);

      }
    }
}

//gsensor中断初始化
void bc_gsensor_int_init(void)
{
  q_device_open(g_sensor_int_device_handler);
  q_device_reg_callback(g_sensor_int_device_handler,0,bc_g_sensor_int_callback);
  gsensor_int_timer_create();
}

//gsensor中断查找
void bc_g_sensor_int_find(void)
{
  g_sensor_int_device_handler = q_device_find("acc_int_1");
  q_device_assert(g_sensor_int_device_handler);
}
/*************************************************************************/



//gsensor设备查找
void bc_g_sensor_device_find(void)
{
    bc_g_sensor_device_i2c_find();
    bc_g_sensor_int_find();
}



//gsensor初始化
enum g_sensor_result bc_gsensor_init(void)
{
#if (HARDWARE_413_ENABLED == 1)	
		
	bc_ldo_imu_power_on();
	bc_delay_ms(20);
#endif	
	
    bc_g_sensor_i2c_open();
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
		
	bc_device_six_axis_config * six_axis_config = bc_device_six_axis_config_get();
	if(qma6100_init(six_axis_config->acc_frequency) == QMA6100_FAIL)
    {
        bc_g_sensor_i2c_close();
        return 	G_SENSOR_FAILD;
    }
		
	 

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688
if(bc_g_sensor_acc_and_gyro_status())
  {
     bc_g_sensor_acc_and_gyro();
  }
  else
  {
     icm42688_init();
     bc_g_sensor_acc_and_gyro_status_flag = false;
  }
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267

   da267_init();
  
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW
  lsm6sdo_init();
  bc_gsensor_set_sport_state(25);
  lsm6sdo_disable_anymotion();
  lsm6sdo_on_and_off(false);
#endif	
    
    bc_gsensor_clearSteps();
    bc_gsensor_int_init();
    bc_g_sensor_i2c_close();
#if (HARDWARE_413_ENABLED == 1)	
		
	bc_ldo_imu_power_off();
#endif	
    return G_SENSOR_SUCCESS;
}

//gsensor初始化
enum g_sensor_result bc_gsensor_init_status(void)
{
#if (HARDWARE_413_ENABLED == 1)	
		
	bc_ldo_imu_power_on();
	bc_delay_ms(20);
#endif		
    bc_g_sensor_i2c_open();
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
	 bc_device_six_axis_config * six_axis_config = bc_device_six_axis_config_get();
	if(qma6100_init(six_axis_config->acc_frequency) == QMA6100_FAIL)
    {
        bc_g_sensor_i2c_close();
        return 	G_SENSOR_FAILD;
    }

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688
   if(bc_g_sensor_acc_and_gyro_status())
  {
//    bc_g_sensor_acc_and_gyro();
    icm42688_init();
  }
  else
  {
    
  }
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267  
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW	    
#endif	
    bc_g_sensor_acc_and_gyro_status_flag = false;

    bc_g_sensor_i2c_close();
	
#if (HARDWARE_413_ENABLED == 1)	
		
	bc_ldo_imu_power_off();
#endif		
    return G_SENSOR_SUCCESS;
}

void bc_g_sensor_irq_reg(void)
{
    bc_g_sensor_i2c_open();
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
	
	qma6100_irq_hdlr();

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW
#endif		
    
    bc_g_sensor_i2c_close();	
}

//读id
void bc_gsensor_set_sport_state(uint8_t odr)
{

 
	
    bc_g_sensor_i2c_open();
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
	
#if (HARDWARE_413_ENABLED == 1)	
		
	bc_ldo_imu_power_on();
	bc_delay_ms(20);
	id = qma6100_chip_id();
	bc_ldo_imu_power_off();
#else
	id = qma6100_chip_id();
#endif		

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

	id = get_icm42688_id();
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267
  id = da267_get_id();
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW  
  lsm6sdo_sport_state(odr);
#endif		
    
    bc_g_sensor_i2c_close();
}

//读id
uint8_t bc_gsensor_getId(void)
{
    uint8_t id = 0;
 
    bc_g_sensor_i2c_open();
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
	
#if (HARDWARE_413_ENABLED == 1)	
		
	bc_ldo_imu_power_on();
	bc_delay_ms(20);
	id = qma6100_chip_id();
	bc_ldo_imu_power_off();
#else
	id = qma6100_chip_id();
#endif		

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

	id = get_icm42688_id();
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267
  id = da267_get_id();
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW  
  id = lsm6sdo_get_chip_id();
#endif		
    
    bc_g_sensor_i2c_close();
    return id;
}

bool bc_gsensor_id_hardware_check(void)
{
    uint8_t id = 0;
    
    bc_g_sensor_i2c_open();
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
#if (HARDWARE_413_ENABLED == 1)	
		
	bc_ldo_imu_power_on();
	bc_delay_ms(20);
	id = qma6100_chip_id();
	bc_ldo_imu_power_off();
#else
	id = qma6100_chip_id();
#endif	

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

	id = get_icm42688_id();
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267  

  id = da267_get_id();
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW
#endif		
   
    bc_g_sensor_i2c_close();

#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
	if(id == 0xFA || id == 0x90)
	{
		return true;
	}
	return false;

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

	if(id == 0x47)
	{
		return true;
	}
	return false;
  
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267
  if(id == 0x13)
	{
		return true;
	}
	return false;

#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW
  if(id == 0x6C)
	{
		return true;
	}
	return false;
#endif		
}

bool bc_gsensor_hardware_check(void)
{
    int32_t pdata[3] = {0};
    bc_g_sensor_i2c_open();
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P

#if (HARDWARE_413_ENABLED == 1)	
		
	bc_ldo_imu_power_on();
	bc_delay_ms(20);
	qma6100_read_raw_xyz(pdata);
	bc_ldo_imu_power_off();
#else
	qma6100_read_raw_xyz(pdata);
#endif		
	

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

	bsp_IcmGetAccelerometer((icm42688RawData_t*)pdata);
    
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267    
    
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW    
    
#endif	
    
    bc_g_sensor_i2c_close();

    if(abs(pdata[0]) > 2400 && abs(pdata[1]) > 2400 && abs(pdata[2]) > 2400)
	{
		return false;
	}
	return true;
}

//读步数
uint32_t bc_gsensor_getStep(void)
{
    uint32_t step = 0;

    bc_g_sensor_i2c_open();
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P

#if (HARDWARE_413_ENABLED == 1)	
		
	bc_ldo_imu_power_on();
	bc_delay_ms(20);
	step = qma6100_read_stepcounter();
	bc_ldo_imu_power_off();
#else
	step = qma6100_read_stepcounter();
#endif		
	

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

	step = bsp_IcmGetStep();
  
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267  
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW  
#endif	
    
    bc_g_sensor_i2c_close();
    
    return step;
}
//清步数
void bc_gsensor_clearSteps(void)
{
    bc_g_sensor_i2c_open();
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P

#if (HARDWARE_413_ENABLED == 1)	
		
	bc_ldo_imu_power_on();
	bc_delay_ms(20);
	qma6100_clear_step();
	bc_ldo_imu_power_off();
#else
	qma6100_clear_step();
#endif	
	
	
#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688
  if(bc_g_sensor_acc_and_gyro_status())
  {
    
  }
  else
  {
    icm42688_init();
  }
  
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267  
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW	  
#endif		
    
    bc_g_sensor_i2c_close();
}



bool bc_g_sensor_acc_and_gyro_status(void)
{
	return bc_g_sensor_acc_and_gyro_status_flag;
}

void bc_g_sensor_acc_and_gyro(void)
{
    bc_g_sensor_i2c_open();
	bc_g_sensor_acc_and_gyro_status_flag = true;
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688
    bc_device_six_axis_config * six_axis_config = bc_device_six_axis_config_get();
	bsp_Icm42688RegCfg_acc_and_gyro_data(six_axis_config->acc_frequency);
  
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267  
  
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW  
#endif	
    
    bc_g_sensor_i2c_close();	
	
	
}

void bc_g_sensor_acc_and_gyro_config(uint8_t acc)
{
    bc_g_sensor_i2c_open();
	bc_g_sensor_acc_and_gyro_status_flag = true;
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688
    
//	bsp_Icm42688RegCfg_acc_and(acc);
	bsp_Icm42688RegCfg_acc_and_gyro_data(acc);
  
  
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267  
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW  
#endif	
    
    bc_g_sensor_i2c_close();	
	
	
}




//读三轴数据
void bc_gsensor_dataRead(int *pdata)
{
	
	 bc_g_sensor_i2c_open();	
	
   
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
	
	 qma6100_read_raw_xyz(pdata);

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

	bsp_IcmGetAccelerometer((icm42688RawData_t*)pdata);
  
  
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267  
  uint8_t p_num = 0;
   da267_read_fifo((da267_acc_data_t *)pdata,&p_num);
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW  
#endif	
	
	bc_g_sensor_i2c_close();
   
    
}

//读陀螺仪数据
void bc_gsensor_Gyroscope_dataRead(int *pdata)
{
    bc_g_sensor_i2c_open();
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P


#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

	 bsp_IcmGetGyroscope((icm42688RawData_t*)pdata);
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW  
#endif
	
    bc_g_sensor_i2c_close();
}

//读陀螺仪与三轴数据
void bc_gsensor_RawData_dataRead(void *pdata_Accelerometer,void *Gyroscope)
{
    bc_g_sensor_i2c_open();
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P


#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

	 bsp_IcmGetRawData((icm42688RawData_t*)pdata_Accelerometer,(icm42688RawData_t*)Gyroscope);
#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267

#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW  
  
#endif
	
    bc_g_sensor_i2c_close();
}


//开中断
void bc_gsensor_irqOn(void)
{
    bc_g_sensor_i2c_open();
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P

#if (HARDWARE_413_ENABLED == 1)	
		
	bc_ldo_imu_power_on();
	bc_delay_ms(20);
	qma6100_anymotion_config(QMA6100_MAP_INT1, QMA6100_ENABLE);
	bc_ldo_imu_power_off();
#else
	qma6100_anymotion_config(QMA6100_MAP_INT1, QMA6100_ENABLE);
#endif	
	
	
#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267	 
  
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW  
  
  
#endif	
    
    bc_g_sensor_i2c_close();
}

//关中断
void bc_gsensor_irqOff(void)
{
    bc_g_sensor_i2c_open();
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P

#if (HARDWARE_413_ENABLED == 1)	
		
	bc_ldo_imu_power_on();
	bc_delay_ms(20);
	qma6100_anymotion_config(QMA6100_MAP_INT1, QMA6100_DISABLE);
	bc_ldo_imu_power_off();
#else
	qma6100_anymotion_config(QMA6100_MAP_INT1, QMA6100_DISABLE);
#endif		
	
	
#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW	 
#endif		
    
    bc_g_sensor_i2c_close();
}



//读fifo数据
void bc_gsensor_fifoRead(int16_t rdata[][3])
{
    bc_g_sensor_i2c_open();
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P

	
#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW	 
#endif
    bc_g_sensor_i2c_close();
}

uint8_t bc_gsensor_sport_num_get(void)
{
	return sport_num;
}

void bc_gsensor_sport_num_clear(void)
{
	sport_num = 0;
}

bool bc_g_sensor_int_irq_register_callback(const void *error_callback)
{
	if(error_callback == NULL)
	{
		return false;
	}
	g_sensor_irq_callback = (g_sensor_int_irq_callback)error_callback;
	return true;

}

bool bc_g_sensor_tap_irq_register_callback(void *callback)
{
	
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
	return qm6100_tap_irq_register_callback(callback);
	
#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267	
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW  
#endif

}

bool bc_g_sensor_any_motion_irq_register_callback(void *callback)
{
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P

	return qm6100_any_motion_irq_register_callback(callback);
#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688

#elif (G_SENSOR_DEVIECE_TYPE == 2)  // LIS2DH12


#elif (G_SENSOR_DEVIECE_TYPE == 3)  // DA267
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6DSOW 
#endif	
	
}

#endif /* !defined(HANDWARE_1_23_3) && !defined(HANDWARE_1_23_4) */

