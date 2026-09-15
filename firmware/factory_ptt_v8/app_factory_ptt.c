#include "app_factory_ptt.h"
#include "bc_rtos.h"
#include "app_pdm_handler.h"
#include "app_factory_controls.h"
#include "app_cmd_handler.h"
#include "app_package.h"
#include <string.h>

/* No heap, new task, or recorder implementation. P05 publishes the result
 * after the unchanged factory recorder returns, never merely on a touch edge. */
app_factory_ptt_state factory_ptt_diagnostics;
static TaskHandle_t command_worker;
#define PTT_LEASE_TICKS pdMS_TO_TICKS(750)
#define PTT_POLL_TICKS pdMS_TO_TICKS(50)
#define P factory_ptt_diagnostics

static void cancel_locked(void)
{
    P.wanted = false;
    P.armed = false; /* Require a fresh all-fingers-up report after cancellation. */
}

static void expire_locked(uint32_t now)
{
    if (P.wanted && (uint32_t)(now - P.last_tick) >= PTT_LEASE_TICKS) {
        ++P.timeouts;
        cancel_locked();
    }
}

static void wake_worker(void)
{
    TaskHandle_t worker;
    taskENTER_CRITICAL();
    worker = command_worker;
    taskEXIT_CRITICAL();
    if (worker) xTaskNotifyGive(worker);
}

void app_factory_ptt_cancel(void)
{
    taskENTER_CRITICAL();
    cancel_locked();
    taskEXIT_CRITICAL();
    wake_worker();
}

void app_factory_ptt_report(const uint8_t d[8], bool ok)
{
    unsigned fingers;
    uint16_t x, y;
    uint32_t now = xTaskGetTickCount();
    bool hold, changed, was_wanted;
    taskENTER_CRITICAL();
    was_wanted = P.wanted;
    expire_locked(now);
    if (!ok || !d || (d[2] & 0xf8U) || (d[3] & 0x10U)) {
        ++P.faults;
        cancel_locked();
    } else {
        fingers = d[3] & 3U;
        hold = (d[0] & 8U) != 0U;
        x = (uint16_t)d[4] | ((uint16_t)d[5] << 8);
        y = (uint16_t)d[6] | ((uint16_t)d[7] << 8);
        if (fingers == 3U || (!fingers && hold) ||
            (fingers && (x == 0xffffU || y == 0xffffU))) {
            ++P.faults;
            cancel_locked();
        } else {
            /* The IQS callback is dispatched after this raw report, with
             * task/I2C yield points between them. Remember a conflicting tap
             * even if the worker finishes PTT before that callback arrives.
             * Also cover a hold and tap coalesced into the first hold frame. */
            if (P.wanted || P.owned || P.settings_lock ||
                (hold && fingers && P.armed && !P.memo_owned &&
                 app_factory_controls_action(0) == 1U))
                P.suppress_taps |= d[0] & 6U;
            P.contact = fingers != 0U;
            P.sampled = true;
            P.last_tick = now;
            ++P.samples;
            if (!fingers) {
                P.wanted = false;
                P.attempted = false;
                P.armed = true;
            } else if (hold) {
                if (P.armed) {
                    P.armed = false;
                    if (!P.owned && !P.memo_owned && !P.settings_lock &&
                        app_factory_controls_action(0) == 1U) {
                        P.hold_started_tick = now;
                        P.wanted = true;
                    }
                }
            } else if (P.wanted || P.owned) {
                /* Originating hold finger lifted, even if another remains. */
                cancel_locked();
            }
        }
    }
    changed = was_wanted != P.wanted;
    taskEXIT_CRITICAL();
    if (changed) wake_worker();
}

static bool tap(unsigned gesture)
{
    bool enabled, suppressed;
    uint8_t bit = (uint8_t)(1U << gesture);
    taskENTER_CRITICAL();
    enabled = app_factory_controls_action(gesture) == 2U;
    suppressed = (P.suppress_taps & bit) != 0U;
    P.suppress_taps &= (uint8_t)~bit;
    if (!P.settings_lock && enabled) {
        if (suppressed || P.wanted || P.owned) cancel_locked();
        else P.toggle_pending = !P.toggle_pending; /* two toggles cancel */
    }
    taskEXIT_CRITICAL();
    wake_worker();
    return enabled;
}
void app_factory_ptt_double_tap(void) { (void)tap(1); }
bool app_factory_ptt_triple_tap(void) { return tap(2); }

void app_factory_ptt_worker_init(void)
{
    taskENTER_CRITICAL();
    command_worker = xTaskGetCurrentTaskHandle();
    taskEXIT_CRITICAL();
}

static bool on_worker(void)
{
    return command_worker && xTaskGetCurrentTaskHandle() == command_worker;
}

static void recording_snapshot(uint8_t *out)
{
    unsigned i;
    out[0] = P.recording_state;
    for (i = 0; i < 4; ++i) out[1+i] = (uint8_t)(P.recording_sequence >> (8U*i));
}

static void recording_publish(unsigned state)
{
    /* Existing factory raw-key envelope: type,id,cmd,sub,len,payload.
     * P05 payload is ['P','5',schema=1,state,sequence_le32], exactly 8 bytes.
     * Queue copies these bytes; no extra task or live audio channel. */
    uint8_t packet[13] = {0,9,0x61,1,8,'P','5',1};
    if (P.recording_state == state) return;
    P.recording_state = (uint8_t)state;
    ++P.recording_sequence;
    recording_snapshot(packet + 8);
    app_package_send_enqueue((struct app_cmd_package *)packet, sizeof(packet));
}

bool app_factory_ptt_status_command(const uint8_t *data, unsigned length)
{
    uint8_t packet[14];
    if (!data || length < 4 || data[2] != 0x84 || data[3] != 0x12) return false;
    if (!on_worker() || length != 9 || data[0] != 0 || data[4] != 1) return true;
    /* GET echoes schema and request nonce, then the same five-byte snapshot. */
    memcpy(packet, data, 9);
    recording_snapshot(packet + 9);
    app_package_send_enqueue((struct app_cmd_package *)packet, sizeof(packet));
    return true;
}

bool app_factory_ptt_begin_settings(void)
{
    bool idle;
    if (!on_worker() || app_pdm_work_status()) return false;
    taskENTER_CRITICAL();
    idle = P.sampled && !P.contact && !P.wanted && !P.owned &&
        !P.memo_owned && !P.toggle_pending && !P.settings_lock;
    if (idle) { P.settings_lock = true; cancel_locked(); }
    taskEXIT_CRITICAL();
    return idle;
}
void app_factory_ptt_end_settings(void)
{
    taskENTER_CRITICAL();
    P.settings_lock = false;
    P.toggle_pending = false;
    cancel_locked(); /* a new all-fingers-up sample is required */
    taskEXIT_CRITICAL();
}

/* Only this worker starts/stops recordings. Tap actions use the unchanged
 * local ADPCM recorder; they never enter Z62's connected online-stream path. */
void app_factory_ptt_worker_service(void)
{
    bool stop, start, ok, was_working, lost_recording = false, memo = false;
    if (!on_worker()) return;
    was_working = app_pdm_work_status();
    taskENTER_CRITICAL();
    if ((P.owned || P.memo_owned) && !was_working && P.recording_state == 1) {
        P.owned = P.memo_owned = false;
        cancel_locked();
        lost_recording = true;
    }
    expire_locked(xTaskGetTickCount());
    if (P.toggle_pending) {
        P.toggle_pending = false;
        if (P.memo_owned || P.owned) {
            P.owned = true; P.memo_owned = false; cancel_locked();
        } else memo = true;
    }
    stop = P.owned && !P.wanted;
    /* The unchanged IQS configuration confirms a hold after 500 ms. Wait
     * only the additional selected delay, retaining the same live-report
     * lease and release/fault cancellation while waiting. Tick subtraction
     * remains wrap-safe. The worker never starts from an expired hold. */
    start = memo || (P.wanted && !P.owned && !P.attempted &&
        (uint32_t)(xTaskGetTickCount() - P.hold_started_tick) >=
        pdMS_TO_TICKS(app_factory_controls_hold_delay_ms() - 500U));
    if (start) {
        P.attempted = true;
        if (memo) P.memo_owned = true;
        else P.owned = true;
    }
    taskEXIT_CRITICAL();
    if (lost_recording) recording_publish(2);
    if (stop) {
        ok = app_pdm_recording_stop();
        taskENTER_CRITICAL();
        P.owned = false; ++P.stops;
        if (!ok) ++P.stop_failures;
        taskEXIT_CRITICAL();
        recording_publish(ok ? 0 : 2);
    }
    if (start) {
        ok = !app_pdm_work_status() && app_pdm_recording_start();
        taskENTER_CRITICAL();
        if (memo) P.memo_owned = ok; else P.owned = ok;
        if (ok) ++P.starts;
        else { ++P.start_failures; cancel_locked(); }
        expire_locked(xTaskGetTickCount());
        stop = P.owned && !P.wanted;
        taskEXIT_CRITICAL();
        recording_publish(ok ? 1 : 2);
        if (stop) {
            ok = app_pdm_recording_stop();
            taskENTER_CRITICAL();
            P.owned = false; ++P.stops;
            if (!ok) ++P.stop_failures;
            taskEXIT_CRITICAL();
            recording_publish(ok ? 0 : 2);
        }
    }
}

void app_factory_ptt_before_command(const uint8_t *d, unsigned n)
{
    bool changes_recorder = false;
    if (!on_worker() || !d || n < 4U) return;
    /* Factory frame is [type, id, command, subcommand, payload...]. */
    if (d[2] == 0x71U) {
        /* Retired streaming commands must not cancel a local recording. */
        changes_recorder = d[3] == 5U || d[3] == 0xfeU || d[3] == 0xf9U;

    }
    if (changes_recorder) {
        taskENTER_CRITICAL();
        P.toggle_pending = false;
        if (P.memo_owned) { P.memo_owned = false; P.owned = true; }
        taskEXIT_CRITICAL();
        app_factory_ptt_cancel();
        /* Stop our own recording before handing ownership to the command. */
        app_factory_ptt_worker_service();
    }
}

void app_factory_ptt_worker_wait(void)
{
    bool pending;
    if (!on_worker()) return;
    taskENTER_CRITICAL();
    pending = P.wanted || P.owned;
    taskEXIT_CRITICAL();
    /* Preserve the factory 1000-tick inter-command wait when idle. A touch
     * transition interrupts that wait. Queue receive itself polls at 50 ms. */
    (void)ulTaskNotifyTake(pdTRUE, pending ? PTT_POLL_TICKS : 1000U);
}
