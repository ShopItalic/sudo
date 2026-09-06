#ifndef __Q_DEVICE_H__
#define __Q_DEVICE_H__


#include "q_init.h"

#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>


#include "bc_logger.h"

#include "ring_config.h"

#define Q_DEVICE_VERSION        "V0.0.1"

#if defined(FML_FREERTOS)
#include "fml_freertos.h"
#endif

#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)

#include "clock.h"
#endif

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)

#include "nrf_delay.h"

#endif

#define Q_DEVICE_LOG   1
          
#define array_size(array) (sizeof(array)/sizeof(*array))
	
#define    disable_irq()  __disable_irq()
#define    enable_irq()    __enable_irq()   



#if defined(FML_FREERTOS)
#define   q_device_delay_ms(ms)    fml_rtos_delay(ms)
#endif

#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)
#define   q_device_delay_ms(ms)    WaitMs(ms)
#endif

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)
#define   q_device_delay_ms(ms)    nrf_delay_ms(ms)
#define   q_device_delay_us(us)    nrf_delay_us(us)
#endif


#if Q_DEVICE_LOG 


#define Q_DEVICE_LOG_INFO(format, ...)       printf("\r\n[Q_DEVICE_INFO %s(%d)] " format, __MODULE__, __LINE__, ##__VA_ARGS__); 	                
#define Q_DEVICE_LOG_DEBUG(format, ...)      printf("\r\n[Q_DEVICE_DEBUG %s:%d]: "format,__MODULE__, __LINE__, ##__VA_ARGS__);                  
#define Q_DEVICE_LOG_ERROR(format, ...)      printf("\r\n[Q_DEVICE_ERROR %s:%d]: "format, __MODULE__, __LINE__, ##__VA_ARGS__);            
#define Q_DEVICE_LOG_WARN(format, ...)       printf("\r\n[Q_DEVICE_WARN %s:%d]: "format, __MODULE__, __LINE__, ##__VA_ARGS__);	

#define Q_DEVICE_LOG_PRINTF(format, ...)       			printf(format,##__VA_ARGS__); 
												 
#define Q_DEVICE_LOG_HEX(chars,data,length)               printf("%s",chars); \
												    for(uint32_t i = 0; i < length; i++) \
												    { \
													   printf("0x%02x ",data[i]); \
												    }; \
												    printf("\r\n");

#else

#define Q_DEVICE_LOG_INFO(...)
#define Q_DEVICE_LOG_DEBUG( ...)
#define Q_DEVICE_LOG_ERROR( ...) 
#define Q_DEVICE_LOG_WARN(...) 
#define Q_DEVICE_LOG_PRINTF(...)
#define Q_DEVICE_LOG_HEX(chars,data,length)              Q_DEVICE_LOG_PRINTF("%s,%d,%d",chars,data[0],length) ;

#endif


#define q_device_assert(p) do { \
										if (!(p)) { \
											Q_DEVICE_LOG_ERROR("BUG at assert\n"); \
										}       \
								 } while (0)   
//											
								 

typedef void (*q_device_mutex_lock_create)(void); 
typedef void (*q_device_mutex_lock_delete)(void); 
typedef void (*q_device_mutex_lock_take)(void); 
typedef void (*q_device_mutex_lock_give)(void); 

								 
enum result_state
{
	RESULT_OK = 0,
	RESULT_GPIO_OUTPUT_DEV_NULL_ERR,
	RESULT_GPIO_INPUT_DEV_NULL_ERR,
	RESULT_UART_DEV_NULL_ERR,
	RESULT_UART_CONFIG_NULL_ERR,
	RESULT_UART_SEND_ERR,
	RESULT_UART_OPEN_ERR,
	RESULT_UART_CLOSE_ERR,
	RESULT_I2C_DEV_NULL_ERR,
	RESULT_I2C_CONFIG_NULL_ERR,
	RESULT_I2C_DEV_UNOPENED_ERR,
	RESULT_I2C_SEND_ERR,
	RESULT_I2C_READ_ERR,
	RESULT_I2C_OPEN_ERR,
	RESULT_I2C_CLOSE_ERR,	
	RESULT_RTC_OPEN_ERR,
	RESULT_RTC_CLOSE_ERR,	
	RESULT_RTC_DEV_NULL_ERR,
	RESULT_DOG_DEV_NULL_ERR,
	RESULT_DOG_DEV_UNOPENED_ERR,
	RESULT_FLASH_DEV_NULL_ERR,
	RESULT_FLASH_WRITE_ERR,
	RESULT_FLASH_READ_ERR,
	RESULT_SPI_SEND_ERR,
	RESULT_OPEN_ERR,
	
	RESULT_INVALID_COMMAND_ERR,
	RESULT_POINTER_NULL_ERR,
	RESULT_DEV_UNOPENED_ERR,
	RESULT_DEV_UNIMITIALIZED_ERR,
	RESULT_DEV_NULL_ERR,
	RESULT_SEND_ERR,
	RESULT_READ_ERR,
};


enum DEV_sleep
{
	  DEV_SLEEP= 0x00,
    DEV_DEEP_SLEEP ,
    DEV_STOP,
    DEV_STANDBY,
	
};

/**********   uart  ************/


typedef enum
{
	UART_1_SERIAL = 0,
	UART_3_SERIAL,
	UART_4_SERIAL,
	UART_5_SERIAL,
	UART_SERAIL_NUM
}serial_com;

enum serial_baud_rate_type
{
	UART_BAUD_RATE_2400 = 0,
	UART_BAUD_RATE_4800,
	UART_BAUD_RATE_9600,
	UART_BAUD_RATE_14400,
	UART_BAUD_RATE_19200,
	UART_BAUD_RATE_57600,
	UART_BAUD_RATE_115200,
	UART_BAUD_RATE_128000,
	UART_BAUD_RATE_256000,
	UART_BAUD_RATE_500000,
	UART_BAUD_RATE_1000000,
	UART_BAUD_RATE_NUM
};

extern uint32_t serial_baud_rate[UART_BAUD_RATE_NUM];


/***********  urt   **********/
typedef struct
{
	uint8_t *uart_data_buff;
	uint16_t uart_data_leng;
	serial_com uart_serial_port;
}serial_uart_package;

struct serial_configure
{
   enum serial_baud_rate_type baud_rate;
};

/***********  i2c   **********/
struct i2c_package
{
    uint8_t slave_addr;
    uint16_t reg_addr;
    uint8_t *write_buff;
    uint16_t write_length;
    uint8_t *read_buff;
    uint16_t read_length;
};

/***********  stacmd   **********/
struct stacmd_package
{
	uint8_t reg_addr;
	uint8_t *write_buff;
	uint8_t write_length;
	uint8_t *read_buff;
	uint8_t read_length;
};

/***********  spi   **********/
struct bsp_spi_mutex_lock         //spi 互斥锁
{
	bool spi_mutex_lock_enable;     //true:使能   false:失能
	q_device_mutex_lock_take spi_mutex_lock_take;
	q_device_mutex_lock_give spi_mutex_lock_give;
};

struct spi_package
{
	uint8_t *write_buff;
	uint8_t write_length;
	uint8_t *read_buff;
	uint8_t read_length;
};


/***********  gpio   **********/
typedef enum                 //io 控制输出模式
{
	GPIO_OUTPUT_LOW = 0,
	GPIO_OUTPUT_HIGH,
	GPIO_OUTPUT_TOGGLE,
	GPIO_CTRL_OUTPUT_MODE_NUM,
#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)	
	GPIO_FLOAT,         //pull 
	GPIO_UP_S,              //pull 
	GPIO_UP,                //pull 
	GPIO_DOWN,              //pull 
	GPIO_REGISTER,
	GPIO_UNREGISTER,
#endif	
	
}gpio_ctrl_cmd;


typedef enum 
{
  GPIOT_CONFIG_POLARITY_LoToHi = 0,       ///<  Low to high.
  GPIOT_CONFIG_POLARITY_HiToLo,           ///<  High to low.
}gpiot_polarity;  //边沿检测


struct bsp_gpio_mutex_lock         //io 互斥锁
{
	bool gpio_mutex_lock_enable;     //true:使能   false:失能
	q_device_mutex_lock_take gpio_mutex_lock_take;
	q_device_mutex_lock_give gpio_mutex_lock_give;
};


/**********   rtc  ************/

struct rtc_time
{
	struct tm bj_time;
	time_t unix_time;
  uint64_t unix_ms_time;
};

/**********   watchdog  ************/
enum bsp_wdt_cmd
{
	WDT_FEED_DOG = 0,
};

/**********   sys  ************/
enum sys_cmd_type
{
	SYS_RESET_REASON_GET = 1,
	SYS_REBOOT,
	SYS_DEVICE_MAC_GET,
	
};

/**********   pdm  ************/
struct pdm_data_config
{
	int16_t *pdm_data_buff;
	int16_t pdm_data_buff_legth;
	void *pdm_data_callback;
};

/**********   flash  ************/

enum falsh_cmd
{
	ERASE_FLASH = 0,
	READ_FLASH_CONFIG,
};

struct flash_write_package
{
	const uint8_t *data;
	uint32_t offset;
	uint32_t data_length;
};

struct flash_read_package
{
	uint8_t *data;
	uint32_t offset;
	uint32_t data_length;
};



struct flash_config
{
	uint8_t page_num;
	uint32_t strat_addr;
	uint32_t en_addr;
	uint32_t page_size;
};

struct flash_mutex_lock         // 互斥锁
{
	bool mutex_lock_enable;     //true:使能   false:失能
	q_device_mutex_lock_take mutex_lock_take;
	q_device_mutex_lock_give mutex_lock_give;
};

/**********   pwm  ************/

struct pwm_parameter
{
	uint16_t top_value;
	uint16_t *p_common;
	uint16_t length;
	uint32_t repeats;
	uint16_t playback_count;
    uint32_t flags;
};

struct pwm_config
{
	bool pwm_aisle0_enable_status;
	bool pwm_aisle1_enable_status;
	bool pwm_aisle2_enable_status;
	bool pwm_aisle3_enable_status;
	struct pwm_parameter pwm_parameter_config;
};

enum pwm_register_callback
{
	PWM_REGISTER_FINISHED_CALLBACK = 0,
	PWM_REGISTER_END_SEQ0_CALLBACK,
	PWM_REGISTER_END_SEQ1_CALLBACK,
	PWM_REGISTER_STOPPED_CALLBACK,
};

enum pwm_flag
{
    PWM_FLAG_STOP = 0x01, /**< When the requested playback is finished,
                                    the peripheral will be stopped.
                                    @note The STOP task is triggered when
                                    the last value of the final sequence is
                                    loaded from RAM, and the peripheral stops
                                    at the end of the current PWM period.
                                    For sequences with configured repeating
                                    of duty cycle values, this might result in
                                    less than the requested number of repeats
                                    of the last value. */
    PWM_FLAG_LOOP = 0x02, /**< When the requested playback is finished,
                                    it will be started from the beginning.
                                    This flag is ignored if used together
                                    with @ref NRFX_PWM_FLAG_STOP.
                                    @note The playback restart is done via a
                                    shortcut configured in the PWM peripheral.
                                    This shortcut triggers the proper starting
                                    task when the final value of previous
                                    playback is read from RAM and applied to
                                    the pulse generator counter.
                                    When this mechanism is used together with
                                    the @ref NRF_PWM_STEP_TRIGGERED mode,
                                    the playback restart will occur right
                                    after switching to the final value (this
                                    final value will be played only once). */
    PWM_FLAG_SIGNAL_END_SEQ0 = 0x04, /**< The event handler is to be
                                               called when the last value
                                               from sequence 0 is loaded. */
    PWM_FLAG_SIGNAL_END_SEQ1 = 0x08, /**< The event handler is to be
                                               called when the last value
                                               from sequence 1 is loaded. */
    PWM_FLAG_NO_EVT_FINISHED = 0x10, /**< The playback finished event
                                               (enabled by default) is to be
                                               suppressed. */
    PWM_FLAG_START_VIA_TASK = 0x80, /**< The playback must not be
                                              started directly by the called
                                              function. Instead, the function
                                              must only prepare it and
                                              return the address of the task
                                              to be triggered to start the
                                              playback. */
};

enum pwm_ctrl
{
	PWM_CTRL_START = 0,
	PWM_CTRL_STOP,
};

enum pwm_status
{
	PWM_IDIE = 0,
	PWM_BUSY,
};

/*********  q_device    ***********/

enum q_device_result
{
	RESULT_Q_DEVICE_OK = 0,
	RESULT_DEV_POINTER_NULL_ERROR,
	RESULT_READ_POINTER_NULL_ERROR,
	RESULT_WRITE_POINTER_NULL_ERROR,
	RESULT_INIT_POINTER_NULL_ERROR,
	RESULT_OPEN_POINTER_NULL_ERROR,
	RESULT_CLOSE_POINTER_NULL_ERROR,
	RESULT_CONTROL_POINTER_NULL_ERROR,
	RESULT_CONFIG_POINTER_NULL_ERROR,
	RESULT_REG_CALLBACK_POINTER_NULL_ERROR,
	RESULT_GPIO_CONFIG_NULL_ERR,
	RESULT_CONFIG_NULL_ERR,
};

typedef struct q_device  q_device_t;

struct q_device_ops
{
	int  (*init)   (q_device_t *dev);                                             //设备初始化函数指针，用于注册设备初始化函数
	int  (*uninit) (q_device_t *dev);                                             //反初始化设备函数指针，用于注册卸载设备初始化函数
	int  (*open)   (q_device_t *dev);                                             //打开设备函数指针，用于注册打开设备函数
	int  (*close)  (q_device_t *dev);                                             //关闭设备函数指针，用于注册关闭设备函数
	int  (*read)   (q_device_t *dev, int pos, const void *  buffer, int size);    //读设备函数指针，用于注册读设备函数
	int  (*write)  (q_device_t *dev, int pos, const void *buffer, int size);      //写设备函数指针，用于注册写设备函数
	int  (*control)(q_device_t *dev, int cmd, void *args);                        //控制设备函数指针，用于注册控制设备函数
	int  (*config) (q_device_t *dev, void *args, void *var);                      //配置设备函数指针，用于注册配置设备函数
	int  (*register_callback) (q_device_t *dev, int pos,void *callback);          //注册回调函数设备函数指针，用于注册设备回调函数
};

struct q_device
{
    const char * name;
    const struct q_device_ops *dops;
    void   *owner;
    void   *argv;
    int    data;
    struct q_device *next;
};



/*
    驱动注册
*/
int q_device_register(q_device_t *dev);
/*
    驱动查找
*/
q_device_t *q_device_find(const char *name);

/*
    驱动初始化
*/
int q_device_init(q_device_t *dev);

/*
    驱动释放
*/
int q_device_uninit(q_device_t *dev);
/*
    驱动打开
*/
int q_device_open(q_device_t *dev);
/*
    驱动关闭
*/
int q_device_close(q_device_t *dev);
/*
    驱动读
*/
int q_device_read(q_device_t *dev,  int pos,const void * buffer, int size);
/*
    驱动写
*/
int q_device_write(q_device_t *dev, int pos,const void *buffer, int size);

/*
    驱动控制
*/
int q_device_ctrl(q_device_t *dev,  int cmd, void *arg);
/*
    驱动配置
*/
int q_device_cfg(q_device_t *dev, void *args, void *var);

/*
    驱动注册回调函数
*/
int q_device_reg_callback(q_device_t *dev,  int pos, void *callback);

/*
    设置驱动属于哪个任务
*/
void q_device_set_owner(q_device_t *dev, const void *owner);





#endif
