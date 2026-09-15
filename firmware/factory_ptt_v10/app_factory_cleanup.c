#include "app_factory_cleanup.h"
#include <string.h>

enum { META_UNCONFIRMED=0, META_PENDING=1, META_FAILED=2 };
static uint32_t get32(const uint8_t *p)
{ return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static void put32(uint8_t *p, uint32_t v)
{ unsigned i; for(i=0;i<4;++i) p[i]=(uint8_t)(v>>(i*8U)); }
static uint32_t crc(const uint8_t *p, unsigned n)
{
    uint32_t v=0xffffffffUL; unsigned i,b;
    for(i=0;i<n;++i) { v^=p[i]; for(b=0;b<8;++b) v=(v>>1)^((v&1U)?0xedb88320UL:0U); }
    return ~v;
}
static bool nonzero(const uint8_t *p, unsigned n)
{ unsigned i; uint8_t v=0; for(i=0;i<n;++i) v|=p[i]; return v!=0; }
static bool path_for(char *out, const uint8_t *name, unsigned n)
{
    unsigned i; uint8_t b;
    if(!name || n<5U || n>58U || name[0]=='.' || memcmp(name+n-4U,".bin",4)) return false;
    for(i=0;i<n;++i) {
        b=name[i];
        if(!((b>='0'&&b<='9')||(b>='A'&&b<='Z')||(b>='a'&&b<='z')||
             b=='_'||b=='-'||b==':'||b=='.')) return false;
    }
    out[0]='/'; memcpy(out+1,name,n); out[n+1]=0; return true;
}
static bool valid(const uint8_t *m)
{
    return !memcmp(m,"P10R",4) && m[4]<=META_FAILED && m[5]<=FC_ATTEMPTS &&
        m[6]<=FC_FAILED && m[7]==1U && nonzero(m+8,16) &&
        get32(m+60)==crc(m,60) &&
        (m[4]==META_UNCONFIRMED || (get32(m+24)>0 && nonzero(m+28,32)));
}
static factory_cleanup_result fault(factory_cleanup *c, int err)
{
    c->storage_error=err; c->result=FC_STORAGE; c->faulted=true; c->mode=0;
    return c->result; /* An unrecordable failure must never become erase authority. */
}
static factory_cleanup_result read_meta(factory_cleanup *c)
{
    int n=lfs_getattr(c->fs,c->path,FC_ATTR,c->check,FC_META_BYTES);
    if(n==LFS_ERR_NOENT) return FC_ABSENT;
    if(n==LFS_ERR_NOATTR) return FC_STALE;
    if(n<0) return fault(c,n);
    if(n!=FC_META_BYTES || !valid(c->check)) return FC_MISMATCH;
    return FC_READY;
}
static bool save_meta(factory_cleanup *c)
{
    int err;
    put32(c->meta+60,crc(c->meta,60));
    err=lfs_setattr(c->fs,c->path,FC_ATTR,c->meta,FC_META_BYTES);
    if(err<0) { fault(c,err); return false; }
    if(read_meta(c)!=FC_READY || memcmp(c->check,c->meta,FC_META_BYTES)) {
        fault(c,LFS_ERR_CORRUPT); return false;
    }
    return true;
}
static void descriptor(factory_cleanup *c)
{ memcpy(c->descriptor,c->meta+8,FC_DESCRIPTOR_BYTES); }
static factory_cleanup_result same_object(factory_cleanup *c)
{
    factory_cleanup_result r=read_meta(c); int err;
    if(r!=FC_READY) return r;
    if(memcmp(c->check,c->meta,FC_META_BYTES)) return FC_STALE;
    err=lfs_stat(c->fs,c->path,&c->info);
    if(err==LFS_ERR_NOENT) return FC_ABSENT;
    if(err<0) return fault(c,err);
    if(c->info.type!=LFS_TYPE_REG || c->info.size!=get32(c->meta+24)) return FC_MISMATCH;
    return FC_READY;
}
static void start_hash(factory_cleanup *c, unsigned mode)
{ c->offset=0; (void)sha256_init(&c->sha); c->mode=mode; c->result=FC_WORKING; }
void factory_cleanup_init(factory_cleanup *c, lfs_t *fs)
{
    memset(c,0,sizeof(*c)); c->fs=fs; c->result=FC_READY;
    c->file_cfg.buffer=c->cache;
}
factory_cleanup_result factory_cleanup_prepare(factory_cleanup *c,
    const uint8_t *name,unsigned length,const uint8_t fresh_id[FC_ID_BYTES],uint32_t now)
{
    int err; factory_cleanup_result r;
    if(!c || !c->fs || c->faulted) return FC_STORAGE;
    if(c->mode==1U || c->mode==3U) return FC_BUSY;
    if(!path_for(c->path,name,length)) return FC_INVALID;
    c->mode=0; c->prepared_at=now;
    err=lfs_stat(c->fs,c->path,&c->info);
    if(err==LFS_ERR_NOENT) return c->result=FC_ABSENT;
    if(err<0) return fault(c,err);
    if(c->info.type!=LFS_TYPE_REG || !c->info.size) return c->result=FC_INVALID;
    r=read_meta(c);
    if(r==FC_STALE) {
        if(!fresh_id || !nonzero(fresh_id,16)) return c->result=FC_BUSY;
        memset(c->meta,0,FC_META_BYTES); memcpy(c->meta,"P10R",4);
        c->meta[7]=1; memcpy(c->meta+8,fresh_id,16);
    } else if(r==FC_READY) memcpy(c->meta,c->check,FC_META_BYTES);
    else return c->result=r;
    if(c->meta[4]==META_PENDING && c->meta[5]>=FC_ATTEMPTS) {
        c->meta[4]=META_FAILED; c->meta[6]=FC_FAILED;
        if(!save_meta(c)) return c->result;
    }
    if(c->meta[4]!=META_UNCONFIRMED) {
        descriptor(c); c->mode=2;
        return c->result=c->meta[4]==META_FAILED?FC_FAILED:FC_READY;
    }
    put32(c->meta+24,c->info.size); memset(c->meta+28,0,32);
    if(!save_meta(c)) return c->result;
    start_hash(c,1); return c->result;
}
factory_cleanup_result factory_cleanup_confirm(factory_cleanup *c,
    const uint8_t d[FC_DESCRIPTOR_BYTES],bool explicit_retry,uint32_t now)
{
    factory_cleanup_result r;
    if(!c || !d || c->faulted) return FC_STORAGE;
    if(c->mode!=2U || !nonzero(d+20,32)) return FC_BUSY;
    if(memcmp(d,c->descriptor,FC_DESCRIPTOR_BYTES)) return FC_STALE;
    r=same_object(c); if(r!=FC_READY) return c->result=r;
    if(c->meta[4]==META_FAILED && !explicit_retry) return c->result=FC_FAILED;
    if(c->meta[4]!=META_PENDING) {
        c->meta[4]=META_PENDING; c->meta[5]=0; c->meta[6]=0;
        if(!save_meta(c)) return c->result;
    }
    c->wake_at=now+1000U; c->mode=0; c->cursor=0;
    return c->result=FC_PENDING; /* This is a durable receipt, never a delete ACK. */
}
static void failed_attempt(factory_cleanup *c,factory_cleanup_result reason,uint32_t now)
{
    factory_cleanup_result r=read_meta(c);
    if(r==FC_ABSENT) { c->mode=0; c->result=FC_ABSENT; c->cursor=0; return; }
    if(r!=FC_READY || memcmp(c->check,c->meta,FC_META_BYTES)) {
        c->mode=0; c->result=r==FC_READY?FC_STALE:r; return;
    }
    /* The attempt was charged durably before hashing/removal began. */
    c->meta[6]=(uint8_t)reason;
    if(reason==FC_MISMATCH || c->meta[5]>=FC_ATTEMPTS) c->meta[4]=META_FAILED;
    if(!save_meta(c)) return;
    c->mode=0; c->cursor=0;
    c->result=c->meta[4]==META_FAILED?FC_FAILED:FC_PENDING;
    c->wake_at=now+(1000U<<(c->meta[5]>4U?4U:c->meta[5]));
}
static void hash_step(factory_cleanup *c,uint32_t now)
{
    factory_cleanup_result r=same_object(c); int n,err,closed;
    unsigned want,mode=c->mode;
    if(r!=FC_READY) {
        if(mode==3 && r==FC_MISMATCH) failed_attempt(c,r,now);
        else { c->result=r; c->mode=0; c->cursor=0; }
        return;
    }
    want=get32(c->meta+24)-c->offset;
    if(want>FC_HASH_SLICE) want=FC_HASH_SLICE;
    memset(&c->file,0,sizeof(c->file));
    err=lfs_file_opencfg(c->fs,&c->file,c->path,LFS_O_RDONLY,&c->file_cfg);
    if(err<0) { if(mode==3) failed_attempt(c,FC_STORAGE,now); else fault(c,err); return; }
    err=lfs_file_seek(c->fs,&c->file,c->offset,LFS_SEEK_SET);
    n=err==(int)c->offset ? lfs_file_read(c->fs,&c->file,c->buffer,want) : LFS_ERR_IO;
    closed=lfs_file_close(c->fs,&c->file);
    if(n!=(int)want || closed<0) {
        if(mode==3) failed_attempt(c,FC_STORAGE,now); else fault(c,closed<0?closed:LFS_ERR_IO);
        return;
    }
    (void)sha256_update(&c->sha,c->buffer,want); c->offset+=want;
    if(c->offset<get32(c->meta+24)) return;
    (void)sha256_final(&c->sha,c->buffer,0);
    r=same_object(c);
    if(r!=FC_READY) { c->mode=0; c->result=r; c->cursor=0; return; }
    if(mode==1) {
        memcpy(c->meta+28,c->buffer,32);
        if(!save_meta(c)) return;
        descriptor(c); c->mode=2; c->prepared_at=now; c->result=FC_READY;
    } else {
        if(memcmp(c->meta+28,c->buffer,32)) { failed_attempt(c,FC_MISMATCH,now); return; }
        /* Identity, size and hash have matched under the caller's exclusive
         * storage reservation. No asynchronous gap exists before removal. */
        err=lfs_remove(c->fs,c->path);
        if(err<0) { failed_attempt(c,FC_STORAGE,now); return; }
        err=lfs_stat(c->fs,c->path,&c->info);
        if(err!=LFS_ERR_NOENT) { failed_attempt(c,FC_STORAGE,now); return; }
        c->mode=0; c->cursor=0; c->result=FC_ABSENT;
    }
}
void factory_cleanup_step(factory_cleanup *c,uint32_t now)
{
    int n,closed; unsigned len; factory_cleanup_result r;
    if(!c || !c->fs || c->faulted) return;
    if(c->mode==2) {
        if((uint32_t)(now-c->prepared_at)<30000U) return;
        c->mode=0; c->cursor=0; /* Unconfirmed descriptors never authorize deletion. */
    }
    if(c->mode==1 || c->mode==3) { hash_step(c,now); return; }
    if((int32_t)(now-c->wake_at)<0) return;
    memset(&c->dir,0,sizeof(c->dir));
    n=lfs_dir_open(c->fs,&c->dir,"/"); if(n<0) { fault(c,n); return; }
    n=lfs_dir_seek(c->fs,&c->dir,c->cursor);
    if(n==0) n=lfs_dir_read(c->fs,&c->dir,&c->info);
    if(n>0) c->cursor=lfs_dir_tell(c->fs,&c->dir);
    closed=lfs_dir_close(c->fs,&c->dir);
    if(n<0 || closed<0) { c->cursor=0; c->wake_at=now+30000U; c->result=FC_STORAGE; return; }
    if(!n) { c->cursor=0; c->wake_at=now+30000U; return; }
    if(c->info.type!=LFS_TYPE_REG) return;
    for(len=0;len<sizeof(c->info.name)&&c->info.name[len];++len) {}
    if(!path_for(c->path,(const uint8_t *)c->info.name,len)) return;
    r=read_meta(c);
    if(r!=FC_READY || c->check[4]!=META_PENDING) return;
    memcpy(c->meta,c->check,FC_META_BYTES);
    if(c->meta[5]>=FC_ATTEMPTS) {
        c->meta[4]=META_FAILED; c->meta[6]=FC_FAILED;
        if(save_meta(c)) c->result=FC_FAILED;
        return;
    }
    /* Write-ahead attempt accounting: no hash/delete can start until this
     * increment has committed and been independently read back. A reset may
     * consume an unused attempt, but cannot recreate its budget on reboot. */
    ++c->meta[5]; c->meta[6]=FC_WORKING;
    if(!save_meta(c)) return;
    start_hash(c,3);
}
