


#include "IQS323.h"

#if defined(HANDWARE_1_5_6)

#include "IQS323_init_156.h"

#else

#include "IQS323_init.h"
            
#endif


#include <stdint.h>
#include <stdio.h>
#include "bc_delay.h"

#include "bc_touch_button_device_port.h"
#include "bc_strategy_value.h"


typedef void (*touch_button_gesture_event_flick_positive_callback)(void);

typedef void (*touch_button_gesture_event_flick_negative_callback)(void);

typedef void (*touch_button_gesture_event_swipe_positive_callback)(void);

typedef void (*touch_button_gesture_event_hold_callback)(void);

typedef void (*touch_button_CH0_in_touch_callback)(void);

typedef void (*touch_button_CH1_in_touch_callback)(void);

typedef void (*touch_button_CH2_in_touch_callback)(void);

typedef void (*touch_button_error_callback)(void);

typedef void (*touch_button_event_rawdata_callback)(uint8_t *rawdata,uint8_t rawdata_length);

typedef void (*touch_button_check_callback)(uint16_t ch0_count,uint16_t ch1_count,uint16_t ch2_count);


static touch_button_gesture_event_flick_positive_callback  gesture_event_flick_positive_callback = NULL;

static touch_button_gesture_event_flick_negative_callback  gesture_event_flick_negative_callback = NULL;

static touch_button_gesture_event_hold_callback  gesture_event_hold_callback = NULL;

static touch_button_gesture_event_swipe_positive_callback  gesture_event_swipe_positive_callback = NULL;

static touch_button_CH0_in_touch_callback   CH0_in_touch_callback = NULL;
static touch_button_CH1_in_touch_callback   CH1_in_touch_callback = NULL;
static touch_button_CH2_in_touch_callback   CH2_in_touch_callback = NULL;
static touch_button_error_callback   error_callback = NULL;
static touch_button_event_rawdata_callback event_rawdata_callback = NULL;

static touch_button_check_callback  check_callback = NULL;

static bool config_flag = false;


bool gesture_event_flick_positive_register_callback(void *callback)
{
	if(callback != NULL)
	{
		gesture_event_flick_positive_callback = callback;
		return true;
	}
	return false;
}

bool gesture_event_flick_negative_register_callback(void *callback)
{
	if(callback != NULL)
	{
		gesture_event_flick_negative_callback = callback;
		return true;
	}
	return false;
}

bool gesture_event_swipe_positive_register_callback(void *callback)
{
	if(callback != NULL)
	{
		gesture_event_swipe_positive_callback = callback;
		return true;
	}
	return false;
}

bool gesture_event_hold_register_callback(void *callback)
{
	if(callback != NULL)
	{
		gesture_event_hold_callback = callback;
		return true;
	}
	return false;
}

bool CH0_in_touch_register_callback(void *callback)
{
	if(callback != NULL)
	{
		CH0_in_touch_callback = callback;
		return true;
	}
	return false;
}

bool CH1_in_touch_register_callback(void *callback)
{
	if(callback != NULL)
	{
		CH1_in_touch_callback = callback;
		return true;
	}
	return false;
}

bool CH2_in_touch_register_callback(void *callback)
{
	if(callback != NULL)
	{
		CH2_in_touch_callback = callback;
		return true;
	}
	return false;
}

bool error_register_callback(void *callback)
{
	if(callback != NULL)
	{
		error_callback = callback;
		return true;
	}
	return false;
}

bool event_rawdata_callback_register_callback(void *callback)
{
	if(callback != NULL)
	{
		event_rawdata_callback = callback;
		return true;
	}
	return false;
}

bool check_callback_register_callback(void *callback)
{
	if(callback != NULL)
	{
		check_callback = callback;
		return true;
	}
	return false;
}

/*
//芯片初始化
*/

void IQS323_Init(void)
{
    uint8_t buffer[32];

		/* Change the I2C settings */
/* Memory Map Position 0xE0 - 0xDF */
#if 1
		buffer[0] = I2C_SETUP|0x01;
		
		touch_i2c_write(0xE0,&buffer[0],1);
#endif			
	
    /* Change the Sensor 0 Settings */
    /* Memory Map Position 0x30 - 0x39 */
        buffer[0] = S0_SETUP;
		buffer[1] = S0_TX_SELECT;
		buffer[2] = S0_CONV_FREQ_FRAC;
		buffer[3] = S0_CONV_FREQ_PERIOD;
		buffer[4] = S0_PRX_CTRL_0;
		buffer[5] = S0_PRX_CTRL_1;
		buffer[6] = S0_TG_CTRL;
		buffer[7] = S0_RX_SELECT;
		buffer[8] = S0_CALCAP_INACTIVE_RX;
		buffer[9] = S0_PATTERN_SETUP;
		buffer[10] = S0_PATTERN_SELECT;
		buffer[11] = S0_BIAS_CURRENT;
		buffer[12] = S0_ATI_SETUP_0;
		buffer[13] = S0_ATI_SETUP_1;
		buffer[14] = S0_ATI_BASE_0;
		buffer[15] = S0_ATI_BASE_1;
		buffer[16] = S0_ATI_COARSE;
		buffer[17] = S0_ATI_FINE;
		buffer[18] = S0_COMPENSATION_0;
		buffer[19] = S0_COMPENSATION_1;

		touch_i2c_write(0x30,&buffer[0],20);
		

    /* Change the Sensor 1 Settings */
    /* Memory Map Position 0x40 - 0x49 */
		buffer[0] = S1_SETUP;
		buffer[1] = S1_TX_SELECT;
		buffer[2] = S1_CONV_FREQ_FRAC;
		buffer[3] = S1_CONV_FREQ_PERIOD;
		buffer[4] = S1_PRX_CTRL_0;
		buffer[5] = S1_PRX_CTRL_1;
		buffer[6] = S1_TG_CTRL;
		buffer[7] = S1_RX_SELECT;
		buffer[8] = S1_CALCAP_INACTIVE_RX ;
		buffer[9] = S1_PATTERN_SETUP;
		buffer[10] = S1_PATTERN_SELECT;
		buffer[11] = S1_BIAS_CURRENT;
		buffer[12] = S1_ATI_SETUP_0;
		buffer[13] = S1_ATI_SETUP_1;
		buffer[14] = S1_ATI_BASE_0;
		buffer[15] = S1_ATI_BASE_1;
		buffer[16] = S1_ATI_COARSE;
		buffer[17] = S1_ATI_FINE;
		buffer[18] = S1_COMPENSATION_0;
		buffer[19] = S1_COMPENSATION_1;

    touch_i2c_write(0x40,&buffer[0],20);
		
		
    /* Change the Sensor 2 Settings */
    /* Memory Map Position 0x50 - 0x59 */
    buffer[0] = S2_SETUP;
			buffer[1] = S2_TX_SELECT;
			buffer[2] = S2_CONV_FREQ_FRAC;
			buffer[3] = S2_CONV_FREQ_PERIOD;
			buffer[4] = S2_PRX_CTRL_0;
			buffer[5] = S2_PRX_CTRL_1;
			buffer[6] = S2_TG_CTRL;
			buffer[7] = S2_RX_SELECT;
			buffer[8] = S2_CALCAP_INACTIVE_RX ;
			buffer[9] = S2_PATTERN_SETUP;
			buffer[10] = S2_PATTERN_SELECT;
			buffer[11] = S2_BIAS_CURRENT;
			buffer[12] = S2_ATI_SETUP_0;
			buffer[13] = S2_ATI_SETUP_1;
			buffer[14] = S2_ATI_BASE_0;
			buffer[15] = S2_ATI_BASE_1;
			buffer[16] = S2_ATI_COARSE;
			buffer[17] = S2_ATI_FINE;
			buffer[18] = S2_COMPENSATION_0;
			buffer[19] = S2_COMPENSATION_1;

		touch_i2c_write(0x50,&buffer[0],20);
	
	
	/* Change the Channel 0 settings */
    /* Memory Map Position 0x60 - 0x63 */
    buffer[0] = CH0_REF_UI_SETUP;
		buffer[1] = CH0_FOLLOWER_MASK;
		buffer[2] = CH0_PROX_THRESHOLD;
		buffer[3] = CH0_PROX_DEBOUNCE;
//		
#if defined(HANDWARE_1_5_6)
    buffer[4] = CH0_TOUCH_THRESHOLD;

#else

		buffer[4] = (uint8_t)bc_get_business_strategy_value(BUSINESS_STRATEGY_CH0_TOUCH_THRESHOLD);
#endif
		buffer[5] = CH0_TOUCH_HYSTERESIS;
		buffer[6] = CH0_FOLLOWER_WEIGHT_0;
		buffer[7] = CH0_FOLLOWER_WEIGHT_1;

		touch_i2c_write(0x60,&buffer[0],8);		

		
	/* Change the Channel 1 settings */
    /* Memory Map Position 0x70 - 0x73 */
    buffer[0] = CH1_REF_UI_SETUP;
		buffer[1] = CH1_FOLLOWER_MASK;
		buffer[2] = CH1_PROX_THRESHOLD;
		buffer[3] = CH1_PROX_DEBOUNCE;
//		
#if defined(HANDWARE_1_5_6)
    buffer[4] = CH1_TOUCH_THRESHOLD;

#else

		buffer[4] = (uint8_t)bc_get_business_strategy_value(BUSINESS_STRATEGY_CH1_TOUCH_THRESHOLD);
#endif
		
		buffer[5] = CH1_TOUCH_HYSTERESIS;
		buffer[6] = CH1_FOLLOWER_WEIGHT_0;
		buffer[7] = CH1_FOLLOWER_WEIGHT_1;

		touch_i2c_write(0x70,&buffer[0],8);	


		/* Change the Channel 2 settings */
    /* Memory Map Position 0x80 - 0x83 */
    buffer[0] = CH2_REF_UI_SETUP;
		buffer[1] = CH2_FOLLOWER_MASK;
		buffer[2] = CH2_PROX_THRESHOLD;
		buffer[3] = CH2_PROX_DEBOUNCE;
//		
#if defined(HANDWARE_1_5_6)
    buffer[4] = CH2_TOUCH_THRESHOLD;

#else

		buffer[4] = (uint8_t)bc_get_business_strategy_value(BUSINESS_STRATEGY_CH2_TOUCH_THRESHOLD);
#endif
		
		buffer[5] = CH2_TOUCH_HYSTERESIS;
		buffer[6] = CH2_FOLLOWER_WEIGHT_0;
		buffer[7] = CH2_FOLLOWER_WEIGHT_1;

		touch_i2c_write(0x80,&buffer[0],8);		
		
		
		/* Change the Slider Configuration */
/* Memory Map Position 0x90 - 0x98 */
		buffer[0] = SLIDER_SETUP;
		buffer[1] = LOWER_CALIBRATION;
		buffer[2] = UPPER_CALIBRATION;
		buffer[3] = BOTTOM_SPEED;
		buffer[4] = TOP_SPEED_0;
		buffer[5] = TOP_SPEED_1;
		buffer[6] = SLIDER_RESOLUTION_0;
		buffer[7] = SLIDER_RESOLUTION_1;
		buffer[8] = ENABLE_MASK_0;
		buffer[9] = ENABLE_MASK_1;
		buffer[10] = ENABLE_STATUS_POINTER_0;
		buffer[11] = ENABLE_STATUS_POINTER_1;
		buffer[12] = DELTA_LINK0_0;
		buffer[13] = DELTA_LINK0_1;
		buffer[14] = DELTA_LINK1_0;
		buffer[15] = DELTA_LINK1_1;
		buffer[16] = DELTA_LINK2_0;
		buffer[17] = DELTA_LINK2_1;
		
		touch_i2c_write(0x90,&buffer[0],18);	


		/* Change the Gesture Setup */
/* Memory Map Position 0xA0 - 0xA6 */
		buffer[0] = GESTURE_SELECT;
		buffer[1] = RESERVED_BYTE;
		buffer[2] = MINIMUM_TIME_0;
		buffer[3] = MINIMUM_TIME_1;
		buffer[4] = MAXIMUM_TAP_TIME_0;
		buffer[5] = MAXIMUM_TAP_TIME_1;
		buffer[6] = MAXIMUM_SWIPE_TIME_0;
		buffer[7] = MAXIMUM_SWIPE_TIME_1;
		buffer[8] = MINIMUM_HOLD_TIME_0;
		buffer[9] = MINIMUM_HOLD_TIME_1;
		buffer[10] = MAXIMUM_TAP_DISTANCE_0;
		buffer[11] = MAXIMUM_TAP_DISTANCE_1;
		buffer[12] = MINIMUM_SWIPE_DISTANCE_0;
		buffer[13] = MINIMUM_SWIPE_DISTANCE_1;
		
		touch_i2c_write(0xA0,&buffer[0],14);	


		/* Change the Filter Betas */
/* Memory Map Position 0xB0 - 0xB4 */
		buffer[0] = NP_COUNTS_FILTER;
		buffer[1] = LP_COUNTS_FILTER;
		buffer[2] = NP_LTA_FILTER;
		buffer[3] = LP_LTA_FILTER;
		buffer[4] = NP_LTA_FAST_FILTER;
		buffer[5] = LP_LTA_FAST_FILTER;
		buffer[6] = NP_ACTIVATION_LTA_FILTER;
		buffer[7] = LP_ACTIVATION_LTA_FILTER;
		buffer[8] = FAST_FILTER_BAND_0;
		buffer[9] = FAST_FILTER_BAND_1;
		
		touch_i2c_write(0xB0,&buffer[0],10);	
		
		
		/* Change the Power mode & System Settings */
/* Memory Map Position 0xC0 - 0xC5 */
		buffer[0] = SYSTEM_CONTROL | 0x80 | 0x40 | 0x01;  //0x80:event mode,0x04:do ATI,0x01:ack reset
		buffer[1] = RESERVED_BYTE;
		buffer[2] = NP_REPORT_RATE_0;
		buffer[3] = NP_REPORT_RATE_1;
		buffer[4] = LP_REPORT_RATE_0;
		buffer[5] = LP_REPORT_RATE_1;
		buffer[6] = ULP_REPORT_RATE_0;
		buffer[7] = ULP_REPORT_RATE_1;
		buffer[8] = HALT_REPORT_RATE_0;
		buffer[9] = HALT_REPORT_RATE_1;
		buffer[10] = POWER_MODE_TIMEOUT_0;
		buffer[11] = POWER_MODE_TIMEOUT_1;
		
		touch_i2c_write(0xC0,&buffer[0],12);	


		/* Change the I2C Settings and Events Mask */
/* Memory Map Position 0xD0 - 0xD4 */
		buffer[0] = OUTA_MASK_0;
		buffer[1] = OUTA_MASK_1;
		buffer[2] = I2C_TIMEOUT_0;
		buffer[3] = I2C_TIMEOUT_1;
		buffer[4] = PROX_EVENT_TIMEOUT;
		buffer[5] = TOUCH_EVENT_TIMEOUT;
		buffer[6] = EVENTS_ENABLE;
		buffer[7] = ACTIVATION_THRESHOLD;
		buffer[8] = RELEASE_DELTA_PERCENTAGE;
		buffer[9] = DELTA_SNAP_SAMPLE_DELAY;
		
		touch_i2c_write(0xD0,&buffer[0],10);	
				

}

/*
//芯片初始化
*/

void IQS323_unint(void)
{
    uint8_t buffer[32];

		/* Change the I2C settings */
/* Memory Map Position 0xE0 - 0xDF */
#if 1
		buffer[0] = I2C_SETUP|0x01;
		
		touch_i2c_write(0xE0,&buffer[0],1);
#endif			
	
    /* Change the Sensor 0 Settings */
    /* Memory Map Position 0x30 - 0x39 */
        buffer[0] = 0;
		buffer[1] = S0_TX_SELECT;
		buffer[2] = S0_CONV_FREQ_FRAC;
		buffer[3] = S0_CONV_FREQ_PERIOD;
		buffer[4] = S0_PRX_CTRL_0;
		buffer[5] = S0_PRX_CTRL_1;
		buffer[6] = S0_TG_CTRL;
		buffer[7] = S0_RX_SELECT;
		buffer[8] = S0_CALCAP_INACTIVE_RX;
		buffer[9] = S0_PATTERN_SETUP;
		buffer[10] = S0_PATTERN_SELECT;
		buffer[11] = S0_BIAS_CURRENT;
		buffer[12] = S0_ATI_SETUP_0;
		buffer[13] = S0_ATI_SETUP_1;
		buffer[14] = S0_ATI_BASE_0;
		buffer[15] = S0_ATI_BASE_1;
		buffer[16] = S0_ATI_COARSE;
		buffer[17] = S0_ATI_FINE;
		buffer[18] = S0_COMPENSATION_0;
		buffer[19] = S0_COMPENSATION_1;

		touch_i2c_write(0x30,&buffer[0],20);
		

    /* Change the Sensor 1 Settings */
    /* Memory Map Position 0x40 - 0x49 */
	buffer[0] = 0;
			buffer[1] = S1_TX_SELECT;
			buffer[2] = S1_CONV_FREQ_FRAC;
			buffer[3] = S1_CONV_FREQ_PERIOD;
			buffer[4] = S1_PRX_CTRL_0;
			buffer[5] = S1_PRX_CTRL_1;
			buffer[6] = S1_TG_CTRL;
			buffer[7] = S1_RX_SELECT;
			buffer[8] = S1_CALCAP_INACTIVE_RX ;
			buffer[9] = S1_PATTERN_SETUP;
			buffer[10] = S1_PATTERN_SELECT;
			buffer[11] = S1_BIAS_CURRENT;
			buffer[12] = S1_ATI_SETUP_0;
			buffer[13] = S1_ATI_SETUP_1;
			buffer[14] = S1_ATI_BASE_0;
			buffer[15] = S1_ATI_BASE_1;
			buffer[16] = S1_ATI_COARSE;
			buffer[17] = S1_ATI_FINE;
			buffer[18] = S1_COMPENSATION_0;
			buffer[19] = S1_COMPENSATION_1;

    touch_i2c_write(0x40,&buffer[0],20);
		
		
    /* Change the Sensor 2 Settings */
    /* Memory Map Position 0x50 - 0x59 */
    buffer[0] = 0;
			buffer[1] = S2_TX_SELECT;
			buffer[2] = S2_CONV_FREQ_FRAC;
			buffer[3] = S2_CONV_FREQ_PERIOD;
			buffer[4] = S2_PRX_CTRL_0;
			buffer[5] = S2_PRX_CTRL_1;
			buffer[6] = S2_TG_CTRL;
			buffer[7] = S2_RX_SELECT;
			buffer[8] = S2_CALCAP_INACTIVE_RX ;
			buffer[9] = S2_PATTERN_SETUP;
			buffer[10] = S2_PATTERN_SELECT;
			buffer[11] = S2_BIAS_CURRENT;
			buffer[12] = S2_ATI_SETUP_0;
			buffer[13] = S2_ATI_SETUP_1;
			buffer[14] = S2_ATI_BASE_0;
			buffer[15] = S2_ATI_BASE_1;
			buffer[16] = S2_ATI_COARSE;
			buffer[17] = S2_ATI_FINE;
			buffer[18] = S2_COMPENSATION_0;
			buffer[19] = S2_COMPENSATION_1;

		touch_i2c_write(0x50,&buffer[0],20);
	
	
	/* Change the Channel 0 settings */
    /* Memory Map Position 0x60 - 0x63 */
    buffer[0] = CH0_REF_UI_SETUP;
		buffer[1] = CH0_FOLLOWER_MASK;
		buffer[2] = CH0_PROX_THRESHOLD;
		buffer[3] = CH0_PROX_DEBOUNCE;
//		
#if defined(HANDWARE_1_5_6)

    buffer[4] = CH0_TOUCH_THRESHOLD;
#else

		buffer[4] = (uint8_t)bc_get_business_strategy_value(BUSINESS_STRATEGY_CH0_TOUCH_THRESHOLD);
#endif
		
		buffer[5] = CH0_TOUCH_HYSTERESIS;
		buffer[6] = CH0_FOLLOWER_WEIGHT_0;
		buffer[7] = CH0_FOLLOWER_WEIGHT_1;

		touch_i2c_write(0x60,&buffer[0],8);		

		
	/* Change the Channel 1 settings */
    /* Memory Map Position 0x70 - 0x73 */
    buffer[0] = CH1_REF_UI_SETUP;
		buffer[1] = CH1_FOLLOWER_MASK;
		buffer[2] = CH1_PROX_THRESHOLD;
		buffer[3] = CH1_PROX_DEBOUNCE;
//		buffer[4] = CH1_TOUCH_THRESHOLD;
#if defined(HANDWARE_1_5_6)

    buffer[4] = CH1_TOUCH_THRESHOLD;
#else

		buffer[4] = (uint8_t)bc_get_business_strategy_value(BUSINESS_STRATEGY_CH1_TOUCH_THRESHOLD);
#endif
		
		buffer[5] = CH1_TOUCH_HYSTERESIS;
		buffer[6] = CH1_FOLLOWER_WEIGHT_0;
		buffer[7] = CH1_FOLLOWER_WEIGHT_1;

		touch_i2c_write(0x70,&buffer[0],8);	


		/* Change the Channel 2 settings */
    /* Memory Map Position 0x80 - 0x83 */
    buffer[0] = CH2_REF_UI_SETUP;
		buffer[1] = CH2_FOLLOWER_MASK;
		buffer[2] = CH2_PROX_THRESHOLD;
		buffer[3] = CH2_PROX_DEBOUNCE;
//		
#if defined(HANDWARE_1_5_6)
    buffer[4] = CH2_TOUCH_THRESHOLD;
#else

		buffer[4] = (uint8_t)bc_get_business_strategy_value(BUSINESS_STRATEGY_CH2_TOUCH_THRESHOLD);
#endif
		
		buffer[5] = CH2_TOUCH_HYSTERESIS;
		buffer[6] = CH2_FOLLOWER_WEIGHT_0;
		buffer[7] = CH2_FOLLOWER_WEIGHT_1;

		touch_i2c_write(0x80,&buffer[0],8);		
		
		
		/* Change the Slider Configuration */
/* Memory Map Position 0x90 - 0x98 */
		buffer[0] = SLIDER_SETUP;
		buffer[1] = LOWER_CALIBRATION;
		buffer[2] = UPPER_CALIBRATION;
		buffer[3] = BOTTOM_SPEED;
		buffer[4] = TOP_SPEED_0;
		buffer[5] = TOP_SPEED_1;
		buffer[6] = SLIDER_RESOLUTION_0;
		buffer[7] = SLIDER_RESOLUTION_1;
		buffer[8] = ENABLE_MASK_0;
		buffer[9] = ENABLE_MASK_1;
		buffer[10] = ENABLE_STATUS_POINTER_0;
		buffer[11] = ENABLE_STATUS_POINTER_1;
		buffer[12] = DELTA_LINK0_0;
		buffer[13] = DELTA_LINK0_1;
		buffer[14] = DELTA_LINK1_0;
		buffer[15] = DELTA_LINK1_1;
		buffer[16] = DELTA_LINK2_0;
		buffer[17] = DELTA_LINK2_1;
		
		touch_i2c_write(0x90,&buffer[0],18);	


		/* Change the Gesture Setup */
/* Memory Map Position 0xA0 - 0xA6 */
		buffer[0] = GESTURE_SELECT;
		buffer[1] = RESERVED_BYTE;
		buffer[2] = MINIMUM_TIME_0;
		buffer[3] = MINIMUM_TIME_1;
		buffer[4] = MAXIMUM_TAP_TIME_0;
		buffer[5] = MAXIMUM_TAP_TIME_1;
		buffer[6] = MAXIMUM_SWIPE_TIME_0;
		buffer[7] = MAXIMUM_SWIPE_TIME_1;
		buffer[8] = MINIMUM_HOLD_TIME_0;
		buffer[9] = MINIMUM_HOLD_TIME_1;
		buffer[10] = MAXIMUM_TAP_DISTANCE_0;
		buffer[11] = MAXIMUM_TAP_DISTANCE_1;
		buffer[12] = MINIMUM_SWIPE_DISTANCE_0;
		buffer[13] = MINIMUM_SWIPE_DISTANCE_1;
		
		touch_i2c_write(0xA0,&buffer[0],14);	


		/* Change the Filter Betas */
/* Memory Map Position 0xB0 - 0xB4 */
		buffer[0] = NP_COUNTS_FILTER;
		buffer[1] = LP_COUNTS_FILTER;
		buffer[2] = NP_LTA_FILTER;
		buffer[3] = LP_LTA_FILTER;
		buffer[4] = NP_LTA_FAST_FILTER;
		buffer[5] = LP_LTA_FAST_FILTER;
		buffer[6] = NP_ACTIVATION_LTA_FILTER;
		buffer[7] = LP_ACTIVATION_LTA_FILTER;
		buffer[8] = FAST_FILTER_BAND_0;
		buffer[9] = FAST_FILTER_BAND_1;
		
		touch_i2c_write(0xB0,&buffer[0],10);	
		
		
		/* Change the Power mode & System Settings */
/* Memory Map Position 0xC0 - 0xC5 */
		buffer[0] = 0x30 | 0x80 | 0x40 | 0x01;  //0x80:event mode,0x04:do ATI,0x01:ack reset
		buffer[1] = RESERVED_BYTE;
		buffer[2] = NP_REPORT_RATE_0;
		buffer[3] = NP_REPORT_RATE_1;
		buffer[4] = LP_REPORT_RATE_0;
		buffer[5] = LP_REPORT_RATE_1;
		buffer[6] = ULP_REPORT_RATE_0;
		buffer[7] = ULP_REPORT_RATE_1;
		buffer[8] = HALT_REPORT_RATE_0;
		buffer[9] = HALT_REPORT_RATE_1;
		buffer[10] = POWER_MODE_TIMEOUT_0;
		buffer[11] = POWER_MODE_TIMEOUT_1;
		
		touch_i2c_write(0xC0,&buffer[0],12);	


		/* Change the I2C Settings and Events Mask */
/* Memory Map Position 0xD0 - 0xD4 */
		buffer[0] = OUTA_MASK_0;
		buffer[1] = OUTA_MASK_1;
		buffer[2] = I2C_TIMEOUT_0;
		buffer[3] = I2C_TIMEOUT_1;
		buffer[4] = PROX_EVENT_TIMEOUT;
		buffer[5] = TOUCH_EVENT_TIMEOUT;
		buffer[6] = 00;
		buffer[7] = ACTIVATION_THRESHOLD;
		buffer[8] = RELEASE_DELTA_PERCENTAGE;
		buffer[9] = DELTA_SNAP_SAMPLE_DELAY;
		
		touch_i2c_write(0xD0,&buffer[0],10);	
				

}


/*
//stop信号结束通信功能关闭
*/
void IQS323_Stop_Bit_Disabled(void)
{
    uint8_t buffer[2];
	  
	  buffer[0] = I2C_SETUP|0x01;
	
	  touch_i2c_write(0xE0,&buffer[0],1);

}


/*
//关闭通信窗口
*/
void IQS323_Stop_I2C_Comm_Window(void)
{
    uint8_t buffer[2];
	
	  buffer[0] = 0x00;
	
	 touch_i2c_write(0xFF,&buffer[0],1);
	
}

/*
//处理RDY中断事件
*/
void Process_IQS323_Events(void)
{
	  static uint8_t PCC_flag = 0;
	  uint8_t i;
	  uint8_t buffer[2];
	  uint8_t PCC_buffer[16];
	
	  uint8_t Version_Details_buffer[20];
	  uint8_t System_Data_buffer[18];

	  uint16_t Product_Number = 0;
    uint16_t Major_Version = 0;
    uint16_t Minor_Version = 0;	
	
	  uint16_t Counts_CH0 = 0;
    uint16_t Counts_CH1 = 0;
    uint16_t Counts_CH2 = 0;	
	
	  uint16_t LTA_CH0 = 0;
    uint16_t LTA_CH1 = 0;
    uint16_t LTA_CH2 = 0;	
	  
	  if(touch_io_irq_status() == 0)
		{
			  IQS323_Stop_Bit_Disabled();
			
		    touch_i2c_read(0x00,&Version_Details_buffer[0],6);			
			touch_i2c_read(0x10,&System_Data_buffer[0],18);
			
//			 printf("\n Channel 0 Filtered Counts =%d, Channel 1 Filtered Counts =%d, Channel 2 Filtered Counts =%d",System_Data_buffer[3],System_Data_buffer[5],System_Data_buffer[7]);
		    
			  if(System_Data_buffer[0]&0x80)
				{//Reset occurred 复位初始化芯片
					  printf("\n IQS323_Reset_occurred");
					if(!config_flag)
					{
						IQS323_Init();
						 printf("\n IQS323_Init_finished");
					}
					else
					{
						IQS323_unint();
//						touch_io_irq_disnable();
//						touch_rdy_out_high();
						 printf("\n IQS323_unInit_finished");
////						return;
					}
					 										
						
					  Product_Number = ((uint16_t)Version_Details_buffer[1]<<8) + Version_Details_buffer[0];
					  Major_Version = ((uint16_t)Version_Details_buffer[3]<<8) + Version_Details_buffer[2];
					  Minor_Version = ((uint16_t)Version_Details_buffer[5]<<8) + Version_Details_buffer[4];
				      printf("\n Product_Number =%d, Major_Version =%d, Minor_Version =%d",Product_Number,Major_Version,Minor_Version);
				}
				else
				{

				    if(System_Data_buffer[0]&0x60)
						{
							  if(System_Data_buffer[0]&0x20)
							  {
								    //ATI Active
						        printf("\n IQS323_ATI_Active");
								}
							  	
								if(System_Data_buffer[0]&0x40)
							  {
								    //ATI Error
						        printf("\n IQS323_ATI_Error");
									  
								buffer[0] = SYSTEM_CONTROL | 0x80 | 0x04;  //0x80:event mode,0x04:do ATI
								touch_i2c_write(0xC0,&buffer[0],1);
								 if(error_callback != NULL)
								 {
									 error_callback();
								 }
								}
						}
						else
						{//处理event
#if 1								
							    Counts_CH0 = ((uint16_t)System_Data_buffer[7]<<8) + System_Data_buffer[6];
								LTA_CH0 = ((uint16_t)System_Data_buffer[9]<<8) + System_Data_buffer[8];
								
                                Counts_CH1 = ((uint16_t)System_Data_buffer[11]<<8) + System_Data_buffer[10];
								LTA_CH1 = ((uint16_t)System_Data_buffer[13]<<8) + System_Data_buffer[12];
								
								Counts_CH2 = ((uint16_t)System_Data_buffer[15]<<8) + System_Data_buffer[14];
								LTA_CH2 = ((uint16_t)System_Data_buffer[17]<<8) + System_Data_buffer[16];
								
//								printf("\n Counts_CH0 =%d, LTA_CH0 =%d",Counts_CH0,LTA_CH0);
//								printf("\n Counts_CH1 =%d, LTA_CH1 =%d",Counts_CH1,LTA_CH1);
//								printf("\n Counts_CH2 =%d, LTA_CH2 =%d",Counts_CH2,LTA_CH2);
//							bc_delay_ms(80);
//							printf("System_Data_buffer[2]: %02x \r\n",System_Data_buffer[2]);	
                            if(event_rawdata_callback != NULL)
							{
								event_rawdata_callback(System_Data_buffer,sizeof(System_Data_buffer));
							}							
						    if(System_Data_buffer[2]&0x40)
							{//Gesture Event

								if(System_Data_buffer[2]&0x40)
									{
										if(System_Data_buffer[2]&0x01)
											{//Tap
//												    printf("\n IQS323_Gesture_Event_Tap");
											}
											
											if(System_Data_buffer[2]&0x02)
											{//Swipe Positive
//												    printf("\n IQS323_Gesture_Event_Swipe_Positivellllllllllllll \r\n");
												if(gesture_event_swipe_positive_callback != NULL)
												{
													gesture_event_swipe_positive_callback();
													
												}
											}
											
											if(System_Data_buffer[2]&0x04)
											{//Swipe Negative
//												    printf("\n IQS323_Gesture_Event_Swipe_Negativellllllllllllllll \r\n");
											}
											
											if(System_Data_buffer[2]&0x08)
											{//Flick Positive
												    printf("\n IQS323_Gesture_Event_Flick_Positivellllllllllllllllll \r\n");
												if(gesture_event_flick_positive_callback != NULL)
												{
													gesture_event_flick_positive_callback();
												
												}
											}
											
											if(System_Data_buffer[2]&0x10)
											{//Flick Negative
												    printf("\n IQS323_Gesture_Event_Flick_Negativelllllllllllllllll \r\n");
												if(gesture_event_flick_negative_callback != NULL)
												{
													gesture_event_flick_negative_callback();
													
												}
											}
											
											if(System_Data_buffer[2]&0x20)
											{
												//Hold
//												    printf("\n IQS323_Gesture_Event_Holdlllllllllllllllllllllllllllll        \r\n");
												if(gesture_event_hold_callback != NULL)
												{
													gesture_event_hold_callback();
													
												}
											}
									}
							}
#endif		
								
#if 1						               
//								printf("System_Data_buffer[1]: %02x \r\n",System_Data_buffer[1]);	
								//Touch State
								if(System_Data_buffer[1]&0x02)
								{//CH0 in Touch
//								    printf("\n IQS323_CH0_in_Touch");
									if(CH0_in_touch_callback != NULL)
									{
										CH0_in_touch_callback();
									}
								}
								else
								{//CH0 Release
//								    printf("\n IQS323_CH0_Release");
								}	

								if(System_Data_buffer[1]&0x08)
								{//CH1 in Touch
//								    printf("\n IQS323_CH1_in_Touch");
									if(CH1_in_touch_callback != NULL)
									{
										CH1_in_touch_callback();
									}
								}
								else
								{//CH1 Release
//								    printf("\n IQS323_CH1_Release");
								}	

								if(System_Data_buffer[1]&0x20)
								{//CH2 in Touch
//								    printf("\n IQS323_CH2_in_Touch");
									if(CH2_in_touch_callback != NULL)
									{
										CH2_in_touch_callback();
									}
								}
								else
								{//CH2 Release
//								    printf("\n IQS323_CH2_Release");
								}
                                if(System_Data_buffer[1]&0x02 && System_Data_buffer[1]&0x08 && System_Data_buffer[1]&0x20)
								{
									uint16_t ch0_count = (*(uint16_t*)&System_Data_buffer[8]) - (*(uint16_t*)&System_Data_buffer[6]);
									uint16_t ch1_count = (*(uint16_t*)&System_Data_buffer[12]) - (*(uint16_t*)&System_Data_buffer[10]);
									uint16_t ch2_count = (*(uint16_t*)&System_Data_buffer[16]) - (*(uint16_t*)&System_Data_buffer[14]);
									
									if(check_callback != NULL)
									{
										check_callback(ch0_count,ch1_count,ch2_count);
									}
								}									

#endif					

            }
				}
			
			  IQS323_Stop_I2C_Comm_Window();	
//        printf("\n IQS323_Close_Comm_Window");				
				
		}
	  

}

void config_flag_set(bool flag)
{
	config_flag = flag;
}

bool config_flag_get(void)
{
	return config_flag;
}

__weak bool touch_i2c_write(uint8_t reg_add ,uint8_t *data,uint8_t length)
{
	UNUSED(reg_add);
	UNUSED(data);
	UNUSED(length);
}

__weak bool touch_i2c_read(uint8_t reg_add ,uint8_t *data,uint8_t length)
{
	UNUSED(reg_add);
	UNUSED(data);
	UNUSED(length);	
}

__weak uint8_t touch_io_irq_status(void)
{
	
}
