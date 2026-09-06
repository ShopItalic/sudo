#include "sbrk.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static unsigned checks;
static unsigned failures;

static void check_condition(bool condition, const char *expression,
                            unsigned line)
{
    ++checks;
    if (!condition) {
        ++failures;
        fprintf(stderr, "FAIL line %u: %s\n", line, expression);
    }
}

#define CHECK(condition) check_condition((condition), #condition, __LINE__)

static void test_forward_and_backward_steps(void)
{
    uintptr_t next = 0U;

    CHECK(sudo_gnu_heap_advance(0x1000U, 8, 0x1000U, 0x2000U, &next));
    CHECK(next == 0x1008U);
    CHECK(sudo_gnu_heap_advance(next, 0, 0x1000U, 0x2000U, &next));
    CHECK(next == 0x1008U);
    CHECK(sudo_gnu_heap_advance(next, -8, 0x1000U, 0x2000U, &next));
    CHECK(next == 0x1000U);
}

static void test_limit_and_underflow_are_rejected(void)
{
    uintptr_t next = UINTPTR_MAX;

    CHECK(!sudo_gnu_heap_advance(0x1ff8U, 16, 0x1000U, 0x2000U, &next));
    CHECK(next == UINTPTR_MAX);
    CHECK(!sudo_gnu_heap_advance(0x1000U, -8, 0x1000U, 0x2000U, &next));
    CHECK(next == UINTPTR_MAX);
    CHECK(!sudo_gnu_heap_advance(0x1000U, INTPTR_MIN,
                                  0x1000U, 0x2000U, &next));
    CHECK(next == UINTPTR_MAX);
}

static void test_alignment_and_bounds_are_rejected(void)
{
    uintptr_t next = UINTPTR_MAX;

    CHECK(!sudo_gnu_heap_advance(0x1000U, 1, 0x1000U, 0x2000U, &next));
    CHECK(!sudo_gnu_heap_advance(0x1004U, 8, 0x1000U, 0x2000U, &next));
    CHECK(!sudo_gnu_heap_advance(0x1000U, 8, 0x1004U, 0x2000U, &next));
    CHECK(!sudo_gnu_heap_advance(0x1000U, 8, 0x1000U, 0x2004U, &next));
    CHECK(!sudo_gnu_heap_advance(0x0ff8U, 8, 0x1000U, 0x2000U, &next));
    CHECK(!sudo_gnu_heap_advance(0x2008U, 0, 0x1000U, 0x2000U, &next));
    CHECK(!sudo_gnu_heap_advance(0x1000U, 8, 0x2000U, 0x1000U, &next));
    CHECK(!sudo_gnu_heap_advance(0x1000U, 8, 0x1000U, 0x2000U, NULL));
    CHECK(next == UINTPTR_MAX);
}

int main(void)
{
    test_forward_and_backward_steps();
    test_limit_and_underflow_are_rejected();
    test_alignment_and_bounds_are_rejected();

    if (failures != 0U) {
        fprintf(stderr, "%u/%u checks failed\n", failures, checks);
        return 1;
    }

    printf("GNU _sbrk bounds: %u checks passed\n", checks);
    return 0;
}
