#include "bc_voice_service.h"

#include <string.h>

static bc_rec_result store_result(bc_rec_store_status result)
{
    switch (result) {
    case BC_REC_STORE_OK: return BC_REC_OK;
    case BC_REC_STORE_END: case BC_REC_STORE_NOT_FOUND: return BC_REC_NOT_FOUND;
    case BC_REC_STORE_BUSY: return BC_REC_BUSY;
    case BC_REC_STORE_NO_SPACE: return BC_REC_NO_SPACE;
    case BC_REC_STORE_OPEN_ERROR: return BC_REC_OPEN_ERROR;
    case BC_REC_STORE_WRITE_ERROR: return BC_REC_WRITE_ERROR;
    case BC_REC_STORE_SYNC_ERROR: return BC_REC_SYNC_ERROR;
    case BC_REC_STORE_CLOSE_ERROR: return BC_REC_CLOSE_ERROR;
    case BC_REC_STORE_CORRUPT: return BC_REC_CRC_ERROR;
    case BC_REC_STORE_EMPTY: return BC_REC_EMPTY_AUDIO;
    case BC_REC_STORE_RECEIPT_MISMATCH: return BC_REC_CUSTODY_REQUIRED;
    case BC_REC_STORE_DELETED: return BC_REC_NOT_FOUND;
    case BC_REC_STORE_DELETE_ERROR: return BC_REC_WRITE_ERROR;
    default: return BC_REC_INVALID;
    }
}

static uint16_t message_id(bc_voice_service *s)
{
    ++s->next_message;
    if (s->next_message == 0) ++s->next_message;
    return s->next_message;
}

static void reply_init(bc_voice_message *out, const bc_voice_message *request,
                        bc_rec_result result)
{
    memset(out, 0, sizeof(*out));
    out->message_id = request->message_id;
    out->kind = request->kind;
    out->direction = BC_VOICE_RESPONSE;
    out->length = 5;
    memcpy(out->payload, request->payload, 4);
    out->payload[4] = (uint8_t)result;
}

static bool queue(bc_voice_service *s, const bc_voice_message *message)
{
    if (s->control_count == BC_VOICE_CONTROL_SLOTS) return false;
    s->controls[(s->control_read + s->control_count) % BC_VOICE_CONTROL_SLOTS] = *message;
    ++s->control_count;
    return true;
}

static void simple_reply(bc_voice_service *s, const bc_voice_message *request,
                          bc_rec_result result)
{
    bc_voice_message response;
    reply_init(&response, request, result);
    (void)queue(s, &response);
}

static void live_prefix_clear(bc_voice_service *s)
{
    s->live_prefix_read = 0U;
    s->live_prefix_count = 0U;
}

static bool live_prefix_push(bc_voice_service *s, uint32_t sequence,
                             const uint8_t *data, uint16_t length)
{
    uint8_t index;
    if (s->live_prefix_count >= BC_VOICE_LIVE_PREFIX_SLOTS)
        return false;
    index = (uint8_t)((s->live_prefix_read + s->live_prefix_count) %
                      BC_VOICE_LIVE_PREFIX_SLOTS);
    s->live_prefix[index].sequence = sequence;
    s->live_prefix[index].length = length;
    memcpy(s->live_prefix[index].data, data, length);
    ++s->live_prefix_count;
    return true;
}

static bool live_prefix_push_front(bc_voice_service *s, uint32_t sequence,
                                   const uint8_t *data, uint16_t length)
{
    if (s->live_prefix_count >= BC_VOICE_LIVE_PREFIX_SLOTS)
        return false;
    s->live_prefix_read = (uint8_t)((s->live_prefix_read == 0U) ?
        BC_VOICE_LIVE_PREFIX_SLOTS - 1U : s->live_prefix_read - 1U);
    s->live_prefix[s->live_prefix_read].sequence = sequence;
    s->live_prefix[s->live_prefix_read].length = length;
    memcpy(s->live_prefix[s->live_prefix_read].data, data, length);
    ++s->live_prefix_count;
    return true;
}

static void live_transport_clear(bc_voice_service *s)
{
    live_prefix_clear(s);
    s->live_count = 0U;
    s->live_ack = 0U;
    if (s->tx_active && s->tx.kind == BC_VOICE_LIVE) {
        s->tx_active = false;
        s->tx_offset = 0U;
    }
}

static void live_preview_disable(bc_voice_service *s)
{
    s->ready = false;
    s->live_disabled = true;
    live_transport_clear(s);
}

static bool live_requeue_active_tx(bc_voice_service *s)
{
    if (!s->tx_active || s->tx.kind != BC_VOICE_LIVE)
        return true;
    if (s->tx.length < 9U || s->tx.length > 8U + BC_REC_FRAME_MAX ||
        !live_prefix_push_front(s, bc_voice_get32(s->tx.payload + 4U),
                                s->tx.payload + 8U, (uint16_t)(s->tx.length - 8U)))
        return false;
    s->tx_active = false;
    s->tx_offset = 0U;
    return true;
}

static void encode_snapshot(bc_voice_message *out, const bc_rec_snapshot *r,
                              uint32_t token)
{
    uint8_t *p = out->payload;
    uint8_t length = 0;
    while (length < BC_REC_NAME_SIZE - 1U && r->file.name[length] != 0) ++length;
    bc_voice_put64(p + 5, r->start.id);
    p[13] = (uint8_t)r->start.trigger; p[14] = (uint8_t)r->phase;
    p[15] = (uint8_t)r->error;
    p[16] = (r->file.complete ? 1U : 0U) | (r->file.recovered ? 2U : 0U) |
            (r->file.delivered ? 4U : 0U);
    bc_voice_put32(p + 17, r->start.duration_limit_ms);
    bc_voice_put32(p + 21, r->revision);
    bc_voice_put32(p + 25, r->accepted_bytes);
    bc_voice_put32(p + 29, r->accepted_frames);
    bc_voice_put32(p + 33, r->durable_bytes);
    bc_voice_put32(p + 37, r->file.bytes);
    bc_voice_put32(p + 41, r->file.frames);
    bc_voice_put32(p + 45, r->file.crc32);
    bc_voice_put32(p + 49, r->live_sent_frames);
    bc_voice_put32(p + 53, r->live_dropped_frames);
    bc_voice_put32(p + 57, token);
    p[61] = length;
    memcpy(p + 62, r->file.name, length);
    /* S05: the per-recording descriptor follows the name. */
    bc_audio_format_encode(p + 62 + length, &r->file.audio);
    out->length = (uint16_t)(62U + length + BC_AUDIO_FORMAT_WIRE_SIZE);
}

static void snapshot_reply(bc_voice_service *s, const bc_voice_message *request,
                            bc_rec_result result, const bc_rec_snapshot *snapshot,
                            uint32_t token)
{
    bc_voice_message response;
    reply_init(&response, request, result);
    encode_snapshot(&response, snapshot, token);
    (void)queue(s, &response);
}

static void settings_reply(bc_voice_service *s, const bc_voice_message *request,
                            bc_rec_result result)
{
    bc_voice_message response;
    reply_init(&response, request, result);
    bc_voice_put32(response.payload + 5, s->settings.ptt_limit_ms);
    bc_voice_put32(response.payload + 9, s->settings.memo_limit_ms);
    response.payload[13] = s->settings.memo_enabled;
    response.payload[14] = s->settings.led_enabled;
    response.payload[15] = s->settings.haptic_enabled;
    response.length = 16;
    (void)queue(s, &response);
}

static void inputs_reply(bc_voice_service *s, const bc_voice_message *request,
                          bc_rec_result result)
{
    bc_voice_message response;
    bc_voice_inputs inputs = {0};
    uint8_t status = 0U;
    if (!s->inputs_port.get || !s->inputs_port.get(s->inputs_port.ctx, &inputs, &status))
        result = BC_REC_UNSUPPORTED;
    reply_init(&response, request, result);
    bc_voice_put16(response.payload + 5U, inputs.hold_ms);
    response.payload[7] = inputs.hold_action;
    response.payload[8] = inputs.double_action;
    response.payload[9] = inputs.triple_action;
    response.payload[10] = status;
    response.length = 11U;
    (void)queue(s, &response);
}

bool bc_voice_service_set_inputs_port(bc_voice_service *s, const bc_voice_inputs_port *port)
{
    if (!s || !port || !port->get || !port->set) return false;
    s->inputs_port = *port;
    return true;
}

bool bc_voice_service_input(bc_voice_service *s, uint8_t input, uint8_t phase)
{
    bc_voice_input_event *event;
    uint8_t binding;
    if (!s || !s->connected || input < BC_VOICE_INPUT_HOLD || input > BC_VOICE_INPUT_TRIPLE ||
        phase < BC_VOICE_INPUT_ACTIVATED || phase > BC_VOICE_INPUT_CANCELLED ||
        (input != BC_VOICE_INPUT_HOLD && phase != BC_VOICE_INPUT_ACTIVATED) ||
        s->input_sequence == UINT32_MAX) return false;
    binding = input == BC_VOICE_INPUT_HOLD ? s->gesture->inputs.hold_action :
        (input == BC_VOICE_INPUT_DOUBLE ? s->gesture->inputs.double_action : s->gesture->inputs.triple_action);
    if (binding != BC_VOICE_INPUT_APP) return false;
    if (input == BC_VOICE_INPUT_HOLD && phase != BC_VOICE_INPUT_ACTIVATED &&
        !s->input_hold_accepted) return false;
    if (s->input_count == BC_VOICE_INPUT_EVENT_SLOTS) {
        if (phase == BC_VOICE_INPUT_ACTIVATED) return false;
        /* Never retain queued tap activations at the expense of ending a hold. */
        s->input_read = 0U; s->input_count = 0U;
    }
    event = &s->input_events[(s->input_read + s->input_count) % BC_VOICE_INPUT_EVENT_SLOTS];
    event->sequence = ++s->input_sequence;
    event->at_ms = s->now_ms;
    event->input = input; event->phase = phase;
    ++s->input_count;
    if (input == BC_VOICE_INPUT_HOLD)
        s->input_hold_accepted = phase == BC_VOICE_INPUT_ACTIVATED;
    return true;
}

bool bc_voice_tuning_valid(const bc_voice_tuning *t)
{
    return t && t->touch_set >= 32U && t->touch_set <= 80U &&
        t->touch_clear >= 30U && t->touch_clear <= t->touch_set - 2U &&
        t->haptic_strength >= 1U && t->haptic_strength <= 100U &&
        t->start_active_ms >= 20U && t->start_active_ms <= 400U &&
        t->start_active_ms % 20U == 0U && t->stop_active_ms >= 20U &&
        t->stop_active_ms <= 400U && t->stop_active_ms % 20U == 0U;
}

bool bc_voice_service_set_tuning_port(bc_voice_service *s, const bc_voice_tuning_port *port)
{
    if (!s || !port || !port->get || !port->set) return false;
    s->tuning_port = *port;
    return true;
}

bool bc_voice_service_set_outcome_port(bc_voice_service *s,
                                       const bc_voice_outcome_port *port)
{
    if (!s || !port || !port->set_outcome) return false;
    s->outcome_port = *port;
    return true;
}

bool bc_voice_service_set_audio_port(bc_voice_service *s,
                                     const bc_voice_audio_port *port)
{
    if (!s || !port || !port->get) return false;
    s->audio_port = *port;
    return true;
}

/* The format new recordings receive; legacy ADPCM when no audio port exists. */
static bool current_format(bc_voice_service *s, bc_audio_format *format,
                           bc_voice_audio_stats *stats)
{
    bc_voice_audio_stats ignored;
    if (stats == NULL) stats = &ignored;
    memset(stats, 0, sizeof(*stats));
    if (s->audio_port.get && s->audio_port.get(s->audio_port.ctx, format, stats) &&
        bc_audio_format_valid(format) && format->codec != BC_AUDIO_CODEC_NONE)
        return true;
    bc_audio_format_legacy_adpcm(format);
    return false;
}

static void format_reply(bc_voice_service *s, const bc_voice_message *request)
{
    bc_voice_message response;
    bc_audio_format format;
    bc_voice_audio_stats stats;
    uint8_t *p;
    if (!current_format(s, &format, &stats)) {
        simple_reply(s, request, BC_REC_UNSUPPORTED);
        return;
    }
    reply_init(&response, request, BC_REC_OK);
    p = response.payload;
    bc_audio_format_encode(p + 5, &format);
    bc_voice_put32(p + 21, stats.state_bytes);
    bc_voice_put32(p + 25, stats.scratch_bytes);
    bc_voice_put32(p + 29, stats.scratch_high_water);
    bc_voice_put32(p + 33, stats.frames);
    bc_voice_put32(p + 37, stats.max_encode_us);
    bc_voice_put32(p + 41, stats.mean_encode_us);
    bc_voice_put32(p + 45, stats.faults);
    response.length = BC_VOICE_FORMAT_RESPONSE_LENGTH;
    (void)queue(s, &response);
}

static void outcome_clear(bc_voice_service *s)
{
    s->outcome_recording_id = 0U;
    s->outcome_ready_recording_id = 0U;
    s->outcome_live_token = 0U;
    s->outcome_epoch = 0U;
    s->outcome_terminal_ms = 0U;
    s->outcome_ready_accepted = false;
    s->outcome_terminal_ready = false;
    s->outcome_reported = false;
}

static bool outcome_terminal_valid(const bc_rec_snapshot *r)
{
    if (!r || !r->start.id || !r->file.complete || r->file.recovered ||
        !r->file.bytes)
        return false;
    if (r->phase == BC_REC_SAVED)
        return !r->file.delivered;
    return r->phase == BC_REC_DELIVERED && r->file.delivered;
}

static void tuning_reply(bc_voice_service *s, const bc_voice_message *request,
                          bc_rec_result result)
{
    bc_voice_message response;
    bc_voice_tuning tuning;
    uint8_t status;
    if (!s->tuning_port.get ||
        !s->tuning_port.get(s->tuning_port.ctx, &tuning, &status) ||
        !bc_voice_tuning_valid(&tuning) || status > 2U) {
        simple_reply(s, request, BC_REC_UNSUPPORTED); return;
    }
    reply_init(&response, request, result);
    response.payload[5] = tuning.touch_set;
    response.payload[6] = tuning.touch_clear;
    response.payload[7] = tuning.haptic_strength;
    bc_voice_put16(response.payload + 8, tuning.start_active_ms);
    bc_voice_put16(response.payload + 10, tuning.stop_active_ms);
    response.payload[12] = status; response.length = 13U;
    (void)queue(s, &response);
}

bool bc_voice_service_init(bc_voice_service *s, bc_recording *recording,
                            bc_rec_store *store, bc_voice_gesture *gesture,
                            const bc_voice_service_port *port,
                            const bc_voice_settings *settings)
{
    if (!s || !bc_recording_snapshot(recording) || !store || !gesture ||
        !port || !port->send || !port->settings || !settings ||
        settings->ptt_limit_ms != 0U ||
        settings->memo_limit_ms > BC_REC_MAX_INTERVAL) return false;
    memset(s, 0, sizeof(*s));
    s->recording = recording; s->store = store; s->gesture = gesture;
    s->port = *port; s->settings = *settings;
    s->latest = *bc_recording_snapshot(recording);
    return true;
}

bc_rec_result bc_voice_service_cancel_archive(bc_voice_service *s)
{
    bc_rec_result result = BC_REC_OK;
    if (!s) return BC_REC_INVALID;
    if (s->reader.open) result = store_result(bc_rec_store_reader_close(&s->reader));
    if (s->verifying && s->connected)
        simple_reply(s, &s->resume_request, BC_REC_CANCELLED);
    s->verifying = false; s->transferring = false; s->transfer_count = 0;
    /* Already submitted packets retain their transfer token. The client
     * ignores them after cancellation; pairing/connection is unaffected. */
    if (s->tx_active && s->tx.kind == BC_VOICE_FILE) s->tx_active = false;
    return result;
}

void bc_voice_service_link(bc_voice_service *s, uint32_t epoch, bool connected)
{
    bool prefix_at_risk = false;
    const bc_rec_snapshot *snapshot;
    if (!s || (s->epoch == epoch && s->connected == connected)) return;
    snapshot = bc_recording_snapshot(s->recording);
    if (snapshot != NULL) {
        /* Any physical link/epoch change can lose a locally accepted prefix,
         * including a terminal file whose prior wire window was drained. */
        prefix_at_risk = snapshot->accepted_frames != 0U ||
                         s->live_prefix_count != 0U ||
                         s->live_count != 0U ||
                         (s->tx_active && s->tx.kind == BC_VOICE_LIVE);
        if (prefix_at_risk)
            s->live_disabled = true;
    }
    s->connected = false;
    (void)bc_voice_service_cancel_archive(s);
    s->epoch = epoch; s->connected = connected; s->ready = false;
    outcome_clear(s);
    s->input_read = 0U; s->input_count = 0U; s->input_sequence = 0U;
    s->input_hold_accepted = false;
    s->control_read = 0; s->control_count = 0; s->tx_active = false;
    live_transport_clear(s);
    s->stop_pending = false; s->stop_ready = false;
    s->state_pending = connected; s->state_urgent = connected;
    s->transfer_token = 0;
    memset(&s->receiver, 0, sizeof(s->receiver));
    /* The recording owner needs the physical BLE link before the READY
     * preview lease exists, so it can enqueue locally accepted frames. */
    bc_recording_link(s->recording, connected);
}

void bc_voice_service_changed(bc_voice_service *s, const bc_rec_snapshot *r)
{
    if (!s || !r) return;
    if (s->latest.phase != r->phase || s->latest.start.id != r->start.id)
        s->state_urgent = true;
    s->latest = *r;
    s->state_pending = true;
    if (s->stop_pending && r->start.id == s->stop_id &&
        r->phase != BC_REC_STARTING && r->phase != BC_REC_RECORDING && r->phase != BC_REC_STOPPING) {
        s->stop_snapshot = *r;
        s->stop_ready = true;
    }
    if (r->start.id != s->token_recording) {
        outcome_clear(s);
        s->token_recording = r->start.id;
        s->ready = false;
        s->ready_ms = s->now_ms;
        /* Exhaustion disables live for this boot instead of aliasing tokens. */
        if (s->token_counter < UINT32_MAX - 1U) s->live_token = ++s->token_counter;
        else { s->live_token = 0; s->ready = false; s->live_disabled = true; }
        live_transport_clear(s);
        s->live_disabled = s->live_token == 0U ? true : false;
        s->live_sequence = 1U;
        s->live_progress_ms = s->now_ms;
    }
    if (!s->outcome_terminal_ready && s->outcome_ready_accepted &&
        s->outcome_ready_recording_id == r->start.id &&
        s->outcome_epoch == s->epoch && outcome_terminal_valid(r)) {
        /* Keep only the first complete terminal transition. A later receipt
         * may change SAVED to DELIVERED, but must not extend this window. */
        s->outcome_recording_id = r->start.id;
        s->outcome_terminal_ms = s->now_ms;
        s->outcome_terminal_ready = true;
    }
}

bool bc_voice_service_live(bc_voice_service *s, uint64_t id, uint32_t sequence,
                            const uint8_t *data, uint16_t length)
{
    if (!s || !s->connected || s->live_disabled || s->live_token == 0U ||
        id != s->token_recording || !data || length == 0U || length > BC_REC_FRAME_MAX ||
        sequence == 0U || sequence == UINT32_MAX)
        return false;
    if (s->live_sequence == 0U || sequence != s->live_sequence ||
        !live_prefix_push(s, sequence, data, length)) {
        live_preview_disable(s);
        return false;
    }
    ++s->live_sequence;
    return true;
}

static bc_rec_result acknowledge(uint32_t *ends, uint8_t *count,
                                  uint32_t *base, uint32_t next)
{
    uint8_t i;
    if (next == *base) return BC_REC_OK;
    if (next < *base) return BC_REC_INVALID;
    for (i = 0; i < *count; ++i) {
        if (ends[i] == next) {
            ++i; *count = (uint8_t)(*count - i);
            memmove(ends, ends + i, *count * sizeof(*ends));
            *base = next;
            return BC_REC_OK;
        }
    }
    return BC_REC_INVALID;
}

static void stored_snapshot(bc_rec_snapshot *snapshot)
{
    snapshot->phase = snapshot->file.delivered ? BC_REC_DELIVERED :
        snapshot->file.bytes == 0 ? BC_REC_EMPTY :
        snapshot->file.complete ? BC_REC_SAVED : BC_REC_PARTIAL;
    snapshot->accepted_bytes = snapshot->file.bytes;
    snapshot->durable_bytes = snapshot->file.bytes;
    snapshot->accepted_frames = snapshot->file.frames;
    snapshot->error = snapshot->phase == BC_REC_PARTIAL ? BC_REC_INTERRUPTED :
        snapshot->phase == BC_REC_EMPTY ? BC_REC_EMPTY_AUDIO : BC_REC_OK;
}

static void resume_reply(bc_voice_service *s);

static void request(bc_voice_service *s, const bc_voice_message *m)
{
    bc_rec_result result = BC_REC_INVALID;
    bc_rec_snapshot snapshot = {0};
    bc_voice_message response;
    const uint8_t *p = m->payload;
    uint64_t id;
    if (m->direction != BC_VOICE_REQUEST || m->length < 4 || bc_voice_get32(p) == 0)
        return;
    /* Reserve a response slot before any command side effect. The client
     * retries stable request/recording IDs if the transport is saturated. */
    if (s->control_count == BC_VOICE_CONTROL_SLOTS) return;
    /* Replacing/cancelling staged verification also replies to the old
     * Resume. Reserve both slots before the new request can change state. */
    if (s->verifying && s->control_count >= BC_VOICE_CONTROL_SLOTS - 1U &&
        (m->kind == BC_VOICE_START || m->kind == BC_VOICE_RESUME ||
         m->kind == BC_VOICE_RECEIPT || m->kind == BC_VOICE_CANCEL)) return;
    switch (m->kind) {
    case BC_VOICE_HELLO: {
        bc_audio_format format;
        bool has_format;
        if (m->length != 4) break;
        has_format = current_format(s, &format, NULL);
        reply_init(&response, m, BC_REC_OK);
        bc_voice_put32(response.payload + 5, BC_VOICE_CAP_LOCAL | BC_VOICE_CAP_PTT |
            BC_VOICE_CAP_MEMO | BC_VOICE_CAP_LIVE | BC_VOICE_CAP_RESUME |
            BC_VOICE_CAP_CUSTODY | BC_VOICE_CAP_SETTINGS |
            BC_VOICE_CAP_TRIPLE_TAP | BC_VOICE_CAP_PTT_UNTIL_RELEASE |
            (s->inputs_port.get && s->inputs_port.set ? BC_VOICE_CAP_INPUT_MAPPINGS : 0U) |
            (s->tuning_port.get && s->tuning_port.set ? BC_VOICE_CAP_TUNING : 0U) |
            (s->outcome_port.set_outcome ? BC_VOICE_CAP_PHONE_OUTCOME : 0U) |
            (has_format ? BC_VOICE_CAP_AUDIO_FORMAT : 0U));
        bc_voice_put16(response.payload + 9, format.sample_rate_hz);
        bc_voice_put16(response.payload + 11, format.block_samples);
        bc_voice_put16(response.payload + 13, BC_REC_FRAME_MAX);
        bc_voice_put16(response.payload + 15, (uint16_t)s->recording->config.checkpoint_ms);
        bc_voice_put16(response.payload + 17, 1000);
        response.payload[19] = BC_VOICE_TRANSFER_WINDOW; response.length = 20;
        (void)queue(s, &response); return;
    }
    case BC_VOICE_FORMAT_GET:
        if (m->length != 4U) break;
        format_reply(s, m); return;
    case BC_VOICE_START: {
        bc_rec_start start;
        if (m->length != 17) break;
        start.id = bc_voice_get64(p + 4); start.trigger = (bc_rec_trigger)p[12];
        start.duration_limit_ms = bc_voice_get32(p + 13);
        if (start.trigger == BC_REC_PTT && start.duration_limit_ms != 0U) break;
        result = bc_recording_start(s->recording, &start, s->now_ms);
        snapshot_reply(s, m, result, bc_recording_snapshot(s->recording), s->live_token);
        return;
    }
    case BC_VOICE_STOP:
        if (m->length != 12) break;
        id = bc_voice_get64(p + 4);
        if (id && id != bc_recording_snapshot(s->recording)->start.id &&
            !bc_recording_active(s->recording)) {
            result = store_result(bc_rec_store_stat(s->store, id, &snapshot.start, &snapshot.file));
            if (result == BC_REC_OK || result == BC_REC_EMPTY_AUDIO) stored_snapshot(&snapshot);
            snapshot_reply(s, m, result, &snapshot, 0); return;
        }
        if (s->stop_pending && (s->stop_id != id ||
            bc_voice_get32(s->stop_request.payload) != bc_voice_get32(p))) {
            result = BC_REC_BUSY; break;
        }
        if (s->stop_pending && s->stop_ready) {
            snapshot_reply(s, m, s->stop_snapshot.error, &s->stop_snapshot, 0);
            s->stop_pending = false; s->stop_ready = false;
            return;
        }
        result = bc_recording_stop(s->recording, id, s->now_ms);
        if (bc_recording_active(s->recording) &&
            bc_recording_snapshot(s->recording)->start.id == id) {
            s->stop_pending = true; s->stop_ready = false;
            s->stop_id = id; s->stop_request = *m;
        } else snapshot_reply(s, m, result, bc_recording_snapshot(s->recording), s->live_token);
        return;
    case BC_VOICE_QUERY:
        if (m->length != 12) break;
        id = bc_voice_get64(p + 4);
        if (id == 0 || id == bc_recording_snapshot(s->recording)->start.id) {
            snapshot_reply(s, m, BC_REC_OK, bc_recording_snapshot(s->recording), s->live_token);
        } else {
            result = store_result(bc_rec_store_stat(s->store, id, &snapshot.start, &snapshot.file));
            if (result == BC_REC_OK || result == BC_REC_EMPTY_AUDIO) stored_snapshot(&snapshot);
            snapshot_reply(s, m, result, &snapshot, 0);
        }
        return;
    case BC_VOICE_READY:
        if (m->length != 5 || p[4] > 1) break;
        if (!p[4]) {
            s->ready_ms = s->now_ms;
            s->outcome_ready_accepted = false;
            s->outcome_ready_recording_id = 0U;
            s->outcome_live_token = 0U;
            s->outcome_epoch = 0U;
            if (s->token_recording != 0U)
                s->live_disabled = true;
            live_preview_disable(s);
            if (s->token_recording == 0U)
                s->live_disabled = false;
            /* READY(false) only disables the preview lease. The recording
             * owner remains linked to BLE and continues Flash capture. */
            bc_recording_link(s->recording, s->connected);
            reply_init(&response, m, BC_REC_OK);
            bc_voice_put32(response.payload + 5, s->live_token);
            bc_voice_put64(response.payload + 9, s->token_recording);
            response.length = 17U; (void)queue(s, &response); return;
        }
        if (s->live_disabled) {
            s->ready = false;
            live_transport_clear(s);
            bc_recording_link(s->recording, s->connected);
            simple_reply(s, m, BC_REC_INTERRUPTED);
            return;
        }
        if (!s->ready) {
            /* A renewed READY must not discard a frame that was already
             * accepted locally. If a previous token's message is mid-fragment,
             * put its raw frame back before rotating the token. */
            if (!live_requeue_active_tx(s)) {
                live_preview_disable(s);
                bc_recording_link(s->recording, s->connected);
                simple_reply(s, m, BC_REC_INTERRUPTED);
                return;
            }
            if (s->token_counter < UINT32_MAX - 1U) s->live_token = ++s->token_counter;
            else s->live_token = 0;
            s->live_count = 0U; s->live_ack = 0U;
            s->live_progress_ms = s->now_ms;
            s->state_pending = true; s->state_urgent = true;
        }
        s->ready = s->live_token != 0U;
        s->ready_ms = s->now_ms;
        if (!s->ready) {
            live_preview_disable(s);
            bc_recording_link(s->recording, s->connected);
            simple_reply(s, m, BC_REC_INTERRUPTED);
            return;
        }
        bc_recording_link(s->recording, s->connected);
        if (s->outcome_ready_accepted &&
            s->outcome_ready_recording_id == s->token_recording &&
            s->outcome_epoch == s->epoch) {
            /* Renewal retains the original READY admission for this link. */
        } else if (bc_recording_active(s->recording) &&
                   s->token_recording != 0U) {
            s->outcome_ready_accepted = true;
            s->outcome_ready_recording_id = s->token_recording;
            s->outcome_live_token = s->live_token;
            s->outcome_epoch = s->epoch;
        } else {
            /* READY after finalization can serve live metadata, but cannot
             * authorize a keyboard outcome for an old terminal recording. */
            s->outcome_ready_accepted = false;
            s->outcome_ready_recording_id = 0U;
            s->outcome_live_token = 0U;
            s->outcome_epoch = 0U;
        }
        reply_init(&response, m, BC_REC_OK);
        bc_voice_put32(response.payload + 5, s->live_token);
        bc_voice_put64(response.payload + 9, s->token_recording);
        response.length = 17; (void)queue(s, &response); return;
    case BC_VOICE_PHONE_OUTCOME:
        if (m->length != 17U) break;
        id = bc_voice_get64(p + 4U);
        if (p[16] != BC_VOICE_PHONE_OUTCOME_KEYBOARD_INSERTED) break;
        if (!s->outcome_port.set_outcome) {
            result = BC_REC_UNSUPPORTED;
            break;
        }
        if (bc_recording_active(s->recording) || s->stop_pending ||
            s->verifying || s->transferring) {
            result = BC_REC_BUSY;
            break;
        }
        if (!s->connected || s->epoch != s->outcome_epoch ||
            !s->outcome_ready_accepted || s->outcome_live_token == 0U ||
            bc_voice_get32(p + 12U) != s->outcome_live_token ||
            bc_voice_get32(p + 12U) != s->live_token ||
            s->outcome_ready_recording_id != id ||
            !s->outcome_terminal_ready ||
            s->outcome_recording_id != id ||
            s->latest.start.id != id ||
            !outcome_terminal_valid(&s->latest) ||
            bc_recording_snapshot(s->recording)->start.id != id) {
            result = BC_REC_WRONG_SESSION;
            break;
        }
        if ((uint32_t)(s->now_ms - s->outcome_terminal_ms) >=
            BC_VOICE_PHONE_OUTCOME_WINDOW_MS) {
            result = BC_REC_INVALID;
            break;
        }
        /* A previously accepted outcome is idempotent for this link and
         * recording. It never calls into the feedback worker twice. */
        if (s->outcome_reported) {
            result = BC_REC_OK;
            break;
        }
        /* The request path reserved a response slot before entering this
         * switch. Queue the ordinary result before changing confirmation
         * state or invoking the application callback. */
        reply_init(&response, m, BC_REC_OK);
        if (!queue(s, &response)) return;
        s->outcome_reported = true;
        s->outcome_port.set_outcome(s->outcome_port.ctx, id, p[16]);
        return;
    case BC_VOICE_LIVE_ACK:
        if (m->length != 12 || !s->ready || s->live_disabled ||
            bc_voice_get32(p + 4) != s->live_token) break;
        id = s->live_ack;
        result = acknowledge(s->live_ends, &s->live_count, &s->live_ack, bc_voice_get32(p + 8));
        if (result == BC_REC_OK && s->live_ack != id) s->live_progress_ms = s->now_ms;
        if (result == BC_REC_OK) s->ready_ms = s->now_ms;
        break;
    case BC_VOICE_SETTINGS_GET:
        if (m->length != 4) break;
        settings_reply(s, m, BC_REC_OK); return;
    case BC_VOICE_SETTINGS_SET: {
        bc_voice_settings settings;
        bc_voice_gesture_config config = s->gesture->config;
        if (m->length != 15 || p[12] > 1 || p[13] > 1 || p[14] > 1) break;
        settings.ptt_limit_ms = bc_voice_get32(p + 4);
        settings.memo_limit_ms = bc_voice_get32(p + 8);
        settings.memo_enabled = p[12] != 0; settings.led_enabled = p[13] != 0;
        settings.haptic_enabled = p[14] != 0;
        if ((s->inputs_port.get && settings.memo_enabled != s->settings.memo_enabled) ||
            settings.ptt_limit_ms != 0U || settings.memo_limit_ms > BC_REC_MAX_INTERVAL)
            break;
        if (bc_recording_active(s->recording) || s->gesture->hold_attempted || s->gesture->contact_active) {
            settings_reply(s, m, BC_REC_BUSY); return;
        }
        result = s->port.settings(s->port.ctx, &settings);
        if (result == BC_REC_OK) {
            config.ptt_limit_ms = settings.ptt_limit_ms;
            config.memo_limit_ms = settings.memo_limit_ms;
            config.memo_enabled = settings.memo_enabled;
            result = bc_voice_gesture_configure(s->gesture, &config);
            if (result == BC_REC_OK) s->settings = settings;
        }
        settings_reply(s, m, result); return;
    }
    case BC_VOICE_INPUTS_GET:
        if (m->length != 4U) break;
        inputs_reply(s, m, BC_REC_OK); return;
    case BC_VOICE_INPUTS_SET: {
        bc_voice_inputs inputs;
        if (m->length != 9U) break;
        if (!s->inputs_port.get || !s->inputs_port.set) { result = BC_REC_UNSUPPORTED; break; }
        inputs.hold_ms = (uint16_t)p[4] | ((uint16_t)p[5] << 8);
        inputs.hold_action = p[6]; inputs.double_action = p[7]; inputs.triple_action = p[8];
        if (!bc_voice_inputs_valid(&inputs)) break;
        if (bc_recording_active(s->recording) || s->gesture->hold_attempted || s->gesture->contact_active) {
            inputs_reply(s, m, BC_REC_BUSY); return;
        }
        result = s->inputs_port.set(s->inputs_port.ctx, &inputs);
        if (result == BC_REC_OK) {
            result = bc_voice_gesture_set_inputs(s->gesture, &inputs);
            if (result == BC_REC_OK) s->settings.memo_enabled = bc_voice_inputs_memo(&inputs);
        }
        inputs_reply(s, m, result); return;
    }
    case BC_VOICE_TUNING_GET:
        if (m->length != 4U) break;
        tuning_reply(s, m, BC_REC_OK); return;
    case BC_VOICE_TUNING_SET: {
        bc_voice_tuning tuning;
        if (m->length != 11U) break;
        if (!s->tuning_port.get || !s->tuning_port.set) {
            result = BC_REC_UNSUPPORTED; break;
        }
        tuning.touch_set = p[4]; tuning.touch_clear = p[5];
        tuning.haptic_strength = p[6];
        tuning.start_active_ms = (uint16_t)p[7] | ((uint16_t)p[8] << 8);
        tuning.stop_active_ms = (uint16_t)p[9] | ((uint16_t)p[10] << 8);
        if (!bc_voice_tuning_valid(&tuning)) break;
        if (bc_recording_active(s->recording) || s->gesture->hold_attempted || s->gesture->contact_active) {
            tuning_reply(s, m, BC_REC_BUSY); return;
        }
        result = s->tuning_port.set(s->tuning_port.ctx, &tuning);
        tuning_reply(s, m, result); return;
    }
    case BC_VOICE_CATALOG: {
        bc_rec_store_cursor cursor = {0}; bool verified;
        if (m->length != 12) break;
        result = store_result(bc_rec_store_catalog_begin(s->store, &cursor, bc_voice_get64(p + 4)));
        if (result == BC_REC_OK) {
            result = store_result(bc_rec_store_catalog_next(s->store, &cursor,
                                   &snapshot.start, &snapshot.file, &verified));
            if (bc_rec_store_list_end(s->store, &cursor) != BC_REC_STORE_OK) result = BC_REC_CLOSE_ERROR;
            if (result == BC_REC_OK) stored_snapshot(&snapshot);
        }
        snapshot_reply(s, m, result, &snapshot, 0); return;
    }
    case BC_VOICE_RESUME:
        if (m->length != 20 || bc_voice_get32(p + 16) == 0) break;
        if (bc_voice_get32(p + 16) < s->transfer_token ||
            (bc_voice_get32(p + 16) == s->transfer_token &&
             (bc_voice_get64(p + 4) != s->archive_start.id ||
              bc_voice_get32(p + 12) != s->transfer_offset))) break;
        if (bc_recording_active(s->recording)) { result = BC_REC_BUSY; break; }
        if (bc_voice_get32(p + 16) == s->transfer_token) {
            /* A retired attempt cannot be revived by a queued retry. New
             * reads allocate a higher token; only an active attempt is
             * idempotent under the same recording and original offset. */
            if (!s->verifying && !s->transferring) {
                result = BC_REC_CANCELLED;
                break;
            }
            /* Lost handshake response: retain bounded verification progress
             * and the current ACK window instead of starting the scan over. */
            s->resume_request = *m;
            if (s->transferring) resume_reply(s);
            return;
        }
        result = bc_voice_service_cancel_archive(s);
        if (result != BC_REC_OK) break;
        s->transfer_token = bc_voice_get32(p + 16);
        s->transfer_offset = bc_voice_get32(p + 12);
        s->transfer_next = s->transfer_offset; s->transfer_ack = s->transfer_offset;
        result = store_result(bc_rec_store_reader_begin(s->store, bc_voice_get64(p + 4),
                                  &s->reader, &s->archive_start, &s->archive_file));
        if (result != BC_REC_OK) break;
        if (s->archive_file.bytes == 0 || s->transfer_offset > s->archive_file.bytes) {
            (void)bc_voice_service_cancel_archive(s); result = BC_REC_INVALID; break;
        }
        s->resume_request = *m; s->verifying = true;
        s->archive_progress_ms = s->now_ms;
        return;
    case BC_VOICE_TRANSFER_ACK:
        if (m->length != 12 || bc_voice_get32(p + 4) != s->transfer_token)
            break;
        if (!s->transferring) {
            if (s->archive_file.bytes && s->transfer_ack == s->archive_file.bytes &&
                bc_voice_get32(p + 8) == s->transfer_ack) result = BC_REC_OK;
            break;
        }
        id = s->transfer_ack;
        result = acknowledge(s->transfer_ends, &s->transfer_count, &s->transfer_ack,
                               bc_voice_get32(p + 8));
        if (result == BC_REC_OK && s->transfer_ack != id) {
            s->transfer_progress_ms = s->now_ms;
            s->archive_progress_ms = s->now_ms;
            if (s->transfer_next < s->transfer_ack)
                s->transfer_next = s->transfer_ack;
        }
        if (result == BC_REC_OK && s->transfer_ack == s->archive_file.bytes)
            result = bc_voice_service_cancel_archive(s);
        break;
    case BC_VOICE_RECEIPT:
        if (m->length != 21 || p[20] > 1) break;
        if (bc_recording_active(s->recording)) { result = BC_REC_BUSY; break; }
        result = bc_voice_service_cancel_archive(s);
        if (result != BC_REC_OK) break;
        result = store_result(bc_rec_store_receipt(s->store, bc_voice_get64(p + 4),
                                  bc_voice_get32(p + 12), bc_voice_get32(p + 16)));
        if (result == BC_REC_OK)
            (void)bc_recording_delivered(s->recording, bc_voice_get64(p + 4),
                                          bc_voice_get32(p + 12), bc_voice_get32(p + 16));
        if (result == BC_REC_OK && p[20])
            result = store_result(bc_rec_store_delete(s->store, bc_voice_get64(p + 4),
                                      bc_voice_get32(p + 12), bc_voice_get32(p + 16)));
        break;
    case BC_VOICE_CANCEL:
        if (m->length != 8 || bc_voice_get32(p + 4) != s->transfer_token) break;
        result = bc_voice_service_cancel_archive(s); break;
    default: result = BC_REC_UNSUPPORTED; break;
    }
    simple_reply(s, m, result);
}

void bc_voice_service_receive(bc_voice_service *s, uint32_t epoch,
                               const uint8_t *packet, uint16_t length, uint32_t now_ms)
{
    bc_voice_message message;
    if (!s || !s->connected || epoch != s->epoch) return;
    s->now_ms = now_ms;
    if (bc_voice_receive(&s->receiver, epoch, now_ms, packet, length, &message) == BC_WIRE_MESSAGE)
        request(s, &message);
}

static void resume_reply(bc_voice_service *s)
{
    bc_voice_message response;
    reply_init(&response, &s->resume_request, BC_REC_OK);
    bc_voice_put64(response.payload + 5, s->archive_start.id);
    bc_voice_put32(response.payload + 13, s->transfer_token);
    bc_voice_put32(response.payload + 17, s->archive_file.bytes);
    bc_voice_put32(response.payload + 21, s->archive_file.crc32);
    bc_voice_put32(response.payload + 25, s->transfer_offset);
    response.length = 29; (void)queue(s, &response);
}

static void archive_tick(bc_voice_service *s)
{
    /* Bound both a verification starved of response slots and an abandoned
     * transfer, including a blocked first fragment or a resume at EOF. */
    if ((s->verifying || s->transferring) &&
        (uint32_t)(s->now_ms - s->archive_progress_ms) >= BC_VOICE_ARCHIVE_STALL_MS) {
        (void)bc_voice_service_cancel_archive(s);
        return;
    }
    if (s->verifying && s->control_count < BC_VOICE_CONTROL_SLOTS) {
        uint32_t before = s->reader.verify_offset;
        bc_rec_store_status status = bc_rec_store_reader_verify_step(&s->reader, 1024U);
        if (s->reader.verify_offset != before)
            s->archive_progress_ms = s->now_ms;
        if (status == BC_REC_STORE_OK) {
            s->verifying = false; s->transferring = true;
            s->transfer_progress_ms = s->now_ms;
            s->archive_progress_ms = s->now_ms;
            resume_reply(s);
        } else if (status != BC_REC_STORE_MORE) {
            s->verifying = false;
            simple_reply(s, &s->resume_request, store_result(status));
            (void)bc_voice_service_cancel_archive(s);
        }
    }
    if (s->transferring && s->transfer_count &&
        (uint32_t)(s->now_ms - s->transfer_progress_ms) >= BC_VOICE_RETRY_MS) {
        /* Rewind only to the phone's acknowledged offset. In-progress framed
         * messages finish first, so fragments from two messages never mix. */
        if (!s->tx_active) {
            /* Preserve the sent boundaries: a delayed ACK can still prove
             * progress anywhere in this outstanding window during replay. */
            s->transfer_next = s->transfer_ack;
            s->transfer_progress_ms = s->now_ms;
        }
    }
}

bool bc_voice_service_poll(bc_voice_service *s, uint32_t now_ms, uint16_t att_limit)
{
    uint8_t packet[BC_VOICE_PACKET_MAX];
    uint16_t length, next;
    unsigned burst;
    bool sent = false;
    if (!s) return false;
    s->now_ms = now_ms;
    if (s->ready && ((uint32_t)(now_ms - s->ready_ms) >= BC_VOICE_READY_MS ||
        (s->live_count && (uint32_t)(now_ms - s->live_progress_ms) >= BC_VOICE_LIVE_STALL_MS))) {
        live_preview_disable(s);
        /* A lease timeout stops preview delivery only; local capture remains
         * linked to the physical BLE connection and continues into Flash. */
        bc_recording_link(s->recording, s->connected);
    }
    while (s->input_count &&
        (uint32_t)(now_ms - s->input_events[s->input_read].at_ms) >= BC_VOICE_INPUT_EVENT_TTL_MS) {
        s->input_read = (uint8_t)((s->input_read + 1U) % BC_VOICE_INPUT_EVENT_SLOTS);
        --s->input_count;
    }
    if (s->tx_active && s->tx.kind == BC_VOICE_INPUT_EVENT &&
        (uint32_t)(now_ms - s->input_tx_ms) >= BC_VOICE_INPUT_EVENT_TTL_MS) s->tx_active = false;
    if (!s->connected) return false;
    if (s->stop_pending && (s->stop_ready || !bc_recording_active(s->recording)) &&
        s->control_count < BC_VOICE_CONTROL_SLOTS) {
        const bc_rec_snapshot *snapshot = s->stop_ready ? &s->stop_snapshot : bc_recording_snapshot(s->recording);
        snapshot_reply(s, &s->stop_request,
            snapshot->start.id == s->stop_id ? snapshot->error : BC_REC_WRONG_SESSION,
            snapshot, snapshot->start.id == s->token_recording ? s->live_token : 0);
        s->stop_pending = false; s->stop_ready = false;
    }
    if (!bc_recording_active(s->recording)) archive_tick(s);
    if (!s->tx_active) {
        memset(&s->tx, 0, sizeof(s->tx));
        if (s->control_count) {
            s->tx = s->controls[s->control_read];
            s->control_read = (uint8_t)((s->control_read + 1) % BC_VOICE_CONTROL_SLOTS);
            --s->control_count;
        } else if (s->state_pending && (s->state_urgent ||
                       (s->live_prefix_count == 0U &&
                        (uint32_t)(now_ms - s->state_sent_ms) >= 500U))) {
            s->tx.message_id = message_id(s); s->tx.kind = BC_VOICE_STATE;
            s->tx.direction = BC_VOICE_EVENT;
            encode_snapshot(&s->tx, &s->latest, s->live_token);
            s->state_pending = false; s->state_urgent = false; s->state_sent_ms = now_ms;
        } else if (s->input_count) {
            bc_voice_input_event *event = &s->input_events[s->input_read];
            s->tx.message_id = message_id(s); s->tx.kind = BC_VOICE_INPUT_EVENT;
            s->tx.direction = BC_VOICE_EVENT; s->tx.length = 8U;
            bc_voice_put32(s->tx.payload, event->sequence);
            s->tx.payload[4] = event->input; s->tx.payload[5] = event->phase;
            s->tx.payload[6] = BC_VOICE_INPUT_APP; s->tx.payload[7] = 0U;
            s->input_tx_ms = event->at_ms;
            s->input_read = (uint8_t)((s->input_read + 1U) % BC_VOICE_INPUT_EVENT_SLOTS);
            --s->input_count;
        } else if (s->ready && !s->live_disabled && s->live_count < 4U &&
                   s->live_prefix_count != 0U) {
            bc_voice_live_frame *frame = &s->live_prefix[s->live_prefix_read];
            s->tx.message_id = message_id(s); s->tx.kind = BC_VOICE_LIVE;
            s->tx.direction = BC_VOICE_EVENT; s->tx.length = (uint16_t)(8U + frame->length);
            bc_voice_put32(s->tx.payload, s->live_token);
            bc_voice_put32(s->tx.payload + 4U, frame->sequence);
            memcpy(s->tx.payload + 8U, frame->data, frame->length);
            s->live_prefix_read = (uint8_t)((s->live_prefix_read + 1U) %
                                            BC_VOICE_LIVE_PREFIX_SLOTS);
            --s->live_prefix_count;
        } else if (s->transferring && !bc_recording_active(s->recording) &&
                    (s->transfer_count < BC_VOICE_TRANSFER_WINDOW ||
                     s->transfer_next < s->transfer_ends[s->transfer_count - 1U]) &&
                    s->transfer_next < s->archive_file.bytes) {
            int32_t read_count;
            s->tx.message_id = message_id(s); s->tx.kind = BC_VOICE_FILE;
            s->tx.direction = BC_VOICE_EVENT;
            bc_voice_put32(s->tx.payload, s->transfer_token);
            bc_voice_put32(s->tx.payload + 4, s->transfer_next);
            read_count = bc_rec_store_reader_read(&s->reader, s->transfer_next,
                                                   s->tx.payload + 8, BC_REC_FRAME_MAX);
            if (read_count <= 0 || (uint32_t)read_count > BC_REC_FRAME_MAX) {
                simple_reply(s, &s->resume_request, read_count < 0 ?
                    store_result((bc_rec_store_status)read_count) : BC_REC_CRC_ERROR);
                (void)bc_voice_service_cancel_archive(s); return false;
            }
            s->tx.length = (uint16_t)(8 + read_count);
        } else return false;
        s->tx_active = true; s->tx_offset = 0;
    }
    /* Batch only this message's fragments. Never perform another archive
     * read or start another message before the worker checks capture/touch. */
    for (burst = 0U; burst < BC_VOICE_TX_BURST; ++burst) {
        length = bc_voice_fragment(&s->tx, s->tx_offset, att_limit, packet, sizeof(packet), &next);
        if (length == 0 || !s->port.send(s->port.ctx, packet, length, s->epoch)) return sent;
        sent = true;
        s->tx_offset = next;
        if (next != s->tx.length + 4U) continue;
        if (s->tx.kind == BC_VOICE_LIVE && s->ready && !s->live_disabled &&
            bc_voice_get32(s->tx.payload) == s->live_token && s->live_count < 4U) {
            if (s->live_count == 0) s->live_progress_ms = now_ms;
            s->live_ends[s->live_count++] = bc_voice_get32(s->tx.payload + 4) + 1U;
        } else if (s->tx.kind == BC_VOICE_FILE && s->transferring &&
            bc_voice_get32(s->tx.payload) == s->transfer_token) {
            uint32_t end = bc_voice_get32(s->tx.payload + 4) + s->tx.length - 8U;
            uint32_t high = s->transfer_count ?
                s->transfer_ends[s->transfer_count - 1U] : s->transfer_ack;
            /* A delayed ACK may overtake an in-flight retransmission. Finish
             * its framing, but do not regress the cursor or re-add its end. */
            if (s->transfer_next < end) s->transfer_next = end;
            if (s->transfer_next < s->transfer_ack) s->transfer_next = s->transfer_ack;
            if (end > high && s->transfer_count < BC_VOICE_TRANSFER_WINDOW)
                s->transfer_ends[s->transfer_count++] = end;
        }
        s->tx_active = false;
        break;
    }
    return sent;
}
