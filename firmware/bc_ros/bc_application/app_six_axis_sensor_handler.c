#include "app_six_axis_sensor_handler.h"


#include "bc_queue.h"
//#include "bc_event.h"
#include "bc_rtos.h"
#include "bc_logger.h"
#include "bc_gsensor.h"
#include "bc_rtc.h"

#include "app_package.h"
#include "app_ble_handler.h"

#include <string.h>


#include "ring_config.h"
#include "bc_device_info.h"
#include "bc_delay.h"

enum six_axis_sensor_status
{
	SIX_AXIS_SENSOR_IDIE = 0,
	SIX_AXIS_SENSOR_ACCELERATION,
	SIX_AXIS_SENSOR_GYRO,
	SIX_AXIS_SENSOR_ACCELERATION_ADN_GYRO,
	SIX_AXIS_SENSOR_REAL_TIME_ACCELERATION,
	SIX_AXIS_SENSOR_REAL_TIME_GYRO,
	SIX_AXIS_SENSOR_REAL_TIME_ACCELERATION_ADN_GYRO,
};


static enum six_axis_sensor_status sensor_status = SIX_AXIS_SENSOR_IDIE;

static  struct imu_sensor_package  six_axis_sensor_ppg_pcakage = {0};
static  struct imu_sensor_package  imu_sensor_dequeue = {0};

static void app_six_axis_sensor_read_timer_callback(void * pvParameter);




static bc_rtos_timer_struct  timer_struct[SIX_AXIS_SENSOR_TIME_TYPE_NUM] = {
	{
		.timer_name = "collection timeout timer",
		.uxAutoReload = true,
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_402_ENABLED|| HARDWARE_451_ENABLED == 1)	
		.xTimerPeriodInTicks = 45,
#elif (HARDWARE_153_ENABLED == 1 || HARDWARE_181_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1  || HARDWARE_156_ENABLED == 1)
	    .xTimerPeriodInTicks = 22,
	
#endif			
		
		.lock = false,
		.timer_id = SIX_AXIS_SENSOR_READ_TIME,
		.timer_callback_function = app_six_axis_sensor_read_timer_callback,
	},

	
};

static void six_axis_sensor_status_set(enum six_axis_sensor_status status)
{
	sensor_status = status;
}

static enum six_axis_sensor_status six_axis_sensor_status_get(void)
{
	return sensor_status;
}
static uint8_t temp_count = 0;

struct six_axis_sensor_data
{
	uint16_t acc[3];
	uint16_t gyro[3];
};

#define six_axis_sensor_data_max  10

struct six_axis_sensor_data  axis_sensor_data[six_axis_sensor_data_max] = {0};


static void app_six_axis_sensor_enqueue(struct imu_sensor_package *imu_sensor_pack)
{
	bc_queue_enqueue(BC_QUEUE_TYPE_IMU_SEND_DATA,imu_sensor_pack);
}

static void app_six_axis_sensor_read_timer_callback(void * pvParameter)
{
	/* 空闲状态直接返回，避免空转消耗 */
	if(six_axis_sensor_status_get() == SIX_AXIS_SENSOR_IDIE)
	{
		return;
	}
    printf("app_six_axis_sensor_read_timer_callback################\r\n");
	switch(six_axis_sensor_status_get())
	{
				
		case SIX_AXIS_SENSOR_REAL_TIME_ACCELERATION:
		{
			six_axis_sensor_ppg_pcakage.sensor_pack.data[0] = 0x00;
			int temp[3] = {0};
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
            if(temp_count < six_axis_sensor_data_max)
			{
				bc_gsensor_dataRead(temp);
				*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[temp_count*6+1] = (int16_t)temp[0];
				*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[temp_count*6+3] = (int16_t)temp[1];
				*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[temp_count*6+5] = (int16_t)temp[2];
				temp_count++;
				if(temp_count >= six_axis_sensor_data_max)
				{
					six_axis_sensor_ppg_pcakage.sensor_pack.data[0] = 0x00;
					six_axis_sensor_ppg_pcakage.length = 4+1+(temp_count*6);
					/* 统一通过 BLE 发送队列发送，避免多线程并发访问 bc_ble_send 全局缓冲区 */
					app_package_send_enqueue((struct app_cmd_package*)&six_axis_sensor_ppg_pcakage, six_axis_sensor_ppg_pcakage.length);
					temp_count = 0;
				}
			}
#elif (G_SENSOR_DEVIECE_TYPE == 1 || G_SENSOR_DEVIECE_TYPE == 4)  // ICM42688 / LSM6DSOW

			bc_gsensor_dataRead(temp);
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[1] = (int16_t)temp[0];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[3] = (int16_t)temp[1];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[5] = (int16_t)temp[2];
			six_axis_sensor_ppg_pcakage.length = 4+6+1;
			/* 统一通过 BLE 发送队列发送，避免多线程并发访问 bc_ble_send 全局缓冲区 */
			app_package_send_enqueue((struct app_cmd_package*)&six_axis_sensor_ppg_pcakage, six_axis_sensor_ppg_pcakage.length);
#endif
			

			break;
		}
		case SIX_AXIS_SENSOR_REAL_TIME_GYRO:
		{
			six_axis_sensor_ppg_pcakage.sensor_pack.data[0] = 0x00;
			int temp[3] = {0};
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P


#elif (G_SENSOR_DEVIECE_TYPE == 1 || G_SENSOR_DEVIECE_TYPE == 4)  // ICM42688 / LSM6DSOW

			bc_gsensor_Gyroscope_dataRead(temp);
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[1] = (int16_t)temp[0];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[3] = (int16_t)temp[1];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[5] = (int16_t)temp[2];
#endif			
//			
			six_axis_sensor_ppg_pcakage.length = 4+6+1;
			/* 统一通过 BLE 发送队列发送，避免多线程并发访问 bc_ble_send 全局缓冲区 */
			app_package_send_enqueue((struct app_cmd_package*)&six_axis_sensor_ppg_pcakage, six_axis_sensor_ppg_pcakage.length);
			break;
		}
		case SIX_AXIS_SENSOR_REAL_TIME_ACCELERATION_ADN_GYRO:
		{
			
			if(temp_count < six_axis_sensor_data_max)
			{
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P


#elif (G_SENSOR_DEVIECE_TYPE == 1 || G_SENSOR_DEVIECE_TYPE == 4)  // ICM42688 / LSM6DSOW
			{
				int temp_acc[3] = {0};
				int temp_gyro[3] = {0};
//				bc_gsensor_RawData_dataRead(axis_sensor_data[temp_count].acc,axis_sensor_data[temp_count].gyro);
				bc_gsensor_dataRead(temp_acc);
				bc_gsensor_Gyroscope_dataRead(temp_gyro);
				axis_sensor_data[temp_count].acc[0] = (uint16_t)(int16_t)temp_acc[0];
				axis_sensor_data[temp_count].acc[1] = (uint16_t)(int16_t)temp_acc[1];
				axis_sensor_data[temp_count].acc[2] = (uint16_t)(int16_t)temp_acc[2];
				axis_sensor_data[temp_count].gyro[0] = (uint16_t)(int16_t)temp_gyro[0];
				axis_sensor_data[temp_count].gyro[1] = (uint16_t)(int16_t)temp_gyro[1];
				axis_sensor_data[temp_count].gyro[2] = (uint16_t)(int16_t)temp_gyro[2];
			}
#endif					
				temp_count++;
				if(temp_count >= six_axis_sensor_data_max)
				{
					six_axis_sensor_ppg_pcakage.sensor_pack.data[0] = 0x00;
					memcpy(&six_axis_sensor_ppg_pcakage.sensor_pack.data[1],(uint8_t*)&axis_sensor_data,sizeof(struct six_axis_sensor_data) * six_axis_sensor_data_max);
					six_axis_sensor_ppg_pcakage.length = 4+1+(sizeof(struct six_axis_sensor_data) * six_axis_sensor_data_max);
					/* 统一通过 BLE 发送队列发送，避免多线程并发访问 bc_ble_send 全局缓冲区 */
					app_package_send_enqueue((struct app_cmd_package*)&six_axis_sensor_ppg_pcakage, six_axis_sensor_ppg_pcakage.length);
					temp_count = 0;
				}
			}
			
			break;
		}
		default:
		{
			break;
		}
	}
	
}

static bool app_six_axis_config(uint8_t subcmd)
{
	switch(subcmd)
	{
		case 7:
		{
		     uint16_t temp = *(uint16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[0];
			if(temp == 25 || temp == 50 || temp == 100 || temp == 150 || temp == 200)
			{}
			else
			{
				six_axis_sensor_ppg_pcakage.sensor_pack.data[0] = 0;
				app_package_send_enqueue((struct app_cmd_package*)&six_axis_sensor_ppg_pcakage,4+1);
				return true;
			}
			
			bc_device_six_axis_config  six_axis_config = {0};
			six_axis_config.acc_frequency = *(uint16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[0];
			six_axis_config.gyro_frequency = *(uint16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[2];
			if(bc_device_six_axis_config_set(&six_axis_config))
			{
				six_axis_sensor_ppg_pcakage.sensor_pack.data[0] = 1;
			}
			else
			{
				six_axis_sensor_ppg_pcakage.sensor_pack.data[0] = 0;
			}
			app_package_send_enqueue((struct app_cmd_package*)&six_axis_sensor_ppg_pcakage,4+1);
			bc_gsensor_init();
			bc_delay_ms(50);
			return true;
		}
		case 8:
		{
			bc_device_six_axis_config * six_axis_config = bc_device_six_axis_config_get();
			*(uint16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[0] = six_axis_config->acc_frequency;
			*(uint16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[2] = six_axis_config->gyro_frequency;
			app_package_send_enqueue((struct app_cmd_package*)&six_axis_sensor_ppg_pcakage,4+4);
			return true;
		}
		case 9:
		{
			six_axis_sensor_status_set(SIX_AXIS_SENSOR_IDIE);	/* 先设置状态，确保回调立即生效防护 */
			bc_rtos_timer_stop(timer_struct[SIX_AXIS_SENSOR_READ_TIME].timer_handler, 50);
			if(bc_g_sensor_acc_and_gyro_status())
			{
				bc_gsensor_init_status();
			}
			six_axis_sensor_ppg_pcakage.sensor_pack.data[0] = 0x01;
			app_package_send_enqueue((struct app_cmd_package*)&six_axis_sensor_ppg_pcakage,4+1);
			return true;
		}
	}
	return false;
}

void app_six_axis_sensor_event(struct app_cmd_package * pack)
{
	memcpy((uint8_t*)&six_axis_sensor_ppg_pcakage,(uint8_t*)pack,10);
	if(pack->subcmd == (uint8_t)SIX_AXIS_SENSOR_IDIE)
	{
        printf("SIX_AXIS_SENSOR_IDIE******************\r\n");
		six_axis_sensor_status_set(SIX_AXIS_SENSOR_IDIE);	/* 先设置状态，确保回调立即生效防护 */
		bc_rtos_timer_stop(timer_struct[SIX_AXIS_SENSOR_READ_TIME].timer_handler, 50);
		app_package_send_enqueue((struct app_cmd_package*)&six_axis_sensor_ppg_pcakage,4);
		if(bc_g_sensor_acc_and_gyro_status())
		{
			bc_gsensor_init_status();
		}
		return;
	}
	
	if(app_six_axis_config(pack->subcmd))
	{
		return;
	}
	
	if(six_axis_sensor_status_get() != SIX_AXIS_SENSOR_IDIE)
	{
		six_axis_sensor_ppg_pcakage.sensor_pack.data[0] = 0x01;
		app_package_send_enqueue((struct app_cmd_package*)&six_axis_sensor_ppg_pcakage,4+1);
		//busy
		return;
	}
	six_axis_sensor_status_set(pack->subcmd);
	switch(six_axis_sensor_status_get())
	{
		case SIX_AXIS_SENSOR_ACCELERATION:
		{
			six_axis_sensor_ppg_pcakage.sensor_pack.data[0] = 0x00;
			int temp[3] = {0};
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P

			bc_gsensor_dataRead(temp);
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[1] = (int16_t)temp[0];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[3] = (int16_t)temp[1];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[5] = (int16_t)temp[2];
#elif (G_SENSOR_DEVIECE_TYPE == 1 || G_SENSOR_DEVIECE_TYPE == 4)  // ICM42688 / LSM6DSOW

			bc_gsensor_dataRead(temp);
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[1] = (int16_t)temp[0];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[3] = (int16_t)temp[1];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[5] = (int16_t)temp[2];
#endif
			
			six_axis_sensor_ppg_pcakage.length = 4+6+1;
			app_ble_send((uint8_t*)&six_axis_sensor_ppg_pcakage,six_axis_sensor_ppg_pcakage.length);
			six_axis_sensor_status_set(SIX_AXIS_SENSOR_IDIE);
			break;
		}
		case SIX_AXIS_SENSOR_GYRO:
		{
			six_axis_sensor_ppg_pcakage.sensor_pack.data[0] = 0x00;
			int temp[3] = {0};
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P


#elif (G_SENSOR_DEVIECE_TYPE == 1 || G_SENSOR_DEVIECE_TYPE == 4)  // ICM42688 / LSM6DSOW

			bc_gsensor_Gyroscope_dataRead(temp);
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[1] = (int16_t)temp[0];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[3] = (int16_t)temp[1];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[5] = (int16_t)temp[2];
#endif			
//			
			six_axis_sensor_ppg_pcakage.length = 4+6+1;
			app_ble_send((uint8_t*)&six_axis_sensor_ppg_pcakage,six_axis_sensor_ppg_pcakage.length);
			six_axis_sensor_status_set(SIX_AXIS_SENSOR_IDIE);
			break;
		}
		case SIX_AXIS_SENSOR_ACCELERATION_ADN_GYRO:
		{
			six_axis_sensor_ppg_pcakage.sensor_pack.data[0] = 0x00;
			int temp_acc[3] = {0};
			int temp_gyro[3] = {0};
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P


#elif (G_SENSOR_DEVIECE_TYPE == 1 || G_SENSOR_DEVIECE_TYPE == 4)  // ICM42688 / LSM6DSOW
            bc_g_sensor_acc_and_gyro();
			bc_delay_ms(20);
			bc_gsensor_dataRead(temp_acc);
			bc_gsensor_Gyroscope_dataRead(temp_gyro);
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[1] = (int16_t)temp_acc[0];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[3] = (int16_t)temp_acc[1];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[5] = (int16_t)temp_acc[2];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[7] = (int16_t)temp_gyro[0];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[9] = (int16_t)temp_gyro[1];
			*(int16_t*)&six_axis_sensor_ppg_pcakage.sensor_pack.data[11] = (int16_t)temp_gyro[2];
#endif			
//			
			six_axis_sensor_ppg_pcakage.length = 4+12+1;
			app_ble_send((uint8_t*)&six_axis_sensor_ppg_pcakage,six_axis_sensor_ppg_pcakage.length);
			six_axis_sensor_status_set(SIX_AXIS_SENSOR_IDIE);
			if(bc_g_sensor_acc_and_gyro_status())
			{
				bc_gsensor_init_status();
			}
			break;
		}		
		case SIX_AXIS_SENSOR_REAL_TIME_ACCELERATION:
		{
			bc_device_six_axis_config * six_axis_config = bc_device_six_axis_config_get();
			/* 设置传感器输出数据率并开启加速度计 */
			bc_gsensor_set_sport_state(six_axis_config->acc_frequency);
			/* 修改定时器周期并启动 */
			bc_rtos_timer_change_period(
				timer_struct[SIX_AXIS_SENSOR_READ_TIME].timer_handler,
				1000 / six_axis_config->acc_frequency,
				50);
			bc_rtos_timer_start(timer_struct[SIX_AXIS_SENSOR_READ_TIME].timer_handler, 50);
			break;
		}
		case SIX_AXIS_SENSOR_REAL_TIME_GYRO:
		case SIX_AXIS_SENSOR_REAL_TIME_ACCELERATION_ADN_GYRO:
		{
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P


#elif (G_SENSOR_DEVIECE_TYPE == 1 || G_SENSOR_DEVIECE_TYPE == 4)  // ICM42688 / LSM6DSOW
            bc_g_sensor_acc_and_gyro();
			bc_delay_ms(20);

#endif				
			bc_device_six_axis_config * six_axis_config = bc_device_six_axis_config_get();
			/* 设置传感器输出数据率 */
			bc_gsensor_set_sport_state(six_axis_config->acc_frequency);
			/* 修改定时器周期并启动 */
			bc_rtos_timer_change_period(
				timer_struct[SIX_AXIS_SENSOR_READ_TIME].timer_handler,
				1000 / six_axis_config->acc_frequency,
				50);
			bc_rtos_timer_start(timer_struct[SIX_AXIS_SENSOR_READ_TIME].timer_handler, 50);
			break;
		}
		default:
		{
			break;
		}
	}
	
	temp_count = 0;
}

void app_six_axis_sensor_stop(void)
{
	six_axis_sensor_status_set(SIX_AXIS_SENSOR_IDIE);	/* 先设置状态，确保回调立即生效防护 */
	bc_rtos_timer_stop(timer_struct[SIX_AXIS_SENSOR_READ_TIME].timer_handler, 50);	/* 不等待，避免阻塞调用线程 */
	bc_queue_clear(BC_QUEUE_TYPE_IMU_SEND_DATA);
	if(bc_g_sensor_acc_and_gyro_status())
	{
		bc_gsensor_init_status();
	}
}

void app_six_axis_sensor_start(void)
{
	six_axis_sensor_status_set(SIX_AXIS_SENSOR_REAL_TIME_ACCELERATION_ADN_GYRO);
	bc_gsensor_init();
	bc_delay_ms(50);
	bc_rtos_timer_start(timer_struct[SIX_AXIS_SENSOR_READ_TIME].timer_handler, 50);

}

void app_imu_poll(void)
{
	if(bc_queue_dequeue(BC_QUEUE_TYPE_IMU_SEND_DATA,(void*)&imu_sensor_dequeue))
	{
		app_ble_send((uint8_t*)&imu_sensor_dequeue,imu_sensor_dequeue.length);
	}
	
}


 void app_six_axis_sensor_time_create(void)
{
	for(uint8_t i = 0;i < SIX_AXIS_SENSOR_TIME_TYPE_NUM; i++)
	{
		timer_struct[i].timer_handler = bc_rtos_timer_create(
			timer_struct[i].timer_name,
			timer_struct[i].xTimerPeriodInTicks,
			timer_struct[i].uxAutoReload,
			(void *)timer_struct[i].timer_id,
			timer_struct[i].timer_callback_function);
		if(timer_struct[i].timer_handler != NULL)
		{
			BC_LOG_INFO("create %s success!! \r\n",timer_struct[i].timer_name);
		}
		else
		{
			BC_LOG_INFO("create %s fail!! \r\n",timer_struct[i].timer_name);
		}		
	}
	
}




