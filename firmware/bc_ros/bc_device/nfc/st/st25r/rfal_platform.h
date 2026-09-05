
/**
  ******************************************************************************
  * @file           : rfal_platform.h
  * @brief          : Platform header file. Defining platform independent functionality.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#ifdef STM32L053xx
#include "stm32l0xx_hal.h"
#endif

#ifdef STM32L476xx
#include "stm32l4xx_hal.h"
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

#include "nfc_timer.h"
#include "main.h"
#include "bc_logger.h"
#include "RTE_Components.h"
#include "nfc_conf.h"
#include "demo.h"
#include "bc_nfc_port.h"
#include "cmsis_armcc.h"

/** @addtogroup X-CUBE-NFC6_Applications
 *  @{
 */

/** @addtogroup PollingTagDetect
 *  @{
 */

/** @defgroup PTD_Platform
 *  @brief Demo functions containing the example code
 * @{
 */

/* Exported types ------------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
/** @defgroup PTD_Platform_Exported_Macro
 *  @{
 */
//#define platformProtectST25RComm()                do{ globalCommProtectCnt++;                  \
//                                                          __DSB();\
//	                                                       bc_nfc_exit_irq_disable(); \
//                                                          __DSB();                             \
//                                                          __ISB();                             \
//                                                        }while(0)                                   /*!< Protect unique access to ST25R communication channel - IRQ disable on single thread environment (MCU) ; Mutex lock on a multi thread environment      */
#define platformProtectST25RComm()                do{ globalCommProtectCnt++;                  \
	                                                       bc_nfc_exit_irq_disable(); \
                                                        }while(0) 
#define platformUnprotectST25RComm()              do{ globalCommProtectCnt--;             \
                                                          if (globalCommProtectCnt == 0U) \
                                                          {                               \
                                                            bc_nfc_exit_irq_enable();   \
                                                          }                               \
                                                        }while(0)                                   /*!< Unprotect unique access to ST25R communication channel - IRQ enable on a single thread environment (MCU) ; Mutex unlock on a multi thread environment */

#define platformProtectST25RIrqStatus()           platformProtectST25RComm()                /*!< Protect unique access to IRQ status var - IRQ disable on single thread environment (MCU) ; Mutex lock on a multi thread environment */
#define platformUnprotectST25RIrqStatus()         platformUnprotectST25RComm()              /*!< Unprotect the IRQ status var - IRQ enable on a single thread environment (MCU) ; Mutex unlock on a multi thread environment         */

#define platformProtectWorker()                                                                     /* Protect RFAL Worker/Task/Process from concurrent execution on multi thread platforms   */
#define platformUnprotectWorker()                                                                   /* Unprotect RFAL Worker/Task/Process from concurrent execution on multi thread platforms */

#define platformIrqST25RSetCallback( cb )
#define platformIrqST25RPinInitialize()

#define platformLedsInitialize()                                                                    /*!< Initializes the pins used as LEDs to outputs*/

//#define platformLedOff( port, pin )                   platformGpioClear(port, pin)                  /*!< Turns the given LED Off                     */
//#define platformLedOn( port, pin )                    platformGpioSet(port, pin)                    /*!< Turns the given LED On                      */
//#define platformLedToogle( port, pin )                platformGpioToogle(port, pin)                 /*!< Toogle the given LED                        */

//#define platformGpioSet( port, pin )                  HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET)    /*!< Turns the given GPIO High                   */
//#define platformGpioClear( port, pin )                HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET)  /*!< Turns the given GPIO Low                    */
//#define platformGpioToogle( port, pin )               HAL_GPIO_TogglePin(port, pin)                 /*!< Toogles the given GPIO                      */
#define platformGpioIsHigh()                         (bc_nfc_exit_irq_io_state() == 1)              /*!< Checks if the given LED is High             */
#define platformGpioIsLow()                          (!platformGpioIsHigh())                        /*!< Checks if the given LED is Low              */

#define platformTimerCreate( t )                      timerCalculateTimer(t)                        /*!< Create a timer with the given time (ms)     */
#define platformTimerIsExpired( timer )               timerIsExpired(timer)                         /*!< Checks if the given timer is expired        */
#define platformTimerDestroy( timer )                                                               /*!< Stop and release the given timer            */
#define platformDelay( t )                            bc_nfc_delay( t )                                /*!< Performs a delay for the given time (ms)    */

#define platformGetSysTick()                          bc_nfc_get_tick()                                 /*!< Get System Tick ( 1 tick = 1 ms)            */

#define platformErrorHandle()                         bc_nfc_error_handler(__FILE__,__LINE__)             /*!< Global error handler or trap                */
#define platformSpiSelect()                           bc_nfc_spi_cs_low()/*!< SPI SS\CS: Chip|Slave Select                */
#define platformSpiDeselect()                         bc_nfc_spi_cs_high()  /*!< SPI SS\CS: Chip|Slave Deselect              */
#define platformSpiTxRx( txBuf, rxBuf, len )          bc_nfc_spi_write_and_recv(txBuf, rxBuf, len)          /*!< SPI transceive                              */

#define platformLog(...)                              printf(__VA_ARGS__)                         /*!< Log  method                                 */
//#define platformLog(format, ...)         			printf("\r\n[INFO:%d;%s(%d)] " format,0, __MODULE__, __LINE__, ##__VA_ARGS__);

/* Exported functions ------------------------------------------------------- */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_H */

