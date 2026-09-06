#ifndef __Q_QUEUE_H__
#define __Q_QUEUE_H__


#define Q_QUEUE_VERSION   "v0.0.2"

//==============================================================================
//  常量定义
//==============================================================================
#define QUEUE_DATA_T  unsigned char         //队列数据类型定义

//==============================================================================
//  类型定义
//==============================================================================
typedef enum
{
	QUEUE_BYTE_MODE = 0,                    //字节模式 一字节为单位，单字节push pop，内存对齐
	QUEUE_STRUCT_MODE = 1,                  //以结构体类型为单位，结构体类型push pop，内存对齐
}QUEUE_mode;
typedef struct QUEUE_HandleTypeDef{
    unsigned int head;                      //队列头指针
    unsigned int tail;                      //队列尾指针
    unsigned int buffer_length;             //队列缓存长度（初始化时赋值）
	QUEUE_mode   mode;                      //队列模式 
    QUEUE_DATA_T * buffer;		            //队列缓存数组（初始化时赋值）
}QUEUE_HandleTypeDef;

typedef enum
{
    QUEUE_OK           = 0x00U,                 //队列OK
    QUEUE_ERROR        = 0x01U,                 //队列错误
    QUEUE_BUSY         = 0x02U,                 //队列忙
    QUEUE_TIMEOUT      = 0x03U,                 //队列超时
    QUEUE_OVERLOAD     = 0x04U,                 //队列已满
    QUEUE_VOID         = 0x05U,                 //队列已空
	QUEUE_MODE_ERROR   = 0x06U                  //队列模式错误
} QUEUE_status_dypedef;


//==============================================================================
//  公共函数声明
//==============================================================================
void q_queue_init(QUEUE_HandleTypeDef * hqueue, QUEUE_DATA_T * buffer, unsigned int len);

void q_queue_clear(QUEUE_HandleTypeDef * hqueue);

unsigned int q_queue_count(QUEUE_HandleTypeDef * hqueue);

QUEUE_status_dypedef q_queue_push(QUEUE_HandleTypeDef * hqueue, QUEUE_DATA_T data);

QUEUE_status_dypedef q_queue_push_array(QUEUE_HandleTypeDef * hqueue, QUEUE_DATA_T * pdatas, unsigned int len);

QUEUE_status_dypedef q_queue_pop(QUEUE_HandleTypeDef * hqueue, QUEUE_DATA_T * pdata);

QUEUE_status_dypedef q_queue_pop_array(QUEUE_HandleTypeDef * hqueue, QUEUE_DATA_T * pdatas, unsigned int len);

QUEUE_status_dypedef q_queue_peek(QUEUE_HandleTypeDef * hqueue, QUEUE_DATA_T * pdata);

unsigned int q_queue_peek_array(QUEUE_HandleTypeDef * hqueue, QUEUE_DATA_T * pdatas, unsigned int len);

#endif

