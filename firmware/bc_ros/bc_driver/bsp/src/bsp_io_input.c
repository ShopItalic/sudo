/*******************************************************************************
此为bsp io输入文件，通过宏定义来兼容nordic、phy6222硬件平台

接口遵循q_device规则
日  期：2024年1月17日
编写人：邱成凯
 *******************************************************************************/

#include "q_device.h"


#include <string.h>


//此驱动不做防抖处理，如需防抖，建议在上一层做例如 fml  app

typedef void (*bsp_gpio_exit_input_irq_callback)(uint8_t gpio_pin,uint8_t gpio_status); 


#if (HARDWARE_ARCH_TYPE_NORDIC == 1)

#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_assert.h"


struct  BSP_GPIO_INPUT
{
	const char   *name;
	bool          io_lock;
	uint32_t      bsp_io_pin;
	nrf_gpio_pin_pull_t   pull;
	nrf_gpiote_polarity_t sense;
	q_device_t    dev;
	bsp_gpio_exit_input_irq_callback gpio_exit_input_irq_callback;
};

static struct BSP_GPIO_INPUT bsp_list[] = 
{

#if defined(HANDWARE_1_5_3)
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,31),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "touch_rdy_in",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,14),
		.pull = NRF_GPIO_PIN_PULLUP | NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
#elif defined(HANDWARE_1_8_1)
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,31),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},	
#elif (defined(HANDWARE_4_1_1) || defined(HANDWARE_4_1_2) || defined(RONG_WEI_Z2X))
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,25),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_LOTOHI,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "touch_rdy_in",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,14),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "ppg_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,7),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	
#elif defined(HANDWARE_4_0_2) 	
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,31),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_LOTOHI,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
#elif (defined(HANDWARE_4_4_1))
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,26),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_LOTOHI,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "touch_rdy_in",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,19),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "ppg_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,25),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
    {
		.name = "sys_intput_chg",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,5),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_LOTOHI,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},	
#elif (defined(HANDWARE_4_1_3))
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,11),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_LOTOHI,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "touch_rdy_in",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,28),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
    {
		.name = "sys_intput_chg",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,13),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_LOTOHI,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},	
#elif (defined(HANDWARE_BCL601_151))

	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,26),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_LOTOHI,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
#elif defined(HANDWARE_1_12_1)
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,13),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "ppg_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,21),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},	
	{
		.name = "3008_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,14),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "key_od",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,10),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
#elif defined(HANDWARE_1_5_8)
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,15),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "ppg_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,14),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},	
	{
		.name = "touch_rdy_in",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,11),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "aw86235_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,9),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	}
#elif defined(HANDWARE_1_5_6)
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,31),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "ppg_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,17),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},	
	{
		.name = "touch_rdy_in",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,00),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "st25_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,11),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	}	
#elif (defined(HANDWARE_4_5_1))
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,31),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_LOTOHI,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "ppg_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,6),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
    {
		.name = "sys_intput_chg",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,16),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_LOTOHI,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
#elif defined(HANDWARE_1_9_1)
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,17),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_LOTOHI,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "ppg_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,31),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},	
	{
		.name = "touch_rdy_in",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,9),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "mcu_reset_irq",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,18),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "pmic_irq",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,1),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	}
#elif defined(HANDWARE_1_14_1)
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,04),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "ppg_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,21),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},	
	{
		.name = "sar_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,13),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},	
#elif defined(HANDWARE_1_17_1)
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,25),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "ppg_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,4),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},	
  {
		.name = "st25_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,16),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
  {
		.name = "sys_intput_chg",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,3),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_LOTOHI,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
  {
		.name = "key_od",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,9),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
  {
		.name = "vad_gpio",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,1),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
  
#elif defined(HANDWARE_1_18_1)
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,10),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "ppg_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,30),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},	
  {
		.name = "touch_rdy_in",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,9),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},  
#elif defined(HANDWARE_1_18_1)
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,10),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "ppg_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,30),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},	
  {
		.name = "touch_rdy_in",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,9),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},  
#elif defined(HANDWARE_1_19_1)
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,11),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
	{
		.name = "ppg_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,31),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},	
  {
		.name = "key_od",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,9),
		.pull = NRF_GPIO_PIN_PULLUP,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},  
#elif defined(HANDWARE_1_23_1)
    {
		.name = "pmic_irq",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,1),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_Toggle,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
#if defined(HANDWARE_1_23_3)
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,9),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
  {
		.name = "touch_rdy_in",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,17),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	}, 
  {
		.name = "aw86235_int",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,14),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
#elif  defined(HANDWARE_1_23_4)
    {
		.name = "touch_rdy_in",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,31),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
#else
	{
		.name = "acc_int_1",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,10),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	},
  {
		.name = "touch_rdy_in",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,14),
		.pull = NRF_GPIO_PIN_NOPULL,
		.sense = GPIOTE_CONFIG_POLARITY_HiToLo,
		.dev = {0},
		.gpio_exit_input_irq_callback = NULL
	}, 
#endif
#endif		



};


/*******************************************************************************
 * Function Name     : gpiote_event_handler
 * Description       : gpio irq  callback
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/

static void gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    //printf("gpiote_event_handler ent**********");
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if(pin == bsp_list[i].bsp_io_pin && bsp_list[i].gpio_exit_input_irq_callback != NULL)
		{
            //printf("bsp_list[i].name:%s*********",bsp_list[i].name);
			bsp_list[i].gpio_exit_input_irq_callback(pin,action);
		}
	}
}

/*******************************************************************************
 * Function Name     : bsp_gpio_input_init
 * Description       : gpio初始化
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
//static void bsp_gpio_input_init(void)
//{
//	if(!nrf_drv_gpiote_is_init())
//	{
//		nrf_drv_gpiote_init();
//	}
//	nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
//	
//	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
//	{
//		config.sense = bsp_list[i].sense;
//		config.pull = bsp_list[i].pull;
//		nrf_drv_gpiote_in_init(bsp_list[i].bsp_io_pin, &config, gpiote_event_handler);
//		nrf_drv_gpiote_in_event_enable(bsp_list[i].bsp_io_pin, true);	
//	}
//}

/*******************************************************************************
 * Function Name     : bsp_gpio_input_open
 * Description       : gpio初始化
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_gpio_input_open(q_device_t *dev)
{
    //printf("bsp_gpio_input_open\r\n");
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].io_lock)
			{
				return RESULT_OK;
			}
            //printf("bsp_gpio_input_open find ************\r\n");
			 nrf_gpio_cfg_default(bsp_list[i].bsp_io_pin);
 
			if(!nrf_drv_gpiote_is_init())
			{
				nrf_drv_gpiote_init();
			}
            #if defined(HANDWARE_1_23_X_3_NEWx)
			nrf_drv_gpiote_in_config_t config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
            #else
            nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
            #endif
			
			config.sense = bsp_list[i].sense;
			config.pull = bsp_list[i].pull;
            //printf("nrf_drv_gpiote_in_init find ************\r\n");
			nrf_drv_gpiote_in_init(bsp_list[i].bsp_io_pin, &config, gpiote_event_handler);
			nrf_drv_gpiote_in_event_enable(bsp_list[i].bsp_io_pin, true);	
            //printf("nrf_drv_gpiote_in_event_enable find ************\r\n");
//			if( i == 1)
//			{
//				nrf_gpio_cfg(
//					bsp_list[i].bsp_io_pin,
//					NRF_GPIO_PIN_DIR_INPUT,
//					NRF_GPIO_PIN_INPUT_CONNECT,
//					NRF_GPIO_PIN_PULLUP,
//					NRF_GPIO_PIN_H0H1,
//					NRF_GPIO_PIN_NOSENSE);
//			}
			bsp_list[i].io_lock = true;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;	
}
/*******************************************************************************
 * Function Name     : bsp_gpio_input_colse
 * Description       : gpio初始化
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_gpio_input_close(q_device_t *dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].io_lock)
			{
				return RESULT_OK;
			}
			nrfx_gpiote_in_event_disable(bsp_list[i].bsp_io_pin);
			nrfx_gpiote_in_uninit(bsp_list[i].bsp_io_pin);
			bsp_list[i].io_lock = false;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;		
}



/*******************************************************************************
 * Function Name     : bsp_gpio_output_read
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_gpio_read(q_device_t *dev, int pos,const void *buffer, int size)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			*(uint8_t*)buffer = nrf_gpio_pin_read(bsp_list[i].bsp_io_pin);
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;	
} 

/*******************************************************************************
 * Function Name     : bsp_gpio_exit_irq_register_callback
 * Description       : 驱动注册外部中断回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_gpio_exit_irq_register_callback(q_device_t *dev,int pos, void *exit_irq_callback)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			bsp_list[i].gpio_exit_input_irq_callback = (bsp_gpio_exit_input_irq_callback)exit_irq_callback;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;		
}

#endif







#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)


#include "gpio.h"

struct  BSP_GPIO_PUT
{
	const char   *name;
	bool          io_lock;
	gpio_pin_e      bsp_io_pin;
	gpio_pupd_e pull_type;
	q_device_t dev;
	bsp_gpio_exit_input_irq_callback gpio_lo_to_hi_input_irq_callback;
	bsp_gpio_exit_input_irq_callback gpio_hi_to_lo_input_irq_callback;
};

static struct BSP_GPIO_PUT bsp_list[] = 
{
	{
		.name = "key",
		.io_lock = false,
		.bsp_io_pin = GPIO_P01,
		.pull_type  = GPIO_PULL_DOWN,
		.dev = {0},
		.gpio_lo_to_hi_input_irq_callback = NULL,
		.gpio_hi_to_lo_input_irq_callback = NULL,
	},
};

/*******************************************************************************
 * Function Name     : gpiot_event_handler
 * Description       : gpio irq  callback
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/

static void gpio_lo_to_hi_irq_handler(GPIO_Pin_e pin,IO_Wakeup_Pol_e type)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if(pin == bsp_list[i].bsp_io_pin && bsp_list[i].gpio_lo_to_hi_input_irq_callback != NULL)
		{
			bsp_list[i].gpio_lo_to_hi_input_irq_callback(pin,(uint8_t)type);
		}
	}
}

/*******************************************************************************
 * Function Name     : gpiot_event_handler
 * Description       : gpio irq  callback
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/

static void gpio_hi_to_lo_irq_handler(GPIO_Pin_e pin,IO_Wakeup_Pol_e type)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if(pin == bsp_list[i].bsp_io_pin && bsp_list[i].gpio_hi_to_lo_input_irq_callback != NULL)
		{
			bsp_list[i].gpio_hi_to_lo_input_irq_callback(pin,(uint8_t)type);		
		}
	}
}

/*******************************************************************************
 * Function Name     : bsp_gpio_input_open
 * Description       : gpio初始化
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_gpio_input_open(q_device_t *dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].io_lock)
			{
				return RESULT_OK;
			}
            hal_gpio_pin_init(bsp_list[i].bsp_io_pin,GPIO_INPUT);
			hal_gpio_pull_set(bsp_list[i].bsp_io_pin,bsp_list[i].pull_type);
			bsp_list[i].io_lock = true;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;	
}

/*******************************************************************************
 * Function Name     : bsp_gpio_input_colse
 * Description       : gpio初始化
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_gpio_input_close(q_device_t *dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].io_lock)
			{
				return RESULT_OK;
			}

			hal_gpioin_unregister(bsp_list[i].bsp_io_pin);
			bsp_list[i].gpio_hi_to_lo_input_irq_callback = NULL;
			bsp_list[i].gpio_lo_to_hi_input_irq_callback = NULL;
			bsp_list[i].io_lock = false;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;		
}

/*******************************************************************************
 * Function Name     : bsp_gpio_output_read
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_gpio_read(q_device_t *dev, int pos,const void *buffer, int size)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			*(uint8_t*)buffer = (uint8_t)hal_gpio_read(bsp_list[i].bsp_io_pin);
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;	
}

/*******************************************************************************
 * Function Name     : bsp_gpio_exit_irq_register_callback
 * Description       : 驱动注册外部中断回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/

static int bsp_gpio_exit_irq_register_callback(q_device_t *dev,int pos, void *exit_irq_callback)
{

	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(pos == GPIOT_CONFIG_POLARITY_LoToHi)
			{				
				bsp_list[i].gpio_lo_to_hi_input_irq_callback = (bsp_gpio_exit_input_irq_callback)exit_irq_callback;
				if(bsp_list[i].gpio_hi_to_lo_input_irq_callback!=NULL)
				{
					hal_gpioin_register(bsp_list[i].bsp_io_pin, gpio_lo_to_hi_irq_handler, gpio_hi_to_lo_irq_handler);
				}
				else
				{
					hal_gpioin_register(bsp_list[i].bsp_io_pin, gpio_lo_to_hi_irq_handler, NULL);
				}
			}
			else
			{
				bsp_list[i].gpio_hi_to_lo_input_irq_callback = (bsp_gpio_exit_input_irq_callback)exit_irq_callback;
				if(bsp_list[i].gpio_lo_to_hi_input_irq_callback != NULL)
				{
					hal_gpioin_register(bsp_list[i].bsp_io_pin, gpio_lo_to_hi_irq_handler, gpio_hi_to_lo_irq_handler);
				}
				else
				{
					hal_gpioin_register(bsp_list[i].bsp_io_pin, NULL, gpio_hi_to_lo_irq_handler);
				}
			}
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_INPUT_DEV_NULL_ERR;		
}


#endif



/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static struct q_device_ops ops =
{
	.read = bsp_gpio_read,
	.register_callback = bsp_gpio_exit_irq_register_callback,
	.open = bsp_gpio_input_open,
	.close = bsp_gpio_input_close,

	
};

/*******************************************************************************
 * Function Name     : bsp_gpio_output_register
 * Description       : 设备注册
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_gpio_input_register(void)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		bsp_list[i].dev.name = bsp_list[i].name;
		bsp_list[i].dev.dops  = &ops;
		q_device_register(&bsp_list[i].dev);		
	}
}


device_initcall(bsp_gpio_input_register);








