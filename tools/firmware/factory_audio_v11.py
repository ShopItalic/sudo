"""P11 audio delta applied only AFTER immutable P10-r2 parent verification."""
APP = "firmware/bc_ros/bc_application/"
QUEUE = "firmware/bc_ros/bc_module/queue/bc_queue.c"


def once(source, old, new):
    if source.count(old) != 1:
        raise ValueError("P11 expected one pinned anchor: " + repr(old[:100]))
    return source.replace(old, new, 1)


def function(source, signature, body):
    start = source.index(signature + "\n{")
    end = source.index("\n}", start) + 2
    return source[:start] + signature + "\n{\n" + body + "\n}" + source[end:]


def patch_pdm(source):
    source = '#include "app_factory_capture_p11.h"\n' + source
    source = once(source, "static bool factory_pdm_initialized;", "")
    start = source.index("#if defined(TEST_ADPCM_GEN)\nstatic MonoAdpcmProcessor")
    end = source.index("static bool buffer_requested_flag", start)
    source = source[:start] + source[end:]
    for signature, body in (
        ("void buffer_event_handler(nrfx_pdm_evt_t *p_evt)", "    (void)p_evt; /* P11 owns the real callback. */"),
        ("static bool app_pdm_open(void)", "    return p11_capture_start(&pdm_config);"),
        ("static void app_pdm_close(void)", "    p11_capture_stop();"),
        ("static bool factory_pdm_enqueue(void *data)", "    (void)data; return false; /* Legacy queue removed. */"),
        ("static void app_pdm_irq_handler_thread(void *thread_handler)", "    p11_capture_encoder_thread(thread_handler);"),
        ("static void app_pdm_handler_thread(void *thread_handler)", "    p11_capture_writer_thread(thread_handler);"),
        ("void bc_pdm_start(void)", "    /* Raw/live microphone entry is unavailable on P11. */"),
        ("void app_pdm_ble_stop(void)", "    bc_pdm_stop(); /* Same owned drain as all other exits. */"),
        ("void bc_pdm_stop(void)",
         "    if (!factory_capture_active()) return; /* Never uninit an idle driver. */\n"
         "    factory_capture_fault();\n"
         "    if (app_pdm_mode_get() == PDM_MODE_OFFLINE) (void)app_pdm_recording_stop();\n"
         "    else (void)app_pdm_capture_recording_stop();"),
    ):
        source = function(source, signature, body)
    # Only active local start paths produce SOPU. No global USE_OPUS switch:
    # that would activate the unrelated supplier codec implementation.
    for signature in ("bool app_pdm_recording_start(void)", "bool app_pdm_capture_recording_start(void)"):
        start = source.index(signature + "\n{"); end = source.index("\n}", start) + 2
        body = source[start:end].replace("PPG_FILE_TYPE_16K_2_MIC_ADPCM", "PPG_FILE_TYPE_16K_2_MIC_OPUS")
        body = once(body, "    (void)lk_app_ppg_file_close(); factory_capture_end(); pdm_status = PDM_IDIE;",
            "    factory_capture_fault();\n"
            "    if (!p11_capture_idle() || !factory_capture_quiet()) return false; /* Still owned: never close under a worker. */\n"
            "    (void)lk_app_ppg_file_close(); factory_capture_end(); pdm_status = PDM_IDIE;")
        source = source[:start] + body + source[end:]
    # Quiet includes DMA/encoder state, not only the legacy storage ledger.
    source = source.replace("while (!factory_capture_quiet())", "while (!p11_capture_idle() || !factory_capture_quiet())")
    return source


def patch_queue(source):
    start = source.index('.queue_name = "pdm data"')
    end = source.index("\n                                                            },", start)
    # Keep the legacy enum index stable. The new bounded SPSC queue owns
    # actual audio; this unused compatibility queue no longer reserves 22 KB.
    return source[:start] + '.queue_name = "pdm unused",\n .queue_count = 0,\n .queue_depth = 1,\n .queue_buff_length = 1,' + source[end:]


def patch_short_header(source):
    return once(source, "#define FACTORY_SHORT_SAMPLES_PER_STEP 4000U", "#define FACTORY_SHORT_SAMPLES_PER_STEP 8000U")


def patch_short(source):
    source += '''
/* P11 holds producer ownership from Start through codec tail completion. */
bool factory_capture_p11_hold(void)
{
    bool ok;
    taskENTER_CRITICAL();
    ok = capture.active && !capture.producer && !capture.sender && !capture.queued;
    if (ok) capture.producer = true;
    taskEXIT_CRITICAL();
    return ok;
}
void factory_capture_p11_samples(unsigned samples, bool written)
{
    const unsigned maximum = FACTORY_SHORT_MAX_STEPS * FACTORY_SHORT_SAMPLES_PER_STEP;
    taskENTER_CRITICAL();
    if (capture.active && capture.sender) {
        if (!written) capture.failed = true;
        else if (samples >= maximum - capture.samples) capture.samples = maximum;
        else capture.samples += samples;
    }
    taskEXIT_CRITICAL();
}
'''
    return source


def patch_file_header(source):
    for old, new in (
        ("PPG_FILE_TYPE_16K_2_MIC_OPUS,", "PPG_FILE_TYPE_16K_2_MIC_OPUS = 9,"),
        ("PPG_FILE_TYPE_16K_2_MIC_OPUS_CAPTURE,", "PPG_FILE_TYPE_16K_2_MIC_OPUS_CAPTURE = 'C' - '0',"),
        ("PPG_FILE_TYPE_8K_1_MIC_ADPCM,", "PPG_FILE_TYPE_8K_1_MIC_ADPCM = 'D' - '0',"),
        ("PPG_FILE_TYPE_8K_1_MIC_OPUS,", "PPG_FILE_TYPE_8K_1_MIC_OPUS = 'E' - '0',"),
    ):
        source = once(source, old, new)
    return source


def patch_file(source):
    source = '#include "app_factory_capture_p11.h"\n' + source
    source = once(source, 'void app_ppg_file_write(uint8_t *write_buff,uint32_t write_length) \n{',
        'void app_ppg_file_write(uint8_t *write_buff,uint32_t write_length) \n{\n'
        '    if (app_ppg_file_hardle.file_type == PPG_FILE_TYPE_16K_2_MIC_OPUS) {\n'
        '        factory_capture_fault(); return; /* Only the framed P11 writer may append. */\n    }')
    source += '''
bool p11_file_write(const uint8_t *data, unsigned length)
{
    bool ok;
    if (!data || !length || length > 32 || !factory_capture_active() ||
        app_ppg_file_hardle.fls_status != PPG_FLS_WRITE || !app_ppg_file_hardle.ppg_file_status ||
        app_ppg_file_hardle.file_type != PPG_FILE_TYPE_16K_2_MIC_OPUS ||
        app_ppg_file_hardle.current_file_write_size > 1024UL * 1024UL - length) return false;
    ok = ppg_file_write(&app_ppg_file_hardle, (uint8_t *)data, length) == PPG_FILE_SUCCESS;
    if (ok) app_ppg_file_hardle.current_file_write_size += length;
    return ok;
}
bool p11_file_rollover(void)
{
    bool ok;
    if (!factory_capture_active() || !p10_claim_write()) return false;
    /* The trailer is already written. Retain the SAME reservation across
     * close/exclusive-create so cleanup/upload cannot interleave. */
    ok = lk_app_ppg_file_close_p10_inner();
    if (ok) ok = lk_app_ppg_file_open_p10_inner(PPG_FILE_TYPE_16K_2_MIC_OPUS);
    taskENTER_CRITICAL();
    if (!ok) p10_storage_fault = true;
    app_ppg_file_hardle.fls_status = ok ? PPG_FLS_WRITE : PPG_FLS_BUSY;
    p10_reservation_owner = NULL;
    taskEXIT_CRITICAL();
    return ok;
}
'''
    return source


PATCHES = {
    APP + "app.c": lambda s: once(s, "    app_opus_create();", "    /* P11 codec allocation/self-test belongs to the encoder worker at first use. */"),
    APP + "app_pdm_handler.c": patch_pdm,
    APP + "app_factory_short.c": patch_short,
    APP + "app_factory_short.h": patch_short_header,
    APP + "app_ppg_file_data_handler.c": patch_file,
    APP + "app_ppg_file_data_handler.h": patch_file_header,
    QUEUE: patch_queue,
}
