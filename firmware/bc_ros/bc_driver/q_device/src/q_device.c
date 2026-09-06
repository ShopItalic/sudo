
/*******************************************************************************
此q_device为开源bsp框架，宗旨是为MCU设备提供统一的bsp接口，利于硬件设备bsp的管理，便于分层隔离。
支持armcc，gcc编译，c99标准。
严禁用于非法项目。
如若商用必须引用作者。
版本：v0.0.1
作者：邱成凯
https://gitee.com/feiniao-qiu
 *******************************************************************************/

#include "q_device.h"

#include <string.h>
#include <stdbool.h>

uint32_t serial_baud_rate[UART_BAUD_RATE_NUM] = {2400,4800,9600,14400,19200,57600,115200,128000,256000,500000,1000000};

struct q_device *device_list = NULL;


/*******************************************************************************
 * Function Name     : q_device_is_exists
 * Description       : 查找设备是否存在
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年8月7日
 *******************************************************************************/
static bool q_device_is_exists( q_device_t *dev )
{
    q_device_t* cur = device_list;
    while( cur != NULL )
    {
        if( strcmp(cur->name,dev->name)==0)
        {
            return true;
        }
        cur = cur->next;
    }
    return false;
}

/*******************************************************************************
 * Function Name     : qdevice_list_inster
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年8月7日
 *******************************************************************************/
static int q_device_list_inster(q_device_t *dev)
{
    q_device_t *cur = device_list;
    if(NULL == device_list)
    {
        device_list = dev;
        dev->next   = NULL;
    }
    else
    {
        while(NULL != cur->next)
        {
            cur = cur->next;
        }
        cur->next = dev;
        dev->next = NULL;
    }
    return 1;
}


/*******************************************************************************
 * Function Name     : q_device_register
 * Description       : 驱动注册
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年8月7日
 *******************************************************************************/
int q_device_register(q_device_t *dev)
{
    if((NULL == dev) || (q_device_is_exists(dev)))
    {
        return 0;
    }

    if((NULL == dev->name) ||  (NULL == dev->dops))
    {
        return 0;
    }
    return q_device_list_inster(dev);

}

/*******************************************************************************
 * Function Name     : q_device_find
 * Description       : 驱动查找
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年8月7日
 *******************************************************************************/
q_device_t *q_device_find(const char *name)
{
    q_device_t* cur = device_list;
    while( cur != NULL )
    {
        if( strcmp(cur->name,name)==0)
        {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

/*******************************************************************************
 * Function Name     : q_device_read
 * Description       : 驱动读
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年8月7日
 *******************************************************************************/
int q_device_read(q_device_t *dev,  int pos,const void * buffer, int size)
{
    if(dev)
    {
        if(dev->dops->read)
        {
            return dev->dops->read(dev, pos, buffer, size);
        }
				else
				{
					return RESULT_READ_POINTER_NULL_ERROR;
				}
    }
		else
		{
			return RESULT_DEV_POINTER_NULL_ERROR;
		}
}

/*******************************************************************************
 * Function Name     : q_device_write
 * Description       : 驱动写
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年8月7日
 *******************************************************************************/
int q_device_write(q_device_t *dev, int pos,const void *buffer, int size)
{
    if(dev)
    {
        if(dev->dops->write)
        {
            return dev->dops->write(dev, pos, buffer, size);
        }
				else
				{
					return RESULT_WRITE_POINTER_NULL_ERROR;
				}
    }
    else
		{
			return RESULT_DEV_POINTER_NULL_ERROR;
		}
}

/*******************************************************************************
 * Function Name     : q_device_init
 * Description       : 驱动初始化
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年8月7日
 *******************************************************************************/
int q_device_init(q_device_t *dev)
{
    if(dev)
    {
        if(dev->dops->init)
        {
            return dev->dops->init(dev);
        }
				else
				{
					return RESULT_OPEN_POINTER_NULL_ERROR;
				}
    }
    else
		{
			return RESULT_DEV_POINTER_NULL_ERROR;
		}
}


/*******************************************************************************
 * Function Name     : q_device_uninit
 * Description       : 驱动
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年8月7日
 *******************************************************************************/
int q_device_uninit(q_device_t *dev)
{
    if(dev)
    {
        if(dev->dops->uninit)
        {
            return dev->dops->uninit(dev);
        }
				else
				{
					return RESULT_OPEN_POINTER_NULL_ERROR;
				}
    }
    else
		{
			return RESULT_DEV_POINTER_NULL_ERROR;
		}
}


/*******************************************************************************
 * Function Name     : q_device_open
 * Description       : 驱动打开
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年8月7日
 *******************************************************************************/
int q_device_open(q_device_t *dev)
{
    if(dev)
    {
        if(dev->dops->open)
        {
            return dev->dops->open(dev);
        }
				else
				{
					return RESULT_OPEN_POINTER_NULL_ERROR;
				}
    }
    else
		{
			return RESULT_DEV_POINTER_NULL_ERROR;
		}
}



/*******************************************************************************
 * Function Name     : q_device_close
 * Description       : 驱动关闭
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年8月7日
 *******************************************************************************/
int q_device_close(q_device_t *dev)
{
    if(dev)
    {
        if(dev->dops->close)
        {
            return dev->dops->close(dev);
        }
				else
				{
					return RESULT_CLOSE_POINTER_NULL_ERROR;
				}
    }
    else
		{
			return RESULT_DEV_POINTER_NULL_ERROR;
		}
}

/*******************************************************************************
 * Function Name     : q_device_ctrl
 * Description       : 驱动控制
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年8月7日
 *******************************************************************************/
int q_device_ctrl(q_device_t *dev,  int cmd, void *arg)
{
    if(dev)
    {
        if(dev->dops->control)
        {
            return dev->dops->control(dev, cmd, arg);
        }
		else
		{
			return RESULT_CONTROL_POINTER_NULL_ERROR;
		}
    }
    else
	{
		return RESULT_DEV_POINTER_NULL_ERROR;
	}
}

/*******************************************************************************
 * Function Name     : q_device_cfg
 * Description       : 驱动配置
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年8月7日
 *******************************************************************************/
int q_device_cfg(q_device_t *dev, void *args, void *var)
{
    if(dev)
    {
        if(dev->dops->config)
        {
            return dev->dops->config(dev, args, var);
        }
				else
				{
					return RESULT_CONFIG_POINTER_NULL_ERROR;
				}
    }
    else
		{
			return RESULT_DEV_POINTER_NULL_ERROR;
		}
}

/*******************************************************************************
 * Function Name     : q_device_reg_callback
 * Description       : 驱动注册回调函数
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年8月7日
 *******************************************************************************/
int q_device_reg_callback(q_device_t *dev,  int pos, void *callback)
{
    if(dev)
    {
        if(dev->dops->register_callback)
        {
            return dev->dops->register_callback(dev,pos,callback);
        }
				else
				{
					return RESULT_REG_CALLBACK_POINTER_NULL_ERROR;
				}
    }
    else
		{
			return RESULT_DEV_POINTER_NULL_ERROR;
		}
}

/*******************************************************************************
 * Function Name     : q_device_set_owner
 * Description       : 设置驱动属于哪个任务
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年8月7日
 *******************************************************************************/
void q_device_set_owner(q_device_t *dev, const void *owner)
{
    dev->owner = (void *)owner;
}






