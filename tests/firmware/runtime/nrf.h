#ifndef TEST_RUNTIME_NRF_H
#define TEST_RUNTIME_NRF_H
#include <stdint.h>
extern uint32_t test_ipsr;
static inline uint32_t __get_IPSR(void) { return test_ipsr; }
#endif
