#ifndef __BC_RING_PROTOCOL_H__
#define __BC_RING_PROTOCOL_H__

#include <stdint.h>

enum
{
    CMD_SET_TIME                                     = 0x10,
    CMD_GET_VERSION                                  = 0x11,
    CMD_GET_BAT                                      = 0x12,  
    CMD_GET_HRV                                      = 0x31,
    CMD_GET_SPO                                      = 0x32,
    CMD_GET_TEMP                                     = 0x34,
    CMD_GET_SPORT                                    = 0x35,
    CMD_GET_HISTORY                                  = 0x36,
    CMD_SYS_SET                                      = 0x37,
    CMD_GET_ACC                                      = 0x41,
    CMD_TOOL_TEST                                    = 0xF2,

    CMD_UNKNOW = 0xff
};

void ble_recieved_cmd_packet(void);
void bc_ring_ble_send(uint8_t* data,uint8_t data_len);

#endif