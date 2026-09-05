/*******************************************************************************
此为bsp io输出文件，通过宏定义来兼容nordic、phy6222硬件平台

接口遵循q_device规则
日  期：2024年1月17日
编写人：邱成凯
 *******************************************************************************/


#include "q_device.h"

#include <string.h>

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)

#include "nrf_drv_twi.h"


#include "nrf_gpio.h"
#include "bc_logger.h"

#include "nrfx_pwm.h"

#include "nrf_gpio.h"

static volatile ret_code_t nrf_transfer_status;

struct bsp_pwm_aisle
{
	uint32_t aisle_pin;
	bool aisle_status;
};

struct bsp_pwm_parameter
{
	uint16_t top_value;
	uint16_t *p_common;
	uint16_t length;
	uint32_t repeats;
	uint16_t playback_count;
    uint32_t flags;
};

struct bsp_pwm_config
{
	struct bsp_pwm_aisle pwm_aisle_0;
	struct bsp_pwm_aisle pwm_aisle_1;
	struct bsp_pwm_aisle pwm_aisle_2;
	struct bsp_pwm_aisle pwm_aisle_3;
	nrfx_pwm_t pwm_hanler;
	struct bsp_pwm_parameter pwm_parameter;
	
};

typedef void (*bsp_pwm_finished_callback)(void);
typedef void (*bsp_pwm_end_seq0_callback)(void); 
typedef void (*bsp_pwm_end_seq1_callback)(void); 
typedef void (*bsp_pwm_stopped_callback)(void); 

struct  BSP_PWM
{
	const char   *name;
	bool          lock;
	struct bsp_pwm_config pwm_config;
	bsp_pwm_finished_callback pwm_finished_callback;
	bsp_pwm_end_seq0_callback pwm_end_seq0_callback;
	bsp_pwm_end_seq1_callback pwm_end_seq1_callback;
	bsp_pwm_stopped_callback  pwm_stopped_callback;
	void *pwm_callback_handler;
	enum pwm_status pwm_work_status;
	q_device_t dev;
};


static void pwm_callback(nrfx_pwm_evt_type_t event_type);





static struct BSP_PWM bsp_list =    //1--i2c_1(twi1)
{
    .name = "pwm0",
	.lock = false,
#if defined(HANDWARE_1_5_3)

	.pwm_config.pwm_aisle_0.aisle_pin = NRF_GPIO_PIN_MAP(0,17),
#elif defined(HANDWARE_1_5_8)
    .pwm_config.pwm_aisle_0.aisle_pin = NRF_GPIO_PIN_MAP(1,10),
#elif defined(HANDWARE_1_5_6)
    .pwm_config.pwm_aisle_0.aisle_pin = NRF_GPIO_PIN_MAP(0,21),	
#elif defined(HANDWARE_1_16_1)
    .pwm_config.pwm_aisle_0.aisle_pin = NRF_GPIO_PIN_MAP(0,30),	  
#elif defined(HANDWARE_1_17_1)
    .pwm_config.pwm_aisle_0.aisle_pin = NRF_GPIO_PIN_MAP(1,05),	   
#elif defined(HANDWARE_1_19_1)
    .pwm_config.pwm_aisle_0.aisle_pin = NRF_GPIO_PIN_MAP(0,15),	  
#elif defined(HANDWARE_1_23_1)
    .pwm_config.pwm_aisle_0.aisle_pin = NRF_GPIO_PIN_MAP(0,10),	   
//    .pwm_config.pwm_aisle_0.aisle_pin = NRF_GPIO_PIN_MAP(1,04),	   
#endif	
	.pwm_config.pwm_aisle_0.aisle_status = false,

#if defined(HANDWARE_1_19_1)
  .pwm_config.pwm_aisle_1.aisle_pin = NRF_GPIO_PIN_MAP(0,11),
#else
  .pwm_config.pwm_aisle_1.aisle_pin = NRF_GPIO_PIN_MAP(0,29),
#endif  
	
	.pwm_config.pwm_aisle_1.aisle_status = false,
	.pwm_config.pwm_aisle_2.aisle_pin = NRF_GPIO_PIN_MAP(1,10),
	.pwm_config.pwm_aisle_2.aisle_status = false,
	.pwm_config.pwm_aisle_3.aisle_pin = NRF_GPIO_PIN_MAP(0,31),
	.pwm_config.pwm_aisle_3.aisle_status = false,
	.pwm_config.pwm_hanler = NRFX_PWM_INSTANCE(0),
	.pwm_callback_handler = pwm_callback,
	.pwm_finished_callback = NULL,
	.pwm_end_seq0_callback = NULL,
	.pwm_end_seq1_callback = NULL,
	.pwm_stopped_callback = NULL,
	.pwm_work_status = PWM_IDIE,
	.dev = {0},
	
};

static void pwm_callback(nrfx_pwm_evt_type_t event_type)
{
	switch(event_type)
	{
		case NRFX_PWM_EVT_FINISHED:  ///< Sequence playback finished.
		{
			if(bsp_list.pwm_finished_callback != NULL)
			{
				bsp_list.pwm_finished_callback();
			}
			bsp_list.pwm_work_status = PWM_IDIE;
//			nrf_gpio_pin_clear(BSP_LED_0);
			break;
		}
		case NRFX_PWM_EVT_END_SEQ0:  /**< End of sequence 0 reached. Its data can be safely modified now. */
		{
			if(bsp_list.pwm_end_seq0_callback != NULL)
			{
				bsp_list.pwm_end_seq0_callback();
			}
			break;
		}                        
		case NRFX_PWM_EVT_END_SEQ1:  /**< End of sequence 1 reached. Its data can be safely modified now. */
		{
			if(bsp_list.pwm_end_seq1_callback != NULL)
			{
				bsp_list.pwm_end_seq1_callback();
			}
			break;
		}                      
		case NRFX_PWM_EVT_STOPPED:   ///< The PWM peripheral has been stopped.
		{
			if(bsp_list.pwm_stopped_callback != NULL)
			{
				bsp_list.pwm_stopped_callback();
			}
			break;
		}
	}
}

static nrf_pwm_values_common_t seq1_values[] = {10000,10000};//序列1，占空比90%

//播放PWM
#if defined(SUDO_VOICE_ONLY)
static int pwm_play(void)
#else
static void pwm_play(void)
#endif
{
#if defined(SUDO_VOICE_ONLY)
	if(bsp_list.pwm_config.pwm_parameter.p_common == NULL ||
		bsp_list.pwm_config.pwm_parameter.length == 0 ||
		bsp_list.pwm_config.pwm_parameter.playback_count == 0 ||
		bsp_list.pwm_config.pwm_parameter.top_value == 0 ||
		(bsp_list.pwm_config.pwm_parameter.flags != PWM_FLAG_STOP &&
		 bsp_list.pwm_config.pwm_parameter.flags != PWM_FLAG_LOOP))
	{
		return RESULT_CONFIG_NULL_ERR;
	}
#endif
    //定义PWM播放序列，播放序列包含了PWM序列的起始地址、大小和序列播放控制描述
	  nrf_pwm_sequence_t  seq0 =
    {
        .values.p_common = bsp_list.pwm_config.pwm_parameter.p_common,//指向PWM序列
        .length          = bsp_list.pwm_config.pwm_parameter.length,//PWM序列中包含的周期个数
        .repeats         = bsp_list.pwm_config.pwm_parameter.repeats, //序列中周期重复次数为0
        .end_delay       = 0  //序列后不插入延时
    };
	seq1_values[0] = bsp_list.pwm_config.pwm_parameter.top_value;
	seq1_values[1] = bsp_list.pwm_config.pwm_parameter.top_value;
	
	if(bsp_list.pwm_config.pwm_parameter.flags == PWM_FLAG_STOP)
	{
		nrf_pwm_sequence_t  seq1 =
		{
			.values.p_common    = seq1_values,//指向PWM序列1
			.length             = NRF_PWM_VALUES_LENGTH(seq1_values),//PWM序列中包含的周期个数
			.repeats            = 2, //序列中周期重复次数为100
			.end_delay          = 0  //序列后不插入延时
		};
		 nrfx_pwm_complex_playback(&bsp_list.pwm_config.pwm_hanler, &seq0, &seq1,bsp_list.pwm_config.pwm_parameter.playback_count,
                                       bsp_list.pwm_config.pwm_parameter.flags);
	}
	else
	{
		nrfx_pwm_simple_playback(&bsp_list.pwm_config.pwm_hanler,
                                  &seq0,
                                  bsp_list.pwm_config.pwm_parameter.playback_count,
                                  bsp_list.pwm_config.pwm_parameter.flags);
	}
   
#if defined(SUDO_VOICE_ONLY)
	return RESULT_OK;
#endif
}


/*******************************************************************************
 * Function Name     : bsp_gpio_i2c_open
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_pwm_open(q_device_t*dev)
{

	if(bsp_list.lock)
	{
		return RESULT_OK;
	}
	
		//定义PWM初始化配置结构体并初始化参数
    nrfx_pwm_config_t  config0 =
    {
        .output_pins =
        {
			      NRFX_PWM_PIN_NOT_USED,              //通道1不使用
			      NRFX_PWM_PIN_NOT_USED,              //通道1不使用
            NRFX_PWM_PIN_NOT_USED,              //通道2不使用
            NRFX_PWM_PIN_NOT_USED               //通道3不使用
        },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,//中断优先级
        .base_clock   = NRF_PWM_CLK_1MHz,       //PWM时钟频率设置为1MHz  
        .count_mode   = NRF_PWM_MODE_UP,        //向上计数模式
        .top_value    = bsp_list.pwm_config.pwm_parameter.top_value,                  //计数最大值为10000
        .load_mode    = NRF_PWM_LOAD_COMMON,    //通用装载模式
        .step_mode    = NRF_PWM_STEP_AUTO       //序列中的周期自动推进
    };
	if(bsp_list.pwm_config.pwm_aisle_0.aisle_status)
	{
		config0.output_pins[0] = bsp_list.pwm_config.pwm_aisle_0.aisle_pin;
	}
	if(bsp_list.pwm_config.pwm_aisle_1.aisle_status)
	{
		config0.output_pins[1] = bsp_list.pwm_config.pwm_aisle_1.aisle_pin;
	}
	if(bsp_list.pwm_config.pwm_aisle_2.aisle_status)
	{
		config0.output_pins[2] = bsp_list.pwm_config.pwm_aisle_2.aisle_pin;
	}
		//初始化PWM
	 ret_code_t err_code;
	err_code = nrfx_pwm_init(&bsp_list.pwm_config.pwm_hanler, &config0, bsp_list.pwm_callback_handler);
	//Q_DEVICE_LOG_INFO("err_code:%d\r\n",err_code);
#if defined(SUDO_VOICE_ONLY)
	if(err_code != NRF_SUCCESS)
	{
		return (int)err_code;
	}
#else
    APP_ERROR_CHECK(err_code);
#endif
	
	//Q_DEVICE_LOG_INFO("open %s start\r\n",bsp_list.name);			
//    pwm_play();
	bsp_list.lock = true;
	return RESULT_OK;

}

/*******************************************************************************
 * Function Name     : bsp_gpio_output_open
 * Description       : gpio out put open
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_pwm_close(q_device_t*dev)
{

	if(!bsp_list.lock)
	{
		return RESULT_OK;
	}
	
	nrfx_pwm_stop(&bsp_list.pwm_config.pwm_hanler, false);
	nrfx_pwm_uninit(&bsp_list.pwm_config.pwm_hanler);
	//Q_DEVICE_LOG_INFO("close %s \r\n",bsp_list.name);			
  
	bsp_list.lock = false;
	return RESULT_OK;

}


static int bsp_pwm_config(q_device_t *dev, void *args, void *var)
{
	struct pwm_config *cfg = (struct pwm_config *)args;
	if(cfg == NULL)
	{
		return RESULT_GPIO_CONFIG_NULL_ERR;
	}
	
	bsp_list.pwm_config.pwm_aisle_0.aisle_status = cfg->pwm_aisle0_enable_status;
	bsp_list.pwm_config.pwm_aisle_1.aisle_status = cfg->pwm_aisle1_enable_status;	
	bsp_list.pwm_config.pwm_aisle_2.aisle_status = cfg->pwm_aisle2_enable_status;	
	bsp_list.pwm_config.pwm_aisle_3.aisle_status = cfg->pwm_aisle3_enable_status;

	bsp_list.pwm_config.pwm_parameter = *(struct bsp_pwm_parameter*)&cfg->pwm_parameter_config;

	return RESULT_OK;
}


/*******************************************************************************
 * Function Name     : bsp_gpio_exit_irq_register_callback
 * Description       : 驱动注册外部中断回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_pwm_register_callback(q_device_t *dev,int pos, void *exit_irq_callback)
{
	switch(pos)
	{
		case PWM_REGISTER_FINISHED_CALLBACK:
		{
			bsp_list.pwm_finished_callback = (bsp_pwm_finished_callback)exit_irq_callback;
			break;
		}
		case PWM_REGISTER_END_SEQ0_CALLBACK:
		{
			bsp_list.pwm_end_seq0_callback = (bsp_pwm_end_seq0_callback)exit_irq_callback;
			break;
		}
		case PWM_REGISTER_END_SEQ1_CALLBACK:
		{
			bsp_list.pwm_end_seq1_callback = (bsp_pwm_end_seq1_callback)exit_irq_callback;
			break;
		}
		case PWM_REGISTER_STOPPED_CALLBACK:
		{
			bsp_list.pwm_stopped_callback = (bsp_pwm_stopped_callback)exit_irq_callback;
			break;
		}
	}
	return RESULT_OK;	
}

/*******************************************************************************
 * Function Name     : bsp_gpio_output_ctrl
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_pwm_ctrl(q_device_t *dev, int cmd, void *args)
{
   if(!bsp_list.lock)
	{
		return RESULT_DEV_NULL_ERR;
	}
	switch(cmd)
	{
		case PWM_CTRL_START:
		{
			bsp_list.pwm_work_status = PWM_BUSY;
#if defined(SUDO_VOICE_ONLY)
			int result = pwm_play();
			if(result != RESULT_OK)
			{
				return result;
			}
#else
			pwm_play();
#endif
			break;
		}
		case PWM_CTRL_STOP:
		{
			nrfx_pwm_stop(&bsp_list.pwm_config.pwm_hanler, false);
			bsp_list.pwm_work_status = PWM_BUSY;
			break;
		}
	}
	//pwm_play();
	//Q_DEVICE_LOG_INFO("play %s \r\n",bsp_list.name);			
  

	return RESULT_OK;
}

static struct q_device_ops ops =
{
	.register_callback = bsp_pwm_register_callback,
	.open = bsp_pwm_open,
	.close = bsp_pwm_close,
	.config = bsp_pwm_config,
	.control = bsp_pwm_ctrl,
};

/*******************************************************************************
 * Function Name     : bsp_gpio_output_register
 * Description       : 设备注册
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_pwm_register(void)
{
	bsp_list.dev.name = bsp_list.name;
	bsp_list.dev.dops  = &ops;
	q_device_register(&bsp_list.dev);	
}

device_initcall(bsp_pwm_register);

   



#endif