#include "app_factory_short.h"
#include "app_factory_delete.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL %u: %s\n",__LINE__,#c); exit(1); } } while(0)
#define BC_LOG_INFO(...)
#define BC_LOG_WARN(...)
#define FLASH_MIN_USE_SPACE 65536U
enum { PPG_FLS_IDIE, PPG_FLS_UPLOAD, PPG_FLS_WRITE, PPG_FLS_BUSY };
enum ppg_file_err { PPG_FILE_SUCCESS, PPG_FILE_OPEN_ERROR, PPG_FILE_CLOSE_ERROR, PPG_FILE_STATUS_ERROR, PPG_FILE_WRITE_ERROR };
struct ppg_file_hard {
    lfs_t lfs_fls_ppg_handle; lfs_file_t lfs_file_ppg_handle;
    char ppg_file_name[64]; bool ppg_file_status;
    unsigned fls_status, current_file_write_size, file_type;
};
static struct ppg_file_hard app_ppg_file_hardle;
static factory_delete_workspace delete_workspace;
static factory_delete_result factory_delete_last_result;
static int factory_delete_last_storage_error;
static unsigned checks, critical, closes, io_count, cut_at, cut_mode;
static bool dead, triggered, fail_read, slice_open_fail;
static uint8_t disk[4096*256], baseline[sizeof disk], payload[220];
static uint8_t rcache[64], pcache[64], lookahead[64];
uint32_t fixture_ticks;
uintptr_t fixture_task;
void fixture_enter(void) { ++critical; }
void fixture_leave(void) { CHECK(critical); --critical; }
static void bc_spi_flash_device_close(void) { ++closes; }
static bool lk_app_ppg_file_close(void);
static bool lk_app_ppg_file_open(unsigned type);
#include "factory_short_file.inc"
static lfs_t *fs(void) { return &app_ppg_file_hardle.lfs_fls_ppg_handle; }
static int read_flash(const struct lfs_config *c,lfs_block_t b,lfs_off_t off,void *data,lfs_size_t n)
{
    (void)c; CHECK(b<256 && off+n<=4096);
    if(dead || fail_read) return LFS_ERR_IO;
    memcpy(data,disk+b*4096+off,n); return 0;
}
static int change_flash(lfs_block_t b,lfs_off_t off,const void *data,lfs_size_t n,bool erase)
{
    unsigned i,count=n; const uint8_t *p=data; bool cut;
    CHECK(b<256 && off+n<=4096); if(dead) return LFS_ERR_IO;
    cut=++io_count==cut_at;
    if(cut && cut_mode==0) { dead=triggered=true; return LFS_ERR_IO; }
    if(cut && cut_mode==1) count/=2;
    for(i=0;i<count;++i) {
        unsigned at=b*4096+off+i;
        if(erase) disk[at]=255;
        else { CHECK((disk[at]&p[i])==p[i]); disk[at]&=p[i]; }
    }
    if(cut) { dead=triggered=true; return LFS_ERR_IO; } return 0;
}
static int program_flash(const struct lfs_config *c,lfs_block_t b,lfs_off_t off,const void *d,lfs_size_t n)
{ (void)c; return change_flash(b,off,d,n,false); }
static int erase_flash(const struct lfs_config *c,lfs_block_t b)
{ (void)c; return change_flash(b,0,NULL,4096,true); }
static int sync_flash(const struct lfs_config *c)
{ (void)c; if(dead) return LFS_ERR_IO; if(++io_count==cut_at) { dead=triggered=true; return LFS_ERR_IO; } return 0; }
static const struct lfs_config cfg={.read=read_flash,.prog=program_flash,.erase=erase_flash,.sync=sync_flash,
    .read_size=1,.prog_size=1,.block_size=4096,.block_count=256,.block_cycles=500,
    .cache_size=64,.lookahead_size=64,.read_buffer=rcache,.prog_buffer=pcache,.lookahead_buffer=lookahead};
static bool lk_app_ppg_file_close(void)
{
    bool ok=lk_ppg_file_close(&app_ppg_file_hardle)==PPG_FILE_SUCCESS;
    app_ppg_file_hardle.fls_status=ok?PPG_FLS_IDIE:PPG_FLS_BUSY; return ok;
}
static bool lk_app_ppg_file_open(unsigned type)
{
    (void)type; if(slice_open_fail) return false;
    strcpy(app_ppg_file_hardle.ppg_file_name,"/memo-tail.bin");
    if(factory_ppg_file_create(&app_ppg_file_hardle)!=PPG_FILE_SUCCESS) return false;
    app_ppg_file_hardle.fls_status=PPG_FLS_WRITE;
    app_ppg_file_hardle.current_file_write_size=0; return true;
}
static void mount_fresh(void)
{
    dead=fail_read=triggered=false; cut_at=io_count=0;
    memset(&app_ppg_file_hardle,0,sizeof app_ppg_file_hardle);
    CHECK(lfs_mount(fs(),&cfg)==0);
}
static void check_survivor(void)
{
    lfs_file_t file; uint8_t data[8];
    CHECK(lfs_file_open(fs(),&file,"/older.bin",LFS_O_RDONLY)==0);
    CHECK(lfs_file_read(fs(),&file,data,8)==8 && !memcmp(data,"precious",8));
    CHECK(lfs_file_close(fs(),&file)==0);
}
static void begin(const char *path,unsigned step)
{
    strcpy(app_ppg_file_hardle.ppg_file_name,path);
    CHECK(factory_ppg_file_create(&app_ppg_file_hardle)==PPG_FILE_SUCCESS);
    app_ppg_file_hardle.fls_status=PPG_FLS_WRITE;
    app_ppg_file_hardle.current_file_write_size=0;
    CHECK(factory_capture_begin(step)); CHECK(app_factory_capture_bind_file()); factory_capture_enable();
}
static void write_audio(unsigned bytes)
{
    uint8_t header[2];
    while(bytes) {
        unsigned n=bytes<sizeof payload?bytes:sizeof payload;
        factory_capture_ready(); CHECK(factory_capture_producer_enter());
        CHECK(factory_capture_queue_begin(header)); factory_capture_producer_leave();
        CHECK(factory_capture_sender_enter(header)); app_ppg_file_write(payload,n); factory_capture_sender_leave();
        bytes-=n;
    }
}
static bool finish(void) { factory_capture_stop_accepting(); return app_factory_capture_finish_file(); }
int main(void)
{
    unsigned step,kind,cuts=0,i,mode,final_io; int delta,exists; struct lfs_info info;
    lfs_file_t older;
    memset(disk,255,sizeof disk); memset(payload,0x53,sizeof payload);
    CHECK(lfs_format(fs(),&cfg)==0); mount_fresh();
    CHECK(lfs_file_open(fs(),&older,"/older.bin",LFS_O_CREAT|LFS_O_EXCL|LFS_O_WRONLY)==0);
    CHECK(lfs_file_write(fs(),&older,"precious",8)==8); CHECK(lfs_file_close(fs(),&older)==0);
    /* Both recording names use the production create/write/finish functions. */
    for(step=0;step<=10;++step) for(kind=0;kind<2;++kind) for(delta=-1;delta<=1;++delta) {
        unsigned n=step?step*2000U+delta:100U; const char *path=kind?"/memo.bin":"/clip.bin";
        begin(path,step); write_audio(n); CHECK(finish());
        exists=lfs_stat(fs(),path,&info);
        if(step && delta<0) CHECK(exists==LFS_ERR_NOENT);
        else { CHECK(exists==0 && info.size==n); CHECK(lfs_remove(fs(),path)==0); }
        CHECK(!factory_capture_active() && app_ppg_file_hardle.fls_status==PPG_FLS_IDIE); check_survivor();
    }
    /* A long memo's last short segment belongs to the whole recording. */
    begin("/memo-first.bin",10); write_audio(FLASH_MIN_USE_SPACE+100);
    CHECK(finish()); CHECK(lfs_stat(fs(),"/memo-tail.bin",&info)==0 && info.size<20000);
    /* Upload opens the old file; exclusive create cannot truncate it. */
    strcpy(app_ppg_file_hardle.ppg_file_name,"/older.bin");
    CHECK(factory_ppg_file_create(&app_ppg_file_hardle)==PPG_FILE_OPEN_ERROR);
    CHECK(lk_ppg_file_open(&app_ppg_file_hardle)==PPG_FILE_SUCCESS);
    CHECK(lk_ppg_file_close(&app_ppg_file_hardle)==PPG_FILE_SUCCESS); check_survivor();
    strcpy(app_ppg_file_hardle.ppg_file_name,"/missing.bin");
    CHECK(lk_ppg_file_open(&app_ppg_file_hardle)==PPG_FILE_OPEN_ERROR);
    CHECK(lfs_stat(fs(),"/missing.bin",&info)==LFS_ERR_NOENT);
    begin("/interrupted.bin",4); write_audio(100); factory_capture_fault();
    CHECK(!finish()); CHECK(lfs_stat(fs(),"/interrupted.bin",&info)==0 && info.size==100);
    begin("/wrong-owner.bin",4); write_audio(100);
    strcpy(app_ppg_file_hardle.ppg_file_name,"/older.bin");
    CHECK(!finish()); check_survivor();
    CHECK(lfs_stat(fs(),"/wrong-owner.bin",&info)==0 && info.size==100);
    /* Every mutating close/delete boundary: before, partial, and after I/O. */
    CHECK(lfs_unmount(fs())==0); memcpy(baseline,disk,sizeof disk); mount_fresh();
    begin("/cut.bin",4); write_audio(100); io_count=0; CHECK(finish()); final_io=io_count; CHECK(final_io>0);
    CHECK(lfs_unmount(fs())==0);
    for(i=1;i<=final_io;++i) for(mode=0;mode<3;++mode) {
        bool accepted;
        memcpy(disk,baseline,sizeof disk); mount_fresh(); begin("/cut.bin",4); write_audio(100);
        io_count=0; cut_at=i; cut_mode=mode; accepted=finish();
        CHECK(triggered); ++cuts;
        CHECK(!accepted && app_ppg_file_hardle.fls_status==PPG_FLS_BUSY);
        CHECK(lfs_unmount(fs())==0); mount_fresh();
        exists=lfs_stat(fs(),"/cut.bin",&info); CHECK(exists==0 || exists==LFS_ERR_NOENT);
        check_survivor(); CHECK(lfs_unmount(fs())==0);
    }
    memcpy(disk,baseline,sizeof disk); mount_fresh();
    begin("/write-fault.bin",4); dead=true; write_audio(440); dead=false;
    CHECK(!finish()); CHECK(lfs_stat(fs(),"/write-fault.bin",&info)==0); check_survivor();
    CHECK(lfs_unmount(fs())==0);
    printf("PASS P09 real files: %u checks, %u close/delete cold cuts, all thresholds and rollover\n",checks,cuts);
    return 0;
}
