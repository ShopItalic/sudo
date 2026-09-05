/**
 * @copyright (c) 2003 - 2022, Goodix Co., Ltd. All rights reserved.
 *
 * @file    gh3x2x_drv_soft_led_agc.h
 *
 * @brief   gh3x2x led agc process functions
 *
 * @version ref gh3x2x_drv_version.h
 *
 */


#ifndef _GH3X2X_DRV_SOFT_LED_AGC_H_
#define _GH3X2X_DRV_SOFT_LED_AGC_H_
 
 
#include "gh_drv_config.h"
#include "gh_drv.h"

#define  GH3X2X_LED_AGC_DISABLE             (0)
#define  GH3X2X_LED_AGC_ENABLE              (1)


/// led agc adc num max
#define  GH3X2X_AGC_ADC_NUM_MAX             (4u)

/// led agc gain limit number
#define  GH3X2X_AGC_LIMIT_NUM_MAX           (4u)

/// raw data base line value
#define  GH3X2X_DATA_BASE_LINE              (8388608)   //2^23 

/// sys sample rate, 32kHz
#define  GH3X2X_SYS_SAMPLE_RATE             (32000)
#define GH3X2X_FASTEST_SAMPLERATE_ADDR          0x0004

/**
 * @brief soft lead result
 */
typedef struct
{
    GU8 uchAmbSampleEn  : 2;                /**< AMB sampling enable */
    GU8 uchAmbSlot      : 3;                /**< slot of amb sampling */
    GU8 uchRes          : 3;                /**< reserved */
} STAmbSlotCtrl;
 
 /**
 * @brief gain limit
 */
typedef struct
{
    GU8 uchGainLimitBg32uA  : 4;          /**< gain limit bg current */
    GU8 uchGainLimitBg64uA  : 4;          
    GU8 uchGainLimitBg128uA : 4;        
    GU8 uchGainLimitBg256uA : 4;
} STSoftAgcGainLimit;

 /**
  * @brief soft agc parameter
  */
typedef struct
{
    GU8 uchSlotxSoftAgcAdjEn;              /**< soft agc enable */
    GU8 uchSlotxSoftBgAdjEn;               /**< soft bg cancel adjust enable */
    STAmbSlotCtrl stAmbSlotCtrl;           /**< amb slot ctrl */
    GU8 uchRes0;                           /**< reserved */
    GU8 uchSpo2RedSlotCfgEn;
    GU8 uchSpo2IrSlotCfgEn;
    STSoftAgcGainLimit stSoftAgcGainLimit; /**< soft gain limit */
    GU32 unAgcTrigThd_H;                   /**< trig threshold(high) of soft agc */
    GU32 unAgcTrigThd_L;                   /**< trig threshold(low) of soft agc */
    GU32 unAgcRestrainThd_H;               /**< restrain threshold(high) of soft agc */
    GU32 unAgcRestrainThd_L;               /**< restrain threshold(low) of soft agc */
} STSoftAgcCfg;


void GH3X2X_LedAgcReset(void);
void GH3X2X_LedAgcPramWrite(GU16 usRegAddr, GU16 usRegData);
void GH3X2X_LedAgcInit(STGh3x2xAgcInfo *STGh3x2xAgcInfo);
void GH3X2X_LedAgcProcess(GU8* puchReadFifoBuffer, GU16 usFifoLen, STGh3x2xAgcInfo *STGh3x2xAgcInfo);
void GH3X2X_LedAgc_Close(void);
void GH3X2X_LedAgc_Open(void);


///* slot register ctrl index */
//#define  GH3X2X_SLOT_REG_CTRL_0             (0)    /**< slot x ctrl 0 reg */
//#define  GH3X2X_SLOT_REG_CTRL_1             (1)    /**< slot x ctrl 1 reg */
//#define  GH3X2X_SLOT_REG_CTRL_2             (2)    /**< slot x ctrl 2 reg */
//#define  GH3X2X_SLOT_REG_CTRL_3             (3)    /**< slot x ctrl 3 reg */
//#define  GH3X2X_SLOT_REG_CTRL_4             (4)    /**< slot x ctrl 4 reg */
//#define  GH3X2X_SLOT_REG_CTRL_5             (5)    /**< slot x ctrl 5 reg */
//#define  GH3X2X_SLOT_REG_CTRL_6             (6)    /**< slot x ctrl 6 reg */
//#define  GH3X2X_SLOT_REG_CTRL_7             (7)    /**< slot x ctrl 7 reg */
//#define  GH3X2X_SLOT_REG_CTRL_8             (8)    /**< slot x ctrl 8 reg */
//#define  GH3X2X_SLOT_REG_CTRL_9             (9)    /**< slot x ctrl 9 reg */
//#define  GH3X2X_SLOT_REG_CTRL_10            (10)   /**< slot x ctrl 10 reg */
//#define  GH3X2X_SLOT_REG_CTRL_11            (11)   /**< slot x ctrl 11 reg */
//#define  GH3X2X_SLOT_REG_CTRL_12            (12)   /**< slot x ctrl 12 reg */
//#define  GH3X2X_SLOT_REG_CTRL_13            (13)   /**< slot x ctrl 13 reg */

//#define GH3X2X_BG_LEVEL_BIT_MASK            (0x0030)
//#define GH3X2X_ADC_BG_CANCEL_32_UA          (0x0)
//#define GH3X2X_ADC_BG_CANCEL_64_UA          (0x1)
//#define GH3X2X_ADC_BG_CANCEL_128_UA         (0x2)
//#define GH3X2X_ADC_BG_CANCEL_256_UA         (0x3)

///// amb slot select
//#define GH3X2X_AGC_AMB_SLOT_DISABLE         (0x0)
//#define GH3X2X_AGC_AMB_SLOT_USER_DEFINED    (0x1)
//#define GH3X2X_AGC_AMB_SLOT_FOLLOW_CFG      (0x2)

///// get bits data
//#define GH3X2X_GET_REG_BITS_U16(reg_data, lsb, msb)       (((reg_data) & ((0xFFFF << (lsb)) & \
//                                                            (0xFFFF >> (15 - (msb))))) >> (lsb))

///// set bits data
//#define GH3X2X_SET_REG_BITS_U16(reg, lsb, msb, value)     ((reg) = (((reg) & ((0xFFFF << (lsb)) ^ \
//                                                                  (0xFFFF >> (15 - (msb))))) | ((value) << (lsb))))

///// get adc gain
//#define GH3X2X_GET_ADC_GAIN(reg_data, adc_no)             GH3X2X_GET_REG_BITS_U16((reg_data), ((adc_no) * 4), \
//                                                                ((adc_no) * 4 + 3))

///// set adc gain
//#define GH3X2X_SET_ADC_GAIN(reg_data, adc_no, gain)       GH3X2X_SET_REG_BITS_U16((reg_data), ((adc_no) * 4), \
//                                                                ((adc_no) * 4 + 3), (gain))

///// get bg cancel i_dacfs
//#define GH3X2X_GET_BGC_I_DACFS(reg_data, adc_no)          GH3X2X_GET_REG_BITS_U16((reg_data), ((adc_no) << 1), \
//                                                                ((adc_no) * 2 + 1))

///// set bg cancel i_dacfs
//#define GH3X2X_SET_BGC_I_DACFS(reg_data, adc_no, i_dacfs) GH3X2X_SET_REG_BITS_U16((reg_data), ((adc_no) * 2), \
//                                                              ((adc_no) * 2 + 1), (i_dacfs))
//                                                                                         
//#define ABS(x) ((x) > 0 ? (x) : -(x))

///// transfer int to float
//#define GH3X2X_TRANS_INT_TO_FLOAT           (1.0f)

///// agc calc seconds (250ms)
//#define GH3X2X_AGC_CALC_SECONDS             (0.25f)

///// agc bgc donw cnt, 5s
//#define GH3X2X_AGC_BGC_DOWN_CNT             (20)

///// slot tia gain val min
//#define GH3X2X_AGC_GAIN_MIN_VAL             (0)

///// slot tia gain val max, 5s, equal (GH3X2X_SLOT_TIA_GAIN_VAL_MAX - 1)
//#define GH3X2X_AGC_GAIN_MAX_VAL             (12)

///// Less than thr, include rawdata < thr
//#define GH3X2X_AGC_EXCD_FLAG_LESS           (0x01)

///// greater than thr, include rawdatamix > thr | rawdata > thr
//#define GH3X2X_AGC_EXCD_FLAG_GREATER        (0x06)

///// AGC failure reason
//#define GH3X2X_AGC_FAILURE_REASON_1         (0x1) 
//#define GH3X2X_AGC_FAILURE_REASON_2         (0x2)
//#define GH3X2X_AGC_FAILURE_REASON_4         (0x4)
//#define GH3X2X_AGC_FAILURE_REASON_5         (0x5)
//#define GH3X2X_AGC_FAILURE_REASON_6         (0x6)

//#define GH3X2X_AGC_CHECK_EN_BITS(x)         (((x) & (0x03 << 8)) != 0)      /**< check enable bits of ctrl3 */
//#define GH3X2X_AGC_GET_SRC_BITS(x)          (((x) >> 6) & 0x03)             /**< get agc src bits of ctrl3 */
//#define GH3X2X_AGC_CHECK_BG_2X2_BITS(x)     (((x) & (0x03 << 4)) == 0x03)   /**< check 2x2 bg lvl bits of ctrl3 */
//#define GH3X2X_AGC_SET_SRC_BITS(x, n)       (((x) & 0xFF3Fu) | ((n) << 6u)) /**< set agc src bits of ctrl3 */

//#define GH3X2X_AGC_GET_DONE_FAIL_BIT(x, i)  (((x) >> (i)) & 0x01)           /**< get tune done/fail bit of flag */
//#define GH3X2X_AGC_REG_BIT(x, i)            (((x) >> (i)) & 0x01)           /**< get reg bit value */
//#define GH3X2X_AGC_GET_RESULT_BITS(x, i)    (((x) >> (i)) & 0x07)           /**< get result bits of flag  */
//#define GH3X2X_AGC_RESULT_BITS_SIZE         (3u)                           /**< one result 3 bits size of flag  */

#endif  // _GH3X2X_DRV_SOFT_LED_AGC_H_

/********END OF FILE********* Copyright (c) 2003 - 2022, Goodix Co., Ltd. ********/
