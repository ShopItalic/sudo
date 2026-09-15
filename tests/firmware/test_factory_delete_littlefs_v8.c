#include "app_factory_delete.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c) do { checks++; if (!(c)) { fprintf(stderr,"FAIL %u: %s\n",__LINE__,#c); exit(1); } } while(0)
#define BLOCK_SIZE 4096U
#define BLOCK_COUNT 4096U
#define FLASH_SIZE (BLOCK_SIZE * BLOCK_COUNT)
static unsigned checks, critical, opens, closes;
static uint8_t flash_bytes[FLASH_SIZE], baseline[FLASH_SIZE];
static unsigned io_count, cut_at, cut_mode, cut_cases, erase_count;
static bool dead, triggered, fail_read;
static uint8_t rcache[64], pcache[64], lookahead[64];
enum { PPG_FLS_IDIE, PPG_FLS_UPLOAD, PPG_FLS_WRITE, PPG_FLS_BUSY, PPG_FLS_SIZE_ERROR };
static struct { lfs_t lfs_fls_ppg_handle; unsigned fls_status; bool ppg_file_status; } app_ppg_file_hardle;
#define taskENTER_CRITICAL() (++critical)
#define taskEXIT_CRITICAL() do { CHECK(critical); --critical; } while(0)
static void bc_spi_flash_device_open(void) { CHECK(!critical); CHECK(app_ppg_file_hardle.fls_status==PPG_FLS_BUSY); ++opens; }
static void bc_spi_flash_device_close(void) { CHECK(!critical); ++closes; }
#include "factory_delete_adapter.inc"

static int read_flash(const struct lfs_config *c,lfs_block_t block,lfs_off_t off,void *data,lfs_size_t size)
{
    (void)c; CHECK(block<BLOCK_COUNT && off+size<=BLOCK_SIZE);
    if (dead || fail_read) return LFS_ERR_IO;
    memcpy(data,flash_bytes+block*BLOCK_SIZE+off,size); return 0;
}
static int mutate_flash(const struct lfs_config *c,lfs_block_t block,lfs_off_t off,const void *data,lfs_size_t size,bool erase)
{
    const uint8_t *source=data; unsigned i,n=size; bool cut;
    (void)c; CHECK(block<BLOCK_COUNT && off+size<=BLOCK_SIZE);
    if(dead) return LFS_ERR_IO;
    cut=(++io_count==cut_at);
    if(cut && cut_mode==0) { dead=triggered=true; return LFS_ERR_IO; }
    if(cut && cut_mode==1) n=size/2;
    for(i=0;i<n;i++) {
        unsigned address=block*BLOCK_SIZE+off+i;
        if(erase) flash_bytes[address]=255;
        else { CHECK((flash_bytes[address]&source[i])==source[i]); flash_bytes[address]&=source[i]; }
    }
    if(cut) { dead=triggered=true; return LFS_ERR_IO; }
    return 0;
}
static int program_flash(const struct lfs_config *c,lfs_block_t b,lfs_off_t o,const void *data,lfs_size_t n)
{ return mutate_flash(c,b,o,data,n,false); }
static int erase_flash(const struct lfs_config *c,lfs_block_t b)
{ ++erase_count; return mutate_flash(c,b,0,NULL,BLOCK_SIZE,true); }
static int sync_flash(const struct lfs_config *c)
{
    (void)c; if(dead) return LFS_ERR_IO;
    if(++io_count==cut_at) { dead=triggered=true; return LFS_ERR_IO; } return 0;
}
static const struct lfs_config cfg={.read=read_flash,.prog=program_flash,.erase=erase_flash,.sync=sync_flash,
    .read_size=1,.prog_size=1,.block_size=BLOCK_SIZE,.block_count=BLOCK_COUNT,.block_cycles=500,
    .cache_size=64,.lookahead_size=64,.read_buffer=rcache,.prog_buffer=pcache,.lookahead_buffer=lookahead};
static lfs_t *fs(void) { return &app_ppg_file_hardle.lfs_fls_ppg_handle; }
static void create_file(const char *path,unsigned size,unsigned seed)
{
    lfs_file_t file; uint8_t cache[64], data[1024]; unsigned i;
    struct lfs_file_config fc={.buffer=cache};
    for(i=0;i<sizeof data;i++) data[i]=(uint8_t)(i*17+seed);
    CHECK(lfs_file_opencfg(fs(),&file,path,LFS_O_CREAT|LFS_O_EXCL|LFS_O_WRONLY,&fc)==0);
    while(size) { unsigned n=size<sizeof data?size:sizeof data; CHECK(lfs_file_write(fs(),&file,data,n)==(int)n); size-=n; }
    CHECK(lfs_file_close(fs(),&file)==0);
}
static void verify_file(const char *path,unsigned size,unsigned seed)
{
    lfs_file_t file; uint8_t cache[64], data[1024]; unsigned i;
    struct lfs_file_config fc={.buffer=cache}; struct lfs_info info;
    CHECK(lfs_stat(fs(),path,&info)==0 && info.size==size);
    CHECK(lfs_file_opencfg(fs(),&file,path,LFS_O_RDONLY,&fc)==0);
    while(size) { unsigned n=size<sizeof data?size:sizeof data; CHECK(lfs_file_read(fs(),&file,data,n)==(int)n);
        for(i=0;i<n;i++) CHECK(data[i]==(uint8_t)(i*17+seed)); size-=n; }
    CHECK(lfs_file_close(fs(),&file)==0);
}
static void mount_fresh(void)
{
    /* Deliberately abandon all RAM state after an injected cold cut. */
    dead=triggered=fail_read=false; cut_at=io_count=0;
    memset(&app_ppg_file_hardle,0,sizeof app_ppg_file_hardle);
    memset(rcache,0,sizeof rcache); memset(pcache,0,sizeof pcache); memset(lookahead,0,sizeof lookahead);
    CHECK(lfs_mount(fs(),&cfg)==0);
}
int main(void)
{
    const char *name="010203040506_2026_09_15:15:26:24_8.bin";
    const char *path="/010203040506_2026_09_15:15:26:24_8.bin";
    unsigned deletion_io,i,mode; lfs_ssize_t before,after; struct lfs_info info;
    memset(flash_bytes,255,sizeof flash_bytes);
    CHECK(lfs_format(fs(),&cfg)==0); mount_fresh();
    create_file(path,65536,3); create_file("/survivor.bin",32768,91);
    CHECK(lfs_mkdir(fs(),"/directory")==0);
    CHECK(lfs_unmount(fs())==0); memcpy(baseline,flash_bytes,sizeof baseline); mount_fresh();
    before=lfs_fs_size(fs()); verify_file(path,65536,3);
    CHECK(lfs_stat(fs(),path,&info)==0 && lfs_fs_size(fs())==before);
    app_ppg_file_hardle.fls_status=PPG_FLS_UPLOAD;
    CHECK(!app_ppg_file_delete_request((const uint8_t *)name,(unsigned)strlen(name)));
    app_ppg_file_hardle.fls_status=PPG_FLS_IDIE;
    CHECK(!app_ppg_file_delete_request((const uint8_t *)"directory",9));
    CHECK(lfs_stat(fs(),"/directory",&info)==0 && info.type==LFS_TYPE_DIR);
    CHECK(!memcmp(baseline,flash_bytes,sizeof baseline));
    io_count=0;
    CHECK(app_ppg_file_delete_request((const uint8_t *)name,(unsigned)strlen(name)));
    deletion_io=io_count; CHECK(deletion_io>0);
    CHECK(lfs_stat(fs(),path,&info)==LFS_ERR_NOENT);
    after=lfs_fs_size(fs()); CHECK(after<before);
    CHECK(app_ppg_file_delete_request((const uint8_t *)name,(unsigned)strlen(name)));
    CHECK(io_count==deletion_io);
    CHECK(lfs_unmount(fs())==0); mount_fresh();
    CHECK(lfs_stat(fs(),path,&info)==LFS_ERR_NOENT && lfs_fs_size(fs())==after);
    verify_file("/survivor.bin",32768,91);
    for(i=1;i<=deletion_io;i++) for(mode=0;mode<3;mode++) {
        int exists; bool accepted;
        memcpy(flash_bytes,baseline,sizeof baseline); mount_fresh();
        cut_at=i; cut_mode=mode; io_count=0;
        accepted=app_ppg_file_delete_request((const uint8_t *)name,(unsigned)strlen(name));
        CHECK(triggered); ++cut_cases;
        mount_fresh(); exists=lfs_stat(fs(),path,&info);
        CHECK(exists==0 || exists==LFS_ERR_NOENT);
        if(accepted) CHECK(exists==LFS_ERR_NOENT);
        if(exists==0) verify_file(path,65536,3);
        verify_file("/survivor.bin",32768,91);
        CHECK(app_ppg_file_delete_request((const uint8_t *)name,(unsigned)strlen(name)));
        CHECK(lfs_stat(fs(),path,&info)==LFS_ERR_NOENT);
        CHECK(opens==closes && !critical && app_ppg_file_hardle.fls_status==PPG_FLS_IDIE);
    }
    /* Fill the metadata journal until deletion itself must compact/erase.
     * The initial two-operation delete above never visits that failure path. */
    memcpy(flash_bytes,baseline,sizeof baseline); mount_fresh();
    for(i=0;i<256;i++) {
        uint8_t attribute[64]; memset(attribute,(int)i,sizeof attribute);
        CHECK(lfs_setattr(fs(),path,0x70,attribute,1+(i%sizeof attribute))==0);
        CHECK(lfs_unmount(fs())==0); memcpy(baseline,flash_bytes,sizeof baseline); mount_fresh();
        io_count=erase_count=0;
        CHECK(app_ppg_file_delete_request((const uint8_t *)name,(unsigned)strlen(name)));
        deletion_io=io_count;
        if(erase_count) break;
        memcpy(flash_bytes,baseline,sizeof baseline); mount_fresh();
    }
    CHECK(i<256 && erase_count>0);
    for(i=1;i<=deletion_io;i++) for(mode=0;mode<3;mode++) {
        int exists; bool accepted;
        memcpy(flash_bytes,baseline,sizeof baseline); mount_fresh();
        cut_at=i; cut_mode=mode; io_count=0;
        accepted=app_ppg_file_delete_request((const uint8_t *)name,(unsigned)strlen(name));
        CHECK(triggered); ++cut_cases;
        mount_fresh(); exists=lfs_stat(fs(),path,&info);
        CHECK(exists==0 || exists==LFS_ERR_NOENT);
        if(accepted) CHECK(exists==LFS_ERR_NOENT);
        if(exists==0) verify_file(path,65536,3);
        verify_file("/survivor.bin",32768,91);
        CHECK(app_ppg_file_delete_request((const uint8_t *)name,(unsigned)strlen(name)));
        CHECK(lfs_stat(fs(),path,&info)==LFS_ERR_NOENT && opens==closes);
    }
    memcpy(flash_bytes,baseline,sizeof baseline); mount_fresh(); fail_read=true;
    CHECK(!app_ppg_file_delete_request((const uint8_t *)name,(unsigned)strlen(name)));
    CHECK(!memcmp(flash_bytes,baseline,sizeof baseline)); fail_read=false;
    verify_file(path,65536,3); verify_file("/survivor.bin",32768,91);
    CHECK(app_ppg_file_delete_request((const uint8_t *)name,(unsigned)strlen(name)));
    for(i=0;i<24;i++) {
        create_file(path,65536,3);
        CHECK(app_ppg_file_delete_request((const uint8_t *)name,(unsigned)strlen(name)));
        CHECK(lfs_fs_size(fs())<=after+2);
    }
    CHECK(lfs_unmount(fs())==0);
    printf("PASS P08 real LittleFS: %u checks, %u cold-cut cases, %ld reclaimed bytes, 24 reuse cycles\n",
           checks,cut_cases,(long)(before-after)*BLOCK_SIZE);
    return 0;
}
