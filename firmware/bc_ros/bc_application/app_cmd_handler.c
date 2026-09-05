#include "app_cmd_handler.h"

#include "app_package.h"
#include "app_ppg_handler.h"
#include "app_ppg_data_handler.h"
#include "app_authentication_handler.h"
#include "app_tsdb_handler.h"
#include "app_g_sensor_handler.h"
#include "app_led_handler.h"
#include "app_temper_handler.h"
#include "app_touch_button_handler.h"
#include "app_pdm_handler.h"
#include "app_nfc_handler.h"
#include "app_ble_speed_handler.h"
#include "app_rtc_handler.h"
#include "app_pmic_handler.h"

#if defined(HANDWARE_1_23_2)
#include "bc_gsensor.h"
#else
#include "bc_spi_gsensor.h"
#endif
#include "bc_pmic.h"
#include "bc_ppg.h"
//#include "bc_buf.h"
#include "bc_temp.h"

//#include "bc_nfc.h"
//#include "bc_buf.h"
#include "bc_rtc.h"
#include "bc_device_info.h"
#include "bc_delay.h"
#include "bc_alg.h"
#include "bc_logger.h"
#include "bc_strategy_value.h"
//#include "bc_alg.h"
//#include "bc_led.h"
#include "bc_rtos.h"

#if ( HARDWARE_1191_ENABLED == 1)	

#include "bc_wifi.h"
#endif	

#if (HARDWARE_1121_ENABLED == 1 || HARDWARE_153_ENABLED == 1  || HARDWARE_158_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)

#include "bc_ic_led.h"

#endif


#if (HARDWARE_1121_ENABLED == 1 || HARDWARE_153_ENABLED == 1  || HARDWARE_158_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)

#include "bc_touch_button.h"

#endif

#include <stdint.h>
#include <string.h>
#include "app_six_axis_sensor_handler.h"

#include "app_ble_handler.h"
#include "app_hid_handler.h"
#include "app_hardware_check.h"
#include "bc_spi_flash.h"

#include "ring_config.h"

#if (HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)

#include "app_linear_motor_handler.h"
#include "bc_linear_motor.h"
#include "app_motor_handler.h"

#endif

#if (HARDWARE_153_ENABLED == 1)	

#include "bc_piezoelectric_motor.h"  

#endif	

#if (HARDWARE_441_ENABLED == 1  || HARDWARE_413_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1191_ENABLED == 1\
|| HARDWARE_1231_ENABLED == 1)

#include "app_ppg_file_data_handler.h"

#endif

#if (HARDWARE_1121_ENABLED == 1 )

#include "bc_mouse.h"

#endif

#if ( HARDWARE_181_ENABLED == 1  )	

#include "bc_nfc_rs2323_port.h"
	
#endif

#if (HARDWARE_156_ENABLED == 1 )	
#include "bc_nfc_st25dv.h"
#endif

#if (HARDWARE_156_ENABLED == 1 || HARDWARE_158_ENABLED == 1)	
#include "bc_pressure_sensor.h"
#endif		

#if (HARDWARE_1191_ENABLED == 1 )	

#include "app_hardline_tsdb_handler.h"

#endif

#if (defined(HANDWARE_1_23_3 ) || defined(HANDWARE_1_23_4 ))
#include "bc_linear_motor_ic.h"
#endif

static uint8_t app_test_read_ppg_id_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_read_acc_id_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_read_pmic_id_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_read_volage_adv_value_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_read_temper_adc_value_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_set_ship_mode_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_read_pmic_all_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_set_ppg_led_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_reboot_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_hrm_leak_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_spo2_leak_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_ppg_gary_card_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_ppg_reflective_hrm_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_ppg_reflective_spo2_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_ppg_diag_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_read_puf_id_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_read_touch_id_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_motor_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_touch_start_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_touch_stop_callback(struct app_cmd_package * cmd_package);
static uint8_t app_open_close_led_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_led_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_ble_log_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_hardware_check_all_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_read_flash_id_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_motor_config_callback(struct app_cmd_package * cmd_package);

static uint8_t app_test_get_temper_sensor_id_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_get_mouse_id_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_read_motor_id_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_get_temper_value_callback(struct app_cmd_package * cmd_package);

static uint8_t app_test_read_nfc_id_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_read_pressure_sensor_adc_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_ble_loopback_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_ble_speed_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_ble_set_sn_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_ble_get_sn_callback(struct app_cmd_package * cmd_package);
static uint8_t app_test_ble_get_adc_and_voltage_callback(struct app_cmd_package * cmd_package);

static uint8_t app_cang_set_mac(struct app_cmd_package * cmd_package);
static uint8_t app_cang_get_mac(struct app_cmd_package * cmd_package);

static uint8_t app_test_read_cw221x_id_callback(struct app_cmd_package * cmd_package);
	
static uint8_t (*app_test_cmd_callback[TEST_CMD_NUM])(struct app_cmd_package * cmd_package) = {
	                                                                                             app_test_read_ppg_id_callback,                     //0
	                                                                                             app_test_read_acc_id_callback,                     //1
	                                                                                             app_test_read_pmic_id_callback,                    //2
	                                                                                             app_test_read_volage_adv_value_callback,                    //3
	                                                                                             app_test_read_temper_adc_value_callback,                    //4
	                                                                                             app_test_set_ship_mode_callback,                    //5
	                                                                                             app_test_callback,                    //6
	                                                                                             app_test_read_pmic_all_callback,                    //7
	                                                                                             app_test_set_ppg_led_callback,                    //8
	                                                                                             app_test_reboot_callback,                    //9
																									NULL,                    //10
																									NULL,                    //11
																									NULL,                    //12
																									NULL,                    //13
																									NULL,                    //14
																									NULL,                    //15
	                                                                                             app_test_hrm_leak_callback,                    //16
	                                                                                             app_test_spo2_leak_callback,                    //17
	                                                                                             app_test_ppg_gary_card_callback,                    //18
	                                                                                             app_test_ppg_reflective_hrm_callback,                    //19
	                                                                                             app_test_ppg_reflective_spo2_callback,                    //20
																								 app_test_ppg_diag_callback,                    //21
																								 NULL,                    //22
																								 NULL,                    //23
																								 app_test_hardware_check_all_callback,                    //24
																								 NULL,                                       //25
																								 app_test_read_puf_id_callback,                    //26
																								 app_test_read_touch_id_callback,                    //27
																								 NULL,                    //28
																								 NULL,                    //29
																								 NULL,                    //30
																								 app_test_read_nfc_id_callback,                    //31
																								 NULL,                    //32
																								 NULL,                    //33
																								 NULL,                    //34
																								 NULL,                    //35
																								 app_test_motor_callback,                    //36
																								 app_test_touch_start_callback,                    //37
																								 app_test_touch_stop_callback,                    //38
																								 app_test_led_callback,                    //39
																								 app_test_read_flash_id_callback,                    //40
																								 NULL,                                     //41
																								 app_test_ble_log_callback,                    //42
																								 app_test_read_motor_id_callback,                    //43
																								 app_test_get_temper_value_callback,                    //44
																								  NULL,                    //45
																								  NULL,                    //46
																								  NULL,                    //47
																								  app_test_get_temper_sensor_id_callback,                    //48
																								  app_test_motor_config_callback,                    //49
																								  app_test_get_mouse_id_callback,                    //50
																								  NULL,                    //51
																								  NULL,                    //52
																								  NULL,                    //53
																								  NULL,                    //54
																								  NULL,                    //55
																								  NULL,                    //56
																								  NULL,                    //57
																								  app_test_ble_loopback_callback,                    //58
																								  app_test_ble_speed_callback,                    //59
																								  NULL,                    //60
																								  app_open_close_led_callback,                    //61
																								  NULL,                    //62
																								  app_test_read_pressure_sensor_adc_callback,                    //63
                                                  NULL,                                                                    //64
																								  NULL,                    //65
																								  NULL,                    //66
																								  NULL,                    //67
																								  NULL,                    //68
																								  app_test_ble_set_sn_callback,                    //69
                                                  app_test_ble_get_sn_callback,                    //70
                                                  app_test_ble_get_adc_and_voltage_callback,                    //71
                                                  NULL,                    //72
																								  NULL,                    //73
																								  NULL,                    //74
																								  NULL,                    //75
                                                                                                  app_test_read_cw221x_id_callback,                    //76
																								  NULL,                    //77
																								  NULL,                    //78
                                                                                                  NULL,                    //79
																								  NULL,                    //80
																								  NULL,                    //81
                                                                                                  NULL,                    //82
																								  NULL,                    //83
																								  NULL,                    //84
                                                                                                  NULL,                    //85
																								  NULL,                    //86
																								  app_cang_set_mac,                    //87
                                                                                                  app_cang_get_mac,                    //88
																							   };

static uint8_t app_test_ble_get_adc_and_voltage_callback(struct app_cmd_package * cmd_package)
{
//  bc_rect_get_voltage_and_adc(cmd_package->data);
  app_package_send_enqueue(cmd_package,4+4);
  
}  

static uint8_t app_test_ble_set_sn_callback(struct app_cmd_package * cmd_package)
{
  if(bc_device_identity_info_set(cmd_package->data, cmd_package->length-4))
  {
    cmd_package->data[0] = 1;
  }
  else
  {
    cmd_package->data[0] = 0;
  }
  app_package_send_enqueue(cmd_package,5);
}

static uint8_t app_test_ble_get_sn_callback(struct app_cmd_package * cmd_package)
{
  bc_device_identity_info_get(cmd_package->data);
  //app_package_send_enqueue(cmd_package,4+21);
    app_package_send_enqueue(cmd_package,4+strlen((const char *)cmd_package->data));
}    

static uint8_t app_cang_set_mac(struct app_cmd_package * cmd_package)
{
    printf("app_cang_set_mac\r\n");
  if(bc_device_cang_mac_set(cmd_package->data))
  {
    cmd_package->data[0] = 1;
  }
  else
  {
    cmd_package->data[0] = 0;
  }
  app_package_send_enqueue(cmd_package,5);
}

static uint8_t app_cang_get_mac(struct app_cmd_package * cmd_package)
{
    printf("app_cang_get_mac\r\n");
    memset(cmd_package->data, 0, 21);
  bc_device_cang_mac_get(cmd_package->data);
    uint8_t ilen = strlen(cmd_package->data);
  app_package_send_enqueue(cmd_package,4+ilen);
} 


static uint8_t app_test_ble_loopback_callback(struct app_cmd_package * cmd_package)
{
	app_package_send_enqueue(cmd_package,4+1+cmd_package->data[0]);
}	

static uint8_t app_test_ble_speed_callback(struct app_cmd_package * cmd_package)
{
	if(cmd_package->data[0] == 1)
	{
		app_ble_speed_test_start(cmd_package->data[1]);
	}
	else if(cmd_package->data[0] == 0)
	{
		app_ble_speed_test_stop();
	}
}                                                 
static uint8_t app_test_read_pressure_sensor_adc_callback(struct app_cmd_package * cmd_package)
{
#if (HARDWARE_156_ENABLED == 1 || HARDWARE_158_ENABLED == 1)	
	*(uint16_t*)&cmd_package->data[0] = bc_pressure_sensor_get_adc_value();
	app_package_send_enqueue(cmd_package,4+2);
#endif		
}	
																							   
static uint8_t app_test_read_nfc_id_callback(struct app_cmd_package * cmd_package)
{
#if (HARDWARE_156_ENABLED == 1 || HARDWARE_158_ENABLED == 1)	
	cmd_package->data[0] = bc_nfc_st25dv_device_chip_id_get();
	app_package_send_enqueue(cmd_package,5);
#endif		
}

static uint8_t app_test_read_cw221x_id_callback(struct app_cmd_package * cmd_package)
{
#if (defined(HANDWARE_1_23_3 ) || defined(HANDWARE_1_23_4 ))
    if(false == app_cw221x_get_id(cmd_package)) {
        bc_delay_ms(1);
        if(false == app_cw221x_get_id(cmd_package)) {
            cmd_package->data[0] =  0xff;
            app_package_send_enqueue(cmd_package,5);
        }
    }
#endif
#if (HARDWARE_153_ENABLED == 1 )	
	cmd_package->data[0] = bc_piezoelectric_motor_chip_id_get();
	app_package_send_enqueue(cmd_package,5);
#endif		
}
																							   
static uint8_t app_test_read_motor_id_callback(struct app_cmd_package * cmd_package)
{
#if (defined(HANDWARE_1_23_3 ) || defined(HANDWARE_1_23_4 ))
    app_linear_motor_get_id(cmd_package);
#endif
#if (HARDWARE_153_ENABLED == 1 )	
	cmd_package->data[0] = bc_piezoelectric_motor_chip_id_get();
	app_package_send_enqueue(cmd_package,5);
#endif		
}																							   
static uint8_t app_test_get_temper_sensor_id_callback(struct app_cmd_package * cmd_package)
{
#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4 ))
    BC_LOG_INFO("app_test_get_temper_sensor_id_callback \r\n");
    if(false == bc_temper_get_id(cmd_package)) {
        bc_delay_ms(1);
        if(false == bc_temper_get_id(cmd_package)) {
            cmd_package->data[0] = 3;
            cmd_package->data[1] =  0xff;
            cmd_package->data[2] =  0xff;
            cmd_package->data[3] =  0xff;
            app_package_send_enqueue(cmd_package,8);
        }
    }
#else
	bc_temper_id_get(cmd_package->data);
	app_package_send_enqueue(cmd_package,8);
#endif
}

static uint8_t app_test_get_temper_value_callback(struct app_cmd_package * cmd_package)
{
	bc_temper_value_get(cmd_package->data);
	app_package_send_enqueue(cmd_package,4+(cmd_package->data[0] *2)+1);
}


static uint8_t app_test_get_mouse_id_callback(struct app_cmd_package * cmd_package)
{
#if (HARDWARE_1121_ENABLED == 1 )

	cmd_package->data[0] = bc_mouse_id_get();
#endif

	app_package_send_enqueue(cmd_package,5);
}	

/*******************************************************************************
 * Function Name     : app_test_read_flash_id_callback
 * Description       : 测试flash id指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : WJS
 * Modified Date:    : 2024年7月11日
 *******************************************************************************/
static uint8_t app_test_motor_config_callback(struct app_cmd_package * cmd_package)
{
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	
	if(cmd_package->data[0] == 1)
	{
        #if defined(HANDWARE_1_23_3)
        bc_linear_motor_config((*(uint16_t*)&cmd_package->data[1]),cmd_package->data[3],(*(uint16_t*)&cmd_package->data[4]));
        #else
		bc_linear_motor_strong_vibration_pwm_config((*(uint16_t*)&cmd_package->data[1]),cmd_package->data[3],(*(uint16_t*)&cmd_package->data[4]));
        #endif
	}
	else if(cmd_package->data[0] == 2)
	{
		bc_linear_motor_continuous_vibration_pwm_config((*(uint16_t*)&cmd_package->data[1]),cmd_package->data[3],(*(uint16_t*)&cmd_package->data[4]));
	}
#endif	
}
																							   
static uint8_t app_cmd_set_and_get_sys_info(struct app_cmd_package * cmd_package)
{
		uint8_t  sys_type = cmd_package->data[9];
		uint64_t u64recv_unix_time_s = *(uint64_t*)cmd_package->data / 1000;
		bc_rtc_time_set_uinx_time((uint32_t)u64recv_unix_time_s,cmd_package->data[8]);
		bc_device_info_app_update_time_set((uint32_t)u64recv_unix_time_s,cmd_package->data[8]);
			
			//软件版本 [0:9]:固件版本号

#if (HARDWARE_411_ENABLED == 1)
			
			memcpy(&cmd_package->data[0], RING_411_SOFTWARE_VERSION, 10);
			
#elif (HARDWARE_412_ENABLED == 1)
		    
#if defined(RONG_WEI_Z2X)

   memcpy(&cmd_package->data[0], RING_RONG_WEI_Z2X_412_SOFTWARE_VERSION, 10);
#else
   memcpy(&cmd_package->data[0], RING_412_SOFTWARE_VERSION, 10);	
#endif				
#elif (HARDWARE_421_ENABLED == 1)
            memcpy(&cmd_package->data[0], RING_421_SOFTWARE_VERSION, 10);
			
#elif (HARDWARE_441_ENABLED == 1)
            memcpy(&cmd_package->data[0], RING_441_SOFTWARE_VERSION, 10);			
			
#elif (HARDWARE_402_ENABLED == 1)
            memcpy(&cmd_package->data[0], RING_402_SOFTWARE_VERSION, 10);		
#elif (HARDWARE_153_ENABLED == 1)	

		memcpy(&cmd_package->data[0], RING_153_SOFTWARE_VERSION, 10);	
#elif (HARDWARE_1121_ENABLED == 1)	

		memcpy(&cmd_package->data[0], RING_1121_SOFTWARE_VERSION, 10);			
#elif (HARDWARE_BCL601_151_ENABLED == 1)	

		memcpy(&cmd_package->data[0], RING_BCL601_151_SOFTWARE_VERSION, 10);			
#elif (HARDWARE_413_ENABLED == 1)
            memcpy(&cmd_package->data[0], RING_413_SOFTWARE_VERSION, 10);	
#elif (HARDWARE_181_ENABLED == 1)	
		memcpy(&cmd_package->data[0], RING_181_SOFTWARE_VERSION, 10);
#elif (HARDWARE_158_ENABLED == 1)	
		memcpy(&cmd_package->data[0], RING_158_SOFTWARE_VERSION, 10);
#elif (HARDWARE_451_ENABLED == 1)
            memcpy(&cmd_package->data[0], RING_451_SOFTWARE_VERSION, 10);	
#elif (HARDWARE_156_ENABLED == 1)	
		memcpy(&cmd_package->data[0], RING_156_SOFTWARE_VERSION, 10);	
#elif (HARDWARE_1141_ENABLED == 1)	
		memcpy(&cmd_package->data[0], RING_1141_SOFTWARE_VERSION, 10);		
#elif (HARDWARE_1171_ENABLED == 1)	
		memcpy(&cmd_package->data[0], RING_1171_SOFTWARE_VERSION, 10);	 
#elif (HARDWARE_1181_ENABLED == 1)	
		memcpy(&cmd_package->data[0], RING_1181_SOFTWARE_VERSION, 10);	  
#elif (HARDWARE_1191_ENABLED == 1)	
		memcpy(&cmd_package->data[0], RING_1191_SOFTWARE_VERSION, 10);	    
#elif (HARDWARE_1231_ENABLED == 1)	
		  
#if defined(HANDWARE_1_23_2)   
#if defined(HANDWARE_1_23_2L)
  memcpy(&cmd_package->data[0], RING_1232L_SOFTWARE_VERSION, 10);	 
#elif defined(HANDWARE_1_23_2_ONE_SEC)
    memcpy(&cmd_package->data[0], RING_1232_ONE_SEC_SOFTWARE_VERSION, 10);
#else
    memcpy(&cmd_package->data[0], RING_1232_SOFTWARE_VERSION, 10);
#endif
#elif defined(HANDWARE_1_23_3)   
    memcpy(&cmd_package->data[0], RING_1233_SOFTWARE_VERSION, 10);
#elif defined(HANDWARE_1_23_4)   
    memcpy(&cmd_package->data[0], RING_1234_SOFTWARE_VERSION, 10);
#else
  memcpy(&cmd_package->data[0], RING_1231_SOFTWARE_VERSION, 10);	
#endif      
    
#endif
			
          //  硬件版本 硬件版本号
#if (HARDWARE_411_ENABLED == 1)			
			memcpy(&cmd_package->data[10], RING_411_HARDWARE_VERSION, 10);			
#elif (HARDWARE_412_ENABLED == 1)
		    memcpy(&cmd_package->data[10], RING_412_HARDWARE_VERSION, 10);
			
#elif (HARDWARE_441_ENABLED == 1)
		    memcpy(&cmd_package->data[10], RING_441_HARDWARE_VERSION, 10);			
			
#elif (HARDWARE_421_ENABLED == 1)
			
            memcpy(&cmd_package->data[10], RING_421_HARDWARE_VERSION, 10);
			
#elif (HARDWARE_402_ENABLED == 1)
			
            memcpy(&cmd_package->data[10], RING_402_HARDWARE_VERSION, 10);	
#elif (HARDWARE_153_ENABLED == 1)		
			
			memcpy(&cmd_package->data[10], RING_153_HARDWARE_VERSION, 10);
#elif (HARDWARE_1121_ENABLED == 1)		
			
			memcpy(&cmd_package->data[10], RING_1121_HARDWARE_VERSION, 10);			
#elif (HARDWARE_BCL601_151_ENABLED == 1)	

           memcpy(&cmd_package->data[10], RING_BCL601_151_HARDWARE_VERSION, 10);
#elif (HARDWARE_413_ENABLED == 1)					
			memcpy(&cmd_package->data[10], RING_413_HARDWARE_VERSION, 10);	
#elif (HARDWARE_181_ENABLED == 1)				
			memcpy(&cmd_package->data[10], RING_181_HARDWARE_VERSION, 10);
#elif (HARDWARE_158_ENABLED == 1)				
			memcpy(&cmd_package->data[10], RING_158_HARDWARE_VERSION, 10);	
#elif (HARDWARE_451_ENABLED == 1)
		    memcpy(&cmd_package->data[10], RING_451_HARDWARE_VERSION, 10);	
#elif (HARDWARE_156_ENABLED == 1)				
			memcpy(&cmd_package->data[10], RING_156_HARDWARE_VERSION, 10);		
#elif (HARDWARE_1141_ENABLED == 1)				
			memcpy(&cmd_package->data[10], RING_1141_HARDWARE_VERSION, 10);		   
#elif (HARDWARE_1171_ENABLED == 1)				
			memcpy(&cmd_package->data[10], RING_1171_HARDWARE_VERSION, 10);	
#elif (HARDWARE_1181_ENABLED == 1)				
			memcpy(&cmd_package->data[10], RING_1181_HARDWARE_VERSION, 10);	     
#elif (HARDWARE_1191_ENABLED == 1)				
			memcpy(&cmd_package->data[10], RING_1191_HARDWARE_VERSION, 10);	  
#elif (HARDWARE_1231_ENABLED == 1)		

#if defined(HANDWARE_1_23_2)   
#if defined(HANDWARE_1_23_2L)
      memcpy(&cmd_package->data[10], RING_1232L_HARDWARE_VERSION, 10);	
#elif defined(HANDWARE_1_23_2_ONE_SEC)
    memcpy(&cmd_package->data[10], RING_1232_ONE_SEC_HARDWARE_VERSION, 10);
#else
        memcpy(&cmd_package->data[10], RING_1232_HARDWARE_VERSION, 10);
#endif
#elif defined(HANDWARE_1_23_3)   
    memcpy(&cmd_package->data[10], RING_1233_HARDWARE_VERSION, 10);
#elif defined(HANDWARE_1_23_4)   
    memcpy(&cmd_package->data[10], RING_1234_HARDWARE_VERSION, 10);
#else
      memcpy(&cmd_package->data[10], RING_1231_HARDWARE_VERSION, 10);	
#endif
			      
#endif		

             //  充电状态
			cmd_package->data[21] = bc_pmic_get_charge_status();
			
			 //  电量
             #if (defined(HANDWARE_1_23_3 ) || defined(HANDWARE_1_23_4 ))
			cmd_package->data[20] = precent;
            #else
            cmd_package->data[20] = getvpct();//bc_pmic_get_vbat_percen();
            #endif
	
			//  当前采集间隔
			uint32_t temp = bc_get_business_strategy_value(BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_TIME);
			memcpy(&cmd_package->data[22], (uint8_t*)&temp, sizeof(uint32_t));
			
//			uint16_t temp_step = app_g_sensor_sport_step_count_get();
//			memcpy(&cmd_package->data[26], (uint8_t*)&temp_step, sizeof(uint16_t));
			
//			uint16_t temp_check = app_hardware_check_all();
//			memcpy(&cmd_package->data[28], (uint8_t*)&temp_check, sizeof(uint16_t));
			
			//[26:42]:当前HID功能码
			bc_device_hid_info *hid_info;
			hid_info = bc_device_info_get_hid_info();
			switch(sys_type)
			{
				case 0:  //android 
				{
				
					struct touch_hid_info touch_info = {0};
					touch_info.short_video = ANDROID_TOUCH_SHORT_VIDEO_HID;
					touch_info.photograph = ANDROID_TOUCH_PHOTOGRAPH_HID;
					touch_info.music = ANDROID_TOUCH_MUSIC_HID;
					touch_info.ppt = ANDROID_TOUCH_PPT_HID;
					touch_info.up_audio = ANDROID_TOUCH_UP_AUDIO_HID;
					
					struct gesture_hid_info  gesture_info = {0};
					gesture_info.short_video = ANDROID_GESTURE_SHORT_VIDEO_HID;
					gesture_info.photograph = ANDROID_GESTURE_PHOTOGRAPH_HID;
					gesture_info.music = ANDROID_GESTURE_MUSIC_HID;
					gesture_info.ppt = ANDROID_GESTURE_PPT_HID;
					gesture_info.snap = ANDROID_GESTURE_SNAP_HID;					
					
#if defined(RONG_WEI_Z2X)

                   touch_info.music = 0;
					touch_info.ppt = 0;
					
				
#endif					
					cmd_package->data[30] = hid_info->device_hid_type;

					memcpy(&cmd_package->data[31],(uint8_t*)&touch_info,sizeof(struct touch_hid_info));

					memcpy(&cmd_package->data[39],(uint8_t*)&gesture_info,sizeof(struct gesture_hid_info));

					break;
				}
				case 1:
				{
					struct touch_hid_info touch_info = {0};
					touch_info.short_video = IOS_TOUCH_SHORT_VIDEO_HID;
					touch_info.photograph = IOS_TOUCH_PHOTOGRAPH_HID;
					touch_info.music = IOS_TOUCH_MUSIC_HID;
					touch_info.ppt = IOS_TOUCH_PPT_HID;
					touch_info.up_audio = IOS_TOUCH_UP_AUDIO_HID;
					
					struct gesture_hid_info  gesture_info;
					gesture_info.short_video = IOS_GESTURE_SHORT_VIDEO_HID;
					gesture_info.photograph = IOS_GESTURE_PHOTOGRAPH_HID;
					gesture_info.music = IOS_GESTURE_MUSIC_HID;
					gesture_info.ppt = IOS_GESTURE_PPT_HID;
					gesture_info.snap = IOS_GESTURE_SNAP_HID;
					
					cmd_package->data[30] = hid_info->device_hid_type;
					
#if defined(RONG_WEI_Z2X)

                   touch_info.music = 0;
				   touch_info.ppt = 0;
				
#endif						
					memcpy(&cmd_package->data[31],(uint8_t*)&touch_info,sizeof(struct touch_hid_info));
					memcpy(&cmd_package->data[39],(uint8_t*)&gesture_info,sizeof(struct gesture_hid_info));
					break;
				}
				case 2:
				{
					break;
				}
				case 3:
				{
					break;
				}
			}			
			//[43:45]:当前HID模式
			cmd_package->data[47] = hid_info->device_hid_touch_mode ;
			cmd_package->data[48] = hid_info->device_hid_gesture_mode ;
			cmd_package->data[49] = hid_info->device_hid_enable_flag ;
			
			
		
	
#if ( HARDWARE_181_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	
           //[46]:心率曲线支持
             cmd_package->data[50] = 0;
			//[47]:血氧曲线支持
			cmd_package->data[51] = 0;
            cmd_package->data[52] = 0;
			//[49]:压力曲线支持
             cmd_package->data[53] = 0;
			
			//[50]:温度曲线支持
			cmd_package->data[54] = 0;
#else 
			//[46]:心率曲线支持
             cmd_package->data[50] = 1;
			//[47]:血氧曲线支持
			cmd_package->data[51] = 1;
            cmd_package->data[52] = 1;
			//[49]:压力曲线支持
             cmd_package->data[53] = 1;
			
			//[50]:温度曲线支持
			cmd_package->data[54] = 1;
#endif				
			
			
			
			
			//[51]:女性健康支持
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1|| HARDWARE_153_ENABLED == 1  || HARDWARE_181_ENABLED == 1  || \
     HARDWARE_441_ENABLED == 1 ||HARDWARE_1141_ENABLED == 1 || HARDWARE_402_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1|| \
     HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_1181_ENABLED == 1)	
             cmd_package->data[55] = 0;
#endif			

			//[52]:震动闹钟支持
#if ( HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1)	
             cmd_package->data[56] = 1;
#else 
             cmd_package->data[56] = 0;
#endif				
			// 53]: 心电图功能支持
			cmd_package->data[57] = 0;
			
			// [54]:  麦克风支持
#if ( HARDWARE_153_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1191_ENABLED == 1|| HARDWARE_1231_ENABLED == 1)	
             cmd_package->data[58] = 1;
#else 
             cmd_package->data[58] = 0;
#endif				
			// [55]: 运动模式支持
             cmd_package->data[59] = 0;	
}	


																							   
/*******************************************************************************
 * Function Name     : app_test_read_flash_id_callback
 * Description       : 测试flash id指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : WJS
 * Modified Date:    : 2024年7月11日
 *******************************************************************************/
static uint8_t app_test_read_flash_id_callback(struct app_cmd_package * cmd_package)
{
	*(uint32_t*)&cmd_package->data[0] = spi_flash_device_get_id();
	app_package_send_enqueue(cmd_package,8);
}

static uint8_t app_test_hardware_check_all_callback(struct app_cmd_package * cmd_package)
{
	*(uint16_t*)cmd_package->data = app_hardware_check_all();
	app_package_send_enqueue(cmd_package,6);
	return 0;
}	
																							   
static uint8_t app_test_ble_log_callback(struct app_cmd_package * cmd_package)
{
	if(cmd_package->data[0] == 0)
	{
		bc_log_ble_disenable();
	}
	else if(cmd_package->data[0] == 1)
	{
		bc_log_ble_enable();
	}
	app_package_send_enqueue(cmd_package,5);
	return 0;
}

static uint8_t app_open_close_led_callback(struct app_cmd_package * cmd_package)
{
    if(cmd_package->data[0])
        bc_ic_led_test_cmd(10,10,10);
    else
        bc_ic_led_test_cmd(0,0,0);
}

static uint8_t app_test_led_callback(struct app_cmd_package * cmd_package)
{
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_1121_ENABLED == 1  || HARDWARE_158_ENABLED == 1 || HARDWARE_1231_ENABLED == 1 )		
	bc_ic_led_test_cmd(cmd_package->data[1],cmd_package->data[0],cmd_package->data[2]);
#else	
	if(cmd_package->data[0])
	{
		bc_led_red_on();
	}
	else
	{
		bc_led_red_off();
	}
	
	if(cmd_package->data[1])
	{
		
	}
	else
	{
		
	}
	
	if(cmd_package->data[2])
	{
		bc_led_blue_on();
	}
	else
	{
		bc_led_blue_off();
	}	
#endif		
	
	
	return 0;
}																							   
																							   
static uint8_t app_test_motor_callback(struct app_cmd_package * cmd_package)
{
//	bc_piezoelectric_motor_check();
#if defined(HANDWARE_1_23_2)    
    bc_linear_motor_start(LINEAR_MOTOR_MIC_START);

#elif defined(HANDWARE_1_23_3)
    app_linear_motor_ic_start(1);
#endif
	
#if (HARDWARE_153_ENABLED == 1  )		
	
	app_motor_play_test();
	
#endif		
	return 0;
}

static uint8_t app_test_touch_start_callback(struct app_cmd_package * cmd_package)
{
//	app_button_handware_chek_start();
	return 0;
}	

static uint8_t app_test_touch_stop_callback(struct app_cmd_package * cmd_package)
{
//	app_button_handware_chek_stop();
	return 0;
}	
																							   
/*******************************************************************************
 * Function Name     : app_test_read_ppg_id_callback
 * Description       : 测试ppg id指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_read_ppg_id_callback(struct app_cmd_package * cmd_package)
{
#if(PPG_ENABLED)   
	cmd_package->data[0] = bc_ppg_chip_id_get();
#endif  
	app_package_send_enqueue(cmd_package,5);
	return 0;
}
/*******************************************************************************
 * Function Name     : app_test_read_acc_id_callback
 * Description       : 测试g_sensor id指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_read_acc_id_callback(struct app_cmd_package * cmd_package)
{
	cmd_package->data[0] = bc_gsensor_getId();
	app_package_send_enqueue(cmd_package,5);
	return 0;
}



/*******************************************************************************
 * Function Name     : app_test_read_pmic_id_callback
 * Description       : 测试pmic id指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_read_pmic_id_callback(struct app_cmd_package * cmd_package)
{
	bc_pmic_get_id(&cmd_package->data[0]);
	app_package_send_enqueue(cmd_package,5);
	return 0;
}
/*******************************************************************************
 * Function Name     : app_test_read_puf_id_callback
 * Description       : 测试puf id指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_read_puf_id_callback(struct app_cmd_package * cmd_package)
{
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1)		
	cmd_package->data[0] = bc_buf_chip_id_get();
#endif	
	
	app_package_send_enqueue(cmd_package,5);
}
/*******************************************************************************
 * Function Name     : app_test_read_touch_id_callback
 * Description       : 测试touch id指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_read_touch_id_callback(struct app_cmd_package * cmd_package)
{
#if ( HARDWARE_1181_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)		
	cmd_package->data[0] = bc_touch_button_chip_id_get();
#endif	  
	
	app_package_send_enqueue(cmd_package,5);
}
/*******************************************************************************
 * Function Name     : app_test_read_volage_adv_value_callback
 * Description       : 测试vbat adc值指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_read_volage_adv_value_callback(struct app_cmd_package * cmd_package)
{
	*(uint16_t*)cmd_package->data = bc_pmic_get_adc_value();
	app_package_send_enqueue(cmd_package,6);
	return 0;
}
/*******************************************************************************
 * Function Name     : app_test_read_temper_adc_value_callback
 * Description       : 测试温度adc值指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_read_temper_adc_value_callback(struct app_cmd_package * cmd_package)
{
	*(uint16_t*)cmd_package->data = bc_temp_get_temper_adc_value();
	app_package_send_enqueue(cmd_package,6);
	return 0;
}
/*******************************************************************************
 * Function Name     : app_test_set_ship_mode_callback
 * Description       : 测试pmic ship mode指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_set_ship_mode_callback(struct app_cmd_package * cmd_package)
{
	app_package_send_enqueue(cmd_package,4);
	bc_rtos_delay(100);
	bc_pmic_set_shipmode();
	return 0;
}

/*******************************************************************************
 * Function Name     : app_test_callback
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_callback(struct app_cmd_package * cmd_package)
{
	return 0;
}
/*******************************************************************************
 * Function Name     : app_test_read_pmic_all_callback
 * Description       : 测试pmic 寄存器相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_read_pmic_all_callback(struct app_cmd_package * cmd_package)
{
	bc_pmic_read_all(&cmd_package->data[0]);
	app_package_send_enqueue(cmd_package,16);
	return 0;
}
/*******************************************************************************
 * Function Name     : app_test_set_ppg_led_callback(
 * Description       : 测试ppg led相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_set_ppg_led_callback(struct app_cmd_package * cmd_package)
{
	if(cmd_package->data[0] == 1)
	{
#if(PPG_ENABLED)     
		bc_ppg_gre_led_on();
#endif    
	}
	else if(cmd_package->data[1] == 1)
	{
#if(PPG_ENABLED)     
		bc_ppg_red_led_on();
#endif		
	}
	else if(cmd_package->data[2] == 1)
	{
#if(PPG_ENABLED)     
		bc_ppg_ir_led_on();
#endif    
	}
	else
	{
#if(PPG_ENABLED)     
		bc_ppg_led_off();
#endif
	}
	app_package_send_enqueue(cmd_package,4);
	return 0;
}


/*******************************************************************************
 * Function Name     : app_test_reboot_callback
 * Description       : 测试reboot相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_reboot_callback(struct app_cmd_package * cmd_package)
{
	app_package_send_enqueue(cmd_package,4);
  app_rtc_ushut_down_time_record();
	bc_rtos_delay(2000);
	 NVIC_SystemReset();
	return 0;
}
/*******************************************************************************
 * Function Name     : app_test_hrm_leak_callback
 * Description       : 测试心率相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_hrm_leak_callback(struct app_cmd_package * cmd_package)
{
	return 0;
}
/*******************************************************************************
 * Function Name     : app_test_ppg_gary_card_callback
 * Description       : 测试血氧相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_spo2_leak_callback(struct app_cmd_package * cmd_package)
{
	return 0;
}
/*******************************************************************************
 * Function Name     : app_test_ppg_gary_card_callback
 * Description       : 测试灰卡相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_ppg_gary_card_callback(struct app_cmd_package * cmd_package)
{
#if(PPG_ENABLED)   
	app_ppg_gary_card_test_cmd_start(cmd_package);
#endif  
	return 0;
}




/*******************************************************************************
 * Function Name     : app_test_ppg_reflective_hrm_callback
 * Description       : 测试心率相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_ppg_reflective_hrm_callback(struct app_cmd_package * cmd_package)
{
	
//	app_ppg_hrm_test_ble_cmd_start(cmd_package);
	return 0;
}
/*******************************************************************************
 * Function Name     : app_test_ppg_reflective_spo2_callback
 * Description       : 测试血氧相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_ppg_reflective_spo2_callback(struct app_cmd_package * cmd_package)
{
//	app_ppg_spo2_test_ble_cmd_start(cmd_package);
	return 0;
}
/*******************************************************************************
 * Function Name     : app_cmd_set_time_callback
 * Description       : 测试诊断相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_test_ppg_diag_callback(struct app_cmd_package * cmd_package)
{
//	app_ppg_diag(cmd_package);
	return 0;
}


/*******************************************************************************
 * Function Name     : app_cmd_set_time_callback
 * Description       : 时间相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint8_t app_cmd_set_time_callback(struct app_cmd_package * cmd_package)
{
	switch(cmd_package->subcmd)
	{
		case 0:
		{
			uint64_t u64recv_unix_time_s = *(uint64_t*)cmd_package->data / 1000;
            bc_rtc_time_set_uinx_time((uint32_t)u64recv_unix_time_s,cmd_package->data[8]);
			bc_device_info_app_update_time_set((uint32_t)u64recv_unix_time_s,cmd_package->data[8]);
			app_package_send_enqueue(cmd_package,4);
			break;
		}
		case 1:
		{
			uint64_t unix_time_s =  (uint64_t)bg_rtc_time_get_uinx_time() * 1000;
			*(uint64_t*)cmd_package->data = unix_time_s;
			cmd_package->data[8] = 0x08;
			app_package_send_enqueue(cmd_package,13);
			
			break;
		}
    case 2:
		{
			*(uint64_t*)&cmd_package->data[16] = bg_rtc_time_get_uinx_ms_time();
			app_ble_send((uint8_t *)cmd_package,4+24);
			break;
		}
	}
	return 0;
}
/*******************************************************************************
 * Function Name     : app_cmd_get_spo2
 * Description       : 获取软件版本
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_get_version_callback(struct app_cmd_package * cmd_package)
{
	switch(cmd_package->subcmd)
	{
		case 0:
		{
#if (HARDWARE_411_ENABLED == 1)
			
			memcpy(&cmd_package->data[0], RING_411_SOFTWARE_VERSION, 10);
			
#elif (HARDWARE_412_ENABLED == 1)
		    
#if defined(RONG_WEI_Z2X)

   memcpy(&cmd_package->data[0], RING_RONG_WEI_Z2X_412_SOFTWARE_VERSION, 10);
#else
   memcpy(&cmd_package->data[0], RING_412_SOFTWARE_VERSION, 10);	
#endif				
#elif (HARDWARE_421_ENABLED == 1)
            memcpy(&cmd_package->data[0], RING_421_SOFTWARE_VERSION, 10);
			
#elif (HARDWARE_441_ENABLED == 1)
            memcpy(&cmd_package->data[0], RING_441_SOFTWARE_VERSION, 10);			
			
#elif (HARDWARE_402_ENABLED == 1)
            memcpy(&cmd_package->data[0], RING_402_SOFTWARE_VERSION, 10);		
#elif (HARDWARE_153_ENABLED == 1)	

		memcpy(&cmd_package->data[0], RING_153_SOFTWARE_VERSION, 10);	
#elif (HARDWARE_1121_ENABLED == 1)	

		memcpy(&cmd_package->data[0], RING_1121_SOFTWARE_VERSION, 10);		
#elif (HARDWARE_BCL601_151_ENABLED == 1)	

		memcpy(&cmd_package->data[0], RING_BCL601_151_SOFTWARE_VERSION, 10);	
#elif (HARDWARE_413_ENABLED == 1)	

		memcpy(&cmd_package->data[0], RING_413_SOFTWARE_VERSION, 10);	
#elif (HARDWARE_181_ENABLED == 1)	

		memcpy(&cmd_package->data[0], RING_181_SOFTWARE_VERSION, 10);		
#elif (HARDWARE_158_ENABLED == 1)	

		memcpy(&cmd_package->data[0], RING_158_SOFTWARE_VERSION, 10);		
#elif (HARDWARE_451_ENABLED == 1)
            memcpy(&cmd_package->data[0], RING_451_SOFTWARE_VERSION, 10);	
#elif (HARDWARE_156_ENABLED == 1)	
		memcpy(&cmd_package->data[0], RING_156_SOFTWARE_VERSION, 10);			
#elif (HARDWARE_1141_ENABLED == 1)	
		memcpy(&cmd_package->data[0], RING_1141_SOFTWARE_VERSION, 10);	
#elif (HARDWARE_1171_ENABLED == 1)	
		memcpy(&cmd_package->data[0], RING_1171_SOFTWARE_VERSION, 10);	  
#elif (HARDWARE_1181_ENABLED == 1)	
		memcpy(&cmd_package->data[0], RING_1181_SOFTWARE_VERSION, 10);    
#elif (HARDWARE_1191_ENABLED == 1)	
		memcpy(&cmd_package->data[0], RING_1191_SOFTWARE_VERSION, 10);      
#elif (HARDWARE_1231_ENABLED == 1)	

#if defined(HANDWARE_1_23_2)   
#if defined(HANDWARE_1_23_2L)
  memcpy(&cmd_package->data[0], RING_1232L_SOFTWARE_VERSION, 10);	 
#elif defined(HANDWARE_1_23_2_ONE_SEC)
    memcpy(&cmd_package->data[0], RING_1232_ONE_SEC_SOFTWARE_VERSION, 10);
#else
    memcpy(&cmd_package->data[0], RING_1232_SOFTWARE_VERSION, 10);
#endif
#elif defined(HANDWARE_1_23_3)
    memcpy(&cmd_package->data[0], RING_1233_SOFTWARE_VERSION, 10);
#elif defined(HANDWARE_1_23_4)
    memcpy(&cmd_package->data[0], RING_1234_SOFTWARE_VERSION, 10);
#else
  memcpy(&cmd_package->data[0], RING_1231_SOFTWARE_VERSION, 10);	
#endif
		      
#endif
			
			break;
		}
		case 1:
		{
#if (HARDWARE_411_ENABLED == 1)
			
			memcpy(&cmd_package->data[0], RING_411_HARDWARE_VERSION, 10);
			
#elif (HARDWARE_412_ENABLED == 1)
		    memcpy(&cmd_package->data[0], RING_412_HARDWARE_VERSION, 10);
			
#elif (HARDWARE_441_ENABLED == 1)
		    memcpy(&cmd_package->data[0], RING_441_HARDWARE_VERSION, 10);			
			
#elif (HARDWARE_421_ENABLED == 1)
			
            memcpy(&cmd_package->data[0], RING_421_HARDWARE_VERSION, 10);
			
#elif (HARDWARE_402_ENABLED == 1)
			
            memcpy(&cmd_package->data[0], RING_402_HARDWARE_VERSION, 10);	
#elif (HARDWARE_153_ENABLED == 1)		
			
			memcpy(&cmd_package->data[0], RING_153_HARDWARE_VERSION, 10);
#elif (HARDWARE_1121_ENABLED == 1)		
			
			memcpy(&cmd_package->data[0], RING_1121_HARDWARE_VERSION, 10);			
			
#elif (HARDWARE_BCL601_151_ENABLED == 1)	

           memcpy(&cmd_package->data[0], RING_BCL601_151_HARDWARE_VERSION, 10);
		   
#elif (HARDWARE_413_ENABLED == 1)				
			memcpy(&cmd_package->data[0], RING_413_HARDWARE_VERSION, 10);	
#elif (HARDWARE_181_ENABLED == 1)				
			memcpy(&cmd_package->data[0], RING_181_HARDWARE_VERSION, 10);	
#elif (HARDWARE_158_ENABLED == 1)					
			memcpy(&cmd_package->data[0], RING_158_HARDWARE_VERSION, 10);	
#elif (HARDWARE_451_ENABLED == 1)
		    memcpy(&cmd_package->data[0], RING_451_HARDWARE_VERSION, 10);	
#elif (HARDWARE_156_ENABLED == 1)					
			memcpy(&cmd_package->data[0], RING_156_HARDWARE_VERSION, 10);			
#elif (HARDWARE_1141_ENABLED == 1)				
			memcpy(&cmd_package->data[0], RING_1141_HARDWARE_VERSION, 10);		
#elif (HARDWARE_1171_ENABLED == 1)				
			memcpy(&cmd_package->data[0], RING_1171_HARDWARE_VERSION, 10);	
#elif (HARDWARE_1181_ENABLED == 1)				
			memcpy(&cmd_package->data[0], RING_1181_HARDWARE_VERSION, 10);	  
#elif (HARDWARE_1191_ENABLED == 1)				
			memcpy(&cmd_package->data[0], RING_1191_HARDWARE_VERSION, 10);	
#elif (HARDWARE_1231_ENABLED == 1)	

#if defined(HANDWARE_1_23_2)   
#if defined(HANDWARE_1_23_2_ONE_SEC)
    memcpy(&cmd_package->data[0], RING_1232_ONE_SEC_HARDWARE_VERSION, 10); 
#else
      memcpy(&cmd_package->data[0], RING_1232_HARDWARE_VERSION, 10); 
#endif
#elif defined(HANDWARE_1_23_3)
    memcpy(&cmd_package->data[0], RING_1233_HARDWARE_VERSION, 10);
#elif defined(HANDWARE_1_23_4)
    memcpy(&cmd_package->data[0], RING_1234_HARDWARE_VERSION, 10);
#else
      memcpy(&cmd_package->data[0], RING_1231_HARDWARE_VERSION, 10); 
#endif
			     
#endif			
			
			break;
		}
	}
	app_package_send_enqueue(cmd_package,14);
	return 0;
}
/*******************************************************************************
 * Function Name     : app_cmd_get_spo2
 * Description       : 血氧相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_get_spo2(struct app_cmd_package * cmd_package)
{
	if(bc_pmic_get_charge_status() != PMIC_CHARGED_NOT)
	{

		uint8_t temp = PPG_CHARGE;
		app_package_ppg(cmd_package,&temp,sizeof(temp),PPG_PACK_TYPE_RESULT);
		return 0;
	}
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1  || HARDWARE_441_ENABLED == 1 )		
	
	if(cmd_package->subcmd == 2)
	{
		app_touch_init_event();
#if(PPG_ENABLED) 
   app_ppg_stop();
#endif
		cmd_package->subcmd = 3;
		app_package_send_enqueue(cmd_package,4);
		return 0;
	}
#else
	if(cmd_package->subcmd == 6)
	{
#if defined(BLE_MULTI_MASTER)

#else
//					app_touch_init_event();

#endif // defined(BLE_MULTI_MASTER)	
#if(PPG_ENABLED) 
   app_ppg_stop();
#endif
		cmd_package->subcmd = 6;
		app_package_send_enqueue(cmd_package,4);
		return 0;
	}	
	
#endif		
#if defined(BLE_MULTI_MASTER)

#else
//		app_touch_uninit_event();

#endif // defined(BLE_MULTI_MASTER)		
#if(PPG_ENABLED) 	
	app_ppg_spo2_ble_cmd_start(cmd_package);
#endif  
//	app_ppg_gary_card_test_cmd_start(cmd_package);
	
//	app_ppg_ir_ble_cmd_start(cmd_package);
	return 0;
}


/*******************************************************************************
 * Function Name     : app_cmd_get_spo2
 * Description       : 血氧相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_get_ppg_spo2(struct app_cmd_package * cmd_package)
{
	if(bc_pmic_get_charge_status() != PMIC_CHARGED_NOT)
	{
		uint8_t temp = PPG_CHARGE;
		app_package_ppg(cmd_package,&temp,sizeof(temp),PPG_PACK_TYPE_RESULT);
		return 0;
	}
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1  || HARDWARE_441_ENABLED == 1 )		
	
	if(cmd_package->subcmd == 2)
	{
		app_touch_init_event();
#if(PPG_ENABLED) 
   app_ppg_stop();
#endif
		cmd_package->subcmd = 3;
		app_package_send_enqueue(cmd_package,4);
		return 0;
	}
#else
	if(cmd_package->subcmd == 6)
	{
#if defined(BLE_MULTI_MASTER)

#else
//					app_touch_init_event();

#endif // defined(BLE_MULTI_MASTER)	
#if(PPG_ENABLED) 
   app_ppg_stop();
#endif
		cmd_package->subcmd = 6;
		app_package_send_enqueue(cmd_package,4);
		return 0;
	}	
	
#endif	
//	
#if defined(BLE_MULTI_MASTER)

#else
//		app_touch_uninit_event();

#endif // defined(BLE_MULTI_MASTER)	
#if(PPG_ENABLED)   
	app_ppg_spo2_ble_cmd_start(cmd_package);
#endif	
	
	return 0;
}

/*******************************************************************************
 * Function Name     : app_cmd_get_hrv
 * Description       : 心率相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_get_hrv(struct app_cmd_package * cmd_package)
{
	if(bc_pmic_get_charge_status() != PMIC_CHARGED_NOT)
	{
		uint8_t temp = PPG_CHARGE;
		app_package_ppg(cmd_package,&temp,sizeof(temp),PPG_PACK_TYPE_RESULT);
		return 0;
	}
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1  || HARDWARE_441_ENABLED == 1 )		
	
	if(cmd_package->subcmd == 4)
	{
//		app_touch_init_event();
#if(PPG_ENABLED) 
   app_ppg_stop();
#endif
		cmd_package->subcmd = 4;
		app_package_send_enqueue(cmd_package,4);
		return 0;
	}
#else
	if(cmd_package->subcmd == 4)
	{
#if defined(BLE_MULTI_MASTER)

#else
//					app_touch_init_event();

#endif // defined(BLE_MULTI_MASTER)	
#if(PPG_ENABLED) 
   app_ppg_stop();
#endif
		cmd_package->subcmd = 4;
		app_package_send_enqueue(cmd_package,4);
		return 0;
	}
	
#endif		

#if(PPG_ENABLED) 
	app_ppg_hrm_ble_cmd_start(cmd_package);
#endif
  
#if defined(BLE_MULTI_MASTER)

#else
//		app_touch_uninit_event();

#endif // defined(BLE_MULTI_MASTER)	
	return 0;
}

/*******************************************************************************
 * Function Name     : app_cmd_get_ir
 * Description       : 红外相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_get_ir(struct app_cmd_package * cmd_package)
{
	if(bc_pmic_get_charge_status() != PMIC_CHARGED_NOT)
	{
		uint8_t temp = PPG_CHARGE;
		app_package_ppg(cmd_package,&temp,sizeof(temp),PPG_PACK_TYPE_RESULT);
		return 0;
	}
	if(cmd_package->subcmd == 2)
	{
//		app_touch_init_event();
//		app_ppg_stop();
		cmd_package->subcmd = 3;
		app_package_send_enqueue(cmd_package,4);
		return 0;
	}
#if(PPG_ENABLED)   
	app_ppg_ir_ble_cmd_start(cmd_package);
#endif  
//	app_touch_uninit_event();
	return 0;
}



/*******************************************************************************
 * Function Name     : app_cmd_get_tempertion
 * Description       : 温度相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_get_tempertion(struct app_cmd_package * cmd_package)
{
	switch(cmd_package->subcmd)
	{
		case 0:
		{
			cmd_package->data[0] = 0x01;
			*(uint16_t*)&cmd_package->data[1] = bc_alg_temp_append(bc_temp_get_temperature_value());
			app_package_send_enqueue(cmd_package,7);
			app_temper_collection_progress(cmd_package->frame_id,cmd_package->subcmd);
			break;
		}
		case 1:
		{
			break;
		}
		case 2:
		{
			break;
		}
	}
	return 0;
}

/*******************************************************************************
 * Function Name     : app_cmd_get_step_count
 * Description       : 计步相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_get_step_count(struct app_cmd_package * cmd_package)
{
	switch(cmd_package->subcmd)
	{
		case 0:
		{
			*(uint16_t*)cmd_package->data = app_g_sensor_sport_step_count_get();
			app_package_send_enqueue(cmd_package,6);
			break;
		}
		case 1:
		{
			app_g_sensor_sport_step_count_clear();
			app_package_send_enqueue(cmd_package,4);
			break;
		}
	}
	return 0;
}
/*******************************************************************************
 * Function Name     : app_cmd_get_vbat
 * Description       : 电量vbat相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_get_vbat(struct app_cmd_package * cmd_package)
{
	switch(cmd_package->subcmd)
	{
		case 0:
		{
			uint8_t charge_status  = bc_pmic_get_charge_status();
			if(charge_status == 0)
			{
                #if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
                if(false == app_cw221x_get_cap(cmd_package)) {
                    bc_delay_ms(1);
                    if(false == app_cw221x_get_cap(cmd_package)) {
                        cmd_package->data[0] = 255;
                        app_package_send_enqueue(cmd_package,5);
                    }
                }
                #else
                cmd_package->data[0] = getvpct();//bc_pmic_get_vbat_percen();
                app_package_send_enqueue(cmd_package,5);
                #endif
                return 0;
			}
			else if(charge_status == 1)
			{
				cmd_package->data[0] = 101;
			}
			else
			{
				cmd_package->data[0] = 102;
			}
			break;
		}
		case 1:
		{
			cmd_package->data[0] = bc_pmic_get_charge_status();
			break;
		}
        case 3:
        {
            #if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
            printf("set vbat type\r\n");
            if(true == bc_device_info_set_vbat_type(cmd_package->data[0]))
            {
                cmd_package->data[0] = 0;
            }
            else
                cmd_package->data[0] = 1;
            #endif
            break;
        }
        case 4:
        {
            #if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
            printf("get vbat type\r\n");
            cmd_package->data[0] = bc_device_info_get_vbat_type();
            #endif
            break;
        }
	}
	app_package_send_enqueue(cmd_package,5);
	return 0;
}
/*******************************************************************************
 * Function Name     : app_cmd_get_hrstory
 * Description       : 历史记录相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_get_hrstory(struct app_cmd_package * cmd_package)
{
	
	switch(cmd_package->subcmd)
	{
		case 0:                                 //读取本地数据的未上传的历史记录
		{
//			app_tsdb_data_port_updata(cmd_package);
			break;
		}
		case 1:                                //读取本地数据的全部的历史记录
		{
//			app_tsdb_data_all_updata(cmd_package);
			break;
		}
		case 2:                               //停止上传本地数据
		{
//			app_tsdb_data_stop_updata();
//			app_package_send_enqueue(cmd_package,4);
			break;
		}
		case 3:                               //删除全部本地数据历史记录
		{
//			app_tsdb_clear(cmd_package);
			
			break;
		}
		case 4:                               //读取本地数据内存容量信息（保留）
		{
			break;
		}
#if (HARDWARE_441_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_451_ENABLED == 1 ||HARDWARE_1141_ENABLED == 1|| HARDWARE_1171_ENABLED == 1 || HARDWARE_1191_ENABLED == 1 \
    || HARDWARE_1231_ENABLED == 1)		
		case 0x10: //文件列表
		{
			app_ppg_file_ls(cmd_package);
			break;
		}
		case 0x11:  //请求上传文件
		{
			app_ppg_file_upload(cmd_package);
			break;
		}
		case 0x12: //删除文件
		{
			if(app_ppg_file_delete((char*)cmd_package->data))
			{
				cmd_package->data[0] = 1;
			}
			else
			{
				cmd_package->data[0] = 0;
			}
			app_package_send_enqueue(cmd_package,4+1);
			break;
		}
		case 0x13:  //格式化文件系统
		{
			app_ppg_file_format(cmd_package);
			break;
		}
		case 0x14:  //获取文件系统空间信息
		{
			app_ppg_file_sys_size_get(cmd_package);
			break;
		}
    case 0x1A:
    {
      app_ppg_file_one_click_upload(cmd_package);
      break;
    }
     case 0x18: //用于断点续传，上传指定偏移的数据
    {
      app_ppg_file_resume_upload(cmd_package);
      break;
    }
#endif		
		case 0x15:  //设置自动记录采集数据模式
		{
			if(bc_device_info_set_ppg_file_mode(cmd_package->data[0]))
			{
				cmd_package->data[0] = 1;
			}
			else
			{
				cmd_package->data[0] = 0;
			}
			app_package_send_enqueue(cmd_package,4+1);
			break;
		}
		case 0x16:  //获取自动记录采集数据模式
		{
			cmd_package->data[0] = bc_device_info_get_ppg_file_mode();
			app_package_send_enqueue(cmd_package,4+1);
			break;
		}
		case 0x17:
		{
#if (HARDWARE_441_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_158_ENABLED == 1  || HARDWARE_451_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)				
			cmd_package->data[0] = app_ppg_file_status_get();
			app_package_send_enqueue(cmd_package,4+1);
#endif			
			break;
		}
    case 0x1E:                            
		{
      
      if(cmd_package->data[0] == 1)
      {
        struct app_cmd_package  package = {0};
        package.subcmd = 0x1F;
        package.frame_id = 0x09;
        package.cmd = 0x36;
        memcpy(package.data,&cmd_package->data[2],cmd_package->data[1]);
        app_file_active_upload(&package);
      }
      
			break;
		}
    
		case 0x20:                            
		{
      if(cmd_package->data[0] == 0)
      {
        struct app_cmd_package  package = {0};
        package.subcmd = 0x1F;
        package.frame_id = 0x09;
        package.cmd = 0x36;
        memcpy(package.data,&cmd_package->data[2],cmd_package->data[1]);
        app_file_active_upload(&package);
      }
      else if(cmd_package->data[0] == 1)
      {
        bc_device_info_set_capture_audio_file_name((char*)&cmd_package->data[2]);
        
      }
			break;
		}
	}
	sleepClassification_active(0);
	
	return 0;
}
/*******************************************************************************
 * Function Name     : app_cmd_set_sys
 * Description       : 系统设置相关指令
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_set_sys(struct app_cmd_package * cmd_package)
{
	switch(cmd_package->subcmd)
	{
		case 0:
		{
			uint32_t temp = *(uint32_t*)cmd_package->data;	
//			if(temp == (60*5) || temp == (60*20) || temp == (60*30))	
//			{
//			}
//			else
//			{
//				cmd_package->data[0] = 0;
//				app_package_send_enqueue(cmd_package,5);
//				break;
//			}
            if(temp < 60)	
			{
				cmd_package->data[0] = 0;
				app_package_send_enqueue(cmd_package,5);
				break;
			}	
			if(bc_set_business_strategy_value(BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_TIME,temp ))
			{
				cmd_package->data[0] = 1;
			}
			else
			{
				cmd_package->data[0] = 0;
			}
			app_package_send_enqueue(cmd_package,5);
//			app_ppg_automatic_cycle_collection_time_update();
			break;
			
		}
		case 1:
		{
						
			*(uint32_t*)cmd_package->data = bc_get_business_strategy_value(BUSINESS_STRATEGY_PPG_AUTOMATIC_CYCLE_TIME);
			app_package_send_enqueue(cmd_package,8);
		
			break;
		}
		case 2:
		{
			bc_device_info_reset();
			bc_business_strategy_reset();
			app_package_send_enqueue(cmd_package,4);
			break;
		}
		case 3:
		{
		
			if(bc_device_info_set_ble_name(&cmd_package->data[1],cmd_package->data[0]))
			{
				cmd_package->data[0] = 1;
			}
			else
			{
				cmd_package->data[0] = 0;
			}
			app_package_send_enqueue(cmd_package,5);
			if(cmd_package->data[0] == 1)
			{
				bc_rtos_delay(200);
				NVIC_SystemReset();
			}
			break;
		}
		case 4:
		{
			bc_device_info_get_ble_name(&cmd_package->data[1],&cmd_package->data[0]);			
			app_package_send_enqueue(cmd_package,4+1+cmd_package->data[0]);
			break;
		}
		case 5:
		{
			break;
		}
		case 0x0c:
    {
      uint8_t mac[6] = {0};
      mac[0] = cmd_package->data[5];
      mac[1] = cmd_package->data[4];
      mac[2] = cmd_package->data[3];
      mac[3] = cmd_package->data[2];
      mac[4] = cmd_package->data[1];
      mac[5] = cmd_package->data[0];
      app_ble_mac_set(mac);
      bc_device_mac_set(mac);
      bc_delay_ms(2000);
      NVIC_SystemReset();
      break;
    }
		case 0x0d:
		{
			bc_device_mac_get(cmd_package->data);
			app_package_send_enqueue(cmd_package,4+6);
			break;
		}
		case 0xFE:
		{
			uint32_t temp = *(uint32_t*)&cmd_package->data[1];
			if(bc_set_business_strategy_value(cmd_package->data[0],temp ))
			{
				cmd_package->data[0] = 1;
			}
			else
			{
				cmd_package->data[0] = 0;
			}
			app_package_send_enqueue(cmd_package,5);
			break;
		}
		case 0xFD:
		{
			uint8_t temp = cmd_package->data[0];
			*(uint32_t*)&cmd_package->data[1] = bc_get_business_strategy_value(temp);
			app_package_send_enqueue(cmd_package,4+1+4);
			break;
		}		
		case 0xFF:
		{
			app_cmd_set_and_get_sys_info(cmd_package);
			
			cmd_package->subcmd = 0xA0;
			app_package_send_enqueue(cmd_package,4 + 60);
			break;
		}
	}
	return 0;
}




/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_puf(struct app_cmd_package * cmd_package)
{
	switch(cmd_package->subcmd)
	{
		case 0:
		{
//			cmd_package->data[0] = bc_buf_chip_id_get();
//			bc_nfc_write((uint8_t*)cmd_package,5);	
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1)		
			bc_buf_id((char*)cmd_package->data);
#endif				
			
			app_package_send_enqueue(cmd_package,4 + 32);		
			break;
			
		}
		case 1:
		{
			uint8_t temp[32] = {0};
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1)		
			bc_buf_resp((char*)cmd_package->data, 32, (char*)temp, 32);
#endif					
			
			memcpy(cmd_package->data,temp,32);
//			bc_nfc_write((uint8_t*)cmd_package,4 + 32);	
			app_package_send_enqueue(cmd_package,4 + 32);
		
			break;
		}
	}
	return 0;
}

/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_nfc(struct app_cmd_package * cmd_package)
{
	switch(cmd_package->subcmd)
	{
		case 0:
		{

			bc_device_info_get_url(&cmd_package->data[1],&cmd_package->data[0]);
			if(cmd_package->data[0] > 128 || cmd_package->data[0] == 0)
			{
				cmd_package->data[0] = 0;
				app_package_send_enqueue(cmd_package,5);	
				break;
			}
			app_package_send_enqueue(cmd_package,4 + 1 + cmd_package->data[0]);		
			break;
			
		}
		case 1:
		{
			
			
			if(bc_device_info_set_url(&cmd_package->data[1],cmd_package->data[0]))
			{
				cmd_package->data[0] = 1;
				app_package_send_enqueue(cmd_package,5);
				bc_rtos_delay(2000);
				NVIC_SystemReset();
			}
			else
			{
				cmd_package->data[0] = 0;
			}
			app_package_send_enqueue(cmd_package,5);
		
			break;
		}
		case 2:
		{
			switch(cmd_package->data[0])
			{
				case 0:
				{
#if ( HARDWARE_181_ENABLED == 1  )	

					bc_device_nfc_info  nfc_info = {0};
					app_nfc_start();
					nfc_info.nfc_mode = cmd_package->data[0];
					if(bc_device_nfc_info_set(&nfc_info))
					{
						cmd_package->data[0] = 1;
					}
					else
					{
						cmd_package->data[0] = 0;
					}
					app_package_send_enqueue(cmd_package,5);
									   
#endif	
					break;
				}
				case 1:
				{
#if ( HARDWARE_181_ENABLED == 1  )	

					bc_device_nfc_info  nfc_info = {0};
					app_nfc_stop();

					bc_nfc_exit_FM11RF08_on(); 
					nfc_info.nfc_mode = cmd_package->data[0];
					if(bc_device_nfc_info_set(&nfc_info))
					{
						cmd_package->data[0] = 1;
					}
					else
					{
						cmd_package->data[0] = 0;
					}
					app_package_send_enqueue(cmd_package,5);
#endif	
					break;
				}
			}
			break;
		}
		case 3:
		{
			bc_device_nfc_info  *nfc_info =   bc_device_nfc_info_get();
			cmd_package->data[0] = nfc_info->nfc_mode;
			app_package_send_enqueue(cmd_package,5);
			break;
		}
	}
	return 0;
}

#ifndef HANDWARE_1_23_4
/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_six_axis_sensor(struct app_cmd_package * cmd_package)
{
	
	app_six_axis_sensor_event(cmd_package);
	return 0;
}
#endif



/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_authentication(struct app_cmd_package * cmd_package)
{
//	switch(cmd_package->subcmd)
//	{
//		case 0:
//		{		
//			app_authentcation_states_get(&cmd_package->data[0],&cmd_package->data[2],&cmd_package->data[1]);
//			app_package_send_enqueue(cmd_package,4 + 2 + cmd_package->data[1]);
//			break;
//			
//		}
//		case 1:
//		{		
//            uint8_t result = (uint8_t)app_authentication_activate_algorithm_key(&cmd_package->data[1],cmd_package->data[0]);
//			cmd_package->data[0] = result;
//			app_package_send_enqueue(cmd_package,4 + 1);
//			break;
//		}
//		case 2:
//		{
//			break;
//		}
//	}
	return 0;
}



/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_led(struct app_cmd_package * cmd_package)
{
//	if(bc_pmic_get_charge_status() != PMIC_CHARGED_NOT || bc_pmic_get_vbat_percen() < 10 )
//	{
//		app_package_send_enqueue(cmd_package,4);
//		return 0;
//	}
//	app_led_falsh_handler(cmd_package->data[0],&cmd_package->data[2],cmd_package->data[1]);
	switch(cmd_package->subcmd)
	{
		case 0:
		{
//			app_led_falsh_handler(cmd_package->data[0],&cmd_package->data[2],cmd_package->data[1]);
			break;
		}
		case 1:
		{
			BC_LOG_INFO("llllllll\r\n");
//			app_led_Linear_pwm_out(cmd_package->data);

			break;
		}
		case 2:
		{
//			app_led_nonlinear_pwm_out(cmd_package->data);
			break;
		}
 #if (HARDWARE_1121_ENABLED == 1 || HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)   
		case 4:
		{


      bc_ic_led_set(cmd_package->data);
			app_package_send_enqueue(cmd_package,4);			
			break;
		}
		case 5:
		{
			bc_ic_led_breathing_light_start();
			app_package_send_enqueue(cmd_package,4);
			break;
		}
		case 6:
		{
			bc_ic_led_breathing_light_stop();
			app_package_send_enqueue(cmd_package,4);
			break;
		}
		case 7:
		{
			bc_ic_led_stop();
			app_package_send_enqueue(cmd_package,4);
			break;
		}
#endif    
	}
	return 0;
}



/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_hid(struct app_cmd_package * cmd_package)
{
	switch(cmd_package->subcmd)
	{
		case 0:
		{
			switch(cmd_package->data[0])
			{
				case 0:
				{
					app_ble_hid_volume_up();
					break;
				}
				case 1:
				{
					app_ble_hid_volume_down();
					break;
				}
				case 2:
				{
					app_ble_hid_previous_music();
					break;
				}
				case 3:
				{
					app_ble_hid_previous_music();
					break;
				}
			}
			break;
		}
		case 1:
		{
			bc_device_hid_info hid_info = {0};
			hid_info.device_hid_touch_mode = cmd_package->data[0];
			hid_info.device_hid_gesture_mode = cmd_package->data[1];
			hid_info.device_hid_enable_flag = cmd_package->data[2];
			
			
//			if(app_touch_pdm_audio_status_get())
//			{
//				app_touch_pdm_audio_stop();
//			}
			
			if(bc_device_info_set_hid_info(&hid_info))
			{
				cmd_package->data[0] = 1;
				app_package_send_enqueue(cmd_package,4+3);
				
				if( hid_info.device_hid_gesture_mode != 0xFF)
				{
//					app_hid_photograth_stop();
					bc_rtos_delay(20);
//					app_hid_photograth_start();
				}
				else
				{
//					app_hid_photograth_stop();
				}
				if(hid_info.device_hid_touch_mode != 0xFF)
				{
//					app_touch_init_event();
				}
				else
				{
//					app_touch_uninit_event();
				}
				
				uint16_t x =*(uint16_t*)&cmd_package->data[25] , y = *(uint16_t*)&cmd_package->data[23];
//				uint16_t x =1179 , y = 2556;
				BC_LOG_INFO("phone screen type:%s    x:%d  y:%d \r\n",(char*)&cmd_package->data[3],x,y);
//				app_ble_hid_phone_screen_set(y,x,(char*)&cmd_package->data[3],20);
				break;
			}
			else
			{
				cmd_package->data[0] = 0;
				app_package_send_enqueue(cmd_package,4+3);
				break;
			}

			break;
		}
		case 2:
		{
			bc_device_hid_info *hid_info;
			hid_info = bc_device_info_get_hid_info();
			cmd_package->data[0] = hid_info->device_hid_touch_mode ;
			cmd_package->data[1] = hid_info->device_hid_gesture_mode ;
			cmd_package->data[2] = hid_info->device_hid_enable_flag ;
			
			
			//app_ble_hid_phone_screen_get((uint32_t*)&cmd_package->data[3],(uint32_t*)&cmd_package->data[7],(char*)&cmd_package->data[12],(uint8_t*)&cmd_package->data[11]);
			
//			app_package_send_enqueue(cmd_package,4+3+4+4+1+cmd_package->data[11]);
			app_package_send_enqueue(cmd_package,4+3);
			break;
		}
		case 3:
		{
			bc_device_hid_info *hid_info;
			hid_info = bc_device_info_get_hid_info();
			
			switch(cmd_package->data[0])
			{
				case 0:  //android 
				{
				
					struct touch_hid_info touch_info = {0};
					touch_info.short_video = ANDROID_TOUCH_SHORT_VIDEO_HID;
					touch_info.photograph = ANDROID_TOUCH_PHOTOGRAPH_HID;
					touch_info.music = ANDROID_TOUCH_MUSIC_HID;
					touch_info.ppt = ANDROID_TOUCH_PPT_HID;
					touch_info.up_audio = ANDROID_TOUCH_UP_AUDIO_HID;
					
					
					struct gesture_hid_info  gesture_info = {0};
					gesture_info.short_video = ANDROID_GESTURE_SHORT_VIDEO_HID;
					gesture_info.photograph = ANDROID_GESTURE_PHOTOGRAPH_HID;
					gesture_info.music = ANDROID_GESTURE_MUSIC_HID;
					gesture_info.ppt = ANDROID_GESTURE_PPT_HID;
					gesture_info.snap = ANDROID_GESTURE_SNAP_HID;					
					
#if defined(RONG_WEI_Z2X)

                   touch_info.music = 0;
					touch_info.ppt = 0;
					
				
#endif					
					cmd_package->data[0] = hid_info->device_hid_type;

					memcpy(&cmd_package->data[1],(uint8_t*)&touch_info,sizeof(struct touch_hid_info));

					memcpy(&cmd_package->data[9],(uint8_t*)&gesture_info,sizeof(struct gesture_hid_info));

					break;
				}
				case 1:
				{
					struct touch_hid_info touch_info = {0};
					touch_info.short_video = IOS_TOUCH_SHORT_VIDEO_HID;
					touch_info.photograph = IOS_TOUCH_PHOTOGRAPH_HID;
					touch_info.music = IOS_TOUCH_MUSIC_HID;
					touch_info.ppt = IOS_TOUCH_PPT_HID;
					touch_info.up_audio = IOS_TOUCH_UP_AUDIO_HID;
					
					struct gesture_hid_info  gesture_info;
					gesture_info.short_video = IOS_GESTURE_SHORT_VIDEO_HID;
					gesture_info.photograph = IOS_GESTURE_PHOTOGRAPH_HID;
					gesture_info.music = IOS_GESTURE_MUSIC_HID;
					gesture_info.ppt = IOS_GESTURE_PPT_HID;
					gesture_info.snap = IOS_GESTURE_SNAP_HID;
					
					cmd_package->data[0] = hid_info->device_hid_type;
					
#if defined(RONG_WEI_Z2X)

                   touch_info.music = 0;
				   touch_info.ppt = 0;
				
#endif						
					memcpy(&cmd_package->data[1],(uint8_t*)&touch_info,sizeof(struct touch_hid_info));
					memcpy(&cmd_package->data[9],(uint8_t*)&gesture_info,sizeof(struct gesture_hid_info));
					break;
				}
				case 2:
				{
					break;
				}
			}
			app_package_send_enqueue(cmd_package,4+17);
			break;
		}
		case 4:
		{
			bc_device_hid_info *hid_info;
			hid_info = bc_device_info_get_hid_info();
			cmd_package->data[0] = hid_info->device_hid_touch_mode ;
			cmd_package->data[1] = hid_info->device_hid_gesture_mode ;
			cmd_package->data[2] = hid_info->device_hid_enable_flag ;
			
			
			app_ble_hid_phone_screen_get((uint32_t*)&cmd_package->data[3],(uint32_t*)&cmd_package->data[7],(char*)&cmd_package->data[12],(uint8_t*)&cmd_package->data[11]);
			
			app_package_send_enqueue(cmd_package,4+3+4+4+1+cmd_package->data[11]);
			break;
		}
	}
	
	return 0;
}




/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_config_touch(struct app_cmd_package * cmd_package)
{
	switch(cmd_package->subcmd)
	{
		case 0:
		{
//			bc_set_business_strategy_value(BUSINESS_STRATEGY_CH0_TOUCH_THRESHOLD,cmd_package->data[0]);
//			bc_set_business_strategy_value(BUSINESS_STRATEGY_CH1_TOUCH_THRESHOLD,cmd_package->data[1]);
//			bc_set_business_strategy_value(BUSINESS_STRATEGY_CH2_TOUCH_THRESHOLD,cmd_package->data[2]);
			break;
		}
		case 1:
		{
//			cmd_package->data[0] = (uint8_t)bc_get_business_strategy_value(BUSINESS_STRATEGY_CH0_TOUCH_THRESHOLD);
//			cmd_package->data[1] = (uint8_t)bc_get_business_strategy_value(BUSINESS_STRATEGY_CH1_TOUCH_THRESHOLD);
//			cmd_package->data[2] = (uint8_t)bc_get_business_strategy_value(BUSINESS_STRATEGY_CH2_TOUCH_THRESHOLD);
			app_package_send_enqueue(cmd_package,7);
			return 0;
		}
		case 2:
		{
//			app_touch_check_start(cmd_package);
			return 0;
		}
	}
	cmd_package->subcmd = 0;
	app_package_send_enqueue(cmd_package,4);
	return 0;
}

/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_motor(struct app_cmd_package * cmd_package)
{
//	bc_piezoelectric_motor_play((struct bc_slice_parameters *)cmd_package->data);
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

  switch(cmd_package->subcmd)
	{
		case 1:
		{
			break;
		}
		case 2:
		{
			break;
		}
		case 3:
		{
			uint32_t temp = *(uint32_t *)&cmd_package->data[0];
            uint16_t type_vib = *(uint16_t *)&cmd_package->data[4];
            #if (defined(HANDWARE_1_23_3 ) || defined(HANDWARE_1_23_4 ))
                app_linear_motor_ic_start_timer_vib(temp, type_vib);
            #else
                app_linear_motor_set(temp,cmd_package->data[4]);
                cmd_package->data[0] = 1;
            #endif
			app_package_send_enqueue(cmd_package,5);
			return 0;
		}
		case 4:
		{
            #if (defined(HANDWARE_1_23_3 ) || defined(HANDWARE_1_23_4 ))
                //app_linear_motor_ic_start(1);
                app_linear_motor_ic_start_test();
            #else
                app_linear_motor_start(cmd_package->data[0]);
            #endif
			break;
		}
	}
   
#endif		
	
	app_package_send_enqueue(cmd_package,4);
	return 0;
}



/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_port_mode(struct app_cmd_package * cmd_package)
{
#if (HARDWARE_931_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	
	switch(cmd_package->subcmd)
	{
		case 0:
		{		

			break;
			
		}
		case 1:
		{
//			uint32_t temp_flag = 0;		
			uint32_t time = *(uint32_t*)&cmd_package->data[2];		
	        uint32_t slice_storage_time = *(uint32_t*)&cmd_package->data[6];
			
//			bc_device_info_get_ppg_file_flag(&temp_flag);
			if(app_ppg_file_status_get() != 0)
			{
				cmd_package->data[0] = 0;
				app_package_send_enqueue(cmd_package,4 + 1);
				break;
			}				
			if(app_ppg_file_open(PPG_FILE_TYPE_PPG_RED_IR_GREEN_TEMPER))
			{
				app_g_sensor_sport_step_count_clear();
        app_ppg_file_timeout_timer_start(time);
        app_ppg_file_slice_storage_timer_start(slice_storage_time);
#if ( HARDWARE_451_ENABLED == 1 )				
				app_ppg_spo2_always_collecting();
#elif ( HARDWARE_1141_ENABLED == 1)				
				uint8_t temp[4] = {0x00,0x09,0x3C,0x01};
#if(PPG_ENABLED)         
				app_ppg_spo2_hr_led_collecting(0,0,0,temp);
#endif        
				
#endif
				cmd_package->data[0] = 1;		
			}
			else
			{
				cmd_package->data[0] = 0;
			}
			
			app_package_send_enqueue(cmd_package,4 + 1);
			break;
		}
		case 2:
		{
			break;
		}
		case 3:
		{
//			uint32_t temp_flag = 0;			
//			bc_device_info_get_ppg_file_flag(&temp_flag);
      
//			if(app_ppg_file_status_get() == 0)
//			{
//				cmd_package->data[0] = 0;
//				app_package_send_enqueue(cmd_package,4 + 1);
//				break;
//			}
#if(PPG_ENABLED) 
   app_ppg_stop();
#endif
			if(app_ppg_file_close())
			{
				cmd_package->data[0] = 1;	
			}
			else
			{
				cmd_package->data[0] = 0;
			}
//			app_ppg_file_slice_storage_timer_stop();
//			app_ppg_file_close();
			
			app_package_send_enqueue(cmd_package,4 + 1);
		}
	}
#endif	
	return 0;
}

/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static uint8_t app_cmd_pdm(struct app_cmd_package * cmd_package)
{
	switch(cmd_package->subcmd)
	{
		case 0:		
		case 1:
		{
			switch(cmd_package->data[0])
			{
				case 0:
				{
		#if (HARDWARE_153_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED == 1  || HARDWARE_1121_ENABLED == 1|| HARDWARE_158_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || \
          HARDWARE_1191_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	
          cmd_package->subcmd = 0x08;
					app_pdm_stop(cmd_package);
		#endif	  
					break;
				}
				case 1:
				{
		#if (HARDWARE_153_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED == 1  || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || \
          HARDWARE_1191_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	
          cmd_package->subcmd = 0x08;
					app_pdm_start(cmd_package);
		#endif				
					break;
				}
#if defined(HANDWARE_1_23_4)
				case 2:
				{
					/* 录音暂停命令：在线录音时暂停，黄灯慢闪 */
					if(app_pdm_recording_pause())
					{
						cmd_package->data[0] = 1;
						/* LED切换为黄灯慢闪 */
						bc_ic_led_recording_pause_on();
					}
					else
					{
						cmd_package->data[0] = 0;
					}
					app_package_send_enqueue(cmd_package, 4 + 1);
					break;
				}
				case 3:
				{
					/* 录音恢复命令：结束暂停，恢复30%绿灯 */
					if(app_pdm_recording_resume())
					{
						cmd_package->data[0] = 1;
						/* LED恢复为30%绿灯 */
						bc_ic_led_recording_pause_off();
					}
					else
					{
						cmd_package->data[0] = 0;
					}
					app_package_send_enqueue(cmd_package, 4 + 1);
					break;
				}
#endif
				
			}
			break;
		}
		case 2:
		{
			if(bc_device_info_set_audio_up_mode(cmd_package->data[0]))
			{
				cmd_package->data[0] = 1;
			}
			else
			{
				cmd_package->data[0] = 0;
			}
			app_package_send_enqueue(cmd_package,4 + 1);
			break;
		}
		case 3:
		{
			cmd_package->data[0] = bc_device_info_get_audio_up_mode();
			app_package_send_enqueue(cmd_package,4 + 1);
			break;
		}
    case 5:
    {
      if(cmd_package->data[0] == 1)
      {
#if defined(HANDWARE_1_23_2_ONE_SEC)
        /* APP命令控制的录音，优先级最高 */
        app_pdm_set_next_record_priority(3);
#endif
        if(app_pdm_recording_start())
        {
          cmd_package->data[0] = 1;
        }
        else
        {
          cmd_package->data[0] = 0;
        }
      }
      else if(cmd_package->data[0] == 0)
      {
        if(app_pdm_recording_stop())
        {
          cmd_package->data[0] = 1;
        }
        else
        {
          cmd_package->data[0] = 0;
        }
      }
      app_package_send_enqueue(cmd_package,4 + 1);
      break;
    }
    case 6:
    {
 #if (HARDWARE_1191_ENABLED == 1 )	

      app_hardline_upload_tsdb(cmd_package);

#endif     

#if defined(HANDWARE_1_23_2_ONE_SEC)
      /* 读取所有单击标记记录，逐行通过蓝牙回复 */
      app_single_tap_record_upload(cmd_package);
#endif

      break;
    }
    case 7:
    {
#if (HARDWARE_1191_ENABLED == 1 )	
      app_hardline_clear();
      cmd_package->data[0] = 1;
      app_package_send_enqueue(cmd_package,4 + 1);
#endif      
#if defined(HANDWARE_1_23_2_ONE_SEC)
      /* 删除单击标记记录文件 */
      uint8_t ret = app_single_tap_record_clear();
      cmd_package->data[0] = ret;  /* 0=成功 1=失败 */
      app_package_send_enqueue(cmd_package, 4 + 1);
#endif
      break;
    }
    case 0x0B:
    {
#if defined(HANDWARE_1_23_2_ONE_SEC)
      /* 定时录音配置：data[0]=控制(1开/0关) data[1-4]=间隔秒数 data[5-8]=录音秒数 */
      uint8_t enable = cmd_package->data[0];
      uint32_t interval = (uint32_t)cmd_package->data[1] | 
                          ((uint32_t)cmd_package->data[2] << 8) |
                          ((uint32_t)cmd_package->data[3] << 16) |
                          ((uint32_t)cmd_package->data[4] << 24);
      uint32_t duration = (uint32_t)cmd_package->data[5] | 
                          ((uint32_t)cmd_package->data[6] << 8) |
                          ((uint32_t)cmd_package->data[7] << 16) |
                          ((uint32_t)cmd_package->data[8] << 24);
      
      /* 录音时间统一固定为30秒 */
      duration = 30;
      
      /* 最小间隔时间为60秒 */
      if(interval < 60)
      {
          interval = 60;
      }
      
        BC_LOG_INFO("enable :%d, interval: %d, duration: %d\r\n", enable,interval,duration);
      bool save_ok = bc_device_info_timer_record_config_set(enable, interval, duration);
      cmd_package->data[0] = save_ok ? 1 : 0;
      app_package_send_enqueue(cmd_package, 4 + 1);
      
      /* 启动或停止定时录音 */
      if(save_ok)
      {
          if(enable)
          {
              app_timer_record_start();
          }
          else
          {
              app_timer_record_stop();
          }
      }
#endif
      break;
    }
    case 0x0C:
    {
#if defined(HANDWARE_1_23_3)
      cmd_package->data[0] = app_pdm_offline_on_get() ? 0x01 : 0x00;
#else
      cmd_package->data[0] = 0x00;
#endif
      app_package_send_enqueue(cmd_package, 4 + 1);
      break;
    }
    case 0xFE:
    {
      if(cmd_package->data[0] == 1)
      {
        app_pdm_capture_recording_start();
 
      }
      else if(cmd_package->data[0] == 0)
      {
        app_pdm_capture_recording_stop();
      }
      break;
    }
    case 0xF9:
    {
      if(app_pdm_recording_stop())
      {
        cmd_package->data[0] = 1;
      }
      else
      {
        cmd_package->data[0] = 0;
      }
      break;
    }
    case 0xFD:
    {
      app_pdm_switch_online_to_offline();
      break;
    }
    case 0xFC:
    {
      app_touch_pdm_key_flag_clear();
      break;
    }
	}
	
	return 0;
}

#if defined(HANDWARE_1_23_2)
/*******************************************************************************
 * Function Name     : app_cmd_led_motor_mode_set
 * Description       : 设置LED/马达模式（Cmd 0x8d）
 * Input             : cmd_package
 * Output            :
 * Return            :
 *******************************************************************************/
static uint8_t app_cmd_led_motor_mode_set(struct app_cmd_package *cmd_package)
{
    bc_device_led_motor_mode_info info = {0};
    bc_device_info_led_motor_mode_info_get(&info);

    bool save_ok = false;
    switch(cmd_package->subcmd)
    {
        case 0x01:  /* LED */
        {
            uint8_t scene = cmd_package->data[0];
            uint8_t mode = cmd_package->data[1];
            if(mode < 0x01 || mode > 0x03)
            {
                break;
            }
            if(scene == 0x01)           /* 蓝牙连接 */
            {
                info.ble_connect_color = mode;
                save_ok = true;
            }
            else if(scene == 0x02)      /* 蓝牙断开 */
            {
                info.ble_disconnect_color = mode;
                save_ok = true;
            }
            else if(scene == 0x03)      /* 录音 */
            {
                info.recording_color = mode;
                save_ok = true;
            }
            break;
        }
        case 0x02:  /* 马达 */
        {
            uint8_t action = cmd_package->data[0];
            uint8_t mode = cmd_package->data[1];
            if(mode < 0x01 || mode > 0x02)
            {
                break;
            }
            if(action == 0x01)          /* 开启录音 */
            {
                info.motor_start_mode = mode;
                save_ok = true;
            }
            else if(action == 0x02)     /* 关闭录音 */
            {
                info.motor_stop_mode = mode;
                save_ok = true;
            }
            break;
        }
    }

    if(save_ok)
    {
        if(bc_device_info_led_motor_mode_info_set(&info))
        {
            cmd_package->data[0] = 1;
        }
        else
        {
            cmd_package->data[0] = 0;
        }
    }
    else
    {
        cmd_package->data[0] = 0;
    }
    app_package_send_enqueue(cmd_package, 4 + 1);
    return 0;
}

/*******************************************************************************
 * Function Name     : app_cmd_led_motor_mode_get
 * Description       : 查询LED/马达模式（Cmd 0x8e）
 * Input             : cmd_package
 * Output            :
 * Return            :
 *******************************************************************************/
static uint8_t app_cmd_led_motor_mode_get(struct app_cmd_package *cmd_package)
{
    bc_device_led_motor_mode_info info = {0};
    bc_device_info_led_motor_mode_info_get(&info);

    uint8_t mode = 0;
    switch(cmd_package->subcmd)
    {
        case 0x01:  /* LED */
        {
            uint8_t scene = cmd_package->data[0];
            if(scene == 0x01)           /* 蓝牙连接 */
            {
                mode = info.ble_connect_color;
            }
            else if(scene == 0x02)      /* 蓝牙断开 */
            {
                mode = info.ble_disconnect_color;
            }
            else if(scene == 0x03)      /* 录音 */
            {
                mode = info.recording_color;
            }
            break;
        }
        case 0x02:  /* 马达 */
        {
            uint8_t action = cmd_package->data[0];
            if(action == 0x01)          /* 开启录音 */
            {
                mode = info.motor_start_mode;
            }
            else if(action == 0x02)     /* 关闭录音 */
            {
                mode = info.motor_stop_mode;
            }
            break;
        }
    }

    cmd_package->data[1] = mode;
    app_package_send_enqueue(cmd_package, 4 + 2);
    return 0;
}
#endif

static uint8_t app_cmd_app_event(struct app_cmd_package * cmd_package)
{
	switch(cmd_package->subcmd)
	{
		case 0:		
		{
			//app_g_sensor_sport_step_count_clear();	
			app_cmd_set_and_get_sys_info(cmd_package);				
//			app_package_send_enqueue(cmd_package,4 + 60);
			app_ble_send((uint8_t*)cmd_package,4 + 60);
		//	app_tsdb_clear_event();
			
			break;
		}
		case 1:
		case 2:
		{
            app_cmd_set_and_get_sys_info(cmd_package);
//			app_package_send_enqueue(cmd_package,4 + 60);
			app_ble_send((uint8_t*)cmd_package,4 + 60);
		//	app_tsdb_data_port_updata_event();
			break;
		}
		case 3:
		{
//			cmd_package->data[0] = bc_device_info_get_audio_up_mode();
//			app_package_send_enqueue(cmd_package,4 + 1);
			break;
		}
	}
	
	return 0;	
}



static uint8_t app_cmd_ppg_led_data_get(struct app_cmd_package * cmd_package)
{
	switch(cmd_package->subcmd)
	{
		case 0:		
		{
#if(PPG_ENABLED)       
			bc_ppg_current_val_set(cmd_package->data[2],cmd_package->data[3],cmd_package->data[4]);
      
			app_ppg_spo2_hr_led_collecting(cmd_package->data[0],cmd_package->data[5],cmd_package->data[6],(uint8_t*)cmd_package);
#endif      
			*(uint64_t*)cmd_package->data  = bg_rtc_time_get_uinx_ms_time();
			cmd_package->data[8] = 8;
			cmd_package->subcmd = 0x01;
			app_package_send_enqueue(cmd_package,4 + 9);
			break;
		}
		case 4:
		{
#if(PPG_ENABLED) 
   app_ppg_stop();
#endif
			app_package_send_enqueue(cmd_package,4);
			break;
		}
	}
	
	return 0;	
}


static uint8_t app_cmd_rtc_alarm_clock_event(struct app_cmd_package * cmd_package)
{
#if defined(ALARM_CLOCK)

	switch(cmd_package->subcmd)
	{
		case 0:
		{
			break;
		}
		case 1:
		{
			uint8_t length = 0;
			app_alarm_clock_info_get(cmd_package->data,&length);
			app_package_send_enqueue(cmd_package,4 + length);
			break;
		}
		case 2:
		{
			break;
		}
		case 3:
		{
			if(app_alarm_clock_info_update(cmd_package->data))
			{
				cmd_package->data[0] = 1;
			}
			else
			{
				cmd_package->data[0] = 0;
			}
			app_package_send_enqueue(cmd_package,4 + 1);
			break;
		}
		case 4:
		{
			if(app_alarm_clock_week_info_update(cmd_package->data))
			{
				cmd_package->data[0] = 1;
			}
			else
			{
				cmd_package->data[0] = 0;
			}
			app_package_send_enqueue(cmd_package,4 + 1);
				
			break;
		}
		case 5:
		{
			uint8_t length = 0;
			app_alarm_clock_week_info_get(cmd_package->data,&length);
			app_package_send_enqueue(cmd_package,4 + length);
			break;
		}
	}

#endif // defined(ALARM_CLOCK)		
	return 0;
}

static uint8_t app_cmd_wifi_event(struct app_cmd_package * cmd_package)
{
  switch(cmd_package->subcmd)
  {
    case 0x00:
    {
      if(cmd_package->data[0] == 1)
      {
        uint16_t length = *(uint16_t*)&cmd_package->data[1];
        app_wifi_speed_test_start(length);
        
      }
      else if(cmd_package->data[0] == 0)
      {
        app_wifi_speed_test_stop();
      }
      cmd_package->data[0] = 1;
      app_package_send_enqueue(cmd_package,4+1);
      break;
    }
    case 0x01:
    {
      if(cmd_package->data[0] == 1)
      {
#if ( HARDWARE_1191_ENABLED == 1)	

         bc_wifi_open();
#endif	        
       
      }
      else if(cmd_package->data[0] == 0)
      {
#if ( HARDWARE_1191_ENABLED == 1)	

        bc_wifi_close();
#endif	        
        
      }
      cmd_package->data[0] = 1;
      app_package_send_enqueue(cmd_package,4+1);
      break;
    }
  }
}


static uint8_t app_cmd_ipc_event(struct app_cmd_package * cmd_package)
{
  switch(cmd_package->subcmd)
  {
    case 0x00:
    {
      bc_delay_ms(3000);
      app_ppg_list_capture_audio_up_check();
      break;
    }
    case 0x01:
    {
      app_pdm_touch_start();
      break;
    }
    case 0x02:
    {
      app_pdm_touch_stop();
      break;
    }
    case 0x03:
    {
 
      break;
    }
    case 0x04:
    {
   
      break;
    }
  }
}

/************************************************************->*****************
 * Function Name     : app_test_cmd_handler
 * Description       : 工装测试命令处理
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
static void app_test_cmd_handler(struct app_cmd_package * cmd_package)
{
	if(cmd_package->subcmd >= TEST_CMD_NUM)
	{
		return;
	}
	if(app_test_cmd_callback[cmd_package->subcmd] == NULL)
	{
		return;
	}
	app_test_cmd_callback[cmd_package->subcmd](cmd_package);
}
/*******************************************************************************
 * Function Name     : app_cmd_package_parse
 * Description       : 接收到上位机命令包解析
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
void app_cmd_package_parse(uint8_t *cmd_pack,uint16_t pack_length)
{
	struct app_cmd_package *cmd_package = (struct app_cmd_package*)cmd_pack;
    cmd_package->length = pack_length;
	
	switch(cmd_package->cmd)
	{
		case CMD_SET_TIME:
		{
			app_cmd_set_time_callback(cmd_package);
			break;
		}
		case CMD_GET_VERSION:
		{
			app_cmd_get_version_callback(cmd_package);
			break;
		}
		case CMD_GET_BAT:
		{
            app_cmd_get_vbat(cmd_package);
			break;
		}
		case CMD_GET_HRV:
		{
			app_cmd_get_hrv(cmd_package);
			break;
		}
		case CMD_GET_SPO:
		{
			app_cmd_get_spo2(cmd_package);
			break;
		}
		case CMD_GET_TEMP:
		{
			app_cmd_get_tempertion(cmd_package);
			break;
		}
		case CMD_GET_SPORT:
		{
			app_cmd_get_step_count(cmd_package);
			break;
		}
		case CMD_GET_HISTORY:
		{
			app_cmd_get_hrstory(cmd_package);
			break;
		}
		case CMD_SYS_SET:
		{
			app_cmd_set_sys(cmd_package);
			break;
		}
		case CMD_TOOL_TEST:
		{
			app_test_cmd_handler(cmd_package);
			break;
		}
		case CMD_GET_PPG_SPO2:
		{
			app_cmd_get_ppg_spo2(cmd_package);
			break;
		}
		case CMD_PUF:
		{
			app_cmd_puf(cmd_package);
			break;
		}
		case CMD_AUTHENTICATION:
		{
			app_cmd_authentication(cmd_package);
			break;
		}
		case CMD_NFC:
		{
			app_cmd_nfc(cmd_package);
			break;
		}
		case CMD_SIX_AXIS_SENSOR :
		{
            #ifndef HANDWARE_1_23_4
			app_cmd_six_axis_sensor(cmd_package);
            #endif
			break;
		}
		case  CMD_GET_IR:
		{
			app_cmd_get_ir(cmd_package);
			break;
		}
		case CMD_LED:
		{
			app_cmd_led(cmd_package);
			break;
		}
		case CMD_HID:
		{
			app_cmd_hid(cmd_package);
			break;
		}
		case  CMD_CONFIG_TOUCH :
		{
			app_cmd_config_touch(cmd_package);
			break;
		}
		case CMD_PDM:
		{
			app_cmd_pdm(cmd_package);
			break;
		}
#if defined(HANDWARE_1_23_2)
		case CMD_LED_MOTOR_MODE_SET:
		{
			app_cmd_led_motor_mode_set(cmd_package);
			break;
		}
		case CMD_LED_MOTOR_MODE_GET:
		{
			app_cmd_led_motor_mode_get(cmd_package);
			break;
		}
#endif
		case CMD_MOTOR:
		{
			app_cmd_motor(cmd_package);
			break;
		}
		case CMD_PORT_MODE:
		{
			app_cmd_port_mode(cmd_package);
			break;
		}
		case CMD_APP_EVENT:
		{
			app_cmd_app_event(cmd_package);
			break;
		}	
    case CMD_ALARM:
		{
			app_cmd_rtc_alarm_clock_event(cmd_package);
			break;
		}
		case CMD_GET_PPG_LED:
		{
			app_cmd_ppg_led_data_get(cmd_package);
			break;
		}		
    case CMD_WIFI:
    {
      app_cmd_wifi_event(cmd_package);
      break;
    }
    case CMD_IPC:
    {
      app_cmd_ipc_event(cmd_package);
      break;
    }      
		default:
		{
			break;
		}
	}
}















