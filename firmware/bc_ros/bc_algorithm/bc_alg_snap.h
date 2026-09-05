#ifndef __BC_ALG_SNAP_H__
#define __BC_ALG_SNAP_H__

#include <stdint.h>


typedef enum {
    NO_EVENT,        // 无事件
    FINGER_SNAP      // 打响指事件
} GestureEvent;


// 主处理函数，输入加速度数据，输出事件类型
GestureEvent finger_snap(int* acc);


#endif


