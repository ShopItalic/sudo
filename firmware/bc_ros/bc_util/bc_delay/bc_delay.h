#ifndef __BC_DELAY_H__
#define __BC_DELAY_H__


#include "ring_config.h"
#include "bc_rtos.h"

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)

#include "nrf_delay.h"

#define   bc_delay_ms(ms)    bc_rtos_delay(ms)   //
#define   bc_delay_us(us)    nrf_delay_us(us)

#define   bc_systick_get()   NRF_RTC1->COUNTER

#elif (HARDWARE_ARCH_TYPE_PHY6222 == 1)	

#include "clock.h"

#define   bc_delay_ms(ms)    WaitMs(ms)
#define   bc_delay_us(us)    WaitUs(us)

#define   bc_systick_get()     hal_systick()

#endif

























#endif




