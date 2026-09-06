#ifndef __BC_BUF_H__
#define __BC_BUF_H__



#include <stdint.h>
#include <stdbool.h>

uint8_t bc_buf_chip_id_get(void);


bool bc_buf_resp(char *challenge, unsigned int challenge_len, char *resp, unsigned int resp_len);

uint8_t bc_buf_id(char *resp);

uint8_t bc_buf_chip_id_hardware_check(void);


#endif





