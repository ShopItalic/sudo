
/*******************************************************************************
此为bsp 软件模拟i2c2文件，通过宏定义来兼容nordic、phy6222硬件平台

接口遵循q_device规则
日  期：2024年1月17日
编写人：邱成凯
 *******************************************************************************/


#include "q_device.h"


#include <string.h>

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)
#include "nrf_gpio.h"
#include "nrf_delay.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"




typedef void (*bsp_i2c_gpio_output_callback)(uint32_t gpio_pin); 

typedef void (*bsp_i2c_gpio_intput_callback)(uint32_t gpio_pin); 

typedef void (*bsp_i2c_gpio_output_config_callback)(uint32_t gpio_pin);

typedef void (*bsp_i2c_gpio_input_config_callback)(uint32_t gpio_pin,nrf_gpio_pin_pull_t pull_config); 

typedef void (*bsp_i2c_gpio_freed_callback)(uint32_t gpio_pin); 

typedef void (*bsp_i2c_delay_callback)(uint32_t delay_length); 

typedef uint32_t (*bsp_i2c_gpio_read_callback)(uint32_t gpio_pin); 


struct simulation_i2c_config
{
	uint32_t delay_length;
	uint32_t                                 i2c_io_scl_pin;
	uint32_t                                 i2c_io_sda_pin;
	bsp_i2c_gpio_output_callback             i2c_gpio_low;
	bsp_i2c_gpio_output_callback             i2c_gpio_high;
	bsp_i2c_gpio_output_config_callback      i2c_gpio_output_config;
	bsp_i2c_gpio_input_config_callback       i2c_gpio_input_config;
	nrf_gpio_pin_pull_t                      i2c_gpio_input_pull;
	bsp_i2c_delay_callback                   i2c_delay;
	bsp_i2c_gpio_read_callback               i2c_gpio_read;
	bsp_i2c_gpio_freed_callback              i2c_gpio_freed;
	
};

struct  BSP_I2C
{
	const char   *name;
	bool          lock;
    struct simulation_i2c_config i2c_config;
	
	q_device_t dev;
};

static struct BSP_I2C bsp_list =   
{
	
	  .name = "i2c_2",  //i2c0,i2c0为硬件i2c，i2c2之后为模拟i2c
		.lock = false,
	  .i2c_config = {
			              .delay_length = 1,
#if defined(HANDWARE_1_5_3)

						.i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,4),
						.i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,01),
	
#elif (defined(HANDWARE_4_1_1) || defined(HANDWARE_4_1_2) || defined(RONG_WEI_Z2X))
						.i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,00),
						.i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,01),
#elif defined(HANDWARE_4_0_2) 
			              .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,13),
						  .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,05),		
#elif (defined(HANDWARE_4_4_1) )
						.i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,8),
						.i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,6),
#elif (defined(HANDWARE_1_12_1) )	
		  
						.i2c_io_scl_pin = NRF_GPIO_PIN_MAP(1,11),
						.i2c_io_sda_pin = NRF_GPIO_PIN_MAP(1,4),
#elif (defined(HANDWARE_1_5_8) )
//						.i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,17),
//						.i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,21),	
						.i2c_io_scl_pin  = NRF_GPIO_PIN_MAP(0,12),
						.i2c_io_sda_pin = NRF_GPIO_PIN_MAP(1,9),	
#elif (defined(HANDWARE_1_5_6) )	
						.i2c_io_scl_pin  = NRF_GPIO_PIN_MAP(1,9),
						.i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,12),	
#elif (defined(HANDWARE_1_9_1) )	
						.i2c_io_scl_pin  = NRF_GPIO_PIN_MAP(0,15),
						.i2c_io_sda_pin  = NRF_GPIO_PIN_MAP(0,14),	
#elif (defined(HANDWARE_1_14_1) )	
						.i2c_io_scl_pin  = NRF_GPIO_PIN_MAP(1,14),
						.i2c_io_sda_pin  = NRF_GPIO_PIN_MAP(1,11),		
#elif (defined(HANDWARE_1_17_1) )	
						.i2c_io_scl_pin  = NRF_GPIO_PIN_MAP(0,19),
						.i2c_io_sda_pin  = NRF_GPIO_PIN_MAP(0,22),	   
#elif (defined(HANDWARE_1_18_1) )	
						.i2c_io_scl_pin  = NRF_GPIO_PIN_MAP(0,15),
						.i2c_io_sda_pin  = NRF_GPIO_PIN_MAP(0,11),	 
            
#elif (defined(HANDWARE_1_14_3) )	

						.i2c_io_scl_pin  = NRF_GPIO_PIN_MAP(1,14),
						.i2c_io_sda_pin  = NRF_GPIO_PIN_MAP(1,11),	
            
#elif (defined(HANDWARE_1_23_2) )	
						.i2c_io_scl_pin  = NRF_GPIO_PIN_MAP(0,31),
						.i2c_io_sda_pin  = NRF_GPIO_PIN_MAP(1,9),
#elif (defined(HANDWARE_1_23_3))
	.i2c_io_scl_pin = NRF_GPIO_PIN_MAP(1,11),
	.i2c_io_sda_pin = NRF_GPIO_PIN_MAP(1,13), 		  
#endif   
			              .i2c_gpio_low = nrf_gpio_pin_clear,
			              .i2c_gpio_high = nrf_gpio_pin_set,
						  .i2c_gpio_output_config = nrf_gpio_cfg_output,
			              .i2c_gpio_input_config = nrf_gpio_cfg_input,
			              .i2c_gpio_input_pull = NRF_GPIO_PIN_PULLUP,
			              .i2c_delay = nrfx_coredep_delay_us,
			              .i2c_gpio_read = nrf_gpio_pin_read,
			              .i2c_gpio_freed = nrf_gpio_cfg_default,
					},
		.dev = {0},
	
	
};

#endif


#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)
#include "gpio.h"

typedef void (*bsp_i2c_gpio_output_callback)(uint32_t gpio_pin); 

typedef void (*bsp_i2c_gpio_intput_callback)(uint32_t gpio_pin); 

typedef void (*bsp_i2c_gpio_output_config_callback)(uint32_t gpio_pin);

typedef void (*bsp_i2c_gpio_input_config_callback)(uint32_t gpio_pin,gpio_pupd_e pull_config); 

typedef void (*bsp_i2c_gpio_freed_callback)(uint32_t gpio_pin); 

typedef void (*bsp_i2c_delay_callback)(uint32_t delay_length); 

typedef uint32_t (*bsp_i2c_gpio_read_callback)(uint32_t gpio_pin); 


static void bsp_gpio_output_low(uint32_t gpio_pin);

static void bsp_gpio_output_high(uint32_t gpio_pin);

static void bsp_gpio_output_config(uint32_t gpio_pin);

static void bsp_gpio_input_config(uint32_t gpio_pin,gpio_pupd_e pull_config);

static void bsp_gpio_freed(uint32_t gpio_pin);

static void bsp_gpio_delay(uint32_t delay_length);

static uint32_t bsp_gpio_read(uint32_t gpio_pin);

struct simulation_i2c_config
{
	uint32_t delay_length;
	uint32_t                                 i2c_io_scl_pin;
	uint32_t                                 i2c_io_sda_pin;
	bsp_i2c_gpio_output_callback             i2c_gpio_low;
	bsp_i2c_gpio_output_callback             i2c_gpio_high;
	bsp_i2c_gpio_output_config_callback      i2c_gpio_output_config;
	bsp_i2c_gpio_input_config_callback       i2c_gpio_input_config;
	gpio_pupd_e                              i2c_gpio_input_pull;
	bsp_i2c_delay_callback                   i2c_delay;
	bsp_i2c_gpio_read_callback               i2c_gpio_read;
	bsp_i2c_gpio_freed_callback              i2c_gpio_freed;
	
};

struct  BSP_I2C
{
	const char   *name;
	bool          lock;
    struct simulation_i2c_config i2c_config;
	
	q_device_t dev;
};

static struct BSP_I2C bsp_list =   
{
	
	  .name = "i2c_2",  //i2c0,i2c0为硬件i2c，i2c2之后为模拟i2c
	  .lock = false,
	  .i2c_config = {
			              .delay_length = 2,
			              .i2c_io_scl_pin = 0,
						  .i2c_io_sda_pin = 1,
			              .i2c_gpio_low = bsp_gpio_output_low,
			              .i2c_gpio_high = bsp_gpio_output_high,
						  .i2c_gpio_output_config = bsp_gpio_output_config,
			              .i2c_gpio_input_config = bsp_gpio_input_config,
			              .i2c_gpio_input_pull = GPIO_PULL_UP_S,
			              .i2c_delay = bsp_gpio_delay,
			              .i2c_gpio_read = bsp_gpio_read,
			              .i2c_gpio_freed = bsp_gpio_freed,
					},
		.dev = {0},
	
	
};

static void bsp_gpio_output_low(uint32_t gpio_pin)
{
	AP_GPIO->swporta_dr &= ~BIT(gpio_pin);
}

static void bsp_gpio_output_high(uint32_t gpio_pin)
{
	AP_GPIO->swporta_dr |= BIT(gpio_pin);
}


static void bsp_gpio_output_config(uint32_t gpio_pin)
{
	hal_gpio_pin_init((gpio_pin_e)gpio_pin,GPIO_OUTPUT);
}

static void bsp_gpio_input_config(uint32_t gpio_pin,gpio_pupd_e pull_config)
{
	hal_gpio_pin_init((gpio_pin_e)gpio_pin,GPIO_INPUT);
	hal_gpio_pull_set((gpio_pin_e)gpio_pin, pull_config);
	
}

static void bsp_gpio_freed(uint32_t gpio_pin)
{
	 hal_gpio_pin_init((gpio_pin_e)gpio_pin,GPIO_INPUT);
}

static void bsp_gpio_delay(uint32_t delay_length)
{
	__NOP();__NOP();__NOP();__NOP();__NOP(); \
    __NOP();__NOP();__NOP();__NOP();__NOP(); \
    __NOP();__NOP();__NOP();__NOP();__NOP(); \
    __NOP();__NOP();__NOP();__NOP();__NOP();
}

static uint32_t bsp_gpio_read(uint32_t gpio_pin)
{
	return hal_gpio_read((gpio_pin_e)gpio_pin);
}


#endif

/*******************************************************************************
 * Function Name     : bsp_i2c_init
 * Description       : 初始化模拟i2c io
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/

static void bsp_i2c_init(struct simulation_i2c_config const *bsp_i2c)
{
    //BC_LOG_INFO("I2C2 bsp_i2c_init\\r\n");
	bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_scl_pin);
	bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_sda_pin);
	bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);
	bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);
}

/*******************************************************************************
 * Function Name     : bsp_i2c_init
 * Description       : 释放模拟i2c io
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_i2c_uninit(struct simulation_i2c_config const *bsp_i2c)
{
    //BC_LOG_INFO("I2C2 bsp_i2c_uninit\\r\n");
	bsp_i2c->i2c_gpio_freed(bsp_i2c->i2c_io_scl_pin);
	bsp_i2c->i2c_gpio_freed(bsp_i2c->i2c_io_sda_pin);
//	nrf_gpio_cfg_input(bsp_i2c->i2c_io_scl_pin, NRF_GPIO_PIN_NOPULL);
//	nrf_gpio_cfg_input(bsp_i2c->i2c_io_sda_pin, NRF_GPIO_PIN_NOPULL);
}
/*******************************************************************************
 * Function Name     : bsp_i2c_start
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_i2c_start(struct simulation_i2c_config const *bsp_i2c)
{
    //BC_LOG_INFO("I2C2 bsp_i2c_start\\r\n");
	bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_scl_pin);  
	bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_sda_pin); 
	
	bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);          //scl  high
	bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);          //sda  high
	bsp_i2c->i2c_delay(bsp_i2c->delay_length);
    bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_sda_pin);           //sda  low
	bsp_i2c->i2c_delay(bsp_i2c->delay_length);
    bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_scl_pin);           // scl low
    bsp_i2c->i2c_delay(bsp_i2c->delay_length);
}
/*******************************************************************************
 * Function Name     : bsp_i2c_stop
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_i2c_stop(struct simulation_i2c_config const *bsp_i2c)
{
    //BC_LOG_INFO("I2C2 bsp_i2c_stop\\r\n");
  bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_scl_pin);
  bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_sda_pin);

  bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_sda_pin);          //sda  low
  bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);         //scl  high
  bsp_i2c->i2c_delay(bsp_i2c->delay_length);
  bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);         //sda  high
}
/*******************************************************************************
 * Function Name     : bsp_i2c_write_byte
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_i2c_write_byte(uint8_t data , struct simulation_i2c_config const *bsp_i2c)
{
    //BC_LOG_INFO("I2C2 bsp_i2c_write_byte\\r\n");
	/* 先发送字节的高位bit7 */
	for (uint8_t i = 0; i < 8; i++)
	{        
		if (data & 0x80)
		{
			bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);  //sda  high
		}else{
			bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_sda_pin);   //sda  low
		}
		bsp_i2c->i2c_delay(bsp_i2c->delay_length);
		bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);    //scl  high
		bsp_i2c->i2c_delay(bsp_i2c->delay_length);   
		bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_scl_pin);     // scl low
		if (i == 7){
			bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);  // 释放总线
		}
		data <<= 1;    /* 左移一个bit */
		bsp_i2c->i2c_delay(bsp_i2c->delay_length);
	}
}
/*******************************************************************************
 * Function Name     : bsp_i2c_wait_ack
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static uint8_t bsp_i2c_wait_ack(struct simulation_i2c_config const *bsp_i2c)
{
    uint8_t re;
    bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);    /* CPU释放SDA总线 */
	bsp_i2c->i2c_gpio_input_config(bsp_i2c->i2c_io_sda_pin,bsp_i2c->i2c_gpio_input_pull);   //set data input
    bsp_i2c->i2c_delay(bsp_i2c->delay_length);  
    bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);   /* CPU驱动SCL = 1, 此时器件会返回ACK应答 */
    bsp_i2c->i2c_delay(bsp_i2c->delay_length);  
    if (bsp_i2c->i2c_gpio_read(bsp_i2c->i2c_io_sda_pin))    /* CPU读取SDA口线状态 */
    {
        re = 1;
    }else{
        re = 0;
    }
    bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_scl_pin);
	bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_sda_pin);  //set data output
    bsp_i2c->i2c_delay(bsp_i2c->delay_length);  
    return re;
}
/*******************************************************************************
 * Function Name     : bsp_i2c_ack
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_i2c_ack(struct simulation_i2c_config const *bsp_i2c)
{
  bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_sda_pin);/* CPU驱动SDA = 0 */
  bsp_i2c->i2c_delay(bsp_i2c->delay_length);
  bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);    /* CPU产生1个时钟 */
  bsp_i2c->i2c_delay(bsp_i2c->delay_length);
  bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_scl_pin); 
  bsp_i2c->i2c_delay(bsp_i2c->delay_length);
  bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);    /* CPU释放SDA总线 */
}
/*******************************************************************************
 * Function Name     : bsp_i2c_nack
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_i2c_nack(struct simulation_i2c_config const *bsp_i2c)
{
  bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);   /* CPU驱动SDA = 1 */
  bsp_i2c->i2c_delay(bsp_i2c->delay_length);
  bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);     /* CPU产生1个时钟 */
  bsp_i2c->i2c_delay(bsp_i2c->delay_length);
  bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_scl_pin); ;
  bsp_i2c->i2c_delay(bsp_i2c->delay_length);   
}
/*******************************************************************************
 * Function Name     : bsp_i2c_read_byte
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static uint8_t bsp_i2c_read_byte(uint8_t ack,struct simulation_i2c_config const *bsp_i2c)
{
  uint8_t value;
    //BC_LOG_INFO("I2C2 bsp_i2c_read_byte\\r\n");
    /* 读到第1个bit为数据的bit7 */
	bsp_i2c->i2c_gpio_input_config(bsp_i2c->i2c_io_sda_pin,bsp_i2c->i2c_gpio_input_pull);  // set data input    
  bsp_i2c->i2c_delay(bsp_i2c->delay_length);
  value = 0;
	for (uint8_t i = 0; i < 8; i++)
	{
		value <<= 1;
		bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);
		bsp_i2c->i2c_delay(bsp_i2c->delay_length);
		if (bsp_i2c->i2c_gpio_read(bsp_i2c->i2c_io_sda_pin))
		{
				value++;
		}
		bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_scl_pin);
		bsp_i2c->i2c_delay(bsp_i2c->delay_length);  
	}
	bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_sda_pin);   // set data output    
	bsp_i2c->i2c_delay(bsp_i2c->delay_length);  
	if(ack==0)
			bsp_i2c_nack(bsp_i2c);
	else
			bsp_i2c_ack(bsp_i2c);
	return value;
}
/*******************************************************************************
 * Function Name     : bsp_i2c_writereg
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static uint8_t bsp_i2c_writereg(uint8_t slave, uint8_t reg_add,uint8_t reg_dat,struct simulation_i2c_config const *bsp_i2c)
{
    //BC_LOG_INFO("I2C2 bsp_i2c_writereg\\r\n");
	bsp_i2c_start(bsp_i2c);
	bsp_i2c_write_byte(slave,bsp_i2c);
	if(bsp_i2c_wait_ack(bsp_i2c)){
			return RESULT_I2C_SEND_ERR;
	}
	bsp_i2c_write_byte(reg_add,bsp_i2c);    
	if(bsp_i2c_wait_ack(bsp_i2c)){
			return RESULT_I2C_SEND_ERR;
	}
	bsp_i2c_write_byte(reg_dat,bsp_i2c);    
	if(bsp_i2c_wait_ack(bsp_i2c)){
			return RESULT_I2C_SEND_ERR;
	}
	bsp_i2c_stop(bsp_i2c);
	return RESULT_OK;
}

/*******************************************************************************
 * Function Name     : bsp_i2c_writereg
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static uint8_t bsp_i2c_writeregs(uint8_t slave, uint8_t reg_add,uint8_t *reg_data,uint16_t length,struct simulation_i2c_config const *bsp_i2c)
{
    //BC_LOG_INFO("I2C2 bsp_i2c_writeregs\\r\n");
	bsp_i2c_start(bsp_i2c);
	bsp_i2c_write_byte(slave,bsp_i2c);
	if(bsp_i2c_wait_ack(bsp_i2c)){
			return RESULT_I2C_SEND_ERR;
	}
	bsp_i2c_write_byte(reg_add,bsp_i2c);    
	if(bsp_i2c_wait_ack(bsp_i2c)){
			return RESULT_I2C_SEND_ERR;
	}
	for(uint16_t i = 0 ; i < length;i++)
	{
		bsp_i2c_write_byte(reg_data[i],bsp_i2c);    
		if(bsp_i2c_wait_ack(bsp_i2c)){
				return RESULT_I2C_SEND_ERR;
		}
	}
	
	bsp_i2c_stop(bsp_i2c);
	return RESULT_OK;
}
/*******************************************************************************
 * Function Name     : bsp_i2c_readreg
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static uint8_t bsp_i2c_readreg(uint8_t slave, uint8_t reg_add,uint8_t *buf,uint16_t num,struct simulation_i2c_config const *bsp_i2c)
{
    //BC_LOG_INFO("bsp_i2c2_readreg slave:%d, add:%d, num:%d\r\n",slave, reg_add,num);
	bsp_i2c_start(bsp_i2c);
	bsp_i2c_write_byte(slave,bsp_i2c);
	if(bsp_i2c_wait_ack(bsp_i2c)){
			return RESULT_I2C_SEND_ERR;
	}
	 bsp_i2c_write_byte(reg_add,bsp_i2c); 
	if(bsp_i2c_wait_ack(bsp_i2c)){
			return RESULT_I2C_SEND_ERR;
	}

	bsp_i2c_start(bsp_i2c);
	bsp_i2c_write_byte(slave|0x01,bsp_i2c);
	if(bsp_i2c_wait_ack(bsp_i2c)){
			return RESULT_I2C_SEND_ERR;
	}

	for(uint16_t i=0;i<(num-1);i++){
			*buf=bsp_i2c_read_byte(1,bsp_i2c);
			buf++;
	}
	*buf=bsp_i2c_read_byte(0,bsp_i2c);
	bsp_i2c_stop(bsp_i2c);

	return RESULT_OK;
}


/*******************************************************************************
 * Function Name     : bsp_i2c_open
 * Description       : 开启模拟i2c
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_i2c_open(q_device_t*dev)
{
//BC_LOG_INFO("I2C2 bsp_i2c_open\\r\n");
	if(bsp_list.lock)
	{
		return RESULT_OK;
	}
	bsp_i2c_init(&bsp_list.i2c_config);
	bsp_list.lock = true;
//			NRF_LOG_INFO("open %s ",bsp_list[i].name);	
	return RESULT_OK;
}


/*******************************************************************************
 * Function Name     : bsp_i2c_close
 * Description       : 关闭模拟i2c
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_i2c_close(q_device_t*dev)
{
//BC_LOG_INFO("I2C2 bsp_i2c_close\\r\n");
	if(!bsp_list.lock)
	{
		return RESULT_OK;
	}
	bsp_i2c_uninit(&bsp_list.i2c_config);

	bsp_list.lock = false;
//			NRF_LOG_INFO("close %s ",bsp_list[i].name);	
	return RESULT_OK;
		

}


/*******************************************************************************
 * Function Name     : bsp_i2c_write
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_i2c_write(q_device_t *dev, int pos,const void *buffer, int size)
{
	//BC_LOG_INFO("bsp_i2c222_write slave");
	struct i2c_package *package = (struct i2c_package *)buffer;
	if(package == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}
    int err_code;

	if(!bsp_list.lock)
	{
		return RESULT_I2C_DEV_NULL_ERR;
	}
	if(package->write_length == 1)
	{
		err_code = bsp_i2c_writereg(package->slave_addr, package->reg_addr, package->write_buff[0],&bsp_list.i2c_config);
	}
	else
	{
		 err_code = bsp_i2c_writeregs(package->slave_addr, package->reg_addr, package->write_buff,package->write_length,&bsp_list.i2c_config);
	}
	
	
	
	if(err_code != RESULT_OK)
	{
		return RESULT_I2C_SEND_ERR;
	}
	return RESULT_OK;

}


/*******************************************************************************
 * Function Name     : bsp_i2c_read
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_i2c_read(q_device_t *dev, int pos,const void *buffer, int size)
{
	//BC_LOG_INFO("bsp_i2c_read22222222222222 slave");
	struct i2c_package *package = (struct i2c_package *)buffer;
	if(package == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}
    int err_code;

	if(!bsp_list.lock)
	{
		return RESULT_I2C_DEV_UNOPENED_ERR;
	}
	err_code = bsp_i2c_readreg(package->slave_addr, package->reg_addr,package->read_buff, package->read_length,&bsp_list.i2c_config);
	if(err_code != RESULT_OK)
	{
		return RESULT_I2C_READ_ERR;
	}
	return RESULT_OK;

}

/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static struct q_device_ops ops =
{
	.write = bsp_i2c_write,
	.read = bsp_i2c_read,
	.open = bsp_i2c_open,
	.close = bsp_i2c_close,
	
};

/*******************************************************************************
 * Function Name     : bsp_gpio_output_register
 * Description       : 设备注册
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_simulation_i2c2_register(void)
{

		bsp_list.dev.name = bsp_list.name;
		bsp_list.dev.dops  = &ops;
		q_device_register(&bsp_list.dev);		
	
}

device_initcall(bsp_simulation_i2c2_register);

