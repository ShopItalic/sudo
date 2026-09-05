#ifndef __CRC_H
#define __CRC_H	 
#include "stdint.h"

uint16_t Cal_Crc16(uint8_t *ucStarAddr, uint16_t uiDataLen);
uint16_t CRC16(uint8_t *_pu8Data, uint16_t _u16DataLen);
uint16_t CRC16_X25(uint8_t *puchMsg, uint32_t usDataLen);
uint16_t crc16_crc(uint8_t *ptr, uint16_t len);


uint16_t crc16tablefast(uint8_t *ptr, uint16_t len);
uint16_t crc16table(uint8_t *ptr, uint16_t len);

/*   modbus crc ะฃั้   */
uint16_t crc16bitbybit(uint8_t *ptr, uint16_t len);
uint8_t  calculate_crc(const uint8_t *ptr, uint32_t length, uint8_t crc);
uint16_t crc16(uint8_t const *buffer, uint16_t len, uint16_t crc);
#endif
