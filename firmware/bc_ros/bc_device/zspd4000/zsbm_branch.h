/*
 *  Copyright(c) Zettasensing. All rights reserved.
 *  Description: configuration algo function, use to generate header file
 *   Created on: Dec 12, 2022
 *       Author: ZHOU YING
 */

#ifndef SRC_ALGO_ZSBM_BRANCH_H_
#define SRC_ALGO_ZSBM_BRANCH_H_

#define ZSBM_ALGO_VERSION_INFO "1.0.9-f07b998(2024-06-05 18:38)"

/*======================= Part 1: Fucntional Mode ======================*/
/* 1.1 Basic function, open default */
#define MODE_PPG_WEARING_CHECK
#define MODE_PPG_REST_HR_TIME    // <==> old: MODE_PPG_HEART_RATE_1, time
/* #undef MODE_PPG_EXERCISE_HR_V2 */
#define MODE_PPG_SPO2
/* #undef MODE_PPG_SPO2_WITH_ACC */

/* 1.2 Optional fucntion, precompile program */
/* #undef MODE_ECG */
/* #undef MODE_ECG_MEDICAL */
/* #undef MODE_ECG_CAL */
/* #undef MODE_PPG_REST_HR_FREQ */
/* #undef MODE_PPG_EXERCISE_HR_V1 */
/* #undef MODE_PPG_SPO2_FFT */
/* #undef MODE_PPG_REST_PPI */
#define MODE_PPG_EXERCISE_HR_V3  // use machine and deep learning model to predict hr

/*======================= Part 2: Demo for test ======================*/
// name refer to CMakeLists.txt "Debug function"

// Function Wear
/* #undef ZSBM_DEBUG_WEAR_10HZ */
/* #undef ZSBM_DEBUG_WEAR_FEATS */

// Function Spo2
/* #undef ZSBM_DEBUG_SPO2_50HZ */
/* #undef ZSBM_DEBUG_SPO2_100HZ */
/* #undef ZSBM_DEBUG_SPO2_100HZ_FFT */
/* #undef ZSBM_DEBUG_SPO2_FEATS */

// Function rest HR
/* #undef ZSBM_DEBUG_RESTHR_25HZ_TIME */
/* #undef ZSBM_DEBUG_RESTHR_50HZ_TIME */
/* #undef ZSBM_DEBUG_RESTHR_100HZ_TIME */
/* #undef ZSBM_DEBUG_RESTHR_200HZ_TIME */
/* #undef ZSBM_DEBUG_RESTHR_FEATS */

// Function exercise HR
/* #undef ZSBM_DEBUG_EXHR_25HZ */
/* #undef ZSBM_DEBUG_EXHR_25HZ_V3 */
/* #undef ZSBM_DEBUG_EXHR_FEATS */
#define ENABLE_EXHRV3_FUSION_FRNN
/* #undef ENABLE_MALLOC_INIT */
#define ENABLE_ZSBM_DATA_FORMAT

/* Function ecg */
/* #undef ZSBM_DEBUG_ECG_250HZ */
/* #undef ZSBM_DEBUG_ECG_300HZ */
/* #undef ZSBM_DEBUG_ECG_500HZ */
/* #undef ZSBM_DEBUG_ECG_600HZ */
/* #undef ZSBM_DEBUG_ECG_1000HZ */
/* #undef ZSBM_DEBUG_ECG_1200HZ */
/* #undef ZSBM_DEBUG_ECG_FEATS */
/* #undef ENABLE_ZSBM_ADDRESS_MIN_8BYTES */

#endif  // SRC_ALGO_ZSBM_BRANCH_H_
