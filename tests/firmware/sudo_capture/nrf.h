#ifndef SUDO_CAPTURE_TEST_NRF_H
#define SUDO_CAPTURE_TEST_NRF_H

#include <stdint.h>

/* The capture adapter uses the Cortex-M DWT cycle counter for encode timing
 * instrumentation. The host shim advances the counter on every read so the
 * measured values are deterministic and nonzero. */
typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t CYCCNT;
} test_dwt_t;

typedef struct {
    volatile uint32_t DEMCR;
} test_coredebug_t;

extern test_dwt_t test_dwt;
extern test_coredebug_t test_coredebug;
extern uint32_t SystemCoreClock;
extern uint32_t test_cycles_per_read;

#define CoreDebug (&test_coredebug)
#define CoreDebug_DEMCR_TRCENA_Msk (1UL << 24)
#define DWT_CTRL_CYCCNTENA_Msk (1UL << 0)

/* Every access advances the counter; the production code reads CYCCNT twice
 * per encode so each frame costs test_cycles_per_read cycles in the shim. */
static inline test_dwt_t *test_dwt_access(void)
{
    test_dwt.CYCCNT += test_cycles_per_read;
    return &test_dwt;
}
#define DWT (test_dwt_access())

#endif
