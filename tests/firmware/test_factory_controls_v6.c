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
static uint8_t last_reply[20], last_event[13], last_status[14];
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
    assert(n >= 18 && n <= 20); memcpy(last_reply,p,n); ++replies;
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
#include "factory_recording_led.inc"
static void set_light(uint32_t revision, uint32_t values, uint8_t light) {
    uint8_t d[18] = {0,44,0x84,0x11,2,1,2,3,4};
    put32(d+9,revision); put32(d+13,values); d[17]=light;
    assert(app_factory_controls_command(d,18));
}
static void set_delay(uint32_t revision, uint32_t values, uint8_t light, uint8_t steps) {
    uint8_t d[19] = {0,45,0x84,0x11,3,1,2,3,4};
    put32(d+9,revision); put32(d+13,values); d[17]=light; d[18]=steps;
    assert(app_factory_controls_command(d,19));
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
    get[4]=4; app_factory_controls_command(get,9); CHECK(last_reply[9]==INVALID);
    disk[2]^=1; boot(); CHECK(storage_fault && !ready && !app_factory_controls_haptics());
    app_factory_controls_command(get,9); CHECK(last_reply[9]==INVALID);
    get[4]=1; app_factory_controls_command(get,9); CHECK(last_reply[9]==STORAGE);
    CHECK(!app_factory_controls_recording_light());
    /* A real P05 disk record upgrades in memory, preserving all four values,
     * without a flash write on boot. */
    disk[0]=LEGACY_MAGIC; disk[1]=9; disk[2]=0x00020201; disk[3]=crc(disk);
    unsigned old_writes=writes;
    boot(); sample(0,0);
    CHECK(ready && current[1]==9 && writes==old_writes);
    CHECK(app_factory_controls_recording_light() && !app_factory_controls_haptics());
    CHECK(app_factory_controls_action(1)==2 && app_factory_controls_action(2)==2);
    get[4]=2; app_factory_controls_command(get,9);
    CHECK(last_reply[4]==2 && last_reply[17]==0 && last_reply[18]==1);
    set_light(9,0x00020201,0);
    CHECK(saving && app_factory_controls_recording_light());
    /* A pending write does not become active until its FDS completion. */
    complete(0); CHECK(!app_factory_controls_recording_light() && current[1]==10);
    CHECK(last_reply[17]==0 && last_reply[18]==0 && current[0]==MAGIC);
    boot(); sample(0,0); CHECK(!app_factory_controls_recording_light());
    /* Every real recording-color branch suppresses RGB; status colors stay. */
    for(unsigned path=0;path<5;++path) {
        for(unsigned color=1;color<=3;++color) {
            recording_led_fixture(path,color);
            CHECK(rgb_g==0 && rgb_r==0 && rgb_b==0);
        }
    }
    /* Old-app saves must neither unmute the light nor leak packed flags. */
    set(10,0x01020201); complete(0);
    CHECK(app_factory_controls_haptics() && !app_factory_controls_recording_light());
    CHECK(last_reply[4]==1 && last_reply[17]==1 && current[1]==11);
    boot(); sample(0,0); CHECK(!app_factory_controls_recording_light() && app_factory_controls_haptics());
    set_light(10,0x01020201,1); CHECK(last_reply[9]==CONFLICT);
    set_light(11,0x01020201,2); CHECK(last_reply[9]==INVALID);
    set_light(11,0x03020201,1); CHECK(last_reply[9]==INVALID);
    set_light(11,0x01020201,1); CHECK(saving);
    complete(42); CHECK(last_reply[9]==STORAGE && !app_factory_controls_recording_light());
    boot(); sample(0,0); CHECK(!app_factory_controls_recording_light());
    set_light(11,0x01020201,1); boot(); sample(0,0);
    CHECK(!app_factory_controls_recording_light() && current[1]==11);
    set_light(11,0x01020201,1); complete(0);
    CHECK(app_factory_controls_recording_light() && app_factory_controls_haptics());
    for(unsigned path=0;path<5;++path) {
        recording_led_fixture(path,1); CHECK(rgb_g==0 && rgb_r==0 && rgb_b==20);
        recording_led_fixture(path,2); CHECK(rgb_g==0 && rgb_r==20 && rgb_b==0);
        recording_led_fixture(path,3); CHECK(rgb_g==20 && rgb_r==0 && rgb_b==0);
    }
    boot(); sample(0,0); CHECK(app_factory_controls_recording_light() && current[1]==12);
    sample(1,1); app_factory_ptt_worker_service(); CHECK(recording);
    set_light(12,0x01020201,0); CHECK(last_reply[9]==BUSY && app_factory_controls_recording_light());
    sample(0,0); app_factory_ptt_worker_service(); CHECK(!recording);
    for(unsigned n=0;n<23;++n) {
        uint8_t data[23]={0,1,0x84,0x11,2}; unsigned prior=writes;
        app_factory_controls_command(data,n); CHECK(writes==prior);
    }
    /* Every half-second value persists and keeps light/haptics independent. */
    CHECK(app_factory_controls_hold_delay_ms()==500);
    for(unsigned steps=1;steps<=10;++steps) {
        sample(0,0);
        set_delay(current[1],0x01020201,0,(uint8_t)steps);
        if(saving) complete(0);
        CHECK(last_reply[9]==OK && last_reply[4]==3 && last_reply[19]==steps);
        CHECK(app_factory_controls_hold_delay_ms()==steps*500U);
        CHECK(app_factory_controls_haptics() && !app_factory_controls_recording_light());
        boot(); sample(0,0); CHECK(app_factory_controls_hold_delay_ms()==steps*500U);
    }
    /* Schema 1 and 2 writes preserve the delay and return their old sizes. */
    set(current[1],0x00020201); complete(0);
    CHECK(app_factory_controls_hold_delay_ms()==5000 && !app_factory_controls_haptics());
    set_light(current[1],0x00020201,1); complete(0);
    CHECK(app_factory_controls_hold_delay_ms()==5000 && app_factory_controls_recording_light());
    boot(); sample(0,0);
    CHECK(app_factory_controls_hold_delay_ms()==5000);
    set_delay(current[1],0x00020201,1,0); CHECK(last_reply[9]==INVALID);
    set_delay(current[1],0x00020201,1,11); CHECK(last_reply[9]==INVALID);
    set_delay(current[1],0x00020201,1,255); CHECK(last_reply[9]==INVALID);
    set_delay(current[1]-1U,0x00020201,1,2); CHECK(last_reply[9]==CONFLICT);
    set_delay(current[1],0x00020201,1,2); CHECK(saving);
    boot(); sample(0,0); CHECK(app_factory_controls_hold_delay_ms()==5000);
    set_delay(current[1],0x00020201,1,2); complete(42);
    CHECK(last_reply[9]==STORAGE && app_factory_controls_hold_delay_ms()==5000);
    boot(); sample(0,0);
    /* The unpublished light-only record keeps mute and receives 500 ms. */
    disk[0]=LIGHT_MAGIC; disk[1]=7; disk[2]=0x03020201; disk[3]=crc(disk);
    old_writes=writes; boot(); sample(0,0);
    CHECK(app_factory_controls_hold_delay_ms()==500 && writes==old_writes);
    CHECK(!app_factory_controls_recording_light() && app_factory_controls_haptics());
    set_delay(7,0x01020201,0,10); complete(0); boot(); sample(0,0);
    CHECK(app_factory_controls_hold_delay_ms()==5000);
    /* A held finger waiting to activate is still busy for settings. */
    sample(1,1); app_factory_ptt_worker_service(); CHECK(!recording);
    set_delay(current[1],0x01020201,0,1); CHECK(last_reply[9]==BUSY);
    sample(0,0); app_factory_ptt_worker_service();
    for(unsigned n=0;n<24;++n) {
        uint8_t data[24]={0,1,0x84,0x11,3}; unsigned prior=writes;
        app_factory_controls_command(data,n); CHECK(writes==prior);
    }
    /* Separate Ring flash images get their own initial default. */
    uint32_t ring_a[4]; memcpy(ring_a,disk,sizeof(ring_a));
    flash_exists=false; old_writes=writes; boot();
    CHECK(app_factory_controls_hold_delay_ms()==500 && writes==old_writes);
    CHECK(app_factory_controls_haptics() && app_factory_controls_recording_light());
    memcpy(disk,ring_a,sizeof(disk)); flash_exists=true; boot();
    CHECK(app_factory_controls_hold_delay_ms()==5000);
    /* The former 5.5..10 second prototype values normalize to the new maximum
     * without writing at boot, losing mute/mappings, or accepting corruption. */
    for(unsigned step=11;step<=20;++step) {
        disk[0]=MAGIC; disk[1]=17; disk[2]=0x02020201 | ((step-1U)<<26); disk[3]=crc(disk);
        old_writes=writes; boot();
        CHECK(ready && !storage_fault && writes==old_writes && current[1]==17);
        CHECK(app_factory_controls_hold_delay_ms()==5000);
        CHECK(!app_factory_controls_haptics() && !app_factory_controls_recording_light());
        CHECK(app_factory_controls_action(1)==2 && app_factory_controls_action(2)==2);
        CHECK((disk[2] >> 26) == step-1U); /* no boot-time flash mutation */
        get[4]=3; app_factory_controls_command(get,9); CHECK(last_reply[19]==10);
        boot(); CHECK(app_factory_controls_hold_delay_ms()==5000);
    }
    sample(0,0); set_delay(current[1],0x00020201,1,10); complete(0);
    CHECK((disk[2]>>26)==9 && app_factory_controls_recording_light());
    disk[2]|=0xfc000000UL; disk[3]=crc(disk); boot();
    CHECK(storage_fault && !ready && app_factory_controls_action(0)==0);
    CHECK(!critical);
    printf("PASS P06 controls: %u checks; async persistence, reboot, corruption, mute, all gestures, busy, CAS, failures, malformed packets\n",checks);
    return 0;
}
