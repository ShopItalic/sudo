#ifndef __BC_ALG_WEAR_DETECTION_H__
#define __BC_ALG_WEAR_DETECTION_H__


#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PPG_MODE_HR,    // 心率模式（绿光）
    PPG_MODE_SPO2   // 血氧模式（红外）
} ppg_mode_t;

typedef struct {
    float ppg;          // PPG原始值（根据模式选择绿光或红外）
    float accel[3];     // 三轴加速度 (x,y,z 单位: g)
} wear_sensor_data_t;

typedef struct {
    bool is_worn;               // 当前佩戴状态
    float motion_level;         // 运动强度 (0-5g)
    float signal_quality;       // 信号质量 (0-1)
    ppg_mode_t current_mode;    // 当前PPG模式
} wear_detection_result_t;

// 初始化模块
void wear_detection_init(void);

// 输入传感器数据（25Hz调用）
void wear_detection_feed_data(const wear_sensor_data_t *data);

// 切换PPG模式
void wear_detection_set_mode(ppg_mode_t mode);

// 获取检测结果
wear_detection_result_t wear_detection_get_result(void);



























#endif



