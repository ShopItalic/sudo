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

/*
 * The battery ADC uses this bounded publication to avoid sampling while the
 * motor supply and PWM are settling.  Begin is task-safe; the PWM callback
 * publishes completion with the ISR-safe path.  The ADC reads this snapshot
 * from task context and compares generation around its transaction.  The
 * snapshot function is task-context-only because it uses the task critical
 * section variant.
 */
typedef struct
{
  uint32_t generation;
  uint32_t active_since_tick;
  uint32_t last_finished_tick;
  bool active;
  bool has_finished;
} bc_linear_motor_activity_t;

/* Mark the complete legacy pre-LDO/settle path as motor activity. */
void bc_linear_motor_activity_begin(void);
void bc_linear_motor_activity_get(bc_linear_motor_activity_t *activity);
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



