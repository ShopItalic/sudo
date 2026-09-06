#ifndef SUDO_GNU_SBRK_H
#define SUDO_GNU_SBRK_H

/*
 * Bounds-only part of the GNU _sbrk port.  Keeping this operation free of
 * target headers makes the boundary arithmetic testable on the host while
 * ensuring that the target wrapper uses exactly the same checks.
 */

#include <stdbool.h>
#include <stdint.h>

#define SUDO_GNU_SBRK_ALIGNMENT ((uintptr_t)8U)

bool sudo_gnu_heap_advance(uintptr_t current,
                           intptr_t increment,
                           uintptr_t base,
                           uintptr_t limit,
                           uintptr_t *next);

#endif
