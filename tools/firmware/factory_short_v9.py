"""P09 recording finalization and checked storage, applied to factory source."""
from factory_local_recording_v8 import function_span, replace_function
from prepare_factory_ptt_v3 import once

ADDED = ('app_factory_short.c', 'app_factory_short.h', 'app_factory_short_settings.c')

def edit_function(s, signature, edit):
    a,b=function_span(s,signature)
    return s[:a]+edit(s[a:b])+s[b:]

def patch_pdm(s):
    s='#include "app_factory_short.h"\nstatic bool factory_pdm_initialized;\n'+s
    # ISR only publishes readiness/error flags. Codec work remains on its task.
    s=once(s,'void buffer_event_handler(nrfx_pdm_evt_t *p_evt)\n{',
           'void buffer_event_handler(nrfx_pdm_evt_t *p_evt)\n{\n'
           '  if (p_evt->error) factory_capture_isr_fault();\n'
           '  if (p_evt->buffer_requested) factory_capture_ready();')
    # Preserve encoding and sample rate; stamp the unused local packet prefix.
    helper='''static bool factory_pdm_enqueue(void *data)
{
    struct bc_adpcm_package *p = (struct bc_adpcm_package *)data;
    if (!factory_capture_queue_begin(p->pdm_data_buff)) return false;
    if (!bc_queue_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA, data)) {
        factory_capture_queue_failed(); return false;
    }
    return true;
}

'''
    signature='static void app_pdm_irq_handler_thread(void *thread_handler)'
    # The declaration has no body, so function_span locates only the definition.
    def producer(body):
        body=once(body,'      bc_rtos_thread_notify_take(bc_pdTRUE, bc_rtos_max_delay);',
                  '      bc_rtos_thread_notify_take(bc_pdTRUE, bc_rtos_max_delay);\n'
                  '      if (!factory_capture_producer_enter()) continue;')
        for call in ('bc_queue_enqueue','bc_queue_isr_enqueue'):
            body=body.replace(call+'(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package)', 'factory_pdm_enqueue(&adpcm_package)')
        pos=body.rindex('\n  }')
        return body[:pos]+'\n    factory_capture_producer_leave();'+body[pos:]
    s=edit_function(s,signature,producer)
    a,_=function_span(s,signature);s=s[:a]+helper+s[a:]
    def sender(body):
        body=once(body,'      switch(app_pdm_mode_get())',
                  '      if (!factory_capture_sender_enter(pdm_ble_package.pdm_package.pdm_data_buff)) continue;\n'
                  '      switch(app_pdm_mode_get())')
        body=once(body,'      }    \n    }','      }\n      factory_capture_sender_leave();\n    }')
        return body
    s=edit_function(s,'static void app_pdm_handler_thread(void *thread_handler)',sender)
    def hardware_open(body):
        body=body.replace('static void app_pdm_open(void)','static bool app_pdm_open(void)')
        body=once(body,'    err_code = nrfx_pdm_start();',
            '    if (err_code != NRF_SUCCESS) { factory_capture_fault(); return false; }\n'
            '    factory_pdm_initialized = true;\n'
            '    factory_capture_enable();\n'
            '    err_code = nrfx_pdm_start();\n'
            '    if (err_code != NRF_SUCCESS) { factory_capture_stop_accepting(); factory_capture_fault(); nrfx_pdm_uninit(); factory_pdm_initialized = false; return false; }')
        return body[:-1]+'  return true;\n}'
    s=edit_function(s,'static void app_pdm_open(void)',hardware_open)
    s=edit_function(s,'static void app_pdm_close(void)',lambda b:b.replace(
        '   bc_queue_clear(BC_QUEUE_TYPE_PDM_COLLECTION_DATA);',
        '   factory_capture_stop_accepting(); /* Drain accepted packets before file close. */').replace(
        '\tnrfx_pdm_stop();\n\tnrfx_pdm_uninit();',
        '    if (factory_pdm_initialized) { nrfx_pdm_stop(); nrfx_pdm_uninit(); factory_pdm_initialized = false; }'))
    start_signatures=('bool app_pdm_recording_start(void)','bool app_pdm_capture_recording_start(void)')
    for signature in start_signatures:
        def start(body):
            if signature == 'bool app_pdm_capture_recording_start(void)':
                body=body.replace('if(app_ppg_file_open(', 'if(lk_app_ppg_file_open(')
            mode='PDM_MODE_OFFLINE' if signature == start_signatures[0] else 'PDM_MODE_KEY_OFFLINE'
            return once(body,'  app_pdm_open();', '''  if (!factory_capture_begin(app_factory_short_steps())) {
    (void)lk_app_ppg_file_close(); pdm_status = PDM_IDIE; return false;
  }
  app_pdm_mode_set('''+mode+'''); /* Sender must know the mode before capture starts. */
  if (!app_factory_capture_bind_file() || !app_pdm_open()) {
    (void)lk_app_ppg_file_close(); factory_capture_end(); pdm_status = PDM_IDIE;
    app_pdm_mode_set(PDM_MODE_IDIE); return false;
  }''')
        s=edit_function(s,signature,start)
    for signature in ('bool app_pdm_recording_stop(void)','bool app_pdm_capture_recording_stop(void)'):
        def stop(body):
            body=once(body,'  app_pdm_close();', '''  app_pdm_close();
  {
    uint32_t began = xTaskGetTickCount();
    while (!factory_capture_quiet()) {
      if ((uint32_t)(xTaskGetTickCount() - began) >= pdMS_TO_TICKS(500)) {
        factory_capture_fault(); return false; /* Keep file owned; no unsafe close. */
      }
      bc_delay_ms(1);
    }
  }''')
            body=body.replace('lk_app_ppg_file_close()', 'app_factory_capture_finish_file()')
            body=body.replace('app_ppg_file_close()', 'app_factory_capture_finish_file()')
            return body
        s=edit_function(s,signature,stop)
    # Generic power/error stops preserve captured audio. Gestures call the
    # two normal stop functions above directly.
    s=edit_function(s,'void bc_pdm_stop(void)',lambda b:b.replace('\n{','''
{
    if (factory_capture_active()) {
        factory_capture_fault();
        if (app_pdm_mode_get() == PDM_MODE_OFFLINE) (void)app_pdm_recording_stop();
        else (void)app_pdm_capture_recording_stop();
        return; /* Drain owned packets and preserve interrupted audio. */
    }''',1))
    return s

def patch_file(s):
    s='#include "app_factory_short.h"\n'+s
    # Uploads open existing files read-only; new recordings create exclusively.
    # Both avoid the vendor uint8 retry-counter underflow on repeated failure.
    signature='static enum ppg_file_err lk_ppg_file_open(struct ppg_file_hard *file_hardle) '
    s=replace_function(s,signature,'''    if (lfs_file_open(&file_hardle->lfs_fls_ppg_handle,
            &file_hardle->lfs_file_ppg_handle, file_hardle->ppg_file_name,
            LFS_O_RDONLY) != 0) return PPG_FILE_OPEN_ERROR;
    file_hardle->ppg_file_status = true;
    return PPG_FILE_SUCCESS;''')
    a,_=function_span(s,signature)
    s=s[:a]+'''static enum ppg_file_err factory_ppg_file_create(struct ppg_file_hard *file_hardle)
{
    if (lfs_file_open(&file_hardle->lfs_fls_ppg_handle,
            &file_hardle->lfs_file_ppg_handle, file_hardle->ppg_file_name,
            LFS_O_RDWR | LFS_O_CREAT | LFS_O_EXCL) != 0) return PPG_FILE_OPEN_ERROR;
    file_hardle->ppg_file_status = true;
    return PPG_FILE_SUCCESS;
}

'''+s[a:]
    # Restrict exclusive creation to the new-recording call site. Uploads
    # must still open the exact existing file successfully.
    a=s.index('bool lk_app_ppg_file_open(enum ppg_file_type file_type)')
    b=s.index('bool app_ppg_file_open(enum ppg_file_type file_type)',a)
    s=s[:a]+once(s[a:b], 'if(lk_ppg_file_open(&app_ppg_file_hardle) != 0)',
                 'if(factory_ppg_file_create(&app_ppg_file_hardle) != 0)')+s[b:]
    # Upload failure has no open handle to close. Keep the worker alive and
    # storage unavailable rather than returning from a FreeRTOS task.
    for signature in ('static void ppg_file_data_upload_handler_thread(void * p_context)',
                      'static void ppg_file_one_click_upload_handler_thread(void * p_context)'):
        def upload(body):
            marker='if(lk_ppg_file_open(&app_ppg_file_hardle) != 0)'
            a=body.index(marker); b=body.index('}',body.index('return ;',a))+1
            return body[:a]+'''if(lk_ppg_file_open(&app_ppg_file_hardle) != 0)
      {
        bc_spi_flash_device_close();
        app_ppg_file_hardle.fls_status = PPG_FLS_BUSY;
        while (true) bc_delay_ms(1000);
      }'''+body[b:]
        s=edit_function(s,signature,upload)
    # Failed open has no valid lfs_file_t to close.
    s=once(s,'\t\t\tlk_ppg_file_close(&app_ppg_file_hardle);\n\t\t\tapp_ppg_file_hardle.fls_status = PPG_FLS_IDIE;',
             '\t\t\tapp_ppg_file_hardle.fls_status = PPG_FLS_IDIE;')
    s=once(s,'\t\tppg_file_write(&app_ppg_file_hardle,write_buff, write_length);',
             '''        bool written = ppg_file_write(&app_ppg_file_hardle,write_buff,write_length) == PPG_FILE_SUCCESS;
        factory_capture_written(write_length, written);
        if (!written) return;''')
    def write(body):
        body=once(body,'\t\t\tlk_app_ppg_file_close();',
                  '\t\t\tif (!lk_app_ppg_file_close()) { factory_capture_fault(); return; }')
        body=body.replace('BC_LOG_WARN("create new file failed after slice\\r\\n");',
                          'factory_capture_fault(); BC_LOG_WARN("create new file failed after slice\\r\\n");')
        # Even an externally closed/missing handle cannot look like a short
        # successful capture. Faults preserve any previous segments.
        return body[:-1]+'    if (!app_ppg_file_hardle.ppg_file_status) factory_capture_fault();\n}'
    s=edit_function(s,'void app_ppg_file_write(uint8_t *write_buff,uint32_t write_length) ',write)
    # Boot never operates on an unmounted filesystem after a mount failure.
    s=once(s,'  lfs_sfud_init(&app_ppg_file_hardle.lfs_fls_ppg_handle);',
             '''  if (lfs_sfud_init(&app_ppg_file_hardle.lfs_fls_ppg_handle) != 0) {
    app_ppg_file_hardle.fls_status = PPG_FLS_BUSY;
    while (true) bc_delay_ms(1000); /* Keep the RTOS task alive, storage unavailable. */
  }''')
    # Leave low-space recordings intact; a failed create must not reclaim
    # arbitrary earlier recordings to make room for a new short recording.
    s=s.replace('\t\tlk_ppg_space_reclamation();',
                '        bc_spi_flash_device_close(); return false; /* Storage full: preserve existing recordings. */')
    s=once(s,'        BC_LOG_INFO("lk_ppg_space_reclamation********stop\\r\\n");','')
    s+='''
/* A short capture cannot roll over (the slice is much longer than 5 s).
 * Bind its initial path only for this capture, so a foreign handle/name change
 * can never redirect automatic deletion to an older recording. */
static char factory_capture_path[FACTORY_DELETE_PATH_SIZE];
bool app_factory_capture_bind_file(void)
{
    unsigned n=0;
    factory_capture_path[0]=0;
    if (!factory_capture_active() || !app_ppg_file_hardle.ppg_file_status ||
        app_ppg_file_hardle.fls_status != PPG_FLS_WRITE) return false;
    while (n < sizeof(app_ppg_file_hardle.ppg_file_name) && app_ppg_file_hardle.ppg_file_name[n]) ++n;
    if (n >= sizeof(factory_capture_path) || n >= sizeof(app_ppg_file_hardle.ppg_file_name)) return false;
    memcpy(factory_capture_path, app_ppg_file_hardle.ppg_file_name, n+1U);
    return true;
}

/* Only the current capture can request this finalization. No path/ID enters
 * over BLE, no scan of older files, and no deferred deletion is scheduled. */
bool app_factory_capture_finish_file(void)
{
    bool ok, discard, captured;
    unsigned length = 0;
    if (!factory_capture_active() || !factory_capture_quiet() ||
        app_ppg_file_hardle.fls_status != PPG_FLS_WRITE ||
        !app_ppg_file_hardle.ppg_file_status) return false;
    discard = factory_capture_should_discard();
    captured = factory_capture_succeeded();
    if (discard && (!factory_capture_path[0] ||
        strncmp(factory_capture_path,app_ppg_file_hardle.ppg_file_name,sizeof(factory_capture_path)) != 0)) {
        discard=false; captured=false; /* Uncertain ownership: preserve audio. */
    }
    ok = lk_ppg_file_close(&app_ppg_file_hardle) == PPG_FILE_SUCCESS;
    if (ok && discard) {
        const char *path = app_ppg_file_hardle.ppg_file_name;
        while (length < sizeof(app_ppg_file_hardle.ppg_file_name) && path[length]) ++length;
        if (length < 2U || length >= sizeof(app_ppg_file_hardle.ppg_file_name) || path[0] != '/' ||
            !factory_delete_parse_name((const uint8_t *)path + 1, length - 1U, &delete_workspace)) ok = false;
        else {
            factory_delete_last_result = factory_delete_remove(&app_ppg_file_hardle.lfs_fls_ppg_handle, &delete_workspace);
            factory_delete_last_storage_error = delete_workspace.storage_error;
            ok = factory_delete_last_result == FACTORY_DELETE_REMOVED ||
                 factory_delete_last_result == FACTORY_DELETE_ALREADY_ABSENT;
        }
    }
    bc_spi_flash_device_close();
    app_ppg_file_hardle.fls_status = ok ? PPG_FLS_IDIE : PPG_FLS_BUSY;
    factory_capture_end();
    factory_capture_path[0]=0;
    return ok && captured;
}
'''
    return s

def patch_lfs(s):
    # The flash handle remains owned by this adapter; range checks precede I/O.
    signature='static int lfs_deskio_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)'
    # Prototypes vary only in parameter whitespace in the factory source.
    for name, operation, error in [('read','sfud_read',''),('prog','sfud_write',''),('erase','sfud_erase','')]:
        a=s.index('static int lfs_deskio_'+name+'('); b=s.index('\n}',a)+2
        signature=s[a:s.index('\n{',a)]
        size='c->block_size' if name=='erase' else 'size'
        offset='0' if name=='erase' else 'off'
        data='' if name=='erase' else ', buffer'
        call=(f'{operation}(flash, address, {size})' if name=='erase' else f'{operation}(flash, address, size, buffer)')
        buffer_guard='' if name=='erase' else '(size && !buffer) || '
        range_guard='' if name=='erase' else ' || off > c->block_size || size > c->block_size - off'
        body=f'''    uint32_t address;
    if (!c || !flash || !flash->init_ok || !c->block_size || {buffer_guard}block >= c->block_count{range_guard}) return LFS_ERR_IO;
    if ((uint64_t)block * c->block_size + {offset} + {size} > flash->chip.capacity) return LFS_ERR_IO;
    address = block * c->block_size + {offset};
    return {call} == SFUD_SUCCESS ? LFS_ERR_OK : LFS_ERR_IO;'''
        s=s[:a]+signature+'\n{\n'+body+'\n}'+s[b:]
    s=replace_function(s,'int lfs_sfud_init(lfs_t *lfs)', '''    int err;
    bc_spi_flash_device_open();
    if (sfud_init() != SFUD_SUCCESS) { bc_spi_flash_device_close(); return LFS_ERR_IO; }
    flash = sfud_get_device_table();
    err = lfs_mount(lfs, &cfg);
    /* Mount failure is not permission to format a Ring containing recordings. */
    bc_spi_flash_device_close();
    return err;''')
    return s

def patch_spi(s):
    # Propagate q_device failures instead of the vendor's unconditional true.
    s=once(s,'\t\tret = true;','\t\tret = false;') if '\t\tret = true;' in s else s
    # Replace the complete transfer function: uint8_t transport fields need
    # bounded chunks inside the caller-owned CS transaction.
    signature='bool bc_spi_flash_write_and_read(uint8_t *write_buff, uint32_t write_length,\n\tuint8_t *read_buff, uint32_t read_length)'
    a=s.index('bool bc_spi_flash_write_and_read('); b=s.index('\n}',a)+2
    signature=s[a:s.index('\n{',a)]
    s=s.replace('static q_device_t *spi_flash_dev = NULL;',
                'static q_device_t *spi_flash_dev = NULL;\nstatic bool factory_flash_opened, factory_flash_failed;')
    s=replace_function(s,'void bc_spi_flash_device_open(void)', '''    if (factory_flash_opened) return;
    factory_flash_failed = false;
    if (!spi_flash_dev) { factory_flash_failed = true; return; }
    bc_ldo_flash_power_on(); bc_delay_ms(15);
    if (q_device_open(spi_flash_dev) != RESULT_OK) {
        factory_flash_failed = true; bc_ldo_flash_power_off(); return;
    }
    factory_flash_opened = true; bc_delay_ms(10);
    spi_flash_device_wakeup();''')
    s=replace_function(s,'void bc_spi_flash_device_close(void)', '''    if (!factory_flash_opened) return;
    spi_flash_device_lowpower(); q_device_close(spi_flash_dev);
    factory_flash_opened = false; bc_ldo_flash_power_off();''')
    for name in ('high','low'):
        s=edit_function(s,'void bc_spi_flash_cs_'+name+'(void)',
                        lambda b:b.replace('q_device_ctrl(', 'if (factory_flash_opened) q_device_ctrl('))
    a=s.index('bool bc_spi_flash_write_and_read('); b=s.index('\n}',a)+2
    signature=s[a:s.index('\n{',a)]
    body='''    uint32_t n;
    if (!factory_flash_opened || factory_flash_failed || !spi_flash_dev ||
        (write_length && !write_buff) || (read_length && !read_buff)) return false;
    while (write_length || read_length) {
        n = write_length ? write_length : read_length;
        if (n > 255U) n = 255U;
        spi_pack.write_buff = write_buff; spi_pack.read_buff = read_buff;
        spi_pack.write_length = write_length ? n : 0;
        spi_pack.read_length = write_length ? 0 : n;
        if (q_device_write(spi_flash_dev, 0, &spi_pack, 0) != RESULT_OK) {
            factory_flash_failed = true; return false;
        }
        if (write_length) { write_buff += n; write_length -= n; }
        else { read_buff += n; read_length -= n; }
    }
    return true;'''
    return s[:a]+signature+'\n{\n'+body+'\n}'+s[b:]
