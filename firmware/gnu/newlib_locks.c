/*
 * GNU-only newlib retarget locks for the Sudo Voice candidate.
 *
 * Newlib's lock ABI passes opaque pointers.  All of its process-wide and FILE
 * locks use one static token because the FreeRTOS scheduler suspension is the
 * actual coarse gate.  A nested acquire increments the FreeRTOS suspension
 * depth and a matching release decrements it.  No interrupt mask is changed
 * here; only _sbrk owns the short PRIMASK critical section.
 */

#include "newlib_locks.h"

#include <limits.h>
#include <stddef.h>

#if defined(SUDO_GNU_LOCK_HOST_TEST)

extern void vTaskSuspendAll(void);
extern int xTaskResumeAll(void);
extern int sudo_gnu_lock_host_in_isr(void);
extern void sudo_gnu_lock_host_contract_failure(void);

#else

#include "FreeRTOS.h"
#include "task.h"
#include "nrf.h"

#endif

struct __lock sudo_gnu_newlib_lock_token;

/* Define every lock object emitted by this newlib build.  Supplying these
 * symbols keeps libc_a-lock.o (the single-threaded no-op backend) out of the
 * final image while retaining the ABI expected by libc_a-mlock.o and friends.
 */
struct __lock __lock___arc4random_mutex;
struct __lock __lock___at_quick_exit_mutex;
struct __lock __lock___atexit_recursive_mutex;
struct __lock __lock___dd_hash_mutex;
struct __lock __lock___env_recursive_mutex;
struct __lock __lock___malloc_recursive_mutex;
struct __lock __lock___sfp_recursive_mutex;
struct __lock __lock___tz_mutex;

/* Counts only scheduler suspensions owned by this backend.  FreeRTOS permits
 * an unrelated external vTaskSuspendAll() nesting to remain in place. */
static unsigned int gate_depth;

static int in_isr(void)
{
#if defined(SUDO_GNU_LOCK_HOST_TEST)
    return sudo_gnu_lock_host_in_isr();
#else
    return __get_IPSR() != 0U;
#endif
}

static void contract_failure(void)
{
#if defined(SUDO_GNU_LOCK_HOST_TEST)
    /* The host shim longjmps out; this function remains non-returning from
     * the backend's point of view so an ISR can never fall through. */
    sudo_gnu_lock_host_contract_failure();
    __builtin_unreachable();
#else
    NVIC_SystemReset();
    for (;;) {
    }
#endif
}

static void require_task_context(void)
{
    if (in_isr()) {
        contract_failure();
    }
}

static void acquire_gate(void)
{
    require_task_context();

    if (gate_depth == UINT_MAX) {
        contract_failure();
    }

    (void)vTaskSuspendAll();
    ++gate_depth;
}

static void release_gate(void)
{
    require_task_context();

    if (gate_depth == 0U) {
        contract_failure();
    }

    --gate_depth;
    (void)xTaskResumeAll();
}

static void initialize_lock(sudo_gnu_lock_t *lock)
{
    if (lock != NULL) {
        *lock = &sudo_gnu_newlib_lock_token;
    }
}

void __retarget_lock_init(sudo_gnu_lock_t *lock)
{
    initialize_lock(lock);
}

void __retarget_lock_init_recursive(sudo_gnu_lock_t *lock)
{
    initialize_lock(lock);
}

void __retarget_lock_close(sudo_gnu_lock_t lock)
{
    /* The token is static for the image lifetime; close has no resource to
     * release and deliberately performs no synchronization. */
    (void)lock;
}

void __retarget_lock_close_recursive(sudo_gnu_lock_t lock)
{
    (void)lock;
}

void __retarget_lock_acquire(sudo_gnu_lock_t lock)
{
    (void)lock;
    acquire_gate();
}

void __retarget_lock_acquire_recursive(sudo_gnu_lock_t lock)
{
    (void)lock;
    acquire_gate();
}

int __retarget_lock_try_acquire(sudo_gnu_lock_t lock)
{
    (void)lock;
    acquire_gate();
    return 1;
}

int __retarget_lock_try_acquire_recursive(sudo_gnu_lock_t lock)
{
    (void)lock;
    acquire_gate();
    return 1;
}

void __retarget_lock_release(sudo_gnu_lock_t lock)
{
    (void)lock;
    release_gate();
}

void __retarget_lock_release_recursive(sudo_gnu_lock_t lock)
{
    (void)lock;
    release_gate();
}
