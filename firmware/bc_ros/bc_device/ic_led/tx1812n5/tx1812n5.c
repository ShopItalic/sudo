#include "tx1812n5.h"


#include "stdint.h"


#include <stdint.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"

#include "ring_config.h"

#include "bc_rtos.h"

#if defined(__GNUC__)
#define __nop() __NOP()
#endif

// P0 和 P1 基地址
#define GPIO_P0_BASE 0x50000000
#define GPIO_P1_BASE 0x50000300

// 宏定义：根据 PIN 计算正确的引脚号
#define GPIO_PIN_NUMBER(PIN) ((PIN) < 32 ? (PIN) : (PIN) -32)

// 计算寄存器地址的宏
#define PIN_CNF_BASE(PIN) ((PIN) < 32 ? (GPIO_P0_BASE + 0x700) : (GPIO_P1_BASE + 0x700))
#define OUTSET(PIN) ((PIN) < 32 ? (GPIO_P0_BASE + 0x508) : (GPIO_P1_BASE + 0x508))
#define OUTCLR(PIN) ((PIN) < 32 ? (GPIO_P0_BASE + 0x50C) : (GPIO_P1_BASE + 0x50C))

// 寄存器操作的宏定义
#define REG32(addr) (*(volatile uint32_t *)(addr))

#if ( HARDWARE_1121_ENABLED == 1)	

#define GPIO_PIN  3 //NRF_GPIO_PIN_MAP(0,3)

#elif ( HARDWARE_152_ENABLED == 1 || HARDWARE_153_ENABLED == 1) 

#define GPIO_PIN  36 //NRF_GPIO_PIN_MAP(1,4)

#elif ( HARDWARE_158_ENABLED == 1) 

#define GPIO_PIN  36 //NRF_GPIO_PIN_MAP(1,4)

#elif ( HARDWARE_1231_ENABLED == 1) 


#if defined(HANDWARE_1_23_2)  
  
#define GPIO_PIN  3 
  
#elif defined(HANDWARE_1_23_3)
#define GPIO_PIN  28 
#elif defined(HANDWARE_1_23_4)
#define GPIO_PIN  17 
#else
#define GPIO_PIN  14 
#endif



#endif	


void gpio_init(void) {
    // 设置引脚为输出模式
    REG32(PIN_CNF_BASE(GPIO_PIN) + ((GPIO_PIN% 32) * 4)) = (1 << GPIO_PIN_CNF_DIR_Pos) | // 设置方向为输出
                                                  (0 << GPIO_PIN_CNF_INPUT_Pos) | // 断开输入缓冲
                                                   GPIO_PIN_CNF_PULL_Pulldown | // 无上下拉电阻
                                                   GPIO_PIN_CNF_DRIVE_H0H1 | // 标准驱动
                                                  (0 << GPIO_PIN_CNF_SENSE_Pos); // 断开检测
	
}

void gpio_set_high(void) {
    // 设置引脚高电平;
	REG32(OUTSET(GPIO_PIN)) = (1 << GPIO_PIN_NUMBER(GPIO_PIN));
}

void gpio_set_low(void) {
    // 设置引脚低电平
	 REG32(OUTCLR(GPIO_PIN)) = (1 << GPIO_PIN_NUMBER(GPIO_PIN));

}

void gpio_reset(void) {
    // 恢复引脚的默认配置
   REG32(PIN_CNF_BASE(GPIO_PIN) + ((GPIO_PIN % 32) * 4))  = 0x00000000; 
}


void tx1812n5_send_0(void)
{
//	gpio_set_high();
	// 设置引脚高电平;
	REG32(OUTSET(GPIO_PIN)) = (1 << GPIO_PIN_NUMBER(GPIO_PIN));
//	 NRF_P0->OUTSET = (1 << GPIO_PIN);
	__nop();__nop();
	__nop();__nop();
	__nop();__nop();
	__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();
//	gpio_set_low();
	 REG32(OUTCLR(GPIO_PIN)) = (1 << GPIO_PIN_NUMBER(GPIO_PIN));
//	NRF_P0->OUTCLR = (1 << GPIO_PIN);
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
}


void tx1812n5_send_1(void)
{
	// 设置引脚高电平;
	REG32(OUTSET(GPIO_PIN)) = (1 << GPIO_PIN_NUMBER(GPIO_PIN));
//	 NRF_P0->OUTSET = (1 << GPIO_PIN);
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();

	 REG32(OUTCLR(GPIO_PIN)) = (1 << GPIO_PIN_NUMBER(GPIO_PIN));
//	NRF_P0->OUTCLR = (1 << GPIO_PIN);

}


void tx1812n5_byte(uint8_t data)
{
	uint8_t i;
	uint8_t temp = data;
	for(i=8;i>0;i--)
	{
		if(temp & 0x80)
		{
			REG32(OUTSET(GPIO_PIN)) = (1 << GPIO_PIN_NUMBER(GPIO_PIN));
			__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
			__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
			__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
			__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
			__nop();__nop();__nop();__nop();__nop();
			 REG32(OUTCLR(GPIO_PIN)) = (1 << GPIO_PIN_NUMBER(GPIO_PIN));
			__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
			__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
			__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
			__nop();__nop();__nop();__nop();__nop();
		}
		else
		{
			REG32(OUTSET(GPIO_PIN)) = (1 << GPIO_PIN_NUMBER(GPIO_PIN));
			__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
			__nop();__nop();__nop();__nop();__nop();
			 REG32(OUTCLR(GPIO_PIN)) = (1 << GPIO_PIN_NUMBER(GPIO_PIN));
			__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
			__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
			__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
			__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
		}
		temp <<= 1;
	}
}

void tx1812n5_reset(void)
{


	REG32(OUTCLR(GPIO_PIN)) = (1 << GPIO_PIN_NUMBER(GPIO_PIN));

	
	nrf_delay_us(200);

	
	
}


void tx1812n5_RGB(struct rgb_struct * rgb_data,uint16_t rgb_num)
{
	uint16_t i = 0;
//	tx1812n5_reset();
	for(i = 0; i < rgb_num;i++)
	{
        bc_rtos_taskENTER_CRITICAL();
		tx1812n5_byte(rgb_data[i].rgb_g);
		tx1812n5_byte(rgb_data[i].rgb_r);
		tx1812n5_byte(rgb_data[i].rgb_b);
        bc_rtos_taskEXIT_CRITICAL();
	}
}

//void tx1812n5_RGB(struct rgb_struct * rgb_data,uint16_t rgb_num)
//{
//	uint16_t i = 0;
//	tx1812n5_reset();
//	for(i = 0; i < rgb_num;i++)
//	{
//		tx1812n5_byte(rgb_data[i].rgb_g);
//		tx1812n5_byte(rgb_data[i].rgb_r);
//		tx1812n5_byte(rgb_data[i].rgb_b);
//	}
//}


bool flag = false;
  uint8_t led_data[3] = {
        255, 255, 255,  // 第一个 LED：红色
    };
  
struct rgb_struct rgb_config = {.rgb_g = 255,.rgb_r = 255,.rgb_b =255};
void rgb_test(void)
{
  rgb_config.rgb_b = 40;
  rgb_config .rgb_g = 40;
  rgb_config .rgb_r = 40;
	tx1812n5_RGB(&rgb_config ,1);

}


void rgb_test_off(void)
{
  rgb_config.rgb_b = 0;
  rgb_config .rgb_g = 0;
  rgb_config .rgb_r = 0;
	tx1812n5_RGB(&rgb_config ,1);

}

#include "nrf_drv_i2s.h"
#define neopixel_pin                  7   

#define neopixels_number                1   

#define reset_bits                      20                                       /**< Reset bits. */
#define i2s_buffer_size                 ((3 * neopixels_number) + reset_bits)   /**< i2d buffer size for driving the neopixel. */
static uint32_t m_buffer_tx[i2s_buffer_size];
static uint32_t       * volatile mp_block_to_fill  = NULL;

static uint8_t temp_count = 0;

static void i2s_data_handler(nrf_drv_i2s_buffers_t const * p_released,
                             uint32_t                      status)
{
    ret_code_t err_code;
    ASSERT(p_released);

    if (!(status & NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED))
    {
        return;
    }

    if (!p_released->p_rx_buffer)
    {
		temp_count++;
		
		if(temp_count > 3)
		{
			return;
		}
		
        nrf_drv_i2s_buffers_t const next_buffers = {
            .p_tx_buffer = m_buffer_tx,
        };
        err_code = nrf_drv_i2s_next_buffers_set(&next_buffers);
        APP_ERROR_CHECK(err_code);

        mp_block_to_fill = m_buffer_tx;
    }
    else
    {      
        err_code = nrf_drv_i2s_next_buffers_set(p_released);
        APP_ERROR_CHECK(err_code);
        mp_block_to_fill = (uint32_t *)p_released->p_tx_buffer;
    }
}

static uint32_t rgb_channels_value(uint8_t channel_level)
{
    uint32_t value = 0;

    // 0 
    if(channel_level == 0) {
        value = 0x88888888;
    }
    // 255
    else if (channel_level == 255) {
        value = 0xeeeeeeee;
    }
    else 
    {
        // apply 4-bit 0xe HIGH pattern wherever level bits are 1.
        value = 0x88888888;
        for (uint8_t i = 0; i < 8; i++) 
        {
            if((1 << i) & channel_level) 
            {
                uint32_t mask = ~(0x0f << 4*i);
                uint32_t patt = (0x0e << 4*i);
                value = (value & mask) | patt;
            }
        }

        // swap 16 bits
        value = (value >> 16) | (value << 16);
    }

    return value;
}

/**@brief Function for setting the leds (rgb) data in the i2s buffer.
 *
 *@param led_n   neopixel number in the array starting with 0.
 *@param r       red led level.
 *@param g       green led level.
 *@param b       blue led level.
 */
static void set_neopixel_data(uint8_t led_index, uint8_t r, uint8_t g, uint8_t b)
{
    for(int i = 0; i < (3 * neopixels_number); i += 3) 
    {
        if (i == (3 * led_index)) 
        {          
           m_buffer_tx[i]   = rgb_channels_value(g);
           m_buffer_tx[i+1] = rgb_channels_value(r);
           m_buffer_tx[i+2] = rgb_channels_value(b);
          
        }
        else 
        {
            m_buffer_tx[i]   = 0x88888888;
            m_buffer_tx[i+1] = 0x88888888;
            m_buffer_tx[i+2] = 0x88888888;
        }
    }

    // reset 
    for(int i = (3 * neopixels_number); i < i2s_buffer_size; i++) 
    {
        m_buffer_tx[i] = 0;
    }

     nrf_drv_i2s_buffers_t const initial_buffers = {
            .p_tx_buffer = m_buffer_tx,
        };
}

static void i2s_init()
{
    uint32_t err_code = NRF_SUCCESS;

    nrf_drv_i2s_config_t config = NRF_DRV_I2S_DEFAULT_CONFIG;
    config.sdin_pin  = NRFX_I2S_PIN_NOT_USED;
    config.sdout_pin = neopixel_pin;

    config.mck_setup = NRF_I2S_MCK_32MDIV10;
    config.ratio     = NRF_I2S_RATIO_32X;
    config.channels  = NRF_I2S_CHANNELS_STEREO;
    err_code = nrf_drv_i2s_init(&config, i2s_data_handler);
    APP_ERROR_CHECK(err_code);
}


void tx1812n5_rgb_init(void)
{
	gpio_init();
}


void rgb_test_init(void)
{
	gpio_init();
}










