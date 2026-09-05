/*
 * zsbm_algo.h
 *
 *  Created on: Sep 26, 2022
 *      Author: ZHOU YING
 */

#ifndef SRC_ALGORITHM_ZSBM_ALGO_H_
#define SRC_ALGORITHM_ZSBM_ALGO_H_

#include <stdint.h>

#include "zsbm_branch.h"

// #define TEST_MODE

/*======================= Part 1: Common Variables =====================*/
// return value
#define SUCCESS                 0
#define INVALID_ALGO_TYPE       -1
#define INVALID_SAMPLE_RATE_PPG -2
#define INVALID_SAMPLE_RATE_ACC -3
#define INVALID_SAMPLE_RATE_ECG -4
#define INVALID_BIT_WIGTH_PPG   -5
#define INVALID_BIT_WIGTH_ECG   -6

// RAM 建议值：单位B
#define WEARING_CHECK_ALGO_BUFFER_BYTES          452   // 400 + 40 for size of PrePackageInfoS
#define PPG_REST_HR_TIME_ALGO_BUFFER_BYTES_25HZ  1600  // 1670
#define PPG_REST_HR_TIME_ALGO_BUFFER_BYTES_50HZ  2100  // 2420
#define PPG_REST_HR_TIME_ALGO_BUFFER_BYTES_100HZ 6456
#define PPG_REST_HR_TIME_ALGO_BUFFER_BYTES_200HZ 8456
// #define PPG_REST_HR_FREQ_ALGO_BUFFER_BYTES       5000

#ifdef MODE_PPG_EXERCISE_HR_V1
#define PPG_EXERCISE_HR_ALGO_BUFFER_BYTES 10000
#elif defined MODE_PPG_EXERCISE_HR_V2
#define PPG_EXERCISE_HR_ALGO_BUFFER_BYTES 4012
#elif defined MODE_PPG_EXERCISE_HR_V3
#define PPG_EXERCISE_HR_ALGO_BUFFER_BYTES 21012
#endif

#ifdef MODE_PPG_SPO2_WITH_ACC
#define PPG_SPO2_ALGO_BUFFER_BYTES_100HZ 8260  //9088
#define PPG_SPO2_ALGO_BUFFER_BYTES_50HZ  7260  //7588

#else
#define PPG_SPO2_ALGO_BUFFER_BYTES_100HZ 7920
#define PPG_SPO2_ALGO_BUFFER_BYTES_50HZ  5920
#endif

#ifdef MODE_ECG
#ifdef MODE_ECG_CAL

// 16bits
#define ECG_ALGO_BUFFER_BYTES_250HZ  8228
#define ECG_ALGO_BUFFER_BYTES_300HZ  9092
#define ECG_ALGO_BUFFER_BYTES_500HZ  8228
#define ECG_ALGO_BUFFER_BYTES_600HZ  9092
#define ECG_ALGO_BUFFER_BYTES_1000HZ 8228
#define ECG_ALGO_BUFFER_BYTES_1200HZ 9092
// 24/32bits
#define ECG_ALGO_BUFFER_BYTES_24BIT_250HZ  8972
#define ECG_ALGO_BUFFER_BYTES_24BIT_300HZ  9988
#define ECG_ALGO_BUFFER_BYTES_24BIT_500HZ  8972
#define ECG_ALGO_BUFFER_BYTES_24BIT_600HZ  9988
#define ECG_ALGO_BUFFER_BYTES_24BIT_1000HZ 8972
#define ECG_ALGO_BUFFER_BYTES_24BIT_1200HZ 9988

#else  // only data_process

// 16bits
#define ECG_ALGO_BUFFER_BYTES_250HZ        1340
#define ECG_ALGO_BUFFER_BYTES_300HZ        1484
#define ECG_ALGO_BUFFER_BYTES_500HZ        1340
#define ECG_ALGO_BUFFER_BYTES_600HZ        1484
#define ECG_ALGO_BUFFER_BYTES_1000HZ       1340
#define ECG_ALGO_BUFFER_BYTES_1200HZ       1484
// 24/32bits
#define ECG_ALGO_BUFFER_BYTES_24BIT_250HZ  2084
#define ECG_ALGO_BUFFER_BYTES_24BIT_300HZ  2380
#define ECG_ALGO_BUFFER_BYTES_24BIT_500HZ  2084
#define ECG_ALGO_BUFFER_BYTES_24BIT_600HZ  2380
#define ECG_ALGO_BUFFER_BYTES_24BIT_1000HZ 2084
#define ECG_ALGO_BUFFER_BYTES_24BIT_1200HZ 2380


#endif
#endif  // MODE_ECG_CAL

#ifdef MODE_ECG_MEDICAL
#ifdef MODE_ECG_CAL  // data_process+ecg_cal+8000B

// 16bits
#define ECG_MEDICAL_BUFFER_BYTES_250HZ  10492
#define ECG_MEDICAL_BUFFER_BYTES_300HZ  11804
#define ECG_MEDICAL_BUFFER_BYTES_500HZ  17092
#define ECG_MEDICAL_BUFFER_BYTES_600HZ  19740
#define ECG_MEDICAL_BUFFER_BYTES_1000HZ 30316
#define ECG_MEDICAL_BUFFER_BYTES_1200HZ 35604
// 24/32bits
#define ECG_MEDICAL_BUFFER_BYTES_24BIT_250HZ  13484
#define ECG_MEDICAL_BUFFER_BYTES_24BIT_300HZ  15404
#define ECG_MEDICAL_BUFFER_BYTES_24BIT_500HZ  23092
#define ECG_MEDICAL_BUFFER_BYTES_24BIT_600HZ  26940
#define ECG_MEDICAL_BUFFER_BYTES_24BIT_1000HZ 42316
#define ECG_MEDICAL_BUFFER_BYTES_24BIT_1200HZ 50004

#else  // only data_process

// 16bits
#define ECG_MEDICAL_BUFFER_BYTES_250HZ        3604
#define ECG_MEDICAL_BUFFER_BYTES_300HZ        4196
#define ECG_MEDICAL_BUFFER_BYTES_500HZ        6596
#define ECG_MEDICAL_BUFFER_BYTES_600HZ        7796
#define ECG_MEDICAL_BUFFER_BYTES_1000HZ       12596
#define ECG_MEDICAL_BUFFER_BYTES_1200HZ       14996
// 24/32bits
#define ECG_MEDICAL_BUFFER_BYTES_24BIT_250HZ  6596
#define ECG_MEDICAL_BUFFER_BYTES_24BIT_300HZ  7796
#define ECG_MEDICAL_BUFFER_BYTES_24BIT_500HZ  12596
#define ECG_MEDICAL_BUFFER_BYTES_24BIT_600HZ  14996
#define ECG_MEDICAL_BUFFER_BYTES_24BIT_1000HZ 24596
#define ECG_MEDICAL_BUFFER_BYTES_24BIT_1200HZ 29396

#endif
#endif

/*======================= Part 2: Algo Type <> Mode =====================*/
/* algo_type 算法初始化时需提供的算法类型标识, 与 Mode 一一对应*/
#define WEARING_CHECK    (0)     // 佩戴检测,FS10HZ, RAM = 448B
#define PPG_REST_HR_TIME (1)     // 静态心率(高信号质量),时域方法,HR支持FS25HZ 50HZ 100HZ 200HZ, HRV/PPI:100HZ 200HZ
#define PPG_EXERCISE_HR  (7)     // 动态心率 PPG:25HZ;ACC:25HZ, 量程设置+-8g: 静止1g=1024code; V2V3共用算法类型，编译时区分源码，V2_RAM=4KB, V3_RAM=21KB
#define PPG_SPO2         (4)     // 血氧饱和度,RAM见PPG_SPO2_ALGO_BUFFER_BYTES_*HZ
#define PPG_SPO2_DISP    (0x14)  // 血氧饱和度(显示):spo2 display = sqrt(spo2)*10; FS、RAM同PPG_SPO2
#define PPG_SPO2_SIM     (0x24)  // 血氧饱和度(演示)，范围[97 99]%;FS、RAM同PPG_SPO2

#ifdef MODE_ECG  // 心电图展示,support 300/250/500/600/1000/1200HZ
#define ECG (0x05)
#ifdef MODE_ECG_CAL
#define MAX_PEAKS_NUMS (22)  // default 6s windows length, hr maximum 220
typedef struct {
  int16_t P;
  int16_t Q;
  int16_t R;
  int16_t S;
  int16_t T;
  int16_t PN;  // P_onset;
  int16_t TF;  // T_offset;
  int16_t AS;  // Atrial_systole
  int16_t AD;  // Atrial_diastole
  int16_t VS;  // Ventricular_systole
  int16_t VD;  // Ventriculat_diastole
} ECGPeakIndexS;

typedef struct {
  ECGPeakIndexS peaks[MAX_PEAKS_NUMS];
  uint16_t      nums;
} ECGPeakArrayS;
#endif
#endif
#ifdef MODE_ECG_MEDICAL     // 心电图展示,support 300/250/500/600/1000/1200HZ
#define ECG_MEDICAL (0x15)  // for medical
#endif
#ifdef MODE_PPG_REST_HR_FREQ  // <==> MODE_PPG_HEART_RATE_2 静态心率（低信号质量）, using fft,, no use
#define PPG_REST_HR_FREQ (2)
#endif
#ifdef MODE_PPG_EXERCISE_HR_V1  // <==> MODE_PPG_DYNAMIC_HEART_RATE
#define PPG_EXERCISE_HR_V1 (3)
#endif
#ifdef MODE_PPG_SPO2_FFT  // 血氧饱和度, use fft,FS100HZ, RAM=5.4KB, no use
#define PPG_SPO2_FFT (0x06)
#endif

// ppg_signal_level/
// 算法返回的光电信号DC强度指示
#define PPG_SIGNAL_LEVEL_NORMAL   0  // 正常
#define PPG_SIGNAL_LEVEL_INSTABLE 1  // 不稳定（“PPG_DYNAMIC_HEART_RATE”不会返回此值）
#define PPG_SIGNAL_LEVEL_LOW      2  // 过低

// 算法初始化参数
typedef struct {
  int32_t  maximum_val;      // 需要软件初始化：Maximum value of input data(maybe multi-pulse data sum)
  uint16_t sample_rate_ppg;  // 光电信号采样率，单位Hz，可选值25、50、100。“PPG_HEART_RATE”、“PPG_DYNAMIC_HEART_RATE”、“PPG_SPO2”算法需提供。
  uint16_t sample_rate_acc;  // 加速度信号采样率，单位Hz，可选值25、50、100。“PPG_DYNAMIC_HEART_RATE”算法需提供。
  uint16_t sample_rate_ecg;  // 心电信号采样率，单位Hz，可选值300/250/500/600/1000/1200。“ECG”算法需提供。

  uint8_t bit_width_ppg;  // 光电信号数据位宽（含符号位，目前支持16/24/32）。“PPG_HEART_RATE”、“PPG_DYNAMIC_HEART_RATE”、“PPG_SPO2”算法需提供。
  uint8_t bit_width_ecg;  // 心电信号数据位宽（含符号位，目前仅支持16）。“ECG”算法需提供。

  uint8_t algo_type;      // 算法执行的算法类型，见“algo_type”预定义。
  uint8_t pd_nums;        // pd numbers, default >= 1
  uint8_t wear_position;  // 佩戴位置：0：finger；1：ring; 2:wrist

} ZSBM_ALGO_INIT_PARAMETERS;

// 算法输入参数，所有数据指针所指向的原始数据，均不会被算法破坏。
typedef struct {
  /*
   * 分别通过“*green/red/ir/ecg_data”指针提供来自ZSBM芯片FIFO的原始数据
   * “MODE_PPG_REST_HR_TIME”、“MODE_PPG_EXERCISE_HR”算法提供连续绿光数据“*green_data”，多PD数据，顺序存储
   * “PPG_SPO2”算法分别提供连续的红光、红外光数据，分别使用“*red_data”和“*ir_data”
   * “ECG”算法提供连续心电数据，使用“*ecg_data”
   * 每种数据的采样率与位宽在“ZSBM_ALGO_INIT_PARAMETERS”中设置
   */

  void *green_data;  // FIFO green data, sequential storage of multi-pd data, PD0 PD1 PD2...
  void *red_data;    // FIFO red data
  void *ir_data;     // FIFO ir data
  void *ecg_data;    // FIFO ecg data ; ECG模式中ECG_CAL时间~=40ms，1200hz下0.5s数据的预处理时间~=50ms,因此建议每包500ms输入

  int8_t ptr_x;  // 需软件初始化：每位代表不同的*_data输入；bit:00000000每位分别表示----ecg_data ir_data red_data green_data

  /* ppg/red/ir/ecg_data total length, unit:Byte. SPO2_ALGO:PD4000,red+ir总字节;PD8000ir总字节数。
     ECG模式中ECG_CAL时间~=40ms，1200hz下0.5s数据的预处理时间~=50ms,因此建议每包500ms输入
  */
  uint16_t data_length;

  /*来自加速度计的数据 “PPG_DYNAMIC_HEART_RATE”算法需提供*/
  int16_t *accx_data;   // 加速度计X轴数据指针,1024code/g，测量范围+-8g。
  int16_t *accy_data;   // 加速度计Y轴数据指针,1024code/g，测量范围+-8g。
  int16_t *accz_data;   // 加速度计Z轴数据指针,1024code/g，测量范围+-8g。
  uint16_t acc_length;  // 加速度计数据长度（单位为每轴的采样点数）。

} ZSBM_ALGO_INPUT_DATA;

// 算法输出参数
typedef struct {
  /* 心率,“PPG_HEART_RATE”、“PPG_DYNAMIC_HEART_RATE”、“PPG_SPO2”、“ECG”算法会输出*/
  uint8_t heart_rate;
  uint8_t heart_rate_refreshed;  // 本次心率是否有更新。
  uint8_t hrv1;                  // 1分钟心率变异性。“PPG_HEART_RATE”、“ECG”算法会输出。
  uint8_t hrv1_refreshed;        // 本次1分钟心率变异性是否有更新。
  uint8_t hrv5;                  // 5分钟心率变异性。“PPG_HEART_RATE”、“ECG”算法会输出。
  uint8_t hrv5_refreshed;        // 本次5分钟心率变异性是否有更新。
#ifdef MODE_PPG_REST_PPI
  // 每5min的RR间期,rr_5min_list[0:rr_5min_len-1],单位ms,占用算法空间
  int16_t *rr_5min_list;  // RR间期(最大5min,每1min更新（hrv1_refreshed==1），5min清零) rr_5min_list[0:rr_5min_len-1]，单位：ms,占用算法空间
  int16_t rr_5min_len;  // RR间期的总长度，最大5min,每分钟更新（hrv1_refreshed==1），更新值为当前时间的总长度：例如，第2min更新为2min的rr间期长度，第3min更新为3min的rr间期长度
#endif
  uint8_t spo2;            // 血氧饱和度。“PPG_SPO2”算法会输出。
  uint8_t spo2_refreshed;  // 本次血氧饱和度是否有更新。

  /* 信号DC强度,“PPG_HEART_RATE”、“PPG_DYNAMIC_HEART_RATE”、“PPG_SPO2”算法会输出 */
  /* 0-正常，1-异常波动（“PPG_DYNAMIC_HEART_RATE”不会输出此项），2-DC过低 */
  uint8_t  ppg_signal_level;
  uint16_t ppg_signal_strength;            // 信号AC强度。“PPG_HEART_RATE”、“PPG_SPO2”、“ECG”算法会输出
  uint8_t  ppg_signal_strength_refreshed;  // 本次光电信号强度是否有更新。

  /* 滤波后的信号波形和长度。“PPG_HEART_RATE”、“PPG_SPO2”、“ECG”算法会输出 */
  void    *waveform;         // 大小对应当前输入算法的数据包的大小
  uint16_t waveform_length;  // 输出波形的长度，单位：采样点
  uint16_t waveform_fs;      // 输出波形的采样率，可能与输入不一样
#ifdef MODE_ECG_CAL
  const ECGPeakArrayS *wave_arr;  // 常量指针，指针指向的内容不能修改，只读
#endif

  /* calculate with red and ir signal, 0-bad, 1-medium, 2-better, 3-best */
  uint8_t spo2_confidence;

#ifdef TEST_MODE
  uint16_t test_data1;  // 测试数据输出。
  uint16_t test_data2;  // 测试数据输出。
  float    test_data3;  // 测试数据输出。
  uint8_t  test_data4;  // 测试数据输出。
#endif

} ZSBM_ALGO_OUTPUT_DATA;

/*
 * 获取算法版本
 * 返回值：32位值。低3字节为编译日期，为BCD格式。高字节低7位为版本号，二进制编码。高字节最高位为测试版本标识，此位为1的是测试版本，除输出测试数据外，其它特性与此位为0的版本相同。
 */
uint32_t ZSBM_AlgoVersion(void);

/*
 * 获取算法版本号，三段式+commit+日期的字符串格式，如 1.0.5-2a98e63(2023-12-21 18:34)
 */
const char *ZSBM_AlgoVersionInfo(void);

/*
 * 功能指示器:用于查询该版本开启了那些功能
 * 返回值：32位值。每1位代表一个功能：1：开启 0未开启
 * 0b | - - - - | - - - - | - - MODE_ECG_CAL MODE_ECG| - - - - | - - - - | - - - - | - - - MODE_PPG_REST_PPI(HR1) | - - - - |
 * 0b | 0 0 0 0 | 0 0 0 0 | 0 0    1          1      | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 1                      | 0 0 0 0 |
 */
uint32_t ZSBM_AlgoIndicator(void);

/*
 * 算法初始化
 * 作用：获得算法执行所需的buffer指针以及算法参数。
 * 传入参数1（algo_buffer_external）：供算法使用的buffer指针，长度“ALGO_BUFFER_LENGTH”。直至调用“ZSBM_AlgoFree”之前，外部程序不得使用。
 * 传入参数2（zsbm_algo_init_para）：传入的算法初始化所需参数，见“ZSBM_ALGO_INIT_PARAMETERS”定义。
 * 返回值：见“return value”预定义。
 */
int8_t ZSBM_AlgoInit(uint8_t *algo_buffer_external, ZSBM_ALGO_INIT_PARAMETERS *zsbm_algo_init_para);

/*
 * 算法释放
 * 作用：清空并释放算法所占用的RAM空间。
 * 返回值：0。
 */
int8_t ZSBM_AlgoFree(void);

/*
* 算法更新数据
* 作用：定期向算法传入新的数据，并获取返回。
* 传入参数1（algo_buffer_external）：供算法使用的buffer指针，长度“ALGO_BUFFER_LENGTH”。直至调用“ZSBM_AlgoFree”之前，外部程序不得使用。
* 传入参数2（input_data）：传入待处理数据，见“ZSBM_ALGO_INPUT_DATA”定义。
* 传入参数3（output_data）：传出算法输出数据，见“ZSBM_ALGO_OUTPUT_DATA”定义。
* 返回值：0。
*/
int8_t ZSBM_AlgoUpdateData(uint8_t *algo_buffer_external, ZSBM_ALGO_INPUT_DATA *input_data, ZSBM_ALGO_OUTPUT_DATA *output_data);

/************************************************************************************
 * 功  能: 转换red和ir的数据格式，不增加额外内存，在原有数据内存直接调整，from 
 * raw_data
 *    |
 *    V
 * {red_1, ir_1, red_2, ir_2 ... ... red_n, ir_n}  to
 * out_red                 out_ir
 *    |                       |
 *    V                       V
 * {red_1, red_2 ... red_n, ir_1, ir_2 ... ir_n}
 * 参  数: *raw_data：原格式数据指针，tot_bytes：总字节数，bit_width：数据位宽
 * 返回值: *out_red 和 *out_ir分别指向转换后数据的red和ir起始位置，*tot_bytes减半
 ************************************************************************************/
void zsbm_convert_red_ir_data_format(void *raw_data, uint16_t *tot_bytes, uint8_t bit_width, void **out_red, void **out_ir);

#ifdef MODE_ECG
typedef struct {
  int32_t prev_val;  // previous predict value
  int32_t index;     // step table index
} AdpcmStateS;       // ADPCM state struct
/**
 * @brief : 解压ADPCM数据，分别支持 4bits->16bits 和 6bits->24bits的解压
 * @param  *indata: 输入压缩ADPCM数据指针，uint8类型
 * @param  input_len: 输入压缩ADPCM数据的长度，即uint8类型的元素个数
 * @param  *outdata: 输出解压后的数据指针，按照解压方法，分别为int16和int32类型
 * @param  *ouput_len: 输出解压后的数据长度，输入时初始化为0，函数执行完毕，算法更新为实际的元素个数
 * @param  *state: 解析过程的中间状态，第一次解析，初始化为00，如果连续解析多个数据包，此状态需要维持，不能重复初始化
 * @return void, 无返回值，更新数据指针 *outputdata 和对应的数据长度 *output_len
 */
void zsbm_adpcm_decoder_16bits(uint8_t *indata, uint32_t input_len, int16_t *outdata, uint32_t *ouput_len, AdpcmStateS *state);
void zsbm_adpcm_decoder_24bits(uint8_t *indata, uint32_t input_len, int32_t *outdata, uint32_t *ouput_len, AdpcmStateS *state);
#endif

#endif /* SRC_ALGORITHM_ZSBM_ALGO_H_ */
