#ifndef __TX1812N5_H__
#define __TX1812N5_H__

#include <stdint.h>

struct rgb_struct
{
	uint8_t rgb_r;
	uint8_t rgb_g;
	uint8_t rgb_b;
};

void tx1812n5_RGB(struct rgb_struct * rgb_data,uint16_t rgb_num);

void tx1812n5_reset(void);

void rgb_test(void);
void rgb_test_off(void);

void rgb_test_init(void);
void tx1812n5_rgb_init(void);


#endif

