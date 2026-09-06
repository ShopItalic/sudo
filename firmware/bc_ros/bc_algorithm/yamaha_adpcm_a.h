#ifndef __YAMAHA_ADPCM_A_H__
#define __YAMAHA_ADPCM_A_H__


/**
 * @brief 编码PCM缓冲区为ADPCM-A格式
 * @param buffer 输入的16位PCM样本缓冲区
 * @param outbuffer 输出的ADPCM数据缓冲区（每个字节存储两个4位样本）
 * @param len 输入PCM样本的数量
 */
void yamaha_encode(int16_t *buffer,uint8_t *outbuffer,long len);

/**
 * @brief 解码ADPCM-A数据为PCM格式
 * @param buffer 输入的ADPCM数据缓冲区
 * @param outbuffer 输出的16位PCM样本缓冲区
 * @param len 输出PCM样本的数量
 */
void yamaha_decode(uint8_t *buffer,int16_t *outbuffer,long len);























#endif


