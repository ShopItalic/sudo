/* Host implementation of the libopus allocation port used by every Sudo
 * codec test. The scratch buffer is static and pattern-filled so tests can
 * measure the pseudostack high-water mark that bounds the target profile. */
#include "custom_support.h"
#include "bc_opus_port_host.h"

#include <stdlib.h>
#include <string.h>

#define FILL 0xa5U

static unsigned char scratch[GLOBAL_STACK_SIZE];
static unsigned alloc_calls, free_calls, scratch_calls;
static bool deny_alloc;

void *bc_opus_port_alloc(size_t size)
{
    ++alloc_calls;
    if (deny_alloc) return NULL;
    return malloc(size);
}

void bc_opus_port_free(void *ptr)
{
    ++free_calls;
    free(ptr);
}

void *bc_opus_port_alloc_scratch(size_t size)
{
    ++scratch_calls;
    if (size != GLOBAL_STACK_SIZE) abort();
    memset(scratch, FILL, sizeof(scratch));
    return scratch;
}

void test_opus_scratch_refill(void)
{
    memset(scratch, FILL, sizeof(scratch));
}

size_t test_opus_scratch_high_water(void)
{
    size_t i;
    for (i = sizeof(scratch); i > 0U; --i)
        if (scratch[i - 1U] != FILL) return i;
    return 0U;
}

size_t test_opus_scratch_capacity(void) { return sizeof(scratch); }
unsigned test_opus_alloc_calls(void) { return alloc_calls; }
unsigned test_opus_free_calls(void) { return free_calls; }
unsigned test_opus_scratch_calls(void) { return scratch_calls; }
void test_opus_deny_alloc(bool deny) { deny_alloc = deny; }
