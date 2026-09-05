#include "app_ppg_file_data_handler.h"

#include "lfs.h"
#include "lfs_port.h"

#include <string.h>
#include <stdlib.h>

#include "bc_spi_flash_port.h"
#include "bc_logger.h"
#include "bc_device_info.h"
#include "bc_rtc.h"
#include "bc_pdm.h"
#include "bc_sem.h"


#include "bc_util.h"

#include "app_package.h"
#include "app_ppg_data_handler.h"
#include "app_pmic_handler.h"
#include "bc_delay.h"

#include "bc_watchdog.h"
#include "app_ble_handler.h"
#include "app_linear_motor_handler.h"
#include "app_pdm_handler.h"
#include "bc_ic_led.h"

#if ( HARDWARE_1191_ENABLED == 1)	

#include "bc_wifi_port.h"

#endif	


#if (HARDWARE_153_ENABLED == 1 )	

	
   
#endif

//zxh add for config flash save file space parameters
#if defined(HANDWARE_1_23_3 )
#define FLASH_MAX_SPACE 8 * 1024 * 1024					//Flash 最大可用空间,字节为单位
#define FLASH_MIN_USE_SPACE 1024 * 1024					//一次数据采集需要最大空间，依据项目调整
#define FLASH_RECLAMAT_SPACE FLASH_MIN_USE_SPACE*2		//Flash可用空间小于一次采样最大空间是，删除最早文件需要空出来的空间大小，为避免每次采集都需要删除文件操作，建议一次预留空间大一些
#elif defined(HANDWARE_1_23_2 )
#define FLASH_MAX_SPACE 16 * 1024 * 1024					//Flash 最大可用空间,字节为单位
#define FLASH_MIN_USE_SPACE 1024 * 1024					//一次数据采集需要最大空间，依据项目调整
#define FLASH_RECLAMAT_SPACE FLASH_MIN_USE_SPACE*2		//Flash可用空间小于一次采样最大空间是，删除最早文件需要空出来的空间大小，为避免每次采集都需要删除文件操作，建议一次预留空间大一些
#else
#define FLASH_MAX_SPACE 8 * 1024 * 1024					//Flash 最大可用空间,字节为单位
#define FLASH_MIN_USE_SPACE 1024 * 1024					//一次数据采集需要最大空间，依据项目调整
#define FLASH_RECLAMAT_SPACE FLASH_MIN_USE_SPACE*2		//Flash可用空间小于一次采样最大空间是，删除最早文件需要空出来的空间大小，为避免每次采集都需要删除文件操作，建议一次预留空间大一些
#endif


static struct app_cmd_package app_ppg_file_package = {0};


enum app_file_up_mode
{
  BLE_UP_MODE = 0,
  SPI_UP_MODE
};
 
static enum app_file_up_mode  file_up_mode = BLE_UP_MODE;

enum ppg_file_err
{
	PPG_FILE_SUCCESS = 0,
	PPG_FILE_SIZE_MAX_ERROR,
	PPG_FILE_WRITE_ERROR,
	PPG_FILE_READ_ERROR,
	PPG_FILE_OPEN_ERROR,
	PPG_FILE_CLOSE_ERROR,
	PPG_FILE_SYNC_ERROR,
	PPG_FILE_STATUS_ERROR,
};



enum ppg_fls_status
{
	PPG_FLS_IDIE = 0,
	PPG_FLS_UPLOAD,
	PPG_FLS_WRITE,
	PPG_FLS_BUSY,
	PPG_FLS_SIZE_ERROR,
	
};

#define FILE_NAME_LENG 60
#define FILE_NUMBER_MAX 200

struct ppg_file_hard
{
	lfs_t lfs_fls_ppg_handle;
	lfs_file_t lfs_file_ppg_handle;
	uint32_t ppg_file_size_max;
	uint32_t ppg_file_sys_size_min;
	char ppg_file_name[FILE_NAME_LENG];
	bool ppg_file_status;
	enum ppg_file_type file_type;
	enum ppg_fls_status fls_status;
	uint32_t current_file_write_size;    /* 当前文件已写入数据量，用于滚动切片 */
};

struct ppg_file_one_click_upload
{
  char ppg_file_name[FILE_NUMBER_MAX][FILE_NAME_LENG];
  uint8_t ppg_file_index;
  uint8_t ppg_file_number;
};

static struct ppg_file_one_click_upload  file_one_click_upload = {0};
static struct lfs_info recinfo[50] = {0};


static uint16_t app_ppg_list_files_number_get(struct ppg_file_hard *file_hardle,const char *path);

enum app_ppg_data_task
{
	PPG_FILE_DATA_TASK_TYPE_UPLOAD = 0,
  APP_FILE_RESUME_UPLOAD_TASK_TYPE,
//  APP_FILE_RESUME_STORAGE_TASK_TYPE,
  APP_FILE_ONE_CLICK_UPLOAD_TASK_TYPE,
	PPG_FILE_DATA_TASK_TYPE_NUM
};

static void ppg_file_data_upload_handler_thread(void * p_context);
static void ppg_file_resume_upload_handler_thread(void * p_context);
static void ppg_file_resume_storage_handler_thread(void * p_context);
static void ppg_file_one_click_upload_handler_thread(void * p_context);
static uint32_t ppg_file_sys_size(struct ppg_file_hard *file_hardle);
void lk_ppg_space_reclamation();

static bc_rtos_thread_struct task_thread[PPG_FILE_DATA_TASK_TYPE_NUM] = {
																	{
																	  .thread_name          = "file upload handler task",
																	  .thread_stack_depth   = APP_TASK_FILE_UPLOAD_STACK_SIZE,
																	  .thread_priority      = APP_TASK_FILE_UPLOAD_PRIO,
																	  .thread_parameters    = NULL,
																	  .thread_task_code     = ppg_file_data_upload_handler_thread,
																	}, 
                                  {
																	  .thread_name          = "file resume  up task",
																	  .thread_stack_depth   = APP_TASK_FILE_UPLOAD_STACK_SIZE,
																	  .thread_priority      = APP_TASK_FILE_UPLOAD_PRIO,
																	  .thread_parameters    = NULL,
																	  .thread_task_code     = ppg_file_resume_upload_handler_thread,
																	},
//                                  {
//																	  .thread_name          = "file resume storage task",
//																	  .thread_stack_depth   = APP_TASK_FILE_UPLOAD_STACK_SIZE,
//																	  .thread_priority      = APP_TASK_FILE_UPLOAD_PRIO,
//																	  .thread_parameters    = NULL,
//																	  .thread_task_code     = ppg_file_resume_storage_handler_thread,
//																	},
                                  {
																	  .thread_name          = "file one-click upload task",
																	  .thread_stack_depth   = APP_TASK_FILE_UPLOAD_STACK_SIZE,
																	  .thread_priority      = APP_TASK_FILE_UPLOAD_PRIO,
																	  .thread_parameters    = NULL,
																	  .thread_task_code     = ppg_file_one_click_upload_handler_thread,
																	},
																};

enum app_file_timer_event
{
	APP_FILE_SLICE_STORAGE_TIMER_EVENT = 0,
  APP_FILE_TOMEOUT_TIMER_EVENT,
	APP_FILE_TIMER_NUM
};

static void app_slice_storage_timer_callback(void * pvParameter);
static void app_timeout_timer_callback(void * pvParameter);

static bc_rtos_timer_struct  timer_struct[APP_FILE_TIMER_NUM] = {	
	{
		.timer_name = "slice storage timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 1200,
		.lock = false,
		.timer_callback_function = app_slice_storage_timer_callback,
	},
  {
		.timer_name = "timeout timer",
		.uxAutoReload = true,
		.xTimerPeriodInTicks = 1200,
		.lock = false,
		.timer_callback_function = app_timeout_timer_callback,
	},
};

static void file_up_mode_set(enum app_file_up_mode mode)
{
  file_up_mode = mode;
}

static enum app_file_up_mode file_up_mode_get(void)
{
  return file_up_mode;
}

static struct ppg_file_hard app_ppg_file_hardle = {0};


static bool file_slice_storage_flag = false;
static uint8_t slice_storage_count = 0;
static void ppg_file_slice_storage_event_callback(void * p_context)
{
//  BC_LOG_INFO("file_slice_storage_flag :%d\r\n",file_slice_storage_flag);
	if(file_slice_storage_flag)
	{
		//切片保存处理
		app_ppg_file_close();
    if(app_ppg_list_files_number_get(&app_ppg_file_hardle,"/") >= 72)
    {
//      app_ppg_stop_event();
      slice_storage_count = 0;
      return;
    }
//    lfs_sfud_gc(&app_ppg_file_hardle.lfs_fls_ppg_handle);
		app_ppg_file_open(PPG_FILE_TYPE_PPG_RED_IR_GREEN_TEMPER);
    slice_storage_count++;
		
		file_slice_storage_flag = false;
	}
}

static void app_slice_storage_timer_callback(void * pvParameter)
{
	file_slice_storage_flag = true;
//  BC_LOG_INFO("file_slice_storage_flag :%d\r\n",file_slice_storage_flag);
}

static void app_timeout_timer_callback(void * pvParameter)
{
  app_package_sports_stop();
  bc_rtos_timer_stop(timer_struct[APP_FILE_TOMEOUT_TIMER_EVENT].timer_handler,50);
}

static uint16_t app_ppg_list_files_number_get(struct ppg_file_hard *file_hardle,const char *path) 
{
    lfs_dir_t dir;
	uint32_t file_number = 0;
	uint32_t count_number = 1;
    struct lfs_info info;
	bc_spi_flash_device_open(); 

    // 打开目录
    int err = lfs_dir_open(&file_hardle->lfs_fls_ppg_handle, &dir, path);
    if (err) {
        printf("Failed to open directory %s\n", path);
        return 0;
    }
	
	// 读取目录中的条目
    while (true) {
        err = lfs_dir_read(&file_hardle->lfs_fls_ppg_handle, &dir, &info);
        if (err < 0) {

            break;
        }
        // 检查是否到达目录末尾
        if (err == 0) {
            break;
        }
        // 打印文件/目录信息
        if (info.type == LFS_TYPE_REG) {
			
			file_number++;
        } else if (info.type == LFS_TYPE_DIR) {

        }
        bc_dog_feed();
    }
	
	lfs_dir_seek(&file_hardle->lfs_fls_ppg_handle, &dir, 0);
    // 关闭目录
   lfs_dir_close(&file_hardle->lfs_fls_ppg_handle, &dir);
	bc_spi_flash_device_close();
    return file_number;
}


static uint32_t app_ppg_files_time_get(struct ppg_file_hard *file_hardle,const char *path) 
{
    lfs_dir_t dir;
	uint32_t file_number = 0;
  uint32_t file_temp_count = 0;
  struct lfs_info info;
  uint64_t file_data_time = 0;
  uint32_t data_time = 0;
	bc_spi_flash_device_open(); 
  char file_name[60] = {0};

    // 打开目录
    int err = lfs_dir_open(&file_hardle->lfs_fls_ppg_handle, &dir, path);
    if (err) {
        printf("Failed to open directory %s\n", path);
        return 0;
    }
	
	// 读取目录中的条目
    while (true) {
        err = lfs_dir_read(&file_hardle->lfs_fls_ppg_handle, &dir, &info);
        if (err < 0) {

            break;
        }
        // 检查是否到达目录末尾
        if (err == 0) {
            break;
        }
        // 打印文件/目录信息
        if (info.type == LFS_TYPE_REG) {
        printf("File: %s, Size: %d\n", info.name, info.size);
			file_number++;
        } else if (info.type == LFS_TYPE_DIR) {

        }
    }
	
	lfs_dir_seek(&file_hardle->lfs_fls_ppg_handle, &dir, 0);
	if(file_number  > 0)
	{
		// 读取目录中的条目
		while (true) {
			err = lfs_dir_read(&file_hardle->lfs_fls_ppg_handle, &dir, &info);
			if (err < 0) {
				printf("Failed to read directory\n");
				break;
			}
			// 检查是否到达目录末尾
			if (err == 0) {
				break;
			}
			// 打印文件/目录信息
			if (info.type == LFS_TYPE_REG) {
        file_temp_count++;
        
        if(file_number == file_temp_count)
        {
          lfs_dir_close(&file_hardle->lfs_fls_ppg_handle, &dir);
          printf("time File: %s, Size: %d\n", info.name, info.size);
//          uint32_t ppg_file_pack_size = ((4*3) ) * 5 + 8 ;
          uint32_t ppg_file_pack_size = ((4*3) + (3 *2) + (3 *2) + (3 *2)) * 5 + 8 ;
          if(info.size == 0 || info.size < ppg_file_pack_size)
          {
            data_time = 1735660800;
          }
          else
          {
            uint32_t offset = info.size - ppg_file_pack_size;
            file_name[0] = '/';
            strncat(file_name,info.name,strlen(info.name));
            int error = lfs_file_open(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle,file_name, LFS_O_RDONLY);
            if(error != 0)
            {
              BC_LOG_WARN("ppg open file error,file path:%s \r\n",file_name);
              break;
            }
            lfs_file_seek(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle,offset, LFS_SEEK_SET);
            if(lfs_file_read(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle,(uint8_t*)&file_data_time,sizeof(file_data_time)) != sizeof(file_data_time))
            {
              BC_LOG_WARN("ppg read file error,file path:%s \r\n",file_name);
              break;
            }
            if(lfs_file_close(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle) != 0)
            {
              BC_LOG_WARN("ppg close file error,file path:%s \r\n",file_name);
              break;
            }
            data_time = file_data_time ;
            BC_LOG_INFO("data_time:0x%08x    %d \r\n",data_time,data_time);
            BC_LOG_HEX_P("file_data_time:",(uint8_t*)&file_data_time,sizeof(file_data_time));
            
          }
        }
				

				
			} else if (info.type == LFS_TYPE_DIR) {
				printf("Directory: %s\n", info.name);
			}
		}
	}
	else
	{
		
	}	
    // 关闭目录
    lfs_dir_close(&file_hardle->lfs_fls_ppg_handle, &dir);
	bc_spi_flash_device_close();
  return data_time;
}

static void app_ppg_list_files(struct ppg_file_hard *file_hardle,const char *path) 
{
    lfs_dir_t dir;
	uint32_t file_number = 0;
	uint32_t count_number = 1;
    struct lfs_info info;
	bc_spi_flash_device_open(); 

    // 打开目录
    int err = lfs_dir_open(&file_hardle->lfs_fls_ppg_handle, &dir, path);
    if (err) {
        printf("Failed to open directory %s\n", path);
        return;
    }
	
	// 读取目录中的条目
    while (true) {
        err = lfs_dir_read(&file_hardle->lfs_fls_ppg_handle, &dir, &info);
        if (err < 0) {

            break;
        }
        // 检查是否到达目录末尾
        if (err == 0) {
            break;
        }
        // 打印文件/目录信息
        if (info.type == LFS_TYPE_REG) {
			
			file_number++;
        } else if (info.type == LFS_TYPE_DIR) {

        }
        bc_dog_feed();
    }
	lfs_dir_seek(&file_hardle->lfs_fls_ppg_handle, &dir, 0);
	if(file_number  > 0)
	{
		// 读取目录中的条目
		while (true) {
			err = lfs_dir_read(&file_hardle->lfs_fls_ppg_handle, &dir, &info);
			if (err < 0) {
				printf("Failed to read directory\n");
				break;
			}
			// 检查是否到达目录末尾
			if (err == 0) {
				break;
			}
			// 打印文件/目录信息
			if (info.type == LFS_TYPE_REG) {
				printf("File: %s, Size: %d\n", info.name, info.size);
				*(uint32_t*)&app_ppg_file_package.data[0] = file_number;
				*(uint32_t*)&app_ppg_file_package.data[4] = count_number;
				*(uint32_t*)&app_ppg_file_package.data[8] = info.size;
				memcpy((uint8_t*)&app_ppg_file_package.data[12],info.name,strlen(info.name));
                
				app_package_ppg_file_uplaod(&app_ppg_file_package,12 + strlen(info.name));
				count_number++;
				
			} else if (info.type == LFS_TYPE_DIR) {
				printf("Directory: %s\n", info.name);
			}
            bc_dog_feed();
		}
	}
	else
	{
		*(uint32_t*)&app_ppg_file_package.data[0] = 0;
		*(uint32_t*)&app_ppg_file_package.data[4] = 0;
		*(uint32_t*)&app_ppg_file_package.data[8] = info.size;
		memcpy((uint8_t*)&app_ppg_file_package.data[12],info.name,strlen(info.name));
		
		app_package_ppg_file_uplaod(&app_ppg_file_package,12 + strlen(info.name));
	}	
    // 关闭目录
    lfs_dir_close(&file_hardle->lfs_fls_ppg_handle, &dir);
	bc_spi_flash_device_close();
}

static uint16_t app_ppg_list_files_info_get(struct ppg_file_hard *file_hardle,const char *path,struct ppg_file_one_click_upload  *one_click_upload) 
{
    lfs_dir_t dir;
	uint32_t file_number = 0;
	uint32_t count_number = 1;
    struct lfs_info info;
	bc_spi_flash_device_open(); 
    // 打开目录
    int err = lfs_dir_open(&file_hardle->lfs_fls_ppg_handle, &dir, path);
    if (err) {
        printf("Failed to open directory %s\n", path);
        return 0;
    }
	
	// 读取目录中的条目
    while (true) {
        err = lfs_dir_read(&file_hardle->lfs_fls_ppg_handle, &dir, &info);
        if (err < 0) {

            break;
        }
        // 检查是否到达目录末尾
        if (err == 0) {
            break;
        }
        // 打印文件/目录信息
        if (info.type == LFS_TYPE_REG) {
			
			file_number++;
        } else if (info.type == LFS_TYPE_DIR) {

        }
        bc_dog_feed();
    }
    printf("file_number:%d \r\n",file_number);
    lfs_dir_seek(&file_hardle->lfs_fls_ppg_handle, &dir, 0);
    if(file_number  > 0)
    {
      // 读取目录中的条目
      while (true) {
        err = lfs_dir_read(&file_hardle->lfs_fls_ppg_handle, &dir, &info);
        if (err < 0) {
          printf("Failed to read directory\n");
          break;
        }
        // 检查是否到达目录末尾
        if (err == 0) {
          break;
        }
        // 打印文件/目录信息
        if (info.type == LFS_TYPE_REG) {
          printf("File: %s, Size: %d\n", info.name, info.size);
          memcpy((uint8_t*)&one_click_upload->ppg_file_name[count_number][0],info.name,strlen(info.name));
          
          count_number++;
          
        } else if (info.type == LFS_TYPE_DIR) {
          printf("Directory: %s\n", info.name);
        }
        bc_dog_feed();
      }
    }
	one_click_upload->ppg_file_number = count_number - 1;
	lfs_dir_seek(&file_hardle->lfs_fls_ppg_handle, &dir, 0);
    // 关闭目录
   lfs_dir_close(&file_hardle->lfs_fls_ppg_handle, &dir);
	bc_spi_flash_device_close();
    return file_number;
}

static enum ppg_file_err lk_ppg_file_open(struct ppg_file_hard *file_hardle) 
{
	//bc_spi_flash_device_open();  
    #if 1
    uint8_t icnt = 5;
    do {
        if(!lfs_file_open(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle,file_hardle->ppg_file_name, LFS_O_RDWR | LFS_O_CREAT))
            break;
        else {
            BC_LOG_WARN("ppg open file error,retry. file path:%s \r\n",file_hardle->ppg_file_name);
            bc_delay_ms(3);
        }
    } while(icnt--);
    if(!icnt) {
        BC_LOG_WARN("ppg open file error,file path:%s \r\n",file_hardle->ppg_file_name);
        //bc_spi_flash_device_close();
		return PPG_FILE_OPEN_ERROR;
    }
    #else
	if(lfs_file_open(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle,file_hardle->ppg_file_name, LFS_O_RDWR | LFS_O_CREAT) != 0)
	{
		BC_LOG_WARN("ppg open file error,file path:%s \r\n",file_hardle->ppg_file_name);
    bc_spi_flash_device_close();
		return PPG_FILE_OPEN_ERROR;
	}
    #endif
	file_hardle->ppg_file_status = true;
  //bc_spi_flash_device_close();
	return PPG_FILE_SUCCESS;
	
}



static enum ppg_file_err ppg_file_open(struct ppg_file_hard *file_hardle) 
{
	bc_spi_flash_device_open();  
    #if 1
    uint8_t icnt = 5;
    do {
        if(!lfs_file_open(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle,file_hardle->ppg_file_name, LFS_O_RDWR | LFS_O_CREAT))
            break;
        else {
            BC_LOG_WARN("ppg open file error,retry. file path:%s \r\n",file_hardle->ppg_file_name);
            bc_delay_ms(3);
        }
    } while(icnt--);
    if(!icnt) {
        BC_LOG_WARN("ppg open file error,file path:%s \r\n",file_hardle->ppg_file_name);
        bc_spi_flash_device_close();
		return PPG_FILE_OPEN_ERROR;
    }
    #else
	if(lfs_file_open(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle,file_hardle->ppg_file_name, LFS_O_RDWR | LFS_O_CREAT) != 0)
	{
		BC_LOG_WARN("ppg open file error,file path:%s \r\n",file_hardle->ppg_file_name);
    bc_spi_flash_device_close();
		return PPG_FILE_OPEN_ERROR;
	}
    #endif
	file_hardle->ppg_file_status = true;
  bc_spi_flash_device_close();
	return PPG_FILE_SUCCESS;
	
}

static uint32_t ppg_file_size(struct ppg_file_hard *file_hardle) 
{
	return lfs_file_size(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle);
}

static uint32_t ppg_file_sys_size(struct ppg_file_hard *file_hardle)
{
    #if 1 // by liukun
    lfs_size_t total_size = file_hardle->lfs_fls_ppg_handle.cfg->block_size * file_hardle->lfs_fls_ppg_handle.cfg->block_count;
    #else
	lfs_size_t total_size = 4096 * 1024;
    #endif

    // 获取已使用空间大小（使用的块数 * 每块的大小）
    lfs_ssize_t used_blocks = lfs_fs_size(&file_hardle->lfs_fls_ppg_handle);
    lfs_size_t used_size = (used_blocks >= 0) ? used_blocks * file_hardle->lfs_fls_ppg_handle.cfg->block_size : 0;

    // 计算可用空间大小（总空间大小 - 已使用空间大小）
    lfs_size_t available_size = total_size - used_size;
	
	
	return available_size;
}

static enum ppg_file_err ppg_file_close(struct ppg_file_hard *file_hardle) 
{
  bc_spi_flash_device_open();
	if(lfs_file_close(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle) != 0)
	{
		BC_LOG_WARN("ppg close file error,file path:%s \r\n",file_hardle->ppg_file_name);
		bc_spi_flash_device_close();
		file_hardle->ppg_file_status = false;
		return PPG_FILE_CLOSE_ERROR;
	}
	bc_spi_flash_device_close();
	file_hardle->ppg_file_status = false;
	return PPG_FILE_SUCCESS;
}

static enum ppg_file_err lk_ppg_file_close(struct ppg_file_hard *file_hardle) 
{
  //bc_spi_flash_device_open();
	if(lfs_file_close(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle) != 0)
	{
		BC_LOG_WARN("ppg close file error,file path:%s \r\n",file_hardle->ppg_file_name);
		bc_spi_flash_device_close();
		file_hardle->ppg_file_status = false;
		return PPG_FILE_CLOSE_ERROR;
	}
	//bc_spi_flash_device_close();
	file_hardle->ppg_file_status = false;
	return PPG_FILE_SUCCESS;
}

static uint16_t temp_sync_count = 0;
static enum ppg_file_err ppg_file_write(struct ppg_file_hard *file_hardle,uint8_t *write_buff,uint32_t write_length) 
{
//  bc_spi_flash_device_open();
	if(!file_hardle->ppg_file_status)
	{
		BC_LOG_WARN("ppg status file error,stats:%d ,file path:%s \r\n",file_hardle->ppg_file_status,file_hardle->ppg_file_name);
//    bc_spi_flash_device_close();
		return PPG_FILE_STATUS_ERROR;
	}
	
#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	

//	uint32_t file_sys_size = ppg_file_sys_size(file_hardle);
//	if(file_sys_size <= app_ppg_file_hardle.ppg_file_sys_size_min)
//	{
//		app_ppg_file_close();
//		return PPG_FILE_SIZE_MAX_ERROR;
//	}
//	uint32_t temp_size = lfs_file_size(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle);
//	if((temp_size + write_length) > file_hardle->ppg_file_size_max)
//	{
//		BC_LOG_WARN("ppg file length beyond,file size:%d  write size:%d  file size max:%d \r\n",temp_size,write_length,file_hardle->ppg_file_size_max);
//		app_ppg_file_close();
//		return PPG_FILE_SIZE_MAX_ERROR;
//	}

#endif		

	
	if(lfs_file_write(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle,write_buff, write_length) != write_length)
	{
		BC_LOG_WARN("ppg write file error,file path:%s \r\n",file_hardle->ppg_file_name);
//    bc_spi_flash_device_close();
		return PPG_FILE_WRITE_ERROR;
	}
#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	

//  if(temp_sync_count < 200)
//  {
//    temp_sync_count++;
//  }
//  else
//  {
//    if(lfs_file_sync(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle) != 0)
//    {
//      BC_LOG_WARN("ppg sync file error,file path:%s \r\n",file_hardle->ppg_file_name);
//      return PPG_FILE_SYNC_ERROR;
//    }
//    temp_sync_count = 0;
//  }

#endif	
    
#if (defined(HANDWARE_1_23_2 ) || defined(HANDWARE_1_23_3 ) || defined(HANDWARE_1_23_4 ))
  if(temp_sync_count < 300)
  {
    temp_sync_count++;
  }
  else
  {
      uint32_t ret = lfs_file_sync(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle);
    if(ret)
    {
        BC_LOG_WARN("ppg sync file error,file path:%s error:%d\r\n",file_hardle->ppg_file_name, ret);
      return PPG_FILE_SYNC_ERROR;
    }
    temp_sync_count = 0;
  }
#endif
  
	return PPG_FILE_SUCCESS;
}

static enum ppg_file_err lk_ppg_file_read(struct ppg_file_hard *file_hardle,uint8_t *read_buff,uint32_t read_length) 
{
  //bc_spi_flash_device_open();
	if(!file_hardle->ppg_file_status)
	{
		BC_LOG_WARN("ppg status file error,stats:%d ,file path:%s \r\n",file_hardle->ppg_file_status,file_hardle->ppg_file_name);
		return PPG_FILE_STATUS_ERROR;
	}
	if(lfs_file_read(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle,read_buff,read_length) != read_length)
	{
		BC_LOG_WARN("ppg read file error,file path:%s \r\n",file_hardle->ppg_file_name);
		return PPG_FILE_READ_ERROR;
	}
	return PPG_FILE_SUCCESS;
}

static enum ppg_file_err ppg_file_read(struct ppg_file_hard *file_hardle,uint8_t *read_buff,uint32_t read_length) 
{
  bc_spi_flash_device_open();
	if(!file_hardle->ppg_file_status)
	{
		BC_LOG_WARN("ppg status file error,stats:%d ,file path:%s \r\n",file_hardle->ppg_file_status,file_hardle->ppg_file_name);
		return PPG_FILE_STATUS_ERROR;
	}
	if(lfs_file_read(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle,read_buff,read_length) != read_length)
	{
		BC_LOG_WARN("ppg read file error,file path:%s \r\n",file_hardle->ppg_file_name);
		return PPG_FILE_READ_ERROR;
	}
	return PPG_FILE_SUCCESS;
}

static void ppg_file_seek(struct ppg_file_hard *file_hardle,enum lfs_whence_flags flags) 
{
  
	lfs_file_seek(&file_hardle->lfs_fls_ppg_handle, &file_hardle->lfs_file_ppg_handle,0, flags);
}




uint32_t ppg_file_delete(struct ppg_file_hard *file_hardle,char *path)
{
	return lfs_remove(&file_hardle->lfs_fls_ppg_handle,path);
}

static void ppg_file_name_create(enum ppg_file_type file_type)
{
	uint8_t temp_id[6] = {0x01,0x02,0x03,0x04,0x05,0x06};
	
	app_ppg_file_hardle.ppg_file_name[0] = '/';
	splitHexArray(temp_id, sizeof(temp_id), &app_ppg_file_hardle.ppg_file_name[1], 20);
	
    app_ppg_file_hardle.ppg_file_name[1 + (sizeof(temp_id) *2)] = '_';
	
//	uint64_t time = bg_rtc_time_get_uinx_ms_time();
//	splitHexArray((uint8_t*)&time, 8, &app_ppg_file_hardle.ppg_file_name[1 + (sizeof(temp_id) *2) + 1], 20);
  bc_rtc_format_beijing_time(&app_ppg_file_hardle.ppg_file_name[1 + (sizeof(temp_id) *2) + 1], 20);
	
	switch(file_type)
	{
		case PPG_FILE_TYPE_ACC:
		{
			char temp[8] = "_1.bin";
			strncat(app_ppg_file_hardle.ppg_file_name,temp,strlen(temp));
			break;
		}
		case PPG_FILE_TYPE_ACC_GRYO:
		{
			char temp[8] = "_2.bin";
			strncat(app_ppg_file_hardle.ppg_file_name,temp,strlen(temp));
			break;
		}
		case PPG_FILE_TYPE_SPO2:
		{
			char temp[8] = "_3.bin";
			strncat(app_ppg_file_hardle.ppg_file_name,temp,strlen(temp));
			break;
		}
		case PPG_FILE_TYPE_HR:
		{
			char temp[8] = "_4.bin";
			strncat(app_ppg_file_hardle.ppg_file_name,temp,strlen(temp));
			break;
		}
		case PPG_FILE_TYPE_PPG_IR:
		{
			char temp[8] = "_5.bin";
			strncat(app_ppg_file_hardle.ppg_file_name,temp,strlen(temp));
			break;
		}
		case PPG_FILE_TYPE_TEMP:
		{
			char temp[8] = "_6.bin";
			strncat(app_ppg_file_hardle.ppg_file_name,temp,strlen(temp));
			break;
		}
		case PPG_FILE_TYPE_PPG_RED_IR_GREEN_TEMPER:
		{
			char temp[8] = "_7.bin";
			strncat(app_ppg_file_hardle.ppg_file_name,temp,strlen(temp));
			break;
		}
    case PPG_FILE_TYPE_16K_2_MIC_ADPCM:
    {
      char temp[8] = "_8.bin";
			strncat(app_ppg_file_hardle.ppg_file_name,temp,strlen(temp));
      break;
    }
    case PPG_FILE_TYPE_16K_2_MIC_ADPCM_CAPTURE:
    {
      char temp[8] = "_B.bin";
			strncat(app_ppg_file_hardle.ppg_file_name,temp,strlen(temp));
      break;
    }
    case PPG_FILE_TYPE_16K_2_MIC_OPUS:
    {
      char temp[8] = "_9.bin";
			strncat(app_ppg_file_hardle.ppg_file_name,temp,strlen(temp));
      break;
    }  
    case PPG_FILE_TYPE_16K_2_MIC_OPUS_CAPTURE:
    {
      char temp[8] = "_C.bin";
			strncat(app_ppg_file_hardle.ppg_file_name,temp,strlen(temp));
      break;
    } 
    case PPG_FILE_TYPE_8K_1_MIC_ADPCM:
    {
      char temp[8] = "_D.bin";
			strncat(app_ppg_file_hardle.ppg_file_name,temp,strlen(temp));
      break;
    }
    case PPG_FILE_TYPE_8K_1_MIC_OPUS:
    {
      char temp[8] = "_E.bin";
			strncat(app_ppg_file_hardle.ppg_file_name,temp,strlen(temp));
      break;
    }    
		default:
		{
			break;
		}
	}
}

//static char new_name[FILE_NAME_LENG] = {0};
static char new_name[FILE_NAME_LENG] = "/45678925345454545454545.txt";
static bool ppg_file_rename(enum ppg_file_type file_type)
{
//	uint8_t temp_id[6] = {0x01,0x02,0x03,0x04,0x05,0x06};
//	
//	uint8_t temp_length = 1 + (sizeof(temp_id) *2) + 1 +(sizeof(uint64_t) * 2) +1; // "/"+id+'_'+uint64_t+'_'
//	memcpy(new_name,app_ppg_file_hardle.ppg_file_name,temp_length);
//	uint64_t time = bg_rtc_time_get_uinx_ms_time();
//	splitHexArray((uint8_t*)&time, 8, &new_name[temp_length], 20);
//	switch(file_type)
//	{
//		case PPG_FILE_TYPE_ACC:
//		{
//			char temp[8] = "_1.bin";
//			strncat(new_name,temp,strlen(temp));
//			break;
//		}
//		case PPG_FILE_TYPE_ACC_GRYO:
//		{
//			char temp[8] = "_2.bin";
//			strncat(new_name,temp,strlen(temp));
//			break;
//		}
//		case PPG_FILE_TYPE_SPO2:
//		{
//			char temp[8] = "_3.bin";
//			strncat(new_name,temp,strlen(temp));
//			break;
//		}
//		case PPG_FILE_TYPE_HR:
//		{
//			char temp[8] = "_4.bin";
//			strncat(new_name,temp,strlen(temp));
//			break;
//		}
//		case PPG_FILE_TYPE_PPG_IR:
//		{
//			char temp[8] = "_5.bin";
//			strncat(new_name,temp,strlen(temp));
//			break;
//		}
//		case PPG_FILE_TYPE_TEMP:
//		{
//			char temp[8] = "_6.bin";
//			strncat(new_name,temp,strlen(temp));
//			break;
//		}
//		case PPG_FILE_TYPE_PPG_RED_IR_GREEN_TEMPER:
//		{
//			char temp[8] = "_7.";
//			strncat(new_name,temp,strlen(temp));
//			break;
//		}		
//		default:
//		{
//			break;
//		}
//	}
	BC_LOG_INFO("new_name:%s ",new_name);
	BC_LOG_INFO("app_ppg_file_hardle.ppg_file_name:%s ",app_ppg_file_hardle.ppg_file_name);
	int ret = lfs_rename(&app_ppg_file_hardle.lfs_fls_ppg_handle, app_ppg_file_hardle.ppg_file_name,new_name);
	BC_LOG_INFO("ret:%d ",ret);
	if(ret < 0)
	{
		return false;
	}
	return true;
}

static void ppg_file_data_upload_handler_thread(void * p_context)
{
	uint32_t ppg_file_pack_number = 0;
	uint32_t ppg_file_pack_count = 1;
	uint32_t ppg_file_pack_size = 0;
	uint32_t ppg_file_temp_size = 0;
	uint32_t file_size = 0;
  lfs_sfud_init(&app_ppg_file_hardle.lfs_fls_ppg_handle);
#if ( HARDWARE_1191_ENABLED == 1)	

//    bc_wifi_device_enable();
  bc_wifi_device_disable();

#endif	  
  
  bc_rtos_thread_suspend(task_thread[PPG_FILE_DATA_TASK_TYPE_UPLOAD].thread_handler);
  while(true)
  {
    ppg_file_pack_number = 0;
	  ppg_file_pack_count = 1;
	  ppg_file_pack_size = 0;
	  ppg_file_temp_size = 0;
	  file_size = 0;
      bc_spi_flash_device_open();  
    if(lk_ppg_file_open(&app_ppg_file_hardle) != 0)
    {
      lk_ppg_file_close(&app_ppg_file_hardle);
        bc_spi_flash_device_close();
      return ;
    }
    file_size = ppg_file_size(&app_ppg_file_hardle);
  ; 
    switch(app_ppg_file_hardle.file_type)
    {
      case PPG_FILE_TYPE_ACC:
      {
        break;
      }
      case PPG_FILE_TYPE_ACC_GRYO:
      {
        break;
      }
      case PPG_FILE_TYPE_SPO2:
      {
        ppg_file_pack_size = ((4*2) + (3 *2)) * 15;
        break;
      }
      case PPG_FILE_TYPE_HR:
      {
        ppg_file_pack_size = ((4*1) + (3 *2)) * 20;
        break;
      }
      case PPG_FILE_TYPE_PPG_IR:
      {
        ppg_file_pack_size = (4*1) * 50;
        break;
      }
      case PPG_FILE_TYPE_TEMP:
      {
        break;
      }
      case PPG_FILE_TYPE_PPG_RED_IR_GREEN_TEMPER:
      {
#if ( HARDWARE_1141_ENABLED == 1 ||  HARDWARE_451_ENABLED == 1)	

        ppg_file_pack_size = ((4*3) + (3 *2) + (3 *2) + (3 *2)) * 5 + 8;
#else
        ppg_file_pack_size = ((4*3) + (3 *2) + 2) * 10;
#endif			
        
        break;
      }
      case PPG_FILE_TYPE_16K_2_MIC_ADPCM:
      case PPG_FILE_TYPE_16K_2_MIC_ADPCM_CAPTURE:
      case PPG_FILE_TYPE_16K_2_MIC_OPUS:
      case PPG_FILE_TYPE_16K_2_MIC_OPUS_CAPTURE:
      case PPG_FILE_TYPE_8K_1_MIC_ADPCM:
      case PPG_FILE_TYPE_8K_1_MIC_OPUS:
      {
        ppg_file_pack_size = PDM_DATA_SEND_SIZE;
        break;
      }
      default:
      {
        break;
      }
    }
    
    if(file_size % ppg_file_pack_size == 0)
    {
      ppg_file_pack_number = file_size / ppg_file_pack_size;
    }
    else
    {
      ppg_file_pack_number = (file_size / ppg_file_pack_size) + 1;
    }
    //bc_spi_flash_device_open(); 
    ppg_file_seek(&app_ppg_file_hardle,LFS_SEEK_SET);
    app_ble_conn_time_audio_set();
    while(ppg_file_pack_count < ppg_file_pack_number+1)
    {
      if((file_size - (ppg_file_pack_size * (ppg_file_pack_count - 1))) >= ppg_file_pack_size)
      {
        app_ppg_file_package.data[0] = PPG_FLS_UPLOAD;
        memcpy(&app_ppg_file_package.data[1], &file_size, sizeof(uint32_t));
        memcpy(&app_ppg_file_package.data[5], &ppg_file_pack_number, sizeof(uint32_t));
        memcpy(&app_ppg_file_package.data[9], &ppg_file_pack_count, sizeof(uint32_t));
        memcpy(&app_ppg_file_package.data[13], &ppg_file_pack_size, sizeof(uint32_t));
            BC_LOG_INFO("ppg_file_pack_number:%d ppg_file_pack_count:%d  ppg_file_pack_size:%d\r\n",ppg_file_pack_number,ppg_file_pack_count,ppg_file_pack_size);
        lk_ppg_file_read(&app_ppg_file_hardle,&app_ppg_file_package.data[17],ppg_file_pack_size);

       if(file_up_mode_get() == BLE_UP_MODE)
       {
         app_package_ppg_file_uplaod(&app_ppg_file_package,17 + ppg_file_pack_size);
         
       }
       else if(file_up_mode_get() == SPI_UP_MODE)
       {
          app_package_file_spi_uplaod(&app_ppg_file_package,17 + ppg_file_pack_size);
          bc_delay_ms(17);
       }


        
      }   
      else
      {
            
             ppg_file_temp_size = file_size - (ppg_file_pack_size * (ppg_file_pack_count - 1));	
        
        app_ppg_file_package.data[0] = PPG_FLS_UPLOAD;
              memcpy(&app_ppg_file_package.data[1], &file_size, sizeof(uint32_t));
        memcpy(&app_ppg_file_package.data[5], &ppg_file_pack_number, sizeof(uint32_t));
        memcpy(&app_ppg_file_package.data[9], &ppg_file_pack_count, sizeof(uint32_t));
        memcpy(&app_ppg_file_package.data[13], &ppg_file_temp_size, sizeof(uint32_t));
        BC_LOG_INFO("ppg_file_pack_number:%d ppg_file_pack_count:%d  ppg_file_pack_size:%d\r\n",ppg_file_pack_number,ppg_file_pack_count,ppg_file_pack_size);
        lk_ppg_file_read(&app_ppg_file_hardle,&app_ppg_file_package.data[17],ppg_file_temp_size);
      if(file_up_mode_get() == BLE_UP_MODE)
       {
         app_package_ppg_file_uplaod(&app_ppg_file_package,17 + ppg_file_temp_size);
         
       }
       else if(file_up_mode_get() == SPI_UP_MODE)
       {
          app_package_file_spi_uplaod(&app_ppg_file_package,17 + ppg_file_temp_size);
          bc_delay_ms(17);
       }

        
      }
      ppg_file_pack_count++;
      bc_dog_feed();
    }
    app_ble_conn_time_audio_reset();
    BC_LOG_INFO("ppg_file_pack_number:%d ppg_file_pack_count:%d  ppg_file_pack_size:%d\r\n",ppg_file_pack_number,ppg_file_pack_count,ppg_file_pack_size);
    
    for(uint8_t close_count = 0; close_count < 3;close_count++)
    {
      if(lk_ppg_file_close(&app_ppg_file_hardle) == 0)
      {
        break;
      }
    }
    bc_spi_flash_device_close(); 

#if defined(HANDWARE_1_23_2_ONE_SEC)
    /* 录音文件上传完成：短振1次 */
    switch(app_ppg_file_hardle.file_type)
    {
      case PPG_FILE_TYPE_16K_2_MIC_ADPCM:
      case PPG_FILE_TYPE_16K_2_MIC_ADPCM_CAPTURE:
      case PPG_FILE_TYPE_16K_2_MIC_OPUS:
      case PPG_FILE_TYPE_16K_2_MIC_OPUS_CAPTURE:
      case PPG_FILE_TYPE_8K_1_MIC_ADPCM:
      case PPG_FILE_TYPE_8K_1_MIC_OPUS:
        app_vibrate_start(VIBRATE_MODE_SHORT, 1);
        break;
      default:
        break;
    }
#endif

    app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;
    bc_rtos_thread_suspend(task_thread[PPG_FILE_DATA_TASK_TYPE_UPLOAD].thread_handler);
  }
	
}


static uint32_t ppg_file_resume_upload_offset = 0;
static void ppg_file_resume_upload_handler_thread(void * p_context)
{
	uint32_t ppg_file_pack_number = 0;
	uint32_t ppg_file_pack_count = 1;
	uint32_t ppg_file_pack_size = 0;
	uint32_t ppg_file_temp_size = 0;
	uint32_t file_size = 0;
  uint8_t temp_count = 0;
  
  while(true)
  { 
    
    
    ppg_file_pack_number = 0;
    ppg_file_pack_count = 1;
    ppg_file_pack_size = 0;
    ppg_file_temp_size = 0;
    file_size = 0;
    temp_count = 0;
    
    if(ppg_file_open(&app_ppg_file_hardle) != 0)
    {
      ppg_file_close(&app_ppg_file_hardle);
      return ;
    }
    file_size = ppg_file_size(&app_ppg_file_hardle);
    file_size -= ppg_file_resume_upload_offset;
    switch(app_ppg_file_hardle.file_type)
    {
      case PPG_FILE_TYPE_ACC:
      {
        break;
      }
      case PPG_FILE_TYPE_ACC_GRYO:
      {
        break;
      }
      case PPG_FILE_TYPE_SPO2:
      {
        ppg_file_pack_size = ((4*2) + (3 *2)) * 15;
        break;
      }
      case PPG_FILE_TYPE_HR:
      {
        ppg_file_pack_size = ((4*1) + (3 *2)) * 20;
        break;
      }
      case PPG_FILE_TYPE_PPG_IR:
      {
        ppg_file_pack_size = (4*1) * 50;
        break;
      }
      case PPG_FILE_TYPE_TEMP:
      {
        break;
      }
      case PPG_FILE_TYPE_PPG_RED_IR_GREEN_TEMPER:
      {
  #if ( HARDWARE_1141_ENABLED == 1 ||  HARDWARE_451_ENABLED == 1)	

        ppg_file_pack_size = ((4*3) + (3 *2) + (3 *2) + (3 *2)) * 5 + 8 ;
  //			ppg_file_pack_size = ((4*3) + (3 *2) + (3 *2) + (3 *2)) * 5 + 8+64;
  //      ppg_file_pack_size = (((4*3) + (3 *2) + (3 *2) + (3 *2)) * 5 + 8) * 6;
  #else
        ppg_file_pack_size = ((4*3) + (3 *2) + 2) * 10;
  #endif			
        
        break;
      }
      case PPG_FILE_TYPE_16K_2_MIC_ADPCM:
      case PPG_FILE_TYPE_16K_2_MIC_ADPCM_CAPTURE:
      case PPG_FILE_TYPE_16K_2_MIC_OPUS:
      case PPG_FILE_TYPE_16K_2_MIC_OPUS_CAPTURE:
      case PPG_FILE_TYPE_8K_1_MIC_ADPCM:
      case PPG_FILE_TYPE_8K_1_MIC_OPUS:
      {
        ppg_file_pack_size = PDM_DATA_SEND_SIZE;
        break;
      }
      default:
      {
        break;
      }
    }
    
    if(file_size % ppg_file_pack_size == 0)
    {
      ppg_file_pack_number = file_size / ppg_file_pack_size;
    }
    else
    {
      ppg_file_pack_number = (file_size / ppg_file_pack_size) + 1;
    }
    bc_spi_flash_device_open(); 
    ppg_file_seek(&app_ppg_file_hardle,LFS_SEEK_SET);

    while(ppg_file_pack_count < ppg_file_pack_number + 1)
    {

      if((file_size - (ppg_file_pack_size * (ppg_file_pack_count - 1))) >= ppg_file_pack_size)
      {
        app_ppg_file_package.data[0] = PPG_FLS_UPLOAD;
        memcpy(&app_ppg_file_package.data[1], &file_size, sizeof(uint32_t));
        memcpy(&app_ppg_file_package.data[5], &ppg_file_pack_number, sizeof(uint32_t));
        memcpy(&app_ppg_file_package.data[9], &ppg_file_pack_count, sizeof(uint32_t));
        memcpy(&app_ppg_file_package.data[13], &ppg_file_pack_size, sizeof(uint32_t));
        BC_LOG_INFO("ppg_file_pack_number:%d ppg_file_pack_count:%d  ppg_file_pack_size:%d\r\n",ppg_file_pack_number,ppg_file_pack_count,ppg_file_pack_size);
        ppg_file_read(&app_ppg_file_hardle,&app_ppg_file_package.data[17],ppg_file_pack_size);
      

        app_package_ppg_file_uplaod(&app_ppg_file_package,17 + ppg_file_pack_size);
      }   
      else
      {
            
             ppg_file_temp_size = file_size - (ppg_file_pack_size * (ppg_file_pack_count - 1));	
        
        app_ppg_file_package.data[0] = PPG_FLS_UPLOAD;
              memcpy(&app_ppg_file_package.data[1], &file_size, sizeof(uint32_t));
        memcpy(&app_ppg_file_package.data[5], &ppg_file_pack_number, sizeof(uint32_t));
        memcpy(&app_ppg_file_package.data[9], &ppg_file_pack_count, sizeof(uint32_t));
        memcpy(&app_ppg_file_package.data[13], &ppg_file_temp_size, sizeof(uint32_t));
        BC_LOG_INFO("ppg_file_pack_number:%d ppg_file_pack_count:%d  ppg_file_pack_size:%d\r\n",ppg_file_pack_number,ppg_file_pack_count,ppg_file_pack_size);
        ppg_file_read(&app_ppg_file_hardle,&app_ppg_file_package.data[17],ppg_file_temp_size);
        app_package_ppg_file_uplaod(&app_ppg_file_package,17 + ppg_file_temp_size);
  //			app_ble_send((uint8_t*)&upload_file_package,4+17+ppg_file_pack_size);
      }
      ppg_file_pack_count++;
      bc_dog_feed();
    }
    BC_LOG_INFO("ppg_file_pack_number:%d ppg_file_pack_count:%d  ppg_file_pack_size:%d\r\n",ppg_file_pack_number,ppg_file_pack_count,ppg_file_pack_size);
    bc_spi_flash_device_close(); 
    if(ppg_file_close(&app_ppg_file_hardle) != 0)
    {
#if defined(HANDWARE_1_23_2_ONE_SEC)
      /* 录音文件上传完成：短振1次 */
      switch(app_ppg_file_hardle.file_type)
      {
        case PPG_FILE_TYPE_16K_2_MIC_ADPCM:
        case PPG_FILE_TYPE_16K_2_MIC_ADPCM_CAPTURE:
        case PPG_FILE_TYPE_16K_2_MIC_OPUS:
        case PPG_FILE_TYPE_16K_2_MIC_OPUS_CAPTURE:
        case PPG_FILE_TYPE_8K_1_MIC_ADPCM:
        case PPG_FILE_TYPE_8K_1_MIC_OPUS:
          app_vibrate_start(VIBRATE_MODE_SHORT, 1);
          break;
        default:
          break;
      }
#endif
      app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;
      bc_rtos_thread_suspend(task_thread[APP_FILE_RESUME_UPLOAD_TASK_TYPE].thread_handler);
      return;
    }
#if defined(HANDWARE_1_23_2_ONE_SEC)
    /* 录音文件上传完成：短振1次 */
    switch(app_ppg_file_hardle.file_type)
    {
      case PPG_FILE_TYPE_16K_2_MIC_ADPCM:
      case PPG_FILE_TYPE_16K_2_MIC_ADPCM_CAPTURE:
      case PPG_FILE_TYPE_16K_2_MIC_OPUS:
      case PPG_FILE_TYPE_16K_2_MIC_OPUS_CAPTURE:
      case PPG_FILE_TYPE_8K_1_MIC_ADPCM:
      case PPG_FILE_TYPE_8K_1_MIC_OPUS:
        app_vibrate_start(VIBRATE_MODE_SHORT, 1);
        break;
      default:
        break;
    }
#endif
    app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;
    bc_rtos_thread_suspend(task_thread[APP_FILE_RESUME_UPLOAD_TASK_TYPE].thread_handler);
  }
	
}

static uint8_t ppg_file_one_click_upload_index = 1;

static void ppg_file_one_click_upload_handler_thread(void * p_context)
{
 	uint32_t ppg_file_pack_number = 0;
	uint32_t ppg_file_pack_count = 1;
	uint32_t ppg_file_pack_size = 0;
	uint32_t ppg_file_temp_size = 0;
	uint32_t file_size = 0;
  uint8_t temp_count = 0;
  uint8_t progress_count = 0;
  uint8_t all_up_progress_count = 0;
  uint32_t start_time_unix = 0;
  uint32_t stop_time_unix = 0;
  uint32_t one_file_start_time_unix = 0;
  uint32_t one_file_stop_time_unix = 0;
  
  while(true)
  {      
    ppg_file_pack_number = 0;
    ppg_file_pack_count = 1;
    ppg_file_pack_size = 0;
    ppg_file_temp_size = 0;
    file_size = 0;
    temp_count = 0;
    progress_count = 0;
    all_up_progress_count = 0;
    start_time_unix = 0;
    stop_time_unix = 0;
    one_file_start_time_unix = 0;
    one_file_stop_time_unix = 0;
#if defined(HANDWARE_1_23_2_ONE_SEC)
    bool has_audio_file = false;  /* 标记是否有录音文件 */
#endif
    BC_LOG_INFO("ppg_file_one_click_upload_handler_thread***********\r\n");
    app_ppg_list_files_info_get(&app_ppg_file_hardle,"/",&file_one_click_upload);
    file_one_click_upload.ppg_file_index = ppg_file_one_click_upload_index;
    BC_LOG_INFO("llll***********\r\n");
    //上传文件开始
      app_ble_conn_time_audio_set();
    app_ppg_file_package.subcmd = 0x1A;
#if defined(HANDWARE_1_23_2_ONE_SEC)
    bc_ic_led_file_sync_start();
#endif
    start_time_unix = bg_rtc_time_get_uinx_time();
    app_ppg_file_package.data[0] = 0x01;
    memcpy(&app_ppg_file_package.data[1], &start_time_unix, sizeof(uint32_t));
    memcpy(&app_ppg_file_package.data[5], &stop_time_unix, sizeof(uint32_t));
    app_package_ppg_file_uplaod(&app_ppg_file_package,4+9);
    for(uint8_t index = ppg_file_one_click_upload_index; index < file_one_click_upload.ppg_file_number+1;index++)
    {
      
      ppg_file_pack_number = 0;
    ppg_file_pack_count = 1;
     ppg_file_pack_size = 0;
    ppg_file_temp_size = 0;
    file_size = 0;
    temp_count = 0;
     progress_count = 0;
    stop_time_unix = 0;
    one_file_start_time_unix = 0;
     one_file_stop_time_unix = 0;
      
      app_ppg_file_hardle.ppg_file_name[0] = '/';
      memcpy((uint8_t*)&app_ppg_file_hardle.ppg_file_name[1],(uint8_t*)&file_one_click_upload.ppg_file_name[index][0],FILE_NAME_LENG);
      app_ppg_file_hardle.file_type = (enum ppg_file_type)(app_ppg_file_hardle.ppg_file_name[34] - '0');
      BC_LOG_INFO("index:%d file_one_click_upload.ppg_file_number:%d app_ppg_file_hardle.ppg_file_name:%s \r\n",index,file_one_click_upload.ppg_file_number,app_ppg_file_hardle.ppg_file_name);
      BC_LOG_INFO("app_ppg_file_hardle.file_type:%d app_ppg_file_hardle.ppg_file_name[34]:%d ppg_file_one_click_upload_index:%d\r\n",app_ppg_file_hardle.file_type,app_ppg_file_hardle.ppg_file_name[34],ppg_file_one_click_upload_index);
#if defined(HANDWARE_1_23_2_ONE_SEC)
      /* 标记是否有录音文件 */
      switch(app_ppg_file_hardle.file_type)
      {
        case PPG_FILE_TYPE_16K_2_MIC_ADPCM:
        case PPG_FILE_TYPE_16K_2_MIC_ADPCM_CAPTURE:
        case PPG_FILE_TYPE_16K_2_MIC_OPUS:
        case PPG_FILE_TYPE_16K_2_MIC_OPUS_CAPTURE:
        case PPG_FILE_TYPE_8K_1_MIC_ADPCM:
        case PPG_FILE_TYPE_8K_1_MIC_OPUS:
          has_audio_file = true;
          break;
        default:
          break;
      }
#endif
      bc_spi_flash_device_open();  
        if(lk_ppg_file_open(&app_ppg_file_hardle) != 0)
      {
        lk_ppg_file_close(&app_ppg_file_hardle);
          bc_spi_flash_device_close();
        return ;
      }
      file_size = ppg_file_size(&app_ppg_file_hardle);

      switch(app_ppg_file_hardle.file_type)
      {
        case PPG_FILE_TYPE_ACC:
        {
          break;
        }
        case PPG_FILE_TYPE_ACC_GRYO:
        {
          break;
        }
        case PPG_FILE_TYPE_SPO2:
        {
          ppg_file_pack_size = ((4*2) + (3 *2)) * 15;
          break;
        }
        case PPG_FILE_TYPE_HR:
        {
          ppg_file_pack_size = ((4*1) + (3 *2)) * 20;
          break;
        }
        case PPG_FILE_TYPE_PPG_IR:
        {
          ppg_file_pack_size = (4*1) * 50;
          break;
        }
        case PPG_FILE_TYPE_TEMP:
        {
          break;
        }
        case PPG_FILE_TYPE_PPG_RED_IR_GREEN_TEMPER:
        {
    #if ( HARDWARE_1141_ENABLED == 1 ||  HARDWARE_451_ENABLED == 1)	

          ppg_file_pack_size = ((4*3) + (3 *2) + (3 *2) + (3 *2)) * 5 + 8 ;
    //			ppg_file_pack_size = ((4*3) + (3 *2) + (3 *2) + (3 *2)) * 5 + 8+64;
    //      ppg_file_pack_size = (((4*3) + (3 *2) + (3 *2) + (3 *2)) * 5 + 8) * 6;
    #else
          ppg_file_pack_size = ((4*3) + (3 *2) + 2) * 10;
    #endif			
          
          break;
        }
        case PPG_FILE_TYPE_16K_2_MIC_ADPCM:
        case PPG_FILE_TYPE_16K_2_MIC_ADPCM_CAPTURE:
        case PPG_FILE_TYPE_16K_2_MIC_OPUS:
        case PPG_FILE_TYPE_16K_2_MIC_OPUS_CAPTURE:
        case PPG_FILE_TYPE_8K_1_MIC_ADPCM:
        case PPG_FILE_TYPE_8K_1_MIC_OPUS:
        {
          ppg_file_pack_size = PDM_DATA_SEND_SIZE;
          break;
        }
        default:
        {
          break;
        }
      }
      
      if(file_size % ppg_file_pack_size == 0)
      {
        ppg_file_pack_number = file_size / ppg_file_pack_size;
      }
      else
      {
        ppg_file_pack_number = (file_size / ppg_file_pack_size) + 1;
      }
      //bc_spi_flash_device_open(); 
      ppg_file_seek(&app_ppg_file_hardle,LFS_SEEK_SET);

      //上传文件开始
      app_ppg_file_package.subcmd = 0x1B;
      one_file_start_time_unix = bg_rtc_time_get_uinx_time();
      one_file_stop_time_unix = 0;
      app_ppg_file_package.data[0] = index;
      app_ppg_file_package.data[1] = 0;
      memcpy(&app_ppg_file_package.data[2], &one_file_start_time_unix, sizeof(uint32_t));
      memcpy(&app_ppg_file_package.data[6], &one_file_stop_time_unix, sizeof(uint32_t));
      memcpy(&app_ppg_file_package.data[10],(uint8_t*)&app_ppg_file_hardle.ppg_file_name[1],strlen(app_ppg_file_hardle.ppg_file_name) - 1);
     
      app_package_ppg_file_uplaod(&app_ppg_file_package,4+10+strlen(app_ppg_file_hardle.ppg_file_name) - 1);
      
      while(ppg_file_pack_count < ppg_file_pack_number + 1)
      {

        if((file_size - (ppg_file_pack_size * (ppg_file_pack_count - 1))) >= ppg_file_pack_size)
        {
          app_ppg_file_package.data[0] = PPG_FLS_UPLOAD;
          memcpy(&app_ppg_file_package.data[1], &file_size, sizeof(uint32_t));
          memcpy(&app_ppg_file_package.data[5], &ppg_file_pack_number, sizeof(uint32_t));
          memcpy(&app_ppg_file_package.data[9], &ppg_file_pack_count, sizeof(uint32_t));
          memcpy(&app_ppg_file_package.data[13], &ppg_file_pack_size, sizeof(uint32_t));

          lk_ppg_file_read(&app_ppg_file_hardle,&app_ppg_file_package.data[17],ppg_file_pack_size);

          app_ppg_file_package.subcmd = 0x11;
          app_package_ppg_file_uplaod(&app_ppg_file_package,17 + ppg_file_pack_size);
          
          //上传进度
          progress_count = (ppg_file_pack_count -1) / ppg_file_pack_number * 100;
          if(progress_count % 10 == 0)
          {
            app_ppg_file_package.subcmd = 0x1C;
            app_ppg_file_package.data[0] = progress_count;
            app_package_ppg_file_uplaod(&app_ppg_file_package,4+1);
          }

        }   
        else
        {
              
               ppg_file_temp_size = file_size - (ppg_file_pack_size * (ppg_file_pack_count - 1));	
          
          app_ppg_file_package.data[0] = PPG_FLS_UPLOAD;
                memcpy(&app_ppg_file_package.data[1], &file_size, sizeof(uint32_t));
          memcpy(&app_ppg_file_package.data[5], &ppg_file_pack_number, sizeof(uint32_t));
          memcpy(&app_ppg_file_package.data[9], &ppg_file_pack_count, sizeof(uint32_t));
          memcpy(&app_ppg_file_package.data[13], &ppg_file_temp_size, sizeof(uint32_t));
          lk_ppg_file_read(&app_ppg_file_hardle,&app_ppg_file_package.data[17],ppg_file_temp_size);
          
          app_ppg_file_package.subcmd = 0x11;
          app_package_ppg_file_uplaod(&app_ppg_file_package,17 + ppg_file_temp_size);
          
          //上传进度
          progress_count = (ppg_file_pack_count -1) / ppg_file_pack_number * 100;
          if(progress_count % 10 == 0)
          {
            app_ppg_file_package.subcmd = 0x1C;
            app_ppg_file_package.data[0] = progress_count;
            app_package_ppg_file_uplaod(&app_ppg_file_package,4+1);
          }
    //			app_ble_send((uint8_t*)&upload_file_package,4+17+ppg_file_pack_size);
        }
        ppg_file_pack_count++;
        bc_dog_feed();
      }
      
      //上传文件结束
      app_ppg_file_package.subcmd = 0x1B;
      one_file_stop_time_unix = bg_rtc_time_get_uinx_time();
      app_ppg_file_package.data[0] = index;
      app_ppg_file_package.data[1] = 1;
      memcpy(&app_ppg_file_package.data[2], &one_file_start_time_unix, sizeof(uint32_t));
      memcpy(&app_ppg_file_package.data[6], &one_file_stop_time_unix, sizeof(uint32_t));
      memcpy(&app_ppg_file_package.data[10],(uint8_t*)&app_ppg_file_hardle.ppg_file_name[1],strlen(app_ppg_file_hardle.ppg_file_name) - 1);
      
      //上传进度
      app_ppg_file_package.subcmd = 0x1C;
      app_ppg_file_package.data[0] = 100;
      app_package_ppg_file_uplaod(&app_ppg_file_package,4+1);
      
      all_up_progress_count = (index - ppg_file_one_click_upload_index) / (file_one_click_upload.ppg_file_number - ppg_file_one_click_upload_index) * 100;
      //上传进度
      app_ppg_file_package.subcmd = 0x1D;
      app_ppg_file_package.data[0] = all_up_progress_count;
      app_package_ppg_file_uplaod(&app_ppg_file_package,4+1);
      
      if(lk_ppg_file_close(&app_ppg_file_hardle) != 0)
      {
          bc_spi_flash_device_close(); 
        return;
      }
      bc_spi_flash_device_close(); 
    }
    //上传文件结束
    app_ppg_file_package.subcmd = 0x1A;
    stop_time_unix = bg_rtc_time_get_uinx_time();
    app_ppg_file_package.data[0] = 0x02;
    memcpy(&app_ppg_file_package.data[1], &start_time_unix, sizeof(uint32_t));
    memcpy(&app_ppg_file_package.data[5], &stop_time_unix, sizeof(uint32_t));
    app_package_ppg_file_uplaod(&app_ppg_file_package,4+9);
    
    //上传进度
    app_ppg_file_package.subcmd = 0x1D;
    app_ppg_file_package.data[0] = 100;
    app_package_ppg_file_uplaod(&app_ppg_file_package,4+1);
    
#if defined(HANDWARE_1_23_2_ONE_SEC)
    bc_ic_led_file_sync_stop();
#endif
    app_ble_conn_time_audio_reset();
    ppg_file_one_click_upload_index = 1;

#if defined(HANDWARE_1_23_2_ONE_SEC)
    /* 全部录音文件上传完成：短振1次 */
    if(has_audio_file)
    {
      app_vibrate_start(VIBRATE_MODE_SHORT, 1);
    }
#endif

    app_ppg_file_hardle.fls_status = PPG_FLS_IDIE; 
    bc_rtos_thread_suspend(task_thread[APP_FILE_ONE_CLICK_UPLOAD_TASK_TYPE].thread_handler);
  }
}


void app_ppg_file_format(struct app_cmd_package * pack)
{
	memcpy((uint8_t*)&app_ppg_file_package,(uint8_t*)pack,40);
	if(app_ppg_file_hardle.fls_status != PPG_FLS_IDIE)
	{
		app_ppg_file_package.data[0] = 0;
		app_package_ppg_file_uplaod(&app_ppg_file_package,1);
		return;
	}
	if(!lfs_sfud_format(&app_ppg_file_hardle.lfs_fls_ppg_handle))
        app_ppg_file_package.data[0] = 1;
    else
        app_ppg_file_package.data[0] = 0;
//	bc_device_info_set_ppg_file_flag(0);
	app_package_ppg_file_uplaod(&app_ppg_file_package,1);
}


#if defined(HANDWARE_1_23_2_ONE_SEC)
uint8_t lk_app_ppg_file_open(enum ppg_file_type file_type)
#else
bool lk_app_ppg_file_open(enum ppg_file_type file_type)
#endif
{
	if(app_ppg_file_hardle.fls_status != PPG_FLS_IDIE)
	{
#if defined(HANDWARE_1_23_2_ONE_SEC)
		return 1;
#else
		return false;
#endif
	}
	
	//uint32_t temp_flag = 0;
	 bc_spi_flash_device_open(); 
	uint32_t file_sys_size = ppg_file_sys_size(&app_ppg_file_hardle);
	 //bc_spi_flash_device_close(); 
	if(file_sys_size <= app_ppg_file_hardle.ppg_file_sys_size_min)
	{
#if defined(HANDWARE_1_23_2_ONE_SEC)
        /* 1.23.2_one_sec版本：空间不足不回收，返回2 */
        BC_LOG_INFO("lk_app_ppg_file_open: space not enough, size=%d, min=%d\r\n", 
                    file_sys_size, app_ppg_file_hardle.ppg_file_sys_size_min);
        bc_spi_flash_device_close(); 
        return 2;
#else
        /* 空间不足，先回收最早的文件，留出空间后再创建 */
        BC_LOG_INFO("lk_ppg_space_reclamation********start\r\n");
		lk_ppg_space_reclamation();
        BC_LOG_INFO("lk_ppg_space_reclamation********stop\r\n");
#endif
	}
	
//	bc_device_info_get_ppg_file_flag(&temp_flag);
//	if(temp_flag == 1)
//	{
//		bc_device_info_get_ppg_file_name(app_ppg_file_hardle.ppg_file_name);
//		if(ppg_file_open(&app_ppg_file_hardle) != 0)
//		{
//			ppg_file_close(&app_ppg_file_hardle);
//			app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;
//			return false;
//		}
//		ppg_file_seek(&app_ppg_file_hardle,LFS_SEEK_END);
//	
//	}
//	else  if(temp_flag == 0)
//	{
		ppg_file_name_create(file_type);
		if(lk_ppg_file_open(&app_ppg_file_hardle) != 0)
		{
			lk_ppg_file_close(&app_ppg_file_hardle);
			app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;
            bc_spi_flash_device_close(); 
#if defined(HANDWARE_1_23_2_ONE_SEC)
			return 1;
#else
			return false;
#endif
		}
//		bc_device_info_set_ppg_file_flag(1);
//		bc_device_info_set_ppg_file_name(app_ppg_file_hardle.ppg_file_name);
    //bc_spi_flash_device_open(); 
		ppg_file_seek(&app_ppg_file_hardle,LFS_SEEK_SET);
    //bc_spi_flash_device_close();
//	}
	app_ppg_file_hardle.file_type = file_type;
	app_ppg_file_hardle.fls_status = PPG_FLS_WRITE;
	app_ppg_file_hardle.current_file_write_size = 0;    /* 重置当前文件写入计数 */
	
	//bc_spi_flash_device_open();
	
		
#if defined(HANDWARE_1_23_2_ONE_SEC)
	return 0;
#else
	return true;
#endif
	
}

bool app_ppg_file_open(enum ppg_file_type file_type)
{
	if(app_ppg_file_hardle.fls_status != PPG_FLS_IDIE)
	{
		return false;
	}
	
	uint32_t temp_flag = 0;
	 bc_spi_flash_device_open(); 
	uint32_t file_sys_size = ppg_file_sys_size(&app_ppg_file_hardle);
	 bc_spi_flash_device_close(); 
	if(file_sys_size <= app_ppg_file_hardle.ppg_file_sys_size_min)
	{
		return false;
	}
	
//	bc_device_info_get_ppg_file_flag(&temp_flag);
//	if(temp_flag == 1)
//	{
//		bc_device_info_get_ppg_file_name(app_ppg_file_hardle.ppg_file_name);
//		if(ppg_file_open(&app_ppg_file_hardle) != 0)
//		{
//			ppg_file_close(&app_ppg_file_hardle);
//			app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;
//			return false;
//		}
//		ppg_file_seek(&app_ppg_file_hardle,LFS_SEEK_END);
//	
//	}
//	else  if(temp_flag == 0)
//	{
		ppg_file_name_create(file_type);
		if(ppg_file_open(&app_ppg_file_hardle) != 0)
		{
			ppg_file_close(&app_ppg_file_hardle);
			app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;
			return false;
		}
//		bc_device_info_set_ppg_file_flag(1);
//		bc_device_info_set_ppg_file_name(app_ppg_file_hardle.ppg_file_name);
    bc_spi_flash_device_open(); 
		ppg_file_seek(&app_ppg_file_hardle,LFS_SEEK_SET);
    bc_spi_flash_device_close();
//	}
	app_ppg_file_hardle.file_type = file_type;
	app_ppg_file_hardle.fls_status = PPG_FLS_WRITE;
	
	bc_spi_flash_device_open();
	
		
	return true;
	
}

bool lk_app_ppg_file_close(void)
{
	
    if(app_ppg_file_hardle.fls_status !=  PPG_FLS_WRITE)
	{
		return false;
	}
	
#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	

  bc_spi_flash_device_open(); 
	if(lfs_file_sync(&app_ppg_file_hardle.lfs_fls_ppg_handle, &app_ppg_file_hardle.lfs_file_ppg_handle) != 0)
	{
		BC_LOG_WARN("ppg sync file error,file path:%s \r\n",app_ppg_file_hardle.ppg_file_name);
    bc_spi_flash_device_close(); 
//		return PPG_FILE_SYNC_ERROR;
	}
  bc_spi_flash_device_close(); 
#endif		
	
	if(lk_ppg_file_close(&app_ppg_file_hardle) != 0)
	{
		app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;
		
#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	
       
//	   bc_device_info_set_ppg_file_flag(0);

#endif		

bc_spi_flash_device_close();
		return false;
	}
	app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;
#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	

//		bc_device_info_set_ppg_file_flag(0);
//	bc_rtos_delay(300);
//	ppg_file_rename(app_ppg_file_hardle.file_type);

#endif	
    BC_LOG_INFO("file close!! \r\n");
  bc_spi_flash_device_close();
	return true;
	
}

bool app_ppg_file_close(void)
{
	
    if(app_ppg_file_hardle.fls_status !=  PPG_FLS_WRITE)
	{
		return false;
	}
	
#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	

  bc_spi_flash_device_open(); 
	if(lfs_file_sync(&app_ppg_file_hardle.lfs_fls_ppg_handle, &app_ppg_file_hardle.lfs_file_ppg_handle) != 0)
	{
		BC_LOG_WARN("ppg sync file error,file path:%s \r\n",app_ppg_file_hardle.ppg_file_name);
    bc_spi_flash_device_close(); 
//		return PPG_FILE_SYNC_ERROR;
	}
  bc_spi_flash_device_close(); 
#endif		
	
	if(ppg_file_close(&app_ppg_file_hardle) != 0)
	{
		app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;
		
#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	
       
//	   bc_device_info_set_ppg_file_flag(0);

#endif		

bc_spi_flash_device_close();
		return false;
	}
	app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;
#if ( HARDWARE_451_ENABLED == 1 || HARDWARE_1141_ENABLED == 1)	

//		bc_device_info_set_ppg_file_flag(0);
//	bc_rtos_delay(300);
//	ppg_file_rename(app_ppg_file_hardle.file_type);

#endif	
    BC_LOG_INFO("file close!! \r\n");
  bc_spi_flash_device_close();
	return true;
	
}

void app_ppg_file_ls(struct app_cmd_package * pack)
{
	memcpy((uint8_t*)&app_ppg_file_package,(uint8_t*)pack,40);
	if(app_ppg_file_hardle.fls_status != PPG_FLS_IDIE)
	{
		app_ppg_file_package.data[0] =0;
		app_package_ppg_file_uplaod(&app_ppg_file_package,1);
		return;
	}
	app_ppg_list_files(&app_ppg_file_hardle,"/");
}

bool app_ppg_file_upload(struct app_cmd_package * pack)
{
	memset((uint8_t*)&app_ppg_file_package,0,sizeof(app_ppg_file_package));
	memcpy((uint8_t*)&app_ppg_file_package,(uint8_t*)pack,60);
	if(app_ppg_file_hardle.fls_status != PPG_FLS_IDIE)
	{
		app_ppg_file_package.data[0] = PPG_FLS_BUSY;
		app_package_ppg_file_uplaod(&app_ppg_file_package,1);
		return false;
	}
	
	if(app_ppg_file_hardle.ppg_file_status)
	{
		ppg_file_close(&app_ppg_file_hardle);
	}

	app_ppg_file_hardle.ppg_file_name[0] = '/';
	memcpy(&app_ppg_file_hardle.ppg_file_name[1],pack->data,38);
	
	app_ppg_file_hardle.file_type = (enum ppg_file_type)(pack->data[33] - '0');
	app_ppg_file_hardle.fls_status = PPG_FLS_UPLOAD;
	BC_LOG_INFO("app_ppg_file_hardle.file_type  %d   %d \r\n",app_ppg_file_hardle.file_type,(enum ppg_file_type)(pack->data[33] - '0'));
	bc_rtos_delay(10);
#if ( defined(HANDWARE_1_19_1) )
    file_up_mode_set(SPI_UP_MODE);
#else
    file_up_mode_set(BLE_UP_MODE);
#endif  
  BC_LOG_INFO("nnnnnnnnn \r\n");
  bc_rtos_thread_resume(task_thread[PPG_FILE_DATA_TASK_TYPE_UPLOAD].thread_handler);   
}

bool app_file_active_upload(struct app_cmd_package * pack)
{
	memset((uint8_t*)&app_ppg_file_package,0,sizeof(app_ppg_file_package));
	memcpy((uint8_t*)&app_ppg_file_package,(uint8_t*)pack,60);
	if(app_ppg_file_hardle.fls_status != PPG_FLS_IDIE)
	{
		app_ppg_file_package.data[0] = PPG_FLS_BUSY;
		app_package_ppg_file_uplaod(&app_ppg_file_package,1);
		return false;
	}
	
	if(app_ppg_file_hardle.ppg_file_status)
	{
		ppg_file_close(&app_ppg_file_hardle);
	}

	app_ppg_file_hardle.ppg_file_name[0] = '/';
	memcpy(&app_ppg_file_hardle.ppg_file_name[1],pack->data,38);
	
	app_ppg_file_hardle.file_type = (enum ppg_file_type)(pack->data[33] - '0');
	app_ppg_file_hardle.fls_status = PPG_FLS_UPLOAD;
	BC_LOG_INFO("app_ppg_file_hardle.file_type  %d   %d \r\n",app_ppg_file_hardle.file_type,(enum ppg_file_type)(pack->data[33] - '0'));
	bc_rtos_delay(10);

  file_up_mode_set(BLE_UP_MODE);

  
  bc_rtos_thread_resume(task_thread[PPG_FILE_DATA_TASK_TYPE_UPLOAD].thread_handler);   
}


bool app_ppg_file_resume_upload(struct app_cmd_package * pack)
{
  
	memset((uint8_t*)&app_ppg_file_package,0,sizeof(app_ppg_file_package));
	memcpy((uint8_t*)&app_ppg_file_package,(uint8_t*)pack,60);

	if(app_ppg_file_hardle.fls_status != PPG_FLS_IDIE)
	{
		app_ppg_file_package.data[0] = PPG_FLS_BUSY;
		app_package_ppg_file_uplaod(&app_ppg_file_package,1);
		return false;
	}
	
	if(app_ppg_file_hardle.ppg_file_status)
	{
		ppg_file_close(&app_ppg_file_hardle);
	}
  ppg_file_resume_upload_offset = *(uint32_t*)pack->data;
	app_ppg_file_hardle.ppg_file_name[0] = '/';
	memcpy(&app_ppg_file_hardle.ppg_file_name[1],&pack->data[4],38);
	
	app_ppg_file_hardle.file_type = (enum ppg_file_type)(pack->data[37] - '0');
	app_ppg_file_hardle.fls_status = PPG_FLS_UPLOAD;
	BC_LOG_INFO("app_ppg_file_hardle.file_type  %d   %d \r\n",app_ppg_file_hardle.file_type,(enum ppg_file_type)(pack->data[37] - '0'));
	bc_delay_ms(100);

  bc_rtos_thread_resume(task_thread[APP_FILE_RESUME_UPLOAD_TASK_TYPE].thread_handler);
}

bool app_ppg_file_one_click_upload(struct app_cmd_package * pack)
{
  
	memset((uint8_t*)&app_ppg_file_package,0,sizeof(app_ppg_file_package));
	memcpy((uint8_t*)&app_ppg_file_package,(uint8_t*)pack,60);

	if(app_ppg_file_hardle.fls_status != PPG_FLS_IDIE)
	{
		app_ppg_file_package.data[0] = 0;
		app_package_ppg_file_uplaod(&app_ppg_file_package,1);
		return false;
	}
  uint8_t file_num = app_ppg_list_files_number_get(&app_ppg_file_hardle,"/");
  if(pack->data[0]<1 || pack->data[0] > file_num)
  {
    app_ppg_file_package.data[0] = 3;
		app_package_ppg_file_uplaod(&app_ppg_file_package,1);
		return false;
  }
	if(app_ppg_file_hardle.ppg_file_status)
	{
		ppg_file_close(&app_ppg_file_hardle);
	}
	app_ppg_file_hardle.fls_status = PPG_FLS_UPLOAD;
  ppg_file_one_click_upload_index = pack->data[0];
	bc_delay_ms(100);
	bc_rtos_thread_resume(task_thread[APP_FILE_ONE_CLICK_UPLOAD_TASK_TYPE].thread_handler);
}



bool app_ppg_file_delete(char *path)
{
	if(app_ppg_file_hardle.fls_status != PPG_FLS_IDIE)
	{
		
		return false;
	}
	char file_path[60] = "/";
	strncat(file_path,path,strlen(path));
	 bc_spi_flash_device_open(); 
	if(ppg_file_delete(&app_ppg_file_hardle,file_path) == 0)
	{
		bc_spi_flash_device_close(); 
		return true;
	}
	bc_spi_flash_device_close(); 
//	bc_device_info_set_ppg_file_flag(0);
	return false;
}

void app_ppg_list_capture_audio_up_check(void) 
{
   uint8_t file_number =  app_ppg_list_files_info_get(&app_ppg_file_hardle,"/",&file_one_click_upload);
  BC_LOG_INFO("file_number:%d \r\n",file_number);
  for(int8_t i = file_number ; i>=0;i--)
  {
   // BC_LOG_INFO("file_one_click_upload.ppg_file_name[i][33]:%d \r\n",file_one_click_upload.ppg_file_name[i][33]);
    if(file_one_click_upload.ppg_file_name[i][33] == 'B')
    {
      char file_name[38] = {0};
      bc_device_info_get_capture_audio_file_name((char*)file_name);
      BC_LOG_INFO("file name:%s\r\n",file_name);
      if(memcmp(file_name,&file_one_click_upload.ppg_file_name[i][0],sizeof(file_name)) != 0)
      {
        app_package_active_upload_file_name((uint8_t*)&file_one_click_upload.ppg_file_name[i][0],sizeof(file_name));
      }
      else
      {
        return;
      }
    }
  }
}

void app_ppg_file_sys_size_get(struct app_cmd_package * pack)
{
	if(app_ppg_file_hardle.fls_status != PPG_FLS_IDIE)
	{	
		return ;
	}
	memcpy((uint8_t*)&app_ppg_file_package,(uint8_t*)pack,40);
	
    bc_spi_flash_device_open(); 
	
     #if 1 // by liukun
    lfs_size_t total_size = app_ppg_file_hardle.lfs_fls_ppg_handle.cfg->block_size * app_ppg_file_hardle.lfs_fls_ppg_handle.cfg->block_count;
    #else
	lfs_size_t total_size = 4096 * 1024;
    #endif

    // 获取已使用空间大小（使用的块数 * 每块的大小）
    lfs_ssize_t used_blocks = lfs_fs_size(&app_ppg_file_hardle.lfs_fls_ppg_handle);
    lfs_size_t used_size = (used_blocks >= 0) ? used_blocks * app_ppg_file_hardle.lfs_fls_ppg_handle.cfg->block_size : 0;

    // 计算可用空间大小（总空间大小 - 已使用空间大小）
    lfs_size_t available_size = total_size - used_size;
	bc_spi_flash_device_close(); 
	
	*(uint32_t*)&app_ppg_file_package.data[0] = total_size;
	*(uint32_t*)&app_ppg_file_package.data[4] = used_size;
	*(uint32_t*)&app_ppg_file_package.data[8] = available_size;
	
	app_package_ppg_file_uplaod(&app_ppg_file_package,12);
}

void app_ppg_file_write(uint8_t *write_buff,uint32_t write_length) 
{
  
	if(app_ppg_file_hardle.ppg_file_status)
	{
		ppg_file_write(&app_ppg_file_hardle,write_buff, write_length);
		
		/* 累计当前文件写入量，用于滚动切片 */
		app_ppg_file_hardle.current_file_write_size += write_length;
		
		/* 当写入量达到 FLASH_MIN_USE_SPACE 时，关闭当前文件并创建新文件，实现滚动覆盖 */
		if(app_ppg_file_hardle.current_file_write_size >= FLASH_MIN_USE_SPACE)
		{
			BC_LOG_INFO("file slice: write_size=%d, close and create new file\r\n", 
				app_ppg_file_hardle.current_file_write_size);
			
			/* 关闭当前文件（使用对外接口，包含完整的状态管理和sync操作） */
			lk_app_ppg_file_close();
			
			/* 创建新文件（使用对外接口，包含空间检查、回收、文件名生成等完整流程） */
#if defined(HANDWARE_1_23_2_ONE_SEC)
            uint8_t stas = lk_app_ppg_file_open(app_ppg_file_hardle.file_type);
			if(0 == stas)
			{
				BC_LOG_INFO("new file created: %s\r\n", app_ppg_file_hardle.ppg_file_name);
			}
			else if (1 == stas)
			{
				BC_LOG_WARN("create new file failed after slice\r\n");
                app_pdm_recording_stop();
            }
            else if (2 == stas)
            {
                BC_LOG_WARN("space not enough\r\n");
                app_pdm_recording_stop();
                app_vibrate_start(VIBRATE_MODE_LONG, 2);
			}
#else
			if(lk_app_ppg_file_open(app_ppg_file_hardle.file_type))
			{
				BC_LOG_INFO("new file created: %s\r\n", app_ppg_file_hardle.ppg_file_name);
			}
			else
			{
				BC_LOG_WARN("create new file failed after slice\r\n");
			}
#endif
		}
		
//		ppg_file_slice_storage_event_callback(NULL);//切片存储处理
//    BC_LOG_INFO("ppg_file_write\r\n");
	}
	
}



void app_ppg_file_slice_storage_timer_stop(void)
{
	bc_rtos_timer_stop(timer_struct[APP_FILE_SLICE_STORAGE_TIMER_EVENT].timer_handler,50);

}

void app_ppg_file_slice_storage_timer_start(uint32_t slice_storage_timer)
{
	timer_struct[APP_FILE_SLICE_STORAGE_TIMER_EVENT].xTimerPeriodInTicks = 1000*slice_storage_timer;
  bc_rtos_timer_change_period(timer_struct[APP_FILE_SLICE_STORAGE_TIMER_EVENT].timer_handler,timer_struct[APP_FILE_SLICE_STORAGE_TIMER_EVENT].xTimerPeriodInTicks, 50);
	bc_rtos_timer_start(timer_struct[APP_FILE_SLICE_STORAGE_TIMER_EVENT].timer_handler,50);
  BC_LOG_INFO("slice_storage_timer start :%d   %d\r\n",slice_storage_timer,timer_struct[APP_FILE_SLICE_STORAGE_TIMER_EVENT].xTimerPeriodInTicks);
}

void app_ppg_file_timeout_timer_stop(void)
{
  bc_rtos_timer_stop(timer_struct[APP_FILE_TOMEOUT_TIMER_EVENT].timer_handler,50);
}

void app_ppg_file_timeout_timer_start(uint32_t timeout_timer)
{
	timer_struct[APP_FILE_TOMEOUT_TIMER_EVENT].xTimerPeriodInTicks = 1000*timeout_timer;
  bc_rtos_timer_change_period(timer_struct[APP_FILE_TOMEOUT_TIMER_EVENT].timer_handler,timer_struct[APP_FILE_TOMEOUT_TIMER_EVENT].xTimerPeriodInTicks, 50);
	bc_rtos_timer_start(timer_struct[APP_FILE_TOMEOUT_TIMER_EVENT].timer_handler,50);
  BC_LOG_INFO("slice_storage_timer start :%d   %d\r\n",timeout_timer,timer_struct[APP_FILE_TOMEOUT_TIMER_EVENT].xTimerPeriodInTicks);
}


uint8_t app_ppg_file_status_get(void)
{
	return app_ppg_file_hardle.fls_status;
}

void app_ppg_file_time_init(void)
{
  uint32_t time = app_ppg_files_time_get(&app_ppg_file_hardle,"/");
  if(time != 0)
  {
    bc_rtc_time_set_uinx_time(time,8);
  }
  else
  {
    bc_rtc_time_set_uinx_time(1735660800,8);//2025.01.01.00:00:00
  }
}

#if defined(HANDWARE_1_23_2_ONE_SEC)

/* 单击标记记录文件名 */
#define SINGLE_TAP_RECORD_FILE_NAME    "single_tap_event_flag.txt"

/*******************************************************************************
 * Function Name     : app_single_tap_record_write
 * Description       : 记录单击事件到单击标记记录文件
 *                     格式: "single | unix_timestamp\n"
 * Input             : 无
 * Output            : 无
 * Return            : true-成功 false-失败
 * Author            : liukun
 *******************************************************************************/
bool app_single_tap_record_write(void)
{
    lfs_file_t single_tap_file;
    char write_buf[64];
    int err;
    int write_len;
    uint32_t unix_time;

    /* 获取当前Unix时间戳 */
    unix_time = bg_rtc_time_get_uinx_time();

    /* 组装写入字符串："时间戳 | single\n" */
    write_len = snprintf(write_buf, sizeof(write_buf), "%d | single\n", unix_time);
    BC_LOG_INFO("app_single_tap_record_write: %s\r\n", write_buf);
    if(write_len <= 0 || write_len >= sizeof(write_buf))
    {
        return false;
    }

    /* 打开文件：不存在则创建，追加写入 */
    err = lfs_file_open(&app_ppg_file_hardle.lfs_fls_ppg_handle,
                        &single_tap_file,
                        SINGLE_TAP_RECORD_FILE_NAME,
                        LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
    if(err != 0)
    {
        BC_LOG_ERROR("single tap file open fail, err:%d\r\n", err);
        return false;
    }

    /* 写入数据 */
    err = lfs_file_write(&app_ppg_file_hardle.lfs_fls_ppg_handle,
                         &single_tap_file,
                         write_buf,
                         write_len);
    if(err != write_len)
    {
        BC_LOG_ERROR("single tap file write fail, err:%d\r\n", err);
        lfs_file_close(&app_ppg_file_hardle.lfs_fls_ppg_handle, &single_tap_file);
        return false;
    }

    /* 关闭文件 */
    lfs_file_close(&app_ppg_file_hardle.lfs_fls_ppg_handle, &single_tap_file);

    BC_LOG_INFO("single tap record ok: %s", write_buf);

    /* 单击记录成功：极短振1次提示 */
    app_vibrate_start(VIBRATE_MODE_VERY_SHORT, 1);

    return true;
}

/*******************************************************************************
 * Function Name     : app_single_tap_record_upload
 * Description       : 读取单击标记文件所有行，逐行通过蓝牙回复
 *                     每行格式: 4字节总条数 + 4字节当前条数 + 4字节时间戳 + 1字节类型
 *                     文件每行格式: "时间戳 | 类型\n"，例如 "1783926772 | single"
 * Input             : cmd_package - 命令包(用于构造回复包)
 * Output            : 无
 * Return            : 无
 * Author            : liukun
 *******************************************************************************/
#define READ_BUF_SIZE     512
#define MAX_RECORD_COUNT  500
static char read_buf[READ_BUF_SIZE];
static uint32_t record_timestamp[MAX_RECORD_COUNT];
static uint8_t  record_type[MAX_RECORD_COUNT];

void app_single_tap_record_upload(struct app_cmd_package * cmd_package)
{
    lfs_file_t single_tap_file;
    lfs_ssize_t read_size;
    uint8_t *p_data = cmd_package->data;
    uint32_t total_count = 0;
    uint32_t i;
    int err;
    char line_buf[64];
    int line_pos = 0;
    lfs_ssize_t j;

    bc_spi_flash_device_open();
    /* 打开文件 */
    err = lfs_file_open(&app_ppg_file_hardle.lfs_fls_ppg_handle,
                        &single_tap_file,
                        SINGLE_TAP_RECORD_FILE_NAME,
                        LFS_O_RDONLY);
    if(err != 0)
    {
        /* 文件不存在，发送一条全0记录表示0条 */
        memset(p_data, 0, 13);
        app_package_send_enqueue(cmd_package, 4 + 13);
        bc_spi_flash_device_close();
        return;
    }

    /* 一遍扫描：逐块读取，逐行解析，存入数组 */
    total_count = 0;
    line_pos = 0;
    while(true)
    {
        bc_dog_feed();
        read_size = lfs_file_read(&app_ppg_file_hardle.lfs_fls_ppg_handle,
                                  &single_tap_file,
                                  read_buf,
                                  READ_BUF_SIZE);
        if(read_size <= 0) break;

        for(j = 0; j < read_size; j++)
        {
            if(read_buf[j] == '\n' || line_pos >= sizeof(line_buf) - 1)
            {
                if(line_pos > 0 && total_count < MAX_RECORD_COUNT)
                {
                    line_buf[line_pos] = '\0';

                    /* 解析："时间戳 | 类型" */
                    char *pipe_pos = strstr(line_buf, " | ");
                    if(pipe_pos != NULL)
                    {
                        *pipe_pos = '\0';
                        record_timestamp[total_count] = strtoul(line_buf, NULL, 10);
                        pipe_pos += 3;
                        if(strcmp(pipe_pos, "single") == 0)
                        {
                            record_type[total_count] = 0;
                        }
                        else
                        {
                            record_type[total_count] = 0xFF;
                        }
                        total_count++;
                    }
                }
                line_pos = 0;
            }
            else
            {
                line_buf[line_pos++] = read_buf[j];
            }
        }
    }

    /* 处理最后一行 */
    if(line_pos > 0 && total_count < MAX_RECORD_COUNT)
    {
        line_buf[line_pos] = '\0';
        char *pipe_pos = strstr(line_buf, " | ");
        if(pipe_pos != NULL)
        {
            *pipe_pos = '\0';
            record_timestamp[total_count] = strtoul(line_buf, NULL, 10);
            pipe_pos += 3;
            if(strcmp(pipe_pos, "single") == 0)
            {
                record_type[total_count] = 0;
            }
            else
            {
                record_type[total_count] = 0xFF;
            }
            total_count++;
        }
    }

    /* 关闭文件 */
    lfs_file_close(&app_ppg_file_hardle.lfs_fls_ppg_handle, &single_tap_file);
    bc_spi_flash_device_close();

    /* 逐行发送（此时已经释放文件，不阻塞其他任务） */
    for(i = 0; i < total_count; i++)
    {
        uint32_t cru_cnt = i+1;
        *(uint32_t *)p_data = total_count;
        *(uint32_t *)&p_data[4] = cru_cnt;
        *(uint32_t *)&p_data[8] = record_timestamp[i];
        p_data[12] = record_type[i];
        app_package_send_enqueue(cmd_package, 4 + 13);
    }

    /* 没有记录时发一条全0 */
    if(total_count == 0)
    {
        memset(p_data, 0, 13);
        app_package_send_enqueue(cmd_package, 4 + 13);
    }
}

/*******************************************************************************
 * Function Name     : app_single_tap_record_clear
 * Description       : 删除单击标记记录文件
 * Input             : 无
 * Output            : 无
 * Return            : 0-成功 1-失败
 * Author            : liukun
 *******************************************************************************/
uint8_t app_single_tap_record_clear(void)
{
    bc_spi_flash_device_open();
    int ret = lfs_remove(&app_ppg_file_hardle.lfs_fls_ppg_handle, SINGLE_TAP_RECORD_FILE_NAME);
    bc_spi_flash_device_close();

    if(ret == 0)
    {
        BC_LOG_INFO("%s deleted ok\r\n", SINGLE_TAP_RECORD_FILE_NAME);
        return 0;
    }
    else
    {
        BC_LOG_INFO("%s delete fail, ret=%d\r\n", SINGLE_TAP_RECORD_FILE_NAME, ret);
        return 1;
    }
}
#endif /* HANDWARE_1_23_2_ONE_SEC */


//void app_ppg_resume_storage(void)
//{
//  uint32_t temp_flag = 0;
//  uint32_t resume_count = 0;
//  bc_device_info_get_ppg_file_resume_count(&resume_count);
//  bc_device_info_get_ppg_file_flag(&temp_flag);
//  if(temp_flag == 1)
//  {
//    if(resume_count >= 1)
//    {
//      return;
//    }
//    
//  }
//  else
//  {
//    return;
//  }
//  bc_event_set(&event_struct[APP_FILE_RESUME_STORAGE_TASK_EVENT]);
//}

//回收文件空间
void app_ppg_space_reclamation()
{
    uint32_t minspace = FLASH_RECLAMAT_SPACE;//app_ppg_file_hardle.ppg_file_sys_size_min * 2;
    struct ppg_file_hard *file_hardle = &app_ppg_file_hardle;
    //检索文件目录
    char path[] = "/";
    lfs_dir_t dir;
    uint32_t file_number = 0;
    uint32_t count_number = 1;
    struct lfs_info info[50];
    uint32_t releasespace = 0;
    bc_spi_flash_device_open();
    // 打开目录
    BC_LOG_INFO("app_ppg_space_reclamation \r\n");
    int err = lfs_dir_open(&file_hardle->lfs_fls_ppg_handle, &dir, path);
    if (err) {
        printf("Failed to open directory %s\n", path);
        bc_spi_flash_device_close();
        return ;
    }
    while(true)
    {
        err = lfs_dir_read(&file_hardle->lfs_fls_ppg_handle, &dir, &info[file_number]);

        if (err < 0) {

            break;
        }
        // 检查是否到达目录末尾
        if (err == 0) {
            break;
        }
        // 打印文件/目录信息
        if (info[file_number].type == LFS_TYPE_REG) {
            releasespace = info[file_number].size;
            file_number++;
            if(releasespace>=minspace) break;
        }
    }
    if(releasespace>=minspace)
    {
        for(int i = 0; i<file_number; i++)
        {
            app_ppg_file_delete(info[i].name);
        }
    }
    lfs_dir_close(&file_hardle->lfs_fls_ppg_handle, &dir);
    bc_spi_flash_device_close();
    BC_LOG_INFO("releasespace = %d \r\n",releasespace);
}

void lk_ppg_space_reclamation()
{
    uint32_t minspace = FLASH_RECLAMAT_SPACE;//app_ppg_file_hardle.ppg_file_sys_size_min * 2;
    struct ppg_file_hard *file_hardle = &app_ppg_file_hardle;
    //检索文件目录
    char path[] = "/";
    lfs_dir_t dir;
    uint32_t file_number = 0;
    uint32_t count_number = 1;
    
    uint32_t releasespace = 0;
    //bc_spi_flash_device_open();
    // 打开目录
    BC_LOG_INFO("app_ppg_space_reclamation \r\n");
    int err = lfs_dir_open(&file_hardle->lfs_fls_ppg_handle, &dir, path);
    if (err) {
        printf("Failed to open directory %s\n", path);
        bc_spi_flash_device_close();
        return ;
    }
    while(true)
    {
        err = lfs_dir_read(&file_hardle->lfs_fls_ppg_handle, &dir, &recinfo[file_number]);

        if (err < 0) {

            break;
        }
        // 检查是否到达目录末尾
        if (err == 0) {
            break;
        }
        // 打印文件/目录信息
        if (recinfo[file_number].type == LFS_TYPE_REG) {
            releasespace = recinfo[file_number].size;
            file_number++;
            if(releasespace>=minspace) break;
        }
    }
    if(releasespace>=minspace)
    {
        for(int i = 0; i<file_number; i++)
        {
            app_ppg_file_delete(recinfo[i].name);
        }
    }
    lfs_dir_close(&file_hardle->lfs_fls_ppg_handle, &dir);
    //bc_spi_flash_device_close();
    BC_LOG_INFO("releasespace = %d \r\n",releasespace);
}

void app_ppg_file_init(void) 
{
	app_ppg_file_hardle.ppg_file_size_max = ((25*4 * 2) + (3*2*25)) * 60 *60 *30;  //30小时 red+ir+acc（xyz）
	app_ppg_file_hardle.ppg_file_sys_size_min = FLASH_MIN_USE_SPACE;//1024 * 2;
//	char temp[30] = "/ppg_sport_spo2.text";
//	memcpy(app_ppg_file_hardle.ppg_file_name,temp,strlen(temp));
//	lfs_sfud_init(&app_ppg_file_hardle.lfs_fls_ppg_handle);
	
	for(uint8_t i = 0;i < APP_FILE_TIMER_NUM; i++)
	{
		timer_struct[i].timer_handler = bc_rtos_timer_create(timer_struct[i].timer_name,
														  timer_struct[i].xTimerPeriodInTicks,
														  timer_struct[i].uxAutoReload, 
														   (void *)timer_struct[i].timer_id,
															timer_struct[i].timer_callback_function);
		if(timer_struct[i].timer_handler != NULL)
		{
			BC_LOG_INFO("create %s succeed\r\n",timer_struct[i].timer_name);
		}
		else
		{
			BC_LOG_ERROR("create %s fail\r\n",timer_struct[i].timer_name);
		}			
	}
	
	bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < PPG_FILE_DATA_TASK_TYPE_NUM; i++)
	{
		x_return  = bc_rtos_thread_create((TaskFunction_t )task_thread[i].thread_task_code,     	
                                     (const char*    )task_thread[i].thread_name,   	
                                     (uint16_t       )task_thread[i].thread_stack_depth, 
                                     (void*          )&task_thread[i].thread_parameters,				
                                     (UBaseType_t    )task_thread[i].thread_priority,	
                                     (TaskHandle_t*  )&task_thread[i].thread_handler); 
		if(x_return != NULL)
		{
			BC_LOG_INFO("create %s succeed \r\n",task_thread[i].thread_name);
      if(i >0)
      {
        bc_rtos_thread_suspend(task_thread[i].thread_handler);
      }
      
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",task_thread[i].thread_name);
		}
     
	}
	
}


















