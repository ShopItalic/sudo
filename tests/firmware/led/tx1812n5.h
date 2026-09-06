#ifndef SUDO_LED_TEST_TX1812N5_H
#define SUDO_LED_TEST_TX1812N5_H

#include <stdint.h>

struct rgb_struct
{
    uint8_t rgb_r;
    uint8_t rgb_g;
    uint8_t rgb_b;
};

void tx1812n5_RGB(struct rgb_struct *rgb_config, uint16_t count);
void tx1812n5_rgb_init(void);

#endif
