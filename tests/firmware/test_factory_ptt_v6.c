#include "app_factory_ptt.h"
#include "app_factory_controls.h"
static unsigned actions[3] = {1,0,0};
static unsigned hold_delay_ms = 500;
unsigned app_factory_controls_hold_delay_ms(void) { return hold_delay_ms; }
unsigned app_factory_controls_action(unsigned g) { return actions[g]; }
void app_factory_controls_service(void) {}
bool app_factory_controls_command(const uint8_t *d, unsigned n) { return false; }
#include "app_pdm_handler.h"
#include "bc_rtos.h"
#include "IQS7211E.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL %u: %s\n",__LINE__,#x); exit(1); } } while(0)
static unsigned checks, starts, stops, critical, notifications, button_events;
static void dispatch_double_tap(void);
static bool running_worker;
static void enqueue_upload(void);
uint32_t fixture_ticks;
TaskHandle_t fixture_task = 1;
static uint8_t status_bytes[8];
static bool opened, fail_i2c_open, fail_read, fail_file_open, fail_file_close;
static bool inject_upload, queued_upload, legacy_race, upload_resumed;
static bool release_in_start, retouch_in_start, tap_in_start;
static unsigned start_delay;
static unsigned gestures[8];
static char upload_name[64];

void fixture_enter(void) { ++critical; }
void fixture_leave(void) { CHECK(critical); --critical; }
void fixture_notify(TaskHandle_t task) { CHECK(task == 2 && !critical); ++notifications; }
unsigned fixture_wait(unsigned clear, unsigned ticks) {
    CHECK(clear == 1 && fixture_task == 2);
    if (notifications) { notifications = 0; return 1; }
    if (running_worker) fixture_ticks += ticks;
    return 0;
}
static void service(void) {
    TaskHandle_t saved = fixture_task;
    fixture_task = 2; app_factory_ptt_worker_service(); fixture_task = saved;
}
int touch_i2c_open(void) { opened = !fail_i2c_open; return fail_i2c_open ? 1 : 0; }
int touch_i2c_close(void) { CHECK(opened); opened = false; return 0; }
bool touch_i2c_read(uint8_t reg, uint8_t *data, uint8_t length) {
    CHECK(opened);
    if (fail_read) return false;
    if (reg == 0x0e && length == 8) memcpy(data, status_bytes, 8);
    else memset(data, 0, length);
    return true;
}
bool touch_i2c_write(uint8_t reg, uint8_t *data, uint8_t length) { CHECK(opened); return true; }
uint8_t touch_io_irq_status(void) { return 0; }
int fixture_log(const char *format, ...) { return 0; }
static void report(unsigned fingers, unsigned gesture, unsigned swipe, bool dispatch) {
    memset(status_bytes, 0, 8);
    status_bytes[0] = gesture; status_bytes[1] = swipe; status_bytes[3] = fingers;
    if (!fingers) memset(status_bytes + 4, 0xff, 4);
    else { status_bytes[4] = 1; status_bytes[6] = 1; }
    TaskHandle_t saved = fixture_task; fixture_task = 1;
    Process_IQS7211E_Events(); fixture_task = saved;
    CHECK(!opened);
    if (dispatch) service();
}

/* Actual factory recorder and file-open/upload bodies are included below.
 * Hardware, LittleFS primitives and task scheduling are injected dependencies.
 * In particular, neither the recorder's busy guards nor file state transitions
 * are mocked: this covers the specific P01 shared-file race. */
#define BC_LOG_INFO(...) ((void)0)
#define LFS_SEEK_SET 0
#define BLE_UP_MODE 0
#define PPG_FILE_DATA_TASK_TYPE_UPLOAD 0
#define LINEAR_MOTOR_MIC_STOP 2
enum ppg_file_type { PPG_FILE_TYPE_16K_2_MIC_ADPCM = 8 };
enum { PPG_FLS_IDIE, PPG_FLS_UPLOAD, PPG_FLS_WRITE, PPG_FLS_BUSY };
enum { PDM_IDIE, PDM_WORK };
enum { PDM_MODE_IDIE, PDM_MODE_ONLINE, PDM_MODE_OFFLINE };
#include "app_cmd_handler.h"
struct file_state {
    int fls_status; bool ppg_file_status; char ppg_file_name[64];
    enum ppg_file_type file_type; unsigned ppg_file_sys_size_min, current_file_write_size;
};
static struct file_state app_ppg_file_hardle;
static struct app_cmd_package app_ppg_file_package;
static struct { void *thread_handler; } task_thread[1];
static unsigned pdm_status, pdm_seq, mode;
static bool pdm_stop_flag;
static unsigned recording_events;
void app_package_send_enqueue(struct app_cmd_package *packet, uint8_t length) {
    const uint8_t *p = (const uint8_t *)packet;
    CHECK(fixture_task == 2 && !critical);
    CHECK(length == 13 && p[0] == 0 && p[1] == 9 && p[2] == 0x61 && p[3] == 1);
    CHECK(p[4] == 8 && p[5] == 'P' && p[6] == '5' && p[7] == 1);
    CHECK(p[8] <= 2);
    if (p[8] == 1) CHECK(pdm_status == PDM_WORK && mode == PDM_MODE_OFFLINE);
    if (p[8] == 0) CHECK(pdm_status == PDM_IDIE && !fail_file_close);
    ++recording_events;
}

bool app_ppg_file_upload(struct app_cmd_package *pack);
static void upload(void) {
    struct app_cmd_package request = {0};
    CHECK(fixture_task == 2 && !critical);
    memset(request.data, 'U', 38); request.data[33] = '8';
    /* Factory success has no return statement; intentionally do not read it. */
    (void)app_ppg_file_upload(&request);
}
static void bc_spi_flash_device_open(void) {
    CHECK(fixture_task == 2 && !critical && !opened);
    if (inject_upload) {
        inject_upload = false;
        if (legacy_race) {
            upload();
            CHECK(app_ppg_file_hardle.fls_status == PPG_FLS_UPLOAD);
            strcpy(upload_name, app_ppg_file_hardle.ppg_file_name);
        } else {
            queued_upload = true; /* BLE arrival while this worker is busy. */
            if (running_worker) enqueue_upload();
        }
    }
    if (release_in_start || tap_in_start) {
        if (tap_in_start) dispatch_double_tap();
        else report(0, 0, 0, false);
        if (retouch_in_start) report(1, 8, 0, false);
    }
    fixture_ticks += start_delay;
}
static unsigned ppg_file_sys_size(struct file_state *f) { return 10000; }
static void bc_spi_flash_device_close(void) {}
static void lk_ppg_space_reclamation(void) { CHECK(false); }
static void ppg_file_name_create(enum ppg_file_type t) { strcpy(app_ppg_file_hardle.ppg_file_name, "/PTT-RECORDING"); }
static int lk_ppg_file_open(struct file_state *f) { return fail_file_open ? -1 : 0; }
static int lk_ppg_file_close(struct file_state *f) { return 0; }
static int ppg_file_close(struct file_state *f) { return 0; }
static void ppg_file_seek(struct file_state *f, int seek_mode) {}
static void app_package_ppg_file_uplaod(struct app_cmd_package *p, int n) {}
static void bc_rtos_delay(int n) {}
static void file_up_mode_set(int m) {}
static void bc_rtos_thread_resume(void *h) { upload_resumed = true; }
static unsigned app_ppg_file_status_get(void) { return app_ppg_file_hardle.fls_status; }
static void bc_ic_led_mic_offline_recording_on(void) {}
static void bc_ic_led_mic_offline_recording_off(void) {}
static void app_pdm_motor_start_by_config(void) {}
static void app_pdm_motor_stop_by_config(void) {}
static bool lk_app_ppg_file_close(void) { app_ppg_file_hardle.fls_status = PPG_FLS_IDIE; return !fail_file_close; }
static void app_pdm_open(void) { CHECK(fixture_task == 2 && !critical && !opened); ++starts; }
static void app_pdm_close(void) { CHECK(fixture_task == 2 && !critical && !opened); ++stops; }
static void app_pdm_mode_set(unsigned m) { mode = m; }
static unsigned app_pdm_mode_get(void) { return mode; }
static void app_touch_pdm_key_flag_set(bool flag) {}
static void bc_linear_motor_start(unsigned type) {}
static void bc_delay_ms(unsigned ms) {}
bool app_pdm_work_status(void) { return pdm_status == PDM_WORK; }
#define BUTTON_THREE_PRESS 3
static void app_package_button_up(unsigned event) { ++button_events; }
#include "factory_touch.inc"
#include "factory_recorder.inc"

/* Execute the actual patched BLE receive loop and timed queue wrapper, using
 * a bounded fake FreeRTOS queue. The control flow that serializes file I/O is
 * therefore tested, not just asserted by the fixture's manual dispatch. */
#define BC_LOG_BLE(...) ((void)0)
#define BC_LOG_HEX_P(...) ((void)0)
#define BC_LOG_ERROR(...) ((void)0)
#define bc_base_type_t int
#define bc_pdPASS 1
#define bc_rtos_taskENTER_CRITICAL() fixture_enter()
#define bc_rtos_taskEXIT_CRITICAL() fixture_leave()
#define BLE_CONNECT_IDIE_TIMEOUT_TIMER 0
#define BLE_CONN_PARAMS_FAST 0
typedef unsigned bc_queue_type;
#define BC_QUEUE_TYPE_BLE_RECV 0
struct bc_ble_data_package { uint8_t data[250]; uint16_t data_length; };
static struct bc_ble_data_package ble_recv_msg, queue_items[5];
static struct { unsigned queue_count, dequeue_conut; void *queue_handler; } bc_queue[1];
static unsigned queue_head, queue_size, commands_processed;
static uint32_t worker_deadline;
static jmp_buf worker_exit;
static void enqueue_command(unsigned cmd, unsigned sub, unsigned arg) {
    CHECK(queue_size < 5);
    unsigned index = (queue_head + queue_size++) % 5;
    memset(&queue_items[index], 0, sizeof(queue_items[index]));
    queue_items[index].data[2] = cmd; queue_items[index].data[3] = sub;
    queue_items[index].data[4] = arg; queue_items[index].data_length = 5;
    ++bc_queue[0].queue_count;
}
static void enqueue_upload(void) { enqueue_command(0x70, 0, 0); }
static int xQueueReceive(void *handle, void *buffer, uint32_t ticks) {
    CHECK(fixture_task == 2 && !critical && ticks == pdMS_TO_TICKS(50));
    if (fixture_ticks >= worker_deadline) longjmp(worker_exit, 1);
    if (!queue_size) { fixture_ticks += ticks; return 0; }
    memcpy(buffer, &queue_items[queue_head], sizeof(ble_recv_msg));
    queue_head = (queue_head + 1) % 5; --queue_size;
    return 1;
}
static void app_ble_adv_light_start(void) {}
static void app_connect_idie_timer_start(unsigned type) {}
static void params_update(unsigned params) {}
static struct { void (*ble_connect_params_update)(unsigned); } ble_calss = {params_update};
static void app_cmd_package_parse(uint8_t *data, unsigned length) {
    CHECK(fixture_task == 2 && !critical);
    ++commands_processed;
    if (data[2] == 0x70) upload();
    if (data[2] == 0x71 && data[3] == 5) {
        if (data[4]) (void)app_pdm_recording_start();
        else (void)app_pdm_recording_stop();
    }
}
#include "factory_worker.inc"
static void run_worker(unsigned duration) {
    running_worker = true; worker_deadline = fixture_ticks + duration;
    fixture_task = 2;
    if (!setjmp(worker_exit)) app_ble_recv_handler_thread(NULL);
    fixture_task = 1; running_worker = false;
    CHECK(!critical);
}

static void g0(void) { ++gestures[0]; }
/* The production IQS callback posts a touch-task event. Our counter observes
 * that callback; the test explicitly delivers app_factory_ptt_double_tap later
 * to model the separate touch-task dispatch, including an intervening Stop. */
static void g1(void) { ++gestures[1]; }
static void g2(void) { ++gestures[2]; }
static void g3(void) { ++gestures[3]; }
static void g4(void) { ++gestures[4]; }
static void g5(void) { ++gestures[5]; }
static void g6(void) { ++gestures[6]; }
static void g7(void) { ++gestures[7]; }

static void reset(void) {
    hold_delay_ms = 500;
    actions[0] = 1; actions[1] = actions[2] = 0;
    CHECK(!critical && !opened);
    memset(&factory_ptt_diagnostics, 0, sizeof(factory_ptt_diagnostics));
    memset(&app_ppg_file_hardle, 0, sizeof(app_ppg_file_hardle));
    starts = stops = button_events = notifications = recording_events = 0;
    pdm_status = PDM_IDIE; mode = PDM_MODE_IDIE; fixture_ticks = start_delay = 0;
    fail_i2c_open = fail_read = fail_file_open = fail_file_close = false;
    inject_upload = queued_upload = legacy_race = upload_resumed = false;
    release_in_start = retouch_in_start = tap_in_start = false;
    running_worker = false; queue_head = queue_size = commands_processed = 0;
    memset(bc_queue, 0, sizeof(bc_queue));
    fixture_task = 2; app_factory_ptt_worker_init(); fixture_task = 1;
    report(0, 0, 0, true);
}

int main(void) {
    unsigned i;
    single_tap_register_callback(g0); double_tap_register_callback(g1);
    triple_tap_register_callback(g2); gesture_event_hold_register_callback(g3);
    swipe_left_register_callback(g4); swipe_right_register_callback(g5);
    swipe_up_register_callback(g6); swipe_down_register_callback(g7);
    fixture_task = 2; app_factory_ptt_worker_init(); fixture_task = 1;
    report(1, 8, 0, true); CHECK(starts == 0); /* Boot held. */
    reset(); report(1, 0, 0, true); CHECK(starts == 0);
    report(1, 8, 0, false);
    app_factory_ptt_worker_service(); CHECK(starts == 0); /* Wrong task cannot start. */
    service(); CHECK(starts == 1 && app_pdm_work_status());
    for (i = 0; i < 600; ++i) {
        fixture_ticks += 102; report(1, 8, 0, true);
        CHECK(starts == 1 && stops == 0 && app_pdm_work_status());
    }
    report(0, 0, 0, false);
    app_factory_ptt_worker_service(); CHECK(stops == 0); /* Wrong task cannot stop. */
    service(); CHECK(stops == 1 && !app_pdm_work_status());

    reset(); memset(gestures, 0, sizeof(gestures));
    report(0, 7, 15, true); report(1, 8, 0, true);
    for (i = 0; i < 8; ++i) CHECK(gestures[i] == 1);
    report(2, 0, 0, true); CHECK(stops == 1); /* Original holding finger lifted. */
    reset(); report(1, 8, 0, false); report(0, 0, 0, true); CHECK(starts == 0);
    reset(); report(1, 8, 0, true); fixture_ticks += 768; service();
    CHECK(stops == 1 && factory_ptt_diagnostics.timeouts == 1);
    report(1, 8, 0, true); CHECK(starts == 1);
    report(0, 0, 0, true); report(1, 8, 0, true); CHECK(starts == 2);
    reset(); report(1, 8, 0, true); fixture_ticks += 900;
    report(1, 8, 0, true); CHECK(stops == 1);
    reset(); fixture_ticks = UINT32_MAX - 300U; report(0, 0, 0, true);
    report(1, 8, 0, true); fixture_ticks += 500U; service(); CHECK(stops == 0);
    fixture_ticks += 268U; service(); CHECK(stops == 1);

    reset(); fail_file_open = true; report(1, 8, 0, true);
    CHECK(starts == 0 && !app_pdm_work_status() && factory_ptt_diagnostics.start_failures == 1);
    fail_file_open = false; report(1, 8, 0, true); CHECK(starts == 0);
    reset(); pdm_status = PDM_WORK; mode = PDM_MODE_OFFLINE;
    report(1, 8, 0, true); report(0, 0, 0, true);
    CHECK(app_pdm_work_status() && starts == 0 && stops == 0);
    reset(); release_in_start = true; report(1, 8, 0, true);
    CHECK(starts == 1 && stops == 1 && !app_pdm_work_status());
    reset(); release_in_start = retouch_in_start = true; report(1, 8, 0, true);
    CHECK(starts == 1 && stops == 1); service(); CHECK(starts == 1);
    reset(); start_delay = 800; report(1, 8, 0, true); CHECK(starts == 1 && stops == 1);
    reset(); report(1, 8, 0, true); fail_file_close = true;
    report(0, 0, 0, true); CHECK(factory_ptt_diagnostics.stop_failures == 1);

    reset(); report(1, 8, 0, true); fail_read = true;
    Process_IQS7211E_Events(); service(); CHECK(stops == 1 && !opened);
    reset(); report(1, 8, 0, true); fail_i2c_open = true;
    Process_IQS7211E_Events(); service(); CHECK(stops == 1 && !opened && !get_chip_status());
    for (i = 8; i <= 128; i <<= 1) {
        reset(); report(1, 8, 0, true); status_bytes[2] = i;
        Process_IQS7211E_Events(); service(); CHECK(stops == 1);
    }
    reset(); report(1, 8, 0, true); status_bytes[3] = 0x11;
    Process_IQS7211E_Events(); service(); CHECK(stops == 1);

    /* Dispatch the actual generated touch branch. Double tap has no action:
     * idle, during a hold, inside a blocking Start, or delayed after release. */
    reset();
    for (i = 0; i < 4; ++i) { dispatch_double_tap(); service(); }
    CHECK(starts == 0 && stops == 0 && button_events == 0);
    report(1, 8, 0, true); dispatch_double_tap(); service();
    CHECK(starts == 1 && stops == 0 && app_pdm_work_status() && button_events == 0);
    report(1, 10, 0, true); dispatch_double_tap(); service();
    CHECK(starts == 1 && stops == 0 && app_pdm_work_status() && button_events == 0);
    report(0, 2, 0, false); service(); dispatch_double_tap(); service();
    CHECK(starts == 1 && stops == 1 && !app_pdm_work_status() && button_events == 0);
    dispatch_double_tap(); service(); CHECK(starts == 1 && stops == 1 && button_events == 0);
    reset(); tap_in_start = true; report(1, 8, 0, true); service();
    CHECK(starts == 1 && stops == 0 && app_pdm_work_status() && button_events == 0);
    report(0, 0, 0, true); CHECK(stops == 1);

    /* An external lifecycle command takes over; later finger release must
     * not stop that new recording. Read-only command leaves PTT running. */
    reset(); report(1, 8, 0, true);
    uint8_t cmd[] = {0, 0, 0x71, 2, 0};
    fixture_task = 2; app_factory_ptt_before_command(cmd, sizeof(cmd)); CHECK(stops == 0);
    cmd[3] = 5; app_factory_ptt_before_command(cmd, sizeof(cmd)); CHECK(stops == 1);
    CHECK(app_pdm_recording_start()); fixture_task = 1;
    report(0, 0, 0, true); CHECK(stops == 1 && app_pdm_work_status());

    /* Actual shared-file guards under both serialized arrival orders. */
    reset(); fixture_task = 2; upload(); fixture_task = 1;
    report(1, 8, 0, true);
    CHECK(upload_resumed && starts == 0 && app_ppg_file_hardle.fls_status == PPG_FLS_UPLOAD);
    reset(); inject_upload = true; report(1, 8, 0, true);
    CHECK(queued_upload && !upload_resumed && starts == 1);
    fixture_task = 2; upload(); fixture_task = 1;
    CHECK(!upload_resumed && app_ppg_file_hardle.fls_status == PPG_FLS_WRITE);
    CHECK(strcmp(app_ppg_file_hardle.ppg_file_name, "/PTT-RECORDING") == 0);
    report(0, 0, 0, true); CHECK(stops == 1);
    /* Negative scheduling control: reproduce P01 by interleaving the same
     * real upload body inside file open, after its idle guard. */
    reset(); inject_upload = legacy_race = true; report(1, 8, 0, true);
    CHECK(upload_resumed && starts == 1 && app_ppg_file_hardle.fls_status == PPG_FLS_WRITE);
    CHECK(strcmp(upload_name, app_ppg_file_hardle.ppg_file_name) != 0);

    /* Run the real worker control flow, including a transfer arriving in
     * Start, an empty queue, and all five receive slots occupied. */
    reset(); inject_upload = true; report(1, 8, 0, false); run_worker(100);
    CHECK(commands_processed == 1 && starts == 1 && !upload_resumed);
    CHECK(app_ppg_file_hardle.fls_status == PPG_FLS_WRITE);
    report(0, 0, 0, false); run_worker(100); CHECK(stops == 1);
    reset(); report(1, 8, 0, false); report(0, 0, 0, false);
    run_worker(100); CHECK(starts == 0);
    reset(); report(1, 8, 0, false); run_worker(900);
    CHECK(starts == 1 && stops == 1 && factory_ptt_diagnostics.timeouts == 1);
    reset(); report(1, 8, 0, true); report(0, 0, 0, false);
    for (i = 0; i < 5; ++i) enqueue_command(0x71, 2, 0);
    run_worker(100); CHECK(stops == 1 && commands_processed > 0);
    reset(); report(1, 8, 0, true); enqueue_command(0x71, 5, 1);
    run_worker(100); CHECK(starts == 2 && stops == 1 && app_pdm_work_status());
    report(0, 0, 0, false); run_worker(100); CHECK(stops == 1);
    /* Actual generated factory triple dispatch keeps its old notification
     * with action off, and executes only the assigned local action when on. */
    reset(); dispatch_triple_tap(); service(); CHECK(button_events == 1 && starts == 0);
    actions[2] = 2; dispatch_triple_tap(); service(); CHECK(button_events == 1 && starts == 1);
    dispatch_triple_tap(); service(); CHECK(stops == 1);
    for (unsigned gesture = 1; gesture <= 2; ++gesture) {
        unsigned bit = 1U << gesture;
        reset(); actions[gesture] = 2;
        report(1, 8, 0, true); CHECK(starts == 1);
        report(0, bit, 0, false); service(); CHECK(stops == 1);
        if (gesture == 1) dispatch_double_tap(); else dispatch_triple_tap();
        service(); CHECK(starts == 1 && !factory_ptt_diagnostics.memo_owned);
        /* A later, independent tap works after consuming the latch. */
        report(0, bit, 0, false);
        if (gesture == 1) dispatch_double_tap(); else dispatch_triple_tap();
        service(); CHECK(starts == 2 && factory_ptt_diagnostics.memo_owned);
        if (gesture == 1) dispatch_double_tap(); else dispatch_triple_tap();
        service(); CHECK(stops == 2 && !factory_ptt_diagnostics.memo_owned);
        /* First hold and tap flags can share the same raw frame. */
        reset(); actions[gesture] = 2;
        report(1, 8 | bit, 0, true); report(0, 0, 0, true);
        if (gesture == 1) dispatch_double_tap(); else dispatch_triple_tap();
        service(); CHECK(starts == 1 && stops == 1 && !factory_ptt_diagnostics.memo_owned);
    }
    /* The first sensor hold means 500 ms already elapsed. Exercise every
     * configured total duration with real IQS parsing and recorder bodies. */
    for(unsigned steps=1;steps<=10;++steps) {
        reset(); hold_delay_ms=steps*500U;
        uint32_t extra=pdMS_TO_TICKS(hold_delay_ms-500U);
        report(1,8,0,true);
        CHECK(starts==(steps==1 ? 1U : 0U));
        uint32_t elapsed=0;
        while(elapsed<extra) {
            uint32_t jump=extra-elapsed > 100U ? 100U : extra-elapsed;
            if(jump==extra-elapsed && jump>1U) --jump;
            elapsed+=jump; fixture_ticks+=jump; report(1,8,0,true);
            CHECK(starts==(elapsed>=extra ? 1U : 0U));
        }
        CHECK(starts==1 && app_pdm_work_status());
        for(unsigned n=0;n<15;++n) { fixture_ticks+=100; report(1,8,0,true); }
        CHECK(starts==1 && stops==0); /* Delay never caps recording length. */
        report(0,0,0,true); CHECK(stops==1 && !app_pdm_work_status());
    }
    reset(); hold_delay_ms=2000;
    report(1,8,0,true); fixture_ticks+=100; report(0,0,0,true);
    fixture_ticks+=5000; service(); CHECK(starts==0 && recording_events==0);
    /* A fresh touch must wait the whole delay again. */
    report(1,8,0,true); CHECK(starts==0);
    for(unsigned n=0;n<16;++n) { fixture_ticks+=100; report(1,8,0,true); }
    CHECK(starts==1); report(0,0,0,true); CHECK(stops==1);
    reset(); hold_delay_ms=1000; report(1,8,0,true);
    fixture_ticks+=pdMS_TO_TICKS(750); service(); CHECK(starts==0);
    report(1,8,0,true); CHECK(starts==0); /* Expired finger cannot rearm. */
    reset(); hold_delay_ms=1000; report(1,8,0,true);
    fail_read=true; report(1,8,0,true); fail_read=false;
    fixture_ticks+=1000; report(1,8,0,true); CHECK(starts==0);
    reset(); hold_delay_ms=1000; actions[1]=2;
    report(1,8|2,0,true); dispatch_double_tap(); service();
    fixture_ticks+=1000; service(); CHECK(starts==0); /* No memo from hold/tap. */
    reset(); hold_delay_ms=1000;
    fixture_ticks=UINT32_MAX-200U; report(1,8,0,true);
    for(unsigned n=0;n<5;++n) { fixture_ticks+=100; report(1,8,0,true); CHECK(starts==0); }
    fixture_ticks+=12; report(1,8,0,true); CHECK(starts==1);
    report(0,0,0,true); CHECK(stops==1);
    fprintf(stdout, "PASS P06: %u checks; actual IQS + factory recorder/file guards; double tap disabled, 60-second hold, release, faults, handoff, serialized upload; P01 race reproduced as control\n", checks);
    return 0;
}
