#include "test.h"



#include "fml_freertos.h"

#include "log.h"

#include "q_device.h"

#include <flashdb.h>

#include "test_flashdb.h"


static  uint32_t temp_count = 0;


static void test_timer_callback (void * pvParameter);

static fml_rtos_timer_struct  test_timer = {
																						 .timer_name = "test timer",
																						 .uxAutoReload = pdTRUE,
																						 .xTimerPeriodInTicks = 3000,
																						 .timer_id = TEST_TIMER_ID,
																						 .timer_callback_function = test_timer_callback,                                                  
											                  	  };

																  
static q_device_t *flash_dev;	

static struct flash_write_package flash_write_pack;
static struct flash_read_package flash_read_pack;
static struct flash_config flash_cfg;
static struct flash_mutex_lock  mutex_lock;
																  
static fml_rtos_sem_struct  sem_handler  = {
											.sem_name = "fml_flash_sem",
											.sem_handler = NULL,
										   };



static void fml_flash_sem_take(void)
{
	fml_rtos_semaphore_take(sem_handler.sem_handler);
}

static void fml_flash_sem_give(void)
{
	fml_rtos_semaphore_give(sem_handler.sem_handler);
}

static bool fml_flash_sem_create(void)
{

	sem_handler.sem_handler = fml_rtos_sem_create();
	if(sem_handler.sem_handler != NULL)
	{
		LOG_INFO("create %s succeed \r\n",sem_handler.sem_name);
		return true;
	}
	else
	{
		LOG_ERROR("create  %s fail",sem_handler.sem_name);
		return false;
	}	

}

static int bsp_device_find(void)
{
    flash_dev = q_device_find("device_flash");
	q_device_assert(flash_dev);
	if(fml_flash_sem_create())
	{
		mutex_lock.mutex_lock_enable = true;
		mutex_lock.mutex_lock_take = fml_flash_sem_take;
		mutex_lock.mutex_lock_give = fml_flash_sem_give;
		q_device_cfg(flash_dev,&mutex_lock,0);
	}
    q_device_init(flash_dev);
    return 1;
}

static int read(long offset, uint8_t *buf, size_t size)
{	
	memset((uint8_t *)&flash_read_pack,0,sizeof(flash_read_pack));
	flash_read_pack.data = buf;
	flash_read_pack.data_length = size;
	flash_read_pack.offset = offset;
	if(q_device_read(flash_dev,0,&flash_read_pack,0) != RESULT_OK)
	{
		LOG_ERROR("flash read error!!! \r\n");
	}
    return size;
}

static int write(long offset, const uint8_t *buf, size_t size)
{	
	memset((uint8_t *)&flash_write_pack,0,sizeof(flash_write_pack));
	flash_write_pack.offset = offset;
	flash_write_pack.data_length = size;
    flash_write_pack.data = buf;
	if(q_device_write(flash_dev,0,&flash_write_pack,0) != RESULT_OK)
	{
		LOG_ERROR("flash write error!!!! \r\n");
	}
	return size;
}


static int erase(long offset, size_t size)
{
	memset((uint8_t *)&flash_write_pack,0,sizeof(flash_write_pack));
	flash_write_pack.offset = offset;
	flash_write_pack.data_length = size;
	if(q_device_ctrl(flash_dev,ERASE_FLASH,&flash_write_pack) != RESULT_OK)
	{
		LOG_ERROR("falsh erase error!!!!!!\r\n");
	}
	LOG_INFO("lllllllllll\r\n");
	return size;
}
																  
																  
static void test_flash_read_write(void)
{
	uint8_t temp[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x010,0x011,0x12,0x13};
	uint8_t read_temp[12] = {0};
	q_device_ctrl(flash_dev,READ_FLASH_CONFIG,&flash_cfg);
	LOG_INFO("flash info:  str addr:0x%08x,en addr:0x%08x,page size:%d, page num:%d \r\n",flash_cfg.strat_addr,flash_cfg.en_addr,flash_cfg.page_size,flash_cfg.page_num);
//	flash_write_pack.data = temp;
//	flash_write_pack.data_length = sizeof(temp);
//	flash_write_pack.offset = 0;
	
//	q_device_ctrl(flash_dev,ERASE_FLASH,&flash_write_pack);
	erase(0, sizeof(temp));
	write(0,temp,sizeof(temp));
	read(0,read_temp,sizeof(read_temp));
//	q_device_write(flash_dev,0,&flash_write_pack,0);
//	memset((uint8_t *)&flash_read_pack,0,sizeof(flash_read_pack));
//    flash_read_pack.data = read_temp;
//	flash_read_pack.data_length = sizeof(read_temp);
//	flash_read_pack.offset = 0;
	
//	q_device_read(flash_dev,0,&flash_read_pack,0);
	
//	LOG_HEX("read temp:",flash_read_pack.data,flash_read_pack.data_length);
	LOG_HEX("read temp:",read_temp,sizeof(read_temp));
	
	
	
}	






#define FDB_LOG_TAG "[main]"

static uint32_t boot_count = 0;
static time_t boot_time[10] = {0, 1, 2, 3};
/* default KV nodes */
static struct fdb_default_kv_node default_kv_table[] = {
        {"username", "armink", 0}, /* string KV */
        {"password", "123456", 0}, /* string KV */
        {"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
        {"boot_time", &boot_time, sizeof(boot_time)},    /* int array type KV */
};
/* KVDB object */
static struct fdb_kvdb kvdb = { 0 };
/* TSDB object */
struct fdb_tsdb tsdb = { 0 };
/* counts for simulated timestamp */
static int counts = 0;

extern void kvdb_basic_sample(fdb_kvdb_t kvdb);
extern void kvdb_type_string_sample(fdb_kvdb_t kvdb);
extern void kvdb_type_blob_sample(fdb_kvdb_t kvdb);
extern void tsdb_sample(fdb_tsdb_t tsdb);

static void lock(fdb_db_t db)
{
    __disable_irq();
}

static void unlock(fdb_db_t db)
{
    __enable_irq();
}

static fdb_time_t get_time(void)
{
    /* Using the counts instead of timestamp.
     * Please change this function to return RTC time.
     */
    return ++counts;
}


void kvdb_type_blob_sample(fdb_kvdb_t kvdb)
{
    struct fdb_blob blob;
     
    FDB_INFO("==================== kvdb_type_blob_sample ====================\n");

    { /* CREATE new Key-Value */
        int temp_data = 36;

        /* It will create new KV node when "temp" KV not in database.
         * fdb_blob_make: It's a blob make function, and it will return the blob when make finish.
         */
        fdb_kv_set_blob(kvdb, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
        FDB_INFO("create the 'temp' blob KV, value is: %d\n", temp_data);
    }

    { /* GET the KV value */
        int temp_data = 0;

        /* get the "temp" KV value */
        fdb_kv_get_blob(kvdb, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
        /* the blob.saved.len is more than 0 when get the value successful */
        if (blob.saved.len > 0) {
            FDB_INFO("get the 'temp' value is: %d\n", temp_data);
        }
    }

    { /* CHANGE the KV value */
        int temp_data = 38;

        /* change the "temp" KV's value to 38 */
        fdb_kv_set_blob(kvdb, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
        FDB_INFO("set 'temp' value to %d\n", temp_data);
    }

    { /* DELETE the KV by name */
        fdb_kv_del(kvdb, "temp");
        FDB_INFO("delete the 'temp' finish\n");
    }

    FDB_INFO("===========================================================\n");
}

void kvdb_type_string_sample(fdb_kvdb_t kvdb)
{
    FDB_INFO("==================== kvdb_type_string_sample ====================\n");

    { /* CREATE new Key-Value */
        char temp_data[10] = "36C";

        /* It will create new KV node when "temp" KV not in database. */
        fdb_kv_set(kvdb, "temp", temp_data);
        FDB_INFO("create the 'temp' string KV, value is: %s\n", temp_data);
    }

    { /* GET the KV value */
        char *return_value, temp_data[10] = { 0 };

        /* Get the "temp" KV value.
         * NOTE: The return value saved in fdb_kv_get's buffer. Please copy away as soon as possible.
         */
        return_value = fdb_kv_get(kvdb, "temp");
        /* the return value is NULL when get the value failed */
        if (return_value != NULL) {
            strncpy(temp_data, return_value, sizeof(temp_data));
            FDB_INFO("get the 'temp' value is: %s\n", temp_data);
        }
    }

    { /* CHANGE the KV value */
        char temp_data[10] = "38C";

        /* change the "temp" KV's value to "38.1" */
        fdb_kv_set(kvdb, "temp", temp_data);
        FDB_INFO("set 'temp' value to %s\n", temp_data);
    }

    { /* DELETE the KV by name */
        fdb_kv_del(kvdb, "temp");
        FDB_INFO("delete the 'temp' finish\n");
    }

    FDB_INFO("===========================================================\n");
}

struct env_status {
    int temp;
    int humi;
};

static bool query_cb(fdb_tsl_t tsl, void *arg);
static bool query_by_time_cb(fdb_tsl_t tsl, void *arg);
static bool set_status_cb(fdb_tsl_t tsl, void *arg);

void tsdb_sample(fdb_tsdb_t tsdb)
{
    struct fdb_blob blob;

    FDB_INFO("==================== tsdb_sample ====================\n");

    { /* APPEND new TSL (time series log) */
        struct env_status status;

        /* append new log to TSDB */
        status.temp = 36;
        status.humi = 85;
        fdb_tsl_append(tsdb, fdb_blob_make(&blob, &status, sizeof(status)));
        FDB_INFO("append the new status.temp (%d) and status.humi (%d)\n", status.temp, status.humi);

        status.temp = 38;
        status.humi = 90;
        fdb_tsl_append(tsdb, fdb_blob_make(&blob, &status, sizeof(status)));
        FDB_INFO("append the new status.temp (%d) and status.humi (%d)\n", status.temp, status.humi);
    }

    { /* QUERY the TSDB */
        /* query all TSL in TSDB by iterator */
        fdb_tsl_iter(tsdb, query_cb, tsdb);
    }

    { /* QUERY the TSDB by time */
        /* prepare query time (from 1970-01-01 00:00:00 to 2020-05-05 00:00:00) */
        struct tm tm_from = { .tm_year = 1970 - 1900, .tm_mon = 0, .tm_mday = 1, .tm_hour = 0, .tm_min = 0, .tm_sec = 0 };
        struct tm tm_to = { .tm_year = 2020 - 1900, .tm_mon = 4, .tm_mday = 5, .tm_hour = 0, .tm_min = 0, .tm_sec = 0 };
        time_t from_time = mktime(&tm_from), to_time = mktime(&tm_to);
        size_t count;
        /* query all TSL in TSDB by time */
        fdb_tsl_iter_by_time(tsdb, from_time, to_time, query_by_time_cb, tsdb);
        /* query all FDB_TSL_WRITE status TSL's count in TSDB by time */
        count = fdb_tsl_query_count(tsdb, from_time, to_time, FDB_TSL_WRITE);
        FDB_INFO("query count is: %zu\n", count);
    }

    { /* SET the TSL status */
        /* Change the TSL status by iterator or time iterator
         * set_status_cb: the change operation will in this callback
         *
         * NOTE: The actions to modify the state must be in order.
         *       like: FDB_TSL_WRITE -> FDB_TSL_USER_STATUS1 -> FDB_TSL_DELETED -> FDB_TSL_USER_STATUS2
         *       The intermediate states can also be ignored.
         *       such as: FDB_TSL_WRITE -> FDB_TSL_DELETED
         */
        fdb_tsl_iter(tsdb, set_status_cb, tsdb);
    }

    FDB_INFO("===========================================================\n");
}

#ifdef FDB_USING_TIMESTAMP_64BIT
#define __PRITS "ld"
#else
#define __PRITS "d"
#endif

static bool query_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct env_status status;
    fdb_tsdb_t db = arg;

    fdb_blob_read((fdb_db_t) db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &status, sizeof(status))));
//    FDB_INFO("[query_cb] queried a TSL: time: %" __PRITS ", temp: %d, humi: %d\n", tsl->time, status.temp, status.humi);

    return false;
}

static bool query_by_time_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct env_status status;
    fdb_tsdb_t db = arg;

    fdb_blob_read((fdb_db_t) db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &status, sizeof(status))));
//    FDB_INFO("[query_by_time_cb] queried a TSL: time: %" __PRITS ", temp: %d, humi: %d\n", tsl->time, status.temp, status.humi);

    return false;
}

static bool set_status_cb(fdb_tsl_t tsl, void *arg)
{
    fdb_tsdb_t db = arg;

//    FDB_INFO("set the TSL (time %" __PRITS ") status from %d to %d\n", tsl->time, tsl->status, FDB_TSL_USER_STATUS1);
    fdb_tsl_set_status(db, tsl, FDB_TSL_USER_STATUS1);

    return false;
}

uint8_t i = 0;
int kv_init(void)
{
		fdb_err_t result;
	
	struct fdb_default_kv default_kv;

	default_kv.kvs = default_kv_table;
	default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
//	if(i == 0)
//	{
//		i= 1;
		/* set the lock and unlock function if you want */
//		fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, (void *)lock);
//		fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void *)unlock);
		/* Key-Value database initialization
		 *
		 *       &kvdb: database object
		 *       "env": database name
		 * "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
		 *              Please change to YOUR partition name.
		 * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
		 *        NULL: The user data if you need, now is empty.
		 */
		result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);

		if (result != FDB_NO_ERR) {
			return -1;
		}
//   }
}

int test_db_kv(void)
{
	
	fdb_err_t result;
	
	struct fdb_default_kv default_kv;

	default_kv.kvs = default_kv_table;
	default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
	if(i == 0)
	{
		i= 1;
		/* set the lock and unlock function if you want */
//		fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, (void *)lock);
//		fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void *)unlock);
		/* Key-Value database initialization
		 *
		 *       &kvdb: database object
		 *       "env": database name
		 * "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
		 *              Please change to YOUR partition name.
		 * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
		 *        NULL: The user data if you need, now is empty.
		 */
		result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);
       LOG_INFO("llrrrr  result:%d \r\n",result);
		if (result != FDB_NO_ERR) {
			LOG_INFO("llllllllllll  result:%d \r\n",result);
			return -1;
		}
		else
		{
			LOG_INFO("llllllllllll  result:%d \r\n",result);
			i = 2;
		}
   }
	
   if(i == 2)
   {
//	   kvdb_basic_sample(&kvdb);
   }

//	/* run basic KV samples */
//	kvdb_basic_sample(&kvdb);
	/* run string KV samples */
//	kvdb_type_string_sample(&kvdb);
//	/* run blob KV samples */
//	kvdb_type_blob_sample(&kvdb);
}
static uint8_t test_b = 0;
int test_db_ts(void)
{
	
	fdb_err_t result;
	
	struct fdb_default_kv default_kv;

	default_kv.kvs = default_kv_table;
	default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
	if(test_b == 0)
	{
		test_b= 1;
		/* set the lock and unlock function if you want */
//		fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, (void *)lock);
//		fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void *)unlock);
		/* Key-Value database initialization
		 *
		 *       &kvdb: database object
		 *       "env": database name
		 * "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
		 *              Please change to YOUR partition name.
		 * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
		 *        NULL: The user data if you need, now is empty.
		 */
		result = fdb_tsdb_init(&tsdb, "log", "fdb_tsdb1", get_time, 128, NULL);
        /* read last saved time for simulated timestamp */
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &counts);
       LOG_INFO("llrrrr  result:%d \r\n",result);
		if (result != FDB_NO_ERR) {
			LOG_INFO("llllllllllll  result:%d \r\n",result);
			return -1;
		}
		else
		{
			LOG_INFO("llllllllllll  result:%d \r\n",result);
			test_b = 2;
		}
   }
}


/*******************************************************************************
 * Function Name     : app_test_timer_callback
 * Description       : test定时器回调
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
static void test_timer_callback (void * pvParameter)
{
	
//	q_device_ctrl(app_led_dev,GPIO_OUTPUT_TOGGLE,0);
//	LOG_INFO("test \r\n");
	LOG_INFO("test  %d \r\n",temp_count++);
//	test_flash_read_write();
//NRF_LOG_INFO("test");
//	test_db_kv();
//	   if(i == 2)
//   {
////	   kvdb_basic_sample(&kvdb);
////	   kvdb_type_string_sample(&kvdb);
//	   kvdb_type_blob_sample(&kvdb);
//   }
//	printf("lllll\r\n");
//	if(test_b == 2)
//	{
//		tsdb_sample(&tsdb);
//	}
}




/*******************************************************************************
 * Function Name     : app_test_timer_create
 * Description       : test定时器
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
void test_timer_create(void)
{
	
	test_timer.timer_handler = fml_rtos_timer_create( test_timer.timer_name,
															 test_timer.xTimerPeriodInTicks,
															 test_timer.uxAutoReload, 
															 (void *)test_timer.timer_id,
														     test_timer.timer_callback_function);
	if(test_timer.timer_handler != NULL)
	{
		fml_rtos_timer_start(test_timer.timer_handler,100);
		LOG_INFO("create %s succeed\r\n",test_timer.timer_name);
	}
  else
	{
		LOG_ERROR("create %s fail\r\n",test_timer.timer_name);
	}		
//	bsp_device_find();
//	kv_init();
//	test_db_kv();
//	test_db_ts();
}


