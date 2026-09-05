#include "bc_linear_motor.h"


#include "q_device.h"
#include "bc_logger.h"
#include "bc_ldo_switch.h"

#include "string.h"
#include "bc_delay.h"


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

void bc_linear_motor_start(enum LINEAR_MOTOR_MODE mode)
{
  bc_ldo_motor_power_on();
    bc_delay_ms(20);
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
	if(pwm_falsh.pwm_status != LINEAR_MOTOR_PWM_IDIE)
	{
		q_device_ctrl(linear_motor_pwm_dev, PWM_CTRL_STOP, NULL);
	}
	
	struct pwm_config *cfg = (struct pwm_config *)linear_motor_config;
	memcpy((uint8_t *)linear_motor_pwm_seq_values,(uint8_t*)cfg->pwm_parameter_config.p_common,cfg->pwm_parameter_config.length*2);
	linear_motor_pwm_config = *(struct pwm_config *)linear_motor_config;
	linear_motor_pwm_config.pwm_parameter_config.p_common = linear_motor_pwm_seq_values;
	linear_motor_pwm_start(&linear_motor_pwm_config);
}

void bc_linear_motor_strong_vibration_start(void)
{
	bc_ldo_motor_power_on();
	linear_motor_pwm_start(&strong_vibration_pwm_config);
}

void bc_linear_motor_continuous_vibration_start(void)
{
	bc_ldo_motor_power_on();
	linear_motor_pwm_start(&continuous_vibration_pwm_config);
}

void bc_linear_motor_stop(void)
{
	bc_ldo_motor_power_off();
	q_device_close(linear_motor_pwm_dev);
	pwm_falsh.pwm_status = LINEAR_MOTOR_PWM_IDIE;
}

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
	q_device_reg_callback(linear_motor_pwm_dev,PWM_REGISTER_STOPPED_CALLBACK,linear_motor_pwm_callback);
	
}



