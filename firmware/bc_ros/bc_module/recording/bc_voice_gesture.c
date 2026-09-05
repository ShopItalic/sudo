#include "bc_voice_gesture.h"

#include <string.h>

static bool config_valid(const bc_voice_gesture_config *config)
{
    return config != NULL && config->ptt_limit_ms <= BC_REC_MAX_INTERVAL &&
           config->memo_limit_ms <= BC_REC_MAX_INTERVAL &&
           config->touch_timeout_ms >= 100U && config->touch_timeout_ms <= 750U &&
           config->double_tap_debounce_ms >= 100U && config->double_tap_debounce_ms <= 500U;
}

bool bc_voice_gesture_init(bc_voice_gesture *gesture, bc_recording *recording,
                            const bc_voice_gesture_config *config,
                            bc_rec_result (*new_id)(void *, uint64_t *), void *ctx)
{
    if (gesture == NULL || bc_recording_snapshot(recording) == NULL ||
        !config_valid(config) || new_id == NULL)
        return false;
    memset(gesture, 0, sizeof(*gesture));
    gesture->recording = recording;
    gesture->config = *config;
    gesture->new_id = new_id;
    gesture->id_ctx = ctx;
    return true;
}

bc_rec_result bc_voice_gesture_configure(bc_voice_gesture *gesture,
                                         const bc_voice_gesture_config *config)
{
    if (gesture == NULL || gesture->recording == NULL || !config_valid(config))
        return BC_REC_INVALID;
    if (bc_recording_active(gesture->recording) || gesture->hold_attempted)
        return BC_REC_BUSY;
    gesture->config = *config;
    return BC_REC_OK;
}

static bool owns_ptt(const bc_voice_gesture *gesture)
{
    const bc_rec_snapshot *snapshot = bc_recording_snapshot(gesture->recording);
    return snapshot != NULL && gesture->ptt_id != 0U &&
           snapshot->start.id == gesture->ptt_id && snapshot->start.trigger == BC_REC_PTT &&
           bc_recording_active(gesture->recording);
}

static bc_rec_result start(bc_voice_gesture *gesture, bc_rec_trigger trigger,
                            uint32_t now_ms)
{
    bc_rec_result result;
    bc_rec_start request = {0};
    if (bc_recording_active(gesture->recording))
        return BC_REC_BUSY;
    result = gesture->new_id(gesture->id_ctx, &request.id);
    if (result != BC_REC_OK)
        return result;
    if (request.id == 0U)
        return BC_REC_INVALID;
    request.trigger = trigger;
    request.duration_limit_ms = trigger == BC_REC_PTT ?
        gesture->config.ptt_limit_ms : gesture->config.memo_limit_ms;
    result = bc_recording_start(gesture->recording, &request, now_ms);
    if (result == BC_REC_OK && trigger == BC_REC_PTT)
        gesture->ptt_id = request.id;
    return result;
}

void bc_voice_gesture_tick(bc_voice_gesture *gesture, uint32_t now_ms)
{
    if (gesture == NULL || gesture->recording == NULL)
        return;
    if (gesture->have_report && owns_ptt(gesture) &&
        (uint32_t)(now_ms - gesture->last_report_ms) >= gesture->config.touch_timeout_ms) {
        const bc_rec_snapshot *snapshot = bc_recording_snapshot(gesture->recording);
        if (snapshot->error == BC_REC_OK)
            (void)bc_recording_fault(gesture->recording, gesture->ptt_id,
                                     BC_REC_TOUCH_ERROR, now_ms);
    }
}

bc_rec_result bc_voice_gesture_report(bc_voice_gesture *gesture,
                                      const bc_touch_report_t *report,
                                      uint32_t now_ms)
{
    const bc_rec_snapshot *snapshot;
    bc_rec_result result = BC_REC_OK;
    if (gesture == NULL || gesture->recording == NULL || report == NULL)
        return BC_REC_INVALID;
    /* A late report cannot erase the fact that the sensor's liveness lease
     * expired. Preserve the uncertain clip, and require release before retry. */
    bc_voice_gesture_tick(gesture, now_ms);
    if (!report->valid) {
        if (owns_ptt(gesture))
            return bc_recording_fault(gesture->recording, gesture->ptt_id,
                                       BC_REC_TOUCH_ERROR, now_ms);
        return BC_REC_TOUCH_ERROR;
    }
    if (report->hold && (!report->contact || report->double_tap))
        return BC_REC_INVALID;
    gesture->last_report_ms = now_ms;
    gesture->have_report = true;
    if (!report->contact) {
        if (owns_ptt(gesture))
            result = bc_recording_stop(gesture->recording, gesture->ptt_id, now_ms);
        gesture->hold_attempted = false;
        gesture->ptt_id = 0;
    } else if (report->hold && !gesture->hold_attempted) {
        gesture->hold_attempted = true;
        result = start(gesture, BC_REC_PTT, now_ms);
    }
    if (!report->double_tap)
        return result;
    if (gesture->have_double &&
        (uint32_t)(now_ms - gesture->last_double_ms) < gesture->config.double_tap_debounce_ms)
        return BC_REC_DUPLICATE;
    gesture->have_double = true;
    gesture->last_double_ms = now_ms;
    if (!gesture->config.memo_enabled)
        return BC_REC_UNSUPPORTED;
    snapshot = bc_recording_snapshot(gesture->recording);
    if (bc_recording_active(gesture->recording)) {
        if (snapshot->start.trigger == BC_REC_PTT)
            return BC_REC_BUSY;
        return bc_recording_stop(gesture->recording, snapshot->start.id, now_ms);
    }
    return start(gesture, BC_REC_MEMO, now_ms);
}
