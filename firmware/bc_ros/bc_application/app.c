#include "app.h"


#include "app_rtc_handler.h"
#include "app_ppg_handler.h"
#include "app_ble_handler.h"
#include "app_ppg_data_handler.h"
#include "app_pmic_handler.h"
#include "app_g_sensor_handler.h"
//#include "app_authentication_handler.h"
//#include "app_nfc_handler.h"
//#include "app_tsdb_handler.h"
//#include "app_sleep_handler.h"
//#include "app_model_handler.h"
#include "app_hardware_check.h"
#include "app_touch_button_handler.h"
//#include "app_led_handler.h"
#include "app_temper_handler.h"
#include "app_six_axis_sensor_handler.h"
//#include "app_hid_handler.h"
#include "app_ble_speed_handler.h"


#include "ring_config.h"

#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4 ))
#include "bc_linear_motor_ic.h"
#include "bc_fuel_gauge.h"
#endif

#if (HARDWARE_1191_ENABLED == 1 )	
#include "app_hardline_tsdb_handler.h"
#endif


#if (HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1  || HARDWARE_156_ENABLED == 1  || HARDWARE_1171_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

#include "app_linear_motor_handler.h"

#endif	

#if (HARDWARE_153_ENABLED == 1 || HARDWARE_1231_ENABLED == 1 )	

#include "app_motor_handler.h"

#endif

#if (HARDWARE_153_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || \
    HARDWARE_1191_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

#include "app_pdm_handler.h"

#endif	

#if (HARDWARE_441_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_158_ENABLED == 1  || HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1191_ENABLED == 1 \
     || HARDWARE_1231_ENABLED == 1)	

#include "app_ppg_file_data_handler.h"		
   
#endif

#if ( HARDWARE_1121_ENABLED == 1 )	

#include "app_mouse_handler.h"


#endif	

#if ( HARDWARE_1121_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1191_ENABLED == 1  || HARDWARE_1231_ENABLED == 1)	


#include "app_key_handler.h"

#endif	


#include "app_opus.h"

//#if ( HARDWARE_158_ENABLED == 1  || HARDWARE_156_ENABLED == 1)	

//#include "app_nfc_charing_tag.h"

//#endif

//#if ( HARDWARE_451_ENABLED == 1)	

//#include "app_sport_adv_handler.h"

//#endif

void app_init(void)
{
//	/*   初始化rtc  */
	app_rtc_handler_init();	
	/*   初始化ble资源  */
	app_ble_handler_thread_create();
	app_package_init();
  
#if (!defined(BLE_POWER_TEST))
  
#if !defined(SUDO_VOICE_ONLY)
  app_ble_speed_time_create();
#endif
  
//	
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_153_ENABLED == 1  || HARDWARE_441_ENABLED == 1 || HARDWARE_191_ENABLED == 1 || \
	 HARDWARE_402_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 || \
   HARDWARE_1171_ENABLED == 1 || HARDWARE_1181_ENABLED == 1)		
	
	/*   初始化ppg相关资源  */
	app_ppg_init();
	app_ppg_data_handler_task_event_init();
	
#endif	

#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_153_ENABLED == 1  || HARDWARE_181_ENABLED == 1 || HARDWARE_191_ENABLED == 1 || \
	 HARDWARE_441_ENABLED == 1 || HARDWARE_402_ENABLED == 1 || HARDWARE_1121_ENABLED == 1|| HARDWARE_158_ENABLED == 1  || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1|| \
   HARDWARE_1141_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)		

//#ifndef HANDWARE_1_23_3
#ifndef HANDWARE_1_23_4
	/*   初始化三轴 六轴 相关资源  */
	app_g_sensor_time_create();
	app_six_axis_sensor_time_create();
#endif
//#endif
	
#endif	
//	
	/*   初始化电源管理 相关资源  */	
	app_pmic_handler_timer_create();

//	
//#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_441_ENABLED == 1)	

//	/*   初始化恒爱鉴权秘钥  */	
//	app_authentication_info_init();
//	
//#endif	

//#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_153_ENABLED == 1 || HARDWARE_181_ENABLED == 1 || HARDWARE_441_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || \
//     HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1)	

//	/*   初始化nfc  */	
//	app_nfc_init();
//	
//#endif	

//#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_153_ENABLED == 1  || HARDWARE_441_ENABLED == 1 || \
//     HARDWARE_402_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1  || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1)	

//	/*   初始化 fLashdb  */	
//	app_flashdb_init();
//	/*   初始化 睡眠处理资源  */	
//	app_sleep_handler_init();
//	/*   初始化 模式处理调度资源 */	
//	app_model_time_create();
//	
//#endif	

#if (HARDWARE_1191_ENABLED == 1 )	


	app_hardline_tsdb_create();
	
#endif




#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_153_ENABLED == 1  || HARDWARE_441_ENABLED == 1 || HARDWARE_158_ENABLED == 1  || \
     HARDWARE_156_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

	/*   初始化touch相关资源 */	
	app_touch_handler_init();
	
#endif
//	
//	
//	
//#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_153_ENABLED == 1 || HARDWARE_441_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || \
//     HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1)	

//    /*   初始化hid相关资源 */	
//	app_hid_handler_init();
//	
//#endif

//#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_441_ENABLED == 1 || HARDWARE_402_ENABLED == 1 || HARDWARE_156_ENABLED == 1 )	

//    /*   初始化led相关资源 */	
//	app_led_init();
//	
//#endif	

#if (HARDWARE_153_ENABLED == 1  || HARDWARE_BCL601_151_ENABLED || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || \
     HARDWARE_1191_ENABLED == 1  || HARDWARE_1231_ENABLED == 1)	

   /*   初始化pdm相关资源 */	
   app_pdm_thread_create();
   
   
#endif	

#if (HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
    bc_linear_motor_ic_device_init();

#else
    /*   初始化线性无刷马达相关资源 */	
	app_linear_motor_time_create();
#endif
   
#endif

//#if (HARDWARE_153_ENABLED == 1 )	

//    /*   初始化线性无刷马达相关资源 */	

//	app_motor_time_create();
//   
//#endif

#if (HARDWARE_441_ENABLED == 1  || HARDWARE_413_ENABLED == 1 || HARDWARE_159_ENABLED == 1  || HARDWARE_451_ENABLED == 1|| HARDWARE_1141_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1191_ENABLED == 1\
    || HARDWARE_1231_ENABLED == 1)

//    /*   初始化ppg文件处理相关资源 */	
#if !defined(SUDO_VOICE_ONLY)
   app_ppg_file_init();
#endif
   
#endif

#if ( HARDWARE_1121_ENABLED == 1)	

     /*   初始化mouse相关资源 */	
	app_mouse_init();


#endif	


#if ( HARDWARE_1121_ENABLED == 1 || HARDWARE_1171_ENABLED == 1  || HARDWARE_1191_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

	 /*   初始化key相关资源 */	
	app_key_handler_init();

#endif	




#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_153_ENABLED == 1 || HARDWARE_441_ENABLED == 1 || HARDWARE_402_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || \
     HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || defined(HANDWARE_1_23_2)  || defined(HANDWARE_1_23_3))	
#ifndef HANDWARE_1_23_3
    /*   初始化温度相关资源 */
	app_temper_time_create();//liukun 20260508 注释掉为了调试aw8235
#endif	
#endif	

//#if ( HARDWARE_158_ENABLED == 1  || HARDWARE_156_ENABLED == 1)	

//     app_nfc_charing_tag_init();

//#endif

//#if ( HARDWARE_451_ENABLED == 1)	

//     app_sport_adv_init();

//#endif




#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_153_ENABLED == 1  || HARDWARE_441_ENABLED == 1 || HARDWARE_191_ENABLED == 1 || \
	 HARDWARE_402_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 ||  \
   HARDWARE_1181_ENABLED == 1 || HARDWARE_1171_ENABLED == 1  || HARDWARE_1231_ENABLED == 1)		
	
	/*   初始化硬件校验相关资源 */	
	app_hardware_check_task_create();
	
#endif	



#else

///*   初始化pdm相关资源 */	
//   app_pdm_thread_create();

#endif	

#if defined(HANDWARE_1_23_3)
    //bc_linear_motor_ic_device_init();
#endif

#if defined(USE_OPUS)
    app_opus_create();
#endif
	
}
