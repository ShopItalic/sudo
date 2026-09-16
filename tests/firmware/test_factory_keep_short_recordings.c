#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bc_rtos.h"
#include "app_factory_short.c"
#include "app_factory_short_settings.c"

static unsigned checks, critical, closes, power_closes, replies;
static bool close_ok = true;
static uint8_t response[17];
uint32_t fixture_ticks;
TaskHandle_t fixture_task = 2;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
void fixture_enter(void) { ++critical; }
void fixture_leave(void) { assert(critical); --critical; }
void app_package_send_enqueue(struct app_cmd_package *p, uint8_t n) {
    CHECK(n == 17 && !critical); memcpy(response, p, n); ++replies;
}
enum { PPG_FLS_IDIE, PPG_FLS_WRITE, PPG_FLS_BUSY, PPG_FILE_SUCCESS = 0 };
static struct { bool ppg_file_status; unsigned fls_status; } app_ppg_file_hardle;
static unsigned lk_ppg_file_close(void *handle) {
    CHECK(handle == &app_ppg_file_hardle); ++closes;
    app_ppg_file_hardle.ppg_file_status = false;
    return close_ok ? PPG_FILE_SUCCESS : 42;
}
static void bc_spi_flash_device_close(void) { ++power_closes; }
#include "retained_finish.inc"

static void start(unsigned steps) {
    CHECK(factory_capture_begin(steps));
    app_ppg_file_hardle.ppg_file_status = true;
    app_ppg_file_hardle.fls_status = PPG_FLS_WRITE;
    CHECK(app_factory_capture_bind_file()); factory_capture_enable();
}
int main(void) {
    unsigned step, size, before;
    uint8_t get[9] = {0,42,0x84,0x13,1,1,2,3,4};
    uint8_t set[14] = {0,43,0x84,0x14,1,9,8,7,6};
    app_factory_short_init(); app_factory_short_service();
    CHECK(app_factory_short_steps() == 0);
    CHECK(app_factory_short_command(get, sizeof(get)) && response[9] == OK);
    CHECK(!memcmp(response, get, 9));
    for (size = 10; size < sizeof(response); ++size) CHECK(response[size] == 0);
    for (step = 0; step < 256; ++step) {
        set[13] = step;
        CHECK(app_factory_short_command(set, sizeof(set)));
        CHECK(response[9] == (step ? INVALID : OK));
        CHECK(app_factory_short_steps() == 0 && response[14] == 0);
        for (size = 0; size < 4; ++size) {
            uint8_t header[2];
            start(step); before = closes;
            CHECK(!app_factory_capture_finish_file() && closes == before);
            factory_capture_ready(); CHECK(factory_capture_producer_enter());
            CHECK(factory_capture_queue_begin(header));
            factory_capture_stop_accepting();
            CHECK(!factory_capture_quiet() && !app_factory_capture_finish_file());
            factory_capture_producer_leave();
            CHECK(factory_capture_sender_enter(header));
            factory_capture_written(size == 3 ? 0xffffffffU : size, true);
            factory_capture_sender_leave();
            CHECK(factory_capture_quiet() && !factory_capture_should_discard());
            CHECK(app_factory_capture_finish_file() && closes == before + 1);
            CHECK(!factory_capture_active() && app_ppg_file_hardle.fls_status == PPG_FLS_IDIE);
        }
    }
    set[9] = 1; set[13] = 0;
    CHECK(app_factory_short_command(set, sizeof(set)) && response[9] == CONFLICT);
    for (size = 0; size < 9; ++size) {
        before = replies; app_factory_short_command(get, size); CHECK(replies == before);
    }
    start(4); factory_capture_fault(); factory_capture_stop_accepting();
    before = closes; CHECK(!app_factory_capture_finish_file() && closes == before + 1);
    CHECK(app_ppg_file_hardle.fls_status == PPG_FLS_IDIE);
    start(4); factory_capture_stop_accepting(); close_ok = false;
    CHECK(!app_factory_capture_finish_file() && app_ppg_file_hardle.fls_status == PPG_FLS_BUSY);
    CHECK(!critical && closes == power_closes);
    printf("Keep-short-recordings: %u checks passed\n", checks);
    return 0;
}
