#ifndef __APP_CMD_HANDLER_H__
#define __APP_CMD_HANDLER_H__



#include "stdint.h"


#define    CMD_SET_TIME                                     0x10
#define    CMD_GET_VERSION                                  0x11
#define    CMD_GET_BAT                                      0x12
#define    CMD_GET_HRV                                      0x31
#define    CMD_GET_SPO                                      0x32
#define    CMD_GET_TEMP                                     0x34
#define    CMD_GET_SPORT                                    0x35
#define    CMD_GET_HISTORY                                  0x36
#define    CMD_SYS_SET                                      0x37
#define    CMD_PORT_MODE                                    0x38
#define    CMD_GET_IR                                       0x39
#define    CMD_GET_PPG_LED                                  0x3C
#define    CMD_SIX_AXIS_SENSOR                              0x40

#define    CMD_GET_PPG_SPO2                                 0x42

#define    CMD_BUTTON_UP                                    0x61
#define    CMD_LED                                          0x62

#define    CMD_PDM                                          0x71

#define    CMD_LED_MOTOR_MODE_SET                           0x8d
#define    CMD_LED_MOTOR_MODE_GET                           0x8e

#define    CMD_PUF                                          0x51
#define    CMD_AUTHENTICATION                               0x81
#define    CMD_NFC                                          0x82
#define    CMD_MOTOR                                        0x83
#define    CMD_ALARM                                        0x87

#define    CMD_TOOL_TEST                                    0xF2

#define    CMD_CONFIG_TOUCH                                 0x84
#define    CMD_HID                                          0x85

#define    CMD_APP_EVENT                                    0xA0

#define    CMD_WIFI                                         0xB1

#define    CMD_APP_MULTPLE_HOST_DEVIE                       0x93


#define    CMD_IPC                                          0xEE  //内用调度操作指令


enum app_test_cmd
{
	TEST_CMD_READ_PPG_ID = 0,
	TEST_CMD_READ_ACC_ID,
	TEST_CMD_READ_PMIC_ID,
	TEST_CMD_READ_VOLAGE_ADC_VALUE,
	TEST_CMD_READ_TEMPER_ADC_VALUE,
	TEST_CMD_READ_SET_SHIP_MODE,
	TEST_CMD_SET_FLAG = 0x06,                           // 
	TEST_CMD_READ_PMIC_ALL_REG = 0x07,
	TEST_CMD_READ_SET_PPG_LED,
	TEST_CMD_READ_REBOOT,
	TEST_CMD_NULL_0A,
	TEST_CMD_NULL_0b,
	TEST_CMD_NULL_0c,
	TEST_CMD_NULL_0d,
	TEST_CMD_NULL_0e,
	TEST_CMD_NULL_0f,
	TEST_CMD_HRM_LEAK,
	TEST_CMD_SPO2_LEAK,
	TEST_CMD_PPG_GRAY_CARD,
	TEST_CMD_PPG_REFLECTIVE_HRM,
	TEST_CMD_PPG_REFLECTIVE_SPO2,
	TEST_CMD_PPG_DIAG,
	TEST_CMD_NULL_16,
	TEST_CMD_NULL_17,
	TEST_CMD_HARDWARE_CHECK_ALL,
	TEST_CMD_NULL_19,
	TEST_CMD_READ_PUF_ID,
	TEST_CMD_READ_TOUCH_ID,
	TEST_CMD_NULL_1C,
	TEST_CMD_NULL_1D,
	TEST_CMD_NULL_1E,
	TEST_CMD_READ_NFC_ID,
	TEST_CMD_NULL_20,
	TEST_CMD_NULL_21,
	TEST_CMD_NULL_22,
	TEST_CMD_NULL_23,
	TEST_CMD_MOTOR,
	TEST_CMD_TOUCH_START,
	TEST_CMD_TOUCH_STOP,
	TEST_CMD_TOUCH_LED,
	TEST_CMD_READ_FLASH_ID,
	TEST_CMD_NULL_29,
	TEST_CMD_BLE_LOG = 0x2A,
	TEST_CMD_READ_MOTOR_ID,
	TEST_CMD_NULL_2C,
	TEST_CMD_NULL_2D,
	TEST_CMD_NULL_2E,
	TEST_CMD_NULL_2F,
	TEST_CMD_GET_TEMPER_SENSOR_ID = 0x30,
	TEST_CMD_MOTOR_CONFIG = 0x31,
	TEST_CMD_GET_MOUSE_ID = 0x32,
	TEST_CMD_NULL_33= 0x33,
	TEST_CMD_NULL_34= 0x34,
	TEST_CMD_NULL_35= 0x35,
	TEST_CMD_NULL_36= 0x36,
	TEST_CMD_NULL_37= 0x37,
	TEST_CMD_NULL_38= 0x38,
	TEST_CMD_NULL_39= 0x39,
	TEST_CMD_BLE_LOOPBACK_TEST= 0x3A,
	TEST_CMD_BLE_SPEED_TEST= 0x3B,
	TEST_CMD_NULL_3C= 0x3C,
	TEST_CMD_NULL_3D= 0x3D,
	TEST_CMD_NULL_3E= 0x3E,
	TEST_CMD_PRESSURE_SENSOR_ADC= 0x3F,
	TEST_CMD_NULL_40= 0x40,
	TEST_CMD_NULL_41= 0x41,
	TEST_CMD_NULL_42= 0x42,
	TEST_CMD_NULL_43= 0x43,
	TEST_CMD_NULL_44= 0x44,
	TEST_CMD_SET_SN= 0x45,
  TEST_CMD_GET_SN= 0x46,
  TEST_CMD_GET_RECT_ADC_VOLTAGE= 0x47,
  TEST_CMD_NULL_48= 0x48,
  TEST_CMD_NULL_49= 0x49,
  TEST_CMD_NULL_4A= 0x4A,
  TEST_CMD_NULL_4B= 0x4B,
  TEST_CMD_NULL_4C= 0x4C,
  TEST_CMD_NULL_4D= 0x4D,
  TEST_CMD_NULL_4E= 0x4E,
  TEST_CMD_NULL_4F= 0x4F,
  TEST_CMD_NULL_50= 0x50,
  TEST_CMD_NULL_51= 0x51,
  TEST_CMD_NULL_52= 0x52,
  TEST_CMD_NULL_53= 0x53,
  TEST_CMD_NULL_54= 0x54,
  TEST_CMD_NULL_55= 0x55,
  TEST_CMD_NULL_56= 0x56,
  TEST_CMD_CANG_MAE_SET= 0x57,
  TEST_CMD_CANG_MAE_GET= 0x58,
	TEST_CMD_NUM,
};

struct app_cmd_package
{
	uint8_t frame_type;
	uint8_t frame_id;
	uint8_t cmd;
	uint8_t subcmd;
	uint8_t data[250];
    uint8_t length;
};




void app_cmd_package_parse(uint8_t *cmd_pack,uint16_t pack_length);


#endif


