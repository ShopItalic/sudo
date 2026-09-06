#ifndef __BC_WAVEFORM_GENERATOR_H__
#define __BC_WAVEFORM_GENERATOR_H__


/* 
 * 文件名： continuous_waveform_nrf_full.c
 * 功能描述： NRF52840优化的连续波形生成器（正弦波、三角波、方波）
 * 特性：
 * 1. 所有波形类型都保证连续性
 * 2. 针对ARM Cortex-M4优化
 * 3. 支持单声道和立体声
 * 4. 可配置频率、振幅、占空比
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

//#define TEST_ADPCM_GEN

#if defined(TEST_ADPCM_GEN)
/* 8kHz采样率波形数据 */
extern int16_t saw_wave_8k_buffer[200];   /* 8kHz锯齿波，200个采样点 */
extern int16_t sine_wave_8k_buffer[200];  /* 8kHz正弦波，200个采样点 */
extern int16_t sqr_wave_8k_buffer[200];   /* 8kHz方波，200个采样点 */
extern int16_t tri_wave_8k_buffer[200];   /* 8kHz三角波，200个采样点 */

/* 16kHz采样率波形数据 */
extern int16_t saw_wave_16k_buffer[400];  /* 16kHz锯齿波，400个采样点 */
extern int16_t sine_wave_16k_buffer[400]; /* 16kHz正弦波，400个采样点 */
extern int16_t sqr_wave_16k_buffer[400];  /* 16kHz方波，400个采样点 */
extern int16_t tri_wave_16k_buffer[400];  /* 16kHz三角波，400个采样点 */
#endif




#endif




