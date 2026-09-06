#include "bc_linear_motor.h"


#include "q_device.h"
#include "bc_logger.h"
#include "bc_ldo_switch.h"

#include "string.h"
#include "bc_delay.h"

#if defined(SUDO_VOICE_ONLY)
#include "bc_rtos.h"
#endif


static q_device_t *linear_motor_pwm_dev = NULL;

typedef void (*bc_linear_motor_pwm_idie_callback)(void);
static bc_linear_motor_pwm_idie_callback linear_motor_pwm_idie_callback = NULL;

enum linear_motor_pwm_status
{
	LINEAR_MOTOR_PWM_IDIE = 0,
	LINEAR_MOTOR_PWM_BUSY,
	LINEAR_MOTOR_PWM_WORK,
};

enum linear_motor_pwm_mode
{
	PWM_STOP = 0,
  PWM_LOOP , 
};

struct linear_motor_pwm_falsh
{
	enum linear_motor_pwm_status  pwm_status;
	enum linear_motor_pwm_mode   pwm_mode;
};

static struct linear_motor_pwm_falsh pwm_falsh;

static uint16_t linear_motor_pwm_seq_values[200] = {0};
static  struct pwm_config  linear_motor_pwm_config = {0};

static uint16_t strong_vibration_pwm_seq_values[3] = {3200,3200,10000};
static  struct pwm_config  strong_vibration_pwm_config = {

	
	.pwm_aisle0_enable_status = true,
	.pwm_aisle1_enable_status = false,
	.pwm_aisle2_enable_status = false,
	.pwm_parameter_config.top_value = 10000,
	.pwm_parameter_config.length = sizeof(strong_vibration_pwm_seq_values)/sizeof(uint16_t),    //PWM序列中包含的周期个数
	.pwm_parameter_config.p_common =  strong_vibration_pwm_seq_values,                          //指向PWM序列
	.pwm_parameter_config.playback_count = 3,                                                  //序列执行次数
	.pwm_parameter_config.repeats = 13,                                                         //序列中周期重复次数
	.pwm_parameter_config.flags = PWM_FLAG_STOP,                                                //执行模式
};

static uint16_t continuous_vibration_pwm_seq_values[2] = {3200,10000};

static  struct pwm_config  continuous_vibration_pwm_config = {
	.pwm_aisle0_enable_status = true,
	.pwm_aisle1_enable_status = false,
	.pwm_aisle2_enable_status = false,
	.pwm_parameter_config.top_value = 10000,
	.pwm_parameter_config.length = sizeof(continuous_vibration_pwm_seq_values)/sizeof(uint16_t),    //PWM序列中包含的周期个数
	.pwm_parameter_config.p_common =  continuous_vibration_pwm_seq_values,                          //指向PWM序列
	.pwm_parameter_config.playback_count = 10,                                                  //序列执行次数
	.pwm_parameter_config.repeats = 50,                                                         //序列中周期重复次数
	.pwm_parameter_config.flags = PWM_FLAG_STOP,                                                //执行模式
};

#if defined(SUDO_VOICE_ONLY)
/* The worker applies the persisted policy; a fresh process starts enabled. */
static volatile bool sudo_feedback_enabled = true;
static volatile uint32_t sudo_feedback_generation;

/*
 * These are intentionally bounded, naturally aligned scalar publications.
 * Publication and snapshots use the task/ISR critical-section variants only
 * around these scalars; no PWM, GPIO, or I/O is held in that section.  The
 * ADC compares generation around its whole transaction, so a publication
 * racing a snapshot cannot validate a stale sample.
 */
static volatile uint32_t sudo_motor_activity_generation;
static volatile uint32_t sudo_motor_activity_active_since_tick;
static volatile uint32_t sudo_motor_activity_last_finished_tick;
static volatile uint32_t sudo_motor_activity_active;
static volatile uint32_t sudo_motor_activity_has_finished;

static void linear_motor_activity_begin(void)
{
	bc_rtos_taskENTER_CRITICAL();
  if(sudo_motor_activity_active == 0U)
  {
    sudo_motor_activity_active_since_tick =
      (uint32_t)bc_rtos_task_get_tick_count();
    sudo_motor_activity_active = 1U;
    ++sudo_motor_activity_generation;
  }
	bc_rtos_taskEXIT_CRITICAL();
}

static void linear_motor_activity_finish_publish(uint32_t finished_tick)
{
	/* Caller holds either the task or ISR publication critical section. */
  if(sudo_motor_activity_active != 0U)
  {
    sudo_motor_activity_last_finished_tick = finished_tick;
    sudo_motor_activity_has_finished = 1U;
    sudo_motor_activity_active = 0U;
    ++sudo_motor_activity_generation;
  }

}

static void linear_motor_activity_finished_task(void)
{
	bc_rtos_taskENTER_CRITICAL();
	linear_motor_activity_finish_publish(
		(uint32_t)bc_rtos_task_get_tick_count());
	bc_rtos_taskEXIT_CRITICAL();
}

static void linear_motor_activity_finished_from_isr(void)
{
	UBaseType_t saved_interrupt_mask = taskENTER_CRITICAL_FROM_ISR();
	linear_motor_activity_finish_publish(
		(uint32_t)xTaskGetTickCountFromISR());
	taskEXIT_CRITICAL_FROM_ISR(saved_interrupt_mask);
}

void bc_linear_motor_activity_begin(void)
{
  linear_motor_activity_begin();
}

void bc_linear_motor_activity_get(bc_linear_motor_activity_t *activity)
{
  if(activity == NULL)
    return;

	bc_rtos_taskENTER_CRITICAL();
  activity->generation = sudo_motor_activity_generation;
  activity->active_since_tick = sudo_motor_activity_active_since_tick;
  activity->last_finished_tick = sudo_motor_activity_last_finished_tick;
  activity->active = sudo_motor_activity_active != 0U;
  activity->has_finished = sudo_motor_activity_has_finished != 0U;
	bc_rtos_taskEXIT_CRITICAL();
}
#endif



#if defined(SUDO_VOICE_ONLY)
static bool linear_motor_pwm_stop_existing(void)
{
	int stop_result = RESULT_Q_DEVICE_OK;
	int close_result = RESULT_Q_DEVICE_OK;
	bc_linear_motor_activity_t activity;

	bc_linear_motor_activity_get(&activity);
	if(pwm_falsh.pwm_status == LINEAR_MOTOR_PWM_IDIE && !activity.active)
	{
		return true;
	}

	/*
	 * Invalidate the old transfer before asking the driver to stop it. The
	 * Nordic PWM uninit path disables its IRQ, so a late STOPPED event from
	 * that transfer cannot tear down a later pulse.
	 */
	pwm_falsh.pwm_status = LINEAR_MOTOR_PWM_IDIE;
	pwm_falsh.pwm_mode = PWM_STOP;
	stop_result = q_device_ctrl(linear_motor_pwm_dev, PWM_CTRL_STOP, NULL);
	close_result = q_device_close(linear_motor_pwm_dev);
	bc_ldo_motor_power_off();
	if(stop_result == RESULT_Q_DEVICE_OK &&
		close_result == RESULT_Q_DEVICE_OK)
	{
		linear_motor_activity_finished_task();
	}

	return stop_result == RESULT_Q_DEVICE_OK &&
		close_result == RESULT_Q_DEVICE_OK;
}

static bool linear_motor_pwm_cleanup(void)
{
	int stop_result = RESULT_Q_DEVICE_OK;
	int close_result = RESULT_Q_DEVICE_OK;
	bc_linear_motor_activity_t activity;
	bool had_activity;

	bc_linear_motor_activity_get(&activity);
	had_activity = activity.active;

	pwm_falsh.pwm_status = LINEAR_MOTOR_PWM_IDIE;
	pwm_falsh.pwm_mode = PWM_STOP;
	if(linear_motor_pwm_dev != NULL)
	{
		stop_result = q_device_ctrl(linear_motor_pwm_dev, PWM_CTRL_STOP, NULL);
		close_result = q_device_close(linear_motor_pwm_dev);
	}
	else if(had_activity)
	{
		/* A lost device cannot prove that the motor transfer stopped. */
		stop_result = RESULT_DEV_POINTER_NULL_ERROR;
	}
	bc_ldo_motor_power_off();
	if(stop_result == RESULT_Q_DEVICE_OK &&
		close_result == RESULT_Q_DEVICE_OK)
	{
		linear_motor_activity_finished_task();
	}
	return stop_result == RESULT_Q_DEVICE_OK &&
		close_result == RESULT_Q_DEVICE_OK;
}

static bool linear_motor_pwm_start(struct pwm_config *linear_motor_config)
{
	int result;
	uint32_t policy_generation = sudo_feedback_generation;

	if(!sudo_feedback_enabled)
	{
		/* A disable may race the caller's power-settle delay. Cleanup releases
		 * that lease and rejects the peripheral start. */
		linear_motor_pwm_cleanup();
		return false;
	}

	if(linear_motor_config == NULL || linear_motor_pwm_dev == NULL ||
		linear_motor_config->pwm_parameter_config.p_common == NULL ||
		linear_motor_config->pwm_parameter_config.length == 0 ||
		linear_motor_config->pwm_parameter_config.length >
			(sizeof(linear_motor_pwm_seq_values) / sizeof(linear_motor_pwm_seq_values[0])) ||
		linear_motor_config->pwm_parameter_config.playback_count == 0 ||
		(linear_motor_config->pwm_parameter_config.flags != PWM_FLAG_STOP &&
		 linear_motor_config->pwm_parameter_config.flags != PWM_FLAG_LOOP))
	{
		linear_motor_pwm_cleanup();
		return false;
	}

	if(pwm_falsh.pwm_status != LINEAR_MOTOR_PWM_IDIE &&
		!linear_motor_pwm_stop_existing())
	{
		return false;
	}

	result = q_device_close(linear_motor_pwm_dev);
	if(result != RESULT_Q_DEVICE_OK)
	{
		linear_motor_pwm_cleanup();
		return false;
	}
	result = q_device_cfg(linear_motor_pwm_dev, linear_motor_config, NULL);
	if(result != RESULT_Q_DEVICE_OK)
	{
		linear_motor_pwm_cleanup();
		return false;
	}
	result = q_device_open(linear_motor_pwm_dev);
	if(result != RESULT_Q_DEVICE_OK)
	{
		linear_motor_pwm_cleanup();
		return false;
	}

	if(!sudo_feedback_enabled || policy_generation != sudo_feedback_generation)
	{
		linear_motor_pwm_cleanup();
		return false;
	}

	/* Set STOP before starting so a prior LOOP cannot leave cleanup latched out. */
	pwm_falsh.pwm_mode = (linear_motor_config->pwm_parameter_config.flags == PWM_FLAG_LOOP)
		? PWM_LOOP : PWM_STOP;
	pwm_falsh.pwm_status = LINEAR_MOTOR_PWM_BUSY;
	result = q_device_ctrl(linear_motor_pwm_dev, PWM_CTRL_START, NULL);
	if(result != RESULT_Q_DEVICE_OK || !sudo_feedback_enabled ||
		policy_generation != sudo_feedback_generation)
	{
		linear_motor_pwm_cleanup();
		return false;
	}
	return true;
}
#else
static void linear_motor_pwm_start(struct pwm_config *linear_motor_config)
{
	if(pwm_falsh.pwm_status != LINEAR_MOTOR_PWM_IDIE)
	{
		q_device_ctrl(linear_motor_pwm_dev, PWM_CTRL_STOP, NULL);
	}
	
	q_device_close(linear_motor_pwm_dev);
	q_device_cfg(linear_motor_pwm_dev, linear_motor_config, NULL);
	q_device_open(linear_motor_pwm_dev);
	pwm_falsh.pwm_status = LINEAR_MOTOR_PWM_BUSY;
	if(linear_motor_config->pwm_parameter_config.flags == 0x01)
	{
		pwm_falsh.pwm_mode = PWM_STOP;
	}
	if(linear_motor_config->pwm_parameter_config.flags == 0x02)
	{
		pwm_falsh.pwm_mode = PWM_LOOP;
	}
	//BC_LOG_INFO("\r\n");
    q_device_ctrl(linear_motor_pwm_dev, PWM_CTRL_START, NULL);
}
#endif

#if defined(SUDO_VOICE_ONLY)
static void linear_motor_pwm_callback(void)
{
	int close_result;

	if(pwm_falsh.pwm_status == LINEAR_MOTOR_PWM_BUSY &&
		pwm_falsh.pwm_mode == PWM_STOP)
	{
		/* Mark idle first so a duplicate stopped event cannot power off twice. */
		pwm_falsh.pwm_status = LINEAR_MOTOR_PWM_IDIE;
		pwm_falsh.pwm_mode = PWM_STOP;
		close_result = q_device_close(linear_motor_pwm_dev);
		if(close_result != RESULT_Q_DEVICE_OK)
		{
			BC_LOG_ERROR("LINEAR MOTOR PWM close failed\r\n");
		}
		bc_ldo_motor_power_off();
		if(close_result == RESULT_Q_DEVICE_OK)
		{
			linear_motor_activity_finished_from_isr();
			if(linear_motor_pwm_idie_callback != NULL)
			{
				linear_motor_pwm_idie_callback();
			}
		}
	}
	BC_LOG_INFO("LINEAR MOTOR PWM IDIE\r\n");
}
#else
static void linear_motor_pwm_callback(void)
{
	if(pwm_falsh.pwm_mode == PWM_STOP)
	{
		q_device_close(linear_motor_pwm_dev);
		pwm_falsh.pwm_status = LINEAR_MOTOR_PWM_IDIE;
		bc_ldo_motor_power_off();
		if(linear_motor_pwm_idie_callback != NULL)
		{
			linear_motor_pwm_idie_callback();
		}
	}
	BC_LOG_INFO("LINEAR MOTOR PWM IDIE\r\n");
}
#endif
#if defined(SUDO_VOICE_ONLY)
bool bc_linear_motor_pulse(uint8_t strength_percent, uint16_t active_ms)
{
	struct pwm_config pulse_config = {0};
	uint32_t compare;
	uint32_t policy_generation = sudo_feedback_generation;

	if(!sudo_feedback_enabled || strength_percent < 1 || strength_percent > 100 ||
		active_ms < 20 || active_ms > 400 || (active_ms % 20) != 0 ||
		linear_motor_pwm_dev == NULL)
	{
		return false;
	}

	/* Stop and close the previous transfer before touching its DMA buffer or
	 * taking the motor power lease for the replacement pulse. */
	if(!linear_motor_pwm_stop_existing())
	{
		return false;
	}
	if(!sudo_feedback_enabled)
	{
		return false;
	}

	compare = 10000u - ((uint32_t)strength_percent * 68u);
	linear_motor_pwm_seq_values[0] = (uint16_t)compare;
	linear_motor_pwm_seq_values[1] = (uint16_t)compare;
	linear_motor_pwm_seq_values[2] = 10000;
	pulse_config.pwm_aisle0_enable_status = true;
	pulse_config.pwm_aisle1_enable_status = false;
	pulse_config.pwm_aisle2_enable_status = false;
	pulse_config.pwm_aisle3_enable_status = false;
	/* At 1 MHz with top 10000, the two active values schedule 20 ms
	 * per repeat group; the final top value is the existing quiet tail.
	 * Physical motor response for this schedule remains unmeasured. */
	pulse_config.pwm_parameter_config.top_value = 10000;
	pulse_config.pwm_parameter_config.p_common = linear_motor_pwm_seq_values;
	pulse_config.pwm_parameter_config.length = 3;
	pulse_config.pwm_parameter_config.repeats = (active_ms / 20) - 1;
	pulse_config.pwm_parameter_config.playback_count = 1;
	pulse_config.pwm_parameter_config.flags = PWM_FLAG_STOP;

	linear_motor_activity_begin();
	bc_ldo_motor_power_on();
	bc_delay_ms(20);
	if(policy_generation != sudo_feedback_generation)
	{
		linear_motor_pwm_cleanup();
		return false;
	}
	return linear_motor_pwm_start(&pulse_config);
}
#endif

void bc_linear_motor_start(enum LINEAR_MOTOR_MODE mode)
{
#if defined(SUDO_VOICE_ONLY)
	uint32_t policy_generation = sudo_feedback_generation;
	if(!sudo_feedback_enabled)
	{
		return;
	}
	if(!linear_motor_pwm_stop_existing())
	{
		return;
	}
	if(!sudo_feedback_enabled)
	{
		return;
	}
	linear_motor_activity_begin();
#endif
  bc_ldo_motor_power_on();
    bc_delay_ms(20);
#if defined(SUDO_VOICE_ONLY)
  if(!sudo_feedback_enabled || policy_generation != sudo_feedback_generation)
  {
    linear_motor_pwm_cleanup();
    return;
  }
#endif
  switch(mode)
  {
    case LINEAR_MOTOR_MIC_OFFLINER_RECORDING:
    {
      for(uint8_t i = 0; i<5 ;i++)
      {
        linear_motor_pwm_seq_values[i] = 3200;
      }
      linear_motor_pwm_config.pwm_aisle0_enable_status = true;
      linear_motor_pwm_config.pwm_aisle1_enable_status = false;
      linear_motor_pwm_config.pwm_aisle2_enable_status = false;
      linear_motor_pwm_config.pwm_aisle3_enable_status = false;
      linear_motor_pwm_config.pwm_parameter_config.length = 5;
      linear_motor_pwm_config.pwm_parameter_config.playback_count = 3;
      linear_motor_pwm_config.pwm_parameter_config.p_common = linear_motor_pwm_seq_values;
      linear_motor_pwm_config.pwm_parameter_config.top_value = 10000;
      linear_motor_pwm_config.pwm_parameter_config.repeats = 5;
      linear_motor_pwm_config.pwm_parameter_config.flags = PWM_FLAG_STOP; 
      linear_motor_pwm_start(&linear_motor_pwm_config);
      break;
    }
    case LINEAR_MOTOR_MIC_ONLINER_RECORDING_CAPTURE:
    case LINEAR_MOTOR_MIC_OFFLINER_RECORDING_CAPTURE:
    {
      for(uint8_t i = 0; i<5 ;i++)
      {
        linear_motor_pwm_seq_values[i] = 2600;
      }
      linear_motor_pwm_config.pwm_aisle0_enable_status = true;
      linear_motor_pwm_config.pwm_aisle1_enable_status = false;
      linear_motor_pwm_config.pwm_aisle2_enable_status = false;
      linear_motor_pwm_config.pwm_aisle3_enable_status = false;
      linear_motor_pwm_config.pwm_parameter_config.length = 5;
      linear_motor_pwm_config.pwm_parameter_config.playback_count = 1;
      linear_motor_pwm_config.pwm_parameter_config.p_common = linear_motor_pwm_seq_values;
      linear_motor_pwm_config.pwm_parameter_config.top_value = 10000;
      linear_motor_pwm_config.pwm_parameter_config.repeats = 20;
      linear_motor_pwm_config.pwm_parameter_config.flags = PWM_FLAG_STOP; 
      linear_motor_pwm_start(&linear_motor_pwm_config);
      break;
    }
    case LINEAR_MOTOR_MIC_START:
    {
      for(uint8_t i = 0; i<3 ;i++)
      {
        linear_motor_pwm_seq_values[i] = 3200;
      }
      linear_motor_pwm_seq_values[2] = 10000;
      linear_motor_pwm_config.pwm_aisle0_enable_status = true;
      linear_motor_pwm_config.pwm_aisle1_enable_status = false;
      linear_motor_pwm_config.pwm_aisle2_enable_status = false;
      linear_motor_pwm_config.pwm_aisle3_enable_status = false;
      linear_motor_pwm_config.pwm_parameter_config.length = 3;
      linear_motor_pwm_config.pwm_parameter_config.playback_count = 1;
      linear_motor_pwm_config.pwm_parameter_config.p_common = linear_motor_pwm_seq_values;
      linear_motor_pwm_config.pwm_parameter_config.top_value = 10000;
      linear_motor_pwm_config.pwm_parameter_config.repeats = 13;
      linear_motor_pwm_config.pwm_parameter_config.flags = PWM_FLAG_STOP; 
      linear_motor_pwm_start(&linear_motor_pwm_config);
      break;
    }
    case LINEAR_MOTOR_MIC_STOP:
    {
      for(uint8_t i = 0; i<3 ;i++)
      {
        linear_motor_pwm_seq_values[i] = 3200;
      }
      linear_motor_pwm_seq_values[2] = 10000;
      linear_motor_pwm_config.pwm_aisle0_enable_status = true;
      linear_motor_pwm_config.pwm_aisle1_enable_status = false;
      linear_motor_pwm_config.pwm_aisle2_enable_status = false;
      linear_motor_pwm_config.pwm_aisle3_enable_status = false;
      linear_motor_pwm_config.pwm_parameter_config.length = 3;
      linear_motor_pwm_config.pwm_parameter_config.playback_count = 2;
      linear_motor_pwm_config.pwm_parameter_config.p_common = linear_motor_pwm_seq_values;
      linear_motor_pwm_config.pwm_parameter_config.top_value = 10000;
      linear_motor_pwm_config.pwm_parameter_config.repeats = 13;
      linear_motor_pwm_config.pwm_parameter_config.flags = PWM_FLAG_STOP; 
      linear_motor_pwm_start(&linear_motor_pwm_config);
      break;
    }
  }
}

void bc_linear_motor_pwm_out(void *linear_motor_config)
{
#if defined(SUDO_VOICE_ONLY)
	if(!sudo_feedback_enabled)
	{
		linear_motor_pwm_cleanup();
		return;
	}
	if(pwm_falsh.pwm_status != LINEAR_MOTOR_PWM_IDIE &&
		!linear_motor_pwm_stop_existing())
	{
		return;
	}
#else
	if(pwm_falsh.pwm_status != LINEAR_MOTOR_PWM_IDIE)
	{
		q_device_ctrl(linear_motor_pwm_dev, PWM_CTRL_STOP, NULL);
	}
#endif
	
	struct pwm_config *cfg = (struct pwm_config *)linear_motor_config;
#if defined(SUDO_VOICE_ONLY)
	/* Covers callers that acquired the legacy LDO lease before pwm_out(). */
	linear_motor_activity_begin();
#endif
	memcpy((uint8_t *)linear_motor_pwm_seq_values,(uint8_t*)cfg->pwm_parameter_config.p_common,cfg->pwm_parameter_config.length*2);
	linear_motor_pwm_config = *(struct pwm_config *)linear_motor_config;
	linear_motor_pwm_config.pwm_parameter_config.p_common = linear_motor_pwm_seq_values;
	linear_motor_pwm_start(&linear_motor_pwm_config);
}

void bc_linear_motor_strong_vibration_start(void)
{
#if defined(SUDO_VOICE_ONLY)
	if(!sudo_feedback_enabled)
	{
		return;
	}
	if(!linear_motor_pwm_stop_existing())
	{
		return;
	}
	if(!sudo_feedback_enabled)
	{
		return;
	}
	linear_motor_activity_begin();
#endif
	bc_ldo_motor_power_on();
	linear_motor_pwm_start(&strong_vibration_pwm_config);
}

void bc_linear_motor_continuous_vibration_start(void)
{
#if defined(SUDO_VOICE_ONLY)
	if(!sudo_feedback_enabled)
	{
		return;
	}
	if(!linear_motor_pwm_stop_existing())
	{
		return;
	}
	if(!sudo_feedback_enabled)
	{
		return;
	}
	linear_motor_activity_begin();
#endif
	bc_ldo_motor_power_on();
	linear_motor_pwm_start(&continuous_vibration_pwm_config);
}

void bc_linear_motor_stop(void)
{
#if defined(SUDO_VOICE_ONLY)
	(void)linear_motor_pwm_stop_existing();
#else
	bc_ldo_motor_power_off();
	q_device_close(linear_motor_pwm_dev);
	pwm_falsh.pwm_status = LINEAR_MOTOR_PWM_IDIE;
#endif
}

#if defined(SUDO_VOICE_ONLY)
void bc_linear_motor_feedback_enable(bool enabled)
{
	/* Publish disabled before touching the peripheral so a racing start is
	 * rejected by linear_motor_pwm_start(). */
	sudo_feedback_enabled = enabled;
	if(!enabled)
	{
		++sudo_feedback_generation;
		/* Stop PWM, close the device and release motor power even mid-pulse. */
		linear_motor_pwm_cleanup();
	}
}
#endif

void bc_linear_motor_pwm_idie_register_callback(void * register_callback)
{
	linear_motor_pwm_idie_callback = (bc_linear_motor_pwm_idie_callback)register_callback;
}


void bc_linear_motor_strong_vibration_pwm_config(uint16_t pwm_seq_values,uint8_t playback_count,uint16_t repeats)
{
	strong_vibration_pwm_config.pwm_parameter_config.playback_count = playback_count;
	strong_vibration_pwm_config.pwm_parameter_config.repeats = repeats;
	strong_vibration_pwm_seq_values[0] = pwm_seq_values;
	strong_vibration_pwm_seq_values[1] = pwm_seq_values;
}

void bc_linear_motor_continuous_vibration_pwm_config(uint16_t pwm_seq_values,uint8_t playback_count,uint16_t repeats)
{
	continuous_vibration_pwm_config.pwm_parameter_config.playback_count = playback_count;
	continuous_vibration_pwm_config.pwm_parameter_config.repeats = repeats;
	continuous_vibration_pwm_seq_values[0] = pwm_seq_values;
}	

void bc_linear_motor_device_find(void)
{
	
	linear_motor_pwm_dev = q_device_find("pwm0");
	q_device_assert(linear_motor_pwm_dev);
#if defined(SUDO_VOICE_ONLY)
	if(linear_motor_pwm_dev == NULL ||
		q_device_reg_callback(linear_motor_pwm_dev,PWM_REGISTER_STOPPED_CALLBACK,linear_motor_pwm_callback) != RESULT_Q_DEVICE_OK)
	{
		linear_motor_pwm_dev = NULL;
	}
#else
	q_device_reg_callback(linear_motor_pwm_dev,PWM_REGISTER_STOPPED_CALLBACK,linear_motor_pwm_callback);
#endif
	
}



