#ifndef TEST_SPI_FLASH_BC_SEM_H
#define TEST_SPI_FLASH_BC_SEM_H

#include <stdbool.h>

typedef enum
{
    BC_SFUD_SEM = 0,
} bc_rtos_sem_event_type;

bool bc_rtos_sem_take(bc_rtos_sem_event_type sem_event);
bool bc_rtos_sem_give(bc_rtos_sem_event_type sem_event);

#endif
