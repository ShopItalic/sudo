/*******************************************************************************
此q_dqueue为开源队列，宗旨是为MCU设备提供队列服务，方便数据传递
支持armcc，gcc编译，c99标准。
严禁用于非法项目。
如若商用必须引用作者。
版本：v0.0.1
作者：邱成凯
https://gitee.com/feiniao-qiu
 *******************************************************************************/


#include "q_queue.h"
#include "string.h"


/*******************************************************************************
 * Function Name     : q_queue_init
 * Description       : 初始化（创建）队列，每个队列必须先执行该函数才能使用。
 * Input             : hqueue           队列变量指针
                       buffer           队列缓存区地址
					   len              队列缓存区长度
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年5月8日
 *******************************************************************************/
void q_queue_init(QUEUE_HandleTypeDef * hqueue, QUEUE_DATA_T * buffer, unsigned int len)
{
    hqueue->buffer = buffer;
    hqueue->buffer_length = len;
    q_queue_clear(hqueue);
}

/*******************************************************************************
 * Function Name     : q_queue_clear
 * Description       : 清空队列
 * Input             : hqueue           队列变量指针
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年5月8日
 *******************************************************************************/
void q_queue_clear(QUEUE_HandleTypeDef * hqueue)
{
    hqueue->head = 0;
    hqueue->tail = 0;
}

/*******************************************************************************
 * Function Name     : q_queue_count
 * Description       : 获取队列内数据的个数
 * Input             : hqueue           队列变量指针
 * Output            : 
 * Return            : unsigned int     队列内数据的个数
 * Author            : 邱成凯
 * Modified Date:    : 2023年5月8日
 *******************************************************************************/
unsigned int q_queue_count(QUEUE_HandleTypeDef * hqueue)
{
    if(hqueue->head <= hqueue->tail)
    {
        return (unsigned int)(hqueue->tail - hqueue->head);
    }
    else
    {
        return (unsigned int)(hqueue->buffer_length + hqueue->tail - hqueue->head);
    }
}

/*******************************************************************************
 * Function Name     : q_queue_push
 * Description       : 压入数据到队列中
 * Input             : hqueue           队列变量指针
                       data             待压入队列的数据
 * Output            : 
 * Return            : 队列状态
 * Author            : 邱成凯
 * Modified Date:    : 2023年5月8日
 *******************************************************************************/
QUEUE_status_dypedef q_queue_push(QUEUE_HandleTypeDef * hqueue, QUEUE_DATA_T data)
{
	if(hqueue->mode != QUEUE_BYTE_MODE)
	{
		return QUEUE_MODE_ERROR;
	}
    unsigned int tmp = (hqueue->tail + 1) % hqueue->buffer_length;

    if(tmp == hqueue->head)
    {
        return QUEUE_OVERLOAD;
    }
    else
    {
        hqueue->buffer[hqueue->tail] = data;
        hqueue->tail = tmp;
        return QUEUE_OK;
    }
}

/*******************************************************************************
 * Function Name     : q_queue_push_array
 * Description       : 压入一组数据到队列中
 * Input             : hqueue           队列变量指针
                       pdatas           待压入队列的数组地址
                       len              待压入队列的数组长度
 * Output            : 
 * Return            : 队列状态
 * Author            : 邱成凯
 * Modified Date:    : 2023年5月8日
 *******************************************************************************/
QUEUE_status_dypedef q_queue_push_array(QUEUE_HandleTypeDef * hqueue, QUEUE_DATA_T * pdatas, unsigned int len)
{
	if(hqueue->mode != QUEUE_STRUCT_MODE)
	{
		return QUEUE_MODE_ERROR;
	}
	unsigned int tmp = (hqueue->tail + len) % hqueue->buffer_length;
	if(tmp == hqueue->head)
    {
        return QUEUE_OVERLOAD;
    }
	else
	{
	    memcpy(&hqueue->buffer[hqueue->tail],pdatas,len);
		hqueue->tail = tmp;
        return QUEUE_OK;
    }
}

/*******************************************************************************
 * Function Name     : q_queue_pop
 * Description       : 从队列中弹出数据
 * Input             : hqueue           队列变量指针
                       pdata            待弹出队列的数据缓存地址
 * Output            : 
 * Return            : 队列状态
 * Author            : 邱成凯
 * Modified Date:    : 2023年5月8日
 *******************************************************************************/
QUEUE_status_dypedef q_queue_pop(QUEUE_HandleTypeDef * hqueue, QUEUE_DATA_T * pdata)
{
	if(hqueue->mode != QUEUE_BYTE_MODE)
	{
		return QUEUE_MODE_ERROR;
	}
    if(hqueue->head == hqueue->tail)
    {
        return QUEUE_VOID;
    }
    else
    {
        *pdata = hqueue->buffer[hqueue->head];
        hqueue->head = (hqueue->head + 1) % hqueue->buffer_length;
        return QUEUE_OK;
    }
}

/*******************************************************************************
 * Function Name     : q_queue_pop_array
 * Description       : 从队列中弹出一组数据
 * Input             : hqueue           队列变量指针
                       pdata            待弹出队列的数据缓存地址
                       len              待弹出队列的数据的最大长度
 * Output            : 
 * Return            : 队列状态
 * Author            : 邱成凯
 * Modified Date:    : 2023年5月8日
 *******************************************************************************/

QUEUE_status_dypedef q_queue_pop_array(QUEUE_HandleTypeDef * hqueue, QUEUE_DATA_T * pdatas, unsigned int len)
{
	if(hqueue->mode != QUEUE_STRUCT_MODE)
	{
		return QUEUE_MODE_ERROR;
	}
     if(hqueue->head == hqueue->tail)
    {
        return QUEUE_VOID;
    }
    else
    {
		memcpy(pdatas,&hqueue->buffer[hqueue->head],len);
        hqueue->head = (hqueue->head + len) % hqueue->buffer_length;
        return QUEUE_OK;
    }
}

/*******************************************************************************
 * Function Name     : q_queue_peek
 * Description       : 从队列头部返回数据（不删除队列中的数据）
 * Input             : hqueue           队列变量指针
                       pdata            待返回队列的数据缓存地址
 * Output            : 
 * Return            : 队列状态
 * Author            : 邱成凯
 * Modified Date:    : 2023年5月8日
 *******************************************************************************/
QUEUE_status_dypedef q_queue_peek(QUEUE_HandleTypeDef * hqueue, QUEUE_DATA_T * pdata)
{
	if(hqueue->mode != QUEUE_BYTE_MODE)
	{
		return QUEUE_MODE_ERROR;
	}
    if(hqueue->head == hqueue->tail)
    {
        return QUEUE_VOID;
    }
    else
    {
        *pdata = hqueue->buffer[hqueue->head];
        return QUEUE_OK;
    }
}

/*******************************************************************************
 * Function Name     : q_queue_peek_array
 * Description       : 从队列中返回一组数据（不删除队列中的数据）
 * Input             : hqueue           队列变量指针
                       pdata            待返回队列的数据缓存地址
                       len              待返回队列的数据的最大长度
 * Output            : 
 * Return            : 实际返回数据的数量
 * Author            : 邱成凯
 * Modified Date:    : 2023年5月8日
 *******************************************************************************/
unsigned int q_queue_peek_array(QUEUE_HandleTypeDef * hqueue, QUEUE_DATA_T * pdatas, unsigned int len)
{
	if(hqueue->mode != QUEUE_STRUCT_MODE)
	{
		return QUEUE_MODE_ERROR;
	}
    unsigned int i;
    if(hqueue->head == hqueue->tail)
    {
        return 0;
    }
    if(q_queue_count(hqueue) < len)
    {
        len = q_queue_count(hqueue);
    }
    for(i=0; i<len; i++)
    {
        pdatas[i] = hqueue->buffer[(hqueue->head + i) % hqueue->buffer_length];
    }
    return len;
}




