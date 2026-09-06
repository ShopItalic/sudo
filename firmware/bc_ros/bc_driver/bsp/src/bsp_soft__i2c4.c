
/*******************************************************************************
此为bsp 软件模拟i2c4文件，通过宏定义来兼容nordic、phy6222硬件平台

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

#include "ring_config.h"


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

static void i2c_delay_nop(uint32_t time);


static struct BSP_I2C bsp_list =   
{
	
	  .name = "i2c_4",  //i2c0,i2c0为硬件i2c，i2c2之后为模拟i2c
		.lock = false,
	  .i2c_config = {
			              .delay_length = 5,
		  
#if defined(HANDWARE_1_5_3)

						  .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,11),
					      .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,14),
#elif (defined(HANDWARE_4_1_1) || defined(HANDWARE_4_1_2) || defined(RONG_WEI_Z2X))
						  .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,5),
					      .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,13),
#elif defined(HANDWARE_4_0_2) 	
		                  .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,7),
					      .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,6),	
#elif (defined(HANDWARE_4_4_1) )
						  .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,29),
					      .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,28),		  
#elif (defined(HANDWARE_4_1_3) )
						  .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,8),
					      .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,5),		
#elif (defined(HANDWARE_1_12_1) )
						  .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,17),
					      .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,31),	
#elif (defined(HANDWARE_1_5_8) )
						  .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,17),
					      .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,21),	
#elif (defined(HANDWARE_1_5_6) )
						  .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(1,13),
					      .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(1,11),	
#elif (defined(HANDWARE_1_9_1) )
						  .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,0),
					      .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,30),		
#elif (defined(HANDWARE_1_14_1) )
						  .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,17),
					      .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,14),				
#elif (defined(HANDWARE_1_17_1) )
						  .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(1,01),
					    .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,24),	                
#elif (defined(HANDWARE_4_5_1) )
						  .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,29),
					      .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,28),					
#elif (defined(HANDWARE_1_18_1) )
						  .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,17),
					    .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,14),		                
#endif		  
		                  
			              .i2c_gpio_low = nrf_gpio_pin_clear,
			              .i2c_gpio_high = nrf_gpio_pin_set,
						  .i2c_gpio_output_config = nrf_gpio_cfg_output,
			              .i2c_gpio_input_config = nrf_gpio_cfg_input,
			              .i2c_gpio_input_pull = NRF_GPIO_PIN_PULLUP,
//			              .i2c_delay = nrfx_coredep_delay_us,
						  .i2c_delay = i2c_delay_nop,
			              .i2c_gpio_read = nrf_gpio_pin_read,
			              .i2c_gpio_freed = nrf_gpio_cfg_default,
					},
		.dev = {0},
	
	
};

static void i2c_delay_nop(uint32_t time)
{
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	
//	__NOP();
//	__NOP();
//	__NOP();
//	__NOP();
//	__NOP();

}

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
	
	  .name = "i2c_4",  //i2c0,i2c0为硬件i2c，i2c2之后为模拟i2c
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
	bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_scl_pin);
	bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_sda_pin);
	bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);
	bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);
	
		nrf_gpio_cfg(
            bsp_i2c->i2c_io_scl_pin,
            NRF_GPIO_PIN_DIR_OUTPUT,
            NRF_GPIO_PIN_INPUT_DISCONNECT,
            NRF_GPIO_PIN_NOPULL,
            NRF_GPIO_PIN_H0H1,
            NRF_GPIO_PIN_NOSENSE);
                
        nrf_gpio_cfg(
            bsp_i2c->i2c_io_sda_pin,
            NRF_GPIO_PIN_DIR_OUTPUT,
            NRF_GPIO_PIN_INPUT_DISCONNECT,
            NRF_GPIO_PIN_NOPULL,
            NRF_GPIO_PIN_H0H1,
            NRF_GPIO_PIN_NOSENSE);
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
static uint8_t bsp_i2c_writereg(uint8_t slave, uint16_t reg_add,uint8_t reg_dat,struct simulation_i2c_config const *bsp_i2c)
{
	
#if (defined(HANDWARE_1_12_1) || defined(HANDWARE_1_9_1) || defined(HANDWARE_1_14_1) || defined(HANDWARE_1_17_1) || defined(HANDWARE_1_18_1))
	
    bsp_i2c_start(bsp_i2c);
    bsp_i2c_write_byte(slave,bsp_i2c);
    if(bsp_i2c_wait_ack(bsp_i2c)){
            return RESULT_I2C_SEND_ERR;
    }
    bsp_i2c_write_byte(reg_dat,bsp_i2c);    
    if(bsp_i2c_wait_ack(bsp_i2c)){
            return RESULT_I2C_SEND_ERR;
    }
    bsp_i2c_stop(bsp_i2c);
    return RESULT_OK;

	
#elif (defined(HANDWARE_1_5_8) || defined(HANDWARE_1_5_6))
	if(slave == 0x28)
	{
		bsp_i2c_start(bsp_i2c);
		bsp_i2c_write_byte(slave,bsp_i2c);
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
	else
	{
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
#else
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
#endif		
}	
	
	

/*******************************************************************************
 * Function Name     : bsp_i2c_writereg
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static uint8_t bsp_i2c_writeregs(uint8_t slave, uint16_t reg_add,uint8_t *reg_data,uint16_t length,struct simulation_i2c_config const *bsp_i2c)
{
	
#if (defined(HANDWARE_1_12_1) || defined(HANDWARE_1_9_1) || defined(HANDWARE_1_14_1) || defined(HANDWARE_1_17_1) || defined(HANDWARE_1_18_1))
	
    bsp_i2c_start(bsp_i2c);
    bsp_i2c_write_byte(slave,bsp_i2c);
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
#elif (defined(HANDWARE_1_5_8) || defined(HANDWARE_1_5_6))
	if(slave == 0x28)
	{
		bsp_i2c_start(bsp_i2c);
		bsp_i2c_write_byte(slave,bsp_i2c);
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
	else
	{
		bsp_i2c_start(bsp_i2c);
		bsp_i2c_write_byte(slave,bsp_i2c);
		if(bsp_i2c_wait_ack(bsp_i2c)){
				return RESULT_I2C_SEND_ERR;
		}
		bsp_i2c_write_byte(reg_add,bsp_i2c);    
		if(bsp_i2c_wait_ack(bsp_i2c)){
				return RESULT_I2C_SEND_ERR;
		}
		for(uint8_t i = 0 ; i < length;i++)
		{
			bsp_i2c_write_byte(reg_data[i],bsp_i2c);    
			if(bsp_i2c_wait_ack(bsp_i2c)){
					return RESULT_I2C_SEND_ERR;
			}
		}
		
		bsp_i2c_stop(bsp_i2c);
		return RESULT_OK;
	}
#else
	bsp_i2c_start(bsp_i2c);
	bsp_i2c_write_byte(slave,bsp_i2c);
	if(bsp_i2c_wait_ack(bsp_i2c)){
			return RESULT_I2C_SEND_ERR;
	}
	bsp_i2c_write_byte(reg_add,bsp_i2c);    
	if(bsp_i2c_wait_ack(bsp_i2c)){
			return RESULT_I2C_SEND_ERR;
	}
	for(uint8_t i = 0 ; i < length;i++)
	{
		bsp_i2c_write_byte(reg_data[i],bsp_i2c);    
		if(bsp_i2c_wait_ack(bsp_i2c)){
				return RESULT_I2C_SEND_ERR;
		}
	}
	
	bsp_i2c_stop(bsp_i2c);
	return RESULT_OK;
#endif		
	
}
/*******************************************************************************
 * Function Name     : bsp_i2c_readreg
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static uint8_t bsp_i2c_readreg(uint8_t slave, uint16_t reg_add,uint8_t *buf,uint16_t num,struct simulation_i2c_config const *bsp_i2c)
{
#if (defined(HANDWARE_1_12_1) || defined(HANDWARE_1_9_1) || defined(HANDWARE_1_14_1) || defined(HANDWARE_1_17_1) || defined(HANDWARE_1_18_1))
	
     bsp_i2c_start(bsp_i2c);
    bsp_i2c_write_byte(slave,bsp_i2c);
    if(bsp_i2c_wait_ack(bsp_i2c)){
            return RESULT_I2C_SEND_ERR;
    }
    bsp_i2c_write_byte((reg_add >> 8) & 0xff,bsp_i2c); 
    if(bsp_i2c_wait_ack(bsp_i2c)){
        return RESULT_I2C_SEND_ERR;
    }
    bsp_i2c_write_byte(reg_add& 0xff,bsp_i2c); 
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
#elif (defined(HANDWARE_1_5_8) || defined(HANDWARE_1_5_6))
	if(slave == 0x28)
	{
		bsp_i2c_start(bsp_i2c);
		bsp_i2c_write_byte(slave,bsp_i2c);
		if(bsp_i2c_wait_ack(bsp_i2c)){
				return RESULT_I2C_SEND_ERR;
		}
		bsp_i2c_write_byte((reg_add >> 8) & 0xff,bsp_i2c); 
		if(bsp_i2c_wait_ack(bsp_i2c)){
			return RESULT_I2C_SEND_ERR;
		}
		bsp_i2c_write_byte(reg_add& 0xff,bsp_i2c); 
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
    else
	{
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

		for(uint8_t i=0;i<(num-1);i++){
				*buf=bsp_i2c_read_byte(1,bsp_i2c);
				buf++;
		}
		*buf=bsp_i2c_read_byte(0,bsp_i2c);
		bsp_i2c_stop(bsp_i2c);

		return RESULT_OK;
	}		
#else
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

	for(uint8_t i=0;i<(num-1);i++){
			*buf=bsp_i2c_read_byte(1,bsp_i2c);
			buf++;
	}
	*buf=bsp_i2c_read_byte(0,bsp_i2c);
	bsp_i2c_stop(bsp_i2c);

	return RESULT_OK;
#endif		
	
	
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
	if(bsp_list.lock)
	{
		return RESULT_OK;
	}
	bsp_i2c_init(&bsp_list.i2c_config);
	bsp_list.lock = true;
//	Q_DEVICE_LOG_INFO("open %s ",bsp_list.name);	
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
	if(!bsp_list.lock )
	{
		return RESULT_OK;
	}
	
	bsp_i2c_uninit(&bsp_list.i2c_config);

	bsp_list.lock = false;
//	Q_DEVICE_LOG_INFO("close %s ",bsp_list.name);	
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
	
//	Q_DEVICE_LOG_INFO("package->slave_addr %04x ",package->slave_addr);
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
//	Q_DEVICE_LOG_INFO("package->slave_addr %04x ",package->slave_addr);
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
static void bsp_simulation_i2c4_register(void)
{

		bsp_list.dev.name = bsp_list.name;
		bsp_list.dev.dops  = &ops;
		q_device_register(&bsp_list.dev);		
	
}

device_initcall(bsp_simulation_i2c4_register);

