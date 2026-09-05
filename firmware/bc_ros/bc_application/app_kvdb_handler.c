#include "app_kvdb_handler.h"



#include <flashdb.h>

#include "string.h"

#include "bc_logger.h"



#define FDB_LOG_TAG "[sample][kvdb][basic]"



static struct fdb_kvdb kvdb = { 0 };


static struct device_info_kv info_kv[DEVICE_INFO_TYPE_NUM] = {
{"ppg update record"},
{"ppg automatic cycle time"},
{"g_sensor sport setp"},
{"ble name"},
};


static uint32_t boot_count = 0;
static time_t boot_time[10] = {0, 1, 2, 3};
static struct fdb_default_kv_node default_kv_table[DEVICE_INFO_TYPE_NUM+4] = {
        {"username", "armink", 0},                       /* string KV    */
        {"password", "123456", 0},                       /* string KV    */
		{"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
        {"boot_time", &boot_time, sizeof(boot_time)},    /* int array type KV */
};



static void app_kvdb_default_kv_table_init(void)
{
	uint32_t automatic_cycle_tiem = 300;
	uint32_t updata_tiem = 0;
	uint16_t sport_step = 0;
	char ble_name[] = "BCL603";
	memcpy(info_kv[DEVICE_INFO_PPG_AUTOMATIC_CYCLE_TIME].value,(uint8_t*)&automatic_cycle_tiem ,sizeof(automatic_cycle_tiem));
	memcpy(info_kv[DEVICE_INFO_PPG_UPDATA_RECORD].value,(uint8_t*)&updata_tiem ,sizeof(updata_tiem));
	memcpy(info_kv[DEVICE_INFO_G_SENSOR_SPORT_STEP].value,(uint8_t*)&sport_step ,sizeof(sport_step));
	memcpy(info_kv[DEVICE_INFO_BLE_NAME].value,(uint8_t*)ble_name ,6);
	for(uint8_t i = 0; i < DEVICE_INFO_TYPE_NUM;i++)
	{
		default_kv_table[i+4].key = info_kv[i].key;
		default_kv_table[i+4].value = info_kv[i].value;
		default_kv_table[i+4].value_len = sizeof(info_kv[i].value);
	}
}
												

static bool app_kvdb_get(enum device_info_kv_type kv_id,uint8_t *value,uint8_t value_length)
{
	if(kv_id >= DEVICE_INFO_TYPE_NUM)
	{
		return false;
	}
	struct fdb_blob blob;
	
	fdb_kv_get_blob(&kvdb, info_kv[kv_id].key, fdb_blob_make(&blob, value, value_length));
	if (blob.saved.len > 0) 
	{
		BC_LOG_HEX(info_kv[kv_id].key,value,value_length);
		return true;
	}
	
	FDB_INFO("get the %s failed\n",info_kv[kv_id].key);
	return false;
}

static bool app_kvdb_set(enum device_info_kv_type kv_id,uint8_t *value,uint8_t value_length)
{
	if(kv_id >= DEVICE_INFO_TYPE_NUM)
	{
		return false;
	}
	struct fdb_blob blob;
	if(fdb_kv_set_blob(&kvdb, info_kv[kv_id].key, fdb_blob_make(&blob, value, value_length)) == 0)
	{
		return true;
	}
	return false;
}

bool app_kvdb_get_handler(enum device_info_kv_type kv_id,uint8_t *value,uint8_t value_length)
{
	if(value == NULL ||value_length <= 0 )
	{
		return false;
	}
	switch(kv_id)
	{
		case DEVICE_INFO_PPG_UPDATA_RECORD:             //ppg 历史记录上传成功后的时间，用于记录上传时间标志
		{
			return app_kvdb_get(kv_id,value,value_length);
			
		}
		case DEVICE_INFO_PPG_AUTOMATIC_CYCLE_TIME:          //ppg 自动周期采集的间隔时间
		{
			app_kvdb_get(kv_id,value,value_length);
			break;
		}	
		case DEVICE_INFO_G_SENSOR_SPORT_STEP:                //运动步数
		{
			uint32_t temp = 0;
			app_kvdb_get(kv_id,(uint8_t*)&temp,sizeof(temp));
			*value = (uint16_t)temp;
			break;
		}	
		case DEVICE_INFO_BLE_NAME:
		{
		  uint32_t store_data_length=value_length/4+1;
		  if(value_length%4==0)
		  {
			store_data_length-=1;
		  }
		 return  app_kvdb_get(kv_id,value,sizeof(store_data_length));
		}
		default:
		{
			break;
		}
	}
	return true;
}

bool app_kvdb_set_handler(enum device_info_kv_type kv_id,uint8_t *value,uint8_t value_length)
{
	if(value == NULL ||value_length <= 0 )
	{
		return false;
	}
	switch(kv_id)
	{
		case DEVICE_INFO_PPG_UPDATA_RECORD:             //ppg 历史记录上传成功后的时间，用于记录上传时间标志
		{
			return app_kvdb_set(kv_id,value,value_length);
		
		}
		case DEVICE_INFO_PPG_AUTOMATIC_CYCLE_TIME:          //ppg 自动周期采集的间隔时间
		{
			return app_kvdb_set(kv_id,value,value_length);
		
		}	
		case DEVICE_INFO_G_SENSOR_SPORT_STEP:                //运动步数
		{
			uint32_t temp = *value;
			return app_kvdb_get(kv_id,(uint8_t*)&temp,sizeof(temp));
		
		}	
		case DEVICE_INFO_BLE_NAME:
		{ 
			uint32_t store_data_length=value_length/4+1;
		  if(value_length%4==0)
		  {
			store_data_length-=1;
		  }
		  return app_kvdb_set(kv_id,value,sizeof(store_data_length));
			
		}
		default:
		{
			break;
		}
	}
	return true;
}



void app_kvdb_init(void)
{
	fdb_err_t result;
	struct fdb_default_kv default_kv;
    app_kvdb_default_kv_table_init();
	default_kv.kvs = default_kv_table;
	default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
//	fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, (void *)app_tsdb_mutex_lock_take);
//	fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void *)app_tsdb_mutex_lock_give);


	result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);
    BC_LOG_INFO("kvdb result:%d \r\n",result);
    if (result != FDB_NO_ERR) {
			
    }

	
//	app_tsdb_handler_thread_create();
}


void app_kvdb_test(void)
{
//	uint8_t test[20]= {0x01,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02};
	uint16_t test  = 0;
	app_kvdb_get_handler(DEVICE_INFO_G_SENSOR_SPORT_STEP,(uint8_t*)&test,sizeof(test));
	test++;
	app_kvdb_set_handler(DEVICE_INFO_G_SENSOR_SPORT_STEP,(uint8_t*)&test,sizeof(test));

}
