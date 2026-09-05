#include "app_tsdb_handler.h"

#include "app_package.h"
#include "app_g_sensor_handler.h"
#include "app_ppg_handler.h"


#include <string.h>

#include <flashdb.h>

#include "bc_logger.h"
//#include "bc_event.h"
//#include "bc_timer.h"
#include "bc_rtc.h"
#include "bc_delay.h"
#include "bc_device_info.h"
#include "bc_watchdog.h"
#include "bc_rtos.h"

#define FDB_LOG_TAG "[sample][tsdb]"

#ifdef FDB_USING_TIMESTAMP_64BIT
#define __PRITS "ld"
#else
#define __PRITS "d"
#endif

static struct fdb_tsdb tsdb = {0};

static uint32_t tsdb_time = 0;

static const uint32_t uinx_time_7_day = 604800;  //7天的时间

struct ppg_updata_info
{
	bool app_ppg_updata_flag;
	uint32_t data_count_mun;
	uint32_t seq;
	uint32_t start_time;
	struct app_cmd_package cmd_package;
};

static bool tsdb_up_data_flag = false;

enum app_tsdb_status
{
	MEASURING = 0, //测量中
	UP_RECORDING,  //上传中
	CLEAR_RECORDING, //删除中  
	FILE_ERROR,
	TSDB_IDIE
};
enum app_tsdb_status tsdb_status = TSDB_IDIE;
static struct ppg_updata_info  updata_info = {0};



static store_data_unit_t store_data = {0};
 struct fdb_blob blob;



static void app_tsdb_up_event_callback(void * p_context);
static void app_tsdb_up_timeout_event_callback(void * p_context);

static bc_event_struct event_struct[APP_TSDB_EVENT_NUM]={
	{
		.event_name = "app tsdb up event",
		.event_callback_function = app_tsdb_up_event_callback,
	},
	{
		.event_name = "app tsdb up tiemout event",
		.event_callback_function = app_tsdb_up_timeout_event_callback,
	}
};
											

static void app_tsdb_data_timeout_timer_callback(void * p_context);
static bc_timer_struct  up_date_tim_timer = {
			.timer_name = "app_tsdb_data timer",
			.uxAutoReload = true,
			.xTimerPeriodInTicks = 1000,
			.timer_callback_function = app_tsdb_data_timeout_timer_callback,
			.lock = false,
};													
												  
										
static void timer_start(bc_timer_struct  *timer)
{
	if(!timer->lock)
	{
		bc_timer_start(timer);
		timer->lock = true;
	}
}

static void timer_stop(bc_timer_struct  *timer)
{
	if(timer->lock)
	{
		bc_timer_stop(timer);
		timer->lock = false;
	}
}

static void event_set(enum app_tsdb_event event_index)
{
	bc_event_set(&event_struct[event_index]); 
}

static void app_tsdb_data_timeout_timer_callback(void * p_context)
{
	if(tsdb_up_data_flag)
	{
		tsdb_up_data_flag = false;
	}
	else
	{
		event_set(APP_TSDB_UP_TIMEOUT_EVENT);
		timer_stop(&up_date_tim_timer);
	}
}	
													
static fdb_time_t app_tsdb_get_time(void)
{
	return bg_rtc_time_get_uinx_time();
}

static bool app_tsdb_query_cb(fdb_tsl_t tsl, void *arg)
{
   
//    store_data_unit_t store_data;
    fdb_tsdb_t db = arg;

    fdb_blob_read((fdb_db_t) db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &store_data, sizeof(store_data_unit_t))));

	if(updata_info.app_ppg_updata_flag)
	{
		FDB_INFO("[query_cb] queried a TSL: time: %" __PRITS ", temp: %d, humi: %d\n", tsl->time, store_data.accumulated_step, store_data.unix_time_s);
		uint8_t temp_length = sizeof(store_data_unit_t) - sizeof(store_data.rr_array) + (store_data.rr_num * 2) - sizeof(store_data.size);
		if(store_data.unix_time_s >= 1609434061) //时间大于2021.1.1.1.1才上传
		{
			app_package_history_record_up(&updata_info.cmd_package,&store_data.unix_time_s,temp_length,updata_info.data_count_mun,updata_info.seq);
		}
		bc_delay_ms(10);
		updata_info.seq++;
		updata_info.start_time = tsl->time;
		tsdb_up_data_flag = true;
		timer_start(&up_date_tim_timer);
		bc_dog_feed();
	}
	
    return false;	
}

static void app_tsdb_iter_by_time(uint32_t start_time,uint32_t end_time)
{
	 fdb_tsl_iter_by_time(&tsdb, start_time, end_time, app_tsdb_query_cb, &tsdb);
}

static uint32_t app_tsdb_tsl_query_count_get(uint32_t start_time,uint32_t end_time)
{
	return fdb_tsl_query_count(&tsdb, start_time, end_time ,FDB_TSL_WRITE);
}

static bool app_tsdb_init(void)
{
	fdb_err_t result;
	
//	fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_LOCK, (void *)app_tsdb_mutex_lock_take);
//  fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_UNLOCK, (void *)app_tsdb_mutex_lock_give);

	result = fdb_tsdb_init(&tsdb, "log", "fdb_tsdb1", app_tsdb_get_time, sizeof(store_data_unit_t), NULL);
	fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &tsdb_time);
	BC_LOG_INFO("tsdb timer :%d \r\n",tsdb_time);
	if(tsdb_time > 1672502465)
	{
		uint32_t temp_time = bc_device_info_app_update_time_get();
		uint32_t temp_reset_time = bc_device_info_app_reset_time_get();
		
		if(tsdb_time < temp_time)
		{
			tsdb_time  = temp_time;
		}
		
		if(tsdb_time < temp_reset_time)
		{
			tsdb_time  = temp_reset_time;
		}
		
		bc_rtc_time_set_uinx_time(tsdb_time,0);
		if(bc_device_info_get_ppg_update_record_time() == 0)
		{
			bc_device_info_set_ppg_update_record_time(tsdb_time - uinx_time_7_day);
		}
		app_tsdb_tsl_query_count_get(tsdb_time-5,tsdb_time);
	}
	else
	{
		uint32_t temp_time = bc_device_info_app_update_time_get();
		uint32_t temp_reset_time = bc_device_info_app_reset_time_get();
		if(temp_time !=0 || temp_reset_time != 0)
		{
			if(temp_time > temp_reset_time)
			{
				bc_rtc_time_set_uinx_time(temp_time,0);
				if(bc_device_info_get_ppg_update_record_time() == 0)
				{
					bc_device_info_set_ppg_update_record_time(temp_time - uinx_time_7_day);
				}
				app_tsdb_tsl_query_count_get(temp_time-5,temp_time);
			}
			else
			{
				bc_rtc_time_set_uinx_time(temp_reset_time,0);
				if(bc_device_info_get_ppg_update_record_time() == 0)
				{
					bc_device_info_set_ppg_update_record_time(temp_reset_time - uinx_time_7_day);
				}
				app_tsdb_tsl_query_count_get(temp_reset_time-5,temp_reset_time);
			}
		}
	}
	
	if (result != FDB_NO_ERR) 
	{
		BC_LOG_INFO("app tsdb init error result:%d \r\n",result);
       return false;
    }
	BC_LOG_INFO("app tsdb init ok \r\n");
	return true;
}



static void app_tsdb_up_event_callback(void * p_context)
{

	app_tsdb_iter_by_time(updata_info.start_time,bg_rtc_time_get_uinx_time());

}

static void app_tsdb_up_timeout_event_callback(void * p_context)
{
	updata_info.start_time+=2;
	bc_device_info_set_ppg_update_record_time(updata_info.start_time);
	BC_LOG_INFO("updata data storage record time:%d\r\n",updata_info.start_time);
	tsdb_status = TSDB_IDIE;
	updata_info.app_ppg_updata_flag = false;
}

bool app_tsdb_data_write(store_data_unit_t *store_data)
{	
	if(fdb_tsl_append(&tsdb, fdb_blob_make(&blob, store_data, sizeof(store_data_unit_t)))== FDB_NO_ERR)
	{
		FDB_INFO("append the new status.temp (%d) and status.humi (%d)\n",store_data->hr, store_data->unix_time_s);
		return true;
	}
	return false;
	
}


void app_tsdb_data_port_updata(struct app_cmd_package * cmd_package)
{
	if(updata_info.app_ppg_updata_flag)
	{
		cmd_package->subcmd = 0xff;
		uint8_t reault = UP_RECORDING;
		app_package_history_record_up(&updata_info.cmd_package,&reault,sizeof(reault),0,0);
		return ;
	}

	memcpy((uint8_t*)&updata_info.cmd_package,(uint8_t*)cmd_package,10);
	updata_info.app_ppg_updata_flag = true;

	updata_info.seq = 1;
	
//	app_kvdb_get_handler(DEVICE_INFO_PPG_UPDATA_RECORD,(uint8_t*)&updata_info.start_time,sizeof(updata_info.start_time));
	updata_info.start_time = bc_device_info_get_ppg_update_record_time();
	BC_LOG_INFO("data start time:%d\r\n",updata_info.start_time);

	if(updata_info.start_time == 0)
	{
		updata_info.start_time = bg_rtc_time_get_uinx_time() - uinx_time_7_day;
		updata_info.data_count_mun = app_tsdb_tsl_query_count_get(updata_info.start_time,bg_rtc_time_get_uinx_time());
		if(updata_info.data_count_mun == 0)
		{
			app_package_history_record_up(&updata_info.cmd_package,&store_data,0,updata_info.data_count_mun,updata_info.seq);
			updata_info.app_ppg_updata_flag = false;
			return;
		}
		tsdb_status = UP_RECORDING;
		event_set(APP_TSDB_UP_DATA_EVENT);                       //发送事件
	}
	else
	{
		updata_info.data_count_mun = app_tsdb_tsl_query_count_get(updata_info.start_time,bg_rtc_time_get_uinx_time());
		if(updata_info.data_count_mun == 0)
		{
			app_package_history_record_up(&updata_info.cmd_package,&store_data,0,updata_info.data_count_mun,updata_info.seq);
			updata_info.app_ppg_updata_flag = false;
			return;
		}
		tsdb_status = UP_RECORDING;
		event_set(APP_TSDB_UP_DATA_EVENT);                        //发送事件
	}
	

}


void app_tsdb_data_all_updata(struct app_cmd_package * cmd_package)
{
	if(updata_info.app_ppg_updata_flag)
	{
		cmd_package->subcmd = 0xff;
		uint8_t reault = UP_RECORDING;
		app_package_history_record_up(cmd_package,&reault,sizeof(reault),0,0);
		return ;
	}

	memcpy((uint8_t*)&updata_info.cmd_package,(uint8_t*)cmd_package,10);
	updata_info.app_ppg_updata_flag = true;
	updata_info.seq = 1;
	
	updata_info.start_time =bg_rtc_time_get_uinx_time() - uinx_time_7_day;
	updata_info.data_count_mun = app_tsdb_tsl_query_count_get(updata_info.start_time,bg_rtc_time_get_uinx_time());
	if(updata_info.data_count_mun == 0)
	{
		app_package_history_record_up(&updata_info.cmd_package,&store_data,0,updata_info.data_count_mun,updata_info.seq);
		updata_info.app_ppg_updata_flag = false;
		return;
	}

	tsdb_status = UP_RECORDING;
	event_set(APP_TSDB_UP_DATA_EVENT);                        //发送事件


	
}
void app_tsdb_data_stop_updata(void)
{
	if(updata_info.app_ppg_updata_flag)
	{
		updata_info.app_ppg_updata_flag = false;
		updata_info.seq = 1;		
		updata_info.data_count_mun =0;
		updata_info.start_time+=2;
//		app_kvdb_set_handler(DEVICE_INFO_PPG_UPDATA_RECORD,(uint8_t*)&updata_info.start_time,sizeof(updata_info.start_time));
		bc_device_info_set_ppg_update_record_time(updata_info.start_time);
		BC_LOG_INFO("updata data storage record time:%d\r\n",updata_info.start_time);
	}
	
}

static bool tsdb_clear_flag = false;
void app_tsdb_clear(struct app_cmd_package * cmd_package)
{
	if(tsdb_clear_flag)
	{
		tsdb_clear_flag = false;
		fdb_tsl_clean(&tsdb);
		return;
	}
	memcpy((uint8_t*)&updata_info.cmd_package,(uint8_t*)cmd_package,10);
	if(updata_info.app_ppg_updata_flag)
	{
		cmd_package->subcmd = 0xff;
		uint8_t reault = UP_RECORDING;
		app_package_history_record_up(&updata_info.cmd_package,&reault,sizeof(reault),0,0);	
        return;		
	}
	fdb_tsl_clean(&tsdb);
	app_package_send_enqueue(cmd_package,4);

}

struct app_cmd_package cmd_package = {0};
void app_tsdb_data_port_updata_event(void)
{
	cmd_package.cmd = 0x36;
	cmd_package.frame_id = 9;
	cmd_package.frame_type = 0;
	cmd_package.subcmd =  0;
	*(uint32_t*)cmd_package.data = bc_device_info_get_ppg_update_record_time();
    app_ble_recv_enent((uint8_t*)&cmd_package,4+4);
	
}

void app_tsdb_clear_event(void)
{
	cmd_package.cmd = 0x36;
	cmd_package.frame_id = 9;
	cmd_package.frame_type = 0;
	cmd_package.subcmd =  3;
	tsdb_clear_flag = true;
    app_ble_recv_enent((uint8_t*)&cmd_package,4);
//	fdb_tsl_clean(&tsdb);
}




static bool init_flag = false;
void app_flashdb_init(void)
{
	if(!bc_timer_create(&up_date_tim_timer))
	{
		BC_LOG_INFO("create %s fial!! \r\n",up_date_tim_timer.timer_name);
	}
	else
	{
		BC_LOG_INFO("create %s success!! \r\n",up_date_tim_timer.timer_name);
	}
	for(uint8_t i = 0; i < APP_TSDB_EVENT_NUM;i++)
	{
		if(!bc_event_create(&event_struct[i]))                         //创建事件
		{
			BC_LOG_WARN("create %s fial!! \r\n",event_struct[i].event_name);
		}
		else
		{
			BC_LOG_INFO("create %s success!! \r\n",event_struct[i].event_name);
		}
	}
	
	app_tsdb_init();
	bc_delay_ms(10);
	init_flag = true;
}

uint8_t app_tsdb_hr_get(void)
{
	return store_data.hr;
}




 store_data_unit_t store_data_test = {0};
void app_tsdb_test(void)
{
//   if(store_data_test.sleep_accumulated_time < 10 && init_flag)
//   {

////	fml_rtos_delay(10);
////	app_tsdb_iter();
////	fml_rtos_delay(20);
//	store_data_test.accumulated_step++;
//	store_data_test.unix_time_s = bg_rtc_time_get_uinx_time();	
//	store_data_test.hr++;
//	store_data_test.hrv++;
////	store_data_test.sleep_accumulated_time = fml_rtc_time_get_uinx_time();	
//	store_data_test.sleep_accumulated_time++;	
//	store_data_test.sleep_mode++;
//	store_data_test.spo2++;
//	store_data_test.sport_mode++;
//	store_data_test.sprit++;
//	store_data_test.temp++;
//	BC_LOG_INFO("store_data_test.unix_time_s:%d \r\n",store_data_test.unix_time_s);
//	if(!app_tsdb_data_write(&store_data_test))
//	{
//		return;
//	}
//   }
//   if(store_data_test.sleep_accumulated_time >= 10)
//   {
//	   app_tsdb_iter_by_time(updata_info.start_time,bg_rtc_time_get_uinx_time());
//   }
}

















