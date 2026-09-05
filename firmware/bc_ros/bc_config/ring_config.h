#if defined(SUDO_VOICE_ONLY)
#include "sudo_voice_profile.h"
#endif
/*****************************************************************/

/// <<< Use Configuration Wizard in Context Menu >>>\n


//===============================================================
// <h> BCL603 SDK Config V1.0.3

// <e> HARDWARE 2.2.2 配置
//==========================================================
#ifndef HARDWARE_222_ENABLED
#define HARDWARE_222_ENABLED 0
#endif

#if (HARDWARE_222_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52832
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 1
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <o> CUS_VERSION - 客户编号.
// <i> 不同的客户对应的配置不同
// <1=> RW
// <12=> S1L_XDT
#ifndef S1X_CUS_VERSION
#define S1X_CUS_VERSION 12
#endif

// <s> RING_SOFTWARE_VERSION - 软件版本号.
// <i> 长度为7个字节
#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION "2.4.5.0"
#endif

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P 
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 0
#endif

#if (HARDWARE_222_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 0
#endif

#if (HARDWARE_222_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_222_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 32
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 1
#endif

#if (HARDWARE_222_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_222_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_222_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_222_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_222_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_222_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif

// </e>
//-----------------------------------------------------------

// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>

// <e> HARDWARE 1.5.1 配置
//==========================================================
#ifndef HARDWARE_151_ENABLED
#define HARDWARE_151_ENABLED 0
#endif

#if (HARDWARE_151_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0 )
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_SOFTWARE_VERSION - 软件版本号.
// <i> 长度为7个字节
#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION "6.0.0.1Z07"
#endif

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P 
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif

// </e>
//-----------------------------------------------------------

// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>

// <e> HARDWARE 4.1.1 配置
//==========================================================
#ifndef HARDWARE_411_ENABLED
#define HARDWARE_411_ENABLED 0
#endif

#if (HARDWARE_411_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_411_SOFTWARE_VERSION - version.
#ifndef RING_411_SOFTWARE_VERSION
#define RING_411_SOFTWARE_VERSION "4.0.6.CZ12"
#endif

// <s> RING_411_HARDWARE_VERSION - version.
#ifndef RING_411_HARDWARE_VERSION
#define RING_411_HARDWARE_VERSION "603MV4.1.1"
#endif

// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  1
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 1
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 1
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P 
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif




// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 2
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>

// <e> HARDWARE 4.1.2 配置
//==========================================================
#ifndef HARDWARE_412_ENABLED
#define HARDWARE_412_ENABLED 0
#endif

#if (HARDWARE_412_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_412_SOFTWARE_VERSION - version.
#ifndef RING_412_SOFTWARE_VERSION
#define RING_412_SOFTWARE_VERSION "4.0.8.0Z26"
#endif

// <s> RING_RONG_WEI_Z2X_412_SOFTWARE_VERSION - version.
#ifndef RING_RONG_WEI_Z2X_412_SOFTWARE_VERSION
#define RING_RONG_WEI_Z2X_412_SOFTWARE_VERSION "4.0.6.IZ2X"
#endif

// <s> RING_412_HARDWARE_VERSION - version.
#ifndef RING_412_HARDWARE_VERSION
#define RING_412_HARDWARE_VERSION "603MV4.1.2"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  1
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 1
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 1
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------


//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P 
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 2
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>

// <e> HARDWARE 4.1.3 配置
//==========================================================
#ifndef HARDWARE_413_ENABLED
#define HARDWARE_413_ENABLED 0
#endif

#if (HARDWARE_413_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_413_SOFTWARE_VERSION - version.
#ifndef RING_413_SOFTWARE_VERSION
#define RING_413_SOFTWARE_VERSION "4.0.0.5Z3T"
#endif


// <s> RING_413_HARDWARE_VERSION - version.
#ifndef RING_413_HARDWARE_VERSION
#define RING_413_HARDWARE_VERSION "603MV4.1.3"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  1
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 1
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 1
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------


//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P 
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 2
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>



// <e> HARDWARE 4.4.1 配置
//==========================================================
#ifndef HARDWARE_441_ENABLED
#define HARDWARE_441_ENABLED 0
#endif

#if (HARDWARE_441_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_441_SOFTWARE_VERSION - version.
#ifndef RING_441_SOFTWARE_VERSION
#define RING_441_SOFTWARE_VERSION "4.0.0.EZ39"
#endif


// <s> RING_441_HARDWARE_VERSION - version.
#ifndef RING_441_HARDWARE_VERSION
#define RING_441_HARDWARE_VERSION "603MV4.4.1"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  1
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 1
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 1
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif
// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------


//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P 
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 2
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>

// <e> HARDWARE 4.5.1 配置
//==========================================================
#ifndef HARDWARE_451_ENABLED
#define HARDWARE_451_ENABLED 0
#endif

#if (HARDWARE_451_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_451_SOFTWARE_VERSION - version.
#ifndef RING_451_SOFTWARE_VERSION
#define RING_451_SOFTWARE_VERSION "4.0.2.8Z45"
#endif


// <s> RING_451_HARDWARE_VERSION - version.
#ifndef RING_451_HARDWARE_VERSION
#define RING_451_HARDWARE_VERSION "603MV4.5.1"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  0
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 1
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 1
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------


//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P 
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
// <3=> GH3026
// <4=> HX3918
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 4
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>

// <e> HARDWARE 1.5.3 配置
//==========================================================
#ifndef HARDWARE_153_ENABLED
#define HARDWARE_153_ENABLED 0
#endif

#if (HARDWARE_153_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_153_SOFTWARE_VERSION - version.
#ifndef RING_153_SOFTWARE_VERSION
#define RING_153_SOFTWARE_VERSION "6.0.6.5Z2W"
#endif


// <s> RING_153_HARDWARE_VERSION - version.
#ifndef RING_153_HARDWARE_VERSION
#define RING_153_HARDWARE_VERSION "603MV1.5.3"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  1
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 1
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 1
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 1
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 1
#endif


// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P
// <1=> ICM42688
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 1
#endif

#if (HARDWARE_153_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 2
#endif

#if (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------


// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>

// <e> HARDWARE 1.8.1 配置
//==========================================================
#ifndef HARDWARE_181_ENABLED
#define HARDWARE_181_ENABLED 0
#endif

#if (HARDWARE_181_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_181_SOFTWARE_VERSION - version.
#ifndef RING_181_SOFTWARE_VERSION
#define RING_181_SOFTWARE_VERSION "6.0.0.1Z3U"
#endif


// <s> RING_181_HARDWARE_VERSION - version.
#ifndef RING_181_HARDWARE_VERSION
#define RING_181_HARDWARE_VERSION "603MV1.8.1"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  1
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 1
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 1
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 1
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 1
#endif


// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P
// <1=> ICM42688
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 1
#endif

#if (HARDWARE_153_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 2
#endif

#if (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------






// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>

// <e> HARDWARE 1.12.1 配置
//==========================================================
#ifndef HARDWARE_1121_ENABLED
#define HARDWARE_1121_ENABLED 0
#endif

#if (HARDWARE_1121_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_1121_SOFTWARE_VERSION - version.
#ifndef RING_1121_SOFTWARE_VERSION
#define RING_1121_SOFTWARE_VERSION "6.0.0.9Z3V"
#endif


// <s> RING_1121_HARDWARE_VERSION - version.
#ifndef RING_1121_HARDWARE_VERSION
#define RING_1121_HARDWARE_VERSION "603V1.12.1"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  1
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 0
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 1
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 0
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P
// <1=> ICM42688
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 1
#endif

#if (HARDWARE_153_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
// <3=> GH3026
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 3
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 2
#endif

#if (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>

// <e> HARDWARE 1.14.1 配置
//==========================================================
#ifndef HARDWARE_1141_ENABLED
#define HARDWARE_1141_ENABLED 0
#endif

#if (HARDWARE_1141_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_1141_SOFTWARE_VERSION - version.
#ifndef RING_1141_SOFTWARE_VERSION
#define RING_1141_SOFTWARE_VERSION "6.0.1.2Z4H"
#endif


// <s> RING_1141_HARDWARE_VERSION - version.
#ifndef RING_1141_HARDWARE_VERSION
#define RING_1141_HARDWARE_VERSION "603V1.14.1"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  1
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 0
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 1
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 0
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P
// <1=> ICM42688
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 1
#endif

#if (HARDWARE_153_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
// <3=> GH3026
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 3
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 2
#endif

#if (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>


// <e> HARDWARE 1.17.1 配置
//==========================================================
#ifndef HARDWARE_1171_ENABLED
#define HARDWARE_1171_ENABLED 0
#endif

#if (HARDWARE_1171_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_1171_SOFTWARE_VERSION - version.
#ifndef RING_1171_SOFTWARE_VERSION
#define RING_1171_SOFTWARE_VERSION "6.0.1.1Z5J"
#endif


// <s> RING_1171_HARDWARE_VERSION - version.
#ifndef RING_1171_HARDWARE_VERSION
#define RING_1171_HARDWARE_VERSION "603V1.17.1"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  1
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 0
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 1
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 0
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P
// <1=> ICM42688
// <2=> LIS2DH12
// <3=> DA267
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 3
#endif

#if (HARDWARE_153_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
// <3=> GH3026
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 3
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 0
#endif

#if (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>



// <e> HARDWARE 1.18.1 配置
//==========================================================
#ifndef HARDWARE_1181_ENABLED
#define HARDWARE_1181_ENABLED 0
#endif

#if (HARDWARE_1181_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_1181_SOFTWARE_VERSION - version.
#ifndef RING_1181_SOFTWARE_VERSION
#define RING_1181_SOFTWARE_VERSION "6.0.0.4Z5L"
#endif


// <s> RING_1181_HARDWARE_VERSION - version.
#ifndef RING_1181_HARDWARE_VERSION
#define RING_1181_HARDWARE_VERSION "603V1.18.1"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  1
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 1
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 1
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 1
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P
// <1=> ICM42688
// <2=> LIS2DH12
// <3=> DA267
// <4=> LSM6DSOW
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 1
#endif

#if (HARDWARE_153_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
// <3=> GH3026
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 3
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 2
#endif

#if (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>

// <e> HARDWARE 1.19.1 配置
//==========================================================
#ifndef HARDWARE_1191_ENABLED
#define HARDWARE_1191_ENABLED 0
#endif

#if (HARDWARE_1191_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_1191_SOFTWARE_VERSION - version.
#ifndef RING_1191_SOFTWARE_VERSION
#define RING_1191_SOFTWARE_VERSION "6.0.0.9Z5M"
#endif


// <s> RING_1191_HARDWARE_VERSION - version.
#ifndef RING_1191_HARDWARE_VERSION
#define RING_1191_HARDWARE_VERSION "603V1.19.1"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  0
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 0
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 1
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 0
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P
// <1=> ICM42688
// <2=> LIS2DH12
// <3=> DA267
// <4=> LSM6DSOW
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 1
#endif

#if (HARDWARE_153_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
// <3=> GH3026
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 3
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 2
#endif

#if (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>


// <e> HARDWARE 1.23.1 配置
//==========================================================
#ifndef HARDWARE_1231_ENABLED
#define HARDWARE_1231_ENABLED 1
#endif

#if (HARDWARE_1231_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_1231_SOFTWARE_VERSION - version.
#ifndef RING_1231_SOFTWARE_VERSION
#define RING_1231_SOFTWARE_VERSION "6.0.1.7Z5R"
#endif


// <s> RING_1231_HARDWARE_VERSION - version.
#ifndef RING_1231_HARDWARE_VERSION
#define RING_1231_HARDWARE_VERSION "603V1.23.1"
#endif


// <s> RING_1232_SOFTWARE_VERSION - version.
#ifndef RING_1232_SOFTWARE_VERSION
#define RING_1232_SOFTWARE_VERSION "6.0.3.3Z62"
#endif

// <s> RING_1232_HARDWARE_VERSION - version.
#ifndef RING_1232_HARDWARE_VERSION
#define RING_1232_HARDWARE_VERSION "603V1.23.2"
#endif


// <s> RING_1232L_SOFTWARE_VERSION - version.
#ifndef RING_1232L_SOFTWARE_VERSION
#define RING_1232L_SOFTWARE_VERSION "6.0.1.0Z6C"
#endif

// <s> RING_1232L_HARDWARE_VERSION - version.
#ifndef RING_1232L_HARDWARE_VERSION
#define RING_1232L_HARDWARE_VERSION "603V1.23.2"
#endif

// <s> RING_1232_ONE_SEC_SOFTWARE_VERSION - version.
#ifndef RING_1232_ONE_SEC_SOFTWARE_VERSION
#define RING_1232_ONE_SEC_SOFTWARE_VERSION "6.0.0.1Z7F"
#endif

// <s> RING_1232_ONE_SEC_HARDWARE_VERSION - version.
#ifndef RING_1232_ONE_SEC_HARDWARE_VERSION
#define RING_1232_ONE_SEC_HARDWARE_VERSION "603V1.23.2"
#endif


// <s> RING_1233_SOFTWARE_VERSION - version.
#ifndef RING_1233_SOFTWARE_VERSION
#define RING_1233_SOFTWARE_VERSION "6.0.2.7Z6A"
#endif


// <s> RING_1233_HARDWARE_VERSION - version.
#ifndef RING_1233_HARDWARE_VERSION
#define RING_1233_HARDWARE_VERSION "603V1.23.3"
#endif

// <s> RING_1234_SOFTWARE_VERSION - version.
#ifndef RING_1234_SOFTWARE_VERSION
#define RING_1234_SOFTWARE_VERSION "6.0.0.8Z6F"
#endif


// <s> RING_1234_HARDWARE_VERSION - version.
#ifndef RING_1234_HARDWARE_VERSION
#define RING_1234_HARDWARE_VERSION "603V1.23.4"
#endif

// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  0
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 0
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 1
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 0
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------

//===============================================================
// <h> 外围设备配置


// <e> Touch配置
//==========================================================

#ifndef TOUCH_ENABLED
#define TOUCH_ENABLED 1
#endif

// <o> Touch设备型号 
// <0=> IQS323
// <1=> IQS7211E
#ifndef TOUCH_DEVIECE_TYPE
#define TOUCH_DEVIECE_TYPE 1
#endif

// </e>
//-----------------------------------------------------------


// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P
// <1=> ICM42688
// <2=> LIS2DH12
// <3=> DA267
// <4=> LSM6DSOW
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 4
#endif

#if (HARDWARE_153_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 0
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
// <3=> GH3026
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 3
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 2
#endif

#if (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>



// <e> HARDWARE 1.9.1 配置
//==========================================================
#ifndef HARDWARE_191_ENABLED
#define HARDWARE_191_ENABLED 0
#endif

#if (HARDWARE_191_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_191_SOFTWARE_VERSION - version.
#ifndef RING_191_SOFTWARE_VERSION
#define RING_191_SOFTWARE_VERSION "6.0.4.0Z4C"
#endif


// <s> RING_191_HARDWARE_VERSION - version.
#ifndef RING_191_HARDWARE_VERSION
#define RING_191_HARDWARE_VERSION "603MV1.9.1"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  1
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 0
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 1
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 0
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 1
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 0
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P
// <1=> ICM42688
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 0
#endif

#if (HARDWARE_153_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
// <3=> GH3026
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 3
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 2
#endif

#if (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>

// <e> HARDWARE 1.5.8 配置
//==========================================================
#ifndef HARDWARE_158_ENABLED
#define HARDWARE_158_ENABLED 0
#endif

#if (HARDWARE_158_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_158_SOFTWARE_VERSION - version.
#ifndef RING_158_SOFTWARE_VERSION
#define RING_158_SOFTWARE_VERSION "6.0.0.4Z44"
#endif


// <s> RING_158_HARDWARE_VERSION - version.
#ifndef RING_158_HARDWARE_VERSION
#define RING_158_HARDWARE_VERSION "603MV1.5.8"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  1
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 0
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 1
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 0
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P
// <1=> ICM42688
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 1
#endif

#if (HARDWARE_153_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
// <3=> GH3026
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 3
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 2
#endif

#if (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>


// <e> HARDWARE 1.5.6 配置
//==========================================================
#ifndef HARDWARE_156_ENABLED
#define HARDWARE_156_ENABLED 0
#endif

#if (HARDWARE_156_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_156_SOFTWARE_VERSION - version.
#ifndef RING_156_SOFTWARE_VERSION
#define RING_156_SOFTWARE_VERSION "6.0.0.6Z48"
#endif


// <s> RING_156_HARDWARE_VERSION - version.
#ifndef RING_156_HARDWARE_VERSION
#define RING_156_HARDWARE_VERSION "603MV1.5.6"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  1
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 0
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 1
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 0
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P
// <1=> ICM42688
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 1
#endif

#if (HARDWARE_153_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
// <3=> GH3026
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 3
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 2
#endif

#if (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_153_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>

// <e> HARDWARE BCL601 1.5.1 配置
//==========================================================
#ifndef HARDWARE_BCL601_151_ENABLED
#define HARDWARE_BCL601_151_ENABLED 0
#endif

#if (HARDWARE_BCL601_151_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_BCL601_151_SOFTWARE_VERSION - version.
#ifndef RING_BCL601_151_SOFTWARE_VERSION
#define RING_BCL601_151_SOFTWARE_VERSION "6.0.0.2Z3H"
#endif


// <s> RING_BCL601_151_HARDWARE_VERSION - version.
#ifndef RING_BCL601_151_HARDWARE_VERSION
#define RING_BCL601_151_HARDWARE_VERSION "601MV1.5.1"
#endif


// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  0
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 1
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 1
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------


//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P 
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 2
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 2
#endif

#if (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>


// <e> HARDWARE 4.0.2 配置
//==========================================================
#ifndef HARDWARE_402_ENABLED
#define HARDWARE_402_ENABLED 0
#endif

#if (HARDWARE_402_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_402_SOFTWARE_VERSION - version.
#ifndef RING_402_SOFTWARE_VERSION
#define RING_402_SOFTWARE_VERSION "4.0.5.9R  "
#endif

// <s> RING_402_HARDWARE_VERSION - version.
#ifndef RING_402_HARDWARE_VERSION
#define RING_402_HARDWARE_VERSION "603MV4.0.2"
#endif

// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  0
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 1
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// <q> ANDROID_TOUCH_PPT_HID  - android 触摸 PPT(上一页、下一页) 
#ifndef ANDROID_TOUCH_PPT_HID
#define ANDROID_TOUCH_PPT_HID 1
#endif

// <q> ANDROID_TOUCH_UP_AUDIO_HID  - android 触摸上传实时音频
#ifndef ANDROID_TOUCH_UP_AUDIO_HID
#define ANDROID_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 0
#endif

// <q> ANDROID_GESTURE_PPT_HID  - android 手势识别 PPT(上一页、下一页) 
#ifndef ANDROID_GESTURE_PPT_HID
#define ANDROID_GESTURE_PPT_HID 1
#endif

// <q> ANDROID_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef ANDROID_GESTURE_SNAP_HID
#define ANDROID_GESTURE_SNAP_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 1
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// <q> IOS_TOUCH_PPT_HID  - ios 触摸 PPT(上一页、下一页) 
#ifndef IOS_TOUCH_PPT_HID
#define IOS_TOUCH_PPT_HID 1
#endif

// <q> IOS_TOUCH_UP_AUDIO_HID  - ios 触摸上传实时音频
#ifndef IOS_TOUCH_UP_AUDIO_HID
#define IOS_TOUCH_UP_AUDIO_HID 0
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 0
#endif

// <q> IOS_GESTURE_PPT_HID  - ios 手势识别 PPT(上一页、下一页) 
#ifndef IOS_GESTURE_PPT_HID
#define IOS_GESTURE_PPT_HID 1
#endif

// <q> IOS_GESTURE_SNAP_HID  - android 手势识别 响指(拍照) 
#ifndef IOS_GESTURE_SNAP_HID
#define IOS_GESTURE_SNAP_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P 
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
// <2=> ZSPD4000 
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif



// </e>
//-----------------------------------------------------------





// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>





// <e> HARDWARE 4.2.1 配置
//==========================================================
#ifndef HARDWARE_421_ENABLED
#define HARDWARE_421_ENABLED 0
#endif

#if (HARDWARE_421_ENABLED == 1)

// <o> CUS_VERSION - MCU型号.
// <i> 不同的硬件版本对应的MCU不同
// <0=> NORDIC_52X
// <1=> PHY_6222
#ifndef HARDWARE_ARCH_TYPE
#define HARDWARE_ARCH_TYPE 0
#endif

#if (HARDWARE_ARCH_TYPE == 1)
#define HARDWARE_ARCH_TYPE_PHY6222 1
#elif (HARDWARE_ARCH_TYPE == 0)
#define HARDWARE_ARCH_TYPE_NORDIC 1
#endif

// <s> RING_421_SOFTWARE_VERSION - version.
#ifndef RING_421_SOFTWARE_VERSION
#define RING_421_SOFTWARE_VERSION "4.0.5.0Z16"
#endif

// <s> RING_421_HARDWARE_VERSION - version.
#ifndef RING_421_HARDWARE_VERSION
#define RING_421_HARDWARE_VERSION "603MV4.2.1"
#endif

// <e> BLE_DEVICE_HID  - 0 不支持 - 1 支持
//==========================================================

#ifndef BLE_DEVICE_HID
#define BLE_DEVICE_HID  1
#endif

// <e> ANDROID_HID  支持详情
//==========================================================
#ifndef ANDROID_HID
#define ANDROID_HID 1
#endif

// <e> ANDROID_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef ANDROID_TOUCH_HID
#define ANDROID_TOUCH_HID 1
#endif

// <q> ANDROID_TOUCH_SHORT_VIDEO_HID  - android 触摸 短视频(上滑、下滑) 
#ifndef ANDROID_TOUCH_SHORT_VIDEO_HID
#define ANDROID_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_TOUCH_PHOTOGRAPH_HID  - android 触摸 拍照 0 
#ifndef ANDROID_TOUCH_PHOTOGRAPH_HID
#define ANDROID_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_TOUCH_MUSIC_HID  - android 触摸 音乐(上一首、下一首) 
#ifndef ANDROID_TOUCH_MUSIC_HID
#define ANDROID_TOUCH_MUSIC_HID 1
#endif

// </e>  //touch id

// <e> ANDROID_GESTURE_HID  手势支持详情
//==========================================================
#ifndef ANDROID_GESTURE_HID
#define ANDROID_GESTURE_HID 1
#endif

// <q> ANDROID_GESTURE_SHORT_VIDEO_HID  - android 手势识别 短视频(上滑、下滑) 
#ifndef ANDROID_GESTURE_SHORT_VIDEO_HID
#define ANDROID_GESTURE_SHORT_VIDEO_HID 1
#endif

// <q> ANDROID_GESTURE_PHOTOGRAPH_HID  - android 手势识别 拍照 
#ifndef ANDROID_GESTURE_PHOTOGRAPH_HID
#define ANDROID_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> ANDROID_GESTURE_MUSIC_HID  - android 手势识别 音乐(上一首、下一首) 
#ifndef ANDROID_GESTURE_MUSIC_HID
#define ANDROID_GESTURE_MUSIC_HID 1
#endif


// </e>  //gesture hid

// </e>  android hid


// <e> IOS_HID  支持详情
//==========================================================
#ifndef IOS_HID
#define IOS_HID 1
#endif

// <e> IOS_TOUCH_HID  触摸支持详情
//==========================================================
#ifndef IOS_TOUCH_HID
#define IOS_TOUCH_HID 1
#endif

// <q> IOS_TOUCH_SHORT_VIDEO_HID  - ios 触摸 短视频(上滑、下滑) 
#ifndef IOS_TOUCH_SHORT_VIDEO_HID
#define IOS_TOUCH_SHORT_VIDEO_HID 1
#endif

// <q> IOS_TOUCH_PHOTOGRAPH_HID  - ios  触摸 拍照 
#ifndef IOS_TOUCH_PHOTOGRAPH_HID
#define IOS_TOUCH_PHOTOGRAPH_HID 1
#endif

// <q> IOS_TOUCH_MUSIC_HID  - ios  触摸 音乐(上一首、下一首) 
#ifndef IOS_TOUCH_MUSIC_HID
#define IOS_TOUCH_MUSIC_HID 1
#endif

// </e>  //touch id

// <e> IOS_GESTURE_HID  手势支持详情
//==========================================================
#ifndef IOS_GESTURE_HID
#define IOS_GESTURE_HID 1
#endif

// <q> IOS_GESTURE_SHORT_VIDEO_HID  - ios 手势识别 短视频(上滑、下滑) 
#ifndef IOS_GESTURE_SHORT_VIDEO_HID
#define IOS_GESTURE_SHORT_VIDEO_HID 1
#endif

// <q> IOS_GESTURE_PHOTOGRAPH_HID  - ios 手势识别 拍照 
#ifndef IOS_GESTURE_PHOTOGRAPH_HID
#define IOS_GESTURE_PHOTOGRAPH_HID 1
#endif

// <q> IOS_GESTURE_MUSIC_HID  - ios 手势识别 音乐(上一首、下一首) 
#ifndef IOS_GESTURE_MUSIC_HID
#define IOS_GESTURE_MUSIC_HID 1
#endif

// </e>  //gesture hid

// </e>  //ios hid

// </e>
//-----------------------------------------------------------

//===============================================================
// <h> 外围设备配置

// <e> G_Sensor配置
//==========================================================

#ifndef G_SENSOR_ENABLED
#define G_SENSOR_ENABLED 1
#endif

// <o> G_Sensoe设备型号 
// <0=> QMA6100/QMA6100P 
#ifndef G_SENSOR_DEVIECE_TYPE
#define G_SENSOR_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (G_SENSOR_DEVIECE_TYPE == 0)
#define G_SENSOR_TYPE 0
#endif

// <o> ADO引脚配置--只有QMA系列使用
// <i> 只有QMA系列使用
// <0=> GND
// <1=> VCC
#ifndef QMA_ADO_TYPE
#define QMA_ADO_TYPE 1
#endif

// <e> i2c通信
//==========================================================
#ifndef G_SENSOR_I2C_ENABLED
#define G_SENSOR_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 


#ifndef G_SENSOR_I2C_NUMBER
#define G_SENSOR_I2C_NUMBER 0
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef G_SENSOR_I2C_SDA_PIN
#define G_SENSOR_I2C_SDA_PIN 3
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_I2C_CLK_PIN
#define G_SENSOR_I2C_CLK_PIN 2
#endif

// <o> I2C int1 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef G_SENSOR_INT1_IRQ_PIN
#define G_SENSOR_INT1_IRQ_PIN 7
#endif

// <o> I2C int2 irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
// <35=> NULL
#ifndef G_SENSOR_INT2_IRQ_PIN
#define G_SENSOR_INT2_IRQ_PIN 35
#endif

// </e>
// </e>
//-----------------------------------------------------------

// <e> PPG配置
//==========================================================
#ifndef PPG_ENABLED
#define PPG_ENABLED 1
#endif

// <o> PPG设备型号 
// <0=> HX3605
// <1=> AFE4403 
#ifndef PPG_DEVIECE_TYPE
#define PPG_DEVIECE_TYPE 0
#endif

#if (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 0)
#define PPG_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PPG_DEVIECE_TYPE == 1)
#define PPG_TYPE 1
#endif

// <e> I2C通信
//==========================================================
#ifndef PPG_I2C_ENABLED
#define PPG_I2C_ENABLED 1
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PPG_I2C_NUMBER
#define PPG_I2C_NUMBER 1
#endif

// <o> I2C SDA Gpio Number Config 
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34 
#ifndef PPG_I2C_SDA_PIN
#define PPG_I2C_SDA_PIN 15
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_I2C_CLK_PIN
#define PPG_I2C_CLK_PIN 18
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_INT_IRQ_PIN
#define PPG_INT_IRQ_PIN 20
#endif

// <o> PPG_LED EN Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PPG_LED_EN_PIN
#define PPG_LED_EN_PIN 24
#endif

// </e>

// </e>
//-----------------------------------------------------------

// <e> PMIC 配置
//==========================================================
#ifndef PMIC_ENABLED
#define PMIC_ENABLED 1
#endif

// <o> PMIC设备型号 
// <0=> SY6103 
// <1=> ETH4662
// <2=> YHM2712
#ifndef PMIC_DEVIECE_TYPE
#define PMIC_DEVIECE_TYPE 2
#endif

#if (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 0)
#define PMIC_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 1)
#define PMIC_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (PMIC_DEVIECE_TYPE == 2)
#define PMIC_TYPE 2
#endif

// <o> I2C  Number Config(i2c0、i2c1为硬件i2c，其余为软件模拟i2c)
// <i> i2c0、i2c1为硬件i2c，其余为软件模拟i2c 
// <0=> I2C_0
// <1=> I2C_1 
// <2=> I2C_2 
// <3=> I2C_3
// <4=> I2C_4 
// <5=> I2C_5 
#ifndef PMIC_I2C_NUMBER
#define PMIC_I2C_NUMBER 2
#endif

// <o> I2C SDA Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_SDA_PIN
#define PMIC_I2C_SDA_PIN 24
#endif

// <o> I2C CLK Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_I2C_CLK_PIN
#define PMIC_I2C_CLK_PIN 0
#endif

// <o> I2C int irq Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef PMIC_INT_IRQ_PIN
#define PMIC_INT_IRQ_PIN 1
#endif

// </e>
//-----------------------------------------------------------

// <e> BATT 配置
//==========================================================
#ifndef BATT_ENABLED
#define BATT_ENABLED 1
#endif

// <o> 电池型号 
// <0=> GRP 
// <1=> HL
#ifndef BATT_DEVICE_TYPE
#define BATT_DEVICE_TYPE 1
#endif

#if (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 0)
#define BATT_TYPE 0
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 1)
#define BATT_TYPE 1
#elif (HARDWARE_411_ENABLED == 1) && (BATT_DEVICE_TYPE == 2)
#define BATT_TYPE 2
#endif

// <o> BATT_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef BATT_ADC_PIN
#define BATT_ADC_PIN 11
#endif

// </e>
//-----------------------------------------------------------

// <e> Temperature 配置
//==========================================================
#ifndef TEMPERATURE_ENABLED
#define TEMPERATURE_ENABLED 1
#endif

// <o> TEMP_ADC Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef TEMP_ADC_PIN
#define TEMP_ADC_PIN 25
#endif

// </e>
//-----------------------------------------------------------

// <e> LED 配置
//==========================================================
#ifndef LED_ENABLED
#define LED_ENABLED 1
#endif

// <o> LED Gpio Number Config
// <0=> GPIO_0
// <1=> GPIO_1 
// <2=> GPIO_2 
// <3=> GPIO_3
// <4=> GPIO_4 
// <5=> GPIO_5 
// <6=> GPIO_6 
// <7=> GPIO_7 
// <8=> GPIO_8 
// <9=> GPIO_9
// <10=> GPIO_10
// <11=> GPIO_11 
// <12=> GPIO_12 
// <13=> GPIO_13
// <14=> GPIO_14 
// <15=> GPIO_15 
// <16=> GPIO_16 
// <17=> GPIO_17 
// <18=> GPIO_18 
// <19=> GPIO_19
// <20=> GPIO_20
// <21=> GPIO_21 
// <22=> GPIO_22 
// <23=> GPIO_23
// <24=> GPIO_24 
// <25=> GPIO_25 
// <26=> GPIO_26 
// <27=> GPIO_27 
// <28=> GPIO_28 
// <29=> GPIO_29
// <30=> GPIO_30
// <31=> GPIO_31 
// <32=> GPIO_32 
// <33=> GPIO_33
// <34=> GPIO_34
#ifndef LED_PIN
#define LED_PIN 17
#endif

// </e>
//-----------------------------------------------------------

// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
// </e>


//===============================================================
// <h> LOG

// <o> DEBUG_INFO - 调试日志配置
// <i> LOG等级，发布版本时候设置为0
// <0=> CLOSE
// <1=> OPEN
#ifndef DEBUG_INFO
#define DEBUG_INFO 1
#endif


// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===============================================================
// <h> PPG测量参数配置

// <o> PPG_CAPTURE_DURATION - PPG单次采集时长，单位秒
// <i> PPG单次采集时长，单位秒
#ifndef PPG_CAPTURE_DURATION
#define PPG_CAPTURE_DURATION 30
#endif

// <o> PPG_CAPTURE_INTERVAL - PPG采集间隔，单位秒
// <i> PPG采集间隔，单位秒
// <i> 此采集间隔会根据PPG_HR_SPO2_PROPORTION配置进行心率和血氧的循环检测
#ifndef PPG_CAPTURE_INTERVAL
#define PPG_CAPTURE_INTERVAL 1200
#endif

// <o> PPG_HR_SPO2_PROPORTION - 血氧采集配置
// <i> 此定义决定着多少个心率采集之后进行一次血氧采集
#ifndef PPG_HR_SPO2_PROPORTION
#define PPG_HR_SPO2_PROPORTION 5
#endif

// <o> PPG_SAMPLING_RATE - PPG采样率
// <i> PPG采样率.
// <25=> 25HZ
// <50=> 50HZ
// <100=> 100HZ
#ifndef PPG_SAMPLING_RATE
#define PPG_SAMPLING_RATE 25
#endif
// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===============================================================
// <h> BLE配置

#if (CUS_VERSION == 7)
// <s> BLE_NAME - 蓝牙名称，CUS_VERSION选择S1L_EVB才会生效.
// <i> 只有EVB版本支持
    #ifndef BLE_NAME
    #define BLE_NAME "BCL603"
    #endif
#else
    #if (CUS_VERSION == 1) || (CUS_VERSION == 17) || (CUS_VERSION == 21)
    #define BLE_NAME "BCL603_HJK3"
    #elif (CUS_VERSION == 4)
    #define BLE_NAME "R02"
    #elif (CUS_VERSION == 11)
    #define BLE_NAME "Por-Ring"
    #elif (CUS_VERSION == 16) || (CUS_VERSION == 24)
    #define BLE_NAME "mi2100DT"
    #else
    #define BLE_NAME "BCL603"
    #endif
#endif

// <o> FAST_ADV_INTERVAL - 快广播间隔，单位毫秒
// <i> 快广播间隔，单位毫秒
#ifndef FAST_ADV_INTERVAL
#define FAST_ADV_INTERVAL 500
#endif
#define DEFAULT_FAST_ADV_INTERVAL ((FAST_ADV_INTERVAL*1000)/625)

// <o> SLOW_ADV_INTERVAL - 慢广播间隔，单位毫秒
// <i> 慢广播间隔，单位毫秒
#ifndef SLOW_ADV_INTERVAL
#define SLOW_ADV_INTERVAL 500
#endif
#define DEFAULT_SLOW_ADV_INTERVAL ((SLOW_ADV_INTERVAL*1000)/625)

//// <o> MIN_CONN_INTERVAL - 最小连接间隔，单位毫秒
//// <i> 最小连接间隔，单位毫秒
//#ifndef MIN_CONN_INTERVAL
//#define MIN_CONN_INTERVAL 30
//#endif
//#define DEFAULT_MIN_CONN_INTERVAL ((MIN_CONN_INTERVAL*800)/1000)

//// <o> MAX_CONN_INTERVAL - 最大连接间隔，单位毫秒
//// <i> 最大连接间隔，单位毫秒
//#ifndef MAX_CONN_INTERVAL
//#define MAX_CONN_INTERVAL 60
//#endif
//#define DEFAULT_MAX_CONN_INTERVAL ((MAX_CONN_INTERVAL*800)/1000)

// <o> SLAVE_LATENCY - 从机延迟，单位一个连接间隔
// <i> 从机延迟，单位一个连接间隔
#ifndef SLAVE_LATENCY
#define SLAVE_LATENCY 0
#endif
#define DEFAULT_SLAVE_LATENCY SLAVE_LATENCY

// <o> CONN_TIMEOUT - 连接超时，单位十毫秒
// <i> 连接超时，单位十毫秒
#ifndef CONN_TIMEOUT
#define CONN_TIMEOUT 500
#endif
#define DEFAULT_CONN_TIMEOUT CONN_TIMEOUT

// </h> 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// <<< end of configuration section >>>

/************************版本号*******************************/
#if (S1X_CUS_VERSION == 1)
#elif (S1X_CUS_VERSION == 12)
#define MANU_VERSION "C  "
#define HARD_VERSION "603SV2.2.2"
#endif

#define SOFT_VERSION PROGRAM_VERSION MANU_VERSION

/*********************以下用于测试************************/





