/*******************************************************************************
此为bsp io输出文件，通过宏定义来兼容nordic、phy6222硬件平台

接口遵循q_device规则
日  期：2024年1月17日
编写人：邱成凯
 *******************************************************************************/


#include "q_device.h"

#include <string.h>


#if (HARDWARE_ARCH_TYPE_NORDIC == 1)

#include "nrf_gpio.h"

#include "nrf_drv_gpiote.h"

enum bsp_gpio_mode    //nordic  io操作有两种模式
{
	GPIOTE = 0,
	GPIO,
	GPIO_MODE_NUM,
};



struct  BSP_GPIO_PUT
{
	const char   *name;
	bool          io_lock;
	uint32_t      bsp_io_pin;
	gpio_ctrl_cmd  pull;
	enum bsp_gpio_mode gpio_mode;
	nrf_drv_gpiote_out_config_t config;
	struct bsp_gpio_mutex_lock  gpio_mutex_lock;
	q_device_t dev;
};

static struct BSP_GPIO_PUT bsp_list[] = 
{

#if defined(HANDWARE_1_5_3)

	{
		.name = "ppg_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,21),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ppg_vdd_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,10),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "touch_rdy_out",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,14),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
	    .name = "bos_pwr_en",
		.io_lock = false,
	    .bsp_io_pin = NRF_GPIO_PIN_MAP(0,17),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},

#elif defined(HANDWARE_1_8_1)

	{
		.name = "imu_cs",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,3),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "imu_sdo",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,1),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "rs2323_sw",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,10),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "rs2323_vdd_sw",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,11),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
	    .name = "ship_mode_en",
		.io_lock = false,
	    .bsp_io_pin = NRF_GPIO_PIN_MAP(1,14),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},

	
#elif (defined(HANDWARE_4_1_1) || defined(HANDWARE_4_1_2) || defined(RONG_WEI_Z2X))
	{
		.name = "ppg_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,17),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ppg_vdd_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,18),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	
	{
	    .name = "puf_vdd_en",
		.io_lock = false,
	    .bsp_io_pin = NRF_GPIO_PIN_MAP(0,12),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},	
	{
		.name = "sys_int_chg",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,28),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIOTE,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "touch_rdy_out",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,14),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
   {
	    .name = "led_blue",
		.io_lock = false,
	    .bsp_io_pin = NRF_GPIO_PIN_MAP(0,31),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
    {
	    .name = "led_red",
		.io_lock = false,
	    .bsp_io_pin = NRF_GPIO_PIN_MAP(0,27),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	
#elif defined(HANDWARE_4_0_2) 

	{
		.name = "ppg_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,17),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "sys_int_chg",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,11),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIOTE,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
	    .name = "led_blue",
		.io_lock = false,
	    .bsp_io_pin = NRF_GPIO_PIN_MAP(0,12),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
#elif defined(HANDWARE_4_1_3) 

	{
		.name = "ppg_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,6),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "sys_int_chg",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,13),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIOTE,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
	    .name = "led_blue",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,31),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
    {
	    .name = "led_red",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,29),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "touch_rdy_out",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,28),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},	
	{
		.name = "ship_mode_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,17),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},		
#elif defined(HANDWARE_4_4_1) 

	{
		.name = "ppg_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,23),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "sys_int_chg",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,5),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIOTE,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
	    .name = "led_blue",
		.io_lock = false,
//	    .bsp_io_pin = NRF_GPIO_PIN_MAP(0,27),
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,31),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
    {
	    .name = "led_red",
		.io_lock = false,
//	    .bsp_io_pin = NRF_GPIO_PIN_MAP(0,31),
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,27),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "touch_rdy_out",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,19),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},	
	{
		.name = "ship_mode_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,7),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},	

#elif (defined(HANDWARE_BCL601_151))

	{
		.name = "ship_mode_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,1),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},	
	{
		.name = "mic_power_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,12),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},	
	{
		.name = "flash_power_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,8),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	
#elif defined(HANDWARE_1_5_8)

	{
		.name = "ppg_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,00),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ppg_reset_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,01),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "aw86235_reset",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,10),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "touch_rdy_out",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,11),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},	
#elif defined(HANDWARE_1_5_6)

	{
		.name = "ppg_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,14),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ppg_reset_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,9),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "touch_rdy_out",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,0),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "st25_lpd",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,3),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ts2323a_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,28),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ship_mode_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,14),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "motor_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,10),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
	    .name = "led_blue",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,10),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
    {
	    .name = "led_red",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,10),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	
#elif defined(HANDWARE_1_12_1)

	{
		.name = "ppg_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,11),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ppg_vdd_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,28),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ppg_reset_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,30),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "vdd_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,4),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
#elif defined(HANDWARE_4_5_1) 

	{
		.name = "ppg_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,13),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "sys_int_chg",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,16),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIOTE,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
	    .name = "led_blue",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,17),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
    {
	    .name = "led_red",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,17),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},

	{
		.name = "ship_mode_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,15),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},	
#elif defined(HANDWARE_1_9_1)

	{
		.name = "ppg_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,1),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ppg_reset_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,04),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ship_mode_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,9),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},	
	{
		.name = "touch_rdy_out",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,9),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
#elif defined(HANDWARE_1_14_1)

	{
		.name = "ppg_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,30),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ppg_reset_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,04),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "vdd_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,4),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},	
	{
	    .name = "led_blue",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,10),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
    {
	    .name = "led_red",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,10),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ship_mode_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,11),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
#elif defined(HANDWARE_1_17_1)

	{
		.name = "ppg_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,30),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ppg_reset_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,7),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "vdd_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,4),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},	
	{
	    .name = "led_blue",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,10),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
    {
	    .name = "led_red",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,10),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "en_1v2",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,11),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
  {
		.name = "st25_reset",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,00),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
  
#elif defined(HANDWARE_1_18_1)
	{
		.name = "ppg_reset_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,00),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	
	{
	    .name = "led_blue",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,01),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
    {
	    .name = "led_red",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,01),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
  {
		.name = "ship_mode_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,12),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
  {
		.name = "touch_rdy_out",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,14),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	}, 
#elif defined(HANDWARE_1_19_1)
	{
		.name = "ppg_reset_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,04),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	
	{
	    .name = "led_white",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,15),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
    {
	    .name = "led_red",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,11),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
  {
		.name = "ship_mode_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,17),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
  {
		.name = "wifi_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,10),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	}, 
  
#elif defined(HANDWARE_1_23_1)
#if defined(HANDWARE_1_23_3)
	
  {
		.name = "ship_mode_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,04),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
  {
		.name = "touch_rdy_out",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,17),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
  {
		.name = "aw86235_reset",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,10),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
#elif defined(HANDWARE_1_23_4)
    {
		.name = "ship_mode_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,14),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
    {
		.name = "touch_rdy_out",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,31),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
#else
  {
		.name = "ship_mode_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,11),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
  {
		.name = "touch_rdy_out",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,14),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
#endif    
//  {
//		.name = "motor_en",
//		.io_lock = false,
//		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,17),
//		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
//		.pull = GPIO_OUTPUT_LOW,
//		.gpio_mode = GPIO,
//		.gpio_mutex_lock = {0},
//		.dev = {0},
//	},  
#if defined(HANDWARE_1_23_2)  
  
  {
		.name = "vdd_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,28),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
   {
		.name = "motor_vcc",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,9),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
//   {
//		.name = "motor_en",
//		.io_lock = false,
//		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,10),
//		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
//		.pull = GPIO_OUTPUT_LOW,
//		.gpio_mode = GPIO,
//		.gpio_mutex_lock = {0},
//		.dev = {0},
//	},  
//  
#else
#if defined(HANDWARE_1_23_3)
  {
		.name = "vdd_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,3),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
#elif defined(HANDWARE_1_23_4)
    {
		.name = "vdd_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,11),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
#else
  {
		.name = "vdd_led_en",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(0,17),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_LOW,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
#endif
#endif  
#ifndef HANDWARE_1_23_3
  {
	    .name = "led_red",
		.io_lock = false,
		.bsp_io_pin = NRF_GPIO_PIN_MAP(1,04),
		.config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false),
		.pull = GPIO_OUTPUT_HIGH,
		.gpio_mode = GPIO,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
#endif
#endif		  
	

};






static void bsp_gpiote_output_low_callback(struct BSP_GPIO_PUT*dev);
static void bsp_gpiote_output_high_callback(struct BSP_GPIO_PUT *dev);
static void bsp_gpiote_output_toggle_callback(struct BSP_GPIO_PUT *dev);

static void (*bsp_gpiote_output_callback[GPIO_CTRL_OUTPUT_MODE_NUM])(struct BSP_GPIO_PUT *dev) = {
																								bsp_gpiote_output_low_callback,
																								bsp_gpiote_output_high_callback,
																								bsp_gpiote_output_toggle_callback,
																							};

static void bsp_gpio_output_low_callback(struct BSP_GPIO_PUT*dev);
static void bsp_gpio_output_high_callback(struct BSP_GPIO_PUT *dev);
static void bsp_gpio_output_toggle_callback(struct BSP_GPIO_PUT *dev);

static void (*bsp_gpio_output_callback[GPIO_CTRL_OUTPUT_MODE_NUM])(struct BSP_GPIO_PUT *dev) = {
																							bsp_gpio_output_low_callback,
																							bsp_gpio_output_high_callback,
																							bsp_gpio_output_toggle_callback,
																						};	

static void bsp_gpiote_output_open_callback(struct BSP_GPIO_PUT *dev);
static void bsp_gpio_output_open_callback(struct BSP_GPIO_PUT *dev);
																																													
static void (*bsp_gpio_open_callback[GPIO_MODE_NUM])(struct BSP_GPIO_PUT *dev) = {
																					bsp_gpiote_output_open_callback,
	                                                                                 bsp_gpio_output_open_callback,
																																								 };	

static void bsp_gpiote_output_close_callback(struct BSP_GPIO_PUT *dev);
static void bsp_gpio_output_close_callback(struct BSP_GPIO_PUT *dev);
																																													
static void (*bsp_gpio_close_callback[GPIO_MODE_NUM])(struct BSP_GPIO_PUT *dev) = {
																					 bsp_gpiote_output_close_callback,
	                                                                                 bsp_gpio_output_close_callback,
																					};																																								 
																																								 
																																													
/*******************************************************************************
 * Function Name     : bsp_gpiote_output_low_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_gpiote_output_low_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_drv_gpiote_out_clear(dev->bsp_io_pin);
}	

/*******************************************************************************
 * Function Name     : bsp_gpiote_output_high_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_gpiote_output_high_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_drv_gpiote_out_set(dev->bsp_io_pin);
}

/*******************************************************************************
 * Function Name     : bsp_gpiote_output_toggle_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_gpiote_output_toggle_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_drv_gpiote_out_toggle(dev->bsp_io_pin);
}


/*******************************************************************************
 * Function Name     : bsp_gpiote_output_open_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_gpiote_output_open_callback(struct BSP_GPIO_PUT *dev)
{
	ret_code_t err_code;
	if(!nrf_drv_gpiote_is_init())
	{
		err_code = nrf_drv_gpiote_init();
		APP_ERROR_CHECK(err_code);
	}
	err_code = nrf_drv_gpiote_out_init(dev->bsp_io_pin, &dev->config);
	APP_ERROR_CHECK(err_code);
	nrf_drv_gpiote_out_task_enable(dev->bsp_io_pin);	
	
	if(dev->pull == GPIO_OUTPUT_LOW)
	{
		nrf_drv_gpiote_out_clear(dev->bsp_io_pin);
	}
	else
	{
		nrf_drv_gpiote_out_set(dev->bsp_io_pin);
	}
}

/*******************************************************************************
 * Function Name     : bsp_gpiote_output_close_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_gpiote_output_close_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_drv_gpiote_out_task_disable(dev->bsp_io_pin);
	nrfx_gpiote_out_uninit(dev->bsp_io_pin);
//	nrf_gpio_cfg_input(dev->bsp_io_pin, NRF_GPIO_PIN_NOPULL);
}


/*******************************************************************************
 * Function Name     : bsp_gpio_output_low_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_gpio_output_low_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_gpio_pin_clear(dev->bsp_io_pin);
}	

/*******************************************************************************
 * Function Name     : bsp_gpio_output_high_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_gpio_output_high_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_gpio_pin_set(dev->bsp_io_pin);
    //BC_LOG_INFO("bsp_gpio_output_high_callback******bsp_io_pin:%d\r\n",dev->bsp_io_pin);
//	nrf_gpio_cfg(
//            dev->bsp_io_pin,
//            NRF_GPIO_PIN_DIR_OUTPUT,
//            NRF_GPIO_PIN_INPUT_DISCONNECT,
//            NRF_GPIO_PIN_NOPULL,
//            NRF_GPIO_PIN_H0H1,
//            NRF_GPIO_PIN_NOSENSE);
}

/*******************************************************************************
 * Function Name     : bsp_gpio_output_toggle_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_gpio_output_toggle_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_gpio_pin_toggle(dev->bsp_io_pin);
}



/*******************************************************************************
 * Function Name     : bsp_gpio_output_open_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_gpio_output_open_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_gpio_cfg_output(dev->bsp_io_pin);
	if(dev->pull == GPIO_OUTPUT_LOW)
	{
		nrf_gpio_pin_clear(dev->bsp_io_pin);

	}
	else
	{
		nrf_gpio_pin_set(dev->bsp_io_pin);

	}
}

/*******************************************************************************
 * Function Name     : bsp_gpio_output_close_callback
 * Description       : gpio输出回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_gpio_output_close_callback(struct BSP_GPIO_PUT *dev)
{
	nrf_gpio_cfg_default(dev->bsp_io_pin);
//	nrf_gpio_cfg_input(dev->bsp_io_pin, NRF_GPIO_PIN_NOPULL);
}


/*******************************************************************************
 * Function Name     : bsp_gpio_output_open
 * Description       : gpio out put open
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_gpio_output_open(q_device_t*dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].io_lock)
			{
				return RESULT_OK;
			}
			if(bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_enable && bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_take != NULL)
			{
				bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_take();
				bsp_gpio_open_callback[bsp_list[i].gpio_mode](&bsp_list[i]);

			}
			else
			{
				bsp_gpio_open_callback[bsp_list[i].gpio_mode](&bsp_list[i]);
			}
			bsp_list[i].io_lock = true;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
}

/*******************************************************************************
 * Function Name     : bsp_gpio_output_close
 * Description       : gpio out put close
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_gpio_output_close(q_device_t *dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].io_lock)
			{
				return RESULT_OK;
			}
			if(bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_enable && bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_give != NULL)
			{
				bsp_gpio_close_callback[bsp_list[i].gpio_mode](&bsp_list[i]);
				bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_give();
			}
			else
			{
				bsp_gpio_close_callback[bsp_list[i].gpio_mode](&bsp_list[i]);			
			}
			bsp_list[i].io_lock = false;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
}

/*******************************************************************************
 * Function Name     : bsp_gpio_output_ctrl
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_gpio_output_ctrl(q_device_t *dev, int cmd, void *args)
{
    if( cmd >= GPIO_CTRL_OUTPUT_MODE_NUM)
	{
	   return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;
	}
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].io_lock)
			{
				return RESULT_DEV_UNOPENED_ERR;
			}
			if(bsp_list[i].gpio_mode == GPIOTE)
			{
				bsp_gpiote_output_callback[cmd](&bsp_list[i]);
			}
			else
			{
				bsp_gpio_output_callback[cmd](&bsp_list[i]);
				//Q_DEVICE_LOG_INFO("cmd:%d  io:%d \r\n",cmd,i);
			}
	
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;
}



/*******************************************************************************
 * Function Name     : bsp_gpio_output_read
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_gpio_output_read(q_device_t *dev, int pos,const void *buffer, int size)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].io_lock)
			{
				return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;
			}
			*(uint8_t*)buffer = nrf_gpio_pin_read(bsp_list[i].bsp_io_pin);
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
}

static int bsp_gpio_output_config(q_device_t *dev, void *args, void *var)
{
	struct bsp_gpio_mutex_lock *cfg = (struct bsp_gpio_mutex_lock *)args;
	if(cfg == NULL)
	{
		return RESULT_GPIO_CONFIG_NULL_ERR;
	}	
	
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			
			bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_enable = cfg->gpio_mutex_lock_enable;
			bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_take = cfg->gpio_mutex_lock_take;
			bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_give = cfg->gpio_mutex_lock_give;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;
}




#endif


#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)


#include "gpio.h"

struct  BSP_GPIO_PUT
{
	const char   *name;
    bool          register_flag;
	gpio_pin_e      bsp_io_pin;
	struct bsp_gpio_mutex_lock  gpio_mutex_lock;
	q_device_t dev;
};

static struct BSP_GPIO_PUT bsp_list[] = 
{
	{
		.name = "led",
		.register_flag = false,
		.bsp_io_pin = (gpio_pin_e)21,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ppg_afe_rstz",
		.register_flag = false,
		.bsp_io_pin = GPIO_P17,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
	{
		.name = "ppg_afe_pdnz",
		.register_flag = false,
		.bsp_io_pin = GPIO_P16,
		.gpio_mutex_lock = {0},
		.dev = {0},
	},
};



/*******************************************************************************
 * Function Name     : bsp_gpio_output_ctrl
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_gpio_output_ctrl(q_device_t *dev, int cmd, void *args)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if (bsp_list[i].bsp_io_pin > (NUMBER_OF_PINS - 1))
			{
				Q_DEVICE_LOG_ERROR("gpio:%d  number error\r\n",bsp_list[i].bsp_io_pin);
				return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;
			}
			switch(cmd)
			{
				case GPIO_OUTPUT_LOW:
				{
					Q_DEVICE_LOG_INFO("gpio:%d output low \r\n",bsp_list[i].bsp_io_pin);
					AP_GPIO->swporta_dr &= ~BIT(bsp_list[i].bsp_io_pin);
					hal_gpio_pin_init(bsp_list[i].bsp_io_pin,GPIO_OUTPUT);
					break;
				}
				case GPIO_OUTPUT_HIGH:
				{
					Q_DEVICE_LOG_INFO("gpio:%d output high \r\n",bsp_list[i].bsp_io_pin);
					AP_GPIO->swporta_dr |= BIT(bsp_list[i].bsp_io_pin);
					hal_gpio_pin_init(bsp_list[i].bsp_io_pin,GPIO_OUTPUT);
					break;
				}
				case GPIO_OUTPUT_TOGGLE:
				{
					if(hal_gpio_read(bsp_list[i].bsp_io_pin))
					{
						Q_DEVICE_LOG_INFO("gpio:%d output low \r\n",bsp_list[i].bsp_io_pin);
						AP_GPIO->swporta_dr &= ~BIT(bsp_list[i].bsp_io_pin);
					}
					else
					{
						Q_DEVICE_LOG_INFO("gpio:%d output high \r\n",bsp_list[i].bsp_io_pin);
						AP_GPIO->swporta_dr |= BIT(bsp_list[i].bsp_io_pin);
					}
					hal_gpio_pin_init(bsp_list[i].bsp_io_pin,GPIO_OUTPUT);
					break;
				}
				case GPIO_FLOAT:
				{
					Q_DEVICE_LOG_INFO("gpio:%d set floating\r\n",bsp_list[i].bsp_io_pin);
					hal_gpio_pull_set(bsp_list[i].bsp_io_pin,GPIO_FLOATING);
					break;
				}
				case GPIO_UP_S:
				{
					Q_DEVICE_LOG_INFO("gpio:%d set pull up s \r\n",bsp_list[i].bsp_io_pin);
					hal_gpio_pull_set(bsp_list[i].bsp_io_pin,GPIO_PULL_UP_S);
					break;
				}
				case GPIO_UP:
				{
					Q_DEVICE_LOG_INFO("gpio:%d set pull up \r\n",bsp_list[i].bsp_io_pin);
					hal_gpio_pull_set(bsp_list[i].bsp_io_pin,GPIO_PULL_UP);
					break;
				}
				case GPIO_DOWN:
				{
					Q_DEVICE_LOG_INFO("gpio:%d set pull down \r\n",bsp_list[i].bsp_io_pin);
					hal_gpio_pull_set(bsp_list[i].bsp_io_pin,GPIO_PULL_DOWN);
					break;
				}
				case GPIO_REGISTER:
				{
					Q_DEVICE_LOG_INFO("gpio:%d set register \r\n",bsp_list[i].bsp_io_pin);
					bsp_list[i].register_flag = true;
					hal_gpioretention_register(bsp_list[i].bsp_io_pin);
					break;
				}
				case GPIO_UNREGISTER:
				{
					Q_DEVICE_LOG_INFO("gpio:%d set unregister \r\n",bsp_list[i].bsp_io_pin);
					bsp_list[i].register_flag = false;
					hal_gpioretention_unregister(bsp_list[i].bsp_io_pin);
					break;
				}
				default:
				{
					Q_DEVICE_LOG_INFO("gpio:%d  invalid command \r\n",bsp_list[i].bsp_io_pin);
					break;
				}
			}
		
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;
}

static int bsp_gpio_output_config(q_device_t *dev, void *args, void *var)
{
	struct bsp_gpio_mutex_lock *cfg = (struct bsp_gpio_mutex_lock *)args;
	if(cfg == NULL)
	{
		return RESULT_GPIO_CONFIG_NULL_ERR;
	}	
	
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			
			bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_enable = cfg->gpio_mutex_lock_enable;
			bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_take = cfg->gpio_mutex_lock_take;
			bsp_list[i].gpio_mutex_lock.gpio_mutex_lock_give = cfg->gpio_mutex_lock_give;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;
}

/*******************************************************************************
 * Function Name     : bsp_gpio_output_close
 * Description       : gpio out put close
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_gpio_output_close(q_device_t *dev)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].register_flag)
			{
				hal_gpioretention_unregister(bsp_list[i].bsp_io_pin);
			}
			hal_gpio_pin_init(bsp_list[i].bsp_io_pin,GPIO_INPUT);
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
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
	.control = bsp_gpio_output_ctrl,
	.config = bsp_gpio_output_config,
	.close = bsp_gpio_output_close,
	
#if (HARDWARE_ARCH_TYPE_NORDIC == 1)	
	.read = bsp_gpio_output_read,
	.open = bsp_gpio_output_open,
	
#endif
};

/*******************************************************************************
 * Function Name     : bsp_gpio_output_register
 * Description       : 设备注册
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void bsp_gpio_output_register(void)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		bsp_list[i].dev.name = bsp_list[i].name;
		bsp_list[i].dev.dops  = &ops;
		q_device_register(&bsp_list[i].dev);		
	}
}

device_initcall(bsp_gpio_output_register);
