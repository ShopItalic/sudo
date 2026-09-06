#include "newlib_locks.h"

#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>

static unsigned checks;
static unsigned failures;
static unsigned suspend_calls;
static unsigned resume_calls;
static unsigned suspend_depth;
static bool host_in_isr;
static bool scheduler_started;
static unsigned contract_failures;
static jmp_buf contract_jump;

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

void vTaskSuspendAll(void)
{
    ++suspend_calls;
    ++suspend_depth;
}

int xTaskResumeAll(void)
{
    ++resume_calls;
    CHECK(suspend_depth > 0U);
    if (suspend_depth > 0U) {
        --suspend_depth;
    }
    return 0;
}

int sudo_gnu_lock_host_in_isr(void)
{
    return host_in_isr ? 1 : 0;
}

void sudo_gnu_lock_host_contract_failure(void)
{
    ++contract_failures;
    longjmp(contract_jump, 1);
}

static void reset_gate_state(void)
{
    CHECK(suspend_depth == 0U);
    suspend_calls = 0U;
    resume_calls = 0U;
}

static void test_initialization(void)
{
    sudo_gnu_lock_t lock = NULL;
    sudo_gnu_lock_t recursive = NULL;

    __retarget_lock_init(&lock);
    __retarget_lock_init_recursive(&recursive);
    CHECK(lock == &sudo_gnu_newlib_lock_token);
    CHECK(recursive == &sudo_gnu_newlib_lock_token);
    __retarget_lock_close(lock);
    __retarget_lock_close_recursive(recursive);
    CHECK(suspend_depth == 0U);
}

static void test_nested_recursive_gate(void)
{
    reset_gate_state();
    host_in_isr = false;

    __retarget_lock_acquire_recursive(NULL);
    __retarget_lock_acquire_recursive(&sudo_gnu_newlib_lock_token);
    __retarget_lock_acquire(NULL);
    CHECK(suspend_depth == 3U);
    CHECK(suspend_calls == 3U);

    __retarget_lock_release_recursive(NULL);
    __retarget_lock_release_recursive(NULL);
    __retarget_lock_release(NULL);
    CHECK(suspend_depth == 0U);
    CHECK(resume_calls == 3U);
}

static void test_try_gate(void)
{
    reset_gate_state();
    CHECK(__retarget_lock_try_acquire(NULL) == 1);
    CHECK(__retarget_lock_try_acquire_recursive(NULL) == 1);
    CHECK(suspend_depth == 2U);
    __retarget_lock_release(NULL);
    __retarget_lock_release_recursive(NULL);
    CHECK(suspend_depth == 0U);
}

static void test_pre_scheduler_gate(void)
{
    reset_gate_state();
    host_in_isr = false;
    scheduler_started = false;

    __retarget_lock_acquire_recursive(NULL);
    CHECK(suspend_depth == 1U);
    __retarget_lock_release_recursive(NULL);
    CHECK(suspend_depth == 0U);
    CHECK(!scheduler_started);

    scheduler_started = true;
}

static void expect_contract_failure(void (*operation)(void))
{
    int jumped = setjmp(contract_jump);
    if (jumped == 0) {
        operation();
        CHECK(false);
    } else {
        CHECK(contract_failures > 0U);
    }
}

static void bad_acquire(void)
{
    __retarget_lock_acquire(NULL);
}

static void bad_recursive_acquire(void)
{
    __retarget_lock_acquire_recursive(NULL);
}

static void bad_try(void)
{
    (void)__retarget_lock_try_acquire(NULL);
}

static void bad_recursive_try(void)
{
    (void)__retarget_lock_try_acquire_recursive(NULL);
}

static void bad_release(void)
{
    __retarget_lock_release(NULL);
}

static void test_unmatched_release_and_external_gate(void)
{
    reset_gate_state();
    host_in_isr = false;
    contract_failures = 0U;

    expect_contract_failure(bad_release);
    CHECK(contract_failures == 1U);
    CHECK(suspend_calls == 0U);
    CHECK(resume_calls == 0U);

    /* The backend must balance only its own nesting and leave an external
     * scheduler suspension untouched. */
    vTaskSuspendAll();
    CHECK(suspend_depth == 1U);
    __retarget_lock_acquire_recursive(NULL);
    CHECK(suspend_depth == 2U);
    __retarget_lock_release_recursive(NULL);
    CHECK(suspend_depth == 1U);
    CHECK(resume_calls == 1U);
    (void)xTaskResumeAll();
    CHECK(suspend_depth == 0U);
}

static void test_isr_contract(void)
{
    reset_gate_state();
    host_in_isr = true;
    contract_failures = 0U;

    expect_contract_failure(bad_acquire);
    expect_contract_failure(bad_recursive_acquire);
    expect_contract_failure(bad_try);
    expect_contract_failure(bad_recursive_try);
    expect_contract_failure(bad_release);
    CHECK(contract_failures == 5U);
    CHECK(suspend_calls == 0U);
    CHECK(resume_calls == 0U);
    CHECK(suspend_depth == 0U);

    host_in_isr = false;
}

int main(void)
{
    test_initialization();
    test_nested_recursive_gate();
    test_try_gate();
    test_pre_scheduler_gate();
    test_unmatched_release_and_external_gate();
    test_isr_contract();

    if (failures != 0U) {
        fprintf(stderr, "%u/%u checks failed\n", failures, checks);
        return 1;
    }

    printf("GNU newlib retarget locks: %u checks passed (scheduler=%s)\n",
           checks, scheduler_started ? "started" : "stopped");
    return 0;
}
