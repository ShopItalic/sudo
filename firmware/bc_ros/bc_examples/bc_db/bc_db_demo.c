/*******************************************************************************
此为FlashDB demo文件,可参考FlashDB官方demo

q_device规则
日  期：2024年1月18日
编写人：邱成凯
 *******************************************************************************/


#include "bc_db_demo.h"

#include <flashdb.h>
#include "bc_logger.h"
#include "bc_timer.h"
#include "bc_event.h"

static void test_event_callback(void * p_context);

static bc_event_struct event_struct={
	.event_name = "test event",
	.event_callback_function = test_event_callback,
};


static void test_timer_callback(void * p_context);

static bc_timer_struct  test_timer = {
	.timer_name = "test timer",                          //定时器名字
	.uxAutoReload = true,                                //周期定时器
	.xTimerPeriodInTicks = 5000,                         //定时器时间
	.timer_callback_function = test_timer_callback,      //定时器回调
};


#define FDB_LOG_TAG "[main]"

uint8_t i = 0;


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


/* counts for simulated timestamp */
static int counts = 0;

extern void kvdb_basic_sample(fdb_kvdb_t kvdb);
extern void kvdb_type_blob_sample(fdb_kvdb_t kvdb);


//static void lock(fdb_db_t db)
//{
//    __disable_irq();
//}

//static void unlock(fdb_db_t db)
//{
//    __enable_irq();
//}

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

//#define FDB_LOG_TAG "[sample][kvdb][basic]"

void kvdb_basic_sample(fdb_kvdb_t kvdb)
{
    struct fdb_blob blob;
    int boot_count = 0;

    FDB_INFO("==================== kvdb_basic_sample ====================\n");

    { /* GET the KV value */
        /* get the "boot_count" KV value */
        fdb_kv_get_blob(kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
        /* the blob.saved.len is more than 0 when get the value successful */
        if (blob.saved.len > 0) {
            FDB_INFO("get the 'boot_count' value is %d\n", boot_count);
        } else {
            FDB_INFO("get the 'boot_count' failed\n");
        }
    }

    { /* CHANGE the KV value */
        /* increase the boot count */
        boot_count ++;
        /* change the "boot_count" KV's value */
        fdb_kv_set_blob(kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
        FDB_INFO("set the 'boot_count' value to %d\n", boot_count);
    }

    FDB_INFO("===========================================================\n");
}

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
       BC_LOG_INFO("llrrrr  result:%d \r\n",result);
		if (result != FDB_NO_ERR) {
			BC_LOG_INFO("llllllllllll  result:%d \r\n",result);
			return -1;
		}
		else
		{
			BC_LOG_INFO("llllllllllll  result:%d \r\n",result);
			i = 2;
		}
   }
	
   if(i > 2)
   {
	   kvdb_basic_sample(&kvdb);
   }
   i++;

//	/* run basic KV samples */
//	kvdb_basic_sample(&kvdb);
	/* run string KV samples */
//	kvdb_type_string_sample(&kvdb);
//	/* run blob KV samples */
//	kvdb_type_blob_sample(&kvdb);
}


/* TSDB object */
struct fdb_tsdb tsdb = { 0 };

struct env_status {
    int temp;
    int humi;
};

static bool query_cb(fdb_tsl_t tsl, void *arg);
static bool query_by_time_cb(fdb_tsl_t tsl, void *arg);
static bool set_status_cb(fdb_tsl_t tsl, void *arg);
static  struct env_status status = {
	  .temp = 36,
      .humi = 85,
                            };
void tsdb_sample(fdb_tsdb_t tsdb)
{
    struct fdb_blob blob;

    FDB_INFO("==================== tsdb_sample ====================\n");

    { /* APPEND new TSL (time series log) */
       

        /* append new log to TSDB */
        
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
//        fdb_tsl_iter(tsdb, set_status_cb, tsdb);
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


static uint8_t test_b = 0;
int test_db_ts(void)
{
	
	fdb_err_t result;
	
//	struct fdb_default_kv default_kv;

//	default_kv.kvs = default_kv_table;
//	default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
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
       BC_LOG_INFO("llrrrr  result:%d \r\n",result);
		if (result != FDB_NO_ERR) {
			BC_LOG_INFO("llllllllllll  result:%d \r\n",result);
			return -1;
		}
		else
		{
			BC_LOG_INFO("lllggggggg  result:%d \r\n",result);
			test_b = 2;
		}
   } 
}


static void test_timer_callback(void * p_context)
{
//	if(test_b == 0)
//	{
//		test_db_ts();
//	}
//	if(test_b == 2)
//	{
//		tsdb_sample(&tsdb);
//	}
//	test_db_kv();
	bc_event_set(&event_struct);                        //发送事件
}

static void test_event_callback(void * p_context)
{
//	if(test_b == 0)
//	{
//		test_db_ts();
//	}
//	if(test_b == 2)
//	{
//		tsdb_sample(&tsdb);
//	}
	test_db_kv();
}

/*******************************************************************************
 * Function Name     : test_timer_create
 * Description       : 创建
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/

void test_timer_create(void)
{
	if(!bc_timer_create(&test_timer))                            //创建定时器
	{
		BC_LOG_WARN("create %s fial!! \r\n",test_timer.timer_name);              
	}
	else
	{
		BC_LOG_INFO("create %s success!! \r\n",test_timer.timer_name);
		bc_timer_start(&test_timer);                             //启动定时器
	}
	
	if(!bc_event_create(&event_struct))                         //创建事件
	{
		BC_LOG_WARN("create %s fial!! \r\n",event_struct.event_name);
	}
	else
	{
		BC_LOG_INFO("create %s success!! \r\n",event_struct.event_name);
	}
	
}

