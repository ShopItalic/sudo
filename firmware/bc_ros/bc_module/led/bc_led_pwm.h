#ifndef __BC_LED_PWM_H__
#define __BC_LED_PWM_H__




#include <stdint.h>

enum LED_WHITE_MODE
{
  LED_WHITE_BREATHE_2S = 0,
  LED_WHITE_BREATHE_4S,
  LED_WHITE_BREATHE_8S,
  LED_WHITE_LONG_LIGHT_3S,
  LED_WHITE_FlLASH_3S,
  LED_WHITE_FlLASH_CYCLE_300ms_500ms,
  LED_WHITE_FlLASH_CYCLE_500ms_500ms,
  LED_WHITE_FlLASH_CYCLE_300ms_4700ms,
  LED_WHITE_LONG_LIGHT,
};


void bc_led_pwm_out(void *linear_motor_config);

void bc_led_strong_vibration_start(void);

void bc_led_continuous_vibration_start(void);

void bc_led_white_breathe_start(enum LED_WHITE_MODE mode);

void bc_led_stop(void);

void bc_led_pwm_idie_register_callback(void * register_callback);

void bc_led_strong_vibration_pwm_config(uint16_t pwm_seq_values,uint8_t playback_count,uint16_t repeats);

void bc_led_continuous_vibration_pwm_config(uint16_t pwm_seq_values,uint8_t playback_count,uint16_t repeats);

void bc_led_pwm_stop_register_callback(void * register_callback);

void bc_led_pwm_device_find(void);






#endif



