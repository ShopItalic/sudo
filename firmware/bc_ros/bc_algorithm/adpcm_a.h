#ifndef __ADPCM_A_H__
#define __ADPCM_A_H__


/* 
 * 文件名： stereo_adpcm.c
 * 功能描述： 立体声ADPCM编解码器实现
 * 作者：助手
 * 日期：2024年10月30日
 * 版本号： v1.0
 * 
 * 特性：
 * 1. 完整的立体声ADPCM编解码
 * 2. 支持交错格式输入输出
 * 3. 独立左右声道状态管理
 * 4. 完整的测试例程
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "bc_rtos.h"

/* ============================================================================
 * ADPCM核心实现（基于IMA ADPCM）
 * ============================================================================
 */

// ADPCM状态结构体
typedef struct adpcm_state_t {
    short valprev;      // 上一个样本的预测值
    char index;         // 步长索引（0-88）
} adpcm_state;


// ADPCM编码函数（内部使用）
void adpcm_encoder(short* indata, char* outdata, int len, adpcm_state* state);

// ADPCM解码函数（内部使用）
void adpcm_decoder(char* indata, short* outdata, int len, adpcm_state* state);

/* ============================================================================
 * 单声道ADPCM处理器
 * ============================================================================
 */

// 单声道ADPCM处理器结构体
typedef struct {
    adpcm_state mono_state;   // 声道状态
} MonoAdpcmProcessor;

/**
 * @brief 初始化单声道ADPCM处理器
 * @param processor 处理器指针
 */
void mono_adpcm_init(MonoAdpcmProcessor* processor);
  
/* ============================================================================
 * 立体声ADPCM处理器
 * ============================================================================
 */

// 立体声ADPCM处理器结构体
typedef struct {
    adpcm_state left_state;   // 左声道状态
    adpcm_state right_state;  // 右声道状态
} StereoAdpcmProcessor;

/**
 * @brief 初始化立体声ADPCM处理器
 * @param processor 处理器指针
 */
void stereo_adpcm_init(StereoAdpcmProcessor* processor);

/**
 * @brief 重置立体声ADPCM处理器状态
 * @param processor 处理器指针
 * @param left_valprev 左声道预测值
 * @param left_index 左声道步长索引
 * @param right_valprev 右声道预测值
 * @param right_index 右声道步长索引
 */
void stereo_adpcm_reset(StereoAdpcmProcessor* processor,
                       short left_valprev, char left_index,
                       short right_valprev, char right_index);

/**
 * @brief 获取立体声ADPCM处理器状态
 * @param processor 处理器指针
 * @param left_valprev 左声道预测值输出
 * @param left_index 左声道步长索引输出
 * @param right_valprev 右声道预测值输出
 * @param right_index 右声道步长索引输出
 */
void stereo_adpcm_get_state(StereoAdpcmProcessor* processor,
                           short* left_valprev, char* left_index,
                           short* right_valprev, char* right_index);

/**
 * @brief 立体声ADPCM编码
 * @param processor 处理器指针
 * @param stereo_pcm 交错立体声PCM数据（格式：L0,R0,L1,R1...）
 * @param stereo_adpcm 输出立体声ADPCM数据（交错格式）
 * @param num_samples 每个声道的样本数
 * 
 * @note 输出ADPCM数据格式：
 *   每4个字节对应8个PCM样本（4个左声道，4个右声道）
 *   字节0: 左声道样本0-1的ADPCM
 *   字节1: 右声道样本0-1的ADPCM
 *   字节2: 左声道样本2-3的ADPCM
 *   字节3: 右声道样本2-3的ADPCM
 *   ...
 */
void stereo_adpcm_encode(StereoAdpcmProcessor* processor,
                         short* stereo_pcm,
                         char* stereo_adpcm,
                         int num_samples);

/**
 * @brief 立体声ADPCM解码
 * @param processor 处理器指针
 * @param stereo_adpcm 输入立体声ADPCM数据（交错格式）
 * @param stereo_pcm 输出立体声PCM数据（交错格式）
 * @param num_samples 每个声道的样本数
 */
void stereo_adpcm_decode(StereoAdpcmProcessor* processor,
                         char* stereo_adpcm,
                         short* stereo_pcm,
                         int num_samples);

// 正弦波生成器
typedef struct {
    double phase;        // 当前相位
    double phase_inc;    // 相位增量
    double amplitude;    // 振幅（0.0-1.0）
    double sample_rate;  // 采样率
    double frequency;    // 频率
} SineGenerator;


/* ============================================================================
 * 新增：波形类型定义
 * ============================================================================
 */
typedef enum {
    TRIANGLE_WAVE,      // 三角波
    SQUARE_WAVE,        // 方波
    SAWTOOTH_WAVE       // 锯齿波
} AdditionalWaveType;

/* ============================================================================
 * 新增：三角波生成器结构
 * ============================================================================
 */
typedef struct {
    double frequency;      // 频率(Hz)
    double sample_rate;    // 采样率(Hz)
    double amplitude;      // 振幅(0.0-1.0)
    double phase;          // 当前相位(0-1.0)
    double phase_inc;      // 相位增量
} TriangleWaveGenerator;

/* ============================================================================
 * 新增：方波生成器结构
 * ============================================================================
 */
typedef struct {
    double frequency;      // 频率(Hz)
    double sample_rate;    // 采样率(Hz)
    double amplitude;      // 振幅(0.0-1.0)
    double phase;          // 当前相位(0-1.0)
    double phase_inc;      // 相位增量
    double duty_cycle;     // 占空比(0.0-1.0，默认0.5)
} SquareWaveGenerator;

/* ============================================================================
 * 新增：通用波形生成器结构（包含所有波形）
 * ============================================================================
 */
typedef struct {
    double frequency;          // 频率(Hz)
    double sample_rate;        // 采样率(Hz)
    double amplitude;          // 振幅(0.0-1.0)
    double phase;              // 当前相位
    double phase_inc;          // 相位增量
    AdditionalWaveType type;   // 波形类型
    double duty_cycle;         // 占空比（仅方波使用）
} AdditionalWaveGenerator;

/**
 * @brief 初始化正弦波生成器
 */
void sine_init(SineGenerator* gen, double freq, double sample_rate, double amplitude);

/**
 * @brief 生成单声道正弦波
 */
void generate_mono_sine(SineGenerator* gen, short* buffer, int count);

/**
 * @brief 生成立体声正弦波（不同频率）
 * @param left_gen 左声道生成器
 * @param right_gen 右声道生成器
 * @param stereo_buffer 输出立体声缓冲区
 * @param count 每个声道的样本数
 */
void generate_stereo_sine(SineGenerator* left_gen, SineGenerator* right_gen,
                          short* stereo_buffer, int count);


/**
 * @brief 打印立体声数据的前几个样本
 */
void print_stereo_data(const char* label, short* stereo_data, int num_samples, int max_print);

/**
 * @brief 打印ADPCM数据
 */
void print_adpcm_data(const char* label, char* adpcm_data, int num_bytes, int max_print);
/**
 * @brief 计算并打印误差统计
 */
void analyze_stereo_error(short* original, short* decoded, int num_samples);

/**
 * @brief 比较两个立体声数据是否相同
 * @return 相同返回1，不同返回0
 */
int compare_stereo_data(short* data1, short* data2, int num_samples);





/* ============================================================================
 * 新增：三角波生成函数
 * ============================================================================
 */

/**
 * @brief 初始化三角波生成器
 * @param gen 三角波生成器指针
 * @param freq 频率(Hz)
 * @param sample_rate 采样率(Hz)
 * @param amplitude 振幅(0.0-1.0)
 */
void triangle_wave_init(TriangleWaveGenerator* gen, double freq, 
                       double sample_rate, double amplitude);

/**
 * @brief 生成三角波样本
 * @param phase 相位(0.0-1.0)
 * @return 三角波值(-1.0到1.0)
 */
static double generate_triangle_sample(double phase);

/**
 * @brief 生成单声道三角波
 * @param gen 三角波生成器指针
 * @param buffer 输出缓冲区
 * @param count 样本数量
 */
void generate_mono_triangle(TriangleWaveGenerator* gen, short* buffer, int count);

/* ============================================================================
 * 新增：方波生成函数
 * ============================================================================
 */

/**
 * @brief 初始化方波生成器
 * @param gen 方波生成器指针
 * @param freq 频率(Hz)
 * @param sample_rate 采样率(Hz)
 * @param amplitude 振幅(0.0-1.0)
 * @param duty_cycle 占空比(0.0-1.0)
 */
void square_wave_init(SquareWaveGenerator* gen, double freq, 
                     double sample_rate, double amplitude, double duty_cycle);

/**
 * @brief 生成方波样本
 * @param phase 相位(0.0-1.0)
 * @param duty_cycle 占空比(0.0-1.0)
 * @return 方波值(-1.0或1.0)
 */
static double generate_square_sample(double phase, double duty_cycle);

/**
 * @brief 生成单声道方波
 * @param gen 方波生成器指针
 * @param buffer 输出缓冲区
 * @param count 样本数量
 */
void generate_mono_square(SquareWaveGenerator* gen, short* buffer, int count);

/* ============================================================================
 * 新增：通用波形生成函数（统一接口）
 * ============================================================================
 */

/**
 * @brief 初始化通用波形生成器
 * @param gen 波形生成器指针
 * @param freq 频率(Hz)
 * @param sample_rate 采样率(Hz)
 * @param amplitude 振幅(0.0-1.0)
 * @param type 波形类型
 * @param duty_cycle 占空比（仅方波使用，默认0.5）
 */
void additional_wave_init(AdditionalWaveGenerator* gen, double freq, 
                         double sample_rate, double amplitude, 
                         AdditionalWaveType type, double duty_cycle);

/**
 * @brief 锯齿波样本生成（内部函数）
 * @param phase 相位(0.0-1.0)
 * @return 锯齿波值(-1.0到1.0)
 */
static double generate_sawtooth_sample(double phase);

/**
 * @brief 生成单声道通用波形
 * @param gen 波形生成器指针
 * @param buffer 输出缓冲区
 * @param count 样本数量
 */
void generate_mono_additional_wave(AdditionalWaveGenerator* gen, short* buffer, int count);
/**
 * @brief 生成立体声通用波形
 * @param left_gen 左声道波形生成器
 * @param right_gen 右声道波形生成器
 * @param stereo_buffer 输出立体声缓冲区
 * @param count 每个声道的样本数
 */
void generate_stereo_additional_wave(AdditionalWaveGenerator* left_gen,
                                   AdditionalWaveGenerator* right_gen,
                                   short* stereo_buffer, int count);

/* ============================================================================
 * 新增：立体声混合波形生成（结合原有正弦波和新增波形）
 * ============================================================================
 */

/**
 * @brief 生成立体声混合波形（左声道正弦波，右声道三角波/方波）
 * @param sine_gen 正弦波生成器（用于左声道）
 * @param additional_gen 其他波形生成器（用于右声道）
 * @param stereo_buffer 输出立体声缓冲区
 * @param count 每个声道的样本数
 * 
 * @note 此函数使用原有的正弦波生成器和新增的波形生成器
 */
void generate_stereo_mixed_wave(SineGenerator* sine_gen,
                               AdditionalWaveGenerator* additional_gen,
                               short* stereo_buffer, int count);

















#endif


