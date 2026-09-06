#include "bc_led_pwm.h"

#include "q_device.h"
#include "bc_logger.h"
#include "bc_ldo_switch.h"

#include "string.h"


static q_device_t *led_pwm_dev = NULL;
static  struct pwm_config led_pwm_config = {0};

typedef void (*bc_led_pwm_idie_callback)(void);
static bc_led_pwm_idie_callback led_pwm_idie_callback = NULL;


typedef void (*bc_led_pwm_stop_callback)(void);
static bc_led_pwm_stop_callback led_pwm_stop_callback = NULL;

enum led_pwm_status
{
	LED_PWM_IDIE = 0,
	LED_PWM_BUSY,
	LED_PWM_WORK,
};

enum led_pwm_mode
{
	PWM_STOP = 0,
  PWM_LOOP , 
};

struct led_pwm_falsh
{
	enum led_pwm_status  pwm_status;
	enum led_pwm_mode   pwm_mode;
};

static struct led_pwm_falsh pwm_falsh;

static uint16_t led_pwm_seq_values[200] = {0};
static  struct pwm_config  diy_led_pwm_config = {0};

static uint16_t strong_vibration_pwm_seq_values[3] = {8500,8500,10000};
static  struct pwm_config  strong_vibration_pwm_config = {

	
	.pwm_aisle0_enable_status = true,
	.pwm_aisle1_enable_status = false,
	.pwm_aisle2_enable_status = false,
	.pwm_parameter_config.top_value = 10000,
	.pwm_parameter_config.length = sizeof(strong_vibration_pwm_seq_values)/sizeof(uint16_t),    //PWM序列中包含的周期个数
	.pwm_parameter_config.p_common =  strong_vibration_pwm_seq_values,                          //指向PWM序列
	.pwm_parameter_config.playback_count = 3,                                                  //序列执行次数
	.pwm_parameter_config.repeats = 30,                                                         //序列中周期重复次数
	.pwm_parameter_config.flags = PWM_FLAG_STOP,                                                //执行模式
};

static uint16_t continuous_vibration_pwm_seq_values[2] = {8500,10000};

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

static uint16_t led_white_breathe_pwm_seq_values[101] = {8500,8500,10000};
static  struct pwm_config  led_white_breathe_pwm_config = {

	
	.pwm_aisle0_enable_status = true,
	.pwm_aisle1_enable_status = false,
	.pwm_aisle2_enable_status = false,
	.pwm_parameter_config.top_value = 10000,
	.pwm_parameter_config.length = sizeof(led_white_breathe_pwm_seq_values)/sizeof(uint16_t),    //PWM序列中包含的周期个数
	.pwm_parameter_config.p_common =  led_white_breathe_pwm_seq_values,                          //指向PWM序列
	.pwm_parameter_config.playback_count = 1,                                                  //序列执行次数
	.pwm_parameter_config.repeats = 1,                                                         //序列中周期重复次数
	.pwm_parameter_config.flags = PWM_FLAG_LOOP,                                                //执行模式
};

static void led_pwm_start(struct pwm_config *led_config)
{
	if(pwm_falsh.pwm_status != LED_PWM_IDIE)
	{
		q_device_ctrl(led_pwm_dev, PWM_CTRL_STOP, NULL);
	}
	
	q_device_close(led_pwm_dev);
	q_device_cfg(led_pwm_dev, led_config, NULL);
	q_device_open(led_pwm_dev);
	pwm_falsh.pwm_status = LED_PWM_BUSY;
	if(led_config->pwm_parameter_config.flags == 0x01)
	{
		pwm_falsh.pwm_mode = PWM_STOP;
	}
	if(led_config->pwm_parameter_config.flags == 0x02)
	{
		pwm_falsh.pwm_mode = PWM_LOOP;
	}
	BC_LOG_INFO("\r\n");
    q_device_ctrl(led_pwm_dev, PWM_CTRL_START, NULL);
}


static void led_pwm_callback(void)
{
	if(pwm_falsh.pwm_mode == PWM_STOP)
	{
		q_device_close(led_pwm_dev);
		pwm_falsh.pwm_status = LED_PWM_IDIE;
		bc_ldo_motor_power_off();
		if(led_pwm_idie_callback != NULL)
		{
			led_pwm_idie_callback();
		}
	}
  if(led_pwm_stop_callback != NULL)
  {
    led_pwm_stop_callback();
  }
	BC_LOG_INFO("LINEAR MOTOR PWM IDIE\r\n");
	
}

void bc_led_pwm_out(void *led_config)
{
	if(pwm_falsh.pwm_status != LED_PWM_IDIE)
	{
		q_device_ctrl(led_pwm_dev, PWM_CTRL_STOP, NULL);
	}
	
	struct pwm_config *cfg = (struct pwm_config *)led_config;
	memcpy((uint8_t *)led_pwm_seq_values,(uint8_t*)cfg->pwm_parameter_config.p_common,cfg->pwm_parameter_config.length*2);
	diy_led_pwm_config = *(struct pwm_config *)led_config;
	diy_led_pwm_config.pwm_parameter_config.p_common = led_pwm_seq_values;
	led_pwm_start(&diy_led_pwm_config);
}

void bc_led_strong_vibration_start(void)
{
	bc_ldo_motor_power_on();
	led_pwm_start(&strong_vibration_pwm_config);
}

void bc_led_continuous_vibration_start(void)
{
	
	led_pwm_start(&continuous_vibration_pwm_config);
  
//  void bc_led_pwm_out(void *led_config);
}

void bc_led_white_breathe_start(enum LED_WHITE_MODE mode)
{
  for(uint8_t i = 0; i<50 ;i++)
  {
    led_white_breathe_pwm_seq_values[i] = 10000 - (i*20);
  }
  for(uint8_t i = 0; i<50 ;i++)
  {
    led_white_breathe_pwm_seq_values[50+i] = 9020 + (i*20);
  }
  led_white_breathe_pwm_seq_values[100] = 10000;
  led_white_breathe_pwm_config.pwm_parameter_config.p_common = led_white_breathe_pwm_seq_values;
  switch(mode)
  {
    case LED_WHITE_BREATHE_2S:
    {
      led_white_breathe_pwm_config.pwm_parameter_config.length = sizeof(led_white_breathe_pwm_seq_values)/sizeof(uint16_t),
      led_white_breathe_pwm_config.pwm_parameter_config.playback_count = 1,
      led_white_breathe_pwm_config.pwm_parameter_config.repeats = 1;
      led_white_breathe_pwm_config.pwm_parameter_config.flags = PWM_FLAG_STOP;  
      break;
    }
    case LED_WHITE_BREATHE_4S:
    {
      led_white_breathe_pwm_config.pwm_parameter_config.length = sizeof(led_white_breathe_pwm_seq_values)/sizeof(uint16_t),
      led_white_breathe_pwm_config.pwm_parameter_config.playback_count = 1,
      led_white_breathe_pwm_config.pwm_parameter_config.repeats = 2;
      led_white_breathe_pwm_config.pwm_parameter_config.flags = PWM_FLAG_STOP;
      break;
    }
    case LED_WHITE_BREATHE_8S:
    {
      led_white_breathe_pwm_config.pwm_parameter_config.length = sizeof(led_white_breathe_pwm_seq_values)/sizeof(uint16_t),
      led_white_breathe_pwm_config.pwm_parameter_config.playback_count = 1,
      led_white_breathe_pwm_config.pwm_parameter_config.repeats = 4;
      led_white_breathe_pwm_config.pwm_parameter_config.flags = PWM_FLAG_LOOP;
      break;
    
    }
    case LED_WHITE_LONG_LIGHT_3S:
    {
      for(uint8_t i = 0; i<4 ;i++)
      {
        led_white_breathe_pwm_seq_values[i] = 9400;
      }
      led_white_breathe_pwm_seq_values[3] = 10000;
      led_white_breathe_pwm_config.pwm_parameter_config.length =4,    //PWM序列中包含的周期个数
      led_white_breathe_pwm_config.pwm_parameter_config.playback_count = 1,
      led_white_breathe_pwm_config.pwm_parameter_config.repeats = 100;
      led_white_breathe_pwm_config.pwm_parameter_config.flags = PWM_FLAG_STOP;
      break;
    }
    case LED_WHITE_FlLASH_3S:
    {
     
      led_white_breathe_pwm_seq_values[0] = 9020;
      led_white_breathe_pwm_seq_values[1] = 9020;
      led_white_breathe_pwm_seq_values[2] = 10000;
      led_white_breathe_pwm_config.pwm_parameter_config.length =3,    //PWM序列中包含的周期个数
      led_white_breathe_pwm_config.pwm_parameter_config.playback_count = 3,
      led_white_breathe_pwm_config.pwm_parameter_config.repeats = 30;
      led_white_breathe_pwm_config.pwm_parameter_config.flags = PWM_FLAG_STOP;
      break;
    }
    case LED_WHITE_FlLASH_CYCLE_300ms_500ms:
    {
        for(uint8_t i = 0; i<30 ;i++)
      {
        led_white_breathe_pwm_seq_values[i] = 9500;
      }
      for(uint8_t i = 0; i<50 ;i++)
      {
        led_white_breathe_pwm_seq_values[i+30] = 10000;
      }
      led_white_breathe_pwm_config.pwm_parameter_config.length =80,    //PWM序列中包含的周期个数
      led_white_breathe_pwm_config.pwm_parameter_config.playback_count = 300,
      led_white_breathe_pwm_config.pwm_parameter_config.repeats = 1;
      led_white_breathe_pwm_config.pwm_parameter_config.flags = PWM_FLAG_LOOP;
      break;
    }
    case LED_WHITE_FlLASH_CYCLE_500ms_500ms:
    {
      
      for(uint8_t i = 0; i<50 ;i++)
      {
        led_white_breathe_pwm_seq_values[i] = 9500;
      }
      for(uint8_t i = 0; i<50 ;i++)
      {
        led_white_breathe_pwm_seq_values[i+50] = 10000;
      }
      led_white_breathe_pwm_config.pwm_parameter_config.length =100,    //PWM序列中包含的周期个数
      led_white_breathe_pwm_config.pwm_parameter_config.playback_count = 300,
      led_white_breathe_pwm_config.pwm_parameter_config.repeats = 1;
      led_white_breathe_pwm_config.pwm_parameter_config.flags = PWM_FLAG_LOOP;
      break;
    }
    case LED_WHITE_FlLASH_CYCLE_300ms_4700ms:
    {
      for(uint8_t i = 0; i<3 ;i++)
      {
        led_white_breathe_pwm_seq_values[i] = 9500;
      }
      for(uint8_t i = 0; i<47 ;i++)
      {
        led_white_breathe_pwm_seq_values[i+3] = 10000;
      }
      led_white_breathe_pwm_config.pwm_parameter_config.length =50,    //PWM序列中包含的周期个数
      led_white_breathe_pwm_config.pwm_parameter_config.playback_count = 300,
      led_white_breathe_pwm_config.pwm_parameter_config.repeats = 10;
      led_white_breathe_pwm_config.pwm_parameter_config.flags = PWM_FLAG_LOOP;
      break;
    }
    case LED_WHITE_LONG_LIGHT:
    {
      for(uint8_t i = 0; i<100 ;i++)
      {
        led_white_breathe_pwm_seq_values[i] = 9500;
      }
      led_white_breathe_pwm_config.pwm_parameter_config.length =100,    //PWM序列中包含的周期个数
      led_white_breathe_pwm_config.pwm_parameter_config.playback_count = 300,
      led_white_breathe_pwm_config.pwm_parameter_config.repeats = 30;
      led_white_breathe_pwm_config.pwm_parameter_config.flags = PWM_FLAG_LOOP;
      break;
    }
    
  }
                                               
	led_pwm_start(&led_white_breathe_pwm_config);
}



void bc_led_stop(void)
{
	bc_ldo_motor_power_off();
	q_device_close(led_pwm_dev);
	pwm_falsh.pwm_status = LED_PWM_IDIE;
  if(led_pwm_stop_callback != NULL)
  {
    led_pwm_stop_callback();
  }
}

void bc_led_pwm_idie_register_callback(void * register_callback)
{
	led_pwm_idie_callback = (bc_led_pwm_idie_callback)register_callback;
}


void bc_led_pwm_stop_register_callback(void * register_callback)
{
	led_pwm_stop_callback = (bc_led_pwm_stop_callback)register_callback;
}


void bc_led_strong_vibration_pwm_config(uint16_t pwm_seq_values,uint8_t playback_count,uint16_t repeats)
{
	strong_vibration_pwm_config.pwm_parameter_config.playback_count = playback_count;
	strong_vibration_pwm_config.pwm_parameter_config.repeats = repeats;
	strong_vibration_pwm_seq_values[0] = pwm_seq_values;
	strong_vibration_pwm_seq_values[1] = pwm_seq_values;
}

void bc_led_continuous_vibration_pwm_config(uint16_t pwm_seq_values,uint8_t playback_count,uint16_t repeats)
{
	continuous_vibration_pwm_config.pwm_parameter_config.playback_count = playback_count;
	continuous_vibration_pwm_config.pwm_parameter_config.repeats = repeats;
	continuous_vibration_pwm_seq_values[0] = pwm_seq_values;
}	

void bc_led_pwm_device_find(void)
{
	
	led_pwm_dev = q_device_find("pwm1");
	q_device_assert(led_pwm_dev);
	q_device_reg_callback(led_pwm_dev,PWM_REGISTER_STOPPED_CALLBACK,led_pwm_callback);
	
}















