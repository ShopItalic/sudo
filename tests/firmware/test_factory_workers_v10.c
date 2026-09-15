/* Execute actual generated upload workers with strict storage/RTOS shims.
 * Every storage call checks power and handle lifetime; failures retire handles.
 * A queued notification predates worker startup, and the next wait ends a run. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
typedef int32_t lfs_soff_t;
struct lfs_config { uint32_t block_size, block_count; };
typedef struct { const struct lfs_config *cfg; } lfs_t;
typedef struct { int unused; } lfs_file_t;
typedef struct { unsigned index; } lfs_dir_t;
struct lfs_info { unsigned type; uint32_t size; char name[256]; };
enum { LFS_TYPE_REG=1, LFS_TYPE_DIR=2, LFS_ERR_INVAL=-22, LFS_ERR_IO=-5, LFS_SEEK_SET=0 };
#define pdTRUE 1
#define portMAX_DELAY UINT32_MAX
#define PDM_DATA_SEND_SIZE 128
#define FILE_NAME_LENG 60
#define FILE_NUMBER_MAX 200
#define BC_LOG_INFO(...) ((void)0)
struct app_cmd_package { uint8_t cmd,subcmd; uint8_t data[512]; };
#include "worker_types.inc"
static struct app_cmd_package app_ppg_file_package;
static struct ppg_file_hard app_ppg_file_hardle;
static struct ppg_file_one_click_upload file_one_click_upload;
static uint32_t ppg_file_resume_upload_offset;
static uint8_t ppg_file_one_click_upload_index=1;
static const struct lfs_config config={4096,256};
static jmp_buf done;
static bool power,handle,fault;
static unsigned pending,wakes,opens,closes,sends,reads,position,dir_count,dir_handles,dog;
static unsigned size,fail_at;
enum { FAIL_NONE,FAIL_MOUNT,FAIL_OPEN,FAIL_READ,FAIL_SEEK,FAIL_CLOSE,FAIL_DIR_OPEN,FAIL_DIR_READ,FAIL_DIR_CLOSE };
static void bc_spi_flash_device_open(void) { power=true; }
static void bc_spi_flash_device_close(void) { power=false; }
static void p10_latch_storage_fault(void) { fault=true; app_ppg_file_hardle.fls_status=PPG_FLS_BUSY; }
static void p10_park_storage_worker(void) { bc_spi_flash_device_close();p10_latch_storage_fault();longjmp(done,2); }
static unsigned ulTaskNotifyTake(int clear,unsigned timeout) {
    assert(clear==pdTRUE && timeout==portMAX_DELAY);
    if(!pending) { assert(!power && !handle);assert(app_ppg_file_hardle.fls_status==PPG_FLS_IDIE);longjmp(done,1); }
    --pending;++wakes;return 1;
}
static int lfs_sfud_init(lfs_t *fs) { fs->cfg=&config;return fail_at==FAIL_MOUNT?-5:0; }
static void app_factory_cleanup_mount(lfs_t *fs) { assert(fs->cfg==&config); }
static int lk_ppg_file_open(struct ppg_file_hard *f) {
    assert(power && !handle);++opens;if(fail_at==FAIL_OPEN)return -5;
    handle=true;f->ppg_file_status=true;position=0;return 0;
}
static uint32_t ppg_file_size(struct ppg_file_hard *f) { (void)f;assert(power&&handle);return size; }
static int lk_ppg_file_close(struct ppg_file_hard *f) {
    assert(power && handle);++closes;handle=false;f->ppg_file_status=false;return fail_at==FAIL_CLOSE?-5:0;
}
static int ppg_file_close(struct ppg_file_hard *f) { return lk_ppg_file_close(f); }
static int lk_ppg_file_read(struct ppg_file_hard *f,uint8_t *out,unsigned n) {
    (void)f;assert(power&&handle&&position+n<=size);++reads;
    if(fail_at==FAIL_READ)return -5;
    memset(out,(int)(position%251),n);position+=n;return PPG_FILE_SUCCESS;
}
static int ppg_file_read(struct ppg_file_hard *f,uint8_t *out,unsigned n) { return lk_ppg_file_read(f,out,n); }
static lfs_soff_t lfs_file_seek(lfs_t *fs,lfs_file_t *file,lfs_soff_t offset,int whence) {
    (void)fs;(void)file;assert(power&&handle&&whence==LFS_SEEK_SET&&offset>=0&&(unsigned)offset<=size);
    if(fail_at==FAIL_SEEK)return -5;position=(unsigned)offset;return offset;
}
static int lfs_dir_open(lfs_t *fs,lfs_dir_t *dir,const char *path) {
    (void)fs;assert(power&&!strcmp(path,"/"));if(fail_at==FAIL_DIR_OPEN)return -5;
    ++dir_handles;dir->index=0;return 0;
}
static int lfs_dir_read(lfs_t *fs,lfs_dir_t *dir,struct lfs_info *entry) {
    (void)fs;assert(power&&dir_handles==1);if(fail_at==FAIL_DIR_READ)return -5;
    if(dir->index++>=dir_count)return 0;
    memset(entry,0,sizeof(*entry));entry->type=LFS_TYPE_REG;entry->size=size;
    memset(entry->name,'a',33);entry->name[33]='8';memcpy(entry->name+34,".bin",5);return 1;
}
static int lfs_dir_rewind(lfs_t *fs,lfs_dir_t *dir) { (void)fs;assert(power&&dir_handles==1);dir->index=0;return 0; }
static int lfs_dir_close(lfs_t *fs,lfs_dir_t *dir) { (void)fs;(void)dir;assert(power&&dir_handles==1);--dir_handles;return fail_at==FAIL_DIR_CLOSE?-5:0; }
static void bc_dog_feed(void) { assert(++dog<10000); }
static void app_package_ppg_file_uplaod(struct app_cmd_package *p,unsigned n) { assert(p && n<=sizeof(p->data));++sends; }
static void app_package_file_spi_uplaod(struct app_cmd_package *p,unsigned n) { app_package_ppg_file_uplaod(p,n); }
static enum app_file_up_mode file_up_mode_get(void) { return BLE_UP_MODE; }
static void app_ble_conn_time_audio_set(void) {}
static void app_ble_conn_time_audio_reset(void) {}
static void bc_delay_ms(unsigned n) { assert(n==17); }
static uint32_t bg_rtc_time_get_uinx_time(void) { return 100; }
#include "worker_functions.inc"
static void reset(unsigned failure) {
    memset(&app_ppg_file_hardle,0,sizeof(app_ppg_file_hardle));memset(&file_one_click_upload,0,sizeof(file_one_click_upload));
    app_ppg_file_hardle.lfs_fls_ppg_handle.cfg=&config;app_ppg_file_hardle.file_type=PPG_FILE_TYPE_16K_2_MIC_ADPCM;
    app_ppg_file_hardle.fls_status=PPG_FLS_UPLOAD;
    power=handle=fault=false;pending=1;wakes=opens=closes=sends=reads=position=dir_handles=dog=0;
    dir_count=1;size=256;fail_at=failure;ppg_file_resume_upload_offset=0;ppg_file_one_click_upload_index=1;
}
static void run(void (*worker)(void *),bool success) {
    int result=setjmp(done);if(!result){worker(NULL);assert(!"RTOS worker returned");}
    assert(result==(success?1:2));assert(fault!=success);assert(!power&&closes<=opens&&closes<=1);
    if(success)assert(wakes==1&&opens==1&&closes==1&&reads>0);
}
int main(void) {
    void (*workers[])(void *)={ppg_file_data_upload_handler_thread,ppg_file_resume_upload_handler_thread,ppg_file_one_click_upload_handler_thread};
    unsigned i,f;
    for(i=0;i<3;++i) {
        reset(FAIL_NONE);run(workers[i],true);
        for(f=FAIL_OPEN;f<=FAIL_CLOSE;++f) {reset(f);run(workers[i],false);if(f==FAIL_READ)assert(sends==(i==2?2U:0U));}
        reset(FAIL_NONE);size=UINT32_MAX;run(workers[i],false);
    }
    reset(FAIL_MOUNT);run(workers[0],false);assert(!opens);
    reset(FAIL_NONE);app_ppg_file_hardle.file_type=PPG_FILE_TYPE_ACC;run(workers[0],false);
    reset(FAIL_NONE);ppg_file_resume_upload_offset=128;run(workers[1],true);assert(reads==1&&position==256);
    reset(FAIL_NONE);ppg_file_resume_upload_offset=257;run(workers[1],false);
    for(f=FAIL_DIR_OPEN;f<=FAIL_DIR_CLOSE;++f){reset(f);run(workers[2],false);assert(!opens&&!dir_handles);}
    reset(FAIL_NONE);dir_count=201;run(workers[2],false);assert(!opens&&!dir_handles);
    puts("PASS P10 actual upload workers: queued-before-start handoff, exact resume, power/handle ownership, read/seek/open/close faults, bounded directory and sizes");
    return 0;
}
