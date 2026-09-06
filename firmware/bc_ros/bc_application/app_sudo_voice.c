#include "app_sudo_voice.h"

#include "app_sudo_capture.h"
#include "app_pdm_handler.h"
#include "app_ppg_file_data_handler.h"
#include "app_linear_motor_handler.h"
#include "bc_linear_motor.h"
#include "app_ble_handler.h"
#include "bc_voice_service.h"
#include "bc_voice_legacy_archive.h"
#include "bc_touch_button.h"
#include "bc_touch_tuning.h"
#include "bc_spi_flash_port.h"
#include "bc_ic_led.h"
#include "bc_ble.h"
#include "bc_ble_modu_interface.h"
#include "bc_ble_gatt.h"
#include "bc_queue.h"
#include "bc_rtc.h"
#include "bc_watchdog.h"
#include "lfs_port.h"
#include "nrf_soc.h"
#include "nrf_error.h"
#include "queue.h"
#include "user_rtos_config.h"

#include <string.h>

#if !defined(SUDO_VOICE_ONLY) || !defined(HANDWARE_1_23_2) || \
    defined(HANDWARE_1_23_2_ONE_SEC) || defined(HANDWARE_1_23_3)
#error "Sudo Voice worker is for standard 1.23.2 only"
#endif

#define COMMAND_DEPTH 12U
#define TOUCH_DEPTH 12U
#define SETTINGS_ATTR 0xa6U
#define SETTINGS_SIZE 20U
#define TUNING_ATTR 0xa7U
#define TUNING_SIZE 20U

typedef struct {
    uint8_t bytes[250];
    uint16_t length;
    uint32_t epoch;
} voice_command;
typedef struct { bc_touch_report_t report; TickType_t tick; } voice_touch;

static TaskHandle_t worker;
static QueueHandle_t commands, touches;
static lfs_t filesystem;
static bc_rec_store store;
static bc_recording recording;
static bc_voice_gesture gesture;
static bc_voice_service service;
static bc_voice_legacy_archive legacy;
static bool mounted, powered, legacy_ready;
static volatile bool initialized;
static uint32_t idle_since_ms;
static volatile bool working, touch_overflow;
static uint64_t published_ptt, touch_stop_id;
static bool touch_stop_invalid;
static bc_rec_phase observed_phase;
static uint64_t observed_id;
static bool feedback_start_pending, feedback_finish_pending, feedback_finish_success;
static uint64_t legacy_recording_id;
static voice_command legacy_stop;
static bool legacy_stop_pending;
static uint8_t legacy_reply[8];
static uint16_t legacy_reply_length;
static uint32_t legacy_reply_epoch;
static bool legacy_reply_pending;
/* Double tap is opt-in; hold/release remains available on a fresh Ring.
 * Persisted choices still take precedence when SETTINGS is loaded. */
static bc_voice_settings settings = {10000U, 0U, false, true, true};
static bc_voice_tuning tuning = {54U, 52U, 100U, 120U, 280U};

/* Convert elapsed RTOS ticks instead of truncating portTICK_PERIOD_MS (which
 * is zero at the supplier's 1024 Hz tick rate). Preserve fractional ticks and
 * unsigned wrap at both the RTOS and public millisecond clock boundaries. */
static uint32_t clock_ms(void)
{
    static TickType_t last_tick;
    static uint32_t milliseconds, remainder;
    TickType_t tick = xTaskGetTickCount();
    uint64_t elapsed = (uint64_t)(TickType_t)(tick - last_tick) * 1000U + remainder;
    milliseconds += (uint32_t)(elapsed / configTICK_RATE_HZ);
    remainder = (uint32_t)(elapsed % configTICK_RATE_HZ);
    last_tick = tick;
    return milliseconds;
}

static bool power_on(void)
{
    if (!powered) powered = bc_spi_flash_device_open_checked();
    return powered;
}

static bool mount_store(void);

static bool send_packet(void *ctx, const uint8_t *data, uint16_t length, uint32_t epoch)
{
    struct bc_ble_data_package packet = {{0}, 0, 0};
    (void)ctx;
    if (!data || !length || length > sizeof(packet.data) ||
        length > bc_ble_payload_limit() || !app_ble_notify_allowed()) return false;
    memcpy(packet.data, data, length); packet.data_length = length;
    return bc_queue_ble_send(&packet, epoch, 0);
}

static void reply_legacy(const voice_command *command, bool success)
{
    memcpy(legacy_reply, command->bytes, 4);
    legacy_reply[4] = success ? 1U : 0U;
    legacy_reply_length = 5U;
    legacy_reply_epoch = command->epoch;
    legacy_reply_pending = true;
}

static bool export_name(void *ctx, const bc_rec_start *start, char name[BC_REC_NAME_SIZE])
{
    static const char hex[] = "0123456789abcdef";
    lfs_dir_t directory;
    struct lfs_info info;
    char path[BC_REC_STORE_PATH_SIZE];
    uint8_t metadata[BC_REC_STORE_METADATA_SIZE];
    int result;
    unsigned i;
    (void)ctx;
    memset(name, 0, BC_REC_NAME_SIZE);
    for (i = 0; i < 12U; ++i) name[i] = hex[(start->id >> ((11U - i) * 4U)) & 15U];
    name[12] = '_';
    if (!bc_rtc_format_beijing_time(name + 13, 20U)) return false;
    memcpy(name + 32, start->trigger == BC_REC_PTT ? "_B.bin" : "_8.bin", 7U);
    /* Physical storage uses the full 64-bit ID. Also reject a collision in
     * the shorter SDK export alias, so a filename can never select two clips. */
    path[0] = '/'; memcpy(path + 1, name, 39);
    result = lfs_stat(&filesystem, path, &info);
    if (result != LFS_ERR_NOENT) return false;
    result = lfs_dir_open(&filesystem, &directory, BC_REC_STORE_DIR);
    if (result == LFS_ERR_NOENT) return true;
    if (result != 0) return false;
    while ((result = lfs_dir_read(&filesystem, &directory, &info)) > 0) {
        size_t length = strlen(info.name);
        if (info.type != LFS_TYPE_REG || length > 24U) continue;
        memcpy(path, BC_REC_STORE_DIR "/", sizeof(BC_REC_STORE_DIR));
        memcpy(path + sizeof(BC_REC_STORE_DIR), info.name, length + 1U);
        result = lfs_getattr(&filesystem, path, BC_REC_STORE_META_ATTR, metadata, sizeof(metadata));
        if (result == LFS_ERR_NOATTR) continue; /* Interrupted empty create. */
        if (result != sizeof(metadata) || memcmp(metadata, "SREC", 4) != 0 ||
            bc_voice_crc32(metadata, sizeof(metadata) - 4U) != bc_voice_get32(metadata + sizeof(metadata) - 4U) ||
            memcmp(metadata + 44, name, BC_REC_NAME_SIZE) == 0) {
            (void)lfs_dir_close(&filesystem, &directory); return false;
        }
    }
    if (lfs_dir_close(&filesystem, &directory) != 0) return false;
    return result == 0;
}

static bool mount_store(void)
{
    if (!power_on()) return false;
    if (mounted) return true;
    if (lfs_sfud_init(&filesystem) != 0) return false;
    mounted = bc_rec_store_init(&store, &filesystem, export_name, NULL);
    if (!mounted) (void)lfs_unmount(&filesystem);
    else if (initialized)
        legacy_ready = bc_voice_legacy_archive_init(&legacy, &filesystem, &store, send_packet, NULL);
    return mounted;
}

static bc_rec_result new_id(void *ctx, uint64_t *id)
{
    unsigned attempt;
    bc_rec_start existing;
    bc_rec_file file;
    (void)ctx;
    if (!id) return BC_REC_INVALID;
    if (legacy_ready && bc_voice_legacy_archive_cancel(&legacy) != BC_REC_OK) return BC_REC_CLOSE_ERROR;
    if (bc_voice_service_cancel_archive(&service) != BC_REC_OK) return BC_REC_CLOSE_ERROR;
    if (!mount_store()) return BC_REC_OPEN_ERROR;
    for (attempt = 0; attempt < 8U; ++attempt) {
        uint8_t bytes[8];
        bc_rec_store_status result;
        if (sd_rand_application_vector_get(bytes, sizeof(bytes)) != NRF_SUCCESS)
            return BC_REC_BUSY; /* Never manufacture a reused ID from an unset RTC. */
        *id = bc_voice_get64(bytes);
        if (*id == 0) continue;
        result = bc_rec_store_stat(&store, *id, &existing, &file);
        if (result == BC_REC_STORE_NOT_FOUND) return BC_REC_OK;
        if (result != BC_REC_STORE_OK) return BC_REC_OPEN_ERROR;
    }
    return BC_REC_ALREADY_EXISTS;
}

static bc_rec_result storage_open(void *ctx, const bc_rec_start *start, bc_rec_file *file)
{
    bc_rec_result result;
    (void)ctx;
    result = legacy_ready ? bc_voice_legacy_archive_cancel(&legacy) : BC_REC_OK;
    if (result != BC_REC_OK) return result;
    result = bc_voice_service_cancel_archive(&service);
    if (result != BC_REC_OK) return result;
    if (!mount_store()) return BC_REC_OPEN_ERROR;
    return bc_rec_store_open(&store, start, file);
}
static bc_rec_result storage_append(void *ctx, const uint8_t *data, uint16_t length)
{ (void)ctx; return bc_rec_store_append(&store, data, length); }
static bc_rec_result storage_checkpoint(void *ctx, bc_rec_file *file)
{ (void)ctx; return bc_rec_store_checkpoint(&store, file); }
static bc_rec_result storage_finish(void *ctx, bool complete, bc_rec_file *file)
{ (void)ctx; return bc_rec_store_finish(&store, complete, file); }

static bc_rec_result capture_start(void *ctx, uint64_t id)
{
    bc_rec_result result = app_sudo_capture_start(ctx, id);
    const bc_rec_snapshot *snapshot = bc_recording_snapshot(&recording);
    uint64_t cancelled;
    bool invalid;
    if (result == BC_REC_OK && snapshot->start.trigger == BC_REC_PTT) {
        app_sudo_capture_ptt_arm(id, gesture.config.touch_timeout_ms, snapshot->start.duration_limit_ms);
        taskENTER_CRITICAL(); cancelled = touch_stop_id; invalid = touch_stop_invalid; taskEXIT_CRITICAL();
        if (cancelled == id) app_sudo_capture_ptt_touch(id, !invalid, false);
    }
    return result;
}

static bool live(void *ctx, uint64_t id, uint32_t sequence, const uint8_t *data, uint16_t length)
{ (void)ctx; return bc_voice_service_live(&service, id, sequence, data, length); }

static void changed(void *ctx, const bc_rec_snapshot *snapshot)
{
    (void)ctx;
    working = snapshot->phase == BC_REC_STARTING || snapshot->phase == BC_REC_RECORDING ||
              snapshot->phase == BC_REC_STOPPING;
    taskENTER_CRITICAL();
    published_ptt = working && snapshot->start.trigger == BC_REC_PTT ? snapshot->start.id : 0;
    taskEXIT_CRITICAL();
    /* Keep transitions even if Start fails, or Start and Stop both complete
     * before this worker next updates the indicators. A custody receipt does
     * not represent another recording completion. */
    if (snapshot->phase == BC_REC_RECORDING &&
        (observed_phase != BC_REC_RECORDING || observed_id != snapshot->start.id))
        feedback_start_pending = true;
    if (snapshot->start.id && !working && snapshot->phase != BC_REC_DELIVERED &&
        (observed_id != snapshot->start.id || observed_phase != snapshot->phase)) {
        feedback_finish_pending = true;
        feedback_finish_success = snapshot->phase == BC_REC_SAVED;
    }
    observed_id = snapshot->start.id; observed_phase = snapshot->phase;
    /* Hardware feedback stays outside the owner callback. */
    bc_voice_service_changed(&service, snapshot);
}

static void touch_report(const bc_touch_report_t *report)
{
    voice_touch event;
    uint64_t id;
    if (!report || !touches || !worker) return;
    event.report = *report; event.tick = xTaskGetTickCount();
    taskENTER_CRITICAL();
    id = published_ptt;
    if (id && (!report->valid || !report->contact)) {
        touch_stop_id = id; touch_stop_invalid = !report->valid;
    }
    taskEXIT_CRITICAL();
    /* The touch task can stop DMA at its next complete block without waiting
     * for Flash. The queued report still determines owner state/final status. */
    if (id) app_sudo_capture_ptt_touch(id, report->valid, report->contact);
    if (xQueueSend(touches, &event, 0) != pdTRUE) {
        touch_overflow = true;
        if (id) app_sudo_capture_ptt_touch(id, false, false);
    }
    xTaskNotifyGive(worker);
}

static void settings_encode(uint8_t bytes[SETTINGS_SIZE], const bc_voice_settings *value)
{
    memset(bytes, 0, SETTINGS_SIZE); memcpy(bytes, "SVS1", 4);
    bytes[4] = (value->memo_enabled ? 1U : 0U) | (value->led_enabled ? 2U : 0U) |
                (value->haptic_enabled ? 4U : 0U);
    bc_voice_put32(bytes + 8, value->ptt_limit_ms);
    bc_voice_put32(bytes + 12, value->memo_limit_ms);
    bc_voice_put32(bytes + 16, bc_voice_crc32(bytes, 16));
}

static bc_rec_result save_settings(void *ctx, const bc_voice_settings *value)
{
    uint8_t bytes[SETTINGS_SIZE], check[SETTINGS_SIZE];
    int error;
    (void)ctx;
    if (!mount_store()) return BC_REC_OPEN_ERROR;
    settings_encode(bytes, value);
    if (lfs_getattr(&filesystem, "/", SETTINGS_ATTR, check, sizeof(check)) != sizeof(check) ||
        memcmp(bytes, check, sizeof(bytes)) != 0) {
        error = lfs_setattr(&filesystem, "/", SETTINGS_ATTR, bytes, sizeof(bytes));
        if (error != 0) return error == LFS_ERR_NOSPC ? BC_REC_NO_SPACE : BC_REC_SYNC_ERROR;
        if (lfs_getattr(&filesystem, "/", SETTINGS_ATTR, check, sizeof(check)) != sizeof(check) ||
            memcmp(bytes, check, sizeof(bytes)) != 0) return BC_REC_SYNC_ERROR;
    }
    settings = *value;
    bc_ic_led_feedback_enable(settings.led_enabled);
    bc_linear_motor_feedback_enable(settings.haptic_enabled);
    (void)bc_touch_tuning_request(tuning.touch_set, tuning.touch_clear, settings.memo_enabled);
    return BC_REC_OK;
}

static void load_settings(void)
{
    uint8_t bytes[SETTINGS_SIZE];
    if (!mount_store()) return;
    if (lfs_getattr(&filesystem, "/", SETTINGS_ATTR, bytes, sizeof(bytes)) != sizeof(bytes) ||
        memcmp(bytes, "SVS1", 4) != 0 || bytes[4] > 7U || bytes[5] || bytes[6] || bytes[7] ||
        bc_voice_get32(bytes + 16) != bc_voice_crc32(bytes, 16) ||
        bc_voice_get32(bytes + 8) > BC_REC_MAX_INTERVAL ||
        bc_voice_get32(bytes + 12) > BC_REC_MAX_INTERVAL) return;
    settings.ptt_limit_ms = bc_voice_get32(bytes + 8); settings.memo_limit_ms = bc_voice_get32(bytes + 12);
    settings.memo_enabled = (bytes[4] & 1U) != 0; settings.led_enabled = (bytes[4] & 2U) != 0;
    settings.haptic_enabled = (bytes[4] & 4U) != 0;
}

static void tuning_encode(uint8_t bytes[TUNING_SIZE], const bc_voice_tuning *value)
{
    memset(bytes, 0, TUNING_SIZE); memcpy(bytes, "SVT1", 4);
    bytes[4] = value->touch_set; bytes[5] = value->touch_clear;
    bytes[6] = value->haptic_strength;
    bc_voice_put16(bytes + 8, value->start_active_ms);
    bc_voice_put16(bytes + 10, value->stop_active_ms);
    bc_voice_put32(bytes + 16, bc_voice_crc32(bytes, 16));
}

static void load_tuning(void)
{
    uint8_t bytes[TUNING_SIZE];
    bc_voice_tuning value;
    if (!mount_store() || lfs_getattr(&filesystem, "/", TUNING_ATTR, bytes, sizeof(bytes)) != sizeof(bytes))
        return;
    if (memcmp(bytes, "SVT1", 4) != 0 || bytes[7] || bc_voice_get32(bytes + 12) ||
        bc_voice_get32(bytes + 16) != bc_voice_crc32(bytes, 16)) return;
    value.touch_set = bytes[4]; value.touch_clear = bytes[5]; value.haptic_strength = bytes[6];
    value.start_active_ms = (uint16_t)bytes[8] | ((uint16_t)bytes[9] << 8);
    value.stop_active_ms = (uint16_t)bytes[10] | ((uint16_t)bytes[11] << 8);
    if (bc_voice_tuning_valid(&value)) tuning = value;
}

static bool get_tuning(void *ctx, bc_voice_tuning *value, uint8_t *status)
{
    bc_touch_tuning_snapshot touch;
    (void)ctx;
    if (!value || !status || !bc_touch_tuning_snapshot_get(&touch)) return false;
    *value = tuning;
    *status = touch.touch_set == tuning.touch_set && touch.touch_clear == tuning.touch_clear &&
        touch.memo_enabled == settings.memo_enabled ? (uint8_t)touch.status : BC_TOUCH_TUNING_PENDING;
    return true;
}

static bc_rec_result save_tuning(void *ctx, const bc_voice_tuning *value)
{
    uint8_t bytes[TUNING_SIZE], check[TUNING_SIZE];
    int error;
    (void)ctx;
    if (!bc_voice_tuning_valid(value)) return BC_REC_INVALID;
    if (!mount_store()) return BC_REC_OPEN_ERROR;
    tuning_encode(bytes, value);
    if (lfs_getattr(&filesystem, "/", TUNING_ATTR, check, sizeof(check)) != sizeof(check) ||
        memcmp(bytes, check, sizeof(bytes)) != 0) {
        error = lfs_setattr(&filesystem, "/", TUNING_ATTR, bytes, sizeof(bytes));
        if (error != 0) return error == LFS_ERR_NOSPC ? BC_REC_NO_SPACE : BC_REC_SYNC_ERROR;
        if (lfs_getattr(&filesystem, "/", TUNING_ATTR, check, sizeof(check)) != sizeof(check) ||
            memcmp(bytes, check, sizeof(bytes)) != 0) return BC_REC_SYNC_ERROR;
    }
    tuning = *value;
    (void)bc_touch_tuning_request(tuning.touch_set, tuning.touch_clear, settings.memo_enabled);
    return BC_REC_OK;
}

static void feedback(void)
{
    if (feedback_finish_pending) {
        feedback_finish_pending = false;
        feedback_start_pending = false;
        bc_ic_led_mic_offline_recording_off();
        if (settings.haptic_enabled)
            (void)bc_linear_motor_pulse(tuning.haptic_strength,
                feedback_finish_success ? tuning.stop_active_ms : 400U);
        app_ble_conn_time_audio_reset();
    } else if (feedback_start_pending) {
        feedback_start_pending = false;
        if (settings.led_enabled) bc_ic_led_mic_offline_recording_on();
        if (settings.haptic_enabled) (void)bc_linear_motor_pulse(tuning.haptic_strength, tuning.start_active_ms);
        app_ble_conn_time_audio_set();
    }
}

static void legacy_record_command(const voice_command *command, uint32_t now_ms)
{
    uint8_t sub = command->bytes[3];
    const bc_rec_snapshot *snapshot = bc_recording_snapshot(&recording);
    bc_rec_result result = BC_REC_INVALID;
    if (sub == 0xf9 || sub == 0xfc || sub == 0xfd) {
        /* Reconnect, link loss, and the old online-to-offline command cannot
         * terminate a locally owned clip. Native recording is always local. */
        if (sub == 0xfd) reply_legacy(command, bc_recording_active(&recording));
        return;
    }
    if (sub == 0x0c) { reply_legacy(command, bc_recording_active(&recording)); return; }
    if ((sub != 0 && sub != 1 && sub != 5 && sub != 0xfe) ||
        command->length != 5 || command->bytes[4] > 1U) {
        reply_legacy(command, false); return;
    }
    if (command->bytes[4]) {
        if (bc_recording_active(&recording)) {
            /* Legacy Start has no persistent ID: only retry the session
             * already created by this app command, never hijack a gesture. */
            result = snapshot->start.id == legacy_recording_id ? BC_REC_OK : BC_REC_BUSY;
        } else {
            bc_rec_start start = {0};
            result = new_id(NULL, &start.id);
            if (result == BC_REC_OK) {
                start.trigger = BC_REC_APP;
                result = bc_recording_start(&recording, &start, now_ms);
                if (result == BC_REC_OK) legacy_recording_id = start.id;
            }
        }
        reply_legacy(command, result == BC_REC_OK);
    } else if (snapshot->start.id) {
        /* A legacy app Stop may stop a double-tap memo as before. Only v1
         * supports session-tagged Stop and replay across restarts. */
        result = bc_recording_stop(&recording, snapshot->start.id, now_ms);
        if (bc_recording_active(&recording)) { legacy_stop = *command; legacy_stop_pending = true; }
        else reply_legacy(command, result == BC_REC_OK && snapshot->phase == BC_REC_SAVED);
    } else reply_legacy(command, false);
}

static bool owned_legacy_control(uint8_t cmd, uint8_t sub)
{
    if (cmd == CMD_LED || cmd == CMD_MOTOR || cmd == CMD_CONFIG_TOUCH ||
        cmd == CMD_LED_MOTOR_MODE_SET || cmd == CMD_LED_MOTOR_MODE_GET) return true;
    /* Manufacturing commands bypass recording ownership, or reset/disable
     * capture hardware. They are unsupported in this voice-only profile.
     * Pairing, authentication and the existing DFU service are unchanged. */
    if (cmd == CMD_SYS_SET && (sub == 3U || sub == 0x0cU)) return true;
    if (cmd != CMD_TOOL_TEST) return false;
    return sub == 5U || sub == 9U || sub == 24U || sub == 27U ||
        (sub >= 36U && sub <= 40U) || sub == 49U || sub == 58U ||
        sub == 59U || sub == 61U;
}

static void legacy_control(const voice_command *command)
{
    bool success = false;
    uint8_t cmd = command->bytes[2], sub = command->bytes[3];
    if (cmd == CMD_TOOL_TEST && sub == 40U && command->length == 4U) {
        /* Cached at mount: the factory query otherwise powers Flash off. */
        memcpy(legacy_reply, command->bytes, 4U);
        bc_voice_put32(legacy_reply + 4U, lfs_sfud_jedec_id());
        legacy_reply_length = 8U; legacy_reply_epoch = command->epoch;
        legacy_reply_pending = true; return;
    }
    if (!bc_recording_active(&recording) && !gesture.hold_attempted) {
        if (cmd == CMD_LED && sub == 4U && command->length == 7U && settings.led_enabled) {
            uint8_t grb[3]; memcpy(grb, command->bytes + 4U, sizeof(grb));
            bc_ic_led_set(grb); success = true;
        } else if (cmd == CMD_LED && sub == 7U && command->length == 4U) {
            bc_ic_led_stop(); success = true;
        } else if (cmd == CMD_MOTOR && sub == 4U && command->length == 5U &&
                   command->bytes[4] == 1U && settings.haptic_enabled) {
            success = bc_linear_motor_pulse(100U, 120U);
        }
    }
    /* No guessed configuration success or unbounded legacy motor loops.
     * Versioned SETTINGS/TUNING provide durable configuration and readback. */
    reply_legacy(command, success);
}

static void run(void *ctx)
{
    bc_rec_config config = {1000U, 4096U, 500U};
    bc_rec_port port = {NULL, storage_open, storage_append, storage_checkpoint, storage_finish,
        capture_start, app_sudo_capture_stop, app_sudo_capture_abort, live, changed};
    bc_voice_service_port service_port = {NULL, send_packet, save_settings};
    bc_voice_tuning_port tuning_port = {NULL, get_tuning, save_tuning};
    bc_voice_gesture_config gesture_config = {10000U, 0U, 750U, 300U, false};
    uint32_t last_epoch = 0;
    bool last_connected = false;
    (void)ctx;
    load_settings();
    /* Apply persisted application-wide feedback policy before accepting
     * gestures/commands. Bootloader and pre-load boot output are separate. */
    bc_ic_led_feedback_enable(settings.led_enabled);
    bc_linear_motor_feedback_enable(settings.haptic_enabled);
    load_tuning();
    (void)bc_touch_tuning_request(tuning.touch_set, tuning.touch_clear, settings.memo_enabled);
    gesture_config.ptt_limit_ms = settings.ptt_limit_ms;
    gesture_config.memo_limit_ms = settings.memo_limit_ms;
    gesture_config.memo_enabled = settings.memo_enabled;
    app_sudo_capture_init(worker);
    if (!bc_recording_init(&recording, &port, &config) ||
        !bc_voice_gesture_init(&gesture, &recording, &gesture_config, new_id, NULL) ||
        !bc_voice_service_init(&service, &recording, &store, &gesture, &service_port, &settings) ||
        !bc_voice_service_set_tuning_port(&service, &tuning_port)) {
        if (powered) { bc_spi_flash_device_close(); powered = false; }
        /* Invalid static configuration must never register a gesture or
         * acknowledge a recording. The remaining system can still recover
         * the device through its existing pairing and DFU services. */
        for (;;) { bc_dog_feed(); (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100U)); }
    }
    legacy_ready = bc_voice_legacy_archive_init(&legacy, &filesystem, &store, send_packet, NULL);
    bc_touch_button_touch_report_register_callback(touch_report);
    initialized = true;
    for (;;) {
        voice_touch touch;
        voice_command command;
        uint32_t now_ms = clock_ms();
        uint32_t epoch = bc_ble_session_id();
        bool connected = bc_ble_connect_status();
        unsigned i;
        if (epoch != last_epoch || connected != last_connected) {
            if (powered || mounted) power_on();
            (void)bc_voice_legacy_archive_cancel(&legacy);
            bc_voice_service_link(&service, epoch, connected);
            legacy_reply_pending = false; legacy_stop_pending = false;
            last_epoch = epoch; last_connected = connected;
        }
        if (touch_overflow) {
            touch_overflow = false;
            if (bc_recording_active(&recording) && recording.snapshot.start.trigger == BC_REC_PTT)
                (void)bc_recording_fault(&recording, recording.snapshot.start.id, BC_REC_TOUCH_ERROR, now_ms);
        }
        for (i = 0; i < TOUCH_DEPTH && xQueueReceive(touches, &touch, 0) == pdTRUE; ++i) {
            uint32_t age_ms = (uint32_t)((uint64_t)(TickType_t)(xTaskGetTickCount() - touch.tick) * 1000U / configTICK_RATE_HZ);
            (void)bc_voice_gesture_report(&gesture, &touch.report, now_ms - age_ms);
        }
        bc_voice_gesture_tick(&gesture, now_ms);
        bc_recording_tick(&recording, now_ms);
        /* Consume at most one block before checking touch and commands again. */
        (void)app_sudo_capture_poll(&recording, now_ms);
        if (!legacy_reply_pending && !legacy_stop_pending && xQueueReceive(commands, &command, 0) == pdTRUE) {
            if (command.epoch == epoch && connected) {
                power_on();
                if (command.bytes[2] == BC_VOICE_COMMAND) {
                    (void)bc_voice_legacy_archive_cancel(&legacy);
                    bc_voice_service_receive(&service, epoch, command.bytes, command.length, now_ms);
                } else if (command.bytes[2] == CMD_PDM) legacy_record_command(&command, now_ms);
                else if (owned_legacy_control(command.bytes[2], command.bytes[3])) legacy_control(&command);
                else if (legacy_ready && !bc_recording_active(&recording) && !service.reader.open)
                    (void)bc_voice_legacy_archive_request(&legacy, command.bytes, command.length, epoch, bc_ble_payload_limit());
                else reply_legacy(&command, false);
            }
        }
        if (legacy_stop_pending && !bc_recording_active(&recording)) {
            reply_legacy(&legacy_stop, recording.snapshot.phase == BC_REC_SAVED);
            legacy_stop_pending = false;
        }
        if (legacy_reply_pending && (legacy_reply_epoch != epoch || !connected ||
            send_packet(NULL, legacy_reply, legacy_reply_length, epoch))) legacy_reply_pending = false;
        (void)bc_voice_service_poll(&service, now_ms, bc_ble_payload_limit());
        if (!bc_recording_active(&recording) && !service.reader.open)
            (void)bc_voice_legacy_archive_poll(&legacy);
        feedback();
        if (bc_recording_active(&recording) || service.reader.open ||
            bc_voice_legacy_archive_active(&legacy) || uxQueueMessagesWaiting(commands)) idle_since_ms = now_ms;
        else if (powered && (uint32_t)(now_ms - idle_since_ms) >= 500U) {
            bc_spi_flash_device_close(); powered = false;
        }
        bc_dog_feed();
        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5U));
    }
}

bool app_sudo_voice_command(const uint8_t *packet, uint16_t length, uint32_t epoch)
{
    voice_command command;
    if (!packet || length < 4 || length > sizeof(command.bytes)) return false;
    if (packet[2] != BC_VOICE_COMMAND && packet[2] != CMD_PDM && packet[2] != CMD_GET_HISTORY &&
        !owned_legacy_control(packet[2], packet[3]))
        return false;
    if (!commands || !worker) return true;
    memset(&command, 0, sizeof(command));
    memcpy(command.bytes, packet, length); command.length = length; command.epoch = epoch;
    (void)xQueueSend(commands, &command, 0);
    xTaskNotifyGive(worker);
    return true;
}

void app_pdm_thread_create(void)
{
    if (worker) return;
    commands = xQueueCreate(COMMAND_DEPTH, sizeof(voice_command));
    touches = xQueueCreate(TOUCH_DEPTH, sizeof(voice_touch));
    if (!commands || !touches) {
        if (commands) vQueueDelete(commands);
        if (touches) vQueueDelete(touches);
        commands = NULL; touches = NULL;
        return;
    }
    if (xTaskCreate(run, "sudo-voice", 2048U, NULL, APP_TASK_MIC_IRQ_PRIO, &worker) != pdPASS) {
        worker = NULL;
        vQueueDelete(commands); vQueueDelete(touches);
        commands = NULL; touches = NULL;
    }
}

enum app_pdm_mode app_pdm_mode_get(void) { return working ? PDM_MODE_OFFLINE : PDM_MODE_IDIE; }
bool app_pdm_work_status(void) { return working; }
/* Legacy public entry points enqueue; protocol ACKs are emitted only by run.
 * Internal reconnect/disconnect helpers deliberately retain local capture. */
static bool enqueue_record(bool start)
{
    uint8_t packet[5] = {0, 0, CMD_PDM, 5, 0};
    if (!initialized) return false;
    packet[4] = start;
    return app_sudo_voice_command(packet, sizeof(packet), bc_ble_session_id());
}
void app_pdm_start(struct app_cmd_package *pack) { (void)pack; (void)enqueue_record(true); }
void app_pdm_stop(struct app_cmd_package *pack) { (void)pack; (void)enqueue_record(false); }
bool app_pdm_recording_start(void) { return enqueue_record(true); }
bool app_pdm_recording_stop(void) { return enqueue_record(false); }
bool app_pdm_capture_recording_start(void) { return enqueue_record(true); }
bool app_pdm_capture_recording_stop(void) { return enqueue_record(false); }
void app_pdm_touch_start(void) { (void)enqueue_record(true); }
void app_pdm_touch_stop(void) { (void)enqueue_record(false); }
void app_pdm_ble_stop(void) { }
void app_pdm_audio_discooenct_stop(void) { }
void app_pdm_mode_change_to_online(void) { }
bool app_pdm_switch_online_to_offline(void) { return working; }

/* Health writers are not selected in Sudo Voice. All file commands are
 * intercepted before the supplier dispatcher; no second lfs_t is mounted. */
void app_ppg_file_init(void) { }
bool app_ppg_file_open(enum ppg_file_type type) { (void)type; return false; }
bool app_ppg_file_close(void) { return false; }
bool lk_app_ppg_file_open(enum ppg_file_type type) { (void)type; return false; }
bool lk_app_ppg_file_close(void) { return false; }
void app_ppg_file_write(uint8_t *data, uint32_t length) { (void)data; (void)length; }
bool app_ppg_file_delete(char *path) { (void)path; return false; }
uint8_t app_ppg_file_status_get(void) { return working ? 2U : 0U; }
static bool enqueue_file(struct app_cmd_package *pack)
{ return pack && app_sudo_voice_command((const uint8_t *)pack, pack->length, bc_ble_session_id()); }
void app_ppg_file_ls(struct app_cmd_package *pack) { (void)enqueue_file(pack); }
void app_ppg_file_format(struct app_cmd_package *pack) { (void)enqueue_file(pack); }
void app_ppg_file_sys_size_get(struct app_cmd_package *pack) { (void)enqueue_file(pack); }
bool app_ppg_file_upload(struct app_cmd_package *pack) { return enqueue_file(pack); }
bool app_ppg_file_resume_upload(struct app_cmd_package *pack) { return enqueue_file(pack); }
bool app_file_active_upload(struct app_cmd_package *pack) { return enqueue_file(pack); }
bool app_ppg_file_one_click_upload(struct app_cmd_package *pack) { (void)pack; return false; }
void app_ppg_file_upload_cancel(void)
{ uint8_t p[4] = {0, 0, CMD_GET_HISTORY, 2}; (void)app_sudo_voice_command(p, sizeof(p), bc_ble_session_id()); }
void app_ppg_list_capture_audio_up_check(void) { }
void app_ppg_file_slice_storage_timer_stop(void) { }
void app_ppg_file_slice_storage_timer_start(uint32_t time) { (void)time; }
void app_ppg_file_timeout_timer_stop(void) { }
void app_ppg_file_timeout_timer_start(uint32_t time) { (void)time; }
