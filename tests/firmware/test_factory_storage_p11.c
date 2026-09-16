/* Real P11 append/rollover wrappers and P10 reservation code; lower writes
 * inject faults/interleavings. P10's separate suite executes LittleFS itself. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks, critical, bytes, closes, opens;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"storage line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
typedef void *TaskHandle_t;
static TaskHandle_t current = (void *)1, p10_reservation_owner;
static bool p10_mounted = true, p10_storage_fault, active, fail_close, fail_open, fail_write;
static struct { bool faulted; } p10_cleanup;
enum { PPG_FLS_IDIE, PPG_FLS_WRITE, PPG_FLS_BUSY, PPG_FILE_SUCCESS, PPG_FILE_TYPE_16K_2_MIC_OPUS = 9 };
static struct file_handle { unsigned fls_status, file_type, current_file_write_size; bool ppg_file_status; } app_ppg_file_hardle;
#define taskENTER_CRITICAL() (++critical)
#define taskEXIT_CRITICAL() do { CHECK(critical); --critical; } while(0)
static TaskHandle_t xTaskGetCurrentTaskHandle(void) { return current; }
static bool factory_capture_active(void) { return active; }
static bool p10_claim_idle(void);
static bool p10_claim_cleanup(void);
static void interleave(void) {
    current = (void *)2;
    CHECK(!p10_claim_idle()); CHECK(!p10_claim_cleanup());
    current = (void *)1; CHECK(p10_reservation_owner == current);
}
static bool lk_app_ppg_file_close_p10_inner(void) {
    ++closes; CHECK(!critical); interleave();
    app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;
    app_ppg_file_hardle.ppg_file_status = false; interleave();
    return !fail_close;
}
static bool lk_app_ppg_file_open_p10_inner(unsigned type) {
    ++opens; CHECK(type == 9 && !critical); interleave();
    if (fail_open) return false;
    app_ppg_file_hardle.ppg_file_status = true;
    app_ppg_file_hardle.current_file_write_size = 0;
    return true;
}
static unsigned ppg_file_write(struct file_handle *handle, uint8_t *data, unsigned length) {
    CHECK(handle == &app_ppg_file_hardle && data && length && !critical);
    if (fail_write) return 99;
    bytes += length; return PPG_FILE_SUCCESS;
}
#include "storage.inc"
static void reset(void) {
    p10_storage_fault = fail_close = fail_open = fail_write = false;
    p10_reservation_owner = NULL; active = true;
    app_ppg_file_hardle = (struct file_handle){PPG_FLS_WRITE, 9, 0, true};
    bytes = opens = closes = 0;
}
int main(void) {
    uint8_t data[32] = {0}; unsigned i;
    reset();
    CHECK(!p11_file_write(NULL, 1) && !p11_file_write(data, 0) && !p11_file_write(data, 33));
    CHECK(p11_file_write(data, 16)); CHECK(bytes == 16 && app_ppg_file_hardle.current_file_write_size == 16);
    fail_write = true; CHECK(!p11_file_write(data, 32)); CHECK(app_ppg_file_hardle.current_file_write_size == 16);
    fail_write = false;
    for (i = 0; i < 5; ++i) {
        reset();
        if (i == 0) active = false;
        if (i == 1) app_ppg_file_hardle.fls_status = PPG_FLS_BUSY;
        if (i == 2) app_ppg_file_hardle.ppg_file_status = false;
        if (i == 3) app_ppg_file_hardle.file_type = 8;
        if (i == 4) app_ppg_file_hardle.current_file_write_size = 1024U*1024U-31;
        CHECK(!p11_file_write(data, 32)); CHECK(!bytes);
    }
    reset(); CHECK(p11_file_rollover()); CHECK(closes == 1 && opens == 1);
    CHECK(!p10_reservation_owner && app_ppg_file_hardle.fls_status == PPG_FLS_WRITE);
    CHECK(p11_file_write(data, 16));
    reset(); fail_close = true; CHECK(!p11_file_rollover()); CHECK(!opens);
    CHECK(p10_storage_fault && app_ppg_file_hardle.fls_status == PPG_FLS_BUSY && !p10_reservation_owner);
    CHECK(!p11_file_write(data, 32));
    reset(); fail_open = true; CHECK(!p11_file_rollover());
    CHECK(p10_storage_fault && !p10_reservation_owner && !p11_file_write(data, 16));
    reset(); p10_reservation_owner = (void *)2; CHECK(!p11_file_rollover()); CHECK(!closes && !opens);
    reset(); active = false; CHECK(!p11_file_rollover());
    CHECK(!critical); printf("P11 storage: %u checks, 0 failures\n",checks); return 0;
}
