#ifndef __BC_LINEAR_MOTOR_H__
#define __BC_LINEAR_MOTOR_H__


#include <stdint.h>

#if defined(SUDO_VOICE_ONLY)
#include <stdbool.h>
#endif


enum LINEAR_MOTOR_MODE
{
  LINEAR_MOTOR_MIC_OFFLINER_RECORDING = 0,
  LINEAR_MOTOR_MIC_ONLINER_RECORDING_CAPTURE,
  LINEAR_MOTOR_MIC_OFFLINER_RECORDING_CAPTURE,
  LINEAR_MOTOR_MIC_START,
  LINEAR_MOTOR_MIC_STOP,
};

void bc_linear_motor_start(enum LINEAR_MOTOR_MODE mode);

#if defined(SUDO_VOICE_ONLY)
/* Finite 1 MHz PWM pulse: strength 1..100, duration 20..400 ms in 20 ms steps. */
bool bc_linear_motor_pulse(uint8_t strength_percent, uint16_t active_ms);
/* Enable or suppress all normal Sudo haptic feedback. */
void bc_linear_motor_feedback_enable(bool enabled);
#endif

void bc_linear_motor_pwm_out(void *linear_motor_config);

void bc_linear_motor_strong_vibration_start(void);

void bc_linear_motor_continuous_vibration_start(void);

void bc_linear_motor_stop(void);

void bc_linear_motor_device_find(void);

void bc_linear_motor_pwm_idie_register_callback(void * register_callback);

void bc_linear_motor_strong_vibration_pwm_config(uint16_t pwm_seq_values,uint8_t playback_count,uint16_t repeats);

void bc_linear_motor_continuous_vibration_pwm_config(uint16_t pwm_seq_values,uint8_t playback_count,uint16_t repeats);


#endif



