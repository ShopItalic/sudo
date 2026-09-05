#ifndef __BC_TOUCH_BUTTON_H__
#define __BC_TOUCH_BUTTON_H__



#include <stdint.h>
#include <stdbool.h>

#if defined(SUDO_VOICE_ONLY)
#include "bc_touch_report.h"
#endif

uint8_t bc_touch_button_chip_id_get(void);
bool bc_touch_button_chip_id_hardware_check(void);


void bc_touch_button_init(void);
void bc_touch_button_uninit(void);

void bc_touch_button_irq_process(void);

#if defined(SUDO_VOICE_ONLY)
bool bc_touch_button_touch_report_register_callback(bc_touch_report_callback_t callback);
#endif

bool bc_touch_button_config_flag_get(void);

bool bc_touch_button_gesture_event_flick_positive_register_callback(void *callback);

bool bc_touch_button_gesture_event_swipe_positive_register_callback(void *callback);

bool bc_touch_button_CH0_in_touch_register_callback(void *callback);

bool bc_touch_button_CH1_in_touch_register_callback(void *callback);

bool bc_touch_button_CH2_in_touch_register_callback(void *callback);

bool bc_touch_button_error_register_callback(void *callback);

bool bc_touch_button_gesture_event_hold_register_callback(void *callback);

bool bc_touch_button_gesture_event_flick_negative_register_callback(void *callback);

bool bc_touch_event_rawdata_callback_register_callback(void *callback);

bool bc_touch_check_callback_register_callback(void *callback);

bool bc_touch_single_tap_register_callback_register_callback(void *callback);

bool bc_touch_double_tap_register_callback_register_callback(void *callback);

bool bc_touch_triple_tap_register_callback_register_callback(void *callback);

bool bc_touch_swipe_left_register_callback_register_callback(void *callback);

bool bc_touch_swipe_right_register_callback_register_callback(void *callback);

bool bc_touch_swipe_up_register_callback_register_callback(void *callback);

bool bc_touch_swipe_down_register_callback_register_callback(void *callback);

bool bc_touch_alp_ati_error_register_callback(void *callback);

#endif
