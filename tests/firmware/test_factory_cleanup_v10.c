#include "app_factory_cleanup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"FAIL %u: %s\n",__LINE__,#x); exit(1); } } while(0)
static unsigned checks, io_count, cut_at, cut_mode, failed_removes, removed, cut_cases;
static bool dead, cut_hit;
static uint32_t ticks;
static uint8_t disk[4096*256], rcache[256], pcache[256], look[64], audio[6600];
static lfs_t fs;
static factory_cleanup engine;
static uint8_t id[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
static uint8_t saved_descriptor[52];
static int read_flash(const struct lfs_config *c,lfs_block_t b,lfs_off_t o,void *d,lfs_size_t n)
{ (void)c; CHECK(b<256 && o+n<=4096); if(dead)return LFS_ERR_IO; memcpy(d,disk+b*4096+o,n);return 0; }
static int mutate(lfs_block_t b,lfs_off_t o,const void *data,lfs_size_t n,bool erase)
{
    unsigned i,count=n; const uint8_t *p=data; bool cut;
    CHECK(b<256 && o+n<=4096);if(dead)return LFS_ERR_IO;
    cut=++io_count==cut_at;
    if(cut && cut_mode==0){dead=cut_hit=true;return LFS_ERR_IO;}
    if(cut && cut_mode==1) count/=2;
    for(i=0;i<count;++i){unsigned a=b*4096+o+i;if(erase)disk[a]=255;else{CHECK((disk[a]&p[i])==p[i]);disk[a]&=p[i];}}
    if(cut){dead=cut_hit=true;return LFS_ERR_IO;}return 0;
}
static int program(const struct lfs_config*c,lfs_block_t b,lfs_off_t o,const void*d,lfs_size_t n)
{(void)c;return mutate(b,o,d,n,false);}
static int erase(const struct lfs_config*c,lfs_block_t b)
{(void)c;return mutate(b,0,NULL,4096,true);}
static int sync_flash(const struct lfs_config*c)
{(void)c;if(dead)return LFS_ERR_IO;if(++io_count==cut_at){dead=cut_hit=true;return LFS_ERR_IO;}return 0;}
static const struct lfs_config cfg={.read=read_flash,.prog=program,.erase=erase,.sync=sync_flash,
    .read_size=1,.prog_size=1,.block_size=4096,.block_count=256,.block_cycles=20,
    .cache_size=256,.lookahead_size=64,.read_buffer=rcache,.prog_buffer=pcache,.lookahead_buffer=look};
static int test_remove(lfs_t *l,const char *path)
{ if(failed_removes){--failed_removes;return LFS_ERR_IO;}++removed;return lfs_remove(l,path); }
#define lfs_remove test_remove
#include "app_factory_cleanup.c"
#undef lfs_remove
static void boot(void)
{
    memset(&fs,0,sizeof fs);dead=false;cut_at=0;io_count=0;
    CHECK(lfs_mount(&fs,&cfg)==0);factory_cleanup_init(&engine,&fs);
}
static void create(const char *path,const void *p,unsigned n)
{
    lfs_file_t f;CHECK(lfs_file_open(&fs,&f,path,LFS_O_CREAT|LFS_O_EXCL|LFS_O_WRONLY)==0);
    CHECK(lfs_file_write(&fs,&f,p,n)==(int)n);CHECK(lfs_file_close(&fs,&f)==0);
}
static void setup(void)
{
    unsigned i;dead=cut_hit=false;cut_at=io_count=failed_removes=removed=ticks=0;
    memset(&fs,0,sizeof fs);memset(disk,255,sizeof disk);CHECK(lfs_format(&fs,&cfg)==0);boot();
    for(i=0;i<sizeof audio;++i) audio[i]=(uint8_t)(i*31U+i/256U);
    create("/older.bin","precious",8);create("/recording.bin",audio,sizeof audio);
}
static bool exists(const char *path)
{struct lfs_info info;int e=lfs_stat(&fs,path,&info);CHECK(e==0||e==LFS_ERR_NOENT);return e==0;}
static void verify_file(const char *path,const uint8_t *p,unsigned n)
{
    lfs_file_t f;uint8_t b[256];unsigned offset=0,want;
    CHECK(lfs_file_open(&fs,&f,path,LFS_O_RDONLY)==0);
    while(offset<n){want=n-offset;if(want>sizeof b)want=sizeof b;
        CHECK(lfs_file_read(&fs,&f,b,want)==(int)want);CHECK(!memcmp(b,p+offset,want));offset+=want;}
    CHECK(lfs_file_close(&fs,&f)==0);
}
static void survivor(void){verify_file("/older.bin",(const uint8_t *)"precious",8);}
static void steps(unsigned n)
{unsigned i;for(i=0;i<n;++i){ticks+=1000U;factory_cleanup_step(&engine,ticks);}}
static factory_cleanup_result prepare(void)
{
    ++id[0]; /* The adapter supplies fresh entropy on every preparation. */
    factory_cleanup_result r=factory_cleanup_prepare(&engine,(const uint8_t *)"recording.bin",13,id,ticks);
    if(r==FC_WORKING){while(engine.mode==1U){steps(1);CHECK(ticks<1000000U);}r=engine.result;}
    return r;
}
static void prepare_ok(void)
{CHECK(prepare()==FC_READY);memcpy(saved_descriptor,engine.descriptor,sizeof saved_descriptor);}
static void confirm_ok(void)
{CHECK(factory_cleanup_confirm(&engine,saved_descriptor,false,ticks)==FC_PENDING);}
static void known_hash(void)
{
    static const uint8_t expected[32]={0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
       0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad};
    sha256_context_t s;uint8_t h[32];CHECK(!sha256_init(&s));CHECK(!sha256_update(&s,(const uint8_t *)"abc",3));
    CHECK(!sha256_final(&s,h,0));CHECK(!memcmp(h,expected,32));
}
static void normal_and_identity(void)
{
    uint8_t descriptor_copy[52], attr[64];lfs_file_t f;unsigned i;
    setup();steps(100);CHECK(exists("/recording.bin")); /* transmission alone never deletes */
    prepare_ok();steps(100);CHECK(exists("/recording.bin")); /* prepared is not receipted */
    CHECK(prepare()==FC_READY);CHECK(!memcmp(saved_descriptor,engine.descriptor,52));
    memcpy(descriptor_copy,saved_descriptor,52);descriptor_copy[0]^=1;
    CHECK(factory_cleanup_confirm(&engine,descriptor_copy,false,ticks)==FC_STALE);CHECK(exists("/recording.bin"));
    CHECK(prepare()==FC_READY);confirm_ok();boot();steps(200);CHECK(!exists("/recording.bin"));survivor();
    CHECK(prepare()==FC_ABSENT);
    create("/recording.bin",audio,sizeof audio);CHECK(prepare()==FC_READY);
    CHECK(memcmp(saved_descriptor,engine.descriptor,16));
    CHECK(factory_cleanup_confirm(&engine,saved_descriptor,false,ticks)==FC_STALE);steps(100);CHECK(exists("/recording.bin"));
    setup();prepare_ok();confirm_ok();
    CHECK(lfs_file_open(&fs,&f,"/recording.bin",LFS_O_WRONLY)==0);
    CHECK(lfs_file_write(&fs,&f,"changed",7)==7);CHECK(lfs_file_close(&fs,&f)==0);
    steps(200);CHECK(exists("/recording.bin"));CHECK(prepare()==FC_FAILED);survivor();
    setup();prepare_ok();CHECK(lfs_getattr(&fs,"/recording.bin",FC_ATTR,attr,sizeof attr)==sizeof attr);
    attr[10]^=1;CHECK(lfs_setattr(&fs,"/recording.bin",FC_ATTR,attr,sizeof attr)==0);
    boot();steps(100);CHECK(exists("/recording.bin"));CHECK(prepare()==FC_MISMATCH);
    setup();prepare_ok();confirm_ok();failed_removes=100;
    for(i=0;i<FC_ATTEMPTS+3U;++i){steps(200);boot();}
    CHECK(exists("/recording.bin"));CHECK(prepare()==FC_FAILED);
    CHECK(engine.meta[5]==FC_ATTEMPTS);CHECK(failed_removes==100U-FC_ATTEMPTS);
    CHECK(factory_cleanup_confirm(&engine,engine.descriptor,false,ticks)==FC_FAILED);
    CHECK(factory_cleanup_confirm(&engine,engine.descriptor,true,ticks)==FC_PENDING);
    failed_removes=0;steps(200);CHECK(!exists("/recording.bin"));survivor();
}
static unsigned stage(unsigned which,unsigned cut,unsigned mode)
{
    unsigned total;
    setup();
    if(which>0)prepare_ok();
    if(which>1)confirm_ok();
    io_count=0;cut_at=cut;cut_mode=mode;cut_hit=false;
    if(which==0) (void)prepare();
    else if(which==1) (void)factory_cleanup_confirm(&engine,saved_descriptor,false,ticks);
    else steps(200);
    total=io_count;
    if(cut)CHECK(cut_hit);
    boot();steps(200);survivor();
    if(which==0) CHECK(exists("/recording.bin"));
    if(exists("/recording.bin"))verify_file("/recording.bin",audio,sizeof audio);
    if(cut)++cut_cases;
    return total;
}
static void bounds(void)
{
    unsigned n;uint8_t name[60],zero[16]={0};setup();
    for(n=0;n<=60;++n){memset(name,'x',sizeof name);if(n>=4)memcpy(name+n-4,".bin",4);
        CHECK(factory_cleanup_prepare(&engine,name,n,id,0)!=FC_WORKING);}
    CHECK(factory_cleanup_prepare(&engine,(const uint8_t *)"../recording.bin",16,id,0)==FC_INVALID);
    CHECK(factory_cleanup_prepare(&engine,(const uint8_t *)"recording.bin",13,zero,0)==FC_BUSY);
    CHECK(exists("/recording.bin"));survivor();
}
static void repeated_interruption_budget(void)
{
    unsigned i,j,charged;
    setup();prepare_ok();confirm_ok();
    for(i=0;i<FC_ATTEMPTS;++i) {
        boot();
        /* Let scan durably reserve an attempt, then cold-cut the first
         * mutation in removal. No failure bookkeeping can succeed. */
        for(j=0;j<20 && engine.mode!=3U;++j) steps(1);
        CHECK(engine.mode==3U); charged=engine.meta[5];CHECK(charged==i+1U);
        io_count=0;cut_at=1;cut_mode=0;cut_hit=false;
        steps(100);CHECK(cut_hit);boot();survivor();
        CHECK(exists("/recording.bin"));
    }
    steps(200);CHECK(prepare()==FC_FAILED);
    CHECK(engine.meta[5]==FC_ATTEMPTS && removed==FC_ATTEMPTS);
    for(i=0;i<8;++i){boot();steps(200);}
    CHECK(removed==FC_ATTEMPTS);survivor();
    CHECK(prepare()==FC_FAILED);
    CHECK(factory_cleanup_confirm(&engine,engine.descriptor,true,ticks)==FC_PENDING);
    steps(200);CHECK(!exists("/recording.bin"));survivor();
    /* Losing power just after charging also consumes the attempt even when
     * no removal was reached. Conservative retention is the intended result. */
    setup();prepare_ok();confirm_ok();
    for(i=0;i<FC_ATTEMPTS;++i){boot();for(j=0;j<20 && engine.mode!=3U;++j)steps(1);CHECK(engine.mode==3U);}
    boot();steps(200);CHECK(prepare()==FC_FAILED && removed==0);survivor();
}
static void fragmented_nearly_full_storage(void)
{
    unsigned count=0,i;char name[32];
    setup();
    while(lfs_fs_size(&fs)<240) {
        CHECK(count<128);snprintf(name,sizeof name,"/neighbor-%03u.bin",count++);
        create(name,audio,sizeof audio);
    }
    /* Fragment and refill the occupied space. Repeated attribute commits force
     * directory compaction while less than 7% of blocks remain available. */
    for(i=0;i<count;i+=2) {snprintf(name,sizeof name,"/neighbor-%03u.bin",i);CHECK(lfs_remove(&fs,name)==0);}
    for(i=0;i<count;i+=2) {snprintf(name,sizeof name,"/neighbor-%03u.bin",i);create(name,audio,sizeof audio);}
    for(i=0;i<30;++i) {prepare_ok();CHECK(lfs_unmount(&fs)==0);boot();}
    prepare_ok();confirm_ok();CHECK(lfs_unmount(&fs)==0);boot();steps(500);
    CHECK(!exists("/recording.bin"));survivor();
    for(i=0;i<count;++i) {snprintf(name,sizeof name,"/neighbor-%03u.bin",i);verify_file(name,audio,sizeof audio);}
    CHECK(lfs_unmount(&fs)==0);
}
#include "test_factory_cleanup_adapter_v10.c"
int main(void)
{
    unsigned s,n,i,m;
    reservation_interleavings(); wire_lifecycle(); wire_bounds();
    known_hash();bounds();normal_and_identity();repeated_interruption_budget();
    fragmented_nearly_full_storage();
    for(s=0;s<3;++s){n=stage(s,0,0);CHECK(n>0 && n<500);for(i=1;i<=n;++i)for(m=0;m<3;++m)(void)stage(s,i,m);}
    printf("PASS P10 cleanup: %u checks, %u cold cuts; receipts, IDs, SHA-256, reboot, retry cap and neighbor preservation\n",checks,cut_cases);
    return 0;
}
