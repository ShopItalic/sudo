"""P10 receipt protocol and serialized factory storage integration."""
import re
from factory_local_recording_v8 import replace_function, function_span

ADDED = ('app_factory_cleanup.c', 'app_factory_cleanup.h')
SHA = 'firmware/BCL603S2X/app/components/libraries/sha256/sha256.c'

def patch_sha(s):
    # Byte promotion to signed int makes a high-bit byte << 24 undefined C.
    # Preserve the supplier implementation/license, with explicit unsigned words.
    old='m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);'
    new='m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j + 1] << 16) | ((uint32_t)data[j + 2] << 8) | (uint32_t)data[j + 3];'
    if s.count(old)!=1: raise ValueError('Unexpected supplier SHA-256 word assembly')
    return s.replace(old,new)

ADAPTER = r'''
/* P10 owns only idle intervals. Recording/upload workers retain ownership
 * through their existing WRITE/UPLOAD state. Every synchronous filesystem
 * entry below participates in the same atomic reservation. */
static factory_cleanup p10_cleanup;
static TaskHandle_t p10_reservation_owner;
static volatile bool p10_mounted;
static volatile bool p10_storage_fault;
static bool p10_started, p10_prepared, p10_retry;
static uint32_t p10_nonce, p10_last_service;
static unsigned p10_name_size, p10_name_count, p10_receipt_count;
static uint8_t p10_name[58], p10_receipt[FC_DESCRIPTOR_BYTES], p10_random[16];
static uint8_t p10_reply[20];
static factory_cleanup_result p10_receipt_result;
static uint32_t p10_get32(const uint8_t *p)
{ return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static bool p10_claim_status(unsigned status,bool cleanup)
{
    bool ok;
    TaskHandle_t caller=xTaskGetCurrentTaskHandle();
    taskENTER_CRITICAL();
    ok=caller && p10_mounted && !p10_storage_fault && !p10_cleanup.faulted && !p10_reservation_owner &&
       app_ppg_file_hardle.fls_status==status &&
       (status!=PPG_FLS_IDIE || !app_ppg_file_hardle.ppg_file_status) &&
       (!cleanup || !factory_capture_active());
    if(ok) {
        p10_reservation_owner=caller;
        if(status==PPG_FLS_IDIE) app_ppg_file_hardle.fls_status=PPG_FLS_BUSY;
    }
    taskEXIT_CRITICAL();
    return ok;
}
static bool p10_claim_idle(void) { return p10_claim_status(PPG_FLS_IDIE,false); }
static bool p10_claim_cleanup(void) { return p10_claim_status(PPG_FLS_IDIE,true); }
static bool p10_claim_write(void) { return p10_claim_status(PPG_FLS_WRITE,false); }
static void p10_finish_write(bool ok)
{
    TaskHandle_t caller=xTaskGetCurrentTaskHandle();
    taskENTER_CRITICAL();
    if(caller && p10_reservation_owner==caller) {
        if(!ok) p10_storage_fault=true;
        app_ppg_file_hardle.fls_status=ok?PPG_FLS_IDIE:PPG_FLS_BUSY;
        p10_reservation_owner=NULL;
    }
    taskEXIT_CRITICAL();
}
static void p10_release_idle(void)
{
    TaskHandle_t caller=xTaskGetCurrentTaskHandle();
    taskENTER_CRITICAL();
    if(caller && p10_reservation_owner==caller) {
        if(!p10_storage_fault && app_ppg_file_hardle.fls_status==PPG_FLS_BUSY && !app_ppg_file_hardle.ppg_file_status)
            app_ppg_file_hardle.fls_status=PPG_FLS_IDIE;
        p10_reservation_owner=NULL;
    }
    taskEXIT_CRITICAL();
}
static void p10_latch_storage_fault(void)
{
    taskENTER_CRITICAL();
    p10_storage_fault=true; app_ppg_file_hardle.fls_status=PPG_FLS_BUSY;
    taskEXIT_CRITICAL();
}
static void p10_park_storage_worker(void)
{
    bc_spi_flash_device_close();
    p10_latch_storage_fault();
    /* FreeRTOS task functions must never return. Do not reset into the same
     * failing file; retain the recordings and keep BLE/other tasks running. */
    while(true) bc_delay_ms(1000);
}
static bool p10_wake_worker(void *handle)
{
    if(!handle) {
        taskENTER_CRITICAL();
        p10_storage_fault=true; app_ppg_file_hardle.fls_status=PPG_FLS_BUSY;
        taskEXIT_CRITICAL(); return false;
    }
    xTaskNotifyGive((TaskHandle_t)handle); return true;
}
void app_factory_cleanup_mount(lfs_t *fs)
{
    /* Publish initialized RAM on the storage worker under the same scheduler
     * exclusion used by consumers. This performs no filesystem operation. */
    taskENTER_CRITICAL();
    p10_mounted=false;
    factory_cleanup_init(&p10_cleanup,fs);
    if(fs && fs->cfg && fs->cfg->cache_size<=sizeof(p10_cleanup.cache)) p10_mounted=true;
    taskEXIT_CRITICAL();
}
void app_factory_cleanup_service(void)
{
    uint32_t now=(uint32_t)xTaskGetTickCount()*portTICK_PERIOD_MS;
    if(!p10_mounted || p10_cleanup.faulted || (uint32_t)(now-p10_last_service)<50U) return;
    if(p10_cleanup.mode==0 && (int32_t)(now-p10_cleanup.wake_at)<0) return;
    if(p10_cleanup.mode==2 && (uint32_t)(now-p10_cleanup.prepared_at)<30000U) return;
    if(!p10_claim_cleanup()) return;
    p10_last_service=now;
    bc_spi_flash_device_open(); factory_cleanup_step(&p10_cleanup,now); bc_spi_flash_device_close();
    p10_release_idle();
}
static bool p10_page(uint8_t *dst,unsigned size,unsigned *used,
                     const uint8_t *d,unsigned n)
{
    unsigned offset,count;
    if(n<11U) return false;
    offset=(unsigned)d[9]*9U;
    if(offset>=size || offset>*used) return false;
    count=size-offset; if(count>9U) count=9U;
    if(n!=10U+count) return false;
    if(offset<*used) return offset+count<=*used && !memcmp(dst+offset,d+10,count);
    memcpy(dst+offset,d+10,count); *used+=count; return true;
}
bool app_factory_cleanup_command(const uint8_t *d,unsigned n)
{
    factory_cleanup_result result=FC_INVALID;
    uint32_t nonce,now; unsigned page=0,count=0;
    if(!d || n<4U || d[2]!=0x84 || d[3]<0x20 || d[3]>0x24) return false;
    if(n<9U || n>20U) return true;
    memset(p10_reply,0,sizeof(p10_reply)); memcpy(p10_reply,d,9);
    if(d[4]!=1U || !(nonce=p10_get32(d+5))) goto reply;
    now=(uint32_t)xTaskGetTickCount()*portTICK_PERIOD_MS;
    if(!p10_mounted) { result=FC_BUSY; goto reply; }
    if(d[3]==0x20) {
        if(n!=9U) goto reply;
        result=(p10_storage_fault || p10_cleanup.faulted)?FC_STORAGE:FC_READY;
        memcpy(p10_reply+10,"P10",3); p10_reply[13]=16; p10_reply[14]=32;
        p10_reply[15]=FC_ATTEMPTS; p10_reply[16]=9; goto reply;
    }
    if(d[3]==0x21) {
        if(n!=11U || d[9]<5U || d[9]>58U || d[10]>1U) goto reply;
        /* Duplicate begin preserves collected bytes; a new nonce starts a
         * new transaction but cannot erase or cancel a durable receipt. */
        if(!p10_started || nonce!=p10_nonce) {
            p10_started=true; p10_nonce=nonce; p10_name_size=d[9]; p10_retry=d[10]!=0;
            p10_name_count=p10_receipt_count=0; p10_prepared=false; p10_receipt_result=FC_READY;
        } else if(p10_name_size!=d[9] || p10_retry!=(d[10]!=0)) goto reply;
        result=FC_READY; goto reply;
    }
    if(!p10_started || nonce!=p10_nonce) { result=FC_STALE; goto reply; }
    if(d[3]==0x22) {
        if(!p10_page(p10_name,p10_name_size,&p10_name_count,d,n)) goto reply;
        if(p10_name_count<p10_name_size) { result=FC_READY; goto reply; }
        if(p10_prepared) { result=p10_cleanup.result; goto reply; }
        if(!p10_claim_cleanup()) { result=FC_BUSY; goto reply; }
        memset(p10_random,0,sizeof(p10_random));
        if(sd_rand_application_vector_get(p10_random,sizeof(p10_random))!=NRF_SUCCESS)
            memset(p10_random,0,sizeof(p10_random));
        bc_spi_flash_device_open();
        result=factory_cleanup_prepare(&p10_cleanup,p10_name,p10_name_size,p10_random,now);
        bc_spi_flash_device_close(); p10_release_idle();
        if(result!=FC_BUSY) p10_prepared=true;
    } else if(d[3]==0x23) {
        if(n!=10U || d[9]>5U || !p10_prepared) goto reply;
        page=d[9]; p10_reply[10]=(uint8_t)page; result=p10_cleanup.result;
        if(p10_cleanup.mode==2U && (result==FC_READY || result==FC_FAILED)) {
            count=FC_DESCRIPTOR_BYTES-page*9U; if(count>9U) count=9U;
            memcpy(p10_reply+11,p10_cleanup.descriptor+page*9U,count);
            p10_cleanup.prepared_at=now;
        }
    } else if(d[3]==0x24) {
        if(!p10_prepared || !p10_page(p10_receipt,FC_DESCRIPTOR_BYTES,&p10_receipt_count,d,n)) goto reply;
        if(p10_receipt_count<FC_DESCRIPTOR_BYTES) { result=FC_READY; goto reply; }
        if(p10_receipt_result==FC_PENDING) { result=FC_PENDING; goto reply; }
        if(!p10_claim_cleanup()) { result=FC_BUSY; goto reply; }
        bc_spi_flash_device_open();
        result=factory_cleanup_confirm(&p10_cleanup,p10_receipt,p10_retry,now);
        bc_spi_flash_device_close(); p10_release_idle();
        if(result==FC_PENDING) p10_receipt_result=result;
    }
reply:
    p10_reply[9]=(uint8_t)result;
    app_package_send_enqueue((struct app_cmd_package *)p10_reply,sizeof(p10_reply));
    return true;
}
'''

def wrap_idle(s,name,return_type,args,call):
    # One declaration is split by a board #if in the original factory file.
    pattern=r'(?m)^('+return_type+r' '+name+r'\([^\n;]*\)(?:\n#endif)? *\n\{)'
    matches=list(re.finditer(pattern,s))
    if len(matches)!=1: raise ValueError('Expected one storage entry: '+name)
    m=matches[0]; end=s.index('\n}',m.end())+2
    body=s[m.start():end]
    body=body.replace(name+'(',name+'_p10_inner(',1)
    body=body.replace('app_ppg_file_hardle.fls_status != PPG_FLS_IDIE','false')
    body=body.replace('app_ppg_file_hardle.fls_status!=PPG_FLS_IDIE','false')
    fail='return;' if return_type=='void' else 'return false;'
    if name=='lk_app_ppg_file_open':
        fail='''{
#if defined(HANDWARE_1_23_2_ONE_SEC)
        return 1;
#else
        return false;
#endif
    }'''
    invoke=name+'_p10_inner('+call+');'
    if return_type=='void':
        wrapper=f'\n{return_type} {name}({args})\n{{\n    if(!p10_claim_idle()) {fail}\n    {invoke}\n    p10_release_idle();\n}}\n'
    else:
        wrapper=f'\n{return_type} {name}({args})\n{{\n    bool result;\n    if(!p10_claim_idle()) {fail}\n    result={invoke}\n    p10_release_idle();\n    return result;\n}}\n'
    return s[:m.start()]+body+s[end:]+wrapper

def wrap_close(s,name):
    start=s.index('bool '+name+'(void)\n{'); end=s.index('\n}',start)+2
    body=s[start:end].replace(name+'(',name+'_p10_inner(',1)
    # Some factory close paths exposed idle before the final flash power-down.
    # Publish idle only after the entire inner function returns successfully.
    body=body.replace('fls_status = PPG_FLS_IDIE','fls_status = PPG_FLS_WRITE')
    wrapper=f'''\nbool {name}(void)
{{
    bool ok;
    if(!p10_claim_write()) return false;
    ok={name}_p10_inner();
    p10_finish_write(ok);
    return ok;
}}
'''
    return s[:start]+body+s[end:]+wrapper

def directory_body(name):
    listing=name=='app_ppg_list_files'
    info=name=='app_ppg_list_files_info_get'
    fail='return;' if listing else 'return 0;'
    result='return;' if listing else 'return count;'
    init='memset(one_click_upload,0,sizeof(*one_click_upload));' if info else ''
    per='''
        if(count >= FILE_NUMBER_MAX-1U || strlen(entry.name)>=FILE_NAME_LENG) { error=LFS_ERR_INVAL; break; }
        memcpy(one_click_upload->ppg_file_name[count+1],entry.name,strlen(entry.name)+1U);
''' if info else ''
    send='''
    if(error >= 0 && lfs_dir_rewind(&file_hardle->lfs_fls_ppg_handle,&dir) < 0) error=LFS_ERR_IO;
    while(error >= 0 && count && (error=lfs_dir_read(&file_hardle->lfs_fls_ppg_handle,&dir,&entry)) > 0) {
        if(entry.type!=LFS_TYPE_REG) continue;
        if(strlen(entry.name)>sizeof(app_ppg_file_package.data)-12U) { error=LFS_ERR_INVAL; break; }
        ++index;
        memcpy(app_ppg_file_package.data,&count,4);
        memcpy(app_ppg_file_package.data+4,&index,4);
        memcpy(app_ppg_file_package.data+8,&entry.size,4);
        memcpy(app_ppg_file_package.data+12,entry.name,strlen(entry.name));
        app_package_ppg_file_uplaod(&app_ppg_file_package,12U+strlen(entry.name));
        bc_dog_feed();
    }
''' if listing else ''
    after='''
    if(!count) {
        memset(app_ppg_file_package.data,0,12);
        app_package_ppg_file_uplaod(&app_ppg_file_package,12);
    }
''' if listing else ('one_click_upload->ppg_file_number=(uint8_t)count;' if info else '')
    return f'''
    lfs_dir_t dir; struct lfs_info entry;
    uint32_t count=0{', index=0' if listing else ''}; int error,closed;
    {init}
    bc_spi_flash_device_open();
    error=lfs_dir_open(&file_hardle->lfs_fls_ppg_handle,&dir,path);
    if(error<0) {{ bc_spi_flash_device_close(); p10_latch_storage_fault(); {fail} }}
    while((error=lfs_dir_read(&file_hardle->lfs_fls_ppg_handle,&dir,&entry))>0) {{
        if(entry.type!=LFS_TYPE_REG) continue;
        {per}
        ++count; bc_dog_feed();
    }}
    {send}
    closed=lfs_dir_close(&file_hardle->lfs_fls_ppg_handle,&dir);
    bc_spi_flash_device_close();
    if(error<0 || closed<0) {{ p10_latch_storage_fault(); {fail} }}
    {after}
    {result}
'''

def patch_file(s):
    s='#include "app_factory_cleanup.h"\n#include "nrf_soc.h"\nstatic bool p10_claim_idle(void);\nstatic void p10_release_idle(void);\nstatic bool p10_claim_write(void);\nstatic void p10_finish_write(bool ok);\nstatic void p10_latch_storage_fault(void);\nstatic void p10_park_storage_worker(void);\nstatic bool p10_wake_worker(void *handle);\n'+s
    # Resume stays read-only. The app continues verified full-file replay.
    s=s.replace('if(ppg_file_open(&app_ppg_file_hardle) != 0)',
                'if(lk_ppg_file_open(&app_ppg_file_hardle) != 0)')
    # Only the generic new-recording entry needs exclusive creation here.
    start=s.index('bool app_ppg_file_open(enum ppg_file_type file_type)')
    end=s.index('\n}',start)+2
    b=s[start:end].replace('if(lk_ppg_file_open(&app_ppg_file_hardle) != 0)',
                           'if(factory_ppg_file_create(&app_ppg_file_hardle) != 0)')
    b=b.replace('ppg_file_close(&app_ppg_file_hardle);','/* No handle exists after failed exclusive creation. */')
    s=s[:start]+b+s[end:]
    # Independent explicit deletes/format/reclamation cannot bypass receipts.
    s=replace_function(s,'bool app_ppg_file_delete_request(const uint8_t *data, unsigned length)',
                       '    (void)data; (void)length; return false; /* P10 requires an exact durable receipt. */')
    s=replace_function(s,'void app_ppg_file_format(struct app_cmd_package * pack)',
                       '    pack->data[0]=0; app_package_send_enqueue(pack,5);')
    for name in ('app_ppg_space_reclamation','lk_ppg_space_reclamation'):
        s=replace_function(s,'void '+name+'()', '    /* Never reclaim an unacknowledged recording. */')
    for name in ('app_ppg_file_ls','app_ppg_file_sys_size_get'):
        s=wrap_idle(s,name,'void','struct app_cmd_package *pack','pack')
    for name in ('app_ppg_file_upload','app_file_active_upload','app_ppg_file_resume_upload','app_ppg_file_one_click_upload'):
        # Factory success fallthrough was undefined for these bool functions.
        m=re.search(r'(?m)^bool '+name+r'\([^;\n]*\) *\n\{',s); end=s.index('\n}',m.end())
        s=s[:end]+'\n    return true;'+s[end:]
        s=wrap_idle(s,name,'bool','struct app_cmd_package *pack','pack')
    for name in ('lk_app_ppg_file_open','app_ppg_file_open'):
        s=wrap_idle(s,name,'bool','enum ppg_file_type file_type','file_type')
    for name in ('app_ppg_list_capture_audio_up_check','app_ppg_file_time_init'):
        s=wrap_idle(s,name,'void','void','')
    for name in ('lk_app_ppg_file_close','app_ppg_file_close'):
        s=wrap_close(s,name)
    # Mount lives on the original storage worker, before it serves captures.
    anchor='  if (lfs_sfud_init(&app_ppg_file_hardle.lfs_fls_ppg_handle) != 0) {'
    start=s.index(anchor); end=s.index('\n  }',start)+4
    s=s[:end]+'\n  app_factory_cleanup_mount(&app_ppg_file_hardle.lfs_fls_ppg_handle);'+s[end:]
    # Directory enumeration owns no shared file handle. Bound every copy and
    # preserve an incomplete/error outcome instead of claiming an empty list.
    for name,ret,args in (
        ('app_ppg_list_files_number_get','uint16_t','struct ppg_file_hard *file_hardle,const char *path'),
        ('app_ppg_list_files','void','struct ppg_file_hard *file_hardle,const char *path'),
        ('app_ppg_list_files_info_get','uint16_t','struct ppg_file_hard *file_hardle,const char *path,struct ppg_file_one_click_upload *one_click_upload')):
        signature=re.search(r'(?m)^static '+ret+' '+name+r'\([^;\n]*\) *(?=\n\{)',s)[0]
        start,end=function_span(s,signature)
        s=s[:start]+'static '+ret+' '+name+'('+args+')\n{\n'+directory_body(name)+'\n}'+s[end:]
    # A failed capacity query is never permission to create or overwrite audio.
    s=s.replace('lfs_size_t used_size = (used_blocks >= 0) ? used_blocks * file_hardle->lfs_fls_ppg_handle.cfg->block_size : 0;',
        'if(used_blocks < 0) { p10_latch_storage_fault(); return 0; }\n    lfs_size_t used_size = used_blocks * file_hardle->lfs_fls_ppg_handle.cfg->block_size;')
    s=s.replace('lfs_size_t used_size = (used_blocks >= 0) ? used_blocks * app_ppg_file_hardle.lfs_fls_ppg_handle.cfg->block_size : 0;',
        'if(used_blocks < 0) { bc_spi_flash_device_close(); p10_latch_storage_fault(); return; }\n    lfs_size_t used_size = used_blocks * app_ppg_file_hardle.lfs_fls_ppg_handle.cfg->block_size;')
    # Counted task notifications survive a wake-up that happens before the
    # worker starts waiting. Suspend/resume could lose that handoff.
    for name in ('ppg_file_data_upload_handler_thread','ppg_file_resume_upload_handler_thread','ppg_file_one_click_upload_handler_thread'):
        a,b=function_span(s,'static void '+name+'(void * p_context)'); body=s[a:b]
        body=re.sub(r'bc_rtos_thread_suspend\([^;]+;', '', body)
        body=re.sub(r'while\(true\)\s*\{', 'while(true)\n  {\n    (void)ulTaskNotifyTake(pdTRUE,portMAX_DELAY);',body,count=1)
        # Error branches cannot publish idle, retry a retired handle, or return
        # out of a FreeRTOS task. Replace whole balanced branches (some include
        # nested switch statements and preprocessor blocks).
        for pattern in (r'if\((?:lk_)?ppg_file_open\(&app_ppg_file_hardle\) != 0\)',
                        r'if\((?:lk_)?ppg_file_close\(&app_ppg_file_hardle\) != 0\)'):
            while True:
                match=re.search(pattern+r'\s*\{',body)
                if not match: break
                start=match.end()-1; level=1; end=start+1
                while level:
                    if body[end]=='{': level+=1
                    elif body[end]=='}': level-=1
                    end+=1
                call=match[0].split(' != 0')[0][3:]
                body=body[:match.start()]+'if('+call+' != 0) p10_park_storage_worker();'+body[end:]
        body=re.sub(r'for\(uint8_t close_count = 0; close_count < 3;close_count\+\+\)\s*\{\s*if\(lk_ppg_file_close\(&app_ppg_file_hardle\) == 0\)\s*\{\s*break;\s*\}\s*\}',
            'if(lk_ppg_file_close(&app_ppg_file_hardle) != 0) p10_park_storage_worker();',body)
        body=re.sub(r'\breturn\s*;', 'p10_park_storage_worker();',body)
        body=body.replace('while (true) bc_delay_ms(1000);', 'p10_park_storage_worker();')
        # Keep power and UPLOAD ownership across enumeration/open/read/close.
        body=re.sub(r'(?m)^\s*(?://)?bc_spi_flash_device_(?:open|close)\(\);[^\n]*', '',body)
        body=body.replace('(void)ulTaskNotifyTake(pdTRUE,portMAX_DELAY);',
                          '(void)ulTaskNotifyTake(pdTRUE,portMAX_DELAY);\n    bc_spi_flash_device_open();')
        body=body.replace('app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;',
                          'bc_spi_flash_device_close();\n    app_ppg_file_hardle.fls_status = PPG_FLS_IDIE;')
        body=body.replace('file_size = ppg_file_size(&app_ppg_file_hardle);',
            'file_size = ppg_file_size(&app_ppg_file_hardle);\n    if(file_size > app_ppg_file_hardle.lfs_fls_ppg_handle.cfg->block_size * app_ppg_file_hardle.lfs_fls_ppg_handle.cfg->block_count) p10_park_storage_worker();')
        body=body.replace('file_size -= ppg_file_resume_upload_offset;',
            'if(ppg_file_resume_upload_offset > file_size) p10_park_storage_worker();\n    file_size -= ppg_file_resume_upload_offset;')
        body=body.replace('if(file_size % ppg_file_pack_size == 0)',
            'if(!ppg_file_pack_size || ppg_file_pack_size > sizeof(app_ppg_file_package.data)-17U) p10_park_storage_worker();\n    if(file_size % ppg_file_pack_size == 0)')
        offset='ppg_file_resume_upload_offset' if name=='ppg_file_resume_upload_handler_thread' else '0'
        body=body.replace('ppg_file_seek(&app_ppg_file_hardle,LFS_SEEK_SET);',
            'if(lfs_file_seek(&app_ppg_file_hardle.lfs_fls_ppg_handle,&app_ppg_file_hardle.lfs_file_ppg_handle,'+offset+',LFS_SEEK_SET) != (lfs_soff_t)'+offset+') p10_park_storage_worker();')
        body=re.sub(r'(?m)^(\s*)((?:lk_)?ppg_file_read\([^;]+\));',
                    r'\1if(\2 != PPG_FILE_SUCCESS) p10_park_storage_worker();',body)
        body=body.replace('app_ppg_list_files_info_get(&app_ppg_file_hardle,"/",&file_one_click_upload);',
            'if(!app_ppg_list_files_info_get(&app_ppg_file_hardle,"/",&file_one_click_upload)) p10_park_storage_worker();\n    bc_spi_flash_device_open();')
        body=body.replace('(index - ppg_file_one_click_upload_index) / (file_one_click_upload.ppg_file_number - ppg_file_one_click_upload_index) * 100',
            '(index - ppg_file_one_click_upload_index + 1U) * 100U / (file_one_click_upload.ppg_file_number - ppg_file_one_click_upload_index + 1U)')
        s=s[:a]+body+s[b:]
    s=re.sub(r'bc_rtos_thread_resume\((task_thread\[[^\]]+\]\.thread_handler)\);',r'if(!p10_wake_worker(\1)) return false;',s)
    # These workers now wait themselves; suspending them after creation would
    # defeat the notification even when it was delivered successfully.
    s=re.sub(r'if\(i >0\)\s*\{\s*bc_rtos_thread_suspend\(task_thread\[i\]\.thread_handler\);\s*\}', '',s)
    return s+ADAPTER

def patch_ble(s):
    s='#include "app_factory_cleanup.h"\n'+s
    s=s.replace('if (!app_factory_controls_command(',
                'if (!app_factory_cleanup_command(ble_recv_msg.data,ble_recv_msg.data_length) && !app_factory_controls_command(')
    s=s.replace('app_factory_scroll_service();','app_factory_scroll_service();\n            app_factory_cleanup_service();')
    # SHA/LittleFS work needs a measured budget separate from P09's 64-byte
    # static margin. This changes only the receive task's allocation.
    s=s.replace('.thread_stack_depth   = APP_TASK_BLE_RECV_STACK_SIZE',
                '.thread_stack_depth   = (4096U / sizeof(StackType_t))')
    return s
