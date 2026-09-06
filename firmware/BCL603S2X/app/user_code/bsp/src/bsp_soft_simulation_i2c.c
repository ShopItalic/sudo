#include "q_device.h"


#include <string.h>

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

static struct BSP_I2C bsp_list[] =   
{
	{
	  .name = "i2c_2_sys",  //i2c0,i2c0为硬件i2c，i2c2之后为模拟i2c
		.lock = false,
	  .i2c_config = {
			              .delay_length = 2,
			              .i2c_io_scl_pin = NRF_GPIO_PIN_MAP(0,1),
									  .i2c_io_sda_pin = NRF_GPIO_PIN_MAP(0,0),
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
	},
	
};

/*******************************************************************************
 * Function Name     : bsp_i2c_init
 * Description       : 初始化模拟i2c io
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/

static void bsp_i2c_init(struct simulation_i2c_config const *bsp_i2c)
{
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
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_i2c_uninit(struct simulation_i2c_config const *bsp_i2c)
{
	bsp_i2c->i2c_gpio_freed(bsp_i2c->i2c_io_scl_pin);
	bsp_i2c->i2c_gpio_freed(bsp_i2c->i2c_io_sda_pin);
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
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
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
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
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
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
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
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
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
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
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
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
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
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static uint8_t bsp_i2c_writereg(uint8_t slave, uint8_t reg_add,uint8_t reg_dat,struct simulation_i2c_config const *bsp_i2c)
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

/*******************************************************************************
 * Function Name     : bsp_i2c_readreg
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static uint8_t bsp_i2c_readreg(uint8_t slave, uint8_t reg_add,uint8_t *buf,uint8_t num,struct simulation_i2c_config const *bsp_i2c)
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


/*******************************************************************************
 * Function Name     : bsp_i2c_open
 * Description       : 开启模拟i2c
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_i2c_open(q_device_t*dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].lock)
			{
				return RESULT_OK;
			}
			bsp_i2c_init(&bsp_list[i].i2c_config);
			bsp_list[i].lock = true;
//			NRF_LOG_INFO("open %s ",bsp_list[i].name);	
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
}


/*******************************************************************************
 * Function Name     : bsp_i2c_close
 * Description       : 关闭模拟i2c
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_i2c_close(q_device_t*dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].lock)
			{
				return RESULT_OK;
			}
			bsp_i2c_uninit(&bsp_list[i].i2c_config);
			bsp_list[i].lock = true;
//			NRF_LOG_INFO("close %s ",bsp_list[i].name);	
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
}


/*******************************************************************************
 * Function Name     : bsp_i2c_write
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_i2c_write(q_device_t *dev, int pos,const void *buffer, int size)
{
	
	struct i2c_package *package = (struct i2c_package *)buffer;
	if(package == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}
  int err_code;
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_I2C_DEV_NULL_ERR;
			}
			err_code = bsp_i2c_writereg(package->slave_addr, package->reg_addr, package->write_buff[0],&bsp_list[i].i2c_config);
			if(err_code != RESULT_OK)
			{
				return RESULT_I2C_SEND_ERR;
			}
			return RESULT_OK;
		}
	}
  return RESULT_I2C_DEV_NULL_ERR;
}


/*******************************************************************************
 * Function Name     : bsp_i2c_read
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_i2c_read(q_device_t *dev, int pos,const void *buffer, int size)
{
	
	struct i2c_package *package = (struct i2c_package *)buffer;
	if(package == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}
  int err_code;
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_I2C_DEV_UNOPENED_ERR;
			}
			err_code = bsp_i2c_readreg(package->slave_addr, package->reg_addr,package->read_buff, package->read_length,&bsp_list[i].i2c_config);
			if(err_code != RESULT_OK)
			{
				return RESULT_I2C_READ_ERR;
			}
			return RESULT_OK;
		}
	}
  return RESULT_I2C_DEV_NULL_ERR;
}

/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
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
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_simulation_i2c_register(void)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		bsp_list[i].dev.name = bsp_list[i].name;
		bsp_list[i].dev.dops  = &ops;
		q_device_register(&bsp_list[i].dev);		
	}
}

device_initcall(bsp_simulation_i2c_register);

