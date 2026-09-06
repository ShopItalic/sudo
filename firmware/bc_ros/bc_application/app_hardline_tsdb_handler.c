#include "app_hardline_tsdb_handler.h"


#include <string.h>

#include <flashdb.h>

#include "app_package.h"

#include "bc_logger.h"
#include "bc_rtc.h"
#include "bc_delay.h"

#include "bc_rtos.h"
#include "bc_sem.h"



#define FDB_LOG_TAG "[sample][tsdb]"


#ifdef FDB_USING_TIMESTAMP_64BIT
#define __PRITS "ld"
#else
#define __PRITS "d"
#endif

static struct fdb_tsdb tsdb = {0};

static struct fdb_blob blob;



static struct app_hardline_struct hardline_struct = {0};
static struct app_cmd_package  app_tsdb_send_package = {0};

enum app_hardline_tsdb_task
{
	HARDLINE_TSDB_TASK_UP_TSL = 0,
	HARDLINE_TSDB_TASK_TYPE_NUM
};

static void app_handline_tsdb_up_thread(void *thread_handler);

static bc_rtos_thread_struct thread_struct[HARDLINE_TSDB_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "app tsdb up task",
                                                                      .thread_stack_depth   = APP_TASK_HARDLINE_TSDB_STACK_SIZE ,
                                                                      .thread_priority      = APP_TASK_HARDLINE_TSDB_PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = app_handline_tsdb_up_thread,
                                                                    },																	
                                                                  };


static void app_tsdb_mutex_lock_take(void)
{
  bc_rtos_sem_take(BC_HARDLINE_TSDB_SEM);
}

static void app_tsdb_mutex_lock_give(void)
{
  bc_rtos_sem_give(BC_HARDLINE_TSDB_SEM);
}

static fdb_time_t app_tsdb_get_time(void)
{
	return bg_rtc_time_get_uinx_time();
}

static bool app_tsdb_query_cb(fdb_tsl_t tsl, void *arg)
{
   
    fdb_tsdb_t db = arg;

    fdb_blob_read((fdb_db_t) db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &hardline_struct, sizeof(hardline_struct))));
    BC_LOG_INFO("time:%d,type:%d",hardline_struct.timer,hardline_struct.type);
    bc_uinx_to_bj_time_print(hardline_struct.timer);
	
    return false;	
}

static void app_tsdb_iter_by_time(uint32_t start_time,uint32_t end_time)
{
	 fdb_tsl_iter_by_time(&tsdb, start_time, end_time, app_tsdb_query_cb, &tsdb);
}

static bool app_tsdb_lite_ccallback(fdb_tsl_t tsl, void *arg)
{
   
//    store_data_unit_t store_data;
    fdb_tsdb_t db = arg;

    fdb_blob_read((fdb_db_t) db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &hardline_struct, sizeof(hardline_struct))));
    BC_LOG_INFO("time:%d,type:%d",hardline_struct.timer,hardline_struct.type);
    bc_uinx_to_bj_time_print(hardline_struct.timer);
    *(uint32_t*)&app_tsdb_send_package.data[4] += 1;
    memcpy(&app_tsdb_send_package.data[8],(uint8_t*)&hardline_struct,sizeof(hardline_struct));
    app_package_send_enqueue(&app_tsdb_send_package,4+8+sizeof(hardline_struct));
    return false;	
}

static void app_tsdb_tsl_iter(void)
{
  fdb_tsl_iter(&tsdb, app_tsdb_lite_ccallback, &tsdb);
}


static uint32_t app_tsdb_tsl_query_count_get(void)
{
  uint32_t end_time = app_tsdb_get_time();
  uint32_t start_time = 1672502465;
	return fdb_tsl_query_count(&tsdb, start_time, end_time ,FDB_TSL_WRITE);
}

static bool app_tsdb_data_write(struct app_hardline_struct *hardline_data)
{	
	if(fdb_tsl_append(&tsdb, fdb_blob_make(&blob, hardline_data, sizeof(hardline_struct)))== FDB_NO_ERR)
	{
		FDB_INFO("append the new status.temp (%d) and status.humi (%d)\n",hardline_data->timer, hardline_data->type);
		return true;
	}
	return false;
}



static void app_tsdb_clear(void)
{
  fdb_tsl_clean(&tsdb);
}

static void app_tsdb_init(void)
{
  fdb_err_t result;
	
	fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_LOCK, (void *)app_tsdb_mutex_lock_take);
  fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_UNLOCK, (void *)app_tsdb_mutex_lock_give);

	result = fdb_tsdb_init(&tsdb, "log", "fdb_tsdb1", app_tsdb_get_time, sizeof(hardline_struct), NULL);
	//fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &tsdb_time);
}

static void app_handline_tsdb_up_thread(void *thread_handler)
{
  app_tsdb_init();
  bc_rtos_thread_suspend(thread_struct[HARDLINE_TSDB_TASK_UP_TSL].thread_handler);
  while(true)
  {
    *(uint32_t*)app_tsdb_send_package.data = app_tsdb_tsl_query_count_get() ;
    *(uint32_t*)&app_tsdb_send_package.data[4] = 0;
    app_tsdb_tsl_iter();
    bc_rtos_thread_suspend(thread_struct[HARDLINE_TSDB_TASK_UP_TSL].thread_handler);
  }
}

void app_hardline_upload_tsdb(struct app_cmd_package *package)
{
  memcpy((uint8_t*)&app_tsdb_send_package,(uint8_t*)package,4);
  bc_rtos_thread_resume(thread_struct[HARDLINE_TSDB_TASK_UP_TSL].thread_handler);
}


void app_hardline_clear(void)
{
  app_tsdb_clear();
}

void app_hardline_mark(void)
{
  struct app_hardline_struct hardline = {0};
  hardline.timer = bg_rtc_time_get_uinx_time();
  hardline.type = 0;
  app_tsdb_data_write(&hardline);
}


void app_hardline_tsdb_create(void)
{
	bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < HARDLINE_TSDB_TASK_TYPE_NUM; i++)
	{
		x_return  = bc_rtos_thread_create((TaskFunction_t )thread_struct[i].thread_task_code,     	
                                     (const char*    )thread_struct[i].thread_name,   	
                                     (uint16_t       )thread_struct[i].thread_stack_depth, 
                                     (void*          )&thread_struct[i].thread_parameters,				
                                     (UBaseType_t    )thread_struct[i].thread_priority,	
                                     (TaskHandle_t*  )&thread_struct[i].thread_handler); 
		if(x_return != NULL)
		{
			BC_LOG_INFO("create %s succeed \r\n",thread_struct[i].thread_name);
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",thread_struct[i].thread_name);
		}	
	}
	
}































































