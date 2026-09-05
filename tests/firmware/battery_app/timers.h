#ifndef TEST_TIMERS_H
#define TEST_TIMERS_H
#include "FreeRTOS.h"
typedef void (*TimerCallbackFunction_t)(void *);
typedef void *TimerHandle_t;
enum { tmrCOMMAND_START, tmrCOMMAND_STOP, tmrCOMMAND_CHANGE_PERIOD, tmrCOMMAND_RESET };
TimerHandle_t xTimerCreate(const char *, TickType_t, UBaseType_t, void *, TimerCallbackFunction_t);
BaseType_t xTimerGenericCommand(TimerHandle_t, BaseType_t, TickType_t,
                                BaseType_t *, TickType_t);
#endif
