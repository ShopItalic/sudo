#include "bc_touch_button.h"

#include "bc_touch_button_device_port.h"

#include "bc_logger.h"
#include "bc_delay.h"

#include "bc_ldo_switch.h"
#include "bc_delay.h"
		
#if(TOUCH_DEVIECE_TYPE == 0)  
#include "IQS323.h"
#elif(TOUCH_DEVIECE_TYPE == 1)
#include "IQS7211E.h"
#endif 	

#include "nrf_gpio.h"
#include "nrfx_gpiote.h"

#define IQS_REDY                NRF_GPIO_PIN_MAP(0,17)

static void bc_touch_enbale(void)
{
	config_flag_set(false);
}

static void bc_touch_disebale(void)
{
	config_flag_set(true);
}

#if 0
void bc_touch_button_reset(void)
{
    nrf_gpio_cfg_output(IQS_REDY);
    nrf_gpio_pin_write(IQS_REDY,0);
    bc_delay_ms(100);
    //BC_LOG_INFO("nrf_gpio_pin_read1 :%d\r\n", nrf_gpio_pin_out_read(IQS_REDY));
    nrf_gpio_pin_write(IQS_REDY,1);
    bc_delay_ms(130);
    //BC_LOG_INFO("nrf_gpio_pin_read2 :%d\r\n", nrf_gpio_pin_out_read(IQS_REDY));
    nrf_gpio_pin_write(IQS_REDY,0);
    bc_delay_ms(100);
    //BC_LOG_INFO("nrf_gpio_pin_read3 :%d\r\n", nrf_gpio_pin_out_read(IQS_REDY));
    nrf_gpio_cfg_input(IQS_REDY, NRF_GPIO_PIN_NOPULL);
    
}
#else
void bc_touch_button_reset(void)
{
	touch_rdy_out_open();
	touch_rdy_out_low();
	bc_delay_ms(10);
	touch_rdy_out_close();
}
#endif
uint8_t bc_touch_button_chip_id_get_noint(void)
{
    bc_touch_enbale();
    touch_io_irq_disnable();
    BC_LOG_INFO("touch_io_irq_disnable\r\n");
    
    // 复位触摸芯片，产生RDY中断
    bc_touch_button_reset();
    BC_LOG_INFO("bc_touch_button_reset\r\n");
    
    // 关键：等待RDY变为低电平（芯片准备好）
    uint32_t timeout = 1000;  // 10秒超时
    while(touch_io_irq_status() != 0 && timeout > 0) {
        bc_delay_ms(10);
        timeout--;
    }
    
    if(timeout == 0) {
        BC_LOG_ERROR("RDY timeout!\r\n");
        return 0;
    }
    
    BC_LOG_INFO("RDY is low, chip ready\r\n");
    
    uint8_t chip_id = 0;
    uint16_t id_raw = 0;
    
    // 现在可以安全地打开I2C并读取ID
    if(!touch_i2c_open()) {
        //IQS7211E_Stop_Bit_Disabled();
        uint8_t buffer[2];
	  BC_LOG_INFO("\n RDY IQS7211E_Stop_Bit_Disabled");
	  buffer[0] = CONFIG_SETTINGS0 | 0x40;  //bit6 : 1 Write one or two bytes (any data) to the address 0xFF followed by a STOP to end comms;
		//IQS_I2C_Write_Data(IQS7211E_ADDR,0x34,&buffer[0],1,1);
        touch_i2c_write(0x34 ,&buffer[0],1);
        
        // 读取寄存器0x00（Product Number）
        if(touch_i2c_read(0x00, (uint8_t*)&id_raw, 2)) {
            chip_id = (uint8_t)(id_raw & 0x00FF);
            BC_LOG_INFO("Read chip ID success: 0x%04X\r\n", id_raw);
        } else {
            BC_LOG_ERROR("I2C read failed\r\n");
        }
        touch_i2c_close();
    } else {
        BC_LOG_ERROR("I2C open failed\r\n");
    }
    
    // 重新使能中断
    //touch_io_irq_enable();
    
    return chip_id;
}
uint8_t sigreadid(void);


uint8_t bc_touch_button_chip_id_get(void)
{
    uint8_t chip_id = 0;
#if (HARDWARE_413_ENABLED == 1)		
		bc_ldo_touch_power_on();
	    bc_delay_ms(20);
#endif		
		
#if(TOUCH_DEVIECE_TYPE == 0)  
			
		uint16_t chip_id = 0;
	touch_io_irq_disnable();
	bc_touch_button_reset();
	bc_delay_ms(10);
	touch_i2c_open();
	touch_i2c_read(0 ,(uint8_t*)&chip_id,2);
	IQS323_Stop_I2C_Comm_Window();
	touch_i2c_close();
#elif(TOUCH_DEVIECE_TYPE == 1)
	
    //bc_delay_ms(8000);
    //BC_LOG_INFO("start **********iqs7211e_reg_getid: %d\r\n",chip_id);
    
#if 0    
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(0,17));
        nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(0,18));
        nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(0,21));
    while(1) {
        
        bc_delay_ms(200);
        nrf_gpio_pin_set(NRF_GPIO_PIN_MAP(0,17));
        nrf_gpio_pin_set(NRF_GPIO_PIN_MAP(0,18));
        nrf_gpio_pin_set(NRF_GPIO_PIN_MAP(0,21));
        bc_delay_ms(200);
        nrf_gpio_pin_clear(NRF_GPIO_PIN_MAP(0,17));
        nrf_gpio_pin_clear(NRF_GPIO_PIN_MAP(0,18));
        nrf_gpio_pin_clear(NRF_GPIO_PIN_MAP(0,21));
//        bc_touch_button_reset();
//        uint8_t pinv = nrf_gpio_pin_read(IQS_REDY);
//        BC_LOG_INFO("pinv : %d\r\n", pinv);
//        if(!pinv) {
//            if(sigreadid())
//                break;
//            bc_delay_ms(2000);
//        }
//        bc_delay_ms(5000);
    }
#endif
#if 1
    bc_touch_enbale();
    touch_io_irq_disnable();
    iqs7211e_single_read_id();
    bc_touch_button_reset();
    touch_io_irq_enable();
    for(int i=0; i < 3; i++)
    {
        if(!get_chip_status()) {
            chip_id = 0;
            bc_touch_enbale();
            touch_io_irq_disnable();
            iqs7211e_single_read_id();
            bc_touch_button_reset();
            touch_io_irq_enable();
            //bc_delay_ms(10);
            chip_id = iqs7211e_reg_getid();
            BC_LOG_INFO("iqs7211e_reg_getid: %d\r\n",chip_id);
            if(0x58 == chip_id)
                break;
        }
        bc_delay_ms(200);
    }

#else
	if(touch_i2c_open()) {
        BC_LOG_INFO("touch_i2c_open err or busy\r\n");
        return 0;
    }
//	IQS7211E_low_power_off();
	touch_i2c_read(0 ,(uint8_t*)&chip_id,2);
//	IQS7211E_low_power_on();
	touch_i2c_close();
#endif
#endif 	

	
	
//	
	
#if (HARDWARE_413_ENABLED == 1)		
		bc_ldo_touch_power_off();
#endif	
	
	
	//BC_LOG_INFO("touch button chip id:%02x   %d\r\n",chip_id,chip_id);
	return chip_id;
}

bool bc_touch_button_chip_id_hardware_check(void)
{

  
#if(TOUCH_DEVIECE_TYPE == 0)  
			
	if(bc_touch_button_chip_id_get() == 0x52)
	{
		bc_touch_disebale();
		return true;
	}
#elif(TOUCH_DEVIECE_TYPE == 1)
//	bc_touch_enbale();
//	IQS7211E_low_power_off();
//	bc_touch_button_init();
    BC_LOG_INFO("goto bc_touch_button_chip_id_get\r\n");
	if(bc_touch_button_chip_id_get() == 0x58)  //1112
	{

//		bc_touch_disebale();
//		bc_touch_button_reset();
//		bc_touch_button_reset();
//		bc_touch_button_chip_id_get() ;
		//bc_touch_button_uninit();
//		bc_touch_disebale();
//		bc_touch_button_reset();
		return true;
	}
    //bc_touch_button_uninit();
#endif  


	
	
	//bc_touch_disebale();
	//bc_touch_button_reset();
	return false;
}



void bc_touch_button_init(void)
{
#if(TOUCH_DEVIECE_TYPE == 0)  
		
  
  	bc_touch_enbale();

	bc_touch_button_chip_id_get();
#elif(TOUCH_DEVIECE_TYPE == 1)
//	IQS7211E_Init();
//	IQS7211E_low_power_off();
	bc_touch_enbale();
	touch_io_irq_disnable();
	bc_touch_button_reset();
	bc_delay_ms(50);
	bc_touch_button_irq_process();
//	
//	IQS7211E_Init();
//	bc_touch_button_chip_id_get();
#endif  	
	
	
	
#if (HARDWARE_413_ENABLED == 1)		
		bc_ldo_touch_power_on();
	    bc_delay_ms(20);
#endif		

	
}

void bc_touch_button_uninit(void)
{
  
#if(TOUCH_DEVIECE_TYPE == 0)  
		
  
  bc_touch_disebale();

	bc_touch_button_chip_id_get();
#elif(TOUCH_DEVIECE_TYPE == 1)
  bc_touch_disebale();
	touch_io_irq_disnable();
	bc_touch_button_reset();
	//bc_delay_ms(50);
	bc_touch_button_irq_process();
//	IQS7211E_low_power_on();
//	bc_touch_button_chip_id_get();
//	
//	IQS7211E_unInit();
#endif   
  

	
	
#if (HARDWARE_413_ENABLED == 1)		
		bc_ldo_touch_power_on();
	    bc_delay_ms(50);
#endif

//	
//	touch_i2c_close();
#if (HARDWARE_413_ENABLED == 1)		
		bc_ldo_touch_power_off();
#endif	

}


void bc_touch_button_irq_process(void)
{
#if (HARDWARE_413_ENABLED == 1)		
		bc_ldo_touch_power_on();
	    bc_delay_ms(10);
#endif		
//	if(touch_i2c_open()) {
//        BC_LOG_INFO("bc_touch_button_irq_process touch_i2c_open fail\r\n");
//        return;
//    }
//	
  
#if(TOUCH_DEVIECE_TYPE == 0)  
	Process_IQS323_Events();
#elif(TOUCH_DEVIECE_TYPE == 1)
  Process_IQS7211E_Events();
#endif    

//	touch_i2c_close();
#if (HARDWARE_413_ENABLED == 1)		
		bc_ldo_touch_power_off();
#endif	
}

#if defined(SUDO_VOICE_ONLY)
bool bc_touch_button_touch_report_register_callback(bc_touch_report_callback_t callback)
{
#if(TOUCH_DEVIECE_TYPE == 1)
	return IQS7211E_touch_report_register_callback(callback);
#else
	(void)callback;
	return false;
#endif
}
#endif


bool bc_touch_button_config_flag_get(void)
{
	return config_flag_get();
}


bool bc_touch_button_gesture_event_flick_positive_register_callback(void *callback)
{
	return gesture_event_flick_positive_register_callback(callback);
}

bool bc_touch_button_gesture_event_swipe_positive_register_callback(void *callback)
{
	return gesture_event_swipe_positive_register_callback(callback);
}

bool bc_touch_button_CH0_in_touch_register_callback(void *callback)
{
#if(TOUCH_DEVIECE_TYPE == 0)  
	return CH0_in_touch_register_callback(callback);
#elif(TOUCH_DEVIECE_TYPE == 1)
#endif  
}

bool bc_touch_button_CH1_in_touch_register_callback(void *callback)
{
#if(TOUCH_DEVIECE_TYPE == 0)  
	return CH1_in_touch_register_callback(callback);
#elif(TOUCH_DEVIECE_TYPE == 1)
#endif    
	
}

bool bc_touch_button_CH2_in_touch_register_callback(void *callback)
{
#if(TOUCH_DEVIECE_TYPE == 0)  
	return CH2_in_touch_register_callback(callback);
#elif(TOUCH_DEVIECE_TYPE == 1)
#endif  
	
}



bool bc_touch_button_error_register_callback(void *callback)
{
	return error_register_callback(callback);
}

bool bc_touch_button_gesture_event_hold_register_callback(void *callback)
{
	return gesture_event_hold_register_callback(callback);
}

bool bc_touch_button_gesture_event_flick_negative_register_callback(void *callback)
{
	return gesture_event_flick_negative_register_callback(callback);
}


bool bc_touch_event_rawdata_callback_register_callback(void *callback)
{
	return event_rawdata_callback_register_callback(callback);
}

bool bc_touch_check_callback_register_callback(void *callback)
{
  
#if(TOUCH_DEVIECE_TYPE == 0)  
	return check_callback_register_callback(callback);
#elif(TOUCH_DEVIECE_TYPE == 1)
#endif   
	
}

bool bc_touch_single_tap_register_callback_register_callback(void *callback)
{

#if ( HARDWARE_191_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	

	return single_tap_register_callback(callback);

#else

#endif	
	
}

bool bc_touch_double_tap_register_callback_register_callback(void *callback)
{
  
#if(TOUCH_DEVIECE_TYPE == 0)  

#elif(TOUCH_DEVIECE_TYPE == 1)
  return double_tap_register_callback(callback);
#endif   		
	
}

bool bc_touch_triple_tap_register_callback_register_callback(void *callback)
{
  
#if(TOUCH_DEVIECE_TYPE == 0)  

#elif(TOUCH_DEVIECE_TYPE == 1)
  return triple_tap_register_callback(callback);
#endif   		
	
}



bool bc_touch_swipe_left_register_callback_register_callback(void *callback)
{		
	
#if(TOUCH_DEVIECE_TYPE == 0)  

#elif(TOUCH_DEVIECE_TYPE == 1)
  return swipe_left_register_callback(callback);
#endif   
  
  
}

bool bc_touch_swipe_right_register_callback_register_callback(void *callback)
{
  
#if(TOUCH_DEVIECE_TYPE == 0)  

#elif(TOUCH_DEVIECE_TYPE == 1)
  return swipe_right_register_callback(callback);
#endif   
	
}

bool bc_touch_swipe_up_register_callback_register_callback(void *callback)
{		
	
#if(TOUCH_DEVIECE_TYPE == 0)  

#elif(TOUCH_DEVIECE_TYPE == 1)
  return swipe_up_register_callback(callback);
#endif   
  
  
}

bool bc_touch_swipe_down_register_callback_register_callback(void *callback)
{
  
#if(TOUCH_DEVIECE_TYPE == 0)  

#elif(TOUCH_DEVIECE_TYPE == 1)
  return swipe_down_register_callback(callback);
#endif   
	
}

bool bc_touch_alp_ati_error_register_callback(void *callback)
{
  
#if(TOUCH_DEVIECE_TYPE == 0)  

#elif(TOUCH_DEVIECE_TYPE == 1)
  return alp_ati_error_register_callback(callback);
#endif   
	
}







