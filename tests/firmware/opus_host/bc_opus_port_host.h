#ifndef BC_OPUS_PORT_HOST_H
#define BC_OPUS_PORT_HOST_H

#include <stdbool.h>
#include <stddef.h>

void test_opus_scratch_refill(void);
size_t test_opus_scratch_high_water(void);
size_t test_opus_scratch_capacity(void);
unsigned test_opus_alloc_calls(void);
unsigned test_opus_free_calls(void);
unsigned test_opus_scratch_calls(void);
void test_opus_deny_alloc(bool deny);

#endif
