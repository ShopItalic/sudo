#include "app_factory_short.h"
#include "app_factory_scroll.h"
#include "app_factory_controls.h"
#include "app_factory_ptt.h"
#include "app_cmd_handler.h"
#include "app_package.h"
#include "fds.h"
#include <string.h>

/* Independent FDS file; never erase or resize the provisioning record.
 * Static four-word payload must remain alive through the async FDS callback. */
#define LEGACY_MAGIC 0x01435446UL /* FTC, P04/P05 */
#define LIGHT_MAGIC 0x02435446UL /* Unreleased P06 light-only prototype */
#define HOLD_MAGIC 0x03435446UL /* P06 */
#define SWIPE_MAGIC 0x04435446UL /* P07/P08 notification switches */
#define MAGIC 0x05435446UL /* P09: explicit opt-in to Bluetooth scrolling */
#define SWIPE_MUTED_MASK 0x000000f0UL
#define LIGHT_MUTED 0x02000000UL
#define HOLD_DELAY_MASK 0xfc000000UL
#define LEGACY_VALUES_MASK 0x01ffffffUL
#define DEFAULTS (0x01000001UL | SWIPE_MUTED_MASK | ((uint32_t)(FACTORY_HOLD_DEFAULT_STEPS - 1U) << 26))
#define PROTOTYPE_HOLD_MAX_STEPS 20U /* Unpublished P06-r2 disk format */
enum { OK, INVALID, BUSY, STORAGE, CONFLICT, NOT_READY };
static uint32_t current[4], pending[4];
static volatile uint32_t active_values;
static volatile bool ready;
static volatile unsigned init_event, write_event;
static bool loaded, have_record, saving, storage_fault;
static uint8_t request[9];
static fds_record_desc_t descriptor;

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void put32(uint8_t *p, uint32_t v)
{
    unsigned i;
    for (i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (8U*i));
}
static uint32_t crc(const uint32_t *words)
{
    uint32_t value = 0xffffffffUL;
    unsigned i, b;
    for (i = 0; i < 12; ++i) {
        value ^= (words[i/4] >> (8U*(i%4))) & 255U;
        for (b = 0; b < 8; ++b)
            value = (value >> 1) ^ ((value & 1U) ? 0xedb88320UL : 0U);
    }
    return ~value;
}
static bool valid_values(uint32_t v, unsigned max_hold_steps)
{
    unsigned hold = v & 15U, two = (v >> 8) & 255U, three = (v >> 16) & 255U;
    return hold <= 1U && (two == 0U || two == 2U) &&
        (three == 0U || three == 2U) && (v >> 26) < max_hold_steps;
}
static void event(const fds_evt_t *e)
{
    /* Callback may be interrupt context. Only publish aligned flags; no RTOS,
     * recorder, logging, response queue, motor, or filesystem operations. */
    if (e->id == FDS_EVT_INIT) init_event = e->result == NRF_SUCCESS ? 1U : 2U;
    if ((e->id == FDS_EVT_WRITE || e->id == FDS_EVT_UPDATE) &&
        e->write.file_id == FACTORY_CONTROLS_FILE && e->write.record_key == FACTORY_CONTROLS_KEY)
        write_event = e->result == NRF_SUCCESS ? 1U : 2U;
}
void app_factory_controls_init(bool peer_manager_owns_fds)
{
    /* Exactly one init owner. PM must see the first fds_init error itself;
     * this SDK leaves its initializing flag set on some early failures. */
    app_factory_short_init();
    if (fds_register(event) != NRF_SUCCESS) {
        init_event = 2U;
        return;
    }
    if (!peer_manager_owns_fds && fds_init() != NRF_SUCCESS) init_event = 2U;
}
bool app_factory_controls_ready(void) { return ready; }
bool app_factory_controls_haptics(void) { return ready && (active_values & 0x01000000UL) != 0U; }
bool app_factory_controls_recording_light(void)
{
    return ready && (active_values & LIGHT_MUTED) == 0U;
}
unsigned app_factory_controls_hold_delay_ms(void)
{
    return (ready ? 1U + (active_values >> 26) : FACTORY_HOLD_DEFAULT_STEPS) * FACTORY_HOLD_STEP_MS;
}
unsigned app_factory_controls_action(unsigned gesture)
{
    return ready && gesture < 3U ? ((active_values & ~SWIPE_MUTED_MASK) >> (8U*gesture)) & 255U : 0U;
}
bool app_factory_controls_swipe_enabled(unsigned direction)
{
    return ready && direction < 4U && (active_values & (1UL << (direction + 4U))) == 0U;
}
static void apply(void)
{
    app_factory_scroll_reset();
    active_values = current[2];
    ready = true;
    if (!app_factory_controls_haptics()) bc_linear_motor_silence();
}
static void reply(const uint8_t *r, unsigned result)
{
    /* app_package_send_enqueue copies only length bytes synchronously. */
    uint8_t packet[20];
    memcpy(packet, r, 9);
    packet[9] = (uint8_t)result;
    put32(packet + 10, current[1]);
    /* Keep schema 1 byte-for-byte compatible, without exposing packed flags. */
    put32(packet + 14, current[2] & LEGACY_VALUES_MASK & ~SWIPE_MUTED_MASK);
    packet[18] = (current[2] & LIGHT_MUTED) ? 0U : 1U;
    packet[19] = (uint8_t)(1U + (current[2] >> 26));
    /* Schema 4 keeps the 20-byte reply: low nibble = delay (1..10), high
     * nibble = enabled swipes (up/down/left/right). Legacy replies stay exact. */
    if (r[4] >= 4U) packet[19] |= (uint8_t)((~current[2]) & SWIPE_MUTED_MASK);
    app_package_send_enqueue((struct app_cmd_package *)packet,
                             r[4] >= 3U ? 20U : (r[4] == 2U ? 19U : 18U));
}
static void load_record(void)
{
    fds_find_token_t token = {0};
    fds_record_desc_t extra;
    fds_flash_record_t record;
    uint32_t result = fds_record_find(FACTORY_CONTROLS_FILE, FACTORY_CONTROLS_KEY, &descriptor, &token);
    current[0] = MAGIC; current[1] = 0; current[2] = DEFAULTS; current[3] = crc(current);
    if (result == FDS_ERR_NOT_FOUND) { apply(); return; }
    if (result != NRF_SUCCESS) { storage_fault = true; return; }
    have_record = true;
    if (fds_record_open(&descriptor, &record) != NRF_SUCCESS) { storage_fault = true; return; }
    if (record.p_header->length_words == 4U) memcpy(current, record.p_data, sizeof(current));
    else storage_fault = true;
    if (fds_record_close(&descriptor) != NRF_SUCCESS) storage_fault = true;
    /* Refuse corrupt, unknown-schema or duplicate records; never replace them
     * silently with defaults or claim that an unverified mute preference is on. */
    /* Read legacy settings without any boot-time write. Their absent light
     * preference means on; the absent hold delay means 500 ms.
     * The first actual settings change saves schema 4. */
    if ((current[0] != MAGIC && current[0] != SWIPE_MAGIC && current[0] != HOLD_MAGIC && current[0] != LIGHT_MAGIC && current[0] != LEGACY_MAGIC) ||
        (current[0] != MAGIC && current[0] != SWIPE_MAGIC && (current[2] & SWIPE_MUTED_MASK) != 0U) ||
        (current[0] == LEGACY_MAGIC && (current[2] >> 24) > 1U) ||
        (current[0] == LIGHT_MAGIC && (current[2] >> 24) > 3U) ||
        !valid_values(current[2], current[0] == HOLD_MAGIC ? PROTOTYPE_HOLD_MAX_STEPS : FACTORY_HOLD_MAX_STEPS) || current[3] != crc(current)) storage_fault = true;
    if (fds_record_find(FACTORY_CONTROLS_FILE, FACTORY_CONTROLS_KEY, &extra, &token) != FDS_ERR_NOT_FOUND)
        storage_fault = true;
    if (!storage_fault) {
        /* Enabling app notifications on old firmware was not consent to send
         * phone input. Migrate in RAM only; all recording preferences survive. */
        if (current[0] != MAGIC) {
            current[0] = MAGIC;
            current[2] |= SWIPE_MUTED_MASK;
            current[3] = crc(current);
        }
        /* The unpublished r2 prototype allowed up to 10 seconds. Validate its
         * original checksum first, then normalize only the delay in memory.
         * Reboot repeats this interpretation; the next actual settings change
         * persists it. Never write at boot or discard the other preferences. */
        if ((current[2] >> 26) >= FACTORY_HOLD_MAX_STEPS) {
            current[2] = (current[2] & ~HOLD_DELAY_MASK) |
                ((uint32_t)(FACTORY_HOLD_MAX_STEPS - 1U) << 26);
            current[3] = crc(current);
        }
        apply();
    }
}
void app_factory_controls_service(void)
{
    app_factory_short_service();
    bc_linear_motor_service();
    if (!loaded && init_event) {
        loaded = true;
        if (init_event == 1U) load_record(); else storage_fault = true;
    }
    if (saving && write_event) {
        unsigned result = write_event == 1U ? OK : STORAGE;
        if (result == OK) {
            memcpy(current, pending, sizeof(current)); have_record = true; apply();
        } else {
            /* An update failure may have touched flash. Require reboot/readback
             * before another save, retaining the last confirmed runtime state. */
            storage_fault = true;
        }
        saving = false;
        app_factory_ptt_end_settings();
        reply(request, result);
    }
}
bool app_factory_controls_command(const uint8_t *d, unsigned n)
{
    uint32_t values, result, delay_steps;
    fds_record_t record;
    /* File/history commands cannot observe the brief rollover close/open
     * interval or enter while the finalizer owns the capture file. */
    if (d && n >= 4U && d[2] == 0x36U && factory_capture_active()) {
        uint8_t busy[5] = {0, d[1], d[2], d[3], 0};
        app_package_send_enqueue((struct app_cmd_package *)busy, sizeof(busy));
        return true;
    }
    if (app_factory_short_command(d, n)) return true;
    if (app_factory_ptt_status_command(d, n)) return true;
    if (!d || n < 4U || d[2] != 0x84U || (d[3] != 0x10U && d[3] != 0x11U)) return false;
    /* Too short to echo a nonce: consume without reading outside the packet. */
    if (n < 9U) return true;
    if (d[0] != 0U || (d[4] < 1U || d[4] > 5U) || n != (d[3] == 0x10U ? 9U : (d[4] >= 3U ? 19U : 16U + d[4]))) { reply(d, INVALID); return true; }
    if (storage_fault) { reply(d, STORAGE); return true; }
    if (!ready) { reply(d, NOT_READY); return true; }
    if (saving) { reply(d, BUSY); return true; }
    if (d[3] == 0x10U) { reply(d, OK); return true; }
    values = get32(d + 13);
    delay_steps = d[4] >= 3U ? (d[4] >= 4U ? d[18] & 15U : d[18]) : 0U;
    if (!valid_values(values, FACTORY_HOLD_MAX_STEPS) || (values & SWIPE_MUTED_MASK) != 0U || (values >> 24) > 1U ||
        (d[4] >= 2U && d[17] > 1U) ||
        (d[4] >= 3U && (delay_steps < FACTORY_HOLD_MIN_STEPS || delay_steps > FACTORY_HOLD_MAX_STEPS))) { reply(d, INVALID); return true; }
    /* Older schema writes preserve preferences absent from their packet. */
    values |= d[4] >= 2U ? (d[17] ? 0U : LIGHT_MUTED) : (current[2] & LIGHT_MUTED);
    values |= d[4] >= 3U ? ((delay_steps - 1U) << 26) : (current[2] & HOLD_DELAY_MASK);
    if (d[4] == 4U && ((~(uint32_t)d[18]) & SWIPE_MUTED_MASK) != (current[2] & SWIPE_MUTED_MASK)) {
        reply(d, INVALID); return true; /* A scroll-aware app must use schema 5. */
    }
    values |= d[4] == 5U ? ((~(uint32_t)d[18]) & SWIPE_MUTED_MASK) : (current[2] & SWIPE_MUTED_MASK);
    if (get32(d + 9) != current[1]) { reply(d, CONFLICT); return true; }
    if (values == current[2]) { reply(d, OK); return true; }
    if (current[1] == 0xffffffffUL) { reply(d, STORAGE); return true; }
    if (!app_factory_ptt_begin_settings()) { reply(d, BUSY); return true; }
    pending[0] = MAGIC; pending[1] = current[1] + 1U; pending[2] = values; pending[3] = crc(pending);
    memcpy(request, d, sizeof(request));
    record.file_id = FACTORY_CONTROLS_FILE; record.key = FACTORY_CONTROLS_KEY;
    record.data.p_data = pending; record.data.length_words = 4;
    write_event = 0; saving = true;
    result = have_record ? fds_record_update(&descriptor, &record) : fds_record_write(&descriptor, &record);
    if (result != NRF_SUCCESS) {
        saving = false; app_factory_ptt_end_settings();
        reply(d, result == FDS_ERR_NO_SPACE_IN_QUEUES || result == FDS_ERR_BUSY ? BUSY : STORAGE);
    }
    /* No automatic garbage collection: don't reclaim unrelated Peer Manager
     * records or turn a settings save into an unbounded flash operation. */
    return true;
}
