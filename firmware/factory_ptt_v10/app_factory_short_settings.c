#include "app_factory_short.h"
#include "app_factory_ptt.h"
#include "app_cmd_handler.h"
#include "app_package.h"
#include "fds.h"
#include <string.h>

#define SHORT_FILE 0x1001U
#define SHORT_KEY 0x0002U
#define SHORT_MAGIC 0x01524353UL /* SCR, schema 1 */
enum { OK, INVALID, BUSY, STORAGE, CONFLICT, NOT_READY };
static uint32_t current[4], pending[4];
static volatile unsigned init_event, write_event;
static bool loaded, ready, saving, storage_fault, have_record;
static uint8_t request[9];
static fds_record_desc_t descriptor;
static uint32_t get32(const uint8_t *p)
{ return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static void put32(uint8_t *p, uint32_t v)
{ unsigned i; for (i=0;i<4;++i) p[i]=(uint8_t)(v>>(8U*i)); }
static uint32_t crc(const uint32_t *w)
{
    uint32_t c=0xffffffffUL; unsigned i,b;
    for(i=0;i<12;++i) {
        c ^= (w[i/4]>>(8U*(i%4)))&255U;
        for(b=0;b<8;++b) c=(c>>1)^((c&1U)?0xedb88320UL:0U);
    }
    return ~c;
}
static bool read_record(uint32_t *out)
{
    fds_flash_record_t r; bool ok;
    if (fds_record_open(&descriptor,&r)!=NRF_SUCCESS) return false;
    ok=r.p_header->length_words==4U;
    if(ok) memcpy(out,r.p_data,16);
    if(fds_record_close(&descriptor)!=NRF_SUCCESS) ok=false;
    return ok && out[0]==SHORT_MAGIC && out[2]<=FACTORY_SHORT_MAX_STEPS && out[3]==crc(out);
}
static void event(const fds_evt_t *e)
{
    if(e->id==FDS_EVT_INIT) init_event=e->result==NRF_SUCCESS?1U:2U;
    if((e->id==FDS_EVT_WRITE || e->id==FDS_EVT_UPDATE) &&
       e->write.file_id==SHORT_FILE && e->write.record_key==SHORT_KEY)
        write_event=e->result==NRF_SUCCESS?1U:2U;
}
void app_factory_short_init(void)
{
    /* Register before the existing controls/Peer Manager FDS initialization. */
    if(fds_register(event)!=NRF_SUCCESS) init_event=2U;
}
unsigned app_factory_short_steps(void)
{ return ready && !storage_fault ? current[2] : 0U; }
static void reply(const uint8_t *r,unsigned status)
{
    uint8_t p[17]; unsigned ms=app_factory_short_steps()*500U;
    memcpy(p,r,9); p[9]=(uint8_t)status; put32(p+10,current[1]);
    p[14]=(uint8_t)app_factory_short_steps(); p[15]=(uint8_t)ms; p[16]=(uint8_t)(ms>>8);
    app_package_send_enqueue((struct app_cmd_package *)p,sizeof(p));
}
void app_factory_short_service(void)
{
    if(!loaded && init_event) {
        fds_find_token_t token={0}; fds_record_desc_t extra; uint32_t result;
        loaded=true;
        current[0]=SHORT_MAGIC; current[1]=0; current[2]=FACTORY_SHORT_DEFAULT_STEPS; current[3]=crc(current);
        if(init_event!=1U) { storage_fault=true; return; }
        result=fds_record_find(SHORT_FILE,SHORT_KEY,&descriptor,&token);
        if(result==FDS_ERR_NOT_FOUND) { ready=true; return; }
        if(result!=NRF_SUCCESS || !read_record(current) ||
           fds_record_find(SHORT_FILE,SHORT_KEY,&extra,&token)!=FDS_ERR_NOT_FOUND) {
            storage_fault=true; return;
        }
        have_record=true; ready=true;
    }
    if(saving && write_event) {
        uint32_t verified[4];
        bool ok=write_event==1U && read_record(verified) && !memcmp(verified,pending,sizeof(pending));
        if(ok) { memcpy(current,verified,sizeof(current)); have_record=true; }
        else storage_fault=true;
        saving=false; app_factory_ptt_end_settings(); reply(request,ok?OK:STORAGE);
    }
}
bool app_factory_short_command(const uint8_t *d,unsigned n)
{
    fds_record_t record; uint32_t result;
    if(!d || n<4U || d[2]!=0x84U || (d[3]!=0x13U && d[3]!=0x14U)) return false;
    if(n<9U) return true;
    if(d[0]!=0U || d[4]!=1U || n!=(d[3]==0x13U?9U:14U)) { reply(d,INVALID); return true; }
    if(storage_fault) { reply(d,STORAGE); return true; }
    if(!ready) { reply(d,NOT_READY); return true; }
    if(saving) { reply(d,BUSY); return true; }
    if(d[3]==0x13U) { reply(d,OK); return true; }
    if(d[13]>FACTORY_SHORT_MAX_STEPS) { reply(d,INVALID); return true; }
    if(get32(d+9)!=current[1]) { reply(d,CONFLICT); return true; }
    if(d[13]==current[2]) { reply(d,OK); return true; }
    if(current[1]==0xffffffffUL) { reply(d,STORAGE); return true; }
    if(!app_factory_ptt_begin_settings()) { reply(d,BUSY); return true; }
    pending[0]=SHORT_MAGIC; pending[1]=current[1]+1U; pending[2]=d[13]; pending[3]=crc(pending);
    memcpy(request,d,9); write_event=0; saving=true;
    record.file_id=SHORT_FILE; record.key=SHORT_KEY; record.data.p_data=pending; record.data.length_words=4;
    result=have_record?fds_record_update(&descriptor,&record):fds_record_write(&descriptor,&record);
    if(result!=NRF_SUCCESS) {
        saving=false; app_factory_ptt_end_settings();
        reply(d,result==FDS_ERR_BUSY || result==FDS_ERR_NO_SPACE_IN_QUEUES?BUSY:STORAGE);
    }
    return true;
}
