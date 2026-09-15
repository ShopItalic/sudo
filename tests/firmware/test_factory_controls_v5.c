#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bc_rtos.h"
#include "app_factory_ptt.h"
#include "app_factory_controls.c"

static unsigned checks, critical, starts, stops, silences, writes, replies;
#define CHECK(v) do { ++checks; if (!(v)) { fprintf(stderr,"FAIL %u: %s\n",__LINE__,#v); return 1; } } while(0)
uint32_t fixture_ticks;
TaskHandle_t fixture_task = 2;
static bool recording, flash_exists;
static uint32_t disk[4], enqueue_error;
static const uint32_t *queued;
static fds_cb_t callback;
static uint8_t last_reply[18], last_event[13], last_status[14];
static unsigned events, init_calls, registration_error, init_error;
void fixture_enter(void) { ++critical; }
void fixture_leave(void) { assert(critical); --critical; }
void fixture_notify(TaskHandle_t task) { (void)task; }
unsigned fixture_wait(unsigned clear, unsigned ticks) { (void)clear; (void)ticks; return 0; }
bool app_pdm_work_status(void) { return recording; }
bool app_pdm_recording_start(void) { assert(!critical); ++starts; recording = true; return true; }
bool app_pdm_recording_stop(void) { assert(!critical); ++stops; recording = false; return true; }
void bc_linear_motor_silence(void) { ++silences; }
void bc_linear_motor_service(void) {}
ret_code_t fds_register(fds_cb_t cb) { callback = cb; return registration_error; }
ret_code_t fds_init(void) { ++init_calls; if(init_error) return init_error; fds_evt_t e = {0}; callback(&e); return 0; }
ret_code_t fds_record_find(uint16_t f, uint16_t k, fds_record_desc_t *d, fds_find_token_t *t) {
    assert(f == 0x1001 && k == 1); (void)d;
    return flash_exists && !t->position++ ? 0 : FDS_ERR_NOT_FOUND;
}
ret_code_t fds_record_open(fds_record_desc_t *d, fds_flash_record_t *r) {
    static const fds_header_t header = {4}; (void)d;
    r->p_header = &header; r->p_data = disk; return 0;
}
ret_code_t fds_record_close(fds_record_desc_t *d) { (void)d; return 0; }
ret_code_t fds_record_write(fds_record_desc_t *d, const fds_record_t *r) {
    assert(!critical && r->file_id == 0x1001 && r->key == 1 && r->data.length_words == 4);
    (void)d; ++writes; queued = r->data.p_data; return enqueue_error;
}
ret_code_t fds_record_update(fds_record_desc_t *d, const fds_record_t *r) { return fds_record_write(d,r); }
void app_package_send_enqueue(struct app_cmd_package *p, uint8_t n) {
    assert(!critical);
    if (n == 13) { memcpy(last_event,p,n); ++events; return; }
    if (n == 14) { memcpy(last_status,p,n); return; }
    assert(n == 18); memcpy(last_reply,p,n); ++replies;
}
static void boot(void) {
    memset(current,0,sizeof(current)); memset(pending,0,sizeof(pending));
    memset(&factory_ptt_diagnostics,0,sizeof(factory_ptt_diagnostics));
    active_values = 0; ready = loaded = saving = storage_fault = have_record = false;
    init_event = write_event = 0; recording = false; enqueue_error = 0;
    app_factory_ptt_worker_init(); app_factory_controls_init(false); app_factory_controls_service();
}
static void sample(unsigned fingers, unsigned hold) {
    uint8_t d[8] = {0}; d[0] = hold ? 8 : 0; d[3] = fingers;
    app_factory_ptt_report(d,true);
}
static void complete(unsigned result) {
    fds_evt_t e = {0}; e.id = FDS_EVT_UPDATE; e.result = result;
    e.write.file_id = 0x1001; e.write.record_key = 1;
    if (!result) { memcpy(disk,queued,sizeof(disk)); flash_exists = true; }
    callback(&e); app_factory_controls_service();
}
static void set(uint32_t revision, uint32_t values) {
    uint8_t d[17] = {0,42,0x84,0x11,1,1,2,3,4};
    put32(d+9,revision); put32(d+13,values);
    assert(app_factory_controls_command(d,17));
}
int main(void) {
    uint8_t get[9] = {0,43,0x84,0x10,1,9,8,7,6};
    init_calls = 0;
    app_factory_controls_init(true); CHECK(init_calls == 0 && !init_event);
    fds_init(); CHECK(init_calls == 1 && init_event == 1);
    init_event = 0; init_error = 42;
    app_factory_controls_init(false); CHECK(init_calls == 2 && init_event == 2);
    init_event = 0; registration_error = 42;
    app_factory_controls_init(false); CHECK(init_calls == 2 && init_event == 2);
    registration_error = init_error = 0;
    boot(); CHECK(ready && app_factory_controls_haptics() && current[1] == 0 && writes == 0);
    CHECK(app_factory_controls_action(0)==1 && app_factory_controls_action(1)==0 && app_factory_controls_action(2)==0);
    app_factory_ptt_double_tap(); app_factory_ptt_triple_tap(); app_factory_ptt_worker_service(); CHECK(starts==0);
    sample(0,0); sample(1,1); app_factory_ptt_worker_service(); CHECK(starts==1 && recording);
    set(0,0x00020201); CHECK(last_reply[9]==BUSY && writes==0);
    CHECK(events == 1 && last_event[8] == 1 && last_event[9] == 1);
    sample(0,0); app_factory_ptt_worker_service(); CHECK(stops==1 && !recording);
    CHECK(events == 2 && last_event[8] == 0 && last_event[9] == 2);
    uint8_t status_request[9] = {0,99,0x84,0x12,1,4,3,2,1};
    CHECK(app_factory_controls_command(status_request,9));
    CHECK(!memcmp(last_status,status_request,9) && last_status[9] == 0 && last_status[10] == 2);
    for (unsigned n = 0; n < 9; ++n) { CHECK(!app_factory_ptt_status_command(status_request,n) || n >= 4); }
    unsigned event_count = events; app_factory_ptt_worker_service(); CHECK(events == event_count);
    set(0,0x00020201); CHECK(saving && writes==1 && current[1]==0 && app_factory_controls_haptics());
    app_factory_ptt_double_tap(); sample(1,1); app_factory_ptt_worker_service(); CHECK(starts==1);
    app_factory_controls_command(get,9); CHECK(last_reply[9]==BUSY);
    unsigned before = replies; complete(0); CHECK(replies==before+1 && last_reply[9]==OK && current[1]==1);
    CHECK(!app_factory_controls_haptics() && silences>0 && last_reply[17]==0);
    boot(); CHECK(!app_factory_controls_haptics() && current[1]==1 && app_factory_controls_action(1)==2);
    sample(0,0); app_factory_ptt_double_tap(); app_factory_ptt_worker_service(); CHECK(recording && factory_ptt_diagnostics.memo_owned);
    sample(1,1); sample(0,0); app_factory_ptt_worker_service(); CHECK(recording);
    app_factory_ptt_triple_tap(); app_factory_ptt_worker_service(); CHECK(!recording && !factory_ptt_diagnostics.memo_owned);
    set(0,DEFAULTS); CHECK(last_reply[9]==CONFLICT && writes==1);
    set(1,0x01010101); CHECK(last_reply[9]==INVALID && writes==1);
    enqueue_error=FDS_ERR_BUSY; set(1,DEFAULTS); CHECK(last_reply[9]==BUSY && !saving && !factory_ptt_diagnostics.settings_lock);
    enqueue_error=0; sample(0,0); set(1,DEFAULTS); CHECK(saving); complete(42); CHECK(last_reply[9]==STORAGE && !app_factory_controls_haptics() && current[1]==1);
    boot(); CHECK(current[1]==1 && !app_factory_controls_haptics());
    sample(0,0); set(1,DEFAULTS); complete(0); CHECK(app_factory_controls_haptics() && current[1]==2);
    boot(); CHECK(app_factory_controls_haptics() && current[1]==2);
    sample(0,0); set(2,0x00020201); CHECK(saving);
    /* Simulated loss of power before FDS completion retains the prior record. */
    boot(); CHECK(app_factory_controls_haptics() && current[1]==2);
    for(unsigned n=0;n<22;++n) {
        uint8_t data[22] = {0,1,0x84,0x11,1}; unsigned old=writes;
        app_factory_controls_command(data,n); CHECK(writes==old);
    }
    get[4]=2; app_factory_controls_command(get,9); CHECK(last_reply[9]==INVALID);
    disk[2]^=1; boot(); CHECK(storage_fault && !ready && !app_factory_controls_haptics());
    app_factory_controls_command(get,9); CHECK(last_reply[9]==INVALID);
    get[4]=1; app_factory_controls_command(get,9); CHECK(last_reply[9]==STORAGE);
    CHECK(!critical);
    printf("PASS P05 controls: %u checks; async persistence, reboot, corruption, mute, all gestures, busy, CAS, failures, malformed packets\n",checks);
    return 0;
}
