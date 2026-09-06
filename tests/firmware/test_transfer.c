#include "bc_ble_tx.h"
#include "bc_file_transfer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)
typedef struct {
    uint32_t tick, session, waits, attempts, retry_count, last_wait;
    uint32_t cancel_wait, wake_advance, attempt_cost;
    bool fatal;
    const uint8_t *expected;
    uint16_t length;
} radio;
static uint32_t now(void *p) { return ((radio *)p)->tick; }
static bool current(void *p,uint32_t session) { return ((radio *)p)->session==session; }
static void wait_radio(void *p,uint32_t ticks) {
    radio *r=p;
    CHECK(ticks>0); r->last_wait=ticks; ++r->waits;
    r->tick += r->wake_advance ? r->wake_advance : ticks;
    if (r->cancel_wait==r->waits) ++r->session;
}
static bc_ble_tx_attempt_result attempt(void *p,const uint8_t *data,uint16_t length) {
    radio *r=p; ++r->attempts; r->tick+=r->attempt_cost;
    CHECK(length==r->length); CHECK(memcmp(data,r->expected,length)==0);
    if(r->fatal) return BC_BLE_TX_ATTEMPT_FATAL;
    return r->attempts<=r->retry_count ? BC_BLE_TX_ATTEMPT_RETRY : BC_BLE_TX_ATTEMPT_ACCEPTED;
}
static void test_radio(void) {
    uint8_t packet[244]; unsigned i;
    for(i=0;i<sizeof(packet);++i)packet[i]=(uint8_t)i;
    radio r={.session=3,.retry_count=3,.expected=packet,.length=244};
    bc_ble_tx_port port={&r,attempt,now,wait_radio,current};
    CHECK(bc_ble_tx_write(&port,packet,244,3,100,10)==BC_BLE_TX_ACCEPTED);
    CHECK(r.attempts==4 && r.waits==3 && r.tick==30);
    /* The handle could be reused; a different connection epoch still cancels. */
    r=(radio){.session=7,.retry_count=100,.cancel_wait=1,.expected=packet,.length=244};
    CHECK(bc_ble_tx_write(&port,packet,244,7,100,10)==BC_BLE_TX_CANCELLED);
    CHECK(r.attempts==1);
    CHECK(bc_ble_tx_write(&port,packet,244,7,100,10)==BC_BLE_TX_CANCELLED);
    CHECK(r.attempts==1);
    r=(radio){.session=1,.retry_count=1000,.wake_advance=1,.expected=packet,.length=244};
    CHECK(bc_ble_tx_write(&port,packet,244,1,17,10)==BC_BLE_TX_TIMEOUT);
    CHECK(r.tick==17 && r.attempts==17 && r.last_wait==1);
    r=(radio){.session=1,.tick=UINT32_MAX-4,.retry_count=1000,.expected=packet,.length=244};
    CHECK(bc_ble_tx_write(&port,packet,244,1,12,5)==BC_BLE_TX_TIMEOUT);
    CHECK(r.attempts==3 && r.waits==3 && r.last_wait==2 && r.tick==7);
    r=(radio){.session=1,.fatal=true,.expected=packet,.length=244};
    CHECK(bc_ble_tx_write(&port,packet,244,1,12,5)==BC_BLE_TX_FATAL);
    CHECK(r.attempts==1 && r.waits==0);
    r=(radio){.session=1,.retry_count=3,.attempt_cost=12,.expected=packet,.length=244};
    CHECK(bc_ble_tx_write(&port,packet,244,1,12,5)==BC_BLE_TX_TIMEOUT);
    CHECK(r.attempts==1 && r.waits==0);
    CHECK(bc_ble_tx_write(NULL,packet,244,1,12,5)==BC_BLE_TX_INVALID);
    CHECK(bc_ble_tx_write(&port,NULL,244,1,12,5)==BC_BLE_TX_INVALID);
    CHECK(bc_ble_tx_write(&port,packet,245,1,12,5)==BC_BLE_TX_INVALID);
    CHECK(bc_ble_tx_write(&port,packet,0,1,12,5)==BC_BLE_TX_INVALID);
    CHECK(bc_ble_tx_write(&port,packet,244,1,0,5)==BC_BLE_TX_INVALID);
    CHECK(bc_ble_tx_write(&port,packet,244,1,12,0)==BC_BLE_TX_INVALID);
    CHECK(bc_ble_tx_write(&port,packet,244,1,UINT32_MAX,5)==BC_BLE_TX_INVALID);
    port.wait=NULL;
    CHECK(bc_ble_tx_write(&port,packet,244,1,12,5)==BC_BLE_TX_INVALID);
    port.wait=wait_radio;
    /* Consecutive FIFO writes cannot mutate a preceding packet. */
    for(i=0;i<30;++i) {
        packet[0]=(uint8_t)i;
        r=(radio){.session=1,.retry_count=i%4,.expected=packet,.length=244};
        CHECK(bc_ble_tx_write(&port,packet,244,1,100,10)==BC_BLE_TX_ACCEPTED);
        CHECK(packet[0]==i);
    }
}

typedef struct {
    uint8_t bytes[1024], packets[16][240];
    uint16_t lengths[16];
    uint32_t pos, seeks, reads, sends, max_read, fail_read, fail_send, cancel_read;
    bool cancelled, bad_seek;
} file;
static int32_t seek_file(void *p,uint32_t offset) {
    file *f=p; ++f->seeks; f->pos=offset; return f->bad_seek ? -1 : (int32_t)offset;
}
static int32_t read_file(void *p,uint8_t *data,uint32_t length) {
    file *f=p; ++f->reads;
    if(f->fail_read==f->reads) return 0; /* Truncation after a successful earlier read. */
    if(f->max_read && length>f->max_read)length=f->max_read;
    CHECK(f->pos+length<=sizeof(f->bytes));
    memcpy(data,f->bytes+f->pos,length); f->pos+=length;
    if(f->cancel_read==f->reads)f->cancelled=true;
    return (int32_t)length;
}
static bool send_file(void *p,const uint8_t *data,uint16_t length) {
    file *f=p; ++f->sends;
    CHECK(length<=240 && f->sends<=16);
    memcpy(f->packets[f->sends-1],data,length);f->lengths[f->sends-1]=length;
    return f->fail_send!=f->sends;
}
static bool file_current(void *p) { return !((file *)p)->cancelled; }
static uint32_t le32(const uint8_t *p) {
    return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void reset_file(file *f) {
    unsigned i; memset(f,0,sizeof(*f));
    for(i=0;i<sizeof(f->bytes);++i)f->bytes[i]=(uint8_t)(i*13+7);
}
static void test_files(void) {
    file f; unsigned i;
    bc_file_port port={&f,seek_file,read_file,send_file,file_current};
    reset_file(&f); f.max_read=7;
    CHECK(bc_file_transfer(&port,665,220,220)==BC_FILE_DONE);
    CHECK(f.seeks==1 && f.pos==665 && f.sends==3);
    for(i=0;i<3;++i) {
        unsigned n=i==2 ? 5 : 220;
        CHECK(f.packets[i][0]==1);
        CHECK(le32(f.packets[i]+1)==445 && le32(f.packets[i]+5)==3);
        CHECK(le32(f.packets[i]+9)==i+1 && le32(f.packets[i]+13)==n);
        CHECK(f.lengths[i]==17+n);
        CHECK(memcmp(f.packets[i]+17,f.bytes+220+220*i,n)==0);
    }
    reset_file(&f);
    CHECK(bc_file_transfer(&port,440,0,220)==BC_FILE_DONE);
    CHECK(f.sends==2 && f.lengths[1]==237);
    reset_file(&f); f.fail_read=2;
    CHECK(bc_file_transfer(&port,665,0,220)==BC_FILE_READ_ERROR);
    CHECK(f.sends==1 && f.reads==2); /* No stale buffer or fake final packet. */
    reset_file(&f); f.fail_send=1;
    CHECK(bc_file_transfer(&port,665,0,220)==BC_FILE_SEND_ERROR);
    CHECK(f.reads==1 && f.sends==1 && f.pos==220);
    reset_file(&f); f.cancel_read=1;
    CHECK(bc_file_transfer(&port,665,0,220)==BC_FILE_CANCELLED);
    CHECK(f.sends==0);
    reset_file(&f); f.bad_seek=true;
    CHECK(bc_file_transfer(&port,665,0,220)==BC_FILE_READ_ERROR);
    CHECK(f.reads==0);
    reset_file(&f);
    CHECK(bc_file_transfer(&port,665,666,220)==BC_FILE_INVALID);
    CHECK(bc_file_transfer(&port,665,0,0)==BC_FILE_INVALID);
    CHECK(bc_file_transfer(&port,665,0,224)==BC_FILE_INVALID);
    CHECK(bc_file_transfer(&port,UINT32_MAX,0,220)==BC_FILE_INVALID);
    CHECK(f.seeks==0 && f.sends==0);
    CHECK(bc_file_transfer(&port,665,665,220)==BC_FILE_DONE);
    CHECK(f.pos==665 && f.reads==0 && f.sends==0);
    reset_file(&f); f.cancelled=true;
    CHECK(bc_file_transfer(&port,665,0,220)==BC_FILE_CANCELLED);
    CHECK(f.seeks==0);
}
int main(void) {
    test_radio(); test_files();
    printf("PASS: %u checks (BLE retry/deadline/session and file resume/read/send integrity)\n", checks);
    return 0;
}
