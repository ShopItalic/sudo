/*******************************************************************************
此q_device为开源bsp框架，宗旨是为MCU设备提供统一的bsp接口，利于硬件设备bsp的管理，便于分层隔离。
支持armcc，gcc编译，c99标准。
严禁用于非法项目。
如若商用必须引用作者。
版本：v0.0.1
作者：邱成凯
https://gitee.com/feiniao-qiu
 *******************************************************************************/

#include "q_init.h"

void do_init_call(void)
{
#if defined(__CC_ARM)                         /* ARM Compiler */
    extern q_initcall_t q_initcall0init$$Base[];
    extern q_initcall_t q_initcall0init$$Limit[];
    extern q_initcall_t q_initcall1init$$Base[];
    extern q_initcall_t q_initcall1init$$Limit[];
    extern q_initcall_t q_initcall2init$$Base[];
    extern q_initcall_t q_initcall2init$$Limit[];


    q_initcall_t *fn;

    for (fn = q_initcall0init$$Base;
            fn < q_initcall0init$$Limit;
            fn++)
    {
        if(fn)
            (*fn)();
    }

    for (fn = q_initcall1init$$Base;
            fn < q_initcall1init$$Limit;
            fn++)
    {
        if(fn)
            (*fn)();
    }

    for (fn = q_initcall2init$$Base;
            fn < q_initcall2init$$Limit;
            fn++)
    {
        if(fn)
            (*fn)();
    }
#elif defined (__GNUC__)
    extern q_initcall_t __initcall_start[];
    extern q_initcall_t __initcall_end[];

    initcall_t *start = __initcall_start;
    initcall_t *end = __initcall_end;
    initcall_t *fn;

    for (fn = start; fn < end; fn++)
    {
        printf("initcall fn 0x%x\r\n", fn);
        (*fn)();
    }
 #endif // defined
}


void do_app_init_call(void)
{
#if defined(__CC_ARM)
    q_initcall_t *fn;
    extern q_initcall_t q_initcall3init$$Base[];
    extern q_initcall_t q_initcall3init$$Limit[];
    for (fn = q_initcall3init$$Base;
    fn < q_initcall3init$$Limit;
    fn++)
    {
        if(fn)
        (*fn)();
    }
#endif
}



















