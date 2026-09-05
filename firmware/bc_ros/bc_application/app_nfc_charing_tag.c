#include "app_nfc_charing_tag.h"

#include "bc_nfc_st25dv.h"

#include "bc_timer.h"
#include "bc_logger.h"
#include "bc_pmic.h"
#include "bc_delay.h"
#include "bc_watchdog.h"

static uint32_t temp_count = 0;


enum app_nfc_charing_tag_timer_event
{
	APP_NFC_CHARING_TAG_CHECK_TIMER_EVENT = 0,
	APP_NFC_CHARING_TAG_CHARING_TIMER_EVENT,
	APP_NFC_CHARING_TAG_TIMER_NUM
};

static bool app_nfc_charing_init_flag = false;

static void app_nfc_charing_tag_check_timer_callback(void * pvParameter);
static void app_nfc_charing_tag_charing_timer_callback(void * pvParameter);

static bc_timer_struct  timer_struct[APP_NFC_CHARING_TAG_TIMER_NUM] = {	
	{
		.timer_name = "nfc tag check timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 300,
		.lock = false,
		.timer_callback_function = app_nfc_charing_tag_check_timer_callback,
	},
	{
		.timer_name = "nfc tag charing timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 300,
		.lock = false,
		.timer_callback_function = app_nfc_charing_tag_charing_timer_callback,
	},
};

static void app_timer_reset(enum app_nfc_charing_tag_timer_event time_id)
{
	bc_timer_stop(&timer_struct[time_id]);
	bc_delay_ms(1);
	bc_timer_start(&timer_struct[time_id]);
}

static void app_timer_start(enum app_nfc_charing_tag_timer_event time_id)
{

	bc_timer_start(&timer_struct[time_id]);
}

static void app_timer_stop(enum app_nfc_charing_tag_timer_event time_id)
{
	bc_timer_stop(&timer_struct[time_id]);
}
static void app_nfc_charing_handler(void)
{
	enum pmic_charge_status pmic_state =  bc_pmic_get_charge_status();
	BC_LOG_INFO("pmic_state :%d\r\n",pmic_state);
	switch(pmic_state)
	{
		case PMIC_CHARGED_NOT:                                                        //未充电
		{ 
			app_timer_stop(APP_NFC_CHARING_TAG_CHARING_TIMER_EVENT);
			break;
		}
		case PMIC_CHARGED_ING:                                                     //充电中
		case PMIC_CHARGED_OVER:                                                   //充电完成
		{
			bc_nfc_st25dv_send_process(pmic_state,bc_pmic_get_vbat_percen());
			break;
		}
	}
}


static bool app_nfc_charing_tag_check_flag = false;
static void app_nfc_charing_tag_check_timer_callback(void * pvParameter)
{
    app_nfc_charing_tag_check_flag = true;
}

void app_nfc_charing_tag_check_poll(void )
{
    if(!app_nfc_charing_tag_check_flag)
	{
		return;
	}
	if(!app_nfc_charing_init_flag)
	{
		app_nfc_charing_init_flag = true;
		return;
	}
	app_nfc_charing_tag_check_flag = false;
	enum pmic_charge_status pmic_state = bc_pmic_get_charge_status();
	uint8_t vbat_percen = bc_pmic_get_vbat_percen();
	BC_LOG_INFO("pmic_state :%d  vbat_percen:%d \r\n",pmic_state,vbat_percen);
	bc_nfc_st25dv_send_process(pmic_state,vbat_percen);
    bc_dog_feed();
	temp_count++;
	if(temp_count>= ((1000 *30) / timer_struct[APP_NFC_CHARING_TAG_CHECK_TIMER_EVENT].xTimerPeriodInTicks))
	{
		app_timer_stop(APP_NFC_CHARING_TAG_CHECK_TIMER_EVENT);
		app_timer_start(APP_NFC_CHARING_TAG_CHARING_TIMER_EVENT);
	}
}

static  bool app_nfc_charing_tag_charing_flag = false;
static void app_nfc_charing_tag_charing_timer_callback(void * pvParameter)
{
	app_nfc_charing_tag_charing_flag = true;
}

void app_nfc_charing_tag_charing_poll(void)
{

	if(app_nfc_charing_tag_charing_flag)
	{
	   app_nfc_charing_handler();
		app_nfc_charing_tag_charing_flag = false;
	}
}



void app_nfc_charing_tag_init(void)
{
	for(uint8_t i = 0;i < APP_NFC_CHARING_TAG_TIMER_NUM; i++)
	{
		if(!bc_timer_create(&timer_struct[i]))
		{
			BC_LOG_INFO("create %s fial!! \r\n",timer_struct[i].timer_name);
		}
		else
		{
			BC_LOG_INFO("create %s success!! \r\n",timer_struct[i].timer_name);
		}		
	}
	app_timer_start(APP_NFC_CHARING_TAG_CHECK_TIMER_EVENT);
//	bc_nfc_st25dv_device_init();
}




