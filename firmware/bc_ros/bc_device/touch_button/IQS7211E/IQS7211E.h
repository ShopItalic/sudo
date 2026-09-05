#ifndef __IQS7211E_H__
#define __IQS7211E_H__

#include "stdbool.h"
#include <stdint.h>
#include <stdio.h>

#if defined(SUDO_VOICE_ONLY)
#include "bc_touch_report.h"
#include "bc_touch_tuning.h"
#endif

#if (defined(HANDWARE_1_23_1) )	


#if (defined(HANDWARE_1_23_2))   
     #include "IQS7211E_init_1232.h"
     //#include "IQS7211E_init_BCL603MHV1.23.3_260616.h"
#elif (defined(HANDWARE_1_23_3))
    #include "IQS7211E_init_1233.h"
    //#include "IQS7211E_init_BCL603MHV1.23.3_260604.h"
#elif (defined(HANDWARE_1_23_4))
    #include "IQS7211E_init_1234.h"
#else
    #include "IQS7211E_init_1231.h"
#endif

#else
#include "IQS7211E_init.h"
#endif

#define IQS7211E_ADDR               0x56


void IQS7211E_Init(void);
void IQS7211E_unInit(void);
void Process_IQS7211E_Events(void);

#if defined(SUDO_VOICE_ONLY)
bool IQS7211E_touch_report_register_callback(bc_touch_report_callback_t callback);
#endif
void IQS7211E_Force_I2C_Comm_Window(void);

void IQS7211E_low_power_on(void);

void IQS7211E_low_power_off(void);

void config_flag_set(bool flag);

bool config_flag_get(void);

bool gesture_event_flick_positive_register_callback(void *callback);

bool gesture_event_swipe_positive_register_callback(void *callback);

bool CH0_in_touch_register_callback(void *callback);

bool CH1_in_touch_register_callback(void *callback);

bool CH2_in_touch_register_callback(void *callback);

bool error_register_callback(void *callback);

bool gesture_event_hold_register_callback(void *callback);

bool gesture_event_flick_negative_register_callback(void *callback);

bool event_rawdata_callback_register_callback(void *callback);

bool check_callback_register_callback(void *callback);

bool single_tap_register_callback(void *callback);

bool double_tap_register_callback(void *callback);

bool triple_tap_register_callback(void *callback);
  
bool swipe_left_register_callback(void *callback);

bool swipe_right_register_callback(void *callback);

bool swipe_up_register_callback(void *callback);

bool swipe_down_register_callback(void *callback);

bool alp_ati_error_register_callback(void *callback);
uint8_t iqs7211e_reg_getid(void);
void iqs7211e_single_read_id(void);
uint8_t get_chip_status(void);

#endif
