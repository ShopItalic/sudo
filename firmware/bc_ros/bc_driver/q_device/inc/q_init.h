#ifndef __Q_INIT_H__
#define __Q_INIT_H__





#define  __used  __attribute__((__used__))

typedef void (*q_initcall_t)(void);

#define __define_initcall(fn, id) \
    static const q_initcall_t __initcall_##fn##id __used \
    __attribute__((__section__("q_initcall" #id "init"))) = fn;

#define pure_initcall(fn)       __define_initcall(fn, 0) //可用作系统时钟初始化
#define fs_initcall(fn)         __define_initcall(fn, 1) //tick和调试接口初始化
#define device_initcall(fn)     __define_initcall(fn, 2) //驱动初始化
#define late_initcall(fn)       __define_initcall(fn, 3) //传感器初始化


void do_init_call(void);
void do_app_init_call(void);







#endif

