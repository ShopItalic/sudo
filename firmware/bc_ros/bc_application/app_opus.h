#ifndef __APP_OPUS_H__
#define __APP_OPUS_H__

#include <stdint.h>
#include "app_package.h"
#include <stdbool.h>

// 配置参数
#define SAMPLE_RATE      8000     // 16kHz 采样率
#define SAMPLE_CHANNELS         1         // 单声道
#define FRAME_DURATION_MS    20       // 20ms 帧长
#define FRAME_SIZE       ((SAMPLE_RATE * FRAME_DURATION_MS) / 1000) // 160 个采样点
#define BITRATE          16000     // 目标码率 16 kbps

//#define USE_OPUS   1

void app_opus_create(void);

int start_opus_encode(int16_t *in, int frame_size, uint8_t *data, int max_payload_bytes);

#endif


