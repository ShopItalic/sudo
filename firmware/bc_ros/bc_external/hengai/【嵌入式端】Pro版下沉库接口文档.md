# HAPro下沉算法库接口说明文档

***恒爱***
***2022年07月07日***

************************************************************************

| 版本   | 日期         | 作者  | 状态  | 描述             |
| ---- | ---------- | --- | --- | -------------- |
| V0.0 | 2022-07-07 | hl  | 初稿  | 项目初始文档         |
| v0.1 | 2022-09-22 | hl  | 修改稿 | 改接口说明          |
| v0.2 | 2022-10-31 | hl  | 修改稿 | 添加内存初始化接口      |
| v0.3 | 2023-02-01 | hl  | 修改稿 | 添加HRV中间值接口     |
| v0.4 | 2023-05-09 | hl  | 修改稿 | 添加血压接口和获取版本号接口 |
| v0.5 | 2023-11-24 | hl  | 修改稿 | 添加饮酒指数接口       |
|      |            |     |     |                |

************************************************************************

## 1 概述

    本文档定义了下沉算法库算法库接口信息和使用规范。
    本文档的主要目的是为了嵌入式工程师能够按照正确的方法使用算法库，并加快开发进度。
    本文档的阅读对象是项目相关的项目经理、嵌入式工程师。

************************************************************************

## 2 算法库接口定义

---------------------------------------------------------------------------------------------------------------------------

### 2.1 申请认证接口

##### 原型：

```
void request_validation(const unsigned char fact[], const unsigned int fact_len,const unsigned char imei[], const unsigned int imei_len,unsigned char req_code[], unsigned int* req_code_len);
```

##### 输入：

- fact，字符串型，厂商名称

- fact_len，无符号整型，字符串fact长度，不超过16个字符

- imei，字符串型，设备号

- imei_len，无符号整型，字符串imei长度，一般为12个字符

##### 输出：

- req_code，字符串型，算法库激活请求码

- req_code_len，无符号整型指针，字符串req_code长度的指针

##### 返回值：

- 无

##### 说明：

- 调用中间值、压力、心率算法之前调用此接口，并把输出的算法激活请求码req_code持久化存储在本地，并传给手机APP端；
- 手机APP端通过该请求码向服务器申请认证，认证成功则返回激活码，手表接收到此激活码后同样进行持久化存储在本地。

---------------------------------------------------------------------------------------------------------------------------

### 2.2 算法激活接口

```
int activate_algorithm(const unsigned char req_code[], const unsigned int req_code_len,const unsigned char key_code[], const unsigned int key_code_len);
```

##### 输入：

- req_code，字符串型，调用request_validation接口得到的req_code字符串（请求码）

- req_code_len，无符号整型，字符串req_code的长度

- key_code，字符串型，调用request_validation接口后从手机端返回的激活码，或者本地存储的激活码

- key_code_len，无符号整型，字符串key_code的长度

##### 输出：

- 无

##### 返回值：

- status，整型，0表示激活失败，1表示激活成功

##### 说明：

- 调用中间值、压力、心率算法之前，接收到APP端返回的激活码之后，调用此接口。

---------------------------------------------------------------------------------------------------------------------------

### 2.3 日志打印回调函数注册接口

##### 原型：

```
unsigned int extern_printf_cb_reg(extern_printf_cb_t fun);
```

##### 输入：

- fun，typedef void (*extern_printf_cb_t)(const char* format, ...)类型函数指针，改函数由应用方定义并传入给传入给注册接口

##### 返回值：

- 0。

##### 说明：

- 在应用算法之前需要把该接口注册好，方便打印算法内部日志。

---------------------------------------------------------------------------------------------------------------------------

### 2.4 内存初始化接口

##### 原型：

```
int init_algorithm(unsigned char* buf, unsigned int len);
```

##### 输入：

- buf，无符号字符串型，给算法使用的整块存储空间首地址

- len，无符号整型，给算法使用的整块存储空间地址长度（按字节计算）

##### 返回值：

- status，整型，-21表示内存空间不足，0内存空间初始化成功。

##### 说明：

- 当前算法使用大块空间输入必须大于等于 3 * 100 * SAMPLETIME * sizeof(int) + 30 * 10 * sizeof(int) + 25 * 8 * sizeof(float)。

---------------------------------------------------------------------------------------------------------------------------

### 2.5 获取库版本接口

##### 原型：

```
void version_get_string(char* string, unsigned int* str_len);
```

##### 输入：

##### 输出：

- string，字符串数组，用于存放输出的版本号。

- str_len，整型指针，存放版本号的字符长度。

##### 返回值：

- void

##### 说明：

- string的初始空间长度需要设为20以上。

---------------------------------------------------------------------------------------------------------------------------
### 2.6 中间值接口

##### 原型：

```
int ppg_midvalue(int data[], int datalen, int fs, float(** Fm)[8], int* fmlen);
```

##### 输入：

- data，整型数组，采集的ppg数据

- datalen，整型，data长度（一般为5s的数据长度，即5*fs）

- fs，整型，ppg信号的采样率（HA小板采样率为100Hz）

##### 输出：

- Fm，二维浮点型数组指针，返回执行的数据指针，该中间值用于向服务器请求服务器端算法支持。

- fmlen，整型指针，存放中间值的有效个（组）数

##### 返回值：

- status，整型，-1表示该算法未被激活，-2表示未佩戴，-3表示运动中，-4表示信号差计算失败，0表示正常计算出中间值

##### 说明：

- 一定要在调用申请认证接口和算法激活接口后再调用此接口。

- 头文件中的SAMPLETIME值定义每次输入接口的最大数据长度，例如SAMPLETIME值为5，表示每次输入接口的最大数据长度不大于5s，即5*fs个数据。

- 该接口循环调用，每满datalen长度的新数据就调用一次。
************************************************************************

## 3 算法库使用示例

```
#include "lib_hr.h"

int sec_counter = 0;    // 每1s钟增加1，通过计时器函数进行变化
unsigned char buf[8000];    // 3*100*SAMPLETIME*sizeof(int)+60*10*sizeof(int)+50*8*sizeof(float)

void test_get_version()
{
    char ver[20];
    unsigned int ver_len;
    version_get_string(ver, &ver_len);
    printf("result is %s\n%d\n", ver, ver_len);
}

void test_algorithm()
{
    // 初始化内存
    init_algorithm(buf, 8000);
    // 注册回调函数
    extern_printf_cb_reg(printf)；            // 日志打印回调函数注册，printf是外部定义的符合头文件中接口的日志打印函数
    // 激活步骤
    // 申请激活认证（第一次使用该设备和算法的时候调用，需联手机APP）
    unsigned char req_code[128];
    unsigned int req_code_len = 0;
    unsigned char* fact = "hengaigaoke";                // 获取厂商名字（不超过32个字节的字符串）
    const unsigned int fact_len = strlen(fact);
    unsigned char* imei = "584aecdd73d9";        // 获取设备号（12个字节字符串）
    const unsigned int imei_len = strlen(imei);
    request_validation(fact, fact_len, imei, imei_len, req_code, &req_code_len);

    // ToDo：根据得到的req_code_len把req_code中的有效申请码（字符串）通过蓝牙接口发送给手机端，同时把它存储到本地；

    // 手机端响应申请码后如果认证通过，会通过蓝牙发送激活码（字符串）给手表端，手表端程序需把它也存储到本地，同时调用下面的激活接口。
    // 除第一次使用设备时调用以上接口之外，之后每一次重新启动手表程序不用再调用以上接口，可直接读取存储在本地的申请码和激活码，调用以下的激活接口，即可每次激活使用。

    unsigned char* key_code;            // 通过蓝牙接口从手机端接收到的激活码（字符串）
    unsigned int key_code_len = 88;        // 通过蓝牙接口从手机端接收到的激活码（字符串）长度
    int result;
    result = activate_algorithm(req_code, req_code_len, key_code, key_code_len);  //result=1表示算法激活

    // 每次程序运行，以上激活步骤只需调用一次

    // 算法调用步骤
    int counter = 0；
    int status, fmlen;
    int len;
    int fs = SAMPLERATE;
    int segdata[SAMPLERATE*SAMPLETIME];
    const unsigned int seglen = SAMPLERATE * SAMPLETIME;
    float(* Fm)[8] = NULL;
   
    while(1)    // 改为中断循环调用（每满5s钟的数据执行一次）
    {
        len = read_segment(fp, segdata, seglen);        // 获取seglen长度的原始PPG数据，前后两段数据不重叠
        status = ppg_midvalue(segdata, len, fs, &Fm, &fmlen);		// 执行时间约2~3ms
        printf("after get_midvalue, status=%d --- Fmlen = %d\n", status, fmlen);
        printf("Fm = \n");
        for (j = 0; j < fmlen; j++)
        {
            for (k = 0; k < 8; k++)
            {
                printf("%f\t", Fm[j][k]);
                // 传给APP
            }
            printf("\n");
        }
    }
}
```

************************************************************************