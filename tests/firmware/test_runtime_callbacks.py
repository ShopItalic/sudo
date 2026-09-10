#!/usr/bin/env python3
"""Exercise complete RTC/motion callbacks with their real Sudo log macros.

Only peripheral/RTOS boundaries are faked. Linked ARM execution is a separate
qualification step; these checks do not simulate a SoftDevice or scheduler.
"""
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build/firmware/tests/runtime-callbacks"


def function(source, name):
    match = re.search(r"^(?:static\s+)?void\s+" + name + r"\s*\([^;]*?\)\s*\{",
                      source, re.M)
    if not match:
        raise ValueError(f"Missing callback definition: {name}")
    depth = 1
    end = match.end()
    while depth:
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
        end += 1
    return source[match.start():end] + "\n"


PREAMBLE = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define __MODULE__ "callback-test"
#include "bc_logger.h"
uint32_t test_ipsr;
static unsigned scheduler_depth, log_writes;
uint32_t test_task_ticks(void) { assert(test_ipsr == 0); return 0; }
void vTaskSuspendAll(void) { assert(test_ipsr == 0); ++scheduler_depth; }
BaseType_t xTaskResumeAll(void) { assert(scheduler_depth); --scheduler_depth; return 0; }
unsigned SEGGER_RTT_Write(unsigned index, const void *p, unsigned n) {
    (void)p; assert(index == 0 && scheduler_depth && test_ipsr == 0 && n < 240);
    ++log_writes; return n;
}

typedef unsigned nrf_drv_rtc_int_type_t;
#define array_size(x) (sizeof(x) / sizeof((x)[0]))
static uint32_t unix_time;
static uint16_t count;
static unsigned alarm_calls;
static void alarm(void) { assert(test_ipsr == 52); ++alarm_calls; }
static struct { struct tm timer_config; void (*timer_callback)(void); } bsp_list[2];

typedef void q_device_t;
static q_device_t *g_sensor_int_device_handler = (void *)1;
typedef void (*g_sensor_int_irq_callback)(void);
static g_sensor_int_irq_callback g_sensor_irq_callback;
static uint8_t sport_num;
#define G_SENSOR_TIMER_NUM 1
static void sport_count_timer_callback(void *);
static unsigned sequence, create_order, register_order, open_order;
static unsigned closes, opens, timer_starts, yields, notifications, memory_errors;
static int timer_result = pdPASS, allocation_succeeds = 1;
static int last_yield;
static int q_device_close(q_device_t *d) {
    assert(d == g_sensor_int_device_handler && test_ipsr == 22 && timer_result == pdPASS);
    assert(timer_starts); ++closes; return 0;
}
static int q_device_open(q_device_t *d) {
    assert(d == g_sensor_int_device_handler && test_ipsr == 0);
    open_order = ++sequence; ++opens; return 0;
}
static int q_device_reg_callback(q_device_t *d, unsigned n, void (*fn)(uint8_t,uint8_t)) {
    assert(d == g_sensor_int_device_handler && n == 0 && fn != NULL);
    register_order = ++sequence; return 0;
}
static TimerHandle_t test_timer_create(const char *name, uint32_t period,
        UBaseType_t autoreload, void *id, TimerCallbackFunction_t callback) {
    (void)id;
    assert(test_ipsr == 0 && name && period == 5000 && autoreload == 0 && callback);
    create_order = ++sequence;
    return allocation_succeeds ? (void *)2 : NULL;
}
#define bc_rtos_timer_create test_timer_create
static BaseType_t xTimerStartFromISR(TimerHandle_t timer, BaseType_t *woken) {
    assert(test_ipsr == 22 && timer == (void *)2 && woken && *woken == pdFALSE);
    ++timer_starts;
    if (timer_result == pdPASS) *woken = pdTRUE;
    return timer_result;
}
static void test_yield(BaseType_t woken) { ++yields; last_yield = woken; }
#define bc_portYIELD_FROM_ISR(woken) test_yield(woken)
#define NRF_ERROR_NO_MEM 4U
#define APP_ERROR_HANDLER(error) do { assert((error)==NRF_ERROR_NO_MEM); ++memory_errors; } while (0)
static void notification(void) { assert(test_ipsr == 22); ++notifications; }
'''

CHECKS = r'''
int main(void) {
    unsigned i, second;
    uint32_t values[] = {0, 86399, 86400, 1735660800U, 2147483647U, 4294967295U};
    bsp_list[0].timer_callback = alarm;
    bsp_list[1].timer_callback = NULL;
    test_ipsr = 52;
    /* Every second of a day; seven quiet ticks followed by the clock/alarm tick. */
    for (second = 0; second < 86400; ++second) {
        unix_time = second; count = 0; alarm_calls = 0;
        bsp_list[0].timer_config.tm_hour = (int)(((second + 1) % 86400) / 3600);
        bsp_list[0].timer_config.tm_min = (int)(((second + 1) / 60) % 60);
        bsp_list[0].timer_config.tm_sec = (int)((second + 1) % 60);
        for (i = 0; i < 7; ++i) bsp_rtc_callback(0);
        assert(unix_time == second && count == 7 && alarm_calls == 0);
        bsp_rtc_callback(0);
        assert(unix_time == second + 1 && count == 0 && alarm_calls == 1);
    }
    for (i = 0; i < array_size(values); ++i) {
        uint32_t next = values[i] + 1U;
        unix_time = values[i]; count = 7; alarm_calls = 0;
        bsp_list[0].timer_config.tm_hour = (int)((next % 86400U) / 3600U);
        bsp_list[0].timer_config.tm_min = (int)((next / 60U) % 60U);
        bsp_list[0].timer_config.tm_sec = (int)(next % 60U);
        bsp_rtc_callback(0);
        assert(unix_time == next && alarm_calls == 1);
    }
    assert(log_writes == 0 && scheduler_depth == 0);

    test_ipsr = 0;
    bc_gsensor_int_init();
    assert(create_order < register_order && register_order < open_order && opens == 1);
    g_sensor_irq_callback = notification;
    test_ipsr = 22;
    bc_g_sensor_int_callback(1, 1);
    assert(timer_starts == 1 && closes == 1 && yields == 1 && last_yield == pdTRUE);
    assert(sport_num == 1 && notifications == 1);
    timer_result = pdFAIL;
    bc_g_sensor_int_callback(1, 1);
    assert(timer_starts == 2 && closes == 1 && last_yield == pdFALSE);
    assert(sport_num == 2 && notifications == 2);
    g_sensor_timer[0].timer_handler = NULL;
    bc_g_sensor_int_callback(1, 1);
    assert(timer_starts == 2 && closes == 1 && last_yield == pdFALSE);

    test_ipsr = 0;
    sport_count_timer_callback(NULL);
    assert(opens == 2); /* A one-shot callback does not queue a task timer-stop. */
    allocation_succeeds = 0;
    bc_gsensor_int_init();
    assert(memory_errors == 1 && opens == 2 && scheduler_depth == 0);
    puts("RTC/motion callbacks: all 86,400 seconds, wrap, ISR timer, queue-full retry and init failure passed");
    return 0;
}
'''


def main():
    rtc = (ROOT / "firmware/bc_ros/bc_driver/bsp/src/bsp_rtc.c").read_text(errors="replace")
    motion = (ROOT / "firmware/bc_ros/bc_module/gsensor/bc_gsensor.c").read_text(errors="replace")
    start = motion.index("static bc_rtos_timer_struct  g_sensor_timer[")
    end = motion.index("};", start) + 2
    code = PREAMBLE + motion[start:end] + "\n"
    code += function(rtc, "bsp_rtc_callback")
    for name in ("bc_g_sensor_int_callback", "sport_count_timer_callback",
                 "gsensor_int_timer_create", "bc_gsensor_int_init"):
        code += function(motion, name)
    code += CHECKS
    OUT.mkdir(parents=True, exist_ok=True)
    source = OUT / "callbacks.c"
    source.write_text(code)
    command = [os.environ.get("CC", "cc"), "-std=c99", "-Wall", "-Wextra", "-Werror",
               "-Wno-unused-parameter", "-Wno-int-to-void-pointer-cast", "-g",
               "-fsanitize=address,undefined", "-DSUDO_VOICE_ONLY", "-DDEBUG_INFO=1",
               "-I" + str(ROOT / "tests/firmware/runtime"),
               "-I" + str(ROOT / "firmware/bc_ros/bc_util/bc_log"), str(source),
               str(ROOT / "firmware/bc_ros/bc_util/bc_log/bc_log_task.c"),
               "-o", str(OUT / "callbacks")]
    subprocess.run(command, check=True)
    subprocess.run([str(OUT / "callbacks")], check=True)


if __name__ == "__main__":
    main()
