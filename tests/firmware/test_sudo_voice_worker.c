#include "app_sudo_voice.h"

#include "app_sudo_capture.h"
#include "app_cmd_handler.h"
#include "app_linear_motor_handler.h"
#include "app_pdm_handler.h"
#include "bc_ble_modu_interface.h"
#include "bc_rec_store.h"
#include "bc_touch_button.h"
#include "bc_touch_report.h"
#include "bc_touch_tuning.h"
#include "bc_voice_protocol.h"
#include "bc_voice_service.h"
#include "bc_voice_wire.h"
#include "lfs_port.h"
#include "nrf_error.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FLASH_BLOCK_SIZE 256U
#define FLASH_BLOCK_COUNT 128U
#define FLASH_SIZE (FLASH_BLOCK_SIZE * FLASH_BLOCK_COUNT)
#define TEST_CACHE_SIZE 64U
#define TEST_LOOKAHEAD_SIZE (FLASH_BLOCK_COUNT / 8U)
#define MAX_MESSAGES 512U
#define MAX_PACKETS 4096U
#define MAX_QUEUE_LENGTH 16U

typedef struct {
    uint8_t bytes[FLASH_SIZE];
    unsigned prog_calls;
    unsigned read_calls;
    unsigned erase_calls;
    unsigned sync_calls;
    unsigned fail_prog_call;
    unsigned fail_erase_call;
    unsigned fail_sync_call;
} ram_nor;

typedef struct {
    lfs_t lfs;
    struct lfs_config config;
    uint8_t read_cache[TEST_CACHE_SIZE];
    uint8_t prog_cache[TEST_CACHE_SIZE];
    uint8_t lookahead[TEST_LOOKAHEAD_SIZE];
} test_fs;

typedef struct {
    uint8_t data[250];
    uint16_t length;
    uint32_t epoch;
    uint32_t session;
} sent_packet;

typedef struct {
    bc_voice_receiver receiver;
    bc_voice_message messages[MAX_MESSAGES];
    unsigned message_count;
    sent_packet packets[MAX_PACKETS];
    unsigned packet_count;
    unsigned invalid_packets;
} ble_sink;

typedef struct {
    uint8_t *items;
    unsigned length;
    unsigned item_size;
    unsigned read;
    unsigned count;
} test_queue;

typedef struct {
    bool running;
    bool stop_requested;
    bool frame_pending;
    bool tail_frame_pending;
    uint64_t id;
    uint32_t sequence;
    unsigned init_calls;
    unsigned start_calls;
    unsigned stop_calls;
    unsigned abort_calls;
    unsigned poll_calls;
    unsigned ptt_arm_calls;
    unsigned ptt_touch_calls;
    unsigned frames_to_emit;
    unsigned frames_remaining;
    bc_rec_result start_result;
    bc_rec_result stop_result;
    bool abort_quiescent;
} capture_fixture;

typedef struct {
    unsigned open_calls;
    unsigned open_checked_calls;
    unsigned close_calls;
    bool open;
    bool open_result;
    unsigned mic_on_calls;
    unsigned mic_off_calls;
    unsigned led_on_calls;
    unsigned led_off_calls;
    unsigned haptic_calls;
    unsigned motor_pulse_calls;
    uint8_t last_motor_strength;
    uint16_t last_motor_duration;
    uint8_t last_led_rgb[3];
    unsigned led_set_calls;
    unsigned led_stop_calls;
    unsigned conn_audio_set_calls;
    unsigned conn_audio_reset_calls;
    unsigned watchdog_calls;
    unsigned callback_register_calls;
    bc_touch_report_callback_t touch_callback;
    bool lights_enabled, haptics_enabled;
    unsigned settings_writes;
    bc_rec_result settings_result;
    bc_voice_settings persisted_settings;
} hardware_fixture;

typedef struct {
    uint8_t touch_thresholds[2];
    uint8_t gesture_mask[2];
    unsigned write_calls;
    unsigned read_calls;
} tuning_sensor_fixture;

typedef struct worker_fixture {
    ram_nor ram;
    test_fs fs;
    hardware_fixture hardware;
    capture_fixture capture;
    tuning_sensor_fixture tuning_sensor;
    ble_sink sink;
    TaskFunction_t worker_entry;
    TaskHandle_t worker_handle;
    test_queue *command_queue;
    test_queue *touch_queue;
    bool connected;
    uint32_t epoch;
    uint16_t att_limit;
    uint32_t random_counter;
    uint64_t ptt_id;
    uint64_t app_id;
    uint64_t second_id;
    uint64_t failed_id;
    uint64_t reopen_id;
    uint64_t reopen_retry_id;
    unsigned haptic_before_error;
    unsigned haptic_after_save;
    unsigned active_control_motor_before;
    unsigned active_control_led_set_before;
    unsigned active_control_led_stop_before;
    unsigned flash_open_calls_before;
    unsigned flash_close_calls_before;
    unsigned start_calls_before_reopen;
    uint8_t saved_settings_attr[20];
    uint8_t saved_tuning_attr[20];
    bool restart_mode;
    unsigned loop_count;
    unsigned max_loops;
    bool stop_run;
    bool fresh_ready_queued;
    unsigned script_stage;
    void (*yield_hook)(struct worker_fixture *fixture);
    jmp_buf stop_jump;
    unsigned critical_depth;
} worker_fixture;

static worker_fixture *active_fixture;
static worker_fixture *queue_fixture;
static test_fs *worker_fs;
static ram_nor *worker_ram;
static lfs_t *worker_lfs;
static unsigned checks;
static unsigned failures;

unsigned test_critical_depth;
TickType_t test_ticks;
unsigned test_notify_calls;
unsigned test_yield_calls;

static void check_condition(bool condition, const char *expression,
                            unsigned line)
{
    ++checks;
    if (!condition) {
        ++failures;
        fprintf(stderr, "FAIL line %u: %s\n", line, expression);
    }
}

#define CHECK(condition) check_condition((condition), #condition, __LINE__)

static int ram_read(const struct lfs_config *config, lfs_block_t block,
                    lfs_off_t off, void *buffer, lfs_size_t size)
{
    ram_nor *ram = (ram_nor *)config->context;
    uint64_t address = (uint64_t)block * config->block_size + off;
    ++ram->read_calls;
    if (buffer == NULL || block >= FLASH_BLOCK_COUNT ||
        off > config->block_size || size > config->block_size - off ||
        address + size > FLASH_SIZE)
        return LFS_ERR_INVAL;
    memcpy(buffer, ram->bytes + address, size);
    return LFS_ERR_OK;
}

static int ram_prog(const struct lfs_config *config, lfs_block_t block,
                    lfs_off_t off, const void *buffer, lfs_size_t size)
{
    ram_nor *ram = (ram_nor *)config->context;
    const uint8_t *source = (const uint8_t *)buffer;
    uint64_t address = (uint64_t)block * config->block_size + off;
    lfs_size_t i;
    ++ram->prog_calls;
    if (ram->fail_prog_call != 0U &&
        ram->prog_calls == ram->fail_prog_call) {
        ram->fail_prog_call = 0U;
        return LFS_ERR_IO;
    }
    if (buffer == NULL || block >= FLASH_BLOCK_COUNT ||
        off > config->block_size || size > config->block_size - off ||
        address + size > FLASH_SIZE || off % config->prog_size != 0U ||
        size % config->prog_size != 0U)
        return LFS_ERR_INVAL;
    for (i = 0U; i < size; ++i)
        if ((ram->bytes[address + i] & source[i]) != source[i])
            return LFS_ERR_CORRUPT;
    for (i = 0U; i < size; ++i)
        ram->bytes[address + i] &= source[i];
    return LFS_ERR_OK;
}

static int ram_erase(const struct lfs_config *config, lfs_block_t block)
{
    ram_nor *ram = (ram_nor *)config->context;
    ++ram->erase_calls;
    if (ram->fail_erase_call != 0U &&
        ram->erase_calls == ram->fail_erase_call) {
        ram->fail_erase_call = 0U;
        return LFS_ERR_IO;
    }
    if (block >= FLASH_BLOCK_COUNT)
        return LFS_ERR_INVAL;
    memset(ram->bytes + (size_t)block * FLASH_BLOCK_SIZE, 0xff,
           FLASH_BLOCK_SIZE);
    return LFS_ERR_OK;
}

static int ram_sync(const struct lfs_config *config)
{
    ram_nor *ram = (ram_nor *)config->context;
    ++ram->sync_calls;
    if (ram->fail_sync_call != 0U &&
        ram->sync_calls == ram->fail_sync_call) {
        ram->fail_sync_call = 0U;
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

static void fs_configure(test_fs *fs, ram_nor *ram)
{
    memset(fs, 0, sizeof(*fs));
    fs->config.context = ram;
    fs->config.read = ram_read;
    fs->config.prog = ram_prog;
    fs->config.erase = ram_erase;
    fs->config.sync = ram_sync;
    fs->config.read_size = 16U;
    fs->config.prog_size = 16U;
    fs->config.block_size = FLASH_BLOCK_SIZE;
    fs->config.block_count = FLASH_BLOCK_COUNT;
    fs->config.block_cycles = 500;
    fs->config.cache_size = TEST_CACHE_SIZE;
    fs->config.lookahead_size = TEST_LOOKAHEAD_SIZE;
    fs->config.read_buffer = fs->read_cache;
    fs->config.prog_buffer = fs->prog_cache;
    fs->config.lookahead_buffer = fs->lookahead;
    fs->config.name_max = 255U;
    fs->config.file_max = 0x7fffffffUL;
    fs->config.attr_max = 1022U;
    fs->config.inline_max = 1U;
}

static bool fs_format_mount(test_fs *fs, ram_nor *ram)
{
    int error;
    fs_configure(fs, ram);
    error = lfs_format(&fs->lfs, &fs->config);
    if (error != LFS_ERR_OK)
        return false;
    memset(&fs->lfs, 0, sizeof(fs->lfs));
    return lfs_mount(&fs->lfs, &fs->config) == LFS_ERR_OK;
}

static void queue_destroy(test_queue *queue);

static void fixture_destroy(worker_fixture *fixture)
{
    if (worker_lfs != NULL && worker_lfs->cfg != NULL)
        CHECK(lfs_unmount(worker_lfs) == LFS_ERR_OK);
    worker_lfs = NULL;
    queue_destroy(fixture->command_queue);
    queue_destroy(fixture->touch_queue);
    fixture->command_queue = NULL;
    fixture->touch_queue = NULL;
}

static test_queue *queue_create(unsigned length, unsigned item_size)
{
    test_queue *queue = (test_queue *)calloc(1U, sizeof(*queue));
    if (queue == NULL)
        return NULL;
    queue->items = (uint8_t *)calloc(length, item_size);
    if (queue->items == NULL) {
        free(queue);
        return NULL;
    }
    queue->length = length;
    queue->item_size = item_size;
    return queue;
}

static void queue_destroy(test_queue *queue)
{
    if (queue != NULL) {
        free(queue->items);
        free(queue);
    }
}

static BaseType_t queue_send(test_queue *queue, const void *item)
{
    unsigned slot;
    if (queue == NULL || item == NULL || queue->count == queue->length)
        return pdFALSE;
    slot = (queue->read + queue->count) % queue->length;
    memcpy(queue->items + slot * queue->item_size, item, queue->item_size);
    ++queue->count;
    return pdTRUE;
}

static BaseType_t queue_receive(test_queue *queue, void *item)
{
    if (queue == NULL || item == NULL || queue->count == 0U)
        return pdFALSE;
    memcpy(item, queue->items + queue->read * queue->item_size,
           queue->item_size);
    queue->read = (queue->read + 1U) % queue->length;
    --queue->count;
    return pdTRUE;
}

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t item_size)
{
    test_queue *queue = queue_create(length, item_size);
    if (queue_fixture != NULL) {
        if (queue_fixture->command_queue == NULL)
            queue_fixture->command_queue = queue;
        else if (queue_fixture->touch_queue == NULL)
            queue_fixture->touch_queue = queue;
    }
    return (QueueHandle_t)queue;
}

BaseType_t xQueueSend(QueueHandle_t handle, const void *item,
                      TickType_t ticks_to_wait)
{
    (void)ticks_to_wait;
    return queue_send((test_queue *)handle, item);
}

BaseType_t xQueueReceive(QueueHandle_t handle, void *item,
                         TickType_t ticks_to_wait)
{
    (void)ticks_to_wait;
    return queue_receive((test_queue *)handle, item);
}

UBaseType_t uxQueueMessagesWaiting(QueueHandle_t handle)
{
    test_queue *queue = (test_queue *)handle;
    return queue == NULL ? 0U : queue->count;
}

void vQueueDelete(QueueHandle_t handle)
{
    queue_destroy((test_queue *)handle);
}

BaseType_t xTaskCreate(TaskFunction_t task, const char *name,
                       uint16_t stack_depth, void *parameters,
                       UBaseType_t priority, TaskHandle_t *created)
{
    (void)name;
    (void)stack_depth;
    (void)parameters;
    (void)priority;
    if (active_fixture == NULL || task == NULL || created == NULL)
        return pdFAIL;
    active_fixture->worker_entry = task;
    active_fixture->worker_handle = active_fixture;
    *created = active_fixture->worker_handle;
    return pdPASS;
}

TickType_t xTaskGetTickCount(void)
{
    return test_ticks;
}

TickType_t xTaskGetTickCountFromISR(void)
{
    return test_ticks;
}

BaseType_t xTaskNotifyGive(TaskHandle_t task)
{
    (void)task;
    ++test_notify_calls;
    return pdTRUE;
}

void vTaskNotifyGiveFromISR(TaskHandle_t task,
                            BaseType_t *higher_priority_task_woken)
{
    (void)task;
    ++test_notify_calls;
    if (higher_priority_task_woken != NULL)
        *higher_priority_task_woken = pdFALSE;
}

uint32_t ulTaskNotifyTake(BaseType_t clear_count_on_exit,
                          TickType_t ticks_to_wait)
{
    (void)clear_count_on_exit;
    if (active_fixture == NULL)
        return 0U;
    ++active_fixture->loop_count;
    test_ticks += ticks_to_wait;
    if (active_fixture->yield_hook != NULL)
        active_fixture->yield_hook(active_fixture);
    if (active_fixture->stop_run ||
        (active_fixture->max_loops != 0U &&
         active_fixture->loop_count >= active_fixture->max_loops))
        longjmp(active_fixture->stop_jump, 1);
    return 0U;
}

void test_task_enter(void)
{
    ++test_critical_depth;
}

void test_task_exit(void)
{
    CHECK(test_critical_depth != 0U);
    if (test_critical_depth != 0U)
        --test_critical_depth;
}

static void fill_pattern(uint8_t *bytes, uint16_t length, uint8_t seed)
{
    uint16_t i;
    for (i = 0U; i < length; ++i)
        bytes[i] = (uint8_t)(seed + (uint8_t)(i * 17U));
}

static void run_worker(worker_fixture *fixture, unsigned max_loops)
{
    fixture->max_loops = max_loops;
    fixture->stop_run = false;
    fixture->loop_count = 0U;
    active_fixture = fixture;
    if (setjmp(fixture->stop_jump) == 0)
        fixture->worker_entry(NULL);
    active_fixture = NULL;
}

static void stop_worker(worker_fixture *fixture)
{
    fixture->stop_run = true;
}

/* Worker-owned filesystem port. The worker's static lfs_t is passed in, but
 * the block device is this finite RAM NOR model so every file operation is
 * still real LittleFS I/O. */
static struct lfs_config worker_config;
static uint8_t worker_read_cache[TEST_CACHE_SIZE];
static uint8_t worker_prog_cache[TEST_CACHE_SIZE];
static uint8_t worker_lookahead[TEST_LOOKAHEAD_SIZE];

const struct lfs_config cfg = {0};

int lfs_sfud_init(lfs_t *lfs)
{
    if (lfs == NULL || worker_fs == NULL || worker_ram == NULL)
        return LFS_ERR_INVAL;
    memset(&worker_config, 0, sizeof(worker_config));
    worker_config.context = worker_ram;
    worker_config.read = ram_read;
    worker_config.prog = ram_prog;
    worker_config.erase = ram_erase;
    worker_config.sync = ram_sync;
    worker_config.read_size = 16U;
    worker_config.prog_size = 16U;
    worker_config.block_size = FLASH_BLOCK_SIZE;
    worker_config.block_count = FLASH_BLOCK_COUNT;
    worker_config.block_cycles = 500;
    worker_config.cache_size = TEST_CACHE_SIZE;
    worker_config.lookahead_size = TEST_LOOKAHEAD_SIZE;
    worker_config.read_buffer = worker_read_cache;
    worker_config.prog_buffer = worker_prog_cache;
    worker_config.lookahead_buffer = worker_lookahead;
    worker_config.name_max = 255U;
    worker_config.file_max = 0x7fffffffUL;
    worker_config.attr_max = 1022U;
    worker_config.inline_max = 1U;
    worker_lfs = lfs;
    return lfs_mount(lfs, &worker_config);
}

int lfs_sfud_format(lfs_t *lfs)
{
    (void)lfs;
    return LFS_ERR_INVAL;
}

uint32_t lfs_sfud_jedec_id(void)
{
    return 0x12345678U;
}

void bc_spi_flash_device_open(void)
{
    if (active_fixture != NULL) {
        ++active_fixture->hardware.open_calls;
        active_fixture->hardware.open = true;
    }
}

bool bc_spi_flash_device_open_checked(void)
{
    if (active_fixture == NULL)
        return false;
    ++active_fixture->hardware.open_checked_calls;
    if (!active_fixture->hardware.open_result)
        return false;
    ++active_fixture->hardware.open_calls;
    active_fixture->hardware.open = true;
    return true;
}

void bc_spi_flash_device_close(void)
{
    if (active_fixture != NULL) {
        ++active_fixture->hardware.close_calls;
        active_fixture->hardware.open = false;
    }
}

void bc_spi_flash_cs_high(void) { }
void bc_spi_flash_cs_low(void) { }
bool bc_spi_flash_write_and_read(uint8_t *write_buff, uint32_t write_length,
                                 uint8_t *read_buff, uint32_t read_length)
{
    (void)write_buff;
    (void)write_length;
    (void)read_buff;
    (void)read_length;
    return false;
}
void bc_spi_flash_device_find(void) { }

bool app_ble_connect_status(void)
{
    return active_fixture != NULL && active_fixture->connected;
}

bool bc_ble_connect_status(void)
{
    return app_ble_connect_status();
}

bool app_ble_notify_allowed(void) { return true; }

uint32_t bc_ble_session_id(void)
{
    return active_fixture == NULL ? 0U : active_fixture->epoch;
}

uint16_t bc_ble_payload_limit(void)
{
    return active_fixture == NULL ? 244U : active_fixture->att_limit;
}

static void sink_packet(worker_fixture *fixture,
                        const struct bc_ble_data_package *packet,
                        uint32_t session)
{
    bc_voice_message message;
    bc_wire_result result;
    if (fixture->sink.packet_count < MAX_PACKETS) {
        sent_packet *saved = &fixture->sink.packets[fixture->sink.packet_count++];
        memcpy(saved->data, packet->data, packet->data_length);
        saved->length = packet->data_length;
        saved->epoch = session;
        saved->session = packet->session_id;
    }
    result = bc_voice_receive(&fixture->sink.receiver, session, test_ticks,
                              packet->data, packet->data_length, &message);
    if (result == BC_WIRE_MESSAGE) {
        if (fixture->sink.message_count < MAX_MESSAGES)
            fixture->sink.messages[fixture->sink.message_count++] = message;
    } else if (result == BC_WIRE_INVALID && packet->data_length >= BC_VOICE_HEADER &&
               packet->data[2] == BC_VOICE_COMMAND) {
        ++fixture->sink.invalid_packets;
    }
}

bool bc_queue_ble_send(const struct bc_ble_data_package *packet,
                       uint32_t session, uint32_t wait_ticks)
{
    (void)wait_ticks;
    if (active_fixture == NULL || packet == NULL ||
        active_fixture->sink.packet_count >= MAX_PACKETS)
        return false;
    sink_packet(active_fixture, packet, session);
    return true;
}

void app_ble_conn_time_audio_set(void)
{
    if (active_fixture != NULL)
        ++active_fixture->hardware.conn_audio_set_calls;
}

void app_ble_conn_time_audio_reset(void)
{
    if (active_fixture != NULL)
        ++active_fixture->hardware.conn_audio_reset_calls;
}

void bc_dog_feed(void)
{
    if (active_fixture != NULL)
        ++active_fixture->hardware.watchdog_calls;
}

void bc_ic_led_feedback_enable(bool enabled)
{
    if (active_fixture != NULL) active_fixture->hardware.lights_enabled = enabled;
}

void bc_linear_motor_feedback_enable(bool enabled)
{
    if (active_fixture != NULL) active_fixture->hardware.haptics_enabled = enabled;
}

bool bc_linear_motor_pulse(uint8_t strength_percent, uint16_t active_ms)
{
    if (active_fixture == NULL)
        return false;
    ++active_fixture->hardware.motor_pulse_calls;
    active_fixture->hardware.last_motor_strength = strength_percent;
    active_fixture->hardware.last_motor_duration = active_ms;
    return strength_percent >= 1U && strength_percent <= 100U &&
           active_ms >= 20U && active_ms <= 400U;
}

void bc_ic_led_set(uint8_t *rgb_data)
{
    if (active_fixture == NULL || rgb_data == NULL)
        return;
    ++active_fixture->hardware.led_set_calls;
    memcpy(active_fixture->hardware.last_led_rgb, rgb_data, 3U);
}

void bc_ic_led_stop(void)
{
    if (active_fixture != NULL)
        ++active_fixture->hardware.led_stop_calls;
}

void bc_ic_led_mic_offline_recording_on(void)
{
    if (active_fixture != NULL)
        ++active_fixture->hardware.led_on_calls;
}

void bc_ic_led_mic_offline_recording_off(void)
{
    if (active_fixture != NULL)
        ++active_fixture->hardware.led_off_calls;
}

uint8_t app_vibrate_start(vibrate_mode_t mode, uint8_t count)
{
    (void)mode;
    (void)count;
    if (active_fixture != NULL)
        ++active_fixture->hardware.haptic_calls;
    return 0U;
}

bool bc_rtc_format_beijing_time(char *buffer, size_t size)
{
    static const char timestamp[] = "20260906120000000000";
    if (buffer == NULL || size < sizeof(timestamp) - 1U)
        return false;
    memcpy(buffer, timestamp, sizeof(timestamp) - 1U);
    return true;
}

uint32_t sd_rand_application_vector_get(uint8_t *buffer, uint8_t length)
{
    uint8_t i;
    if (active_fixture == NULL || buffer == NULL || length == 0U)
        return 1U;
    ++active_fixture->random_counter;
    for (i = 0U; i < length; ++i)
        buffer[i] = (uint8_t)(active_fixture->random_counter * 29U + i + 1U);
    return NRF_SUCCESS;
}

bool bc_touch_button_touch_report_register_callback(
    bc_touch_report_callback_t callback)
{
    if (active_fixture == NULL)
        return false;
    active_fixture->hardware.touch_callback = callback;
    ++active_fixture->hardware.callback_register_calls;
    return true;
}

/* The remaining touch registration APIs are not selected by Sudo Voice. */
uint8_t bc_touch_button_chip_id_get(void) { return 0U; }
bool bc_touch_button_chip_id_hardware_check(void) { return true; }
void bc_touch_button_init(void) { }
void bc_touch_button_uninit(void) { }
void bc_touch_button_irq_process(void) { }
bool bc_touch_button_config_flag_get(void) { return false; }

static void capture_reset(capture_fixture *capture)
{
    memset(capture, 0, sizeof(*capture));
    capture->start_result = BC_REC_OK;
    capture->stop_result = BC_REC_OK;
    capture->frames_to_emit = 1U;
    capture->abort_quiescent = true;
}

void app_sudo_capture_init(TaskHandle_t worker)
{
    (void)worker;
    if (active_fixture != NULL)
        ++active_fixture->capture.init_calls;
}

void app_sudo_capture_ptt_arm(uint64_t id, uint32_t lease_ms,
                              uint32_t limit_ms)
{
    (void)lease_ms;
    (void)limit_ms;
    if (active_fixture != NULL && active_fixture->capture.id == id)
        ++active_fixture->capture.ptt_arm_calls;
}

void app_sudo_capture_ptt_touch(uint64_t id, bool valid, bool contact)
{
    if (active_fixture != NULL && active_fixture->capture.id == id) {
        ++active_fixture->capture.ptt_touch_calls;
        if (!valid || !contact)
            active_fixture->capture.stop_requested = true;
    }
}

bc_rec_result app_sudo_capture_start(void *ctx, uint64_t id)
{
    (void)ctx;
    if (active_fixture == NULL)
        return BC_REC_CAPTURE_ERROR;
    ++active_fixture->capture.start_calls;
    if (active_fixture->capture.start_result != BC_REC_OK)
        return active_fixture->capture.start_result;
    active_fixture->capture.running = true;
    active_fixture->capture.id = id;
    active_fixture->capture.sequence = 0U;
    active_fixture->capture.frames_remaining = active_fixture->capture.frames_to_emit;
    active_fixture->capture.stop_requested = false;
    active_fixture->capture.frame_pending = active_fixture->capture.frames_remaining != 0U;
    return BC_REC_OK;
}

bc_rec_result app_sudo_capture_stop(void *ctx, uint64_t id)
{
    (void)ctx;
    if (active_fixture == NULL || !active_fixture->capture.running ||
        active_fixture->capture.id != id)
        return BC_REC_WRONG_SESSION;
    ++active_fixture->capture.stop_calls;
    active_fixture->capture.stop_requested = true;
    return active_fixture->capture.stop_result;
}

bool app_sudo_capture_abort(void *ctx, uint64_t id)
{
    (void)ctx;
    if (active_fixture == NULL || active_fixture->capture.id != id)
        return false;
    ++active_fixture->capture.abort_calls;
    if (!active_fixture->capture.abort_quiescent)
        return false;
    active_fixture->capture.running = false;
    return true;
}

bool app_sudo_capture_poll(bc_recording *owner, uint32_t now_ms)
{
    capture_fixture *capture;
    uint8_t frame[BC_REC_FRAME_MAX];
    bool emitted = false;
    const bc_rec_snapshot *snapshot;
    if (active_fixture == NULL || owner == NULL)
        return false;
    capture = &active_fixture->capture;
    ++capture->poll_calls;
    snapshot = bc_recording_snapshot(owner);
    if (!capture->running || snapshot == NULL ||
        snapshot->start.id != capture->id)
        return false;
    if (capture->frame_pending ||
        (capture->stop_requested && capture->tail_frame_pending)) {
        fill_pattern(frame, sizeof(frame),
                     (uint8_t)(0x30U + capture->sequence));
        ++capture->sequence;
        (void)bc_recording_frame(owner, capture->id, capture->sequence,
                                 frame, sizeof(frame), now_ms);
        capture->frame_pending = false;
        capture->tail_frame_pending = false;
        if (capture->frames_remaining != 0U)
            --capture->frames_remaining;
        capture->frame_pending = capture->frames_remaining != 0U;
        emitted = true;
    }
    snapshot = bc_recording_snapshot(owner);
    if (capture->stop_requested && snapshot != NULL &&
        snapshot->phase == BC_REC_STOPPING && !capture->frame_pending &&
        !capture->tail_frame_pending && capture->frames_remaining == 0U &&
        capture->stop_result == BC_REC_OK) {
        capture->running = false;
        (void)bc_recording_drained(owner, capture->id);
    }
    return emitted;
}

static void init_fixture(worker_fixture *fixture)
{
    memset(fixture, 0, sizeof(*fixture));
    fixture->connected = false;
    fixture->epoch = 1U;
    fixture->att_limit = 244U;
    fixture->hardware.settings_result = BC_REC_OK;
    fixture->hardware.open_result = true;
    capture_reset(&fixture->capture);
    fixture->tuning_sensor.touch_thresholds[0] =
        BC_TOUCH_TUNING_DEFAULT_SET;
    fixture->tuning_sensor.touch_thresholds[1] =
        BC_TOUCH_TUNING_DEFAULT_CLEAR;
    fixture->tuning_sensor.gesture_mask[0] =
        (uint8_t)(BC_TOUCH_TUNING_DEFAULT_GESTURE_MASK & 0xffU);
    fixture->tuning_sensor.gesture_mask[1] =
        (uint8_t)(BC_TOUCH_TUNING_DEFAULT_GESTURE_MASK >> 8);
    memset(fixture->ram.bytes, 0xff, sizeof(fixture->ram.bytes));
    CHECK(fs_format_mount(&fixture->fs, &fixture->ram));
    /* The application mounts its own worker-owned lfs_t. Keep only the RAM
     * NOR contents from setup so the fixture never has two live LittleFS
     * handles operating on the same block device. */
    CHECK(lfs_unmount(&fixture->fs.lfs) == LFS_ERR_OK);
    memset(&fixture->fs.lfs, 0, sizeof(fixture->fs.lfs));
    worker_fs = &fixture->fs;
    worker_ram = &fixture->ram;
    queue_fixture = fixture;
    active_fixture = fixture;
    test_ticks = 0U;
    test_critical_depth = 0U;
    test_notify_calls = 0U;
    test_yield_calls = 0U;
    app_pdm_thread_create();
    queue_fixture = NULL;
    CHECK(fixture->worker_entry != NULL && fixture->command_queue != NULL &&
          fixture->touch_queue != NULL);
}

static void clear_sink(worker_fixture *fixture)
{
    memset(&fixture->sink.receiver, 0, sizeof(fixture->sink.receiver));
    fixture->sink.message_count = 0U;
    fixture->sink.packet_count = 0U;
    fixture->sink.invalid_packets = 0U;
}

static bool enqueue_command(const uint8_t *packet, uint16_t length,
                            uint32_t epoch)
{
    return app_sudo_voice_command(packet, length, epoch);
}

static unsigned count_kind(const worker_fixture *fixture, uint8_t kind)
{
    unsigned count = 0U;
    unsigned i;
    for (i = 0U; i < fixture->sink.message_count; ++i)
        if (fixture->sink.messages[i].kind == kind)
            ++count;
    return count;
}

static const bc_voice_message *first_kind(const worker_fixture *fixture,
                                          uint8_t kind)
{
    unsigned i;
    for (i = 0U; i < fixture->sink.message_count; ++i)
        if (fixture->sink.messages[i].kind == kind)
            return &fixture->sink.messages[i];
    return NULL;
}

static const bc_voice_message *find_response(const worker_fixture *fixture,
                                             uint8_t kind, uint32_t request)
{
    unsigned i;
    for (i = 0U; i < fixture->sink.message_count; ++i) {
        const bc_voice_message *message = &fixture->sink.messages[i];
        if (message->direction == BC_VOICE_RESPONSE && message->kind == kind &&
            message->length >= 5U && bc_voice_get32(message->payload) == request)
            return message;
    }
    return NULL;
}

static unsigned count_response(const worker_fixture *fixture, uint8_t kind,
                               uint32_t request)
{
    unsigned count = 0U;
    unsigned i;
    for (i = 0U; i < fixture->sink.message_count; ++i) {
        const bc_voice_message *message = &fixture->sink.messages[i];
        if (message->direction == BC_VOICE_RESPONSE && message->kind == kind &&
            message->length >= 5U && bc_voice_get32(message->payload) == request)
            ++count;
    }
    return count;
}

static const sent_packet *find_legacy_reply(const worker_fixture *fixture,
                                            const uint8_t *prefix,
                                            uint16_t length)
{
    unsigned i;
    for (i = 0U; i < fixture->sink.packet_count; ++i) {
        const sent_packet *packet = &fixture->sink.packets[i];
        if (packet->length == length && memcmp(packet->data, prefix, length) == 0)
            return packet;
    }
    return NULL;
}

static uint16_t next_wire_id;

static bool enqueue_native(uint8_t kind, uint32_t request,
                           const uint8_t *extra, uint16_t extra_length,
                           uint32_t epoch)
{
    bc_voice_message message;
    uint8_t packet[BC_VOICE_PACKET_MAX];
    uint16_t offset = 0U;
    uint16_t total;
    memset(&message, 0, sizeof(message));
    ++next_wire_id;
    if (next_wire_id == 0U)
        ++next_wire_id;
    message.message_id = next_wire_id;
    message.kind = kind;
    message.direction = BC_VOICE_REQUEST;
    bc_voice_put32(message.payload, request);
    if (extra_length != 0U)
        memcpy(message.payload + 4U, extra, extra_length);
    message.length = (uint16_t)(4U + extra_length);
    total = (uint16_t)(message.length + 4U);
    while (offset < total) {
        uint16_t next;
        uint16_t length = bc_voice_fragment(&message, offset, active_fixture->att_limit,
                                            packet, sizeof(packet), &next);
        if (length == 0U || next <= offset)
            return false;
        if (!enqueue_command(packet, length, epoch))
            return false;
        offset = next;
    }
    return true;
}

static bool enqueue_legacy(const uint8_t *packet, uint16_t length,
                           uint32_t epoch)
{
    return enqueue_command(packet, length, epoch);
}

static void encode_start(uint8_t extra[13], uint64_t id,
                         bc_rec_trigger trigger, uint32_t limit_ms)
{
    bc_voice_put64(extra, id);
    extra[8] = (uint8_t)trigger;
    bc_voice_put32(extra + 9, limit_ms);
}

static void encode_id(uint8_t extra[8], uint64_t id)
{
    bc_voice_put64(extra, id);
}

static void encode_settings(uint8_t extra[11], uint32_t ptt_ms,
                            uint32_t memo_ms, bool memo, bool led,
                            bool haptic)
{
    bc_voice_put32(extra, ptt_ms);
    bc_voice_put32(extra + 4U, memo_ms);
    extra[8] = memo ? 1U : 0U;
    extra[9] = led ? 1U : 0U;
    extra[10] = haptic ? 1U : 0U;
}

static bool settings_attr(const worker_fixture *fixture,
                          bc_voice_settings *settings)
{
    uint8_t bytes[20];
    if (worker_lfs == NULL || settings == NULL ||
        lfs_getattr(worker_lfs, "/", 0xa6U, bytes, sizeof(bytes)) !=
            (int)sizeof(bytes) || memcmp(bytes, "SVS1", 4U) != 0 ||
        bc_voice_get32(bytes + 16U) != bc_voice_crc32(bytes, 16U))
        return false;
    settings->ptt_limit_ms = bc_voice_get32(bytes + 8U);
    settings->memo_limit_ms = bc_voice_get32(bytes + 12U);
    settings->memo_enabled = (bytes[4] & 1U) != 0U;
    settings->led_enabled = (bytes[4] & 2U) != 0U;
    settings->haptic_enabled = (bytes[4] & 4U) != 0U;
    (void)fixture;
    return true;
}

static bool attr_bytes(const worker_fixture *fixture, uint8_t attribute,
                       uint8_t *bytes, size_t length)
{
    return fixture != NULL && worker_lfs != NULL && bytes != NULL &&
           lfs_getattr(worker_lfs, "/", attribute, bytes, length) ==
               (int)length;
}

static bool tuning_attr(const worker_fixture *fixture, bc_voice_tuning *value)
{
    uint8_t bytes[20];

    if (value == NULL || !attr_bytes(fixture, 0xa7U, bytes, sizeof(bytes)) ||
        memcmp(bytes, "SVT1", 4U) != 0 || bytes[7] != 0U ||
        bc_voice_get32(bytes + 12U) != 0U ||
        bc_voice_get32(bytes + 16U) != bc_voice_crc32(bytes, 16U))
        return false;
    value->touch_set = bytes[4];
    value->touch_clear = bytes[5];
    value->haptic_strength = bytes[6];
    value->start_active_ms = (uint16_t)bytes[8] |
                             ((uint16_t)bytes[9] << 8);
    value->stop_active_ms = (uint16_t)bytes[10] |
                            ((uint16_t)bytes[11] << 8);
    return true;
}

static void encode_tuning(uint8_t extra[7], uint8_t touch_set,
                          uint8_t touch_clear, uint8_t strength,
                          uint16_t start_active_ms, uint16_t stop_active_ms)
{
    extra[0] = touch_set;
    extra[1] = touch_clear;
    extra[2] = strength;
    bc_voice_put16(extra + 3U, start_active_ms);
    bc_voice_put16(extra + 5U, stop_active_ms);
}

static uint16_t get16(const uint8_t *bytes)
{
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
}

static bool worker_tuning_write(void *ctx, uint8_t reg, uint8_t *data,
                                uint8_t length)
{
    tuning_sensor_fixture *sensor = (tuning_sensor_fixture *)ctx;

    if (sensor == NULL || data == NULL || length != 2U)
        return false;
    ++sensor->write_calls;
    if (reg == BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG)
        memcpy(sensor->touch_thresholds, data, 2U);
    else if (reg == BC_TOUCH_TUNING_GESTURE_ENABLE_REG)
        memcpy(sensor->gesture_mask, data, 2U);
    else
        return false;
    return true;
}

static bool worker_tuning_read(void *ctx, uint8_t reg, uint8_t *data,
                               uint8_t length)
{
    tuning_sensor_fixture *sensor = (tuning_sensor_fixture *)ctx;

    if (sensor == NULL || data == NULL || length != 2U)
        return false;
    ++sensor->read_calls;
    if (reg == BC_TOUCH_TUNING_TOUCH_THRESHOLD_REG)
        memcpy(data, sensor->touch_thresholds, 2U);
    else if (reg == BC_TOUCH_TUNING_GESTURE_ENABLE_REG)
        memcpy(data, sensor->gesture_mask, 2U);
    else
        return false;
    return true;
}

static bool worker_tuning_snapshot_matches(uint8_t touch_set,
                                           uint8_t touch_clear,
                                           bool memo_enabled,
                                           bc_touch_tuning_status status)
{
    bc_touch_tuning_snapshot snapshot;

    if (!bc_touch_tuning_snapshot_get(&snapshot))
        return false;
    return snapshot.touch_set == touch_set &&
           snapshot.touch_clear == touch_clear &&
           snapshot.memo_enabled == memo_enabled && snapshot.status == status;
}

static bool inject_touch(worker_fixture *fixture, bool valid, bool contact,
                         bool hold, bool double_tap)
{
    bc_touch_report_t report;
    memset(&report, 0, sizeof(report));
    report.valid = valid;
    report.contact = contact;
    report.hold = hold;
    report.double_tap = double_tap;
    if (fixture->hardware.touch_callback == NULL)
        return false;
    fixture->hardware.touch_callback(&report);
    return true;
}

static uint32_t crc32_frames(unsigned frame_count)
{
    uint32_t crc = 0xffffffffU;
    unsigned frame;
    for (frame = 0U; frame < frame_count; ++frame) {
        uint32_t i;
        for (i = 0U; i < BC_REC_FRAME_MAX; ++i) {
            unsigned bit;
            uint8_t byte = (uint8_t)(0x30U + (uint8_t)frame +
                                     (uint8_t)(i * 17U));
            crc ^= byte;
            for (bit = 0U; bit < 8U; ++bit)
                crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
        }
    }
    return crc ^ 0xffffffffU;
}

static void restart_script(worker_fixture *fixture)
{
    const bc_voice_message *response;

    switch (fixture->script_stage) {
    case 42:
        if (fixture->loop_count == 1U) {
            clear_sink(fixture);
            CHECK(enqueue_native(BC_VOICE_TUNING_GET, 132U, NULL, 0U,
                                 fixture->epoch));
            CHECK(enqueue_native(BC_VOICE_SETTINGS_GET, 133U, NULL, 0U,
                                 fixture->epoch));
            fixture->script_stage = 43U;
        }
        break;
    case 43:
        response = find_response(fixture, BC_VOICE_TUNING_GET, 132U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->length == 13U);
            CHECK(response->payload[5] == 70U && response->payload[6] == 65U &&
                  response->payload[7] == 80U);
            CHECK(get16(response->payload + 8U) == 240U &&
                  get16(response->payload + 10U) == 360U);
            CHECK(response->payload[12] == BC_TOUCH_TUNING_PENDING);
            CHECK(worker_tuning_snapshot_matches(70U, 65U, true,
                                                 BC_TOUCH_TUNING_PENDING));
            fixture->script_stage = 44U;
        }
        break;
    case 44:
        response = find_response(fixture, BC_VOICE_SETTINGS_GET, 133U);
        if (response != NULL) {
            CHECK(fixture->hardware.lights_enabled && fixture->hardware.haptics_enabled);
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->length == 16U);
            CHECK(bc_voice_get32(response->payload + 5U) == 1234U &&
                  bc_voice_get32(response->payload + 9U) == 2345U);
            CHECK(response->payload[13] == 1U && response->payload[14] == 1U &&
                  response->payload[15] == 1U);
            CHECK(attr_bytes(fixture, 0xa6U, fixture->saved_settings_attr,
                             sizeof(fixture->saved_settings_attr)));
            CHECK(attr_bytes(fixture, 0xa7U, fixture->saved_tuning_attr,
                             sizeof(fixture->saved_tuning_attr)));
            CHECK(worker_tuning_snapshot_matches(70U, 65U, true,
                                                 BC_TOUCH_TUNING_PENDING));
            CHECK(fixture->hardware.open);
            fixture->flash_open_calls_before = fixture->hardware.open_calls;
            fixture->start_calls_before_reopen = fixture->capture.start_calls;
            fixture->reopen_id = UINT64_C(0x7766554433221100);
            fixture->reopen_retry_id = UINT64_C(0x66554433221100ff);
            fixture->capture.frames_to_emit = 1U;
            fixture->capture.start_result = BC_REC_OK;
            fixture->capture.stop_result = BC_REC_OK;
            fixture->capture.abort_quiescent = true;
            /* Keep the worker alive until its actual 500 ms idle policy closes
             * Flash, then exercise a failed cold open through the command
             * path. */
            fixture->script_stage = 45U;
        }
        break;
    case 45:
        if (!fixture->hardware.open) {
            CHECK(fixture->hardware.close_calls != 0U);
            fixture->hardware.open_result = false;
            {
                uint8_t start[13];
                encode_start(start, fixture->reopen_id, BC_REC_APP, 0U);
                CHECK(enqueue_native(BC_VOICE_START, 134U, start, 13U,
                                     fixture->epoch));
            }
            fixture->script_stage = 46U;
        }
        break;
    case 46:
        response = find_response(fixture, BC_VOICE_START, 134U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_OPEN_ERROR);
            CHECK(fixture->capture.start_calls ==
                  fixture->start_calls_before_reopen);
            CHECK(!fixture->capture.running);
            CHECK(!fixture->hardware.open);
            CHECK(fixture->hardware.open_calls ==
                  fixture->flash_open_calls_before);
            CHECK(fixture->hardware.open_checked_calls != 0U);
            fixture->hardware.open_result = true;
            {
                uint8_t start[13];
                encode_start(start, fixture->reopen_retry_id, BC_REC_APP, 0U);
                CHECK(enqueue_native(BC_VOICE_START, 135U, start, 13U,
                                     fixture->epoch));
            }
            fixture->script_stage = 47U;
        }
        break;
    case 47:
        response = find_response(fixture, BC_VOICE_START, 135U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->payload[14] == BC_REC_RECORDING);
            CHECK(fixture->capture.start_calls ==
                  fixture->start_calls_before_reopen + 1U);
            CHECK(fixture->capture.id == fixture->reopen_retry_id);
            CHECK(fixture->capture.running);
            CHECK(fixture->hardware.open);
            {
                uint8_t id_bytes[8];
                encode_id(id_bytes, fixture->reopen_retry_id);
                CHECK(enqueue_native(BC_VOICE_STOP, 136U, id_bytes, 8U,
                                     fixture->epoch));
            }
            fixture->script_stage = 48U;
        }
        break;
    case 48:
        response = find_response(fixture, BC_VOICE_STOP, 136U);
        if (response != NULL && !app_pdm_work_status()) {
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->payload[14] == BC_REC_SAVED);
            CHECK(bc_voice_get32(response->payload + 37U) == BC_REC_FRAME_MAX);
            CHECK(bc_voice_get32(response->payload + 41U) == 1U);
            {
                uint8_t id_bytes[8];
                encode_id(id_bytes, fixture->reopen_id);
                CHECK(enqueue_native(BC_VOICE_QUERY, 137U, id_bytes, 8U,
                                     fixture->epoch));
            }
            fixture->script_stage = 49U;
        }
        break;
    case 49:
        response = find_response(fixture, BC_VOICE_QUERY, 137U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_NOT_FOUND);
            CHECK(fixture->capture.start_calls ==
                  fixture->start_calls_before_reopen + 1U);
            {
                uint8_t id_bytes[8];
                encode_id(id_bytes, fixture->app_id);
                CHECK(enqueue_native(BC_VOICE_QUERY, 138U, id_bytes, 8U,
                                     fixture->epoch));
            }
            fixture->script_stage = 50U;
        }
        break;
    case 50:
        response = find_response(fixture, BC_VOICE_QUERY, 138U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->payload[14] == BC_REC_DELIVERED);
            CHECK(bc_voice_get32(response->payload + 37U) ==
                  6U * BC_REC_FRAME_MAX);
            CHECK(bc_voice_get32(response->payload + 41U) == 6U);
            stop_worker(fixture);
            fixture->script_stage = 51U;
        }
        break;
    default:
        stop_worker(fixture);
        break;
    }
}

static void worker_script(worker_fixture *fixture)
{
    uint8_t extra[17];
    const bc_voice_message *response;
    uint64_t id;
    unsigned i;

    if (fixture->restart_mode) {
        restart_script(fixture);
        return;
    }

    switch (fixture->script_stage) {
    case 0:
        /* Fresh devices ignore accidental double taps. Validate this through
         * the actual worker before proving hold/release still works offline. */
        if (fixture->loop_count == 1U && fixture->hardware.touch_callback != NULL) {
            CHECK(worker_tuning_snapshot_matches(54U, 52U, false,
                                                 BC_TOUCH_TUNING_PENDING));
            CHECK(fixture->hardware.lights_enabled && fixture->hardware.haptics_enabled);
            CHECK(inject_touch(fixture, true, false, false, true));
            fixture->script_stage = 52U;
        }
        break;
    case 52:
        CHECK(fixture->capture.start_calls == 0U);
        CHECK(!app_pdm_work_status());
        CHECK(fixture->hardware.motor_pulse_calls == 0U);
        CHECK(fixture->hardware.led_on_calls == 0U);
        CHECK(inject_touch(fixture, true, true, true, false));
        fixture->script_stage = 1U;
        break;
    case 1:
        /* Release arrives while the first DMA block is still pending. The
         * next poll must append that block before the owner is drained. */
        if (fixture->capture.start_calls == 1U) {
            fixture->ptt_id = fixture->capture.id;
            CHECK(fixture->ptt_id != 0U);
            fixture->connected = true;
            fixture->epoch = 9U;
            app_pdm_ble_stop();
            app_pdm_audio_discooenct_stop();
            CHECK(fixture->capture.running);
            CHECK(inject_touch(fixture, true, false, false, false));
            fixture->script_stage = 2U;
        }
        break;
    case 2:
        if (fixture->capture.stop_calls == 1U && !fixture->capture.running) {
            CHECK(!app_pdm_work_status());
            CHECK(fixture->capture.sequence == 1U);
            CHECK(fixture->hardware.open);
            CHECK(fixture->hardware.led_on_calls != 0U);
            CHECK(fixture->hardware.led_off_calls != 0U);
            CHECK(fixture->hardware.motor_pulse_calls >= 2U);
            CHECK(fixture->hardware.last_motor_strength == 100U);
            CHECK(fixture->hardware.last_motor_duration == 280U);
            /* These vendor reconnect helpers are intentionally inert for a
             * local owner. Move to a connected session only after the PTT
             * has finished, then exercise the native framed path. */
            app_pdm_ble_stop();
            app_pdm_audio_discooenct_stop();
            CHECK(!app_pdm_work_status());
            fixture->connected = true;
            fixture->epoch = 2U;
            fixture->att_limit = 20U;
            clear_sink(fixture);
            CHECK(enqueue_native(BC_VOICE_HELLO, 100U, NULL, 0U,
                                 fixture->epoch));
            fixture->script_stage = 3U;
        }
        break;
    case 3:
        response = find_response(fixture, BC_VOICE_HELLO, 100U);
        if (response != NULL) {
            uint8_t enabled = 1U;
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->length == 20U);
            CHECK(enqueue_native(BC_VOICE_READY, 101U, &enabled, 1U,
                                 fixture->epoch));
            fixture->script_stage = 4U;
        }
        break;
    case 4:
        response = find_response(fixture, BC_VOICE_READY, 101U);
        if (response != NULL) {
            uint8_t flash_id_command[] = {0U, 0U, CMD_TOOL_TEST, 40U};
            /* The offline PTT already accepted audio before this link/epoch
             * change. READY cannot promise a contiguous live prefix from the
             * old connection, so the service disables that preview until a
             * new recording establishes a fresh token. */
            CHECK(response->payload[4] == BC_REC_INTERRUPTED);
            CHECK(response->length == 5U);
            CHECK(count_kind(fixture, BC_VOICE_LIVE) == 0U);
            CHECK(fixture->hardware.conn_audio_set_calls != 0U);
            fixture->flash_open_calls_before = fixture->hardware.open_calls;
            fixture->flash_close_calls_before = fixture->hardware.close_calls;
            CHECK(enqueue_legacy(flash_id_command, sizeof(flash_id_command),
                                 fixture->epoch));
            fixture->capture.frames_to_emit = 6U;
            fixture->app_id = UINT64_C(0x1122334455667788);
            encode_start(extra, fixture->app_id, BC_REC_APP, 0U);
            CHECK(enqueue_native(BC_VOICE_START, 102U, extra, 13U,
                                 fixture->epoch));
            fixture->script_stage = 5U;
        }
        break;
    case 5:
        if (fixture->capture.start_calls == 2U) {
            uint8_t flash_id_reply[] = {0U, 0U, CMD_TOOL_TEST, 40U,
                                        0x78U, 0x56U, 0x34U, 0x12U};
            uint8_t enabled = 1U;
            uint8_t led_off[] = {0U, 0U, CMD_LED, 7U};
            uint8_t motor_pulse[] = {0U, 0U, CMD_MOTOR, 4U, 1U};
            uint8_t touch_test[] = {0U, 0U, CMD_TOOL_TEST, 37U};
            uint8_t ship_mode[] = {0U, 0U, CMD_TOOL_TEST, 5U};
            uint8_t reboot[] = {0U, 0U, CMD_TOOL_TEST, 9U};
            CHECK(fixture->capture.id == fixture->app_id);
            CHECK(app_pdm_work_status());
            CHECK(find_legacy_reply(fixture, flash_id_reply,
                                     sizeof(flash_id_reply)) != NULL);
            CHECK(fixture->hardware.open_calls ==
                  fixture->flash_open_calls_before);
            CHECK(fixture->hardware.close_calls ==
                  fixture->flash_close_calls_before);
            fixture->active_control_motor_before =
                fixture->hardware.motor_pulse_calls;
            fixture->active_control_led_set_before =
                fixture->hardware.led_set_calls;
            fixture->active_control_led_stop_before =
                fixture->hardware.led_stop_calls;
            /* Start 102 has admitted a new recording and therefore cleared
             * the prior interrupted prefix fence. Send READY in a separate
             * worker turn so its two ATT20 fragments cannot be lost while
             * the following duplicate and legacy-control pressure fills the
             * bounded command queue. */
            if (!fixture->fresh_ready_queued) {
                CHECK(enqueue_native(BC_VOICE_READY, 139U, &enabled, 1U,
                                     fixture->epoch));
                fixture->fresh_ready_queued = true;
                break;
            }
            if (find_response(fixture, BC_VOICE_READY, 139U) == NULL)
                break;
            encode_start(extra, fixture->app_id, BC_REC_APP, 0U);
            /* Same recording ID and exact parameters is an idempotent Start;
             * it must not open storage or start capture again. */
            CHECK(enqueue_native(BC_VOICE_START, 102U, extra, 13U,
                                 fixture->epoch));
            encode_start(extra, fixture->app_id, BC_REC_MEMO, 0U);
            CHECK(enqueue_native(BC_VOICE_START, 103U, extra, 13U,
                                 fixture->epoch));
            CHECK(enqueue_legacy(led_off, sizeof(led_off), fixture->epoch));
            CHECK(enqueue_legacy(motor_pulse, sizeof(motor_pulse),
                                 fixture->epoch));
            CHECK(enqueue_legacy(touch_test, sizeof(touch_test),
                                 fixture->epoch));
            CHECK(enqueue_legacy(ship_mode, sizeof(ship_mode),
                                 fixture->epoch));
            CHECK(enqueue_legacy(reboot, sizeof(reboot), fixture->epoch));
            fixture->script_stage = 6U;
        }
        break;
    case 6:
        response = find_response(fixture, BC_VOICE_START, 103U);
        {
            const bc_voice_message *ready =
                find_response(fixture, BC_VOICE_READY, 139U);
            if (response == NULL || ready == NULL)
                break;
            CHECK(ready->payload[4] == BC_REC_OK);
            CHECK(ready->length == 17U);
            CHECK(response->payload[4] == BC_REC_INVALID);
            CHECK(fixture->capture.start_calls == 2U);
            encode_id(extra, fixture->app_id);
            CHECK(enqueue_native(BC_VOICE_STOP, 104U, extra, 8U,
                                 fixture->epoch));
            /* A second Stop while the first is draining is a stable BUSY
             * response and cannot replace the pending request. */
            CHECK(enqueue_native(BC_VOICE_STOP, 105U, extra, 8U,
                                 fixture->epoch));
            fixture->script_stage = 7U;
        }
        break;
    case 7:
        if (fixture->capture.stop_calls == 2U) {
            uint8_t led_off_reply[] = {0U, 0U, CMD_LED, 7U, 0U};
            uint8_t motor_pulse_reply[] = {0U, 0U, CMD_MOTOR, 4U, 0U};
            uint8_t touch_test_reply[] = {0U, 0U, CMD_TOOL_TEST, 37U, 0U};
            uint8_t ship_mode_reply[] = {0U, 0U, CMD_TOOL_TEST, 5U, 0U};
            uint8_t reboot_reply[] = {0U, 0U, CMD_TOOL_TEST, 9U, 0U};
            /* Legacy replies are worker-owned and can trail the initial Stop
             * dispatch under the bounded control queue. Wait until all five
             * replies have actually reached the sink before asserting their
             * contents, rather than sampling an intermediate iteration. */
            if (find_legacy_reply(fixture, led_off_reply,
                                  sizeof(led_off_reply)) == NULL ||
                find_legacy_reply(fixture, motor_pulse_reply,
                                  sizeof(motor_pulse_reply)) == NULL ||
                find_legacy_reply(fixture, touch_test_reply,
                                  sizeof(touch_test_reply)) == NULL ||
                find_legacy_reply(fixture, ship_mode_reply,
                                  sizeof(ship_mode_reply)) == NULL ||
                find_legacy_reply(fixture, reboot_reply,
                                  sizeof(reboot_reply)) == NULL)
                break;
            CHECK(find_response(fixture, BC_VOICE_STOP, 104U) == NULL);
            CHECK(find_legacy_reply(fixture, led_off_reply,
                                    sizeof(led_off_reply)) != NULL);
            CHECK(find_legacy_reply(fixture, motor_pulse_reply,
                                    sizeof(motor_pulse_reply)) != NULL);
            CHECK(find_legacy_reply(fixture, touch_test_reply,
                                    sizeof(touch_test_reply)) != NULL);
            CHECK(find_legacy_reply(fixture, ship_mode_reply,
                                    sizeof(ship_mode_reply)) != NULL);
            CHECK(find_legacy_reply(fixture, reboot_reply,
                                    sizeof(reboot_reply)) != NULL);
            CHECK(fixture->hardware.motor_pulse_calls ==
                  fixture->active_control_motor_before);
            CHECK(fixture->hardware.led_set_calls ==
                  fixture->active_control_led_set_before);
            CHECK(fixture->hardware.led_stop_calls ==
                  fixture->active_control_led_stop_before);
            CHECK(fixture->capture.running);
            fixture->script_stage = 8U;
        }
        break;
    case 8:
        response = find_response(fixture, BC_VOICE_STOP, 104U);
        if (response != NULL && !app_pdm_work_status()) {
            uint32_t bytes = bc_voice_get32(response->payload + 37U);
            uint32_t frames = bc_voice_get32(response->payload + 41U);
            uint32_t crc = bc_voice_get32(response->payload + 45U);
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->payload[14] == BC_REC_SAVED);
            CHECK(bytes == 6U * BC_REC_FRAME_MAX && frames == 6U);
            CHECK(crc == crc32_frames(6U));
            CHECK(bc_voice_get32(response->payload + 25U) == bytes);
            CHECK(bc_voice_get32(response->payload + 29U) == frames);
            CHECK(fixture->capture.start_calls == 2U);
            fixture->haptic_after_save = fixture->hardware.motor_pulse_calls;
            encode_id(extra, fixture->app_id);
            CHECK(enqueue_native(BC_VOICE_QUERY, 106U, extra, 8U,
                                 fixture->epoch));
            fixture->script_stage = 9U;
        }
        break;
    case 9:
        response = find_response(fixture, BC_VOICE_QUERY, 106U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->payload[14] == BC_REC_SAVED);
            CHECK(bc_voice_get32(response->payload + 37U) ==
                  6U * BC_REC_FRAME_MAX);
            CHECK(find_response(fixture, BC_VOICE_STOP, 105U) != NULL);
            /* Exact custody is committed before the owner snapshot changes. */
            encode_id(extra, fixture->app_id);
            bc_voice_put32(extra + 8U, 6U * BC_REC_FRAME_MAX);
            bc_voice_put32(extra + 12U, bc_voice_get32(response->payload + 45U));
            extra[16] = 0U;
            CHECK(enqueue_native(BC_VOICE_RECEIPT, 107U, extra, 17U,
                                 fixture->epoch));
            fixture->script_stage = 10U;
        }
        break;
    case 10:
        response = find_response(fixture, BC_VOICE_RECEIPT, 107U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(count_response(fixture, BC_VOICE_RECEIPT, 107U) == 1U);
            encode_id(extra, fixture->app_id);
            CHECK(enqueue_native(BC_VOICE_QUERY, 108U, extra, 8U,
                                 fixture->epoch));
            fixture->script_stage = 11U;
        }
        break;
    case 11:
        response = find_response(fixture, BC_VOICE_QUERY, 108U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->payload[14] == BC_REC_DELIVERED);
            CHECK((response->payload[16] & 4U) != 0U);
            /* A receipt is metadata custody, not a second completion edge. */
            CHECK(fixture->hardware.motor_pulse_calls == fixture->haptic_after_save);
            encode_id(extra, fixture->app_id);
            bc_voice_put32(extra + 8U, 0U);
            bc_voice_put32(extra + 12U, 1U);
            CHECK(enqueue_native(BC_VOICE_RESUME, 109U, extra, 16U,
                                 fixture->epoch));
            fixture->script_stage = 12U;
        }
        break;
    case 12:
        response = find_response(fixture, BC_VOICE_RESUME, 109U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->length == 29U);
            CHECK(count_kind(fixture, BC_VOICE_LIVE) >= 1U);
            {
                const bc_voice_message *live = first_kind(fixture, BC_VOICE_LIVE);
                CHECK(live != NULL && live->length == 8U + BC_REC_FRAME_MAX);
                CHECK(live != NULL && bc_voice_get32(live->payload + 4U) == 1U);
                CHECK(live != NULL && live->payload[8U] == 0x30U);
            }
            /* The reader remains open while file packets are in flight. A
             * new recording must cancel that archive before opening itself. */
            fixture->capture.frames_to_emit = 1U;
            fixture->second_id = UINT64_C(0x8877665544332211);
            encode_start(extra, fixture->second_id, BC_REC_APP, 0U);
            CHECK(enqueue_native(BC_VOICE_START, 110U, extra, 13U,
                                 fixture->epoch));
            fixture->script_stage = 13U;
        }
        break;
    case 13:
        if (fixture->capture.start_calls == 3U) {
            CHECK(app_pdm_work_status());
            CHECK(fixture->hardware.close_calls == 0U);
            encode_id(extra, fixture->second_id);
            CHECK(enqueue_native(BC_VOICE_STOP, 111U, extra, 8U,
                                 fixture->epoch));
            fixture->script_stage = 14U;
        }
        break;
    case 14:
        response = find_response(fixture, BC_VOICE_STOP, 111U);
        if (response != NULL && !app_pdm_work_status()) {
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->payload[14] == BC_REC_SAVED);
            /* Commands from the previous BLE epoch are discarded by the
             * worker and cannot start a new local owner. */
            fixture->epoch = 3U;
            encode_start(extra, UINT64_C(0x9988776655443322), BC_REC_APP, 0U);
            CHECK(enqueue_native(BC_VOICE_START, 112U, extra, 13U, 2U));
            fixture->script_stage = 15U;
        }
        break;
    case 15:
        if (fixture->loop_count > 0U && fixture->capture.start_calls == 3U) {
            CHECK(!app_pdm_work_status());
            fixture->capture.frames_to_emit = 1U;
            {
                uint8_t legacy_start[] = {0U, 0U, CMD_PDM, 5U, 1U};
                CHECK(enqueue_legacy(legacy_start, sizeof(legacy_start),
                                     fixture->epoch));
                CHECK(enqueue_legacy(legacy_start, sizeof(legacy_start),
                                     fixture->epoch));
            }
            fixture->script_stage = 16U;
        }
        break;
    case 16:
        if (fixture->capture.start_calls == 4U) {
            CHECK(app_pdm_work_status());
            CHECK(fixture->capture.id != fixture->second_id);
            {
                uint8_t legacy_stop[] = {0U, 0U, CMD_PDM, 5U, 0U};
                CHECK(enqueue_legacy(legacy_stop, sizeof(legacy_stop),
                                     fixture->epoch));
                CHECK(enqueue_legacy(legacy_stop, sizeof(legacy_stop),
                                     fixture->epoch));
            }
            fixture->script_stage = 17U;
        }
        break;
    case 17:
        if (fixture->capture.stop_calls == 4U && !app_pdm_work_status()) {
            uint8_t start_reply[] = {0U, 0U, CMD_PDM, 5U, 1U};
            uint8_t stop_reply[] = {0U, 0U, CMD_PDM, 5U, 1U};
            CHECK(find_legacy_reply(fixture, start_reply, sizeof(start_reply)) != NULL);
            CHECK(find_legacy_reply(fixture, stop_reply, sizeof(stop_reply)) != NULL);
            /* Release clears the hold gate; the next hold is a fresh single
             * start attempt. Flood its report queue to exercise overflow. */
            CHECK(inject_touch(fixture, true, false, false, false));
            fixture->script_stage = 18U;
        }
        break;
    case 18:
        if (fixture->loop_count != 0U && fixture->capture.start_calls == 4U) {
            CHECK(inject_touch(fixture, true, true, true, false));
            fixture->script_stage = 19U;
        }
        break;
    case 19:
        if (fixture->capture.start_calls == 5U) {
            for (i = 0U; i < 13U; ++i)
                CHECK(inject_touch(fixture, true, true, false, false));
            fixture->script_stage = 20U;
        }
        break;
    case 20:
        if (!app_pdm_work_status() && fixture->capture.stop_calls == 5U) {
            CHECK(fixture->capture.abort_calls == 0U);
            CHECK(fixture->hardware.open);
            CHECK(inject_touch(fixture, true, false, false, false));
            /* 600 ticks is over the owner's 500 ms abort deadline while
             * staying below the wire assembler's 1000 ms expiry. */
            test_ticks += 600U;
            fixture->script_stage = 21U;
        }
        break;
    case 21:
        if (!fixture->hardware.open) {
            CHECK(fixture->hardware.close_calls != 0U);
            encode_settings(extra, BC_REC_MAX_INTERVAL + 1U, 2345U,
                            true, true, true);
            CHECK(enqueue_native(BC_VOICE_SETTINGS_SET, 113U, extra, 11U,
                                 fixture->epoch));
            fixture->script_stage = 22U;
        }
        break;
    case 22:
        response = find_response(fixture, BC_VOICE_SETTINGS_SET, 113U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_INVALID);
            encode_settings(extra, 1234U, 2345U, false, true, true);
            CHECK(enqueue_native(BC_VOICE_SETTINGS_SET, 114U, extra, 11U,
                                 fixture->epoch));
            fixture->script_stage = 23U;
        }
        break;
    case 23:
        response = find_response(fixture, BC_VOICE_SETTINGS_SET, 114U);
        if (response != NULL) {
            bc_voice_settings persisted;
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(settings_attr(fixture, &persisted));
            CHECK(persisted.ptt_limit_ms == 1234U &&
                  persisted.memo_limit_ms == 2345U &&
                  !persisted.memo_enabled && persisted.led_enabled &&
                  persisted.haptic_enabled);
            CHECK(enqueue_native(BC_VOICE_SETTINGS_GET, 115U, NULL, 0U,
                                 fixture->epoch));
            fixture->script_stage = 24U;
        }
        break;
    case 24:
        response = find_response(fixture, BC_VOICE_SETTINGS_GET, 115U);
        if (response != NULL) {
            fixture->haptic_before_error = fixture->hardware.motor_pulse_calls;
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(bc_voice_get32(response->payload + 5U) == 1234U &&
                  bc_voice_get32(response->payload + 9U) == 2345U &&
                  response->payload[13] == 0U && response->payload[14] == 1U &&
                  response->payload[15] == 1U);
            fixture->failed_id = UINT64_C(0xaabbccddeeff0011);
            fixture->capture.start_result = BC_REC_CAPTURE_ERROR;
            encode_start(extra, fixture->failed_id, BC_REC_APP, 0U);
            CHECK(enqueue_native(BC_VOICE_START, 116U, extra, 13U,
                                 fixture->epoch));
            fixture->script_stage = 25U;
        }
        break;
    case 25:
        response = find_response(fixture, BC_VOICE_START, 116U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_CAPTURE_ERROR);
            CHECK(!app_pdm_work_status());
            CHECK(fixture->capture.start_calls == 6U);
            CHECK(fixture->hardware.motor_pulse_calls > fixture->haptic_before_error);
            CHECK(fixture->hardware.last_motor_strength == 100U);
            CHECK(fixture->hardware.last_motor_duration == 400U);
            fixture->capture.start_result = BC_REC_OK;
            encode_start(extra, fixture->failed_id, BC_REC_APP, 0U);
            CHECK(enqueue_native(BC_VOICE_START, 117U, extra, 13U,
                                 fixture->epoch));
            fixture->script_stage = 26U;
        }
        break;
    case 26:
        response = find_response(fixture, BC_VOICE_START, 117U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_CAPTURE_ERROR);
            CHECK(fixture->capture.start_calls == 6U);
            fixture->ram.fail_prog_call = fixture->ram.prog_calls + 1U;
            id = UINT64_C(0xbbccddeeff001122);
            encode_start(extra, id, BC_REC_APP, 0U);
            CHECK(enqueue_native(BC_VOICE_START, 118U, extra, 13U,
                                 fixture->epoch));
            fixture->script_stage = 27U;
        }
        break;
    case 27:
        response = find_response(fixture, BC_VOICE_START, 118U);
        if (response != NULL) {
            CHECK(response->payload[4] != BC_REC_OK);
            CHECK(fixture->capture.start_calls == 6U);
            fixture->ram.fail_prog_call = 0U;
            fixture->capture.frames_to_emit = 1U;
            fixture->capture.stop_result = BC_REC_CAPTURE_ERROR;
            fixture->capture.abort_quiescent = false;
            fixture->failed_id = UINT64_C(0xccddeeff00112233);
            encode_start(extra, fixture->failed_id, BC_REC_APP, 0U);
            CHECK(enqueue_native(BC_VOICE_START, 119U, extra, 13U,
                                 fixture->epoch));
            fixture->script_stage = 28U;
        }
        break;
    case 28:
        if (fixture->capture.start_calls == 7U) {
            encode_id(extra, fixture->failed_id);
            CHECK(enqueue_native(BC_VOICE_STOP, 120U, extra, 8U,
                                 fixture->epoch));
            fixture->script_stage = 29U;
        }
        break;
    case 29:
        if (fixture->capture.stop_calls == 6U && app_pdm_work_status()) {
            CHECK(fixture->capture.running);
            id = UINT64_C(0xddeeff0011223344);
            encode_start(extra, id, BC_REC_APP, 0U);
            CHECK(enqueue_native(BC_VOICE_START, 121U, extra, 13U,
                                 fixture->epoch));
            fixture->script_stage = 30U;
        }
        break;
    case 30:
        response = find_response(fixture, BC_VOICE_START, 121U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_BUSY);
            CHECK(fixture->capture.start_calls == 7U);
            CHECK(find_response(fixture, BC_VOICE_STOP, 120U) == NULL);
            fixture->capture.abort_quiescent = true;
            test_ticks += 1024U;
            fixture->script_stage = 31U;
        }
        break;
    case 31:
        response = find_response(fixture, BC_VOICE_STOP, 120U);
        if (response != NULL && !app_pdm_work_status()) {
            CHECK(fixture->capture.abort_calls != 0U);
            CHECK(response->payload[4] == BC_REC_CAPTURE_ERROR);
            CHECK(response->payload[14] == BC_REC_PARTIAL);
            CHECK(response->payload[14] != BC_REC_SAVED);
            CHECK(enqueue_native(BC_VOICE_TUNING_GET, 122U, NULL, 0U,
                                 fixture->epoch));
            fixture->script_stage = 32U;
        }
        break;
    case 32:
        response = find_response(fixture, BC_VOICE_TUNING_GET, 122U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->length == 13U);
            CHECK(response->payload[5] == 54U && response->payload[6] == 52U &&
                  response->payload[7] == 100U);
            CHECK(get16(response->payload + 8U) == 120U &&
                  get16(response->payload + 10U) == 280U);
            CHECK(response->payload[12] == BC_TOUCH_TUNING_PENDING);
            CHECK(worker_tuning_snapshot_matches(54U, 52U, false,
                                                 BC_TOUCH_TUNING_PENDING));
            encode_tuning(extra, 64U, 60U, 75U, 200U, 340U);
            CHECK(enqueue_native(BC_VOICE_TUNING_SET, 123U, extra, 7U,
                                 fixture->epoch));
            fixture->script_stage = 33U;
        }
        break;
    case 33:
        response = find_response(fixture, BC_VOICE_TUNING_SET, 123U);
        if (response != NULL) {
            bc_voice_tuning persisted;
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->payload[5] == 64U && response->payload[6] == 60U &&
                  response->payload[7] == 75U);
            CHECK(get16(response->payload + 8U) == 200U &&
                  get16(response->payload + 10U) == 340U);
            CHECK(response->payload[12] == BC_TOUCH_TUNING_PENDING);
            CHECK(tuning_attr(fixture, &persisted));
            CHECK(persisted.touch_set == 64U && persisted.touch_clear == 60U &&
                  persisted.haptic_strength == 75U &&
                  persisted.start_active_ms == 200U &&
                  persisted.stop_active_ms == 340U);
            CHECK(attr_bytes(fixture, 0xa7U, fixture->saved_tuning_attr,
                             sizeof(fixture->saved_tuning_attr)));
            CHECK(worker_tuning_snapshot_matches(64U, 60U, false,
                                                 BC_TOUCH_TUNING_PENDING));
            bc_touch_tuning_on_sample(true, false, false,
                                      worker_tuning_write,
                                      worker_tuning_read,
                                      &fixture->tuning_sensor);
            CHECK(fixture->tuning_sensor.write_calls == 2U &&
                  fixture->tuning_sensor.read_calls == 2U);
            CHECK(fixture->tuning_sensor.touch_thresholds[0] == 64U &&
                  fixture->tuning_sensor.touch_thresholds[1] == 60U);
            CHECK(fixture->tuning_sensor.gesture_mask[0] ==
                      BC_TOUCH_TUNING_GESTURE_HOLD &&
                  fixture->tuning_sensor.gesture_mask[1] == 0U);
            CHECK(worker_tuning_snapshot_matches(64U, 60U, false,
                                                 BC_TOUCH_TUNING_APPLIED));
            CHECK(enqueue_native(BC_VOICE_TUNING_GET, 124U, NULL, 0U,
                                 fixture->epoch));
            fixture->script_stage = 34U;
        }
        break;
    case 34:
        response = find_response(fixture, BC_VOICE_TUNING_GET, 124U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->payload[5] == 64U && response->payload[6] == 60U &&
                  response->payload[7] == 75U &&
                  get16(response->payload + 8U) == 200U &&
                  get16(response->payload + 10U) == 340U);
            CHECK(response->payload[12] == BC_TOUCH_TUNING_APPLIED);
            fixture->ram.fail_prog_call = fixture->ram.prog_calls + 1U;
            encode_tuning(extra, 70U, 65U, 80U, 240U, 360U);
            CHECK(enqueue_native(BC_VOICE_TUNING_SET, 125U, extra, 7U,
                                 fixture->epoch));
            fixture->script_stage = 35U;
        }
        break;
    case 35:
        response = find_response(fixture, BC_VOICE_TUNING_SET, 125U);
        if (response != NULL) {
            CHECK(response->payload[4] != BC_REC_OK);
            /* A failed program must leave the previously committed metadata
             * and the confirmed in-memory tuning unchanged. */
            {
                uint8_t current_attr[20];
                CHECK(attr_bytes(fixture, 0xa7U, current_attr,
                                 sizeof(current_attr)));
                CHECK(memcmp(current_attr, fixture->saved_tuning_attr,
                             sizeof(current_attr)) == 0);
            }
            CHECK(worker_tuning_snapshot_matches(64U, 60U, false,
                                                 BC_TOUCH_TUNING_APPLIED));
            fixture->ram.fail_sync_call = fixture->ram.sync_calls + 1U;
            encode_tuning(extra, 70U, 65U, 80U, 240U, 360U);
            CHECK(enqueue_native(BC_VOICE_TUNING_SET, 126U, extra, 7U,
                                 fixture->epoch));
            fixture->script_stage = 36U;
        }
        break;
    case 36:
        response = find_response(fixture, BC_VOICE_TUNING_SET, 126U);
        if (response != NULL) {
            uint8_t current_attr[20];
            CHECK(response->payload[4] != BC_REC_OK);
            CHECK(attr_bytes(fixture, 0xa7U, current_attr,
                             sizeof(current_attr)));
            /* A failed sync may have physically programmed a new LittleFS
             * metadata block before reporting the error. It still must not
             * publish the unconfirmed value through the worker. */
            CHECK(worker_tuning_snapshot_matches(64U, 60U, false,
                                                 BC_TOUCH_TUNING_APPLIED));
            fixture->ram.fail_prog_call = 0U;
            fixture->ram.fail_sync_call = 0U;
            encode_tuning(extra, 70U, 65U, 80U, 240U, 360U);
            CHECK(enqueue_native(BC_VOICE_TUNING_SET, 127U, extra, 7U,
                                 fixture->epoch));
            fixture->script_stage = 37U;
        }
        break;
    case 37:
        response = find_response(fixture, BC_VOICE_TUNING_SET, 127U);
        if (response != NULL) {
            bc_voice_tuning persisted;
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(tuning_attr(fixture, &persisted));
            CHECK(persisted.touch_set == 70U && persisted.touch_clear == 65U &&
                  persisted.haptic_strength == 80U &&
                  persisted.start_active_ms == 240U &&
                  persisted.stop_active_ms == 360U);
            CHECK(worker_tuning_snapshot_matches(70U, 65U, false,
                                                 BC_TOUCH_TUNING_PENDING));
            bc_touch_tuning_on_sample(true, false, false,
                                      worker_tuning_write,
                                      worker_tuning_read,
                                      &fixture->tuning_sensor);
            CHECK(fixture->tuning_sensor.gesture_mask[0] ==
                      BC_TOUCH_TUNING_GESTURE_HOLD &&
                  fixture->tuning_sensor.gesture_mask[1] == 0U);
            encode_settings(extra, 1234U, 2345U, true, true, true);
            CHECK(enqueue_native(BC_VOICE_SETTINGS_SET, 128U, extra, 11U,
                                 fixture->epoch));
            fixture->script_stage = 38U;
        }
        break;
    case 38:
        response = find_response(fixture, BC_VOICE_SETTINGS_SET, 128U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->payload[13] == 1U && response->payload[14] == 1U &&
                  response->payload[15] == 1U);
            CHECK(worker_tuning_snapshot_matches(70U, 65U, true,
                                                 BC_TOUCH_TUNING_PENDING));
            bc_touch_tuning_on_sample(true, false, false,
                                      worker_tuning_write,
                                      worker_tuning_read,
                                      &fixture->tuning_sensor);
            CHECK(fixture->tuning_sensor.gesture_mask[0] == 0x0AU &&
                  fixture->tuning_sensor.gesture_mask[1] == 0U);
            CHECK(worker_tuning_snapshot_matches(70U, 65U, true,
                                                 BC_TOUCH_TUNING_APPLIED));
            CHECK(attr_bytes(fixture, 0xa6U, fixture->saved_settings_attr,
                             sizeof(fixture->saved_settings_attr)));
            CHECK(attr_bytes(fixture, 0xa7U, fixture->saved_tuning_attr,
                             sizeof(fixture->saved_tuning_attr)));
            CHECK(enqueue_native(BC_VOICE_TUNING_GET, 129U, NULL, 0U,
                                 fixture->epoch));
            fixture->script_stage = 39U;
        }
        break;
    case 39:
        response = find_response(fixture, BC_VOICE_TUNING_GET, 129U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(response->payload[5] == 70U && response->payload[6] == 65U &&
                  response->payload[7] == 80U &&
                  get16(response->payload + 8U) == 240U &&
                  get16(response->payload + 10U) == 360U);
            CHECK(response->payload[12] == BC_TOUCH_TUNING_APPLIED);
            encode_tuning(extra, 72U, 70U, 90U, 280U, 400U);
            CHECK(enqueue_native(BC_VOICE_TUNING_SET, 130U, extra, 7U,
                                 fixture->epoch));
            encode_settings(extra, 4321U, 5432U, false, false, false);
            CHECK(enqueue_native(BC_VOICE_SETTINGS_SET, 131U, extra, 11U,
                                 fixture->epoch));
            fixture->script_stage = 40U;
        }
        break;
    case 40:
        response = find_response(fixture, BC_VOICE_TUNING_SET, 130U);
        if (response != NULL) {
            CHECK(response->payload[4] == BC_REC_OK);
            fixture->script_stage = 41U;
        }
        break;
    case 41:
        response = find_response(fixture, BC_VOICE_SETTINGS_SET, 131U);
        if (response != NULL) {
            uint8_t current_attr[20];
            CHECK(response->payload[4] == BC_REC_OK);
            CHECK(!fixture->hardware.lights_enabled && !fixture->hardware.haptics_enabled);
            CHECK(lfs_setattr(worker_lfs, "/", 0xa6U,
                              fixture->saved_settings_attr,
                              sizeof(fixture->saved_settings_attr)) == 0);
            CHECK(lfs_setattr(worker_lfs, "/", 0xa7U,
                              fixture->saved_tuning_attr,
                              sizeof(fixture->saved_tuning_attr)) == 0);
            CHECK(attr_bytes(fixture, 0xa7U, current_attr,
                             sizeof(current_attr)));
            CHECK(memcmp(current_attr, fixture->saved_tuning_attr,
                         sizeof(current_attr)) == 0);
            fixture->restart_mode = true;
            fixture->script_stage = 42U;
            stop_worker(fixture);
        }
        break;
    default:
        stop_worker(fixture);
        break;
    }
}

static void test_worker_end_to_end(void)
{
    worker_fixture fixture;

    init_fixture(&fixture);
    CHECK(!fixture.connected);
    CHECK(fixture.hardware.callback_register_calls == 0U);
    fixture.yield_hook = worker_script;
    run_worker(&fixture, 600U);
    CHECK(fixture.script_stage == 42U);
    CHECK(fixture.loop_count < 600U);
    CHECK(test_critical_depth == 0U);
    CHECK(fixture.sink.invalid_packets == 0U);

    /* Re-enter the real worker initialization after deliberately changing
     * its in-memory settings/tuning and restoring the previously committed
     * attributes. This exercises the production load path while keeping the
     * finite RAM NOR model in the same process. */
    run_worker(&fixture, 500U);
    CHECK(fixture.script_stage == 51U);
    CHECK(fixture.loop_count < 500U);
    CHECK(test_critical_depth == 0U);
    fixture_destroy(&fixture);
}

int main(void)
{
    test_worker_end_to_end();
    fprintf(stdout, "%u checks, %u failures\n", checks, failures);
    return failures == 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
