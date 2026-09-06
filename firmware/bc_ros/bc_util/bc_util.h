#ifndef __BC_UTIL_H__
#define __BC_UTIL_H__



#include <stdint.h>


#define setbit(x,y)           x|=(1<<y)
#define clrbit(x,y)           x&=~(1<<y)
#define reversebit(x,y)       x^=(1<<y)
#define getbit(x,y)         ((x) >> (y)&1)

int asciiToUint8Array(const uint8_t *ascii_arr, int len, uint8_t *output, int output_size) ;

int splitHexArray(uint8_t *arr, int len, char *output, int output_size) ;

void convert_uchar_to_hex_string(uint8_t *data,uint8_t len,uint8_t *str);

void set_bit(uint8_t *data,uint32_t index,uint8_t flag);

//从左到右获取字节数组的第n个比特
uint8_t get_bit_lr(uint8_t*data,uint32_t index);


uint32_t ASC2BCD(uint8_t *bcd, const uint8_t *asc, uint32_t len);

int int16ArrayToInt32Array(int16_t *arr, int32_t *output, int size);

char* hex2Str(unsigned char * data, uint16_t dataLen);

#endif



