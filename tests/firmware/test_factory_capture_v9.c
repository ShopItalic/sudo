#include "app_factory_short.h"
#include "bc_rtos.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#define HANDWARE_1_23_1 1
#define HANDWARE_1_23_2 1
#define BC_LOG_INFO(...)
#define BC_QUEUE_TYPE_PDM_COLLECTION_DATA 0
#define PDM_DATA_SEND_SIZE 220
#define NRF_SUCCESS 0
#define LINEAR_MOTOR_MIC_STOP 2
enum { PDM_IDIE,PDM_WORK };
enum { PDM_MODE_IDIE,PDM_MODE_OFFLINE,PDM_MODE_KEY_OFFLINE };
enum { PPG_FILE_TYPE_16K_2_MIC_ADPCM,PPG_FILE_TYPE_16K_2_MIC_ADPCM_CAPTURE };
enum { PDM_SEND };
static struct { unsigned thread_handler; } thread_struct[1];
struct bc_adpcm_package { uint8_t pdm_data_buff[226]; };
static struct { struct bc_adpcm_package pdm_package; } pdm_ble_package;
static struct bc_adpcm_package queued;
static unsigned pending,critical,pdm_status,pdm_seq,mode,writes,finishes,uninitializations;
static bool pdm_stop_flag,buffer_requested_flag,factory_pdm_initialized,device_initialized;
static bool file_open,drain=true,discarded,queue_fails,create_fails;
static int mono_processor,pdm_config,init_error,start_error;
typedef int nrfx_err_t;
typedef void (*nrfx_pdm_event_handler_t)(void);
uint32_t fixture_ticks;
TaskHandle_t fixture_task=2;
static jmp_buf done;
void fixture_enter(void) { ++critical; }
void fixture_leave(void) { assert(critical); --critical; }
static int app_pdm_mode_get(void) { return mode; }
static void app_pdm_mode_set(unsigned value) { mode=value; }
static unsigned app_ppg_file_status_get(void) { return file_open?2:0; }
static bool lk_app_ppg_file_open(unsigned kind) { (void)kind; if(create_fails) return false; assert(!file_open); file_open=true;return true; }
static bool lk_app_ppg_file_close(void) { bool was=file_open;file_open=false;return was; }
unsigned app_factory_short_steps(void) { return 4; }
bool app_factory_capture_bind_file(void) { return file_open; }
bool app_factory_capture_finish_file(void) {
    bool ok=factory_capture_succeeded();
    assert(file_open && factory_capture_quiet()); ++finishes;
    discarded=factory_capture_should_discard();file_open=false;factory_capture_end();return ok;
}
static void app_ppg_file_write(uint8_t *data,unsigned n) {
    assert(file_open && data==pdm_ble_package.pdm_package.pdm_data_buff+6 && n==220);
    ++writes;factory_capture_written(n,true);
}
static bool bc_queue_enqueue(unsigned kind,void *data) {
    (void)kind;if(queue_fails)return false; assert(!pending);queued=*(struct bc_adpcm_package *)data;++pending;return true;
}
static bool bc_queue_dequeue(unsigned kind,void *data) {
    (void)kind;if(!pending)longjmp(done,1);*(struct bc_adpcm_package *)data=queued;--pending;return true;
}
static void bc_queue_clear(unsigned kind) { (void)kind;assert(!pending); }
static void mono_adpcm_init(void *p) { assert(p==&mono_processor); }
static void buffer_event_handler(void) {}
static int nrfx_pdm_init(void *p,nrfx_pdm_event_handler_t callback) {
    assert(p==&pdm_config && callback && !device_initialized);
    if(!init_error)device_initialized=true;return init_error;
}
static int nrfx_pdm_start(void) { assert(device_initialized && mode!=PDM_MODE_IDIE);return start_error; }
static void nrfx_pdm_stop(void) { assert(device_initialized); }
static void nrfx_pdm_uninit(void) { assert(device_initialized);device_initialized=false;++uninitializations; }
static void bc_rtos_thread_resume(unsigned handle) { (void)handle; }
static void bc_ic_led_mic_offline_recording_on(void) {}
static void bc_ic_led_mic_offline_recording_off(void) {}
static void app_pdm_motor_start_by_config(void) {}
static void app_pdm_motor_stop_by_config(void) {}
static void bc_linear_motor_start(unsigned value) { (void)value; }
static void app_touch_pdm_key_flag_set(bool on) { (void)on; }
static void bc_delay_ms(unsigned ms);
bool app_pdm_recording_stop(void);
bool app_pdm_capture_recording_stop(void);
#include "capture.inc"
static void bc_delay_ms(unsigned ms) {
    fixture_ticks+=(ms*1024U/1000U); if(drain && pending && setjmp(done)==0)app_pdm_handler_thread(NULL);
}
static void enqueue_audio(void) {
    struct bc_adpcm_package p={{0}};
    factory_capture_ready();assert(factory_capture_producer_enter());
    (void)factory_pdm_enqueue(&p);factory_capture_producer_leave();
}
int main(void) {
    unsigned kind,before;
    for(kind=0;kind<2;++kind) {
        bool (*start)(void)=kind?app_pdm_capture_recording_start:app_pdm_recording_start;
        bool (*stop)(void)=kind?app_pdm_capture_recording_stop:app_pdm_recording_stop;
        assert(start());enqueue_audio();assert(file_open && pending);
        assert(stop());assert(discarded && !file_open && !pending && !factory_capture_active());
        before=uninitializations;assert(stop() && uninitializations==before);
        assert(start());enqueue_audio();drain=false;before=finishes;
        assert(!stop() && file_open && factory_capture_active() && finishes==before);
        before=uninitializations;drain=true;
        assert(!stop());assert(!discarded && !file_open && !factory_capture_active() && uninitializations==before);
        assert(start());enqueue_audio();bc_pdm_stop();assert(!discarded && !file_open && !pending);
        assert(start());queue_fails=true;enqueue_audio();queue_fails=false;
        assert(!stop() && !discarded && !file_open);
        create_fails=true;assert(!start() && !factory_capture_active());create_fails=false;
        init_error=1;assert(!start() && !file_open && !factory_capture_active());init_error=0;
        start_error=1;assert(!start() && !device_initialized && !file_open);start_error=0;
    }
    assert(writes==6 && !critical);
    puts("PASS P09 actual recording paths: both modes, queued drain, timeout/retry, interruption, queue/create/driver failures");
    return 0;
}
