/* GNU-only replacement for the toolchain's unbounded libnosys _sbrk. */

#include "sbrk.h"

#include <stddef.h>

bool sudo_gnu_heap_advance(uintptr_t current,
                           intptr_t increment,
                           uintptr_t base,
                           uintptr_t limit,
                           uintptr_t *next)
{
    uintptr_t amount;
    uintptr_t candidate;

    if (next == NULL || base > limit ||
        (base % SUDO_GNU_SBRK_ALIGNMENT) != 0U ||
        (limit % SUDO_GNU_SBRK_ALIGNMENT) != 0U ||
        current < base || current > limit ||
        (current % SUDO_GNU_SBRK_ALIGNMENT) != 0U)
        return false;

    if (increment >= 0) {
        amount = (uintptr_t)increment;
        if (amount > (limit - current))
            return false;
        candidate = current + amount;
    } else {
        /* Avoid negating the most-negative intptr_t value. */
        amount = (uintptr_t)(-(increment + 1));
        amount += 1U;
        if (amount > (current - base))
            return false;
        candidate = current - amount;
    }

    if ((candidate % SUDO_GNU_SBRK_ALIGNMENT) != 0U)
        return false;

    *next = candidate;
    return true;
}

#ifndef SUDO_GNU_SBRK_HOST_TEST

#include <errno.h>

#include "nrf.h"

extern char __HeapBase;
extern char __HeapLimit;

static uintptr_t heap_break;
static bool heap_initialized;

void *_sbrk(ptrdiff_t increment)
{
    uintptr_t current;
    uintptr_t next;
    uintptr_t base = (uintptr_t)&__HeapBase;
    uintptr_t limit = (uintptr_t)&__HeapLimit;
    uint32_t primask = __get_PRIMASK();
    bool advanced;

    __disable_irq();

    current = heap_initialized ? heap_break : base;
    advanced = sudo_gnu_heap_advance(current, (intptr_t)increment,
                                     base, limit, &next);
    if (advanced) {
        heap_break = next;
        heap_initialized = true;
    } else {
        errno = ENOMEM;
    }

    __set_PRIMASK(primask);

    if (!advanced)
        return (void *)-1;

    return (void *)current;
}

#endif
