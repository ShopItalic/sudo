#include "bc_device_info.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bc_util.h"
#include "bc_logger.h"
#include "bc_ble_modu_interface.h"

#include "q_device.h"

#include "crc.h"

static bc_device_info device_info = {0};
static bc_device_info device_info_temp = {0};

static q_device_t *flash_dev;

static struct flash_write_package flash_write_pack;
static struct flash_read_package flash_read_pack;

#define FLASH_STROGE_DEVICE_INFO_OFFSET   4096*0

static int bsp_device_find(void)
{
    flash_dev = q_device_find("device_flash");
	q_device_assert(flash_dev);
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
		BC_LOG_ERROR("flash read error!!! \r\n");
		return 0;
	}
//	memcpy(buf,flash_read_pack.data,size);
    return 1;
}

static int write(long offset, const uint8_t *buf, size_t size)
{	
	memset((uint8_t *)&flash_write_pack,0,sizeof(flash_write_pack));
	flash_write_pack.offset = offset;
	flash_write_pack.data_length = size;
    flash_write_pack.data = buf;
//	memcpy((uint8_t*)flash_write_pack.data,buf,size);
	if(q_device_write(flash_dev,0,&flash_write_pack,0) != RESULT_OK)
	{
		BC_LOG_ERROR("flash write error!!!! \r\n");
		return 0;
	}
	return 1;
}


static int erase(long offset, size_t size)
{
	memset((uint8_t *)&flash_write_pack,0,sizeof(flash_write_pack));
	flash_write_pack.offset = offset;
	flash_write_pack.data_length = size;
	if(q_device_ctrl(flash_dev,ERASE_FLASH,&flash_write_pack) != RESULT_OK)
	{
		BC_LOG_ERROR("falsh erase error!!!!!!\r\n");
		return 0;
	}
	return 1;
}



/*******************************************************************************
 * Function Name     : fml_device_info_flash_check
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
static bool bc_device_info_flash_check(bc_device_info *device_info_flash)
{
	uint16_t check_crc;
	read(FLASH_STROGE_DEVICE_INFO_OFFSET,(uint8_t *)&device_info_temp,sizeof(device_info_temp));
//	
	check_crc = crc16bitbybit((uint8_t *)&device_info_temp,sizeof(device_info_temp) - sizeof(device_info_temp.crc));
	if(check_crc != device_info_temp.crc)
	{
		BC_LOG_ERROR("device_info_flash->crc:%04x check crc:%04x  size:%d  \r\n",device_info_flash->crc,check_crc,sizeof(bc_device_info));
		return  false;
	}
	memcpy((uint8_t *)&device_info,(uint8_t *)&device_info_temp,sizeof(device_info_temp));
//	BC_LOG_INFO("device mode:%d\r\n",device_info.device_mode);
//	BC_LOG_HEX_P("device info ",(uint8_t *)&device_info,sizeof(device_info) - sizeof(device_info.crc));
	return true;
}

/*******************************************************************************
 * Function Name     : fml_device_info_save
 * Description       : 保存设备信息
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
static bool bc_device_info_save(void)
{
	device_info.crc = crc16bitbybit((uint8_t*)&device_info,sizeof(bc_device_info) - sizeof(device_info.crc));
	if(erase(FLASH_STROGE_DEVICE_INFO_OFFSET,sizeof(bc_device_info)))
	{
		BC_LOG_INFO("save device info erase\r\n");
		if(write(FLASH_STROGE_DEVICE_INFO_OFFSET,(uint8_t*)&device_info,sizeof(bc_device_info)))
		{
			BC_LOG_INFO("save device info succeed crc:%04x\r\n",device_info.crc);
//			BC_LOG_HEX_P("device info ",(uint8_t *)&device_info,sizeof(device_info) - sizeof(device_info.crc));
			return true;
		}
	}
	BC_LOG_ERROR("save device info error\r\n");
	return false;
}

static void device_info_init_config(void)
{
  uint8_t mac[8] = {0};
  uint8_t sn[17] = {0};

  memcpy(sn,device_info.device_identity_info.sn,21);

  memcpy(mac,device_info.mac,8);
	memset((uint8_t*)&device_info,0,sizeof(device_info));
	struct bc_ble_calss ble_calss = bc_ble_new();
	uint8_t ble_mac[6] = {0};
  if(mac[0] != 0xFF && mac[1] != 0xFF && mac[0] != 0x00 && mac[1] != 0x00)
  {
    memcpy(ble_mac,mac,6);
  }
  else
  {
    ble_calss.ble_mac_get(ble_mac);
    memcpy(device_info.mac,ble_mac,6);
  }
	char temp[20] = "BCL603";
	splitHexArray(&ble_mac[4], 2, &temp[6], 12);
	memcpy(device_info.ble_name,temp,10);
	device_info.ble_name_length = 10;
	device_info.ppg_update_record_time = 0;
	device_info.sport_count = 0;
	device_info.authentication_key_code_length = 0;
	device_info.url_length = 0;
	device_info.hid_info.device_hid_type = BLE_DEVICE_HID;
	device_info.hid_info.device_hid_enable_flag = 0x00;
	device_info.hid_info.device_hid_gesture_mode = 0xFF;
	device_info.hid_info.device_hid_touch_mode = 0xFF;
	device_info.device_sleep.device_sleep_mode = 1;
	device_info.device_sleep.device_sleep_mode_flag = 0;
	device_info.file_info.ppg_file_flag = 0;
  device_info.file_info.ppg_file_resume_count = 0;
	device_info.file_info.ppg_file_mode = 0;
	device_info.audio_info.audio_up_mode = 1;  //默认adpcm
	device_info.nfc_info.nfc_mode = 0;
  device_info.sport_info.sport_mode = 0;
  device_info.sport_info.sport_start_time = 0;
  device_info.sport_info.sport_stop_time = 0;
  device_info.shut_down_info.shut_down_time = 1764518400;          //2025.12.1:0:0:0
  
  if(mac[0] != 0xFF && mac[1] != 0xFF && mac[0] != 0x00 && mac[1] != 0x00)
  {
    memcpy(device_info.mac,mac,8);
  }
  //memcpy(device_info.device_identity_info.sn,sn,21);
  strcpy((char *)device_info.device_identity_info.sn,"00000000");
	
#if defined(BLE_MULTI_MASTER)
	
	memset((uint8_t*)&device_info.ble_host_device_list,0,sizeof(device_info.ble_host_device_list));
	
#endif // defined(BLE_MULTI_MASTER)	

#if defined(ALARM_CLOCK)
    
	memset((uint8_t*)&device_info.alarm_clock_info,0,sizeof(struct bc_device_alarm_clock_info));

#endif //ALARM_CLOCK
	
#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
	device_info.vbat_type = 0;
#endif

#if defined(HANDWARE_1_23_3)
	// 线性马达配置默认值
	device_info.linear_motor_info.duration_ms = 5;
	device_info.linear_motor_info.vib_count = 1;
	device_info.linear_motor_info.vib_gain = 0x90;
	device_info.linear_motor_info.interval_ms = 700;
#endif

#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
	device_info.six_axis_config.acc_frequency = 25;
	device_info.six_axis_config.gyro_frequency = 0;

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688
    device_info.six_axis_config.acc_frequency = 25;
    device_info.six_axis_config.gyro_frequency = 25;
#elif (G_SENSOR_DEVIECE_TYPE == 4)  // LSM6SDO
    device_info.six_axis_config.acc_frequency = 25;
    device_info.six_axis_config.gyro_frequency = 25;
#endif

#if defined(HANDWARE_1_23_2_ONE_SEC)
    device_info.timer_record_enable = 0;
    device_info.timer_record_interval = 60;
    device_info.timer_record_duration = 30;
#endif

#if defined(HANDWARE_1_23_2)
    /* LED和马达模式默认值：与图片中默认值对应 */
    device_info.led_motor_mode_info.ble_connect_color = 0x01;      /* 蓝牙连接：蓝灯常亮2秒 */
    device_info.led_motor_mode_info.ble_disconnect_color = 0x01;   /* 蓝牙断开：蓝灯闪烁2次 */
    device_info.led_motor_mode_info.recording_color = 0x03;        /* 录音：绿灯常亮 */
    device_info.led_motor_mode_info.motor_start_mode = 0x01;       /* 开启录音：短振1次 */
    device_info.led_motor_mode_info.motor_stop_mode = 0x01;        /* 关闭录音：短振2次 */
#endif

	bc_device_info_save();	
}

/*******************************************************************************
 * Function Name     : fml_device_info_init
 * Description       : 设备info初始化
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
void bc_device_info_init(void)
{
	bsp_device_find();
	if(!bc_device_info_flash_check(&device_info))
	{
		device_info_init_config();
	}
	device_info.hid_info.device_hid_type = BLE_DEVICE_HID;
	device_info.hid_info.device_hid_enable_flag = 0x00;
	device_info.hid_info.device_hid_gesture_mode = 0xFF;
	device_info.hid_info.device_hid_touch_mode = 0xFF;
}

void bc_device_info_reset(void)
{
	device_info_init_config();
}

/*******************************************************************************
 * Function Name     : bc_device_info_get_ppg_update_record_time
 * Description       : //ppg 历史记录上传成功后的时间，用于记录上传时间标志
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
uint32_t bc_device_info_get_ppg_update_record_time(void)
{
	return device_info.ppg_update_record_time ;
}

/*******************************************************************************
 * Function Name     : bc_device_info_set_ppg_update_record_time
 * Description       : //ppg 历史记录上传成功后的时间，用于记录上传时间标志
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
void bc_device_info_set_ppg_update_record_time(uint32_t time)
{
	if(device_info.ppg_update_record_time == time)
	{
		return;
	}
	device_info.ppg_update_record_time = time;
	bc_device_info_save();
}

/*******************************************************************************
 * Function Name     : bc_device_info_get_ppg_update_record_time
 * Description       : //ppg 历史记录上传成功后的时间，用于记录上传时间标志
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
uint32_t bc_device_info_get_sport_count(void)
{
	return device_info.sport_count ;
}

/*******************************************************************************
 * Function Name     : bc_device_info_set_ppg_update_record_time
 * Description       : //ppg 历史记录上传成功后的时间，用于记录上传时间标志
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
void bc_device_info_set_sport_count(uint32_t sport_count)
{
	if(device_info.sport_count == sport_count)
	{
		return;
	}
	device_info.sport_count = sport_count;
	bc_device_info_save();
}



/*******************************************************************************
 * Function Name     : bc_device_info_get_ppg_update_record_time
 * Description       : //ppg 历史记录上传成功后的时间，用于记录上传时间标志
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
void bc_device_info_get_key_code(uint8_t *key_code,uint8_t *key_code_len)
{
	if(device_info.authentication_key_code_length <= 128 && device_info.authentication_key_code_length != 0)
	{
		memcpy(key_code,device_info.authentication_key_code,device_info.authentication_key_code_length);
		*key_code_len = device_info.authentication_key_code_length;
	}
	else
	{
		*key_code_len = 0;
	}
}

/*******************************************************************************
 * Function Name     : bc_device_info_set_ppg_update_record_time
 * Description       : //ppg 历史记录上传成功后的时间，用于记录上传时间标志
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
void bc_device_info_set_key_code(uint8_t *key_code,uint8_t key_code_len)
{
	if(memcmp(device_info.authentication_key_code,key_code,key_code_len) == 0)
	{
		return;
	}
	memcpy(device_info.authentication_key_code,key_code,key_code_len);
	device_info.authentication_key_code_length = key_code_len;
	bc_device_info_save();
}

/*******************************************************************************
 * Function Name     : bc_device_info_get_ppg_update_record_time
 * Description       : //ppg 历史记录上传成功后的时间，用于记录上传时间标志
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
void bc_device_info_get_url(uint8_t *url,uint8_t *url_length)
{
	if(device_info.url_length <= 128 && device_info.url_length != 0)
	{
		memcpy(url,device_info.url,device_info.url_length);
		*url_length = device_info.url_length;
	}
	else
	{
		*url_length = 0;
	}
}

/*******************************************************************************
 * Function Name     : bc_device_info_set_ppg_update_record_time
 * Description       : //ppg 历史记录上传成功后的时间，用于记录上传时间标志
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
bool bc_device_info_set_url(uint8_t *url,uint8_t url_length)
{
	if(memcmp(device_info.url,url,url_length) == 0)
	{
		return true;
	}
	memcpy(device_info.url,url,url_length);
	device_info.url_length = url_length;
	return bc_device_info_save();
}

/*******************************************************************************
 * Function Name     : bc_device_info_get_ppg_update_record_time
 * Description       : //ppg 历史记录上传成功后的时间，用于记录上传时间标志
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
void bc_device_info_get_ble_name(uint8_t *ble_name,uint8_t *ble_name_length)
{
	if(device_info.ble_name_length <= 20 && device_info.ble_name_length != 0)
	{
		memcpy(ble_name,device_info.ble_name,device_info.ble_name_length);
		*ble_name_length = device_info.ble_name_length;
	}
	else
	{
		*ble_name_length = 0;
	}
}

/*******************************************************************************
 * Function Name     : bc_device_info_set_ppg_update_record_time
 * Description       : //ppg 历史记录上传成功后的时间，用于记录上传时间标志
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2021年3月4日
 *******************************************************************************/
bool bc_device_info_set_ble_name(uint8_t *ble_name,uint8_t ble_name_length)
{
	if(memcmp(device_info.ble_name,ble_name,ble_name_length) == 0)
	{
		return true;
	}
	memcpy(device_info.ble_name,ble_name,ble_name_length);
	device_info.ble_name_length = ble_name_length;
	return bc_device_info_save();
}


bc_device_hid_info *bc_device_info_get_hid_info(void)
{
	return &device_info.hid_info;
}


bool bc_device_info_set_hid_info(bc_device_hid_info *hid_info)
{
	device_info.hid_info.device_hid_gesture_mode = hid_info->device_hid_gesture_mode;
	device_info.hid_info.device_hid_touch_mode = hid_info->device_hid_touch_mode;
	device_info.hid_info.device_hid_enable_flag = hid_info->device_hid_enable_flag;
//	return bc_device_info_save();
	return true;
}

bool bc_device_info_app_update_time_set(uint32_t time,uint32_t zone)
{
	device_info.device_time.device_update_time = time;
	device_info.device_time.device_update_time_zone = zone;
	
	return bc_device_info_save();
}

uint32_t bc_device_info_app_update_time_get(void)
{
	return device_info.device_time.device_update_time;
}

bool bc_device_info_app_reset_time_set(uint32_t time)
{
	device_info.device_time.device_reset_time = time;
	
	return bc_device_info_save();
}

uint32_t bc_device_info_app_reset_time_get(void)
{
	return device_info.device_time.device_reset_time;
}

bc_device_sleep *bc_device_info_sleep_get(void)
{
	return &device_info.device_sleep;
}

bool bc_device_info_sleep_set(bc_device_sleep *device_sleep)
{
	device_info.device_sleep.device_sleep_mode = device_sleep->device_sleep_mode;
	device_info.device_sleep.device_sleep_mode_flag = device_sleep->device_sleep_mode;
	return bc_device_info_save();
}


bc_device_six_axis_config *bc_device_six_axis_config_get(void)
{
	return &device_info.six_axis_config;
}

bool bc_device_six_axis_config_set(bc_device_six_axis_config *six_axis_config)
{
	device_info.six_axis_config.acc_frequency = six_axis_config->acc_frequency;
	device_info.six_axis_config.gyro_frequency = six_axis_config->gyro_frequency;
	return bc_device_info_save();
}

bool bc_device_info_set_ppg_file_flag(uint32_t flag)
{
	BC_LOG_INFO("bc_device_info_set_ppg_file_flag:%d \r\n",flag);
	device_info.file_info.ppg_file_flag = flag;
	BC_LOG_INFO("bc_device_info_set_ppg_file_flag:%d \r\n",device_info.file_info.ppg_file_flag);
	return bc_device_info_save();
}

void bc_device_info_get_ppg_file_flag(uint32_t *flag)
{
	BC_LOG_INFO("bc_device_info_get_ppg_file_flag:%d \r\n",device_info.file_info.ppg_file_flag);
	*flag = device_info.file_info.ppg_file_flag;
}

void bc_device_info_get_ppg_file_resume_count(uint32_t *resume_count)
{
	BC_LOG_INFO("bc_device_info_get_ppg_resume_count:%d \r\n",device_info.file_info.ppg_file_resume_count);
	*resume_count = device_info.file_info.ppg_file_resume_count;
}

bool bc_device_info_clear_ppg_file_resume_count(void)
{
  device_info.file_info.ppg_file_resume_count = 0;
	BC_LOG_INFO("bc_device_info_set_ppg_resume_count:%d \r\n",device_info.file_info.ppg_file_resume_count);
	return bc_device_info_save();
}

bool bc_device_info_set_ppg_file_resume_count(void)
{
  device_info.file_info.ppg_file_resume_count += 1;
	BC_LOG_INFO("bc_device_info_set_ppg_resume_count:%d \r\n",device_info.file_info.ppg_file_resume_count);
	return bc_device_info_save();
}





bool bc_device_info_set_ppg_file_name(char *file_name)
{
	memcpy((uint8_t*)device_info.file_info.ppg_file_name,file_name,38);
	return bc_device_info_save();
}

void bc_device_info_get_ppg_file_name(char *file_name)
{
	memcpy((uint8_t*)file_name,device_info.file_info.ppg_file_name,38);
}

bool bc_device_info_set_ppg_file_mode(uint32_t file_mode)
{
	device_info.file_info.ppg_file_mode = file_mode;
	return bc_device_info_save();
}

uint32_t bc_device_info_get_ppg_file_mode(void)
{
	return device_info.file_info.ppg_file_mode;
}

bool bc_device_info_set_audio_up_mode(uint32_t mode)
{
	device_info.audio_info.audio_up_mode = mode;
	return bc_device_info_save();
}

uint32_t bc_device_info_get_audio_up_mode(void)
{
	return device_info.audio_info.audio_up_mode;
}

bool bc_device_nfc_info_set(bc_device_nfc_info  *nfc_info)
{
	device_info.nfc_info.nfc_mode = nfc_info->nfc_mode;
	return bc_device_info_save();
}

bc_device_nfc_info  *bc_device_nfc_info_get(void)
{
	return &device_info.nfc_info;
}

bool bc_device_reset_io_flag_set(uint8_t flag)
{
	device_info.reset_io_flag = flag;
	return bc_device_info_save();
}

bool bc_device_mac_set(uint8_t *mac)
{
  memcpy(device_info.mac,mac,6);
  
  char temp[20] = "BCL603";
  uint8_t mac_name[2] = {0};
  if(memcmp(device_info.ble_name,temp,6) == 0)
  {
    mac_name[0] = device_info.mac[1];
    mac_name[1] = device_info.mac[0];
    splitHexArray(&mac_name[0], 2, &temp[6], 12);
    memcpy(device_info.ble_name,temp,10);
  }
  return bc_device_info_save();
}

void bc_device_mac_get(uint8_t *mac)
{
  memcpy(mac,device_info.mac,6);
}

bool bc_device_sport_info_set(uint8_t mode,uint32_t start_time,uint32_t stop_time)
{
	device_info.sport_info.sport_mode = mode;
  device_info.sport_info.sport_start_time = start_time;
  device_info.sport_info.sport_stop_time = stop_time;
	return 1; //bc_device_info_save();
}

void bc_device_sport_info_get(uint8_t *mode,uint32_t *start_time,uint32_t *stop_time)
{
	*mode = device_info.sport_info.sport_mode;
  *start_time = device_info.sport_info.sport_start_time;
  *stop_time = device_info.sport_info.sport_stop_time ;
}

#if defined(BLE_MULTI_MASTER)
	
void bc_device_info_ble_host_device_list_get(bc_device_ble_host_device *host_device_list)
{
	memcpy((uint8_t*)host_device_list,(uint8_t*)&device_info.ble_host_device_list,sizeof(bc_device_ble_host_device));
}

bool bc_device_info_ble_host_device_list_set(bc_device_ble_host_device *host_device_list)
{
	memcpy((uint8_t*)&device_info.ble_host_device_list,(uint8_t*)host_device_list,sizeof(bc_device_ble_host_device));
	return bc_device_info_save();
}	
	
#endif // defined(BLE_MULTI_MASTER)	


#if defined(ALARM_CLOCK)
    
void bc_device_info_alarm_clock_info_get(struct bc_device_alarm_clock_info *alarm_clock_info)
{
	memcpy((uint8_t*)alarm_clock_info,(uint8_t*)&device_info.alarm_clock_info,sizeof(struct bc_device_alarm_clock_info));
}

bool bc_device_info_alarm_clock_info_set(struct bc_device_alarm_clock_info *alarm_clock_info)
{
	memcpy((uint8_t*)&device_info.alarm_clock_info,(uint8_t*)alarm_clock_info,sizeof(struct bc_device_alarm_clock_info));
	return bc_device_info_save();
}	

#endif //ALARM_CLOCK

void bc_device_shut_down_time_get(uint32_t *time)
{
  *time = device_info.shut_down_info.shut_down_time;
}

bool bc_device_shut_down_time_set(uint32_t time)
{
  device_info.shut_down_info.shut_down_time = time;
  return bc_device_info_save();
}

void bc_device_identity_info_get(uint8_t *identity_info)
{
  //memcpy(identity_info,device_info.device_identity_info.sn,21);
    strcpy((char *)identity_info,(const char *)device_info.device_identity_info.sn);
}

bool bc_device_identity_info_set(uint8_t *identity_info, uint16_t length)
{
    uint16_t il = sizeof(device_info.device_identity_info.sn) - 1;
    if(length > il)
        length = il;
  //memcpy(device_info.device_identity_info.sn,identity_info,21);
    memset(device_info.device_identity_info.sn, 0, sizeof(device_info.device_identity_info.sn));
    memcpy(device_info.device_identity_info.sn, identity_info, length);
  return bc_device_info_save();
}

bool bc_device_info_set_capture_audio_file_name(char *file_name)
{
	memcpy((uint8_t*)device_info.capture_audio_info.file_name,file_name,38);
	return bc_device_info_save();
}

void bc_device_info_get_capture_audio_file_name(char *file_name)
{
	memcpy((uint8_t*)file_name,device_info.capture_audio_info.file_name,38);
}

#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
bool bc_device_info_set_vbat_type(uint8_t type)
{
	device_info.vbat_type = type;
	return bc_device_info_save();
}

uint8_t bc_device_info_get_vbat_type(void)
{
	return device_info.vbat_type;
}
#endif

#if defined(HANDWARE_1_23_3)
bool bc_device_info_linear_motor_config_set(uint16_t duration_ms, uint8_t vib_count, uint8_t vib_gain, uint16_t interval_ms)
{
	device_info.linear_motor_info.duration_ms = duration_ms;
	device_info.linear_motor_info.vib_count = vib_count;
	device_info.linear_motor_info.vib_gain = vib_gain;
	device_info.linear_motor_info.interval_ms = interval_ms;
	return bc_device_info_save();
}

void bc_device_info_linear_motor_config_get(uint16_t *duration_ms, uint8_t *vib_count, uint8_t *vib_gain, uint16_t *interval_ms)
{
	if(duration_ms) *duration_ms = device_info.linear_motor_info.duration_ms;
	if(vib_count) *vib_count = device_info.linear_motor_info.vib_count;
	if(vib_gain) *vib_gain = device_info.linear_motor_info.vib_gain;
	if(interval_ms) *interval_ms = device_info.linear_motor_info.interval_ms;
}
#endif

void bc_device_cang_mac_get(uint8_t *info)
{
  memcpy(info,device_info.cang_mac,device_info.cang_mac_len);
}

bool bc_device_cang_mac_set(uint8_t *info)
{
    uint8_t ilen = info[0];
    memset(device_info.cang_mac, 0, sizeof(device_info.cang_mac));
  memcpy(device_info.cang_mac,&info[1],ilen);
    device_info.cang_mac_len = ilen;
  return bc_device_info_save();
}

#if defined(HANDWARE_1_23_2_ONE_SEC)
void bc_device_info_timer_record_config_get(uint8_t *enable, uint32_t *interval, uint32_t *duration)
{
    if(enable != NULL)
    {
        *enable = device_info.timer_record_enable;
    }
    if(interval != NULL)
    {
        *interval = device_info.timer_record_interval;
    }
    if(duration != NULL)
    {
        *duration = device_info.timer_record_duration;
    }
}

bool bc_device_info_timer_record_config_set(uint8_t enable, uint32_t interval, uint32_t duration)
{
    device_info.timer_record_enable = enable;
    device_info.timer_record_interval = interval;
    device_info.timer_record_duration = duration;
    return bc_device_info_save();
}
#endif

#if defined(HANDWARE_1_23_2)
void bc_device_info_led_motor_mode_info_get(bc_device_led_motor_mode_info *info)
{
    if(info != NULL)
    {
        memcpy(info, &device_info.led_motor_mode_info, sizeof(bc_device_led_motor_mode_info));
    }
}

bool bc_device_info_led_motor_mode_info_set(bc_device_led_motor_mode_info *info)
{
    if(info == NULL)
    {
        return false;
    }
    memcpy(&device_info.led_motor_mode_info, info, sizeof(bc_device_led_motor_mode_info));
    return bc_device_info_save();
}
#endif

