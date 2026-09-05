#ifndef SUDO_BATTERY_POWER_TEST_Q_DEVICE_H
#define SUDO_BATTERY_POWER_TEST_Q_DEVICE_H

#include <stddef.h>

enum result_state {
    RESULT_OK = 0,
    RESULT_OPEN_ERR = 25,
    RESULT_READ_ERR = 32
};

typedef struct q_device q_device_t;

struct q_device {
    const char *name;
};

q_device_t *q_device_find(const char *name);
int q_device_open(q_device_t *device);
int q_device_close(q_device_t *device);
int q_device_read(q_device_t *device, int position, const void *buffer,
                  int size);
void q_device_assert(q_device_t *device);

#endif
