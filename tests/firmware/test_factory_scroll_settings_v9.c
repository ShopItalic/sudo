#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "app_factory_controls.c"

static unsigned checks, writes, responses, resets;
static bool exists, settings_busy;
static uint32_t disk[4], queued[4];
static uint8_t answer[20];
static unsigned answer_length;
static fds_cb_t callback;
#define CHECK(v) do { ++checks; if (!(v)) { fprintf(stderr,"FAIL %u: %s\n",__LINE__,#v); return 1; } } while (0)
void app_factory_short_init(void) {}
void app_factory_short_service(void) {}
bool app_factory_short_command(const uint8_t *d,unsigned n) { (void)d; (void)n; return false; }
bool factory_capture_active(void) { return false; }
bool app_factory_ptt_status_command(const uint8_t *d,unsigned n) { (void)d; (void)n; return false; }
bool app_factory_ptt_begin_settings(void) { if(settings_busy) return false; settings_busy=true; return true; }
void app_factory_ptt_end_settings(void) { settings_busy=false; }
void app_factory_scroll_reset(void) { ++resets; }
void bc_linear_motor_silence(void) {}
void bc_linear_motor_service(void) {}
ret_code_t fds_register(fds_cb_t cb) { callback=cb; return 0; }
ret_code_t fds_init(void) { fds_evt_t e={0}; callback(&e); return 0; }
ret_code_t fds_record_find(uint16_t f,uint16_t k,fds_record_desc_t *d,fds_find_token_t *t) {
    assert(f==0x1001 && k==1); (void)d;
    return exists && !t->position++ ? 0 : FDS_ERR_NOT_FOUND;
}
ret_code_t fds_record_open(fds_record_desc_t *d,fds_flash_record_t *r) {
    static const fds_header_t h={4}; (void)d; r->p_header=&h; r->p_data=disk; return 0;
}
ret_code_t fds_record_close(fds_record_desc_t *d) { (void)d; return 0; }
ret_code_t fds_record_write(fds_record_desc_t *d,const fds_record_t *r) {
    (void)d; assert(r->file_id==0x1001 && r->key==1 && r->data.length_words==4);
    memcpy(queued,r->data.p_data,sizeof(queued)); ++writes; return 0;
}
ret_code_t fds_record_update(fds_record_desc_t *d,const fds_record_t *r) { return fds_record_write(d,r); }
void app_package_send_enqueue(struct app_cmd_package *packet,uint8_t n) {
    assert(n>=18 && n<=20); memcpy(answer,packet,n); answer_length=n; ++responses;
}
static void boot(void) {
    memset(current,0,sizeof(current)); memset(pending,0,sizeof(pending));
    ready=loaded=saving=storage_fault=have_record=settings_busy=false;
    active_values=init_event=write_event=0;
    app_factory_controls_init(false); app_factory_controls_service();
}
static void seed(uint32_t magic,uint32_t values) {
    exists=true; disk[0]=magic; disk[1]=17; disk[2]=values; disk[3]=crc(disk); boot();
}
static void get(unsigned schema) {
    uint8_t p[9]={0,7,0x84,0x10,0,9,8,7,6}; p[4]=(uint8_t)schema;
    assert(app_factory_controls_command(p,sizeof(p)));
}
static void set(unsigned schema,unsigned mask,uint32_t revision) {
    uint8_t p[19]={0,8,0x84,0x11,0,6,7,8,9}; p[4]=(uint8_t)schema;
    put32(p+9,revision); put32(p+13,current[2]&LEGACY_VALUES_MASK&~SWIPE_MUTED_MASK);
    p[17]=(current[2]&LIGHT_MUTED)?0:1; p[18]=(uint8_t)((mask<<4)|(1U+(current[2]>>26)));
    assert(app_factory_controls_command(p,sizeof(p)));
}
static void complete(unsigned error) {
    fds_evt_t e={0}; e.id=FDS_EVT_WRITE; e.result=error; e.write.file_id=0x1001; e.write.record_key=1;
    if(!error) { memcpy(disk,queued,sizeof(disk)); exists=true; }
    callback(&e); app_factory_controls_service();
}
int main(void) {
    boot(); CHECK(ready && writes==0 && !exists);
    get(5); CHECK(answer_length==20 && answer[9]==0 && answer[19]==1);
    for(unsigned d=0;d<4;++d) CHECK(!app_factory_controls_swipe_enabled(d));
    set(4,15,0); CHECK(answer[9]==INVALID && writes==0); /* old app cannot turn on scrolling */
    for(unsigned mask=0;mask<16;++mask) {
        uint32_t revision=current[1]; unsigned previous=responses; unsigned old_mask=(~current[2]>>4)&15;
        set(5,mask,revision);
        if(mask!=old_mask) {
            CHECK(saving && responses==previous);
            CHECK(((~active_values>>4)&15)==old_mask); /* no optimistic activation */
            complete(0); CHECK(!saving && answer[9]==OK && current[1]==revision+1);
        }
        get(5); CHECK(answer[19]>>4==mask && answer[4]==5);
        for(unsigned d=0;d<4;++d) CHECK(app_factory_controls_swipe_enabled(d)==((mask&(1U<<d))!=0));
        unsigned before=writes; boot(); CHECK(writes==before && ready);
        get(5); CHECK(answer[19]>>4==mask);
    }
    unsigned before=writes; uint32_t revision=current[1];
    set(5,0,revision-1); CHECK(answer[9]==CONFLICT && writes==before);
    set(5,0,revision); CHECK(saving); complete(13); CHECK(storage_fault && answer[9]==STORAGE);
    CHECK(((~active_values>>4)&15)==15); /* failed persistence never claims new state */
    const uint32_t magics[]={LEGACY_MAGIC,LIGHT_MAGIC,HOLD_MAGIC,SWIPE_MAGIC};
    for(unsigned m=0;m<4;++m) {
        for(unsigned old_mask=0;old_mask<16;++old_mask) {
            uint32_t values=0x00020201U; /* both tap mappings on, haptics off */
            if(m>=1) values|=LIGHT_MUTED;
            if(m>=2) values|=6U<<26;
            if(m==3) values|=(~old_mask<<4)&SWIPE_MUTED_MASK;
            before=writes; seed(magics[m],values);
            CHECK(ready && !storage_fault && writes==before && disk[0]==magics[m]);
            CHECK(current[0]==MAGIC && current[1]==17 && (current[2]&SWIPE_MUTED_MASK)==SWIPE_MUTED_MASK);
            CHECK((current[2]&~SWIPE_MUTED_MASK)==(values&~SWIPE_MUTED_MASK));
            get(5); CHECK(answer[19]>>4==0 && answer[9]==OK);
            set(4,15,17); CHECK(answer[9]==INVALID && writes==before);
            set(5,1,17); complete(0); CHECK(disk[0]==MAGIC);
            boot(); CHECK(ready && app_factory_controls_swipe_enabled(0));
        }
    }
    seed(SWIPE_MAGIC,DEFAULTS); disk[3]^=1; boot(); CHECK(!ready && storage_fault);
    exists=false; boot(); before=responses;
    for(unsigned n=0;n<9;++n) { uint8_t p[9]={0,7,0x84,0x10,5,9,8,7,6}; app_factory_controls_command(p,n); }
    CHECK(responses==before);
    for(unsigned schema=1;schema<=5;++schema) { get(schema); CHECK(answer[9]==OK && answer_length==(schema==1?18:schema==2?19:20)); }
    CHECK(resets>0);
    printf("PASS P09 scroll settings: %u checks\n",checks); return 0;
}
