#include "bc_ppg.h"
#include "bc_delay.h"

#include "bc_ldo_switch.h"

#include "bc_ppg_driver_port.h"
#include "bc_ldo_switch.h"

#include "bc_logger.h"

#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

#include "hx3605.h"
#include "hx3605_spo2_agc.h"
#include "hx3605_hrs_agc.h"
#include "hx3605_factory_test.h"
#include "hx3605_led_config.h"

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

#include "zspd4000_drv.h"
#include "zsbm_algo.h"

ZSBM_ALGO_INIT_PARAMETERS zspd_algo_init_para;

#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T

#include "gh_demo.h"

#elif (PPG_DEVIECE_TYPE == 4)  //HX3918

#include "hx3918.h"
#include "hx3918_spo2_agc.h"
#include "hx3918_hrs_agc.h"
#include "hx3918_factory_test.h"
#include "hx3918_led_config.h"
#endif

#include "bc_logger.h"






#include "string.h"





bool bc_ppg_init(void)
{

#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403

#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000
 
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T


	bc_ppg_reset_high();	
	
	
    bc_ldo_ppg_power_on();
	bc_ppg_i2c_open();
    bc_delay_ms(100);
    if(Gh3x2xDemoInit() == GH3X2X_RET_OK)
    {
        BC_LOG_INFO("%s","------------------success ppg init----------------------------- \r\n");
        bc_ppg_i2c_close();
        bc_ldo_ppg_power_off();
        bc_ppg_int_io_irq_disable();

         bc_ppg_reset_low();	
			
		
        return true;
    }
    BC_LOG_INFO("%s","------------------failed ppg init----------------------------- \r\n");
	bc_ppg_i2c_close();
	bc_ldo_ppg_power_off();
	bc_ppg_int_io_irq_disable();

	bc_ppg_reset_low();	

    return false;
#endif
}


#if (PPG_DEVIECE_TYPE == 3)  //gh3228T

static bool bc_ppg_gh3026_init(void)
{

	bc_ppg_reset_high();	

	
	bc_ppg_i2c_open();
    if(Gh3x2xDemoInit() == GH3X2X_RET_OK)
    {
        BC_LOG_INFO("%s","------------------success ppg init----------------------------- \r\n");
        bc_ppg_i2c_close();
        return true;
    }
    BC_LOG_INFO("%s","------------------failed ppg init----------------------------- \r\n");
	bc_ppg_i2c_close();
}


#endif


bool bc_ppg_init_hr(void)
{

	
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605
hx3605_ppg_on();
hx3605_init(HRS_MODE,1,0);

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	if(ZSPD4000_Init(HR_MODE) == ZSPD_OK)
	{
			zspd_algo_init_para.algo_type = PPG_REST_HR_TIME;
			zspd_algo_init_para.sample_rate_ppg = 25; 
			zspd_algo_init_para.bit_width_ppg = 16;
			zspd_algo_init_para.maximum_val = 32767;
		    zspd_algo_init_para.pd_nums = 1;
			zspd_algo_init_para.wear_position = 1;
			ZSPD4000_algo_init_para(&zspd_algo_init_para);
			ZSBM_AlgoInit(zspd_algo_buffer_addr(), &zspd_algo_init_para);
//		ZSPD_IRDeteLowFs();
		return true;
	}
	return false;
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T

    bc_ppg_reset_high();	
	
    bc_ppg_int_io_irq_enable();
    Gh3x2xDemoArrayCfgSwitch(0);
    gh3228t_clear_dataNum();
#if ( HARDWARE_156_ENABLED == 1 || HARDWARE_1171_ENABLED == 1)	

    Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_HR | GH3X2X_FUNCTION_HRV );
 
#else

    Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_HR );

#endif    
    
    GH3X2X_FifoWatermarkThrConfig(20);	
//    hal_gh3x2x_int_handler_call_back();
//    Gh3x2xDemoInterruptProcess();    
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	hx3918_ppg_on();
	hx3918_init(HRS_MODE,1,0);
	hx3918_g_sensor_data_get_timer_start();

#endif
}

bool bc_ppg_init_ecg(void)
{

    
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605


#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

    
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T

	bc_ppg_reset_high();	
	
	bc_ppg_int_io_irq_enable();
    gh3228t_clear_dataNum();
//    Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_ECG);
    Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_LEAD_DET);
    GH3X2X_FifoWatermarkThrConfig(40);
//    Gh3x2xDemoInterruptProcess();
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918
	
#endif
}

bool bc_ppg_init_bt(void)
{

    
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605


#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

    
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T

	bc_ppg_reset_high();	
	
	bc_ppg_int_io_irq_enable();
    gh3228t_clear_dataNum();
    Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_BT);
    GH3X2X_FifoWatermarkThrConfig(20);
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918
	
#endif
}

bool bc_ppg_init_pwtt(void)
{

    
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605


#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

    
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T

	bc_ppg_reset_high();	
	
	bc_ppg_int_io_irq_enable();
    gh3228t_clear_dataNum();
    Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_TEST1);
    GH3X2X_FifoWatermarkThrConfig(40);
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918
	
#endif
}

bool bc_ppg_init_hrv(void)
{
	
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605



#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	if(ZSPD4000_Init(HRV_MODE) == ZSPD_OK)
	{
		
		return true;
	}
	return false;
	
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T	
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	

#endif
}

bool bc_ppg_init_spo2(void)
{

#if (PPG_DEVIECE_TYPE == 0)   //hx 3605
    hx3605_ppg_on();
	hx3605_init(SPO2_MODE,1,0);

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	if(ZSPD4000_Init(SPO2_MODE) == ZSPD_OK)
	{
		zspd_algo_init_para.algo_type = PPG_SPO2;
		zspd_algo_init_para.sample_rate_ppg = 100; 
		zspd_algo_init_para.bit_width_ppg = 32;
		zspd_algo_init_para.maximum_val = 32767*16;
		zspd_algo_init_para.pd_nums = 1;
		zspd_algo_init_para.wear_position = 1;
//		zspd_algo_init_para.maximum_val = 2147483648; //32767*16;  //2??31?η? 2147483648
		ZSPD4000_algo_init_para(&zspd_algo_init_para);
		ZSBM_AlgoInit(zspd_algo_buffer_addr(), &zspd_algo_init_para);
	//	bc_delay_ms(5);
		return true;
	}
	return false;
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T

	bc_ppg_reset_high();	

    bc_ppg_int_io_irq_enable();

#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	
	Gh3x2xDemoArrayCfgSwitch(1);
    gh3228t_clear_dataNum();
    
#if (defined(FLASH_GD25B512) && defined(HANDWARE_1_14_3))

	Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_TEST2 | GH3X2X_FUNCTION_HR | GH3X2X_FUNCTION_HRV | GH3X2X_FUNCTION_SPO2);
#elif (HARDWARE_1141_ENABLED == 1)	
  
  Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_TEST2 | GH3X2X_FUNCTION_HR | GH3X2X_FUNCTION_HRV );  
//Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_TEST2  ); 
    
#endif
  
#if(HARDWARE_1141_ENABLED == 1)
    GH3X2X_FifoWatermarkThrConfig(15);   //fifo的单位是32bit，注意是每个通道，15是3通道 每通道5个点的数据
#else
    GH3X2X_FifoWatermarkThrConfig(50);
#endif
    
#else
     Gh3x2xDemoArrayCfgSwitch(0);
    gh3228t_clear_dataNum();
    Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_SPO2 );
    GH3X2X_FifoWatermarkThrConfig(50);

#endif
		
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	
    hx3918_ppg_on();
	hx3918_init(SPO2_MODE,1,0);
	hx3918_g_sensor_data_get_timer_start();
	
#endif
}


bool bc_ppg_init_rawdata_collection(uint16_t frequency)
{

#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T

	bc_ppg_reset_high();	

    bc_ppg_int_io_irq_enable();
  switch(frequency)
  {
    case 25:
    {
      Gh3x2xDemoArrayCfgSwitch(0);
      break;
    }
    case 50:
    {
      Gh3x2xDemoArrayCfgSwitch(2);
      break;
    }
    case 100:
    {
      Gh3x2xDemoArrayCfgSwitch(1);
      break;
    }
    default:
    {
      Gh3x2xDemoArrayCfgSwitch(0);
      break;
    }
  }
    gh3228t_clear_dataNum();
#if (defined(FLASH_GD25B512) && defined(HANDWARE_1_14_3))

	Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_TEST2 | GH3X2X_FUNCTION_HR | GH3X2X_FUNCTION_HRV | GH3X2X_FUNCTION_SPO2);
#elif (HARDWARE_1141_ENABLED == 1)	
  
//  Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_TEST2 | GH3X2X_FUNCTION_HR | GH3X2X_FUNCTION_HRV );  
     Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_TEST2  ); 
#endif

    GH3X2X_FifoWatermarkThrConfig(15);   //fifo的单位是32bit，注意是每个通道，15是3通道 每通道5个点的数据

		
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	
    hx3918_ppg_on();
	hx3918_init(SPO2_MODE,1,0);
	hx3918_g_sensor_data_get_timer_start();
	
#endif
}



bool bc_ppg_init_ir(void)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605



#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	if(ZSPD4000_Init(SPO2_MODE) == ZSPD_OK)
	{
		zspd_algo_init_para.algo_type = PPG_SPO2;
		zspd_algo_init_para.sample_rate_ppg = 100; 
		zspd_algo_init_para.bit_width_ppg = 32;
		zspd_algo_init_para.maximum_val = 32767*16;
		zspd_algo_init_para.pd_nums = 1;
		zspd_algo_init_para.wear_position = 1;
//		zspd_algo_init_para.maximum_val = 2147483648; //32767*16;  //2??31?η? 2147483648
		ZSPD4000_algo_init_para(&zspd_algo_init_para);
		ZSBM_AlgoInit(zspd_algo_buffer_addr(), &zspd_algo_init_para);
		ppg_ir_flag_set(true);
		bc_delay_ms(5);
		return true;
	}
	return false;
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T

	bc_ppg_reset_high();	

    bc_ppg_int_io_irq_enable();
    gh3228t_clear_dataNum();
    Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_SPO2);
    GH3X2X_FifoWatermarkThrConfig(20);		
	
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	

#endif

}

void bc_ppg_hr_unint(void)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

	hx3605_hrs_disable();
	hx3605_ppg_off();

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	ppg_ir_flag_set(false);
	ZSPD4000_unint();
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
  
#if ( HARDWARE_156_ENABLED == 1 || HARDWARE_1171_ENABLED == 1)	
 Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_HR | GH3X2X_FUNCTION_HRV );
#else
   Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_HR );

#endif 
   
	bc_ppg_int_io_irq_disable();

	bc_ppg_reset_low();	
	
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	hx3918_g_sensor_data_get_timer_stop();
	hx3918_hrs_disable();
	hx3918_ppg_off();
	
#endif

}


void bc_ppg_ecg_unint(void)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605


#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

    
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
    Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_ECG);
    Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_LEAD_DET);
	bc_ppg_int_io_irq_disable();

	bc_ppg_reset_low();	
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918
	
#endif

}

void bc_ppg_bt_unint(void)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605


#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

    
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
    Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_BT);
	bc_ppg_int_io_irq_disable();

	bc_ppg_reset_low();	
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	
#endif

}

void bc_ppg_pwtt_unint(void)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605


#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

    
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
    Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_TEST1);
	bc_ppg_int_io_irq_disable();

	bc_ppg_reset_low();	
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	
#endif

}

void bc_ppg_spo2_unint(void)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

	hx3605_spo2_disable();
	hx3605_ppg_off();

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	ppg_ir_flag_set(false);
	ZSPD4000_unint();
	
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
	
	
#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	

#if (defined(FLASH_GD25B512) && defined(HANDWARE_1_14_3))

	Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_TEST2 | GH3X2X_FUNCTION_HR | GH3X2X_FUNCTION_HRV | GH3X2X_FUNCTION_SPO2);
#elif (HARDWARE_1141_ENABLED == 1)	
  
  Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_TEST2 | GH3X2X_FUNCTION_HR | GH3X2X_FUNCTION_HRV );  
// Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_TEST2 ); 
    
#endif  
  
#else
	Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_SPO2 );	
#endif
   
	bc_ppg_int_io_irq_disable();
	
	bc_ppg_reset_low();	
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	hx3918_g_sensor_data_get_timer_stop();
	hx3918_spo2_disable();
	hx3918_ppg_off();
#endif

}


void bc_ppg_rawdata_collection_unint(void)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

	hx3605_spo2_disable();
	hx3605_ppg_off();

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	ppg_ir_flag_set(false);
	ZSPD4000_unint();
	
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
	
	

#if (defined(FLASH_GD25B512) && defined(HANDWARE_1_14_3))

	Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_TEST2 | GH3X2X_FUNCTION_HR | GH3X2X_FUNCTION_HRV | GH3X2X_FUNCTION_SPO2);
#elif (HARDWARE_1141_ENABLED == 1)	
  
//  Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_TEST2 | GH3X2X_FUNCTION_HR | GH3X2X_FUNCTION_HRV );  
    Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_TEST2 ); 
#endif  

   
	bc_ppg_int_io_irq_disable();
	
	bc_ppg_reset_low();	
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	hx3918_g_sensor_data_get_timer_stop();
	hx3918_spo2_disable();
	hx3918_ppg_off();
#endif

}




void bc_ppg_gray_card_init(void)
{
	
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605
	hx3605_ppg_on();
	hx3605_init(FT_GRAY_CARD_MODE,0,0);
	
#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	if(ZSPD4000_Init(FT_GRI_MODE) == ZSPD_OK)
	{
	}
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T

	bc_ppg_reset_high();	
		
	bc_ppg_int_io_irq_enable();
	Gh3x2xDemoArrayCfgSwitch(3);
    gh3228t_clear_dataNum();
    bc_ppg_int_io_irq_enable();
    Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_TEST1);
    GH3X2X_FifoWatermarkThrConfig(5);

#elif (PPG_DEVIECE_TYPE == 4)  //HX3918
	hx3918_ppg_on();
	hx3918_init(FT_GRAY_CARD_MODE,0,0);
#endif	
	
}

void bc_ppg_gray_card_uninit(void)
{
	
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605
	hx3605_ppg_off();
#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	ppg_ir_flag_set(false);
	ZSPD4000_unint();
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
    Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_TEST1);
    bc_ppg_int_io_irq_disable();

	bc_ppg_reset_low();	

#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	hx3918_ppg_off();
#endif	
	
}


void bc_ppg_io_irq_handler(void)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605



#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000
	zspd_int_flag = 1;
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
    hal_gh3x2x_int_handler_call_back();	

#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	

#endif

}

void bc_ppg_data_handler_poll(void)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605



#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000
	while(!ZSPD4000_DataHandle())
	{
		bc_delay_ms(5);
		zspd_int_flag = 1;
	}
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
    Gh3x2xDemoInterruptProcess();

#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	
#endif

}

void bc_ppg_red_led_on(void)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

    bc_ldo_ppg_power_on();
	bc_delay_ms(20);
	bc_ppg_i2c_open();
	hx3605_ppg_on();
      hx3605_ledon_init(4);

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	bc_ldo_ppg_power_on();
	bc_delay_ms(100);
	bc_ppg_i2c_open();
//	ZSPD4000_CommonInit();
	ZSPD_IrRedNormal(SAMPLE_FS_100HZ);;
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T

	bc_ppg_reset_high();	
	
    bc_ldo_ppg_power_on();
    bc_delay_ms(20);
    bc_ppg_i2c_open();
    Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_TEST1);

#elif (PPG_DEVIECE_TYPE == 4)  //HX3918
	bc_ldo_ppg_power_on();
	bc_delay_ms(20);
	bc_ppg_i2c_open();
	hx3918_ppg_on();
    hx3918_ledon_init(4);
#endif
}

void bc_ppg_gre_led_on(void)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605
    bc_ldo_ppg_power_on();
	bc_delay_ms(20);
	bc_ppg_i2c_open();
	hx3605_ppg_on();
    hx3605_ledon_init(1);

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	bc_ldo_ppg_power_on();
	bc_delay_ms(100);
	bc_ppg_i2c_open();
	
	ZSPD_GreenNormal(SAMPLE_FS_100HZ);
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T

	bc_ppg_reset_high();	
	
    bc_ldo_ppg_power_on();
    bc_delay_ms(20);
    bc_ppg_i2c_open();
    Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_TEST2);
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	bc_ldo_ppg_power_on();
	bc_delay_ms(20);
	bc_ppg_i2c_open();
	hx3918_ppg_on();
    hx3918_ledon_init(1);
#endif

}

void bc_ppg_ir_led_on(void)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

  bc_ldo_ppg_power_on();
	bc_delay_ms(20);
	bc_ppg_i2c_open();
	hx3605_ppg_on();
      hx3605_ledon_init(2);

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	bc_ldo_ppg_power_on();
	bc_delay_ms(20);
	bc_ppg_i2c_open();
	ZSPD4000_SetLedCurrent(Z_IR_CH, 100) ;
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	bc_ldo_ppg_power_on();
	bc_delay_ms(20);
	bc_ppg_i2c_open();
	hx3918_ppg_on();
    hx3918_ledon_init(2);
#endif
	
}

void bc_ppg_led_off(void)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

	hx3605_ledon_init(0);
	hx3605_ppg_off();
	bc_ppg_i2c_close();
	bc_ldo_ppg_power_off();	

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	bc_ppg_i2c_close();
	bc_ldo_ppg_power_off();	
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
//    Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_TEST1);
    Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_TEST2);
    bc_ppg_i2c_close();
    bc_ldo_ppg_power_off();

	bc_ppg_reset_low();	
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	hx3918_ledon_init(0);
	hx3918_ppg_off();
	bc_ppg_i2c_close();
	bc_ldo_ppg_power_off();	
#endif
	
}

uint8_t bc_ppg_chip_id_get(void)
{
	
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

	uint8_t temp;
	bc_ldo_ppg_power_on();
	bc_delay_ms(20);
    bc_ppg_i2c_open();
	hx3605_ppg_on();
	temp = hx3605_get_id();
	hx3605_ppg_off();
    bc_ppg_i2c_close();
	bc_ldo_ppg_power_off();
	return temp;

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	bc_ldo_ppg_power_on();
	bc_delay_ms(20);
	bc_ppg_i2c_open();
	uint16_t chip_id = ZSPD4000_ID_get();
	bc_ppg_i2c_close();
	chip_id = chip_id >> 8;
	bc_ldo_ppg_power_off();
	return (uint8_t)chip_id;
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T

	bc_ppg_reset_high();	

    bc_ldo_ppg_power_on();
    bc_delay_ms(50);
    bc_ppg_i2c_open();
    uint8_t chip_id = Gh3x2xDemo_getId();
    bc_ppg_i2c_close();
    bc_ldo_ppg_power_off();

	bc_ppg_reset_low();	
		
	return chip_id;
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	uint8_t temp;
	bc_ldo_ppg_power_on();
	bc_delay_ms(20);
    bc_ppg_i2c_open();
	hx3918_ppg_on();
	temp = hx3918_read_id();
	hx3918_ppg_off();
    bc_ppg_i2c_close();
	bc_ldo_ppg_power_off();
	return temp;
#endif

	
}

bool bc_ppg_hardware_id_check(void)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

	uint8_t temp;
	bc_ldo_ppg_power_on();
	bc_delay_ms(20);
    bc_ppg_i2c_open();
	hx3605_ppg_on();
	temp = hx3605_get_id();
	hx3605_ppg_off();
    bc_ppg_i2c_close();
	if(temp == 0x25)
	{
		bc_ppg_i2c_close();
		bc_ldo_ppg_power_off();
		return true;
	}
	bc_ppg_i2c_close();
	bc_ldo_ppg_power_off();
	return false;

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	bc_ldo_ppg_power_on();
	bc_delay_ms(20);
	bc_ppg_i2c_open();
	uint16_t chip_id = ZSPD4000_ID_get();
	bc_ppg_i2c_close();
	chip_id = chip_id >> 8;
	bc_ldo_ppg_power_off();
	
	if(chip_id == 0x04)
	{
		return true;
	}
	return false;
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T

	bc_ppg_reset_high();	

    bc_ldo_ppg_power_on();
    bc_delay_ms(50);
    bc_ppg_i2c_open();
    uint8_t chip_id = Gh3x2xDemo_getId();
    bc_ppg_i2c_close();
    bc_ldo_ppg_power_off();

	bc_ppg_reset_low();	
    
    if(chip_id == 0xA5)
    {
        return true;
    }
    return false;
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	uint8_t temp;
	bc_ldo_ppg_power_on();
	bc_delay_ms(20);
    bc_ppg_i2c_open();
	hx3918_ppg_on();
	temp = hx3918_read_id();
	hx3918_ppg_off();
    bc_ppg_i2c_close();
	bc_ldo_ppg_power_off();
	if(temp == 0x27)
	{
		return true;
	}
	return false;
#endif
	
}

bool bc_ppg_ecg_data_callback_regdister(void *function_callback)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605


#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000


#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
    return gh3228t_ecg_data_callback_register(function_callback);
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	
#endif
    
}



bool bc_ppg_hr_data_callback_regdister(void *function_callback)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

	return hx3605_hr_data_callback_register(function_callback);

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	return ZSPD4000_hr_data_callback_register(function_callback);
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
    return gh3228t_hr_data_callback_register(function_callback);
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	return hx3918_hr_data_callback_register(function_callback);
#endif
	
}

bool bc_ppg_spo2_data_callback_regdister(void *function_callback)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

	return hx3605_spo2_data_callback_register(function_callback);

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	return ZSPD4000_spo2_data_callback_register(function_callback);
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
    return gh3228t_spo2_data_callback_register(function_callback);
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	return hx3918_spo2_data_callback_register(function_callback);
#endif

}

bool bc_ppg_spo2_hr_data_callback_regdister(void *function_callback)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605


#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
	return gh3228t_spo2_hr_data_callback_register(function_callback);

#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	return hx3918_spo2_hr_data_callback_register(function_callback);
#endif

}

bool bc_ppg_pwtt_data_callback_regdister(void *function_callback)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605


#elif (PPG_DEVIECE_TYPE == 1)  // afe4403


#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000


#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
    return gh3228t_pwtt_data_callback_register(function_callback);
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	
#endif

}


bool bc_ppg_hr_result_callback_regdister(void *function_callback)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

	return hx3605_hr_result_callback_register(function_callback);

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	return ZSPD4000_hr_result_callback_register(function_callback);
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
    return gh3228t_hr_result_callback_register(function_callback);
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918
	return hx3918_hr_result_callback_register(function_callback);
#endif

}

bool bc_ppg_hrv_result_callback_regdister(void *function_callback)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605



#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000


#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
    return gh3228t_hrv_result_callback_register(function_callback);
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918

#endif

}

bool bc_ppg_spo2_result_callback_regdister(void *function_callback)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

	return hx3605_spo2_result_callback_register(function_callback);

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	return ZSPD4000_spo2_result_callback_register(function_callback);
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
    return gh3228t_spo2_result_callback_register(function_callback);
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	return hx3918_spo2_result_callback_register(function_callback);
#endif

}

bool bc_ppg_spo2_signal_check_callback_regdister(void *function_callback)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

	return hx3605_spo2_signal_check_callback_register(function_callback);

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	return ZSPD4000_spo2_signal_check_callback_register(function_callback);
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
	 return gh3228t_spo2_signal_check_callback_register(function_callback);
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	return hx3918_spo2_signal_check_callback_register(function_callback);
#endif

}

bool bc_ppg_hr_signal_check_callback_regdister(void *function_callback)
{
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

	return hx3605_hr_signal_check_callback_register(function_callback);

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	return ZSPD4000_hr_signal_check_callback_register(function_callback);
#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T
	return gh3228t_hr_signal_check_callback_register(function_callback);
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	return hx3918_hr_signal_check_callback_register(function_callback);
#endif

}




bool bc_ppg_gary_card_callback_regdister(void *function_callback)
{
	
	
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605

	return hx3605_gary_card_callback_register(function_callback);

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

	return ZSPD4000_gary_card_callback_register(function_callback);

#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T	
	
	return gh3228t_garyCard_result_callback_register(function_callback);
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	return  hx3918_gary_card_callback_register(function_callback);
#endif	

}


bool bc_ppg_g_sensor_callback_regdister(void *start_callback,void *stop_callback,void *read_callback)
{

#if (PPG_DEVIECE_TYPE == 0)   //hx 3605



#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000



#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T	
	
	return ppg_g_sensor_register_callback(start_callback,stop_callback,read_callback);
	
#elif (PPG_DEVIECE_TYPE == 4)  //HX3918	
	return ppg_g_sensor_register_callback(start_callback,stop_callback,read_callback);
#endif	
	
}

bool bc_ppg_agc_comp_flg(void)
{
	
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605



#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000

      return zspd400_agc_comp_flg();

#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T	
	
	
#endif		
}

void bc_ppg_current_val_set(uint8_t CurrentVal_0,uint8_t CurrentVal_1,uint8_t CurrentVal_2)
{
	
#if (PPG_DEVIECE_TYPE == 0)   //hx 3605



#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2)  // zspd4000



#elif (PPG_DEVIECE_TYPE == 3)  //gh3228T	
	
	ppg_current_val_set( CurrentVal_0, CurrentVal_1, CurrentVal_2);
#endif		
}
