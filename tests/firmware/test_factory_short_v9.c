#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bc_rtos.h"
#include "app_factory_short.c"
#include "app_factory_short_settings.c"
static unsigned checks, critical, writes, replies, init_calls;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL %u: %s\n",__LINE__,#x); return 1; } } while(0)
uint32_t fixture_ticks;
TaskHandle_t fixture_task=2;
void fixture_enter(void) { ++critical; }
void fixture_leave(void) { assert(critical); --critical; }
static bool settings_lock, recording, disk_exists, duplicate, bad_open, bad_close;
static unsigned queue_error, register_error, words=4;
static uint32_t disk[4];
static const uint32_t *queued;
static fds_cb_t callback;
static uint8_t response[20];
bool app_factory_ptt_begin_settings(void) {
    if(recording || settings_lock || factory_capture_active()) return false;
    settings_lock=true; return true;
}
void app_factory_ptt_end_settings(void) { settings_lock=false; }
ret_code_t fds_register(fds_cb_t cb) { callback=cb; return register_error; }
ret_code_t fds_init(void) { ++init_calls; return 0; }
ret_code_t fds_record_find(uint16_t f,uint16_t k,fds_record_desc_t *d,fds_find_token_t *t) {
    assert(f==0x1001 && k==2); (void)d;
    return disk_exists && (t->position++==0 || duplicate) ? 0 : FDS_ERR_NOT_FOUND;
}
ret_code_t fds_record_open(fds_record_desc_t *d,fds_flash_record_t *r) {
    static fds_header_t h; (void)d; h.length_words=words;
    r->p_header=&h;r->p_data=disk;return bad_open?42:0;
}
ret_code_t fds_record_close(fds_record_desc_t *d) { (void)d;return bad_close?42:0; }
ret_code_t fds_record_write(fds_record_desc_t *d,const fds_record_t *r) {
    assert(!critical && settings_lock && r->file_id==0x1001 && r->key==2 && r->data.length_words==4);
    (void)d;queued=r->data.p_data;++writes;return queue_error;
}
ret_code_t fds_record_update(fds_record_desc_t *d,const fds_record_t *r) { return fds_record_write(d,r); }
void app_package_send_enqueue(struct app_cmd_package *p,uint8_t n) {
    assert(!critical && n==17);memcpy(response,p,n);++replies;
}
static void boot(unsigned init_status) {
    fds_evt_t e={0}; e.id=FDS_EVT_INIT;e.result=init_status;
    ready=loaded=saving=storage_fault=have_record=false;init_event=write_event=0;settings_lock=false;
    memset(current,0,sizeof(current));app_factory_short_init();callback(&e);app_factory_short_service();
}
static void set_steps(uint32_t rev,unsigned steps) {
    uint8_t d[14]={0,42,0x84,0x14,1,1,2,3,4};put32(d+9,rev);d[13]=(uint8_t)steps;
    assert(app_factory_short_command(d,sizeof d));
}
static void complete(unsigned status,bool commit) {
    fds_evt_t e={0};e.id=FDS_EVT_UPDATE;e.result=status;e.write.file_id=0x1001;e.write.record_key=2;
    if(commit) { memcpy(disk,queued,sizeof disk);disk_exists=true; }
    callback(&e);app_factory_short_service();
}
static void audio(unsigned bytes) {
    uint8_t header[6]={0};factory_capture_ready();assert(factory_capture_producer_enter());
    assert(factory_capture_queue_begin(header));factory_capture_producer_leave();
    assert(factory_capture_sender_enter(header));factory_capture_written(bytes,true);factory_capture_sender_leave();
}
int main(void) {
    unsigned step,kind,i;uint8_t header[6]={0},stale[6];
    boot(0);CHECK(ready && app_factory_short_steps()==4 && writes==0 && init_calls==0);
    for(step=0;step<=10;++step) {
        unsigned before=writes;set_steps(current[1],step);
        if(current[2]!=step) { CHECK(saving && writes==before+1 && settings_lock);complete(0,true); }
        CHECK(response[9]==OK && response[14]==step && (unsigned)(response[15]|response[16]<<8)==step*500);
        boot(0);CHECK(ready && app_factory_short_steps()==step);
        for(kind=0;kind<2;++kind) for(i=0;i<3;++i) {
            unsigned bytes=step?step*2000+i-1:i;
            CHECK(factory_capture_begin(step));factory_capture_enable();audio(bytes);
            factory_capture_stop_accepting();CHECK(factory_capture_quiet());
            CHECK(factory_capture_should_discard()==(step!=0 && i==0));
            factory_capture_end();CHECK(!factory_capture_active());
        }
    }
    i=writes;set_steps(current[1],11);CHECK(response[9]==INVALID && writes==i);
    set_steps(current[1]-1,0);CHECK(response[9]==CONFLICT && writes==i);
    recording=true;set_steps(current[1],0);CHECK(response[9]==BUSY && writes==i);recording=false;
    queue_error=FDS_ERR_BUSY;set_steps(current[1],0);CHECK(response[9]==BUSY && !settings_lock && !saving);queue_error=0;
    set_steps(current[1],0);CHECK(saving);complete(0,false);CHECK(storage_fault && app_factory_short_steps()==0 && response[9]==STORAGE);
    boot(0);CHECK(ready);set_steps(current[1],0);complete(42,false);CHECK(storage_fault && response[9]==STORAGE);
    boot(0);disk[3]^=1;boot(0);CHECK(storage_fault && app_factory_short_steps()==0);
    disk[3]^=1;duplicate=true;boot(0);CHECK(storage_fault);duplicate=false;
    words=3;boot(0);CHECK(storage_fault);words=4;
    bad_open=true;boot(0);CHECK(storage_fault);bad_open=false;
    bad_close=true;boot(0);CHECK(storage_fault);bad_close=false;
    boot(42);CHECK(storage_fault && app_factory_short_steps()==0);
    disk_exists=false;boot(0);CHECK(app_factory_short_steps()==4);
    for(i=0;i<256;++i) {
        uint8_t d[15]={0,42,0x84,0x14,1,1,2,3,4};unsigned old=writes;
        put32(d+9,current[1]);d[13]=(uint8_t)i;
        CHECK(app_factory_short_command(d,15) && writes==old && response[9]==INVALID);
    }
    for(i=0;i<9;++i) {
        uint8_t *d=malloc(i?i:1);unsigned old=replies;memset(d,0,i);
        if(i>=4) { d[2]=0x84;d[3]=0x13; }
        app_factory_short_command(d,i);CHECK(replies==old);free(d);
    }
    CHECK(!app_factory_short_command(NULL,14));
    CHECK(factory_capture_begin(4));factory_capture_enable();factory_capture_ready();
    CHECK(factory_capture_producer_enter());CHECK(factory_capture_queue_begin(header));memcpy(stale,header,6);
    factory_capture_stop_accepting();CHECK(!factory_capture_quiet());CHECK(!factory_capture_begin(4));
    factory_capture_producer_leave();CHECK(!factory_capture_quiet());CHECK(factory_capture_sender_enter(header));
    factory_capture_written(220,true);factory_capture_sender_leave();CHECK(factory_capture_quiet());
    CHECK(factory_capture_should_discard());factory_capture_end();
    CHECK(factory_capture_begin(4));factory_capture_enable();CHECK(!factory_capture_sender_enter(stale));
    audio(100);factory_capture_fault();factory_capture_stop_accepting();CHECK(!factory_capture_should_discard());factory_capture_end();
    CHECK(factory_capture_begin(10));factory_capture_enable();audio(0xffffffffU);audio(500);
    factory_capture_stop_accepting();CHECK(!factory_capture_should_discard() && capture.samples==40000);factory_capture_end();
    CHECK(factory_capture_begin(4));factory_capture_enable();factory_capture_ready();
    CHECK(factory_capture_producer_enter());CHECK(factory_capture_queue_begin(header));factory_capture_queue_failed();
    factory_capture_producer_leave();factory_capture_stop_accepting();CHECK(factory_capture_quiet() && !factory_capture_should_discard());factory_capture_end();
    CHECK(factory_capture_begin(4));factory_capture_enable();factory_capture_ready();factory_capture_stop_accepting();
    CHECK(!factory_capture_producer_enter() && !factory_capture_should_discard());factory_capture_end();
    CHECK(!critical && init_calls==0);
    printf("PASS P09 policy/settings: %u checks; all durations, both kinds, persistence, faults, queue ownership, stale packets\n",checks);
    return 0;
}
