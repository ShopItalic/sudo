#include "app_nfc_handler.h"

#include "bc_timer.h"
#include "bc_event.h"

#include "bc_nfc_port.h"

#include "bc_logger.h"
#include "demo.h"

static bool app_nfc_spi_lock = false;
static bool app_nfc_lock = false;

static void app_nrf_poll_timer_callback(void *age);
static void app_nrf_start_timer_callback(void *age);

static bc_timer_struct  app_nfc_timer[APP_NFC_TIMER_TYPE_NUM] = {
	{
		.timer_name = "nfc poll timer",                          //定时器名字
		.uxAutoReload = true,                                    //周期定时器
		.xTimerPeriodInTicks = 100,                             //定时器时间
		.timer_callback_function = app_nrf_poll_timer_callback,  //定时器回调
   },
	{
		.timer_name = "nfc starttimer",                          //定时器名字
		.uxAutoReload = false,                                    //周期定时器
		.xTimerPeriodInTicks = 50,                             //定时器时间
		.timer_callback_function = app_nrf_start_timer_callback,  //定时器回调
   },
};

static void app_nfc_monitor_event_callback(void * p_context);
static void app_nfc_iqr_handler_event_callback(void * p_context);

static bc_event_struct app_nfc_event[APP_NFC_EVENT_TYPE_NUM]={
	{
		.event_name = "nfc monitor event",
		.event_callback_function = app_nfc_monitor_event_callback,
	},
	{
		.event_name = "nfc irq event",
		.event_callback_function = app_nfc_iqr_handler_event_callback,
	}
};


static void app_nfc_device_init(void)
{
	bc_nfc_open_spi();
	if( !demoIni() )
	{
		BC_LOG_WARN("nfc init fial!! \r\n"); 
		app_nfc_spi_lock = true;
	}
	else
	{
		BC_LOG_WARN("nfc init ok!! \r\n"); 
	}
	bc_nfc_close_spi();
}


static void app_nfc_monitor_event_callback(void * p_context)
{
//	BC_LOG_INFO("nfc poll event \r\n");
		if(!app_nfc_spi_lock)
		{
			app_nfc_spi_lock = true;
			bc_nfc_open_spi();
			demoCycle();
			bc_nfc_close_spi();
			app_nfc_spi_lock = false;
		}

}

static void app_nfc_iqr_handler_event_callback(void * p_context)
{
	BC_LOG_INFO("irq event \r\n");

	if(!app_nfc_spi_lock)
	{
		app_nfc_spi_lock = true;
		bc_nfc_open_spi();
		bc_nfc_irq_state_handler();
		bc_nfc_close_spi();
		app_nfc_spi_lock = false;
		bc_event_set(&app_nfc_event[APP_NFC_MONITOR_EVENT]);                        //发送事件
	}
}

static void app_nrf_poll_timer_callback(void *age)
{
	bc_event_set(&app_nfc_event[APP_NFC_MONITOR_EVENT]);                        //发送事件
}

static void app_nrf_start_timer_callback(void *age)
{
//	bc_nfc_open_spi();
	app_nfc_device_init();
	bc_timer_start(&app_nfc_timer[APP_NFC_POLL_MONITOR_TIMER]);                                 //启动定时器
}

/*******************************************************************************
 * Function Name     : key_lo_to_hi_irq_callback
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void app_nfc_exit_lo_to_hi_irq_callback(uint8_t pin,uint8_t status)
{
	BC_LOG_INFO("lo to hi  key:%d  ststus:%d \r\n",pin,status);
	bc_event_set(&app_nfc_event[APP_NFC_IO_IRQ_HANDLER_EVENT]); 
}


/*******************************************************************************
 * Function Name     : key_hi_to_lo_irq_callback
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void app_nfc_exit_hi_to_lo_irq_callback(uint8_t pin,uint8_t status)
{
	BC_LOG_INFO("hi to lo  key:%d  ststus:%d \r\n",pin,status);
//	st25r3916Isr();
}

static void app_nfc_timer_create(void)
{
	for(uint8_t i = 0; i < APP_NFC_TIMER_TYPE_NUM;i++)
	{
		if(!bc_timer_create(&app_nfc_timer[i]))                                //创建定时器
		{
			BC_LOG_WARN("create %s fial!! \r\n",app_nfc_timer[i].timer_name);              
		}
		else
		{
			BC_LOG_INFO("create %s success!! \r\n",app_nfc_timer[i].timer_name);
		}
	}
	bc_timer_start(&app_nfc_timer[APP_NFC_START_TIMER]);                                 //启动定时器
	
}

static void app_nfc_event_create(void)
{
	for(uint8_t i = 0; i < APP_NFC_EVENT_TYPE_NUM;i++)
	{
		if(!bc_event_create(&app_nfc_event[i]))                                //创建事件
		{
			BC_LOG_WARN("create %s fial!! \r\n",app_nfc_event[i].event_name);
		}
		else
		{
			BC_LOG_INFO("create %s success!! \r\n",app_nfc_event[i].event_name);
		}
	}
}

void app_nfc_resoure_init(void)
{
	bc_nfc_io_irq_reg_callback(app_nfc_exit_lo_to_hi_irq_callback,app_nfc_exit_hi_to_lo_irq_callback);
	app_nfc_event_create();
	app_nfc_timer_create();
//	app_nfc_device_init();
}






