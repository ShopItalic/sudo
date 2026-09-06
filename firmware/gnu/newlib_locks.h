/* GNU-only newlib retarget-lock interface used by host tests. */

#ifndef SUDO_GNU_NEWLIB_LOCKS_H
#define SUDO_GNU_NEWLIB_LOCKS_H

#if defined(SUDO_GNU_LOCK_HOST_TEST)
struct __lock {
    unsigned char opaque;
};
typedef struct __lock *sudo_gnu_lock_t;

void __retarget_lock_init(sudo_gnu_lock_t *lock);
void __retarget_lock_init_recursive(sudo_gnu_lock_t *lock);
void __retarget_lock_close(sudo_gnu_lock_t lock);
void __retarget_lock_close_recursive(sudo_gnu_lock_t lock);
void __retarget_lock_acquire(sudo_gnu_lock_t lock);
void __retarget_lock_acquire_recursive(sudo_gnu_lock_t lock);
int __retarget_lock_try_acquire(sudo_gnu_lock_t lock);
int __retarget_lock_try_acquire_recursive(sudo_gnu_lock_t lock);
void __retarget_lock_release(sudo_gnu_lock_t lock);
void __retarget_lock_release_recursive(sudo_gnu_lock_t lock);
#else
#include <sys/lock.h>
struct __lock {
    unsigned char opaque;
};
typedef _LOCK_T sudo_gnu_lock_t;
#endif

/* The backing token is deliberately static and allocation-free. */
extern struct __lock sudo_gnu_newlib_lock_token;

#endif
