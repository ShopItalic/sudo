#include "bc_module.h"

#include "bc_ble_modu_interface.h"
//#include "bc_nfc_port.h"
#include "bc_rtc.h"
//#include "bc_nfc.h"
#include "bc_ppg_driver_port.h"
#include "bc_queue.h"
#include "bc_sem.h"
#include "bc_ldo_switch.h"
#include "bc_pmic.h"
#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
#include "bc_spi_gsensor.h"
#else
#include "bc_gsensor.h"
#endif
//#include "bc_puf_i2c_driver.h"
#include "bc_temp.h"

#include "bc_led.h"
#include "bc_device_info.h"
#include "bc_strategy_value.h"
#include "bc_watchdog.h"

#include "bc_pmic_device_port.h"
#include "bc_ic_led.h"

#if ( HARDWARE_1191_ENABLED == 1)	

#include "bc_wifi_port.h"

#endif	

#if ( HARDWARE_1191_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

#include "bc_led_pwm.h"

#endif	


#include "ring_config.h"

#if ( HARDWARE_153_ENABLED == 1  || HARDWARE_441_ENABLED || HARDWARE_413_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED  || HARDWARE_158_ENABLED == 1  || HARDWARE_451_ENABLED == 1 ||\
      HARDWARE_1141_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1191_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

#include "bc_spi_flash_port.h"
	
#endif

#if ( HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1  || HARDWARE_1171_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

#include "bc_linear_motor.h"
	
#endif


#if ( HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1  || HARDWARE_1181_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

#include "bc_touch_button_device_port.h"
	
#endif

//#if ( HARDWARE_1121_ENABLED == 1)	
//#include "bc_mouse_device_port.h"
//	
//#endif


//#if ( HARDWARE_181_ENABLED == 1  )	

//#include "bc_nfc_rs2323_port.h"
//	
//#endif

#if ( HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_191_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3)  || defined(HANDWARE_1_23_4))	
#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
#include "bc_linear_motor_ic_port.h"
#include "bc_fuel_gauge_port.h"
#endif
#include "bc_temp_port.h"
	
#endif

#if ( HARDWARE_1121_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1191_ENABLED == 1)	

#include "bc_key.h"
	
#endif

//#if ( HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1)	

//#include "bc_nfc_st25dv_port.h"

//#endif

//#if ( HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1)	

//#include "bc_pressure_sensor.h"

//#endif


void bc_module_init(void)
{
	bc_sem_create();
	bc_queue_init();
	bc_device_info_init();
	bc_business_strategy_init();
#if (HARDWARE_ARCH_TYPE_NORDIC == 1)	
	struct bc_ble_calss ble_calss = bc_ble_new();
	ble_calss.ble_init();	

#endif	

	bc_rtc_device_find();	
	

	
#if (!defined(BLE_POWER_TEST))


#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1  || HARDWARE_413_ENABLED == 1 || HARDWARE_402_ENABLED == 1 || HARDWARE_153_ENABLED == 1 || HARDWARE_441_ENABLED == 1 || \
     HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_191_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 || \
     HARDWARE_1171_ENABLED == 1 || HARDWARE_1181_ENABLED == 1)
	bc_ppg_device_find();
	
#endif	


#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1  || HARDWARE_413_ENABLED == 1 || HARDWARE_402_ENABLED == 1 || HARDWARE_153_ENABLED == 1 || HARDWARE_441_ENABLED == 1 || \
     HARDWARE_1121_ENABLED == 1 || HARDWARE_181_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_191_ENABLED == 1 || \
     HARDWARE_1141_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)
#if defined(HANDWARE_1_23_3)
    bc_spi_gsensor_device_find();
#else
#ifndef HANDWARE_1_23_4
	bc_g_sensor_device_find();
#endif
#endif
#endif	

#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1  || HARDWARE_413_ENABLED == 1 || HARDWARE_402_ENABLED == 1 || HARDWARE_153_ENABLED == 1 || HARDWARE_181_ENABLED == 1  || HARDWARE_191_ENABLED == 1|| \
     HARDWARE_441_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1  || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || \
     HARDWARE_1141_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1191_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	
	bc_ldo_power_device_find();
	bc_pmic_device_find();
	
#endif		
	
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 )	

	bc_buf_i2c_device_find();
	
#endif		

#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1  || HARDWARE_413_ENABLED == 1 || HARDWARE_402_ENABLED == 1 || HARDWARE_153_ENABLED == 1 || HARDWARE_441_ENABLED == 1 || \
     HARDWARE_1121_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_191_ENABLED == 1 || HARDWARE_1141_ENABLED == 1  || HARDWARE_1181_ENABLED == 1)

	bc_temp_temperature_adc_find();
#endif			
 	
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_153_ENABLED == 1  || HARDWARE_441_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || \
       HARDWARE_156_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

	bc_touch_button_device_find();
	
#endif	
	
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1  || HARDWARE_413_ENABLED == 1 || HARDWARE_402_ENABLED == 1  || HARDWARE_441_ENABLED == 1  || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || \
       HARDWARE_1181_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1231x_ENABLED == 1)	

	bc_led_device_find();
#endif	
	

	bc_dog_device_find();
//	
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_153_ENABLED == 1  || HARDWARE_BCL601_151_ENABLED == 1 || HARDWARE_1121_ENABLED == 1  || HARDWARE_181_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || \
	 HARDWARE_156_ENABLED == 1 || HARDWARE_191_ENABLED == 1  || HARDWARE_1141_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	
    bc_pmic_device_stacmd_find();
	
#endif		

#if (HARDWARE_153_ENABLED == 1 || HARDWARE_158_ENABLED == 1  || HARDWARE_156_ENABLED == 1  || HARDWARE_1171_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	
#if defined(HANDWARE_1_23_3)
    bc_linear_motor_device_i2c_find();
#else
#ifndef HANDWARE_1_23_4
	bc_linear_motor_device_find();		
#endif
#endif
#endif	

//#if (HARDWARE_153_ENABLED == 1)	
//	bc_piezoelectric_motor_device_i2c_find();
//	
//#endif

#if (HARDWARE_153_ENABLED == 1 || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	
  bc_ic_led_init();
	
#endif

#if ( HARDWARE_1191_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

    //bc_led_pwm_device_find();

#endif	

//#if ( HARDWARE_1121_ENABLED == 1)	
//	bc_mouse_device_i2c_find();
//	
//#endif

#if (HARDWARE_153_ENABLED == 1 || HARDWARE_441_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED == 1 || HARDWARE_158_ENABLED == 1  || HARDWARE_451_ENABLED == 1 || \
     HARDWARE_1141_ENABLED == 1 ||  HARDWARE_1171_ENABLED == 1 || HARDWARE_1191_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	
    bc_spi_flash_device_find();
	
#endif	

//#if ( HARDWARE_181_ENABLED == 1  )	

//    bc_nfc_rs2323_device_find();
//	
//#endif

#if ( HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_191_ENABLED == 1 || HARDWARE_1141_ENABLED == 1|| HARDWARE_1181_ENABLED == 1 || (defined(HANDWARE_1_23_2) ) || (defined(HANDWARE_1_23_3) ) )	
	 bc_temper_device_i2c_find(); //liukun 20260508 注释掉为了调试aw8235
#endif

#if ( HARDWARE_1121_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1191_ENABLED == 1)	
	 bc_key_device_find();
#endif

//#if ( HARDWARE_158_ENABLED == 1  || HARDWARE_156_ENABLED == 1)	
//	bc_nfc_st25dv_device_find();
//#endif

//#if ( HARDWARE_158_ENABLED == 1 || HARDWARE_156_ENABLED == 1)	
//	bc_pressure_sensor_adc_devicet_adc_find();
//#endif
//	
#if ( HARDWARE_1191_ENABLED == 1)	

    bc_wifi_device_find();

#endif	

#if (defined(HANDWARE_1_23_3 ) || (defined(HANDWARE_1_23_4) ))
    bc_cw221x_device_find();
#endif

#endif
}

















