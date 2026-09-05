#include "bc_util.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>


/**
 * @brief 将uint8_t数组拆分为字符数组
 * 
 * @param arr 输入的uint8_t数组
 * @param len 数组长度
 * @param output 输出的字符数组
 * @param output_size 输出字符数组的大小
 * @return 成功拆分后的字符数组长度（包括字符串结束符号'\0'），若输出空间不足返回0
 */
int splitHexArray(uint8_t *arr, int len, char *output, int output_size) 
{
    int required_size = len * 2; // 每个元素拆分为两个字符
    if (output_size < required_size) {
        return 0; // 返回0表示输出空间不足
    }

    for (int i = 0; i < len; i++) {
        snprintf(&output[i*2], 3, "%02X", arr[i]);
    }

    // 添加字符串结束符号'\0'
    output[len * 2] = '\0';

    return required_size;
}

void convert_uchar_to_hex_string(uint8_t *data,uint8_t len,uint8_t *str)
{
  for(uint8_t i=0;i<len;i++)
  {
    unsigned char num_h=data[i]/16;
    unsigned char num_l=data[i]%16;
    str[i*2]=num_h<=9?num_h+48:num_h+87;
    str[i*2+1]=num_l<=9?num_l+48:num_l+87;
   // DEBUG("%02x,%d,%d ",data[i],str[i*2],str[i*2+1]);
  }
}

void set_bit(uint8_t *data,uint32_t index,uint8_t flag)
{
  data[index/8]=flag?data[index/8]|(1 << (7-index & 7)):data[index/8] &(~(1 << (7-index & 7)));
}

//从左到右获取字节数组的第n个比特
uint8_t get_bit_lr(uint8_t*data,uint32_t index) 
{
    uint16_t char_index=index/8;
    uint8_t bit_index=7-index%8;
    uint8_t bit = ((data[char_index]>>bit_index) & 0x1);
    return bit;
}


int asciiToUint8Array(const uint8_t *ascii_arr, int len, uint8_t *output, int output_size) 
{
    if (len % 2 != 0 || len / 2 > output_size) {
        return 0; // 返回0表示输出空间不足或输入字符个数不匹配
    }

    for (int i = 0; i < len; i += 2) {
        char upper_nibble = tolower(ascii_arr[i]);
        char lower_nibble = tolower(ascii_arr[i + 1]);

        if (!isxdigit(upper_nibble) || !isxdigit(lower_nibble)) {
            return 0; // 返回0表示输入包含非法字符
        }

        output[i / 2] = (isdigit(upper_nibble) ? upper_nibble - '0' : upper_nibble - 'a' + 10) << 4 |
                        (isdigit(lower_nibble) ? lower_nibble - '0' : lower_nibble - 'a' + 10);
    }

    return len / 2;
}



static uint8_t bcd2ascii[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
 
static uint8_t ascii2bcd1[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
static uint8_t ascii2bcd2[6]  = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};



uint32_t ASC2BCD(uint8_t *bcd, const uint8_t *asc, uint32_t len)
{
    uint8_t c = 0;
    uint8_t index = 0;
    uint8_t i = 0;  
    
    len >>= 1;    
 
    for(; i < len; i++) {
        //first BCD
        if(*asc >= 'A' && *asc <= 'F') {
            index = *asc - 'A'; 
            c  = ascii2bcd2[index] << 4;
        } else if(*asc >= '0' && *asc <= '9') {
            index = *asc - '0';
            c  = ascii2bcd1[index] << 4;
        }
        asc++;
 
        //second BCD
        if(*asc >= 'A' && *asc <= 'F') {
            index = *asc - 'A'; 
            c  |= ascii2bcd2[index];
        } else if(*asc >= '0' && *asc <= '9') {
            index = *asc - '0';
            c  |= ascii2bcd1[index];
        }
        asc++;
 
        *bcd++ = c;
    }   
 
    return 0;

}



int int16ArrayToInt32Array(int16_t *arr, int32_t *output, int size) 
{
    int output_size = 0;
    for (int i = 0; i < size - 1; i += 2) {
        if (output_size < size / 2) {
            output[output_size] = (int32_t)(((uint32_t)arr[i + 1] << 16) | (uint32_t)arr[i]);
            output_size++;
        }
    }
    return output_size;
}


#define MAX_HEX_STR         4
#define MAX_HEX_STR_LENGTH  240
char hexStr[MAX_HEX_STR][MAX_HEX_STR_LENGTH];
uint8_t hexStrIdx = 0;

char* hex2Str(unsigned char * data, uint16_t dataLen)
{
    unsigned char * pin = data;
    const char * hex = "0123456789ABCDEF";
    char * pout = hexStr[hexStrIdx];
    uint8_t i = 0;
    uint8_t idx = hexStrIdx;

    if( dataLen > (MAX_HEX_STR_LENGTH/2) )
    {
      dataLen = (MAX_HEX_STR_LENGTH/2);
    }

    if(dataLen == 0)
    {
      pout[0] = 0;
    }
    else
    {
      for(; i < dataLen - 1; ++i)
      {
          *pout++ = hex[(*pin>>4)&0xF];
          *pout++ = hex[(*pin++)&0xF];
      }
      *pout++ = hex[(*pin>>4)&0xF];
      *pout++ = hex[(*pin)&0xF];
      *pout = 0;
    }

    hexStrIdx++;
    hexStrIdx %= MAX_HEX_STR;

    return hexStr[idx]; 
}
