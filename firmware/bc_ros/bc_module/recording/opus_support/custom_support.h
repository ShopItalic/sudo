#ifndef SUDO_OPUS_CUSTOM_SUPPORT_H
#define SUDO_OPUS_CUSTOM_SUPPORT_H

/* libopus CUSTOM_SUPPORT hook. The codec never calls the C allocator on its
 * own: encoder/decoder state buffers are supplied by the caller and the
 * pseudostack scratch is a single bounded buffer owned by the port below.
 * Allocation failure is an explicit error, never silent audio loss. */
#include <stddef.h>

#include "opus_defines.h"
#include "bc_opus_profile.h"

/* The target passes -DGLOBAL_STACK_SIZE=BC_OPUS_SCRATCH_BYTES; host suites may
 * use a larger scratch to exercise non-profile scenarios, never a smaller one. */
#ifndef GLOBAL_STACK_SIZE
#define GLOBAL_STACK_SIZE BC_OPUS_SCRATCH_BYTES
#endif
typedef char bc_opus_scratch_size_covers_profile[
    (GLOBAL_STACK_SIZE >= BC_OPUS_SCRATCH_BYTES) ? 1 : -1];

void *bc_opus_port_alloc(size_t size);
void bc_opus_port_free(void *ptr);
void *bc_opus_port_alloc_scratch(size_t size);

#define OVERRIDE_OPUS_ALLOC
#define OVERRIDE_OPUS_FREE
#define OVERRIDE_OPUS_ALLOC_SCRATCH

static OPUS_INLINE void *opus_alloc(size_t size) { return bc_opus_port_alloc(size); }
static OPUS_INLINE void opus_free(void *ptr) { bc_opus_port_free(ptr); }
static OPUS_INLINE void *opus_alloc_scratch(size_t size)
{
    return bc_opus_port_alloc_scratch(size);
}

#endif
