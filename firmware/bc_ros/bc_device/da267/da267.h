#ifndef __DA267_h
#define __DA267_h

#include <stdint.h>

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
}da267_acc_data_t;

int8_t da267_init(void);
void da267_set_odr(uint8_t odr);
int8_t da267_get_id(void);
int32_t da267_read_fifo(da267_acc_data_t *p_fifo_buf,uint8_t *p_num);
void da267_sport_state(uint8_t odr);
void da267_silent_state(void);

#endif


