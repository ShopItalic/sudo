/* Included by the real-LittleFS fixture. Compile the generated production
 * adapter with task/transport shims so interleavings and lost ACKs are tested. */
typedef void *TaskHandle_t;
#include <setjmp.h>
static jmp_buf park_jump;
static bool expect_park;
static unsigned notifications;
static int task_a, task_b;
static TaskHandle_t current_task;
static unsigned critical_depth, flash_depth, reply_count;
static bool capture_active;
static uint8_t wire_reply[20];
enum { PPG_FLS_IDIE=0, PPG_FLS_WRITE=1, PPG_FLS_UPLOAD=2, PPG_FLS_BUSY=3 };
static struct { unsigned fls_status; bool ppg_file_status; } app_ppg_file_hardle;
struct app_cmd_package { uint8_t unused; };
#define taskENTER_CRITICAL() (++critical_depth)
#define taskEXIT_CRITICAL() do { CHECK(critical_depth==1); --critical_depth; } while(0)
#define portTICK_PERIOD_MS 1U
static TaskHandle_t xTaskGetCurrentTaskHandle(void) { return current_task; }
static uint32_t xTaskGetTickCount(void) { return ticks; }
static bool factory_capture_active(void) { return capture_active; }
static void bc_spi_flash_device_open(void) { CHECK(!flash_depth); ++flash_depth; }
static void bc_spi_flash_device_close(void) { CHECK(flash_depth==1); --flash_depth; }
static void bc_delay_ms(unsigned ms) { CHECK(ms==1000 && expect_park); longjmp(park_jump,1); }
static void xTaskNotifyGive(TaskHandle_t task) { CHECK(task); ++notifications; }
static int sd_rand_application_vector_get(uint8_t *out,unsigned n)
{ unsigned i; ++id[0]; for(i=0;i<n;++i) out[i]=id[i%16]; return 0; }
static void app_package_send_enqueue(struct app_cmd_package *p,unsigned n)
{ CHECK(n==20); memcpy(wire_reply,p,n); ++reply_count; }
#include "cleanup_adapter.inc"

static void adapter_reset(void)
{
    setup(); memset(&app_ppg_file_hardle,0,sizeof app_ppg_file_hardle);
    current_task=&task_a; critical_depth=flash_depth=reply_count=0; capture_active=false;
    p10_reservation_owner=NULL; p10_storage_fault=false; p10_mounted=p10_started=p10_prepared=p10_retry=false;
    p10_nonce=p10_last_service=0; p10_name_size=p10_name_count=p10_receipt_count=0;
    app_factory_cleanup_mount(&fs);
}
static void reservation_interleavings(void)
{
    adapter_reset(); CHECK(p10_claim_idle());
    CHECK(!p10_claim_idle()); /* Synchronous reentry cannot share the reservation. */
    app_ppg_file_hardle.fls_status=PPG_FLS_IDIE; /* Legacy inner function returns idle early. */
    current_task=&task_b; CHECK(!p10_claim_idle());
    p10_release_idle(); CHECK(p10_reservation_owner==&task_a);
    current_task=&task_a; p10_release_idle(); CHECK(!p10_reservation_owner);
    current_task=&task_b; CHECK(p10_claim_idle());
    current_task=&task_a; p10_release_idle();
    CHECK(p10_reservation_owner==&task_b && app_ppg_file_hardle.fls_status==PPG_FLS_BUSY);
    current_task=&task_b; app_ppg_file_hardle.fls_status=PPG_FLS_UPLOAD;
    p10_release_idle(); CHECK(!p10_reservation_owner);
    CHECK(app_ppg_file_hardle.fls_status==PPG_FLS_UPLOAD && !p10_claim_idle());
    app_ppg_file_hardle.fls_status=PPG_FLS_WRITE; CHECK(!p10_claim_idle());
    app_ppg_file_hardle.fls_status=PPG_FLS_IDIE; app_ppg_file_hardle.ppg_file_status=true;
    CHECK(!p10_claim_idle()); app_ppg_file_hardle.ppg_file_status=false;
    current_task=NULL; CHECK(!p10_claim_idle());
    current_task=&task_a; capture_active=true;
    CHECK(!p10_claim_cleanup()); /* Rollover can look idle while capture owns it. */
    CHECK(p10_claim_idle()); p10_release_idle(); /* Capture can open its next file. */
    capture_active=false; app_ppg_file_hardle.fls_status=PPG_FLS_WRITE;
    CHECK(p10_claim_write()); current_task=&task_b; CHECK(!p10_claim_write());
    p10_finish_write(true); CHECK(app_ppg_file_hardle.fls_status==PPG_FLS_WRITE);
    current_task=&task_a; p10_finish_write(false);
    CHECK(app_ppg_file_hardle.fls_status==PPG_FLS_BUSY && !p10_claim_cleanup());
    p10_storage_fault=false; /* Only a simulated cold boot clears this latch. */
    app_ppg_file_hardle.fls_status=PPG_FLS_WRITE;
    CHECK(p10_claim_write()); p10_finish_write(true);
    CHECK(app_ppg_file_hardle.fls_status==PPG_FLS_IDIE && p10_claim_cleanup());
    p10_release_idle();
    CHECK(p10_wake_worker(&task_b)); CHECK(notifications==1);
    CHECK(!p10_wake_worker(NULL));CHECK(p10_storage_fault && !p10_claim_idle());
    p10_storage_fault=false;app_ppg_file_hardle.fls_status=PPG_FLS_IDIE;
    CHECK(p10_claim_idle());current_task=&task_b;flash_depth=1;expect_park=true;
    if(!setjmp(park_jump))p10_park_storage_worker();
    expect_park=false;current_task=&task_a;p10_release_idle();
    CHECK(p10_storage_fault && app_ppg_file_hardle.fls_status==PPG_FLS_BUSY);
    CHECK(!critical_depth && !flash_depth);
}
static factory_cleanup_result request(unsigned op,uint32_t nonce,const uint8_t *data,unsigned n)
{
    uint8_t *packet=malloc(9U+n); unsigned before=reply_count;
    CHECK(packet && n<=11); memset(packet,0,9U+n);
    packet[1]=37; packet[2]=0x84; packet[3]=(uint8_t)op; packet[4]=1;
    put32(packet+5,nonce); if(n)memcpy(packet+9,data,n);
    CHECK(app_factory_cleanup_command(packet,9U+n));
    CHECK(reply_count==before+1); CHECK(!memcmp(wire_reply,packet,9)); free(packet);
    CHECK(!critical_depth && !flash_depth);
    return (factory_cleanup_result)wire_reply[9];
}
static factory_cleanup_result page_request(unsigned op,unsigned page,const uint8_t *data,unsigned n)
{
    uint8_t page_data[10]; CHECK(n<=9); page_data[0]=(uint8_t)page;
    memcpy(page_data+1,data,n); return request(op,123,page_data,1U+n);
}
static void wire_lifecycle(void)
{
    uint8_t begin[2]={13,0}, descriptor_bytes[52], page, malformed[10];
    const uint8_t name[]="recording.bin"; unsigned i,n,before;
    adapter_reset();
    CHECK(request(0x20,123,NULL,0)==FC_READY); CHECK(!memcmp(wire_reply+10,"P10",3));
    CHECK(request(0x21,0,begin,2)==FC_INVALID);
    CHECK(request(0x21,123,begin,2)==FC_READY);
    CHECK(page_request(0x22,1,name+9,4)==FC_INVALID); /* Out of order. */
    CHECK(page_request(0x22,0,name,9)==FC_READY);
    CHECK(request(0x21,123,begin,2)==FC_READY); /* Lost begin ACK preserves bytes. */
    CHECK(page_request(0x22,0,name,9)==FC_READY);
    memcpy(malformed,name,9); malformed[0]^=1;
    CHECK(page_request(0x22,0,malformed,9)==FC_INVALID);
    app_ppg_file_hardle.fls_status=PPG_FLS_WRITE;
    CHECK(page_request(0x22,1,name+9,4)==FC_BUSY);
    app_ppg_file_hardle.fls_status=PPG_FLS_IDIE;
    CHECK(page_request(0x22,1,name+9,4)==FC_WORKING);
    for(i=0;i<30;++i){ticks+=50;app_factory_cleanup_service();}
    CHECK(p10_cleanup.result==FC_READY && exists("/recording.bin"));
    for(page=0;page<6;++page){
        CHECK(request(0x23,123,&page,1)==FC_READY); CHECK(wire_reply[10]==page);
        n=52U-page*9U; if(n>9)n=9; memcpy(descriptor_bytes+page*9U,wire_reply+11,n);
    }
    before=removed;
    for(page=0;page<6;++page){
        n=52U-page*9U; if(n>9)n=9;
        CHECK(page_request(0x24,page,descriptor_bytes+page*9U,n)==(page==5?FC_PENDING:FC_READY));
    }
    CHECK(removed==before); /* Receipt ACK cannot masquerade as deletion. */
    CHECK(page_request(0x24,5,descriptor_bytes+45,7)==FC_PENDING); /* Lost final ACK. */
    CHECK(request(0x23,124,&page,1)==FC_STALE);
    CHECK(exists("/recording.bin"));
    /* Recording owns flash: service must leave even an acknowledged file alone. */
    app_ppg_file_hardle.fls_status=PPG_FLS_WRITE;
    for(i=0;i<100;++i){ticks+=50;app_factory_cleanup_service();}
    CHECK(removed==before && exists("/recording.bin"));
    app_ppg_file_hardle.fls_status=PPG_FLS_IDIE;
    for(i=0;i<100;++i){ticks+=50;app_factory_cleanup_service();}
    CHECK(!exists("/recording.bin")); survivor();
    CHECK(page_request(0x24,5,descriptor_bytes+45,7)==FC_PENDING);
    CHECK(!critical_depth && !flash_depth);
}
static void wire_bounds(void)
{
    unsigned n,op,page; uint8_t *p;
    adapter_reset();
    CHECK(!app_factory_cleanup_command(NULL,0));
    for(op=0x20;op<=0x24;++op)for(n=0;n<=22;++n)for(page=0;page<=255;++page){
        p=calloc(n?n:1,1); CHECK(p);
        if(n>2)p[2]=0x84; if(n>3)p[3]=(uint8_t)op; if(n>4)p[4]=1;
        if(n>5)p[5]=123; if(n>9)p[9]=(uint8_t)page;
        (void)app_factory_cleanup_command(p,n); free(p);
    }
    CHECK(exists("/recording.bin")); survivor(); CHECK(!critical_depth && !flash_depth);
}
