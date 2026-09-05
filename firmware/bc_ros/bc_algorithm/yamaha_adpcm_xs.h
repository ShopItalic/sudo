#ifndef __YAMAHA_ADPCM_H__
#define __YAMAHA_ADPCM_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>


// 声道状态结构
typedef struct {
    int16_t predictor;    // 预测值
    int8_t step_index;    // 步长索引
    int16_t step;         // 当前步长
} ADPCMChannelState_normal;

// ADPCM编解码器结构
typedef struct {
    ADPCMChannelState_normal channels[2]; // 双声道状态
} YamahaADPCM_normal;

// 函数声明
void yamaha_adpcm_init_normal(YamahaADPCM_normal* codec);
void yamaha_adpcm_reset_normal(YamahaADPCM_normal* codec);
uint8_t yamaha_adpcm_encode_sample_normal(YamahaADPCM_normal* codec, int16_t sample, int channel);
int16_t yamaha_adpcm_decode_sample_normal(YamahaADPCM_normal* codec, uint8_t code, int channel);
uint32_t yamaha_adpcm_encode_stereo_normal(YamahaADPCM_normal* codec, const int16_t* pcm_data, uint32_t pcm_len, uint8_t* adpcm_data);
uint32_t yamaha_adpcm_decode_stereo_normal(YamahaADPCM_normal* codec, const uint8_t* adpcm_data, uint32_t adpcm_len, int16_t* pcm_data);




#endif