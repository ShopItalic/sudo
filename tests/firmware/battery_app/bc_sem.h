#ifndef TEST_BC_SEM_H
#define TEST_BC_SEM_H
#define BC_LOGGER_SEM 0
static inline void bc_rtos_sem_take(int value) { (void)value; }
static inline void bc_rtos_sem_give(int value) { (void)value; }
#endif
