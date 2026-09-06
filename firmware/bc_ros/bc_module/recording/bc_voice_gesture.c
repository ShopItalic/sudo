#include "bc_voice_gesture.h"

#include <string.h>

static bool config_valid(const bc_voice_gesture_config *config)
{
    return config != NULL && config->ptt_limit_ms == 0U &&
           config->memo_limit_ms <= BC_REC_MAX_INTERVAL &&
           config->touch_timeout_ms >= 100U && config->touch_timeout_ms <= 750U &&
           config->tap_debounce_ms >= 100U && config->tap_debounce_ms <= 500U;
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
    gesture->inputs.hold_ms = BC_VOICE_INPUT_HOLD_DEFAULT_MS;
    gesture->inputs.hold_action = BC_VOICE_INPUT_PTT;
    gesture->inputs.triple_action = config->memo_enabled ? BC_VOICE_INPUT_MEMO : BC_VOICE_INPUT_DISABLED;
    gesture->new_id = new_id;
    gesture->id_ctx = ctx;
    return true;
}

bc_rec_result bc_voice_gesture_configure(bc_voice_gesture *gesture,
                                         const bc_voice_gesture_config *config)
{
    if (gesture == NULL || gesture->recording == NULL || !config_valid(config))
        return BC_REC_INVALID;
    if (bc_recording_active(gesture->recording) || gesture->hold_attempted || gesture->contact_active)
        return BC_REC_BUSY;
    gesture->config = *config;
    if (!gesture->inputs_configured)
        gesture->inputs.triple_action = config->memo_enabled ? BC_VOICE_INPUT_MEMO : BC_VOICE_INPUT_DISABLED;
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

bc_rec_result bc_voice_gesture_set_inputs(bc_voice_gesture *gesture,
                                          const bc_voice_inputs *inputs)
{
    if (!gesture || !gesture->recording || !bc_voice_inputs_valid(inputs))
        return BC_REC_INVALID;
    if (bc_recording_active(gesture->recording) || gesture->hold_attempted ||
        gesture->contact_active) return BC_REC_BUSY;
    gesture->inputs = *inputs;
    gesture->inputs_configured = true;
    memset(gesture->have_tap, 0, sizeof(gesture->have_tap));
    return BC_REC_OK;
}

void bc_voice_gesture_set_event_handler(bc_voice_gesture *gesture,
    bool (*handler)(void *, uint8_t, uint8_t), void *ctx)
{
    if (!gesture) return;
    gesture->input_event = handler;
    gesture->event_ctx = ctx;
}

static void app_hold_end(bc_voice_gesture *gesture, uint8_t phase)
{
    if (!gesture->app_hold_active) return;
    gesture->app_hold_active = false;
    if (gesture->input_event)
        (void)gesture->input_event(gesture->event_ctx, BC_VOICE_INPUT_HOLD, phase);
}

void bc_voice_gesture_tick(bc_voice_gesture *gesture, uint32_t now_ms)
{
    if (gesture == NULL || gesture->recording == NULL) return;
    if (gesture->have_report &&
        (uint32_t)(now_ms - gesture->last_report_ms) >= gesture->config.touch_timeout_ms) {
        app_hold_end(gesture, BC_VOICE_INPUT_CANCELLED);
        if (owns_ptt(gesture)) {
            const bc_rec_snapshot *snapshot = bc_recording_snapshot(gesture->recording);
            if (snapshot->error == BC_REC_OK)
                (void)bc_recording_fault(gesture->recording, gesture->ptt_id,
                                         BC_REC_TOUCH_ERROR, now_ms);
        }
    }
}

static bc_rec_result action(bc_voice_gesture *gesture, uint8_t input,
                             uint8_t binding, uint32_t now_ms)
{
    const bc_rec_snapshot *snapshot = bc_recording_snapshot(gesture->recording);
    switch (binding) {
    case BC_VOICE_INPUT_DISABLED:
        return BC_REC_OK;
    case BC_VOICE_INPUT_APP:
        if (!gesture->input_event ||
            !gesture->input_event(gesture->event_ctx, input, BC_VOICE_INPUT_ACTIVATED))
            return BC_REC_INTERRUPTED;
        if (input == BC_VOICE_INPUT_HOLD) gesture->app_hold_active = true;
        return BC_REC_OK;
    case BC_VOICE_INPUT_PTT:
        /* The default hold also stops a gesture-owned memo. Consume this
         * entire contact; release must precede another PTT start attempt. */
        if (bc_recording_active(gesture->recording) && snapshot->start.trigger == BC_REC_MEMO)
            return bc_recording_stop(gesture->recording, snapshot->start.id, now_ms);
        return start(gesture, BC_REC_PTT, now_ms);
    case BC_VOICE_INPUT_MEMO:
        if (bc_recording_active(gesture->recording)) {
            if (snapshot->start.trigger == BC_REC_PTT) return BC_REC_BUSY;
            return bc_recording_stop(gesture->recording, snapshot->start.id, now_ms);
        }
        return start(gesture, BC_REC_MEMO, now_ms);
    default:
        return BC_REC_INVALID;
    }
}

bc_rec_result bc_voice_gesture_report(bc_voice_gesture *gesture,
                                      const bc_touch_report_t *report,
                                      uint32_t now_ms)
{
    bc_rec_result result = BC_REC_OK;
    unsigned tap;
    if (!gesture || !gesture->recording || !report) return BC_REC_INVALID;
    /* A late report cannot erase an expired sensor lease. */
    bc_voice_gesture_tick(gesture, now_ms);
    if (!report->valid) {
        app_hold_end(gesture, BC_VOICE_INPUT_CANCELLED);
        if (owns_ptt(gesture))
            return bc_recording_fault(gesture->recording, gesture->ptt_id,
                                       BC_REC_TOUCH_ERROR, now_ms);
        return BC_REC_TOUCH_ERROR;
    }
    if ((report->hold && (!report->contact || report->double_tap || report->triple_tap)) ||
        (report->double_tap && report->triple_tap)) return BC_REC_INVALID;
    gesture->last_report_ms = now_ms;
    gesture->have_report = true;
    gesture->contact_active = report->contact;
    if (!report->contact) {
        app_hold_end(gesture, BC_VOICE_INPUT_RELEASED);
        if (owns_ptt(gesture))
            result = bc_recording_stop(gesture->recording, gesture->ptt_id, now_ms);
        gesture->hold_attempted = false;
        gesture->ptt_id = 0;
    } else if (report->hold && !gesture->hold_attempted) {
        gesture->hold_attempted = true;
        result = action(gesture, BC_VOICE_INPUT_HOLD, gesture->inputs.hold_action, now_ms);
    }
    if (!report->double_tap && !report->triple_tap) return result;
    tap = report->triple_tap ? 1U : 0U;
    if (gesture->have_tap[tap] &&
        (uint32_t)(now_ms - gesture->last_tap_ms[tap]) < gesture->config.tap_debounce_ms)
        return BC_REC_DUPLICATE;
    gesture->have_tap[tap] = true;
    gesture->last_tap_ms[tap] = now_ms;
    return action(gesture, report->triple_tap ? BC_VOICE_INPUT_TRIPLE : BC_VOICE_INPUT_DOUBLE,
        report->triple_tap ? gesture->inputs.triple_action : gesture->inputs.double_action, now_ms);
}
