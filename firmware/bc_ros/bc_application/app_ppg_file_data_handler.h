#ifndef __APP_PPG_FILE_DATA_HANDLER_H__
#define __APP_PPG_FILE_DATA_HANDLER_H__

#include <stdbool.h>

#include "app_package.h"

enum ppg_file_type
{
	PPG_FILE_TYPE_IDIE = 0,
	PPG_FILE_TYPE_ACC = 1,
	PPG_FILE_TYPE_ACC_GRYO,
	PPG_FILE_TYPE_SPO2,
	PPG_FILE_TYPE_HR,
	PPG_FILE_TYPE_PPG_IR,	
	PPG_FILE_TYPE_TEMP,
	PPG_FILE_TYPE_PPG_RED_IR_GREEN_TEMPER,
  PPG_FILE_TYPE_16K_2_MIC_ADPCM,
  PPG_FILE_TYPE_16K_2_MIC_ADPCM_CAPTURE = 18,// 'B'-'0'=18
  PPG_FILE_TYPE_16K_2_MIC_OPUS,  
  PPG_FILE_TYPE_16K_2_MIC_OPUS_CAPTURE, 
  PPG_FILE_TYPE_8K_1_MIC_ADPCM,
  PPG_FILE_TYPE_8K_1_MIC_OPUS,  
};

bool app_ppg_file_open(enum ppg_file_type file_type);

bool app_ppg_file_close(void);

void app_ppg_file_ls(struct app_cmd_package * pack);

void app_ppg_file_init(void);


void app_ppg_file_format(struct app_cmd_package * pack);

bool app_ppg_file_upload(struct app_cmd_package * pack);

void app_ppg_file_write(uint8_t *write_buff,uint32_t write_length) ;
void app_ppg_file_sys_size_get(struct app_cmd_package * pack);

bool app_ppg_file_delete(char *path);
void app_ppg_file_sys_size_get(struct app_cmd_package * pack);

uint8_t app_ppg_file_status_get(void);

void app_ppg_file_slice_storage_timer_stop(void);
void app_ppg_file_slice_storage_timer_start(uint32_t slice_storage_timer);

void app_ppg_file_timeout_timer_stop(void);
void app_ppg_file_timeout_timer_start(uint32_t timeout_timer);
bool app_ppg_file_resume_upload(struct app_cmd_package * pack);
bool app_ppg_file_one_click_upload(struct app_cmd_package * pack);
void app_ppg_list_capture_audio_up_check(void) ;
bool app_file_active_upload(struct app_cmd_package * pack);

#if defined(HANDWARE_1_23_2_ONE_SEC)
uint8_t lk_app_ppg_file_open(enum ppg_file_type file_type);
#else
bool lk_app_ppg_file_open(enum ppg_file_type file_type);
#endif
bool lk_app_ppg_file_close(void);

#if defined(HANDWARE_1_23_2_ONE_SEC)
bool app_single_tap_record_write(void);
void app_single_tap_record_upload(struct app_cmd_package * cmd_package);
uint8_t app_single_tap_record_clear(void);
#endif

#endif


