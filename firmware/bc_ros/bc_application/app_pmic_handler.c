#include "app_pmic_handler.h"

#include "bc_pmic_device_port.h"
#include "app_ppg_handler.h"
#include "app_ppg.h"
#include "app_model_handler.h"
#include "app_package.h"
#include "app_touch_button_handler.h"
#include "bc_device_info.h"
#include "app_rtc_handler.h"

#include "bc_pmic.h"
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1  || HARDWARE_441_ENABLED == 1 || HARDWARE_402_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || \
HARDWARE_1181_ENABLED == 1)	

#include "bc_led.h"
#elif (HARDWARE_153_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1)					
//#include "bc_ic_led.h"
//#include "app_pdm_handler.h"
#endif

#if ( HARDWARE_1191_ENABLED == 1)	

#include "bc_led_pwm.h"

#endif	

#if defined(HANDWARE_1_23_2_ONE_SEC)
#include "bc_ic_led.h"
#endif

#if (defined(HANDWARE_1_23_3 ) || defined(HANDWARE_1_23_4 ))

#include "bc_linear_motor_ic.h"
#endif

#if defined(HANDWARE_1_23_4)
#include "bc_ic_led.h"
#endif


#include "bc_delay.h"
#include "bc_logger.h"
#include "bc_rtos.h"
#include "bc_rtc.h"

#if defined(SUDO_VOICE_ONLY)
#include "bc_battery_filter.h"
#else
#define LEN_PRECENT_ARRY        10

typedef struct {
    uint8_t aprecent[LEN_PRECENT_ARRY];
    uint8_t allprecent;
    uint8_t cntprecent;
} STR_PRECENT;
#endif

static uint8_t pmic_percent_low_count = 0;

uint8_t precent = 0;
#if defined(SUDO_VOICE_ONLY)
static bc_battery_filter battery_filter;
static enum pmic_charge_status battery_filter_status = PMIC_CHARGED_NOT;
static bool battery_filter_status_initialized = false;
static volatile bool battery_filter_transaction_active = false;
#else
static STR_PRECENT precentval = {0};
#endif

static 	enum pmic_charge_status pmic_state =PMIC_CHARGED_NOT;
static 	enum pmic_charge_status pmic_state_check =PMIC_CHARGED_NOT;

static void app_pmic_handler_timer_callback (void * pvParameter);

static bool led_flag = false;
static bool CHARGED_NOT_flag = false;
#if !defined(SUDO_VOICE_ONLY)
static uint8_t pre_percent = 0;
#endif

static bc_rtos_timer_struct  timer_struct = {
	
		.timer_name = "pmic_handler timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 100,
		.lock = false,
		.timer_callback_function = app_pmic_handler_timer_callback,
	
};

#if defined(SUDO_VOICE_ONLY)
static bool battery_filter_transaction_try_begin(void)
{
    bool acquired;

    /* Claim the complete status -> ADC -> filter transaction before any
     * potentially blocking PMIC or ADC call. */
    bc_rtos_taskENTER_CRITICAL();
    acquired = !battery_filter_transaction_active;
    if(acquired)
        battery_filter_transaction_active = true;
    bc_rtos_taskEXIT_CRITICAL();
    return acquired;
}
#endif

#if defined(HANDWARE_1_23_4)
static uint8_t irq_status_pre = 0;
static uint8_t irq_cnt = 0;
static void app_pmic_irq_timer_callback (void * pvParameter);
static void app_pmic_irq_callback(uint8_t gpio_pin, uint8_t gpio_status);
static bc_rtos_timer_struct  timer_irq = {
        .timer_handler = NULL,
		.timer_name = "pmic_irq timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 300,
		.lock = false,
		.timer_callback_function = app_pmic_irq_timer_callback,
	
};
#endif
/*******************************************************************************
 * Function Name     : app_pmic_init
 * Description       : pmic初始化
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static void app_pmic_init(void)
{
	bc_pmic_init();
    #if defined(HANDWARE_1_23_4)
    pmic_irq_register_callback(app_pmic_irq_callback);
    pmic_io_irq_enable();
    #endif
}


/*******************************************************************************
 * Function Name     : app_pmic_timer_chargeing_time_update
 * Description       : 如果pmic处于充电中那就改为5s查询一次状态
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static void app_pmic_timer_chargeing_time_update(void)
{
	if(timer_struct.xTimerPeriodInTicks != 2*1000)
	{
		bc_rtos_timer_stop(timer_struct.timer_handler,50);
		timer_struct.xTimerPeriodInTicks = 2*1000;
		bc_rtos_timer_change_period(timer_struct.timer_handler, timer_struct.xTimerPeriodInTicks, 50);
		bc_rtos_timer_start(timer_struct.timer_handler,50);
	}
}

/*******************************************************************************
 * Function Name     : app_pmic_timer_not_charge_time_update
 * Description       : 如果pmic处于充电完成那就改为150s查询一次状态
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static void app_pmic_timer_not_charge_time_update(void)
{
	if(timer_struct.xTimerPeriodInTicks != 30*1000)
	{
		bc_rtos_timer_stop(timer_struct.timer_handler,50);
		timer_struct.xTimerPeriodInTicks = 30*1000;
		bc_rtos_timer_change_period(timer_struct.timer_handler, timer_struct.xTimerPeriodInTicks, 50);
		bc_rtos_timer_start(timer_struct.timer_handler,50);
	}
}

/*******************************************************************************
 * Function Name     : app_pmic_timer_not_charge_time_update
 * Description       : 如果pmic处于充电完成那就改为150s查询一次状态
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static void app_pmic_timer_low_power_update(void)
{
	if(timer_struct.xTimerPeriodInTicks != 60*1000)
	{
		bc_rtos_timer_stop(timer_struct.timer_handler,50);
		timer_struct.xTimerPeriodInTicks = 60*1000;
		bc_rtos_timer_change_period(timer_struct.timer_handler, timer_struct.xTimerPeriodInTicks, 50);
		bc_rtos_timer_start(timer_struct.timer_handler,50);
	}
}

#if defined(HANDWARE_1_23_4)
static void app_pmic_irq_callback(uint8_t gpio_pin, uint8_t gpio_action)
{
    //irq_status_pre = pmic_io_irq_status();
//BC_LOG_INFO("pmic case cmd init level:%d \r\n", irq_status_pre);
    //BC_LOG_INFO("gpio_pin : %d, gpio_status : %d***********************\r\n", gpio_pin, gpio_action);
    //services_print_log("gpio_pin : %d, gpio_status : %d\r\n", gpio_pin, irq_status_pre);
    
	if(!timer_irq.timer_handler)
    {
        timer_irq.timer_handler = bc_rtos_timer_create( timer_irq.timer_name,
													   timer_irq.xTimerPeriodInTicks,
													 timer_irq.uxAutoReload, 
													 (void *)timer_irq.timer_id,
													timer_irq.timer_callback_function);
    }
    irq_cnt = 0;
    bc_rtos_timer_reset(timer_irq.timer_handler,50);
}

static void app_pmic_irq_timer_callback(void * pvParameter)
{
	uint8_t irq_status = pmic_io_irq_status();
    if(!irq_cnt) {
        irq_status_pre = irq_status;
    } else {
        if(irq_status_pre == irq_status)
        {
            //services_print_log("app_pmic_irq_timer_callback %d********************\r\n",irq_status);
            if(!irq_status) {
                BC_LOG_INFO("PMIC_CHARGED_ING****************\r\n");
            } else {
                BC_LOG_INFO("iPMIC_CHARGED_NOT****************\r\n");
                /* HANDWARE_1_23_4: 刚拔出充电器，显示电量指示（亮2秒后熄灭） */
                bc_ic_led_battery_indication(precent);
            }
        }
        bc_rtos_timer_stop(timer_irq.timer_handler,50);
        irq_cnt = 0;
    }
    if(irq_cnt < 0xff)
        irq_cnt++;
}
#endif

uint8_t getvpct_(void)
{
#if defined(SUDO_VOICE_ONLY)
    enum pmic_charge_status charge_status;
    enum pmic_charge_status current_status;
    bool charging;
    bool current_charging;
    uint8_t sample;
    uint8_t result;

    if(!battery_filter_transaction_try_begin())
        return BC_BATTERY_PERCENT_UNKNOWN;

    /* The PMIC status getter may access the PMIC bus, so both status reads and
     * the slow ADC transaction stay outside the critical section.  The second
     * read closes the sampling window: a sample taken in an older charge
     * epoch is discarded instead of changing the current filter history. */
    charge_status = bc_pmic_get_charge_status();
    charging = charge_status != PMIC_CHARGED_NOT;
    sample = bc_pmic_get_vbat_percen();
    current_status = bc_pmic_get_charge_status();
    current_charging = current_status != PMIC_CHARGED_NOT;

    bc_rtos_taskENTER_CRITICAL();
    if(!battery_filter_status_initialized ||
       battery_filter_status != current_status)
    {
        bc_battery_filter_reset(&battery_filter, current_charging);
        battery_filter_status = current_status;
        battery_filter_status_initialized = true;
    }
    if(current_status != charge_status)
    {
        result = BC_BATTERY_PERCENT_UNKNOWN;
    }
    else
    {
        result = bc_battery_filter_update(&battery_filter, sample, charging);
    }
    battery_filter_transaction_active = false;
    bc_rtos_taskEXIT_CRITICAL();
    return result;
#else
    uint8_t i = 0, ipval = 0;
    uint32_t imax = 0, imin = 0, iall = 0;
    
    if(precentval.allprecent < LEN_PRECENT_ARRY)
        precentval.allprecent++;
    ipval = bc_pmic_get_vbat_percen();
    if(ipval > 100)
        ipval = 100;
    precentval.aprecent[precentval.cntprecent++] = ipval;
    if(precentval.cntprecent >= LEN_PRECENT_ARRY)
        precentval.cntprecent = 0;
    
    BC_LOG_HEX("precent value: ", precentval.aprecent, precentval.allprecent);
    if(precentval.allprecent < 3) {
        for(uint8_t i=0; i < LEN_PRECENT_ARRY; i++)
        {
            iall += precentval.aprecent[i];
        }
        ipval = (uint8_t)(iall / precentval.allprecent);
        if(ipval > 100)
            ipval = 100;
        return ipval;
    } else {
        for(uint8_t i=0,imax = imin = precentval.aprecent[0]; i < precentval.allprecent; i++)
        {
            if(i < precentval.allprecent - 1) {
                if(precentval.aprecent[i] > precentval.aprecent[i+1]) {
                    if(precentval.aprecent[i] > imax)
                        imax = precentval.aprecent[i];
                }
                if(precentval.aprecent[i] < precentval.aprecent[i+1]) {
                    if(precentval.aprecent[i] < imin)
                        imin = precentval.aprecent[i];
                }
            }
            iall += precentval.aprecent[i];
        } 
        iall = iall - imax - imin;
        ipval = (uint8_t)(iall / (precentval.allprecent-2));
        if(ipval > 100)
            ipval = 100;
        return ipval;
    }
#endif
}

uint8_t getvpct(void)
{
#if defined(SUDO_VOICE_ONLY)
    return getvpct_();
#else

    uint8_t percent = getvpct_();

    if(pre_percent == 0)
        pre_percent = percent;
    
    if(PMIC_CHARGED_NOT == pmic_state) {
        if(percent < pre_percent) {
            pre_percent = percent;
        }
    } else if (PMIC_CHARGED_ING == pmic_state) {
        if(percent > pre_percent) {
            pre_percent = percent;
        }
    }

    return pre_percent;
#endif
}

/*******************************************************************************
 * Function Name     : app_pmic_handler
 * Description       : pmic处理
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static uint32_t temp_count = 0;
static void app_pmic_handler(void)
{
#if (HARDWARE_153_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1)	

//   if(app_ppg_event_state_get() == PPG_IDIE_EVENT  && !app_pdm_work_status())
//	{
//	if(app_ppg_event_state_get() == PPG_IDIE_EVENT  )
//	{
#else
//   if(app_ppg_event_state_get() == PPG_IDIE_EVENT  )
	{
   
#endif	
				
        #if defined(HANDWARE_1_23_4)
		pmic_io_irq_disnable();
        #endif
		pmic_state = bc_pmic_get_charge_status();
        #if defined(HANDWARE_1_23_4)
        pmic_io_irq_enable();
        #endif
        BC_LOG_INFO("pmic_state :%d \r\n",pmic_state );
		switch(pmic_state)
		{
			case PMIC_CHARGED_NOT:                                                        //未充电
			{
#if ( HARDWARE_156_ENABLED == 1 || HARDWARE_158_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1171_ENABLED == 1|| HARDWARE_1231_ENABLED == 1)	

//				temp_count++;
//				if(temp_count >= 60 && !CHARGED_NOT_flag)
//				{
                    //if(precentval.allprecent >= LEN_PRECENT_ARRY)
                        app_pmic_timer_not_charge_time_update();
                    //app_package_precent_up(bc_pmic_get_vbat_percen());
                #if (defined(HANDWARE_1_23_3 ) || defined(HANDWARE_1_23_4 ))
                    if(false == app_cw221x_get_cap(NULL)) {
                        bc_delay_ms(1);
                        if(false == app_cw221x_get_cap(NULL)) {

                        }
                    }
                #else
                    precent = getvpct();
                    app_package_precent_up(precent);
                #endif
//					CHARGED_NOT_flag = true;
//					app_touch_low_power();
//					break;
//					
//				}
#else			
				temp_count++;
				if(temp_count >= 60 && !CHARGED_NOT_flag)
				{
					app_pmic_timer_not_charge_time_update();
					app_package_precent_up(bc_pmic_get_vbat_percen());
					CHARGED_NOT_flag = true;
//					app_touch_low_power();
					break;
					
				}
#endif					

				
				//precent = bc_pmic_get_vbat_percen();
				if(pmic_state_check == PMIC_CHARGED_ING || pmic_state_check == PMIC_CHARGED_OVER)
				{
					//app_package_precent_up(precent);
          app_package_precent_status_up(0);

				}
				
				BC_LOG_INFO("pmic poer precent:%d \r\n",precent);
				if(precent <= 10)
				{
					pmic_percent_low_count++;
					if(pmic_percent_low_count > 3)
					{
            app_rtc_ushut_down_time_record();
                        #if defined(HANDWARE_1_23_4)
                        /* HANDWARE_1_23_4: 低电关机前红灯快闪4次 */
                        bc_ic_led_breathing_start(LED_BREATHING_FAST, 4, 0, 20, 0);
                        bc_delay_ms(2000);  /* 等待2秒，确保红灯闪烁4次完成 */
                        #endif
                        #if defined(HANDWARE_1_23_4)
                        pmic_io_irq_disnable();
                        #endif
						bc_pmic_set_shipmode();
                        #if defined(HANDWARE_1_23_4)
                        pmic_io_irq_enable();
                        #endif
						BC_LOG_INFO("pmic_set_shipmode   %d  \r\n",pmic_percent_low_count);
					}
				}
				else 	
				{
					pmic_percent_low_count = 0;
				}
				if(precent <= 15)
				{
#if defined(HANDWARE_1_23_2_ONE_SEC)
					/* HANDWARE_1_23_2_ONE_SEC: 低电量红色快闪3次后熄灭 */
					if(!led_flag)
					{
						BC_LOG_INFO("HANDWARE_1_23_2_ONE_SEC: low battery (%d%%), red fast flash 3 times\r\n", precent);
						bc_ic_led_breathing_start(LED_BREATHING_FAST, 3, 0, 20, 0);
					}
#endif
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_441_ENABLED == 1 || HARDWARE_402_ENABLED == 1 || HARDWARE_413_ENABLED == 1  || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || \
          HARDWARE_1181_ENABLED == 1)	

						bc_led_low_power_flash_start();
#elif (HARDWARE_153_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1)					
//						bc_ic_led_on(POWER_LOW_ON);
#endif	
					
//					app_pmic_timer_low_power_update();
					led_flag = true;
				}
				else
				{
					if(led_flag == true)
					{
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_441_ENABLED == 1 || HARDWARE_402_ENABLED == 1 || HARDWARE_413_ENABLED == 1  || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || \
            HARDWARE_1181_ENABLED == 1)	

						bc_led_charge_flash_stop();
						bc_led_low_power_flash_stop();
						 bc_led_charge_over_stop();
#elif (HARDWARE_153_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1)					
//						bc_ic_led_off(POWER_LOW_OFF);
//						bc_ic_led_off(CHARGEING_OFF);
//						bc_ic_led_off(CHARGE_OVER_OFF);
#endif	
								

						led_flag = false;
					}
					
				}			
//				if(app_model_state_get() == APP_MODEL_CHARGING_STATE)
//				{
//					app_model_state_set(APP_MODEL_WORKING_STATE);
//				}
				
				break;
			}
			case PMIC_CHARGED_ING:                                                     //充电中
			{
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_441_ENABLED == 1 || HARDWARE_402_ENABLED == 1 || HARDWARE_413_ENABLED == 1  || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || \
        HARDWARE_1181_ENABLED == 1)	
                        bc_led_low_power_flash_stop();
						bc_led_charge_flash_start();
						
				       
#elif (HARDWARE_153_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1)
//						bc_ic_led_off(POWER_LOW_OFF);
//						bc_ic_led_off(CHARGE_OVER_OFF);				
//						bc_ic_led_on(CHARGEING_ON);
						
#endif				

#if ( HARDWARE_1191_ENABLED == 1)	

      bc_led_white_breathe_start(LED_WHITE_BREATHE_4S);

#endif					
                #if (HARDWARE_1231_ENABLED == 1)
                if(pmic_state_check == PMIC_CHARGED_NOT || pmic_state_check == PMIC_CHARGED_OVER)
				{
					//app_package_precent_up(precent);
                    app_package_precent_status_up(1);
				}
                #endif
				led_flag = true;
//				app_model_state_set(APP_MODEL_CHARGING_STATE);
				app_pmic_timer_chargeing_time_update();
                #if (defined(HANDWARE_1_23_3 ) || defined(HANDWARE_1_23_4 ))
                    if(false == app_cw221x_get_cap(NULL)) {
                        bc_delay_ms(1);
                        if(false == app_cw221x_get_cap(NULL)) {

                        }
                    }
                #else
                    precent = getvpct();
                    app_package_precent_up(precent);
                #endif
				break;
			}
			case PMIC_CHARGED_OVER:                                                   //充电完成
			{
//				app_model_state_set(APP_MODEL_WORKING_STATE);
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_441_ENABLED == 1 || HARDWARE_402_ENABLED == 1 || HARDWARE_413_ENABLED == 1  || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || \
     HARDWARE_1181_ENABLED == 1)	
                
				bc_led_charge_flash_stop();
				bc_led_charge_over_start();
				app_pmic_timer_chargeing_time_update();
#elif (HARDWARE_153_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED || HARDWARE_1121_ENABLED == 1 || HARDWARE_158_ENABLED == 1)					
//						bc_ic_led_off(CHARGEING_OFF);
//						bc_ic_led_off(POWER_LOW_OFF);
//						bc_ic_led_on(CHARGE_OVER_ON);
#endif				
						
                #if (HARDWARE_1231_ENABLED == 1)
                if(pmic_state_check == PMIC_CHARGED_NOT || pmic_state_check == PMIC_CHARGED_ING)
				{
					//app_package_precent_up(precent);
                    app_package_precent_status_up(2);
				}
                #endif
				break;
			}
		}
		 pmic_state_check =  pmic_state;
	}
}
/*******************************************************************************
 * Function Name     : app_pmic_handler_timer_callback
 * Description       : pmic定时器回调
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
static void app_pmic_handler_timer_callback(void * pvParameter)
{
	app_pmic_handler();
}

/*******************************************************************************
 * Function Name     : app_pmic_handler_timer_start
 * Description       : pmic定时器启动
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
void app_pmic_handler_timer_start(void)
{
  app_pmic_init();
	bc_rtos_timer_start(timer_struct.timer_handler,50);
	BC_LOG_INFO(" %s start!! \r\n",timer_struct.timer_name);
}
/*******************************************************************************
 * Function Name     : app_pmic_handler_timer_stop
 * Description       : pmic定时器停止
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
void app_pmic_handler_timer_stop(void)
{
	bc_rtos_timer_stop(timer_struct.timer_handler,50);
	BC_LOG_INFO(" %s stop!! \r\n",timer_struct.timer_name);
}

uint8_t app_pmic_precent_get(void)
{
	return  precent;
};

uint8_t pmic_state_get(void)
{
	return pmic_state;
}

/*******************************************************************************
 * Function Name     :  app_pmic_handler_timer_create
 * Description       : 创建pmic管理定时器
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/
void app_pmic_handler_timer_create(void)
{
	timer_struct.timer_handler = bc_rtos_timer_create( timer_struct.timer_name,
													   timer_struct.xTimerPeriodInTicks,
													 timer_struct.uxAutoReload, 
													 (void *)timer_struct.timer_id,
													timer_struct.timer_callback_function);
	if(timer_struct.timer_handler != NULL)
	{
//		bc_rtos_timer_start(timer_struct.timer_handler,100);
		BC_LOG_INFO("create %s succeed\r\n",timer_struct.timer_name);
//		app_pmic_init();
	}
    else
	{
		BC_LOG_ERROR("create %s fail\r\n",timer_struct.timer_name);
	}	
	
//#if ( HARDWARE_BCL601_151_ENABLED )					
//	app_pmic_handler_timer_start();
//#endif	
	
		
}





