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



/*  user config */
#define APP_TASK_USER_STACK_SIZE                          52
#define APP_TASK_USER_PRIO                                6

/*  app sys task config */


/*  app ble config */
#if defined(SUDO_VOICE_ONLY)
#define APP_TASK_BLE_RECV_STACK_SIZE                      768
#else
#define APP_TASK_BLE_RECV_STACK_SIZE                      512
#endif
#define APP_TASK_BLE_RECV_PRIO                            12

#define APP_TASK_BLE_SEND_STACK_SIZE                      256
#define APP_TASK_BLE_SEND_PRIO                            12

/*  app ppg config */
#define APP_TASK_PPG_IRQ_STACK_SIZE                       1024
#define APP_TASK_PPG_IRQ_PRIO                             11

#define APP_TASK_PPG_WORK_STATE_STACK_SIZE                512
#define APP_TASK_PPG_WORK_STATE_PRIO                      10

#define APP_TASK_PPG_SPO2_STACK_SIZE                      256
#define APP_TASK_PPG_SPO2_PRIO                            9

#define APP_TASK_PPG_HR_STACK_SIZE                        256
#define APP_TASK_PPG_HR_PRIO                              9

#define APP_TASK_PPG_SPO2_HR_STACK_SIZE                   256
#define APP_TASK_PPG_SPO2_HR_PRIO                         9

#define APP_TASK_PPG_STORAGE_RECORD_STACK_SIZE            512
#define APP_TASK_PPG_STORAGE_RECORD_PRIO                  9


/*  app rtc config */
#define APP_TASK_RTC_STACK_SIZE                           128
#define APP_TASK_RTC_PRIO                                 8


/* Sudo budgets are StackType_t words (4 bytes on Cortex-M4). The motor,
 * motion and hardware-check workers reserve 2 KiB each. Linked-path checks
 * and later physical high-water measurements remain separate evidence. */
#if defined(SUDO_VOICE_ONLY)
#define APP_TASK_HARDWARE_CHECK_STACK_SIZE                512
#else
#define APP_TASK_HARDWARE_CHECK_STACK_SIZE                APP_TASK_RTC_STACK_SIZE
#endif

/*  app linear motor config */
#if defined(SUDO_VOICE_ONLY)
#define APP_LINEAR_MOTOR_STACK_SIZE                       512
#else
#define APP_LINEAR_MOTOR_STACK_SIZE                       128
#endif
#define APP_LINEAR_MOTOR_PRIO                             8


/*  app touch config */
#define APP_TOUCH_IRQ_STACK_SIZE                          128
#define APP_TOUCH_IRQ_PRIO                                13

#if defined(SUDO_VOICE_ONLY)
#define APP_TOUCH_EVENT_STACK_SIZE                        768
#else
#define APP_TOUCH_EVENT_STACK_SIZE                        256
#endif
#define APP_TOUCH_EVENT_PRIO                              12



/*  app mic config */
#define APP_TASK_MIC_SED_STACK_SIZE                       512
#define APP_TASK_MIC_SED_PRIO                             10

#define APP_TASK_MIC_IRQ_STACK_SIZE                       4096//1024
#define APP_TASK_MIC_IRQ_PRIO                             10

/*  app sleep config */
#define APP_TASK_SLEEP_STACK_SIZE                         512
#define APP_TASK_SLEEP_PRIO                               8

/*  app g_sensor config */
#if defined(SUDO_VOICE_ONLY)
#define APP_TASK_G_SENSOR_STACK_SIZE                      512
#else
#define APP_TASK_G_SENSOR_STACK_SIZE                      128
#endif
#define APP_TASK_G_SENSOR_PRIO                            8

/*  app tsdb config */
#define APP_TASK_TSDB_STACK_SIZE                          1024
#define APP_TASK_TSDB_PRIO                                8

#define APP_TASK_HARDLINE_TSDB_STACK_SIZE                 512
#define APP_TASK_HARDLINE_TSDB_PRIO                       8

#define APP_TASK_KVDB_STACK_SIZE                          512
#define APP_TASK_KVDB_PRIO                                8


/*  app file config */
#define APP_TASK_FILE_UPLOAD_STACK_SIZE                   1024
#define APP_TASK_FILE_UPLOAD_PRIO                         8

/*  app ble config */
#define APP_TASK_BLE_SPEED_STACK_SIZE                     128
#define APP_TASK_BLE_SPEED_PRIO                           8


/*  bc_ wdgconfig */
#define BC_TASK_WDG_STACK_SIZE                            128
#define BC_TASK_WDG_PRIO                                  7


/*  bc ic led config */
#define BC_IC_LED_STACK_SIZE                              128
#define BC_IC_LED__PRIO                                   12

#endif






