#ifndef __BC_ALG_ADPCM_H__
#define __BC_ALG_ADPCM_H__



#include <stdint.h>



// 正弦波生成器状态结构体
typedef struct {
    double current_phase;      // 当前相位（弧度）
    double phase_increment;    // 每个样本的相位增量
    double frequency;          // 频率（Hz）
    double sample_rate;        // 采样率（Hz）
    double amplitude;          // 振幅（0.0-1.0）
    uint64_t total_samples;    // 已生成的样本总数
} SineWaveGenerator;


void sine_wave_init(SineWaveGenerator* gen, 
                    double freq, 
                    double sample_rate, 
                    double amplitude,
                    double initial_phase) ;
/**
 * @brief 生成连续的正弦波样本（每次100个）
 * 
 * @param gen 生成器指针
 * @param buffer 输出缓冲区（必须至少100个int16_t）
 * @param count 生成样本数量（通常为100）
 * @return int 实际生成的样本数
 */
int generate_sine_wave(SineWaveGenerator* gen, int16_t* buffer, int count);

/**
 * @brief 重置生成器的相位
 * 
 * @param gen 生成器指针
 * @param phase 新的相位（弧度）
 */
void sine_wave_reset_phase(SineWaveGenerator* gen, double phase);

/**
 * @brief 获取当前相位
 * 
 * @param gen 生成器指针
 * @return double 当前相位（弧度）
 */
double sine_wave_get_phase(SineWaveGenerator* gen);

void yma_encode(int16_t* buffer, uint8_t* outbuffer, long len);

void yma_decode(uint8_t* buffer, int16_t* outbuffer, long len);

void yma_encode_reset(void);
// 重置解码器状态
void yma_decode_reset(void);


















#endif



