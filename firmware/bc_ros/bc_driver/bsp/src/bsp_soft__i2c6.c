
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

#define SCL_PIN				NRF_GPIO_PIN_MAP(1,11)
#define SDA_PIN				NRF_GPIO_PIN_MAP(1,13)

#define IIC_SCL_PIN     SCL_PIN  // 例如 P0.27
#define IIC_SDA_PIN     SDA_PIN  // 例如 P0.26


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
	
	  .name = "i2c_6",  //i2c0,i2c0为硬件i2c，i2c2之后为模拟i2c
		.lock = false,
	  .i2c_config = {
			              .delay_length = 1,
							.i2c_io_scl_pin = NRF_GPIO_PIN_MAP(1,11),
							.i2c_io_sda_pin = NRF_GPIO_PIN_MAP(1,13), 		    
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

void IIC_Delay_us(uint32_t us)
{
    for(uint32_t i=0; i<us*4; i++)
    {
        __NOP();
    }
}

// SCL 输出低 / 释放高
void IIC_SCL(uint8_t level)
{
    if(level)
    {
        // 释放 SCL = 输入浮空（无上拉）
        nrf_gpio_cfg_input(IIC_SCL_PIN, NRF_GPIO_PIN_NOPULL);
    }
    else
    {
        // 拉低 SCL = 推挽输出 0
        nrf_gpio_cfg_output(IIC_SCL_PIN);
        nrf_gpio_pin_clear(IIC_SCL_PIN);
    }
}

// SDA 输出低 / 释放高
void IIC_SDA(uint8_t level)
{
    if(level)
    {
        // 释放 SDA = 输入浮空
        nrf_gpio_cfg_input(IIC_SDA_PIN, NRF_GPIO_PIN_NOPULL);
    }
    else
    {
        // 拉低 SDA = 推挽输出 0
        nrf_gpio_cfg_output(IIC_SDA_PIN);
        nrf_gpio_pin_clear(IIC_SDA_PIN);
    }
}

// 读取 SDA 电平
uint8_t IIC_READ_SDA(void)
{
    return nrf_gpio_pin_read(IIC_SDA_PIN);
}

// IIC 初始化
void IIC_Init(void)
{
    IIC_SCL(1);
    IIC_SDA(1);
}

// IIC 起始信号
void IIC_Start(void)
{
    IIC_SDA(1);
    IIC_SCL(1);
    IIC_Delay_us(4);
    IIC_SDA(0);
    IIC_Delay_us(4);
    IIC_SCL(0);
}

// IIC 停止信号
void IIC_Stop(void)
{
    IIC_SDA(0);
    IIC_SCL(1);
    IIC_Delay_us(4);
    IIC_SDA(1);
    IIC_Delay_us(4);
}

// 等待 ACK
uint8_t IIC_Wait_Ack(void)
{
    uint8_t timeout = 0;

    IIC_SDA(1);
    IIC_Delay_us(1);
    IIC_SCL(1);
    IIC_Delay_us(1);

    while(IIC_READ_SDA())
    {
        timeout++;
        if(timeout > 250)
        {
            IIC_Stop();
            return 1; // 无应答
        }
    }

    IIC_SCL(0);
    return 0;
}

// 产生 ACK
void IIC_Ack(void)
{
    IIC_SDA(0);
    IIC_Delay_us(1);
    IIC_SCL(1);
    IIC_Delay_us(1);
    IIC_SCL(0);
    IIC_SDA(1);
}

// 产生 NACK
void IIC_NAck(void)
{
    IIC_SDA(1);
    IIC_Delay_us(1);
    IIC_SCL(1);
    IIC_Delay_us(1);
    IIC_SCL(0);
}

// 发送一个字节
void IIC_Send_Byte(uint8_t data)
{
    uint8_t i;

    for(i=0; i<8; i++)
    {
        if(data & 0x80)
            IIC_SDA(1);
        else
            IIC_SDA(0);

        data <<= 1;
        IIC_Delay_us(1);
        IIC_SCL(1);
        IIC_Delay_us(1);
        IIC_SCL(0);
    }
}

// 读取一个字节
uint8_t IIC_Read_Byte(uint8_t ack)
{
    uint8_t i, data = 0;

    for(i=0; i<8; i++)
    {
        IIC_SCL(0);
        IIC_Delay_us(1);
        IIC_SCL(1);
        data <<= 1;
        if(IIC_READ_SDA()) data++;
        IIC_Delay_us(1);
    }

    if(ack)
        IIC_Ack();
    else
        IIC_NAck();

    return data;
}

//// 写 1 字节
//uint8_t IIC_Write_Byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
//{
//    IIC_Start();
//    IIC_Send_Byte(dev_addr << 1);
//    if(IIC_Wait_Ack()) return 1;

//    IIC_Send_Byte(reg_addr);
//    IIC_Wait_Ack();

//    IIC_Send_Byte(data);
//    IIC_Wait_Ack();

//    IIC_Stop();
//    return 0;
//}

//// 读 1 字节
//uint8_t IIC_Read_Byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data)
//{
//    IIC_Start();
//    IIC_Send_Byte(dev_addr << 1);
//    if(IIC_Wait_Ack()) return 1;

//    IIC_Send_Byte(reg_addr);
//    IIC_Wait_Ack();

//    IIC_Start();
//    IIC_Send_Byte((dev_addr << 1) | 1);
//    IIC_Wait_Ack();

//    *data = IIC_Read_Byte(0);
//    IIC_Stop();
//    return 0;
//}

/*******************************************************************************
 * Function Name     : bsp_i2c_init
 * Description       : 初始化模拟i2c io
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/

static void bsp_i2c_init(struct simulation_i2c_config const *bsp_i2c)
{
	IIC_Init();
/* 	bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_scl_pin);
	bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_sda_pin);
	bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);
	bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin); */
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
	IIC_Start();
/* 	bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_scl_pin);  
	bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_sda_pin); 
	
	bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);          //scl  high
	bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);          //sda  high
	bsp_i2c->i2c_delay(bsp_i2c->delay_length);
    bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_sda_pin);           //sda  low
	bsp_i2c->i2c_delay(bsp_i2c->delay_length);
    bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_scl_pin);           // scl low
    bsp_i2c->i2c_delay(bsp_i2c->delay_length); */
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
	IIC_Stop();
/*   bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_scl_pin);
  bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_sda_pin);

  bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_sda_pin);          //sda  low
  bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);         //scl  high
  bsp_i2c->i2c_delay(bsp_i2c->delay_length);
  bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);         //sda  high */
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
	IIC_Send_Byte(data);
	/* 先发送字节的高位bit7 */
 	// for (uint8_t i = 0; i < 8; i++)
	// {        
		// if (data & 0x80)
		// {
			// bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);  //sda  high
		// }else{
			// bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_sda_pin);   //sda  low
		// }
		// bsp_i2c->i2c_delay(bsp_i2c->delay_length);
		// bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);    //scl  high
		// bsp_i2c->i2c_delay(bsp_i2c->delay_length);   
		// bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_scl_pin);     // scl low
		// if (i == 7){
			// bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);  // 释放总线
		// }
		// data <<= 1;    /* 左移一个bit */
		// bsp_i2c->i2c_delay(bsp_i2c->delay_length);
	// }
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
	return IIC_Wait_Ack();
    // uint8_t re;
    // bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);    /* CPU释放SDA总线 */
	// bsp_i2c->i2c_gpio_input_config(bsp_i2c->i2c_io_sda_pin,bsp_i2c->i2c_gpio_input_pull);   //set data input
    // bsp_i2c->i2c_delay(bsp_i2c->delay_length);  
    // bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);   /* CPU驱动SCL = 1, 此时器件会返回ACK应答 */
    // bsp_i2c->i2c_delay(bsp_i2c->delay_length);  
    // if (bsp_i2c->i2c_gpio_read(bsp_i2c->i2c_io_sda_pin))    /* CPU读取SDA口线状态 */
    // {
        // re = 1;
    // }else{
        // re = 0;
    // }
    // bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_scl_pin);
	// bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_sda_pin);  //set data output
    // bsp_i2c->i2c_delay(bsp_i2c->delay_length);  
    // return re;
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
	IIC_Ack();
  // bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_sda_pin);/* CPU驱动SDA = 0 */
  // bsp_i2c->i2c_delay(bsp_i2c->delay_length);
  // bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);    /* CPU产生1个时钟 */
  // bsp_i2c->i2c_delay(bsp_i2c->delay_length);
  // bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_scl_pin); 
  // bsp_i2c->i2c_delay(bsp_i2c->delay_length);
  // bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);    /* CPU释放SDA总线 */
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
	IIC_NAck();
  // bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_sda_pin);   /* CPU驱动SDA = 1 */
  // bsp_i2c->i2c_delay(bsp_i2c->delay_length);
  // bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);     /* CPU产生1个时钟 */
  // bsp_i2c->i2c_delay(bsp_i2c->delay_length);
  // bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_scl_pin); ;
  // bsp_i2c->i2c_delay(bsp_i2c->delay_length);   
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
	return IIC_Read_Byte(ack);
  // uint8_t value;
    // /* 读到第1个bit为数据的bit7 */
	// bsp_i2c->i2c_gpio_input_config(bsp_i2c->i2c_io_sda_pin,bsp_i2c->i2c_gpio_input_pull);  // set data input    
  // bsp_i2c->i2c_delay(bsp_i2c->delay_length);
  // value = 0;
	// for (uint8_t i = 0; i < 8; i++)
	// {
		// value <<= 1;
		// bsp_i2c->i2c_gpio_high(bsp_i2c->i2c_io_scl_pin);
		// bsp_i2c->i2c_delay(bsp_i2c->delay_length);
		// if (bsp_i2c->i2c_gpio_read(bsp_i2c->i2c_io_sda_pin))
		// {
				// value++;
		// }
		// bsp_i2c->i2c_gpio_low(bsp_i2c->i2c_io_scl_pin);
		// bsp_i2c->i2c_delay(bsp_i2c->delay_length);  
	// }
	// bsp_i2c->i2c_gpio_output_config(bsp_i2c->i2c_io_sda_pin);   // set data output    
	// bsp_i2c->i2c_delay(bsp_i2c->delay_length);  
	// if(ack==0)
			// bsp_i2c_nack(bsp_i2c);
	// else
			// bsp_i2c_ack(bsp_i2c);
	// return value;
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
    BC_LOG_ERROR("bsp_i2c2_readreg slave:%d, add:%d, num:%d\r\n",slave, reg_add,num);
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
	BC_LOG_ERROR("bsp_i2c_write66666666666 slave");
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
	BC_LOG_ERROR("bsp_i2c_read6666666666 slave");
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
static void bsp_simulation_i2c6_register(void)
{

		bsp_list.dev.name = bsp_list.name;
		bsp_list.dev.dops  = &ops;
		q_device_register(&bsp_list.dev);		
	
}

//device_initcall(bsp_simulation_i2c6_register);

