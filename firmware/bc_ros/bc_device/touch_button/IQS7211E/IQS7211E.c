
#include "IQS7211E.h"
#include "bc_logger.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bc_delay.h"

#include "nrf_gpio.h"
#include "nrfx_gpiote.h"

#include "bc_touch_button_device_port.h"
#include "bc_strategy_value.h"
#include "q_device.h"

#define IQS7211E_LOG    1

#if IQS7211E_LOG 
#define log_info(format, ...)         			printf("\r\n[INFO;%s(%d)] " format, __MODULE__, __LINE__, ##__VA_ARGS__); 	
#else
#define log_info(format, ...)
#endif

#define STOP_TRUE  1

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

typedef void (*touch_button_single_tap_callback)(void);
typedef void (*touch_button_double_tap_callback)(void);
typedef void (*touch_button_triple_tap_callback)(void);
typedef void (*touch_button_swipe_left_callback)(void);
typedef void (*touch_button_swipe_right_callback)(void);
typedef void (*touch_button_swipe_up_callback)(void);
typedef void (*touch_button_swipe_down_callback)(void);
typedef void (*touch_button_alp_ati_error_callback)(void);

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

static touch_button_single_tap_callback  single_tap_callback = NULL;
static touch_button_double_tap_callback  double_tap_callback = NULL;
static touch_button_triple_tap_callback  triple_tap_callback = NULL;
static touch_button_swipe_left_callback  swipe_left_callback = NULL;
static touch_button_swipe_right_callback swipe_right_callback = NULL;
static touch_button_swipe_up_callback    swipe_up_callback = NULL;
static touch_button_swipe_down_callback  swipe_down_callback = NULL;
static touch_button_alp_ati_error_callback alp_ati_error_callback = NULL;


static bool config_flag = false;
static uint16_t chip_id = 0;
static uint8_t chip_id_flag = 0;
static uint8_t chip_is_busy = false;

uint8_t get_chip_status(void)
{
    return chip_is_busy;
}

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

#if defined(SUDO_VOICE_ONLY)
bool IQS7211E_touch_report_register_callback(bc_touch_report_callback_t callback)
{
	return bc_touch_report_register_callback(callback);
}
#endif

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

bool single_tap_register_callback(void *callback)
{
	if(callback != NULL)
	{
		single_tap_callback = callback;
		return true;
	}
	return false;
}


bool double_tap_register_callback(void *callback)
{
	if(callback != NULL)
	{
		double_tap_callback = callback;
		return true;
	}
	return false;
}

bool triple_tap_register_callback(void *callback)
{
	if(callback != NULL)
	{
		triple_tap_callback = callback;
		return true;
	}
	return false;
}

bool swipe_left_register_callback(void *callback)
{
	if(callback != NULL)
	{
		swipe_left_callback = callback;
		return true;
	}
	return false;
}


bool swipe_right_register_callback(void *callback)
{
	if(callback != NULL)
	{
		swipe_right_callback = callback;
		return true;
	}
	return false;
}

bool swipe_up_register_callback(void *callback)
{
	if(callback != NULL)
	{
		swipe_up_callback = callback;
		return true;
	}
	return false;
}


bool swipe_down_register_callback(void *callback)
{
	if(callback != NULL)
	{
		swipe_down_callback = callback;
		return true;
	}
	return false;
}

bool alp_ati_error_register_callback(void *callback)
{
  if(callback != NULL)
  {
    alp_ati_error_callback = callback;
    return true;
  }
  return false;
}



bool IQS_I2C_Write_Data(uint8_t slver_addr,uint8_t reg_addr,uint8_t *buff,uint8_t buff_length,uint8_t flag)
{
	(void)slver_addr;
	(void)flag;
	return touch_i2c_write(reg_addr ,buff,buff_length);
}

bool IQS_I2C_Read_Data(uint8_t slver_addr,uint8_t reg_addr,uint8_t *buff,uint8_t buff_length,uint8_t flag)
{
	(void)slver_addr;
	(void)flag;
	return touch_i2c_read(reg_addr,buff,buff_length);
}

#if defined(SUDO_VOICE_ONLY)
static bool IQS7211E_touch_tuning_write(void *ctx, uint8_t reg,
                                         uint8_t *data, uint8_t length)
{
    (void)ctx;
    return IQS_I2C_Write_Data(IQS7211E_ADDR, reg, data, length, STOP_TRUE);
}

static bool IQS7211E_touch_tuning_read(void *ctx, uint8_t reg,
                                        uint8_t *data, uint8_t length)
{
    (void)ctx;
    return IQS_I2C_Read_Data(IQS7211E_ADDR, reg, data, length, STOP_TRUE);
}
#endif

#define UNINIT_TRACKPAD_TOUCH_SET_THRESHOLD             0xFF
#define UNINIT_TRACKPAD_TOUCH_CLEAR_THRESHOLD           0x14
#define UNINIT_ALP_THRESHOLD_0                          0xFF
#define UNINIT_ALP_THRESHOLD_1                          0xFF

void IQS7211E_Init(void)
{
    uint8_t buffer[40];

#if defined(SUDO_VOICE_ONLY)
    bc_touch_tuning_init((uint16_t)GESTURE_ENABLE_0 |
                         ((uint16_t)GESTURE_ENABLE_1 << 8));
#endif

    /* Change the ALP ATI Compensation */
/* Memory Map Position 0x1F - 0x20 */
    buffer[0] = ALP_COMPENSATION_A_0;
		buffer[1] = ALP_COMPENSATION_A_1;
		buffer[2] = ALP_COMPENSATION_B_0;
		buffer[3] = ALP_COMPENSATION_B_1;
	
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x1F,&buffer[0],4,STOP_TRUE);
		

    /* Change the ATI Settings */
/* Memory Map Position 0x21 - 0x27 */
    buffer[0] = TP_ATI_MULTIPLIERS_DIVIDERS_0;
    buffer[1] = TP_ATI_MULTIPLIERS_DIVIDERS_1;
    buffer[2] = TP_COMPENSATION_DIV;
    buffer[3] = TP_REF_DRIFT_LIMIT;
	  buffer[4] = TP_ATI_TARGET_0;
    buffer[5] = TP_ATI_TARGET_1;
    buffer[6] = TP_MIN_COUNT_REATI_0;
    buffer[7] = TP_MIN_COUNT_REATI_1;
    buffer[8] = ALP_ATI_MULTIPLIERS_DIVIDERS_0;
    buffer[9] = ALP_ATI_MULTIPLIERS_DIVIDERS_1;
    buffer[10] = ALP_COMPENSATION_DIV;
    buffer[11] = ALP_LTA_DRIFT_LIMIT;
    buffer[12] = ALP_ATI_TARGET_0;
    buffer[13] = ALP_ATI_TARGET_1;

    IQS_I2C_Write_Data(IQS7211E_ADDR,0x21,&buffer[0],14,STOP_TRUE);
		
		
    /* Change the Report Rates and Timing */
/* Memory Map Position 0x28 - 0x32 */
    buffer[0] = ACTIVE_MODE_REPORT_RATE_0;
    buffer[1] = ACTIVE_MODE_REPORT_RATE_1;
    buffer[2] = IDLE_TOUCH_MODE_REPORT_RATE_0;
    buffer[3] = IDLE_TOUCH_MODE_REPORT_RATE_1;
    buffer[4] = IDLE_MODE_REPORT_RATE_0;
    buffer[5] = IDLE_MODE_REPORT_RATE_1;
    buffer[6] = LP1_MODE_REPORT_RATE_0;
    buffer[7] = LP1_MODE_REPORT_RATE_1;
    buffer[8] = LP2_MODE_REPORT_RATE_0;
    buffer[9] = LP2_MODE_REPORT_RATE_1;
    buffer[10] = ACTIVE_MODE_TIMEOUT_0;
    buffer[11] = ACTIVE_MODE_TIMEOUT_1;
    buffer[12] = IDLE_TOUCH_MODE_TIMEOUT_0;
    buffer[13] = IDLE_TOUCH_MODE_TIMEOUT_1;
    buffer[14] = IDLE_MODE_TIMEOUT_0;
    buffer[15] = IDLE_MODE_TIMEOUT_1;
    buffer[16] = LP1_MODE_TIMEOUT_0;
    buffer[17] = LP1_MODE_TIMEOUT_1;
    buffer[18] = REATI_RETRY_TIME;
    buffer[19] = REF_UPDATE_TIME;
    buffer[20] = I2C_TIMEOUT_0;
    buffer[21] = I2C_TIMEOUT_1;
    
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x28,&buffer[0],22,STOP_TRUE);
	

		/* Change the ALP Settings */
		/* Memory Map Position 0x36 - 0x40 */
		buffer[0] = ALP_SETUP_0;
		buffer[1] = ALP_SETUP_1;
		buffer[2] = ALP_TX_ENABLE_0;
		buffer[3] = ALP_TX_ENABLE_1;
		buffer[4] = TRACKPAD_TOUCH_SET_THRESHOLD;
		buffer[5] = TRACKPAD_TOUCH_CLEAR_THRESHOLD;
		buffer[6] = ALP_THRESHOLD_0;
		buffer[7] = ALP_THRESHOLD_1;
		buffer[8] = ALP_SET_DEBOUNCE;
		buffer[9] = ALP_CLEAR_DEBOUNCE;
		buffer[10] = ALP_COUNT_BETA_LP1;
		buffer[11] = ALP_LTA_BETA_LP1;
		buffer[12] = ALP_COUNT_BETA_LP2;
		buffer[13] = ALP_LTA_BETA_LP2;
		buffer[14] = TP_CONVERSION_FREQUENCY_UP_PASS_LENGTH;
		buffer[15] = TP_CONVERSION_FREQUENCY_FRACTION_VALUE;
		buffer[16] = ALP_CONVERSION_FREQUENCY_UP_PASS_LENGTH;
		buffer[17] = ALP_CONVERSION_FREQUENCY_FRACTION_VALUE;
		buffer[18] = TRACKPAD_HARDWARE_SETTINGS_0;
		buffer[19] = TRACKPAD_HARDWARE_SETTINGS_1;
		buffer[20] = ALP_HARDWARE_SETTINGS_0;
		buffer[21] = ALP_HARDWARE_SETTINGS_1;
		
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x36,&buffer[0],22,STOP_TRUE);	
		
		
		/* Change the Trackpad Settings */
/* Memory Map Position 0x41 - 0x4A */
		buffer[0] = TRACKPAD_SETTINGS_0_0;
		buffer[1] = TRACKPAD_SETTINGS_0_1;
		buffer[2] = TRACKPAD_SETTINGS_1_0;
		buffer[3] = TRACKPAD_SETTINGS_1_1;
		buffer[4] = X_RESOLUTION_0;
		buffer[5] = X_RESOLUTION_1;
		buffer[6] = Y_RESOLUTION_0;
		buffer[7] = Y_RESOLUTION_1;
		buffer[8] = XY_DYNAMIC_FILTER_BOTTOM_SPEED_0;
		buffer[9] = XY_DYNAMIC_FILTER_BOTTOM_SPEED_1;
		buffer[10] = XY_DYNAMIC_FILTER_TOP_SPEED_0;
		buffer[11] = XY_DYNAMIC_FILTER_TOP_SPEED_1;
		buffer[12] = XY_DYNAMIC_FILTER_BOTTOM_BETA;
		buffer[13] = XY_DYNAMIC_FILTER_STATIC_FILTER_BETA;
		buffer[14] = STATIONARY_TOUCH_MOV_THRESHOLD;
		buffer[15] = FINGER_SPLIT_FACTOR;
		buffer[16] = X_TRIM_VALUE;
		buffer[17] = Y_TRIM_VALUE;
		buffer[18] = MINOR_VERSION;
		buffer[19] = MAJOR_VERSION;
		
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x41,&buffer[0],20,STOP_TRUE);	


		/* Change the Gesture Settings */
/* Memory Map Position 0x4B - 0x55 */
		buffer[0] = GESTURE_ENABLE_0;
		buffer[1] = GESTURE_ENABLE_1;
		buffer[2] = TAP_TOUCH_TIME_0;
		buffer[3] = TAP_TOUCH_TIME_1;
		buffer[4] = TAP_WAIT_TIME_0;
		buffer[5] = TAP_WAIT_TIME_1;
		buffer[6] = TAP_DISTANCE_0;
		buffer[7] = TAP_DISTANCE_1;
		buffer[8] = HOLD_TIME_0;
		buffer[9] = HOLD_TIME_1;
		buffer[10] = SWIPE_TIME_0;
		buffer[11] = SWIPE_TIME_1;
		buffer[12] = SWIPE_X_DISTANCE_0;
		buffer[13] = SWIPE_X_DISTANCE_1;
		buffer[14] = SWIPE_Y_DISTANCE_0;
		buffer[15] = SWIPE_Y_DISTANCE_1;
		buffer[16] = SWIPE_X_CONS_DIST_0;
		buffer[17] = SWIPE_X_CONS_DIST_1;
		buffer[18] = SWIPE_Y_CONS_DIST_0;
		buffer[19] = SWIPE_Y_CONS_DIST_1;
		buffer[20] = SWIPE_ANGLE;
		buffer[21] = PALM_THRESHOLD;
		
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x4B,&buffer[0],22,STOP_TRUE);


		/* Change the RxTx Mapping */
/* Memory Map Position 0x56 - 0x5C */
		buffer[0] = RX_TX_MAP_0;
		buffer[1] = RX_TX_MAP_1;
		buffer[2] = RX_TX_MAP_2;
		buffer[3] = RX_TX_MAP_3;
		buffer[4] = RX_TX_MAP_4;
		buffer[5] = RX_TX_MAP_5;
		buffer[6] = RX_TX_MAP_6;
		buffer[7] = RX_TX_MAP_7;
		buffer[8] = RX_TX_MAP_8;
		buffer[9] = RX_TX_MAP_9;
		buffer[10] = RX_TX_MAP_10;
		buffer[11] = RX_TX_MAP_11;
		buffer[12] = RX_TX_MAP_12;
		buffer[13] = RX_TX_MAP_FILLER;
		
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x56,&buffer[0],14,STOP_TRUE);
		
		
		/* Change the Allocation of channels into cycles 0-9 */
/* Memory Map Position 0x5D - 0x6B */
		buffer[0] = PLACEHOLDER_0;
		buffer[1] = CH_1_CYCLE_0;
		buffer[2] = CH_2_CYCLE_0;
		buffer[3] = PLACEHOLDER_1;
		buffer[4] = CH_1_CYCLE_1;
		buffer[5] = CH_2_CYCLE_1;
		buffer[6] = PLACEHOLDER_2;
		buffer[7] = CH_1_CYCLE_2;
		buffer[8] = CH_2_CYCLE_2;
		buffer[9] = PLACEHOLDER_3;
		buffer[10] = CH_1_CYCLE_3;
		buffer[11] = CH_2_CYCLE_3;
		buffer[12] = PLACEHOLDER_4;
		buffer[13] = CH_1_CYCLE_4;
		buffer[14] = CH_2_CYCLE_4;
		buffer[15] = PLACEHOLDER_5;
		buffer[16] = CH_1_CYCLE_5;
		buffer[17] = CH_2_CYCLE_5;
		buffer[18] = PLACEHOLDER_6;
		buffer[19] = CH_1_CYCLE_6;
		buffer[20] = CH_2_CYCLE_6;
		buffer[21] = PLACEHOLDER_7;
		buffer[22] = CH_1_CYCLE_7;
		buffer[23] = CH_2_CYCLE_7;
		buffer[24] = PLACEHOLDER_8;
		buffer[25] = CH_1_CYCLE_8;
		buffer[26] = CH_2_CYCLE_8;
		buffer[27] = PLACEHOLDER_9;
		buffer[28] = CH_1_CYCLE_9;
		buffer[29] = CH_2_CYCLE_9;		
		
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x5D,&buffer[0],30,STOP_TRUE);
		
		
		/* Change the Allocation of channels into cycles 10-19 */
/* Memory Map Position 0x6C - 0x7C */
		buffer[0] = PLACEHOLDER_10;
		buffer[1] = CH_1_CYCLE_10;
		buffer[2] = CH_2_CYCLE_10;
		buffer[3] = PLACEHOLDER_11;
		buffer[4] = CH_1_CYCLE_11;
		buffer[5] = CH_2_CYCLE_11;
		buffer[6] = PLACEHOLDER_12;
		buffer[7] = CH_1_CYCLE_12;
		buffer[8] = CH_2_CYCLE_12;
		buffer[9] = PLACEHOLDER_13;
		buffer[10] = CH_1_CYCLE_13;
		buffer[11] = CH_2_CYCLE_13;
		buffer[12] = PLACEHOLDER_14;
		buffer[13] = CH_1_CYCLE_14;
		buffer[14] = CH_2_CYCLE_14;
		buffer[15] = PLACEHOLDER_15;
		buffer[16] = CH_1_CYCLE_15;
		buffer[17] = CH_2_CYCLE_15;
		buffer[18] = PLACEHOLDER_16;
		buffer[19] = CH_1_CYCLE_16;
		buffer[20] = CH_2_CYCLE_16;
		buffer[21] = PLACEHOLDER_17;
		buffer[22] = CH_1_CYCLE_17;
		buffer[23] = CH_2_CYCLE_17;
		buffer[24] = PLACEHOLDER_18;
		buffer[25] = CH_1_CYCLE_18;
		buffer[26] = CH_2_CYCLE_18;
		buffer[27] = PLACEHOLDER_19;
		buffer[28] = CH_1_CYCLE_19;
		buffer[29] = CH_2_CYCLE_19;
		buffer[30] = PLACEHOLDER_20;
		buffer[31] = CH_1_CYCLE_20;
		buffer[32] = CH_2_CYCLE_20;

		IQS_I2C_Write_Data(IQS7211E_ADDR,0x6C,&buffer[0],33,STOP_TRUE);
		
		/* Change the System Settings */
/* Memory Map Position 0x33 - 0x35 */
		buffer[0] = SYSTEM_CONTROL_0 | 0x80 | 0x20;  // Ack Reset, TP Re-ATI
		buffer[1] = SYSTEM_CONTROL_1;
		buffer[2] = CONFIG_SETTINGS0 | 0x40;  //bit6 : 1 Write one or two bytes (any data) to the address 0xFF followed by a STOP to end comms
		buffer[3] = CONFIG_SETTINGS1 | 0x01;  //0x01 : Enable Event Mode
		buffer[4] = OTHER_SETTINGS_0;
		buffer[5] = OTHER_SETTINGS_1;
		
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x33,&buffer[0],6,STOP_TRUE);
    
}

void IQS7211E_unInit(void)
{
    uint8_t buffer[40];


    /* Change the ALP ATI Compensation */
/* Memory Map Position 0x1F - 0x20 */
    buffer[0] = ALP_COMPENSATION_A_0;
		buffer[1] = ALP_COMPENSATION_A_1;
		buffer[2] = ALP_COMPENSATION_B_0;
		buffer[3] = ALP_COMPENSATION_B_1;
	
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x1F,&buffer[0],4,STOP_TRUE);
		

    /* Change the ATI Settings */
/* Memory Map Position 0x21 - 0x27 */
    buffer[0] = TP_ATI_MULTIPLIERS_DIVIDERS_0;
    buffer[1] = TP_ATI_MULTIPLIERS_DIVIDERS_1;
    buffer[2] = TP_COMPENSATION_DIV;
    buffer[3] = TP_REF_DRIFT_LIMIT;
	  buffer[4] = TP_ATI_TARGET_0;
    buffer[5] = TP_ATI_TARGET_1;
    buffer[6] = TP_MIN_COUNT_REATI_0;
    buffer[7] = TP_MIN_COUNT_REATI_1;
    buffer[8] = ALP_ATI_MULTIPLIERS_DIVIDERS_0;
    buffer[9] = ALP_ATI_MULTIPLIERS_DIVIDERS_1;
    buffer[10] = ALP_COMPENSATION_DIV;
    buffer[11] = ALP_LTA_DRIFT_LIMIT;
    buffer[12] = ALP_ATI_TARGET_0;
    buffer[13] = ALP_ATI_TARGET_1;

    IQS_I2C_Write_Data(IQS7211E_ADDR,0x21,&buffer[0],14,STOP_TRUE);
		
		
    /* Change the Report Rates and Timing */
/* Memory Map Position 0x28 - 0x32 */
    buffer[0] = ACTIVE_MODE_REPORT_RATE_0;
    buffer[1] = ACTIVE_MODE_REPORT_RATE_1;
    buffer[2] = IDLE_TOUCH_MODE_REPORT_RATE_0;
    buffer[3] = IDLE_TOUCH_MODE_REPORT_RATE_1;
    buffer[4] = IDLE_MODE_REPORT_RATE_0;
    buffer[5] = IDLE_MODE_REPORT_RATE_1;
    
//    buffer[6] = LP1_MODE_REPORT_RATE_0;
//    buffer[7] = LP1_MODE_REPORT_RATE_1;
//    buffer[8] = LP2_MODE_REPORT_RATE_0;
//    buffer[9] = LP2_MODE_REPORT_RATE_1;

    buffer[6] = 0xFF;
    buffer[7] = 0x05;
    buffer[8] = 0xFF;
    buffer[9] = 0x05;
    buffer[10] = ACTIVE_MODE_TIMEOUT_0;
    buffer[11] = ACTIVE_MODE_TIMEOUT_1;
    buffer[12] = IDLE_TOUCH_MODE_TIMEOUT_0;
    buffer[13] = IDLE_TOUCH_MODE_TIMEOUT_1;
    buffer[14] = IDLE_MODE_TIMEOUT_0;
    buffer[15] = IDLE_MODE_TIMEOUT_1;
    buffer[16] = LP1_MODE_TIMEOUT_0;
    buffer[17] = LP1_MODE_TIMEOUT_1;
    buffer[18] = REATI_RETRY_TIME;
    buffer[19] = REF_UPDATE_TIME;
    buffer[20] = I2C_TIMEOUT_0;
    buffer[21] = I2C_TIMEOUT_1;
    
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x28,&buffer[0],22,STOP_TRUE);
	

		/* Change the ALP Settings */
		/* Memory Map Position 0x36 - 0x40 */
		buffer[0] = ALP_SETUP_0;
		buffer[1] = ALP_SETUP_1;
		buffer[2] = ALP_TX_ENABLE_0;
		buffer[3] = ALP_TX_ENABLE_1;
		buffer[4] = UNINIT_TRACKPAD_TOUCH_SET_THRESHOLD;
		buffer[5] = UNINIT_TRACKPAD_TOUCH_CLEAR_THRESHOLD;
		buffer[6] = UNINIT_ALP_THRESHOLD_0;
		buffer[7] = UNINIT_ALP_THRESHOLD_1;
		buffer[8] = ALP_SET_DEBOUNCE;
		buffer[9] = ALP_CLEAR_DEBOUNCE;
		buffer[10] = ALP_COUNT_BETA_LP1;
		buffer[11] = ALP_LTA_BETA_LP1;
		buffer[12] = ALP_COUNT_BETA_LP2;
		buffer[13] = ALP_LTA_BETA_LP2;
		buffer[14] = TP_CONVERSION_FREQUENCY_UP_PASS_LENGTH;
		buffer[15] = TP_CONVERSION_FREQUENCY_FRACTION_VALUE;
		buffer[16] = ALP_CONVERSION_FREQUENCY_UP_PASS_LENGTH;
		buffer[17] = ALP_CONVERSION_FREQUENCY_FRACTION_VALUE;
		buffer[18] = TRACKPAD_HARDWARE_SETTINGS_0;
		buffer[19] = TRACKPAD_HARDWARE_SETTINGS_1;
		buffer[20] = ALP_HARDWARE_SETTINGS_0;
		buffer[21] = ALP_HARDWARE_SETTINGS_1;
		
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x36,&buffer[0],22,STOP_TRUE);	
		
		
		/* Change the Trackpad Settings */
/* Memory Map Position 0x41 - 0x4A */
		buffer[0] = TRACKPAD_SETTINGS_0_0;
		buffer[1] = TRACKPAD_SETTINGS_0_1;
		buffer[2] = TRACKPAD_SETTINGS_1_0;
		buffer[3] = TRACKPAD_SETTINGS_1_1;
		buffer[4] = X_RESOLUTION_0;
		buffer[5] = X_RESOLUTION_1;
		buffer[6] = Y_RESOLUTION_0;
		buffer[7] = Y_RESOLUTION_1;
		buffer[8] = XY_DYNAMIC_FILTER_BOTTOM_SPEED_0;
		buffer[9] = XY_DYNAMIC_FILTER_BOTTOM_SPEED_1;
		buffer[10] = XY_DYNAMIC_FILTER_TOP_SPEED_0;
		buffer[11] = XY_DYNAMIC_FILTER_TOP_SPEED_1;
		buffer[12] = XY_DYNAMIC_FILTER_BOTTOM_BETA;
		buffer[13] = XY_DYNAMIC_FILTER_STATIC_FILTER_BETA;
		buffer[14] = STATIONARY_TOUCH_MOV_THRESHOLD;
		buffer[15] = FINGER_SPLIT_FACTOR;
		buffer[16] = X_TRIM_VALUE;
		buffer[17] = Y_TRIM_VALUE;
		buffer[18] = MINOR_VERSION;
		buffer[19] = MAJOR_VERSION;
		
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x41,&buffer[0],20,STOP_TRUE);	


		/* Change the Gesture Settings */
/* Memory Map Position 0x4B - 0x55 */
		buffer[0] = GESTURE_ENABLE_0;
		buffer[1] = GESTURE_ENABLE_1;
		buffer[2] = TAP_TOUCH_TIME_0;
		buffer[3] = TAP_TOUCH_TIME_1;
		buffer[4] = TAP_WAIT_TIME_0;
		buffer[5] = TAP_WAIT_TIME_1;
		buffer[6] = TAP_DISTANCE_0;
		buffer[7] = TAP_DISTANCE_1;
		buffer[8] = HOLD_TIME_0;
		buffer[9] = HOLD_TIME_1;
		buffer[10] = SWIPE_TIME_0;
		buffer[11] = SWIPE_TIME_1;
		buffer[12] = SWIPE_X_DISTANCE_0;
		buffer[13] = SWIPE_X_DISTANCE_1;
		buffer[14] = SWIPE_Y_DISTANCE_0;
		buffer[15] = SWIPE_Y_DISTANCE_1;
		buffer[16] = SWIPE_X_CONS_DIST_0;
		buffer[17] = SWIPE_X_CONS_DIST_1;
		buffer[18] = SWIPE_Y_CONS_DIST_0;
		buffer[19] = SWIPE_Y_CONS_DIST_1;
		buffer[20] = SWIPE_ANGLE;
		buffer[21] = PALM_THRESHOLD;
		
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x4B,&buffer[0],22,STOP_TRUE);


		/* Change the RxTx Mapping */
/* Memory Map Position 0x56 - 0x5C */
		buffer[0] = RX_TX_MAP_0;
		buffer[1] = RX_TX_MAP_1;
		buffer[2] = RX_TX_MAP_2;
		buffer[3] = RX_TX_MAP_3;
		buffer[4] = RX_TX_MAP_4;
		buffer[5] = RX_TX_MAP_5;
		buffer[6] = RX_TX_MAP_6;
		buffer[7] = RX_TX_MAP_7;
		buffer[8] = RX_TX_MAP_8;
		buffer[9] = RX_TX_MAP_9;
		buffer[10] = RX_TX_MAP_10;
		buffer[11] = RX_TX_MAP_11;
		buffer[12] = RX_TX_MAP_12;
		buffer[13] = RX_TX_MAP_FILLER;
		
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x56,&buffer[0],14,STOP_TRUE);
		
		
		/* Change the Allocation of channels into cycles 0-9 */
/* Memory Map Position 0x5D - 0x6B */
		buffer[0] = PLACEHOLDER_0;
		buffer[1] = CH_1_CYCLE_0;
		buffer[2] = CH_2_CYCLE_0;
		buffer[3] = PLACEHOLDER_1;
		buffer[4] = CH_1_CYCLE_1;
		buffer[5] = CH_2_CYCLE_1;
		buffer[6] = PLACEHOLDER_2;
		buffer[7] = CH_1_CYCLE_2;
		buffer[8] = CH_2_CYCLE_2;
		buffer[9] = PLACEHOLDER_3;
		buffer[10] = CH_1_CYCLE_3;
		buffer[11] = CH_2_CYCLE_3;
		buffer[12] = PLACEHOLDER_4;
		buffer[13] = CH_1_CYCLE_4;
		buffer[14] = CH_2_CYCLE_4;
		buffer[15] = PLACEHOLDER_5;
		buffer[16] = CH_1_CYCLE_5;
		buffer[17] = CH_2_CYCLE_5;
		buffer[18] = PLACEHOLDER_6;
		buffer[19] = CH_1_CYCLE_6;
		buffer[20] = CH_2_CYCLE_6;
		buffer[21] = PLACEHOLDER_7;
		buffer[22] = CH_1_CYCLE_7;
		buffer[23] = CH_2_CYCLE_7;
		buffer[24] = PLACEHOLDER_8;
		buffer[25] = CH_1_CYCLE_8;
		buffer[26] = CH_2_CYCLE_8;
		buffer[27] = PLACEHOLDER_9;
		buffer[28] = CH_1_CYCLE_9;
		buffer[29] = CH_2_CYCLE_9;		
		
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x5D,&buffer[0],30,STOP_TRUE);
		
		
		/* Change the Allocation of channels into cycles 10-19 */
/* Memory Map Position 0x6C - 0x7C */
		buffer[0] = PLACEHOLDER_10;
		buffer[1] = CH_1_CYCLE_10;
		buffer[2] = CH_2_CYCLE_10;
		buffer[3] = PLACEHOLDER_11;
		buffer[4] = CH_1_CYCLE_11;
		buffer[5] = CH_2_CYCLE_11;
		buffer[6] = PLACEHOLDER_12;
		buffer[7] = CH_1_CYCLE_12;
		buffer[8] = CH_2_CYCLE_12;
		buffer[9] = PLACEHOLDER_13;
		buffer[10] = CH_1_CYCLE_13;
		buffer[11] = CH_2_CYCLE_13;
		buffer[12] = PLACEHOLDER_14;
		buffer[13] = CH_1_CYCLE_14;
		buffer[14] = CH_2_CYCLE_14;
		buffer[15] = PLACEHOLDER_15;
		buffer[16] = CH_1_CYCLE_15;
		buffer[17] = CH_2_CYCLE_15;
		buffer[18] = PLACEHOLDER_16;
		buffer[19] = CH_1_CYCLE_16;
		buffer[20] = CH_2_CYCLE_16;
		buffer[21] = PLACEHOLDER_17;
		buffer[22] = CH_1_CYCLE_17;
		buffer[23] = CH_2_CYCLE_17;
		buffer[24] = PLACEHOLDER_18;
		buffer[25] = CH_1_CYCLE_18;
		buffer[26] = CH_2_CYCLE_18;
		buffer[27] = PLACEHOLDER_19;
		buffer[28] = CH_1_CYCLE_19;
		buffer[29] = CH_2_CYCLE_19;
		buffer[30] = PLACEHOLDER_20;
		buffer[31] = CH_1_CYCLE_20;
		buffer[32] = CH_2_CYCLE_20;

		IQS_I2C_Write_Data(IQS7211E_ADDR,0x6C,&buffer[0],33,STOP_TRUE);		
		/* Change the System Settings */
/* Memory Map Position 0x33 - 0x35 */
		buffer[0] = SYSTEM_CONTROL_0 | 0x80 | 0x20 ;  // Ack Reset, TP Re-ATI
		buffer[1] = SYSTEM_CONTROL_1;
		buffer[2] = CONFIG_SETTINGS0  | 0x40;  //bit6 : 1 Write one or two bytes (any data) to the address 0xFF followed by a STOP to end comms
		buffer[3] =  0x01;  //0x01 : Enable Event Mode
		buffer[4] = OTHER_SETTINGS_0;
		buffer[5] = OTHER_SETTINGS_1;
		
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x33,&buffer[0],6,STOP_TRUE);
    
}

void iqs7211e_single_read_id(void)
{
    chip_id_flag = 1;
    //chip_id = 0;
}

uint8_t iqs7211e_reg_getid(void)
{
    //NRF_LOG_INFO("touch button chip id:%04x\r\n",chip_id);
    return (uint8_t)(chip_id & 0x00ff);
}

void IQS7211E_low_power_on(void)
{
	 uint8_t buffer[10];
	buffer[0] = SYSTEM_CONTROL_0 | 0x04 ; 
	buffer[1] = SYSTEM_CONTROL_1 ; 
	buffer[2] = CONFIG_SETTINGS0 | 0x80 ; 	
	buffer[3] = CONFIG_SETTINGS1 | 0x01 ; 	                
	IQS_I2C_Write_Data(IQS7211E_ADDR,0x33,&buffer[0],4,STOP_TRUE);
}

void IQS7211E_low_power_off(void)
{
	uint8_t buffer[10];
	buffer[0] = SYSTEM_CONTROL_0 ; 
	buffer[1] = SYSTEM_CONTROL_1 ; 
	buffer[2] = CONFIG_SETTINGS0 ; 	
	buffer[3] = CONFIG_SETTINGS1 | 0x01 ; 	                
	IQS_I2C_Write_Data(IQS7211E_ADDR,0x33,&buffer[0],4,STOP_TRUE);
}


/*
//stop信号结束通信功能关闭
*/
void IQS7211E_Stop_Bit_Disabled(void)
{
    uint8_t buffer[2];
	  //log_info("\n RDY IQS7211E_Stop_Bit_Disabled");
	  buffer[0] = CONFIG_SETTINGS0 | 0x40;  //bit6 : 1 Write one or two bytes (any data) to the address 0xFF followed by a STOP to end comms;
		IQS_I2C_Write_Data(IQS7211E_ADDR,0x34,&buffer[0],1,STOP_TRUE);
}


/*
//关闭通信窗口
*/
void IQS7211E_Stop_I2C_Comm_Window(void)
{
    uint8_t buffer[2];
	
	  //buffer[0] = CONFIG_SETTINGS0 & 0xBF;  //bit6 : 1 Write one or two bytes (any data) to the address 0xFF followed by a STOP to end comms;
		//IQS_I2C_Write_Data(IQS7211E_ADDR,0x34,&buffer[0],1,STOP_TRUE);
	   
		IQS_I2C_Write_Data(IQS7211E_ADDR,0xFF,&buffer[0],0,STOP_TRUE);
	
}


void IQS7211E_Force_I2C_Comm_Window(void)
{
    uint8_t buffer[2];
	
	  buffer[0] = 0x00; 
		IQS_I2C_Write_Data(IQS7211E_ADDR,0xFF,&buffer[0],1,STOP_TRUE);
}

uint8_t sigreadid(void)
{
    touch_i2c_open();
    IQS7211E_Stop_Bit_Disabled();
            
    IQS_I2C_Read_Data(IQS7211E_ADDR,0x00 ,(uint8_t*)&chip_id,2,STOP_TRUE);
    log_info("111chip_id:%x",chip_id); 
    touch_i2c_close();
    if(chip_id == 0x0458){
        return 1;
    }else
        return 0;
}

void IQS7211E_set_int_inter(uint32_t iner)
{
    uint8_t buffer[8];
    buffer[0] = iner;//0x80
    buffer[1] = ACTIVE_MODE_REPORT_RATE_1;//0x0
    IQS_I2C_Write_Data(IQS7211E_ADDR,0x28,&buffer[0],2,STOP_TRUE);
}


/*
//中断处理
//RDY变低，I2C读写数据
*/
void Process_IQS7211E_Events(void)
{
	  uint8_t Version_Details_buffer[20];
	  uint8_t System_Data_buffer[28];
    uint8_t System_Data_buffer2[42];
	  uint8_t Channel_Touch_Status[6];
	  uint8_t PCC_Values[84];
	  uint8_t buffer[2]={0};
	  uint16_t Product_Number = 0;
    uint16_t Major_Version = 0;
    uint16_t Minor_Version = 0;	
		
		uint8_t Finger_Amount = 0;
		uint16_t Finger_1_X_coordinate = 0;
    uint16_t Finger_1_Y_coordinate = 0;	
		uint16_t Finger_2_X_coordinate = 0;
    uint16_t Finger_2_Y_coordinate = 0;
		
		static uint8_t Palm_state = 0;
		static uint8_t Hold_state = 0;
	  
      bool status_read_ok = false;
#if defined(SUDO_VOICE_ONLY)
      bool report_consumer = false;
      bc_touch_report_t touch_report;
#endif
      uint8_t rdypin = touch_io_irq_status();
//      log_info("\n RDY occurred******************:%d\r\n",rdypin);
	  if(rdypin == 0)
		{
            disable_irq();
            chip_is_busy = 1;
            enable_irq();
            if(touch_i2c_open()) {
                BC_LOG_INFO("bc_touch_button_irq_process touch_i2c_open fail\r\n");
#if defined(SUDO_VOICE_ONLY)
                if(bc_touch_report_consumer_installed())
                {
                    (void)bc_touch_report_decode(0, 0, false, &touch_report);
                    bc_touch_report_notify(&touch_report);
                }
#endif
                disable_irq();
                chip_is_busy = 0;
                enable_irq();
                return;
            }
//            uint8_t pinst = nrf_gpio_pin_read(NRF_GPIO_PIN_MAP(0,17));
//            log_info("\n RDY occurred chip_id_flag:%d",pinst);
			  IQS7211E_Stop_Bit_Disabled();
            
            if(chip_id_flag){
                IQS_I2C_Read_Data(IQS7211E_ADDR,0x00 ,(uint8_t*)&chip_id,2,STOP_TRUE);
                if(chip_id == 0x0458){
                    chip_id_flag = 0;
                }
                log_info("111chip_id:%x",chip_id); 
            }
			
		    //IQS_I2C_Read_Data(IQS7211E_ADDR,0x00,&Version_Details_buffer[0],6,STOP_TRUE);	
//        IQS_I2C_Read_Data(IQS7211E_ADDR,0x0E,&System_Data_buffer[0],4,STOP_TRUE);			
			  //IQS_I2C_Read_Data(IQS7211E_ADDR,0x0A,&System_Data_buffer[0],28,STOP_TRUE);
//			  IQS_I2C_Read_Data(IQS7211E_ADDR,0x18,&Channel_Touch_Status[0],4,STOP_FAULT);
			  		
            memset(System_Data_buffer, 0, sizeof(System_Data_buffer));
            
              status_read_ok = IQS_I2C_Read_Data(IQS7211E_ADDR,0x0E,&System_Data_buffer[8],8,STOP_TRUE);//节省通讯数据量

#if defined(SUDO_VOICE_ONLY)
              report_consumer = bc_touch_report_consumer_installed();
              (void)bc_touch_report_decode(&System_Data_buffer[8],
                                          BC_TOUCH_REPORT_STATUS_LENGTH,
                                          status_read_ok,
                                          &touch_report);
              if(report_consumer)
              {
                  bc_touch_report_notify(&touch_report);
              }
#endif
              if(!status_read_ok)
              {
                  BC_LOG_ERROR("IQS7211E status read failed\r\n");
                  goto iqs7211e_event_cleanup;
              }
            
            
		    //0x0F Register Info Flags
			  if(System_Data_buffer[10]&0x80)
				{//Reset occurred
					  log_info("\n IQS7211E_Reset_occurred");
//				    IQS7211E_Init();
//					  log_info("\n IQS7211E_Init_finished");	
          if(!config_flag)
					{
						IQS7211E_Init();
						log_info("\n IQS7211E_Init_finished");
					}
					else
					{
						IQS7211E_unInit();
//                        IQS7211E_Init();
						 printf("\n IQS7211E_unInit_finished");
////						return;
					}					
//					  
                    IQS_I2C_Read_Data(IQS7211E_ADDR,0x00,Version_Details_buffer,6,STOP_TRUE);    
					Product_Number = ((uint16_t)Version_Details_buffer[1]<<8) + Version_Details_buffer[0];
					Major_Version = ((uint16_t)Version_Details_buffer[3]<<8) + Version_Details_buffer[2];
					Minor_Version = ((uint16_t)Version_Details_buffer[5]<<8) + Version_Details_buffer[4];
				    log_info("\n Product_Number =%d, Major_Version =%d, Minor_Version =%d",Product_Number,Major_Version,Minor_Version);
					
					  //IQS7211E_Stop_I2C_Comm_Window();
				}
				else
				{
#if 0					
					  //IQS_I2C_Read_Data(IQS7211E_ADDR,0xE3,&PCC_Values[0],84,STOP_TRUE);
            IQS_I2C_Read_Data_16Bit_Reg_Addr(IQS7211E_ADDR,0xE3,0x00,&PCC_Values[0],16,STOP_TRUE);
            IQS_I2C_Read_Data_16Bit_Reg_Addr(IQS7211E_ADDR,0xE3,0x08,&PCC_Values[16],16,STOP_TRUE);
            IQS_I2C_Read_Data_16Bit_Reg_Addr(IQS7211E_ADDR,0xE3,0x10,&PCC_Values[32],16,STOP_TRUE);	
            IQS_I2C_Read_Data_16Bit_Reg_Addr(IQS7211E_ADDR,0xE3,0x18,&PCC_Values[48],16,STOP_TRUE);
            IQS_I2C_Read_Data_16Bit_Reg_Addr(IQS7211E_ADDR,0xE3,0x20,&PCC_Values[64],16,STOP_TRUE);
            IQS_I2C_Read_Data_16Bit_Reg_Addr(IQS7211E_ADDR,0xE3,0x28,&PCC_Values[80],4,STOP_TRUE);
#endif            					

                    //IQS_I2C_Read_Data(IQS7211E_ADDR,0xE2,&System_Data_buffer2[0],42,STOP_TRUE);//节省通讯数据量
                    
                    //log_info("\n *****************IQS_I2C_Read_Data 0xE2 :%02X", System_Data_buffer[27]);
                    //BC_LOG_HEX("\n *****************IQS_I2C_Read_Data 0xE2\r\n", System_Data_buffer2, 42);
                    
					  if(System_Data_buffer[10]&0x20)
						{//ALP ATI Error
						    log_info("\n IQS7211E_ALP_ATI_Error");	  
//                              if(alp_ati_error_callback != NULL)
//                              {
//                                alp_ati_error_callback();
//                              }
						}
					
					  if(System_Data_buffer[10]&0x08)
						{//ATI Error
							  //buffer[0] = SYSTEM_CONTROL_0 | 0x20;  //TP Re-ATI
							  //IQS_I2C_Write_Data(IQS7211E_ADDR,0x33,&buffer[0],1,STOP_TRUE);
						    log_info("\n IQS7211E_ATI_Error");	  
						}
				    else if(System_Data_buffer[10]&0x10)
						{//ATI active
						    log_info("\n IQS7211E_ATI_active");	  
						}
						else
						{
                            uint16_t Finger_1_X_coordinate = ((uint16_t)System_Data_buffer[13]<<8) + System_Data_buffer[12];
                            uint16_t Finger_1_Y_coordinate = ((uint16_t)System_Data_buffer[15]<<8) + System_Data_buffer[14];
                            //log_info("\n Finger_1_X_coordinate Finger_1_Y_coordinate");	  
                            //services_print_log("%d, %d", Finger_1_X_coordinate, Finger_1_Y_coordinate);
                            //通过检测坐标是否有效，来补充长按的场景缺陷
            //                app_timer_stop(m_release_tmr);//
                            //log_info("X coor:%d,Y coor:%d", Finger_1_X_coordinate,Finger_1_Y_coordinate);
                            if(Finger_1_X_coordinate == 0xFFFF || Finger_1_Y_coordinate == 0xFFFF)//释放手指
                            {
            //                    services_print_log("stop m_push_tick_tmr");
                                //constant_touch_flag = false;
            //                    app_timer_stop(m_push_tick_tmr);//释放松手
            //                    app_timer_stop(m_push_tmr);//释放松手
            //                    touch_push_flag = 0;
                                IQS7211E_set_int_inter(0x10);
                            }
#if 0							
							  Finger_Amount = System_Data_buffer[11]&0x03;
					      Finger_1_X_coordinate = ((uint16_t)System_Data_buffer[13]<<8) + System_Data_buffer[12];                 
						    Finger_1_Y_coordinate = ((uint16_t)System_Data_buffer[15]<<8) + System_Data_buffer[14];
							  log_info("\n X_coordinate>> %d",Finger_1_X_coordinate);
							  log_info("\n Y_coordinate>> %d",Finger_1_Y_coordinate);
							
							  Finger_2_X_coordinate = ((uint16_t)System_Data_buffer[21]<<8) + System_Data_buffer[20];                 
						    Finger_2_Y_coordinate = ((uint16_t)System_Data_buffer[23]<<8) + System_Data_buffer[22];
							  log_info("\n X_coordinate>> %d",Finger_2_X_coordinate);
							  log_info("\n Y_coordinate>> %d",Finger_2_Y_coordinate);
#endif							
//								if(event_rawdata_callback != NULL)
//								{
//									event_rawdata_callback(System_Data_buffer,sizeof(System_Data_buffer));
//                                    //event_rawdata_callback(&System_Data_buffer[8],8);
//								}
#if 1							
							  //Single Tap
								if(System_Data_buffer[8]&0x01)
								{
								    log_info("\n Single Tap");	
									if(single_tap_callback != NULL)
									{
										single_tap_callback();
									}
								}
								
								//Double Tap
								if(System_Data_buffer[8]&0x02)
								{
								    log_info("\n Double Tap");
#if defined(SUDO_VOICE_ONLY)
                  if(!report_consumer && double_tap_callback != NULL)
#else
                  if(double_tap_callback != NULL)
#endif
									{
										double_tap_callback();
									}										
								}
								
								//Triple Tap
								if(System_Data_buffer[8]&0x04)
								{
								    log_info("\n Triple Tap");	
                                      if(triple_tap_callback != NULL)
                                      {
                                        triple_tap_callback();
                                      }
								}

								// Press and Hold / Palm Gesture
								if(System_Data_buffer[8]&0x08 || System_Data_buffer[8]&0x10)
								{
                                    log_info("\n Press and Hold********************************************\r\n");
                                    
                                    /*Manual control of modes are handled by host*/
                                    /*Svstem Control (0x33) Bit 2-0:Mode Select-001: Idle-Touch mode*/
                                    buffer[0] = (SYSTEM_CONTROL_0 & 0xF8) | 0x01; // 001: Idle-Touch mode
                                    IQS_I2C_Write_Data(IQS7211E_ADDR, 0x33, &buffer[0],1, STOP_TRUE) ;

                                    
									  buffer[0] = CONFIG_SETTINGS0 | 0x80 | 0x40; 
										IQS_I2C_Write_Data(IQS7211E_ADDR,0x34,&buffer[0],1,STOP_TRUE);
									
									  if(System_Data_buffer[8]&0x08)
										{
                                            static uint8_t filter_cnt = 0;
										  log_info("\n Press and Hold#########################################%d", filter_cnt);
                                            //if(filter_cnt%10==0)
                                            {
                                                log_info("filter_cnt is 10 0\r\n");
#if defined(SUDO_VOICE_ONLY)
                                                if(!report_consumer && gesture_event_hold_callback != NULL)
#else
                                                if(gesture_event_hold_callback != NULL)
#endif
                                                {
                                                    log_info("gesture_event_hold_callback is not null\r\n");
                                                    gesture_event_hold_callback();
                                                }
                                            }
                                            IQS7211E_set_int_inter(0xA0);
                                            filter_cnt++;
										}
								    
										if(System_Data_buffer[8]&0x10)
										{
										    log_info("\n Palm Gesture");
										}											
								}
								

								//Swipe X+
								if(System_Data_buffer[9]&0x01)
								{
								    log_info("\n Swipe X+");
                   if(swipe_left_callback != NULL)
									 {
										 swipe_left_callback();
									 }										 
								}
								
								//Swipe X-
								if(System_Data_buffer[9]&0x02)
								{
								    log_info("\n Swipe X-");	
									if(swipe_right_callback != NULL)
									{
										swipe_right_callback();
									}
								}
								
								//Swipe Y+
								if(System_Data_buffer[9]&0x04)
								{
								    log_info("\n Swipe Y+");	
//									if(gesture_event_flick_negative_callback != NULL)
//									{
//										gesture_event_flick_negative_callback();
//									}
                  if(swipe_down_callback != NULL)
									{
										swipe_down_callback();
									}
								}
								
								//Swipe Y-
								if(System_Data_buffer[9]&0x08)
								{
								    log_info("\n Swipe Y-");	
//									if(gesture_event_flick_positive_callback != NULL)
//									{
//										gesture_event_flick_positive_callback();
//									}
                  if(swipe_up_callback != NULL)
									{
										swipe_up_callback();
									}
								}
								
								//Swipe and Hold X+
								if(System_Data_buffer[9]&0x10)
								{
								    log_info("\n Swipe and Hold X+");	
//									if(gesture_event_hold_callback != NULL)
//									{
//										gesture_event_hold_callback();
//									}
								}						
								
								//Swipe and Hold X-
								if(System_Data_buffer[9]&0x20)
								{
								    log_info("\n Swipe and Hold X-");	
//									if(gesture_event_hold_callback != NULL)
//									{
//										gesture_event_hold_callback();
//									}
								}
								
								//Swipe and Hold Y+
								if(System_Data_buffer[9]&0x40)
								{
								    log_info("\n Swipe and Hold Y+");	
//									if(gesture_event_hold_callback != NULL)
//									{
//										gesture_event_hold_callback();
//									}
								}
								
								//Swipe and Hold Y-
								if(System_Data_buffer[9]&0x80)
								{
								    log_info("\n Swipe and Hold Y-");	
//									if(gesture_event_hold_callback != NULL)
//									{
//										gesture_event_hold_callback();
//									}
								}
#endif
						}
						
						
				}
				
iqs7211e_event_cleanup:
#if defined(SUDO_VOICE_ONLY)
        if(status_read_ok)
        {
            bc_touch_tuning_on_sample(touch_report.valid,
                                         touch_report.contact,
                                         touch_report.reset_flags != 0U,
                                         IQS7211E_touch_tuning_write,
                                         IQS7211E_touch_tuning_read,
                                         NULL);
        }
#endif
        IQS7211E_Stop_I2C_Comm_Window();
        //log_info("\n END RDY Comms");	
#if 0			
        log_info("\n 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X",PCC_Values[0],PCC_Values[1],PCC_Values[2],PCC_Values[3],PCC_Values[4],PCC_Values[5],PCC_Values[6],PCC_Values[7],PCC_Values[8],PCC_Values[9],PCC_Values[10],PCC_Values[11]);	
        log_info("\n 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X",PCC_Values[12],PCC_Values[13],PCC_Values[14],PCC_Values[15],PCC_Values[16],PCC_Values[17],PCC_Values[18],PCC_Values[19],PCC_Values[20],PCC_Values[21],PCC_Values[22],PCC_Values[23]);				
        log_info("\n 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X",PCC_Values[24],PCC_Values[25],PCC_Values[26],PCC_Values[27],PCC_Values[28],PCC_Values[29],PCC_Values[30],PCC_Values[31],PCC_Values[32],PCC_Values[33],PCC_Values[34],PCC_Values[35]);				
        log_info("\n 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X",PCC_Values[36],PCC_Values[37],PCC_Values[38],PCC_Values[39],PCC_Values[40],PCC_Values[41],PCC_Values[42],PCC_Values[43],PCC_Values[44],PCC_Values[45],PCC_Values[46],PCC_Values[47]);				
        log_info("\n 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X",PCC_Values[48],PCC_Values[49],PCC_Values[50],PCC_Values[51],PCC_Values[52],PCC_Values[53],PCC_Values[54],PCC_Values[55],PCC_Values[56],PCC_Values[57],PCC_Values[58],PCC_Values[59]);				
        log_info("\n 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X",PCC_Values[60],PCC_Values[61],PCC_Values[62],PCC_Values[63],PCC_Values[64],PCC_Values[65],PCC_Values[66],PCC_Values[67],PCC_Values[68],PCC_Values[69],PCC_Values[70],PCC_Values[71]);				
        log_info("\n 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X",PCC_Values[72],PCC_Values[73],PCC_Values[74],PCC_Values[75],PCC_Values[76],PCC_Values[77],PCC_Values[78],PCC_Values[79],PCC_Values[80],PCC_Values[81],PCC_Values[82],PCC_Values[83]);				
				//IQS7211E_Stop_I2C_Comm_Window();
				//log_info("\n END RDY Comms");
#endif					
            touch_i2c_close();
            disable_irq();
            chip_is_busy = 0;
            enable_irq();
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





