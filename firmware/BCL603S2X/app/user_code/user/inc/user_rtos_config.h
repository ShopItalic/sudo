#ifndef __USER_RTOS_CONFIG_H__
#define __USER_RTOS_CONFIG_H__



/*   timer id    */

#define TIMER_ID                                          0
#define TEST_TIMER_ID                                     1
#define FML_DOG_TIMER_ID                                  2
#define APP_PPG_TIMEOUT_TIMER_ID                          3
#define APP_PPG_GET_DATA_STOP_TIMER_ID                    4
#define APP_PPG_REAL_TIMER_ID                             5
#define FML_LED_LOW_POWER_FLASH_TIMER_ID                  6
#define FML_LED_CHARGE_FLASH_TIMER_ID                     7
#define FML_LED_CONNECT_HINT_TIMER_ID                     8
#define FML_LED_DISCONNECT_HINT_TIMER_ID                  9
#define APP_BLE_CONNECT_IDIE_TIMEOUT_TIMER_ID             10
#define FML_HARDWARE_CHECK_HINT_TIMER_ID                  11
#define APP_PPG_AUTOMATIC_CYCLE_TIMER_ID                  12
#define APP_MODEL_ENTER_SILENCE_TIMER_ID                  13
#define FML_G_SENSOR_INT_TIMER_ID                         14
#define FML_G_SENSOR_SPORT_TIMER_ID                       15
#define APP_TSDB_TIMER_ID                                 16
#define APP_PMIC_TIMER_ID                                 17
#define APP_HARDWARE_CHECK_TIMER_ID                       18





/*  app sys task config */


/*  app ble config */
#define APP_TASK_BLE_RECV_STACK_SIZE                      128
#define APP_TASK_BLE_RECV_PRIO                            6

#define APP_TASK_BLE_SEND_STACK_SIZE                      128
#define APP_TASK_BLE_SEND_PRIO                            6

/*  app ppg config */
#define APP_TASK_PPG_ADV_RDY_STACK_SIZE                   1024
#define APP_TASK_PPG_ADV_RDY_PRIO                         9

#define APP_TASK_PPG_DIAG_END_STACK_SIZE                  128
#define APP_TASK_PPG_DIAG_END_PRIO                        6

#define APP_TASK_PPG_SPO2_STACK_SIZE                      256
#define APP_TASK_PPG_SPO2_PRIO                            6

#define APP_TASK_PPG_HRM_STACK_SIZE                       256
#define APP_TASK_PPG_HRM_PRIO                             6


/*  app rtc config */
#define APP_TASK_RTC_STACK_SIZE                           256
#define APP_TASK_RTC_PRIO                                 6


/*  app tsdb config */
#define APP_TASK_TSDB_STACK_SIZE                          1024
#define APP_TASK_TSDB_PRIO                                6

#define APP_TASK_KVDB_STACK_SIZE                          512
#define APP_TASK_KVDB_PRIO                                6



#endif






