#include "app_hid_handler.h"



#include "bc_logger.h"
#include "bc_event.h"
#include "bc_timer.h"
#include "bc_gsensor.h"

#include "bc_alg_acc.h"
#include "bc_device_info.h"
#include "app_ble_handler.h"
#if (HARDWARE_153_ENABLED == 1  || HARDWARE_156_ENABLED == 1)	

#include "bc_alg_gesture.h"

#endif

#include "bc_alg_snap.h"

enum app_hid_timer_event
{
	APP_HID_PHOTOGRAPH_TIMER_EVENT = 0,

	APP_HID_TIMER_NUM
};

static bc_device_hid_info *hid_info = NULL;

static void app_hid_photograth_timer_callback(void * pvParameter);

static bc_timer_struct  timer_struct[APP_HID_TIMER_NUM] = {	
	{
		.timer_name = "photograth timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 40,
		.lock = false,
		.timer_callback_function = app_hid_photograth_timer_callback,
	},
};



struct hid_sensor_data
{
	int16_t acc[3];
	int16_t gyro[3];
};

struct hid_sensor_data sensor_data = {0};


static void app_hid_photograth_timer_callback(void * pvParameter)
{
	 if(hid_info->device_hid_type != 1 && hid_info->device_hid_gesture_mode == 0xFF)
	{
		return;
	}
	
	int32_t temp[3] = {0};
	
	switch(hid_info->device_hid_gesture_mode)
	{
		case BLE_HID_GESTURE_VIDEO_MODE:
	   {
		   if(hid_info->device_hid_enable_flag == ANDROID_HID_DEVICE)
		   {
		   }
		   else if(hid_info->device_hid_enable_flag == IOS_HID_DEVICE)
		   {
			   
		   }
		   break;
	   }
	   case BLE_HID_GESTURE_PHOTOGRAPH_MODE:
	   {
			bc_gsensor_dataRead(temp);
			bc_alg_pinch_append((int16_t)temp[0],(int16_t)temp[1],(int16_t)temp[2]);
			if(bc_alg_get_pinch_result() == 1)
			{
				app_ble_hid_volume_down();
			}	   
		   break;
	   }
	   case BLE_HID_GESTURE_MUISC_MODE:
	   {
//				   app_ble_hid_next_music();
		   break;
	   }
	   case BLE_HID_GESTURE_PPT:
	   {
#if (HARDWARE_153_ENABLED == 1  || HARDWARE_156_ENABLED == 1)	
			int16_t acc[3] = {0},gyro[3] = {0};
			GestureType gesture_type_event = 0; 

			bc_gsensor_dataRead((int*)sensor_data.acc);
			bc_gsensor_Gyroscope_dataRead((int*)sensor_data.gyro);
			
			acc[0] = sensor_data.acc[0];
			acc[1] = sensor_data.acc[1];
			acc[2] = sensor_data.acc[2];
			
			gyro[0] = sensor_data.gyro[0];
			gyro[1] = sensor_data.gyro[1];
			gyro[2] = sensor_data.gyro[2];

			gesture_type_event = alg_mouse_handler(acc,gyro);
   
			BC_LOG_INFO("gesture_type_event   %d\r\n",gesture_type_event);
				   
            switch(gesture_type_event)
			{
				case GESTURE_SWIPE_UP:
				{
					app_ble_mouse_slide_up();
					break;
				}
				case GESTURE_SWIPE_DOWN:
				{
					 app_ble_mouse_slide_down();
					break;
				}
				default:
				{
					break;
				}
			}		
#endif		   
		   break;
	   }
		case BLE_HID_GESTURE_SNAP:
		{
			bc_gsensor_dataRead((int*)sensor_data.acc);
            temp[0] = sensor_data.acc[0];
            temp[1] = sensor_data.acc[1];
            temp[2] = sensor_data.acc[2];
			if(finger_snap(temp) == FINGER_SNAP)
			{
                
				app_ble_hid_volume_down();
			}	
			
			break;
		}
	}
}



void app_hid_photograth_start(void)
{
	hid_info = bc_device_info_get_hid_info();
	if(hid_info->device_hid_type != 1 && hid_info->device_hid_gesture_mode == 0xFF)
	{
		return;
	}
  if(hid_info->device_hid_gesture_mode == 0xFF)
  {
    return;
  }
	
	if(hid_info->device_hid_gesture_mode == BLE_HID_GESTURE_PHOTOGRAPH_MODE)
	{
		bc_alg_pinch_init();
	    timer_struct[APP_HID_PHOTOGRAPH_TIMER_EVENT].xTimerPeriodInTicks = 40;
		BC_LOG_INFO("createeeeeeeeee \r\n");
	}
	else if(hid_info->device_hid_gesture_mode == BLE_HID_GESTURE_PPT)
	{
		timer_struct[APP_HID_PHOTOGRAPH_TIMER_EVENT].xTimerPeriodInTicks = 20;
		if(!bc_g_sensor_acc_and_gyro_status())
		{
			bc_g_sensor_acc_and_gyro();
		}
		BC_LOG_INFO("create hrrrrrrrr \r\n");
	}
	else if(hid_info->device_hid_gesture_mode == BLE_HID_GESTURE_SNAP)
	{
		bc_gsensor_init();
		timer_struct[APP_HID_PHOTOGRAPH_TIMER_EVENT].xTimerPeriodInTicks = 40;
	}	
	BC_LOG_INFO("create hhhhhhhhhh \r\n");
	bc_timer_start(&timer_struct[APP_HID_PHOTOGRAPH_TIMER_EVENT]);
}

void app_hid_photograth_stop(void)
{
	if(bc_g_sensor_acc_and_gyro_status())
	{
		bc_gsensor_init_status();
	}
	bc_timer_stop(&timer_struct[APP_HID_PHOTOGRAPH_TIMER_EVENT]);
}


void app_hid_handler_init(void)
{

	for(uint8_t i = 0;i < APP_HID_TIMER_NUM; i++)
	{
		if(!bc_timer_create(&timer_struct[i]))
		{
			BC_LOG_INFO("create %s fial!! \r\n",timer_struct[i].timer_name);
		}
		else
		{
			BC_LOG_INFO("create %s success!! \r\n",timer_struct[i].timer_name);
		}		
	}

}













