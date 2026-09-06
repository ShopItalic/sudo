#include "bc_recording.h"

#include <limits.h>
#include <string.h>

static bool valid(const bc_recording *rec)
{
    return rec != NULL && rec->initialized;
}

static bool name_valid(const bc_rec_file *file)
{
    return file->name[0] != '\0' &&
           memchr(file->name, '\0', sizeof(file->name)) != NULL;
}

static void changed(bc_recording *rec)
{
    ++rec->snapshot.revision;
    if (rec->port.changed != NULL)
        rec->port.changed(rec->port.ctx, &rec->snapshot);
}

static void error_first(bc_recording *rec, bc_rec_result error)
{
    if (rec->snapshot.error == BC_REC_OK)
        rec->snapshot.error = error;
}

static uint32_t crc_update(uint32_t state, const uint8_t *data, uint16_t length)
{
    uint16_t i;
    for (i = 0; i < length; ++i) {
        unsigned bit;
        state ^= data[i];
        for (bit = 0; bit < 8U; ++bit)
            state = (state >> 1) ^ (0xedb88320UL & (0U - (state & 1U)));
    }
    return state;
}

/* Storage reports a verified prefix, never additional accepted audio or a
 * different file. At checkpoints and successful Stop the whole prefix must
 * equal the owner's byte count, frame count, and independently computed CRC. */
static bool metadata_valid(const bc_recording *rec, const bc_rec_file *file,
                           bool entire)
{
    if (!name_valid(file) || !name_valid(&rec->snapshot.file) || file->delivered ||
        strcmp(file->name, rec->snapshot.file.name) != 0 ||
        file->bytes > rec->snapshot.accepted_bytes ||
        file->frames > rec->snapshot.accepted_frames ||
        ((file->bytes == 0U) != (file->frames == 0U)) ||
        file->frames > file->bytes ||
        (uint64_t)file->bytes > (uint64_t)file->frames * BC_REC_FRAME_MAX)
        return false;
    return !entire || (file->bytes == rec->snapshot.accepted_bytes &&
                       file->frames == rec->snapshot.accepted_frames &&
                       file->crc32 == (rec->crc32_state ^ 0xffffffffUL));
}

bool bc_recording_active(const bc_recording *rec)
{
    return valid(rec) && (rec->snapshot.phase == BC_REC_STARTING ||
                         rec->snapshot.phase == BC_REC_RECORDING ||
                         rec->snapshot.phase == BC_REC_STOPPING);
}

const bc_rec_snapshot *bc_recording_snapshot(const bc_recording *rec)
{
    return valid(rec) ? &rec->snapshot : NULL;
}

bool bc_recording_init(bc_recording *rec, const bc_rec_port *port,
                       const bc_rec_config *config)
{
    if (rec == NULL || port == NULL || config == NULL ||
        port->open == NULL || port->append == NULL || port->checkpoint == NULL ||
        port->finish == NULL || port->capture_start == NULL ||
        port->capture_stop == NULL || port->capture_abort == NULL ||
        config->stop_timeout_ms == 0U ||
        config->stop_timeout_ms > BC_REC_MAX_INTERVAL ||
        config->checkpoint_ms > BC_REC_MAX_INTERVAL ||
        (config->checkpoint_ms == 0U && config->checkpoint_bytes == 0U))
        return false;
    memset(rec, 0, sizeof(*rec));
    rec->config = *config;
    rec->port = *port;
    rec->crc32_state = 0xffffffffUL;
    rec->initialized = true;
    return true;
}

static bc_rec_result finalize(bc_recording *rec)
{
    bc_rec_file file = rec->snapshot.file;
    bool complete = rec->snapshot.error == BC_REC_OK;
    bc_rec_result result;
    if (!rec->opened || rec->capture_active)
        return BC_REC_BUSY;
    result = rec->port.finish(rec->port.ctx, complete, &file);
    rec->opened = false;
    if (result == BC_REC_EMPTY_AUDIO && rec->snapshot.accepted_bytes != 0U)
        result = BC_REC_CLOSE_ERROR;
    if (result != BC_REC_OK)
        error_first(rec, result);
    if (!metadata_valid(rec, &file, complete && result == BC_REC_OK) ||
        (complete && result == BC_REC_OK && (!file.complete || file.recovered))) {
        error_first(rec, BC_REC_CLOSE_ERROR);
        /* Keep the last verified checkpoint instead of publishing untrusted
         * metadata. The storage adapter retains the original for recovery. */
        file = rec->snapshot.file;
    }
    rec->snapshot.file = file;
    rec->snapshot.durable_bytes = file.bytes;
    if (file.bytes == 0U && rec->snapshot.accepted_bytes == 0U &&
        (rec->snapshot.error == BC_REC_OK || rec->snapshot.error == BC_REC_EMPTY_AUDIO)) {
        rec->snapshot.file.complete = false;
        rec->snapshot.error = BC_REC_EMPTY_AUDIO;
        rec->snapshot.phase = BC_REC_EMPTY;
    } else if (rec->snapshot.error != BC_REC_OK) {
        rec->snapshot.file.complete = false;
        rec->snapshot.phase = BC_REC_PARTIAL;
    } else {
        rec->snapshot.phase = BC_REC_SAVED;
    }
    changed(rec);
    return rec->snapshot.error;
}

static bc_rec_result start_failed(bc_recording *rec, bc_rec_result error)
{
    bc_rec_file ignored = rec->snapshot.file;
    rec->snapshot.error = error;
    if (rec->opened) {
        /* No microphone has started. Release a possibly-created destination
         * without claiming that a failed Start produced a complete clip. */
        (void)rec->port.finish(rec->port.ctx, false, &ignored);
        rec->opened = false;
    }
    rec->snapshot.phase = BC_REC_FAILED;
    rec->snapshot.file.complete = false;
    changed(rec);
    return error;
}

static bool start_equal(const bc_rec_start *a, const bc_rec_start *b)
{
    return a->id == b->id && a->trigger == b->trigger &&
           a->duration_limit_ms == b->duration_limit_ms;
}

bc_rec_result bc_recording_start(bc_recording *rec, const bc_rec_start *start,
                                 uint32_t now_ms)
{
    bc_rec_result result;
    uint32_t revision;
    if (!valid(rec) || start == NULL || start->id == 0U ||
        start->trigger < BC_REC_PTT || start->trigger > BC_REC_APP ||
        start->duration_limit_ms > BC_REC_MAX_INTERVAL)
        return BC_REC_INVALID;
    if (rec->snapshot.start.id == start->id) {
        if (!start_equal(&rec->snapshot.start, start))
            return BC_REC_INVALID;
        return rec->snapshot.error;
    }
    if (bc_recording_active(rec))
        return BC_REC_BUSY;
    revision = rec->snapshot.revision;
    memset(&rec->snapshot, 0, sizeof(rec->snapshot));
    rec->snapshot.start = *start;
    rec->snapshot.revision = revision;
    rec->snapshot.phase = BC_REC_STARTING;
    rec->started_ms = now_ms;
    rec->checkpoint_ms = now_ms;
    rec->storage_failed = false;
    rec->timeout_reported = false;
    rec->crc32_state = 0xffffffffUL;
    changed(rec);
    result = rec->port.open(rec->port.ctx, start, &rec->snapshot.file);
    if (result == BC_REC_ALREADY_EXISTS || result == BC_REC_EMPTY_AUDIO) {
        bc_rec_file *file = &rec->snapshot.file;
        if (!name_valid(file) || (result == BC_REC_EMPTY_AUDIO && file->bytes != 0U) ||
            ((file->bytes == 0U) != (file->frames == 0U)) ||
            file->frames > file->bytes ||
            (uint64_t)file->bytes > (uint64_t)file->frames * BC_REC_FRAME_MAX ||
            (file->delivered && file->bytes == 0U) ||
            (file->complete && file->recovered))
            return start_failed(rec, BC_REC_OPEN_ERROR);
        rec->snapshot.accepted_bytes = file->bytes;
        rec->snapshot.accepted_frames = file->frames;
        rec->snapshot.durable_bytes = file->bytes;
        if (file->bytes == 0U) {
            rec->snapshot.phase = BC_REC_EMPTY;
            rec->snapshot.error = BC_REC_EMPTY_AUDIO;
        } else if (!file->complete) {
            rec->snapshot.phase = BC_REC_PARTIAL;
            rec->snapshot.error = BC_REC_INTERRUPTED;
        } else {
            rec->snapshot.phase = BC_REC_SAVED;
        }
        if (file->delivered)
            rec->snapshot.phase = BC_REC_DELIVERED;
        changed(rec);
        return rec->snapshot.error;
    }
    if (result != BC_REC_OK)
        return start_failed(rec, result);
    rec->opened = true;
    if (!name_valid(&rec->snapshot.file) || rec->snapshot.file.bytes != 0U ||
        rec->snapshot.file.frames != 0U || rec->snapshot.file.crc32 != 0U ||
        rec->snapshot.file.complete || rec->snapshot.file.recovered || rec->snapshot.file.delivered)
        return start_failed(rec, BC_REC_OPEN_ERROR);
    result = rec->port.capture_start(rec->port.ctx, start->id);
    if (result != BC_REC_OK)
        return start_failed(rec, result);
    rec->capture_active = true;
    rec->snapshot.phase = BC_REC_RECORDING;
    changed(rec);
    return BC_REC_OK;
}

bc_rec_result bc_recording_stop(bc_recording *rec, uint64_t id, uint32_t now_ms)
{
    bc_rec_result result;
    if (!valid(rec) || id == 0U)
        return BC_REC_INVALID;
    if (id != rec->snapshot.start.id)
        return BC_REC_WRONG_SESSION;
    if (rec->snapshot.phase == BC_REC_STOPPING || !bc_recording_active(rec))
        return rec->snapshot.error;
    if (rec->snapshot.phase != BC_REC_RECORDING)
        return BC_REC_BUSY;
    rec->snapshot.phase = BC_REC_STOPPING;
    rec->stop_ms = now_ms;
    result = rec->port.capture_stop(rec->port.ctx, id);
    if (result != BC_REC_OK)
        error_first(rec, result);
    changed(rec);
    /* Even a failed stop request leaves capture/storage owned by this ID.
     * Only a drain event or a successful forced abort permits finalization. */
    return rec->snapshot.error;
}

bc_rec_result bc_recording_fault(bc_recording *rec, uint64_t id,
                                 bc_rec_result error, uint32_t now_ms)
{
    if (!valid(rec) || id == 0U || error <= BC_REC_OK ||
        error > BC_REC_CRC_ERROR || error == BC_REC_ALREADY_EXISTS ||
        error == BC_REC_DUPLICATE)
        return BC_REC_INVALID;
    if (rec->snapshot.start.id != id)
        return BC_REC_WRONG_SESSION;
    if (!bc_recording_active(rec))
        return BC_REC_BUSY;
    error_first(rec, error);
    if (error == BC_REC_WRITE_ERROR || error == BC_REC_SYNC_ERROR ||
        error == BC_REC_NO_SPACE)
        rec->storage_failed = true;
    if (rec->snapshot.phase == BC_REC_RECORDING)
        (void)bc_recording_stop(rec, id, now_ms);
    else
        changed(rec);
    return rec->snapshot.error;
}

static bc_rec_result checkpoint(bc_recording *rec, uint32_t now_ms)
{
    bc_rec_file file = rec->snapshot.file;
    bc_rec_result result = rec->port.checkpoint(rec->port.ctx, &file);
    if (result == BC_REC_OK &&
        (!metadata_valid(rec, &file, true) || file.complete || file.recovered))
        result = BC_REC_SYNC_ERROR;
    if (result != BC_REC_OK) {
        rec->storage_failed = true;
        return bc_recording_fault(rec, rec->snapshot.start.id, result, now_ms);
    }
    rec->snapshot.file = file;
    rec->snapshot.durable_bytes = file.bytes;
    rec->checkpoint_ms = now_ms;
    changed(rec);
    return BC_REC_OK;
}

bc_rec_result bc_recording_frame(bc_recording *rec, uint64_t id,
                                 uint32_t sequence, const uint8_t *data,
                                 uint16_t length, uint32_t now_ms)
{
    bc_rec_result result;
    if (!valid(rec) || id == 0U)
        return BC_REC_INVALID;
    if (id != rec->snapshot.start.id)
        return BC_REC_WRONG_SESSION;
    if (rec->snapshot.phase != BC_REC_RECORDING && rec->snapshot.phase != BC_REC_STOPPING)
        return BC_REC_BUSY;
    if (length > BC_REC_FRAME_MAX)
        return bc_recording_fault(rec, id, BC_REC_CAPTURE_OVERFLOW, now_ms);
    if (data == NULL || length == 0U || sequence == 0U)
        return bc_recording_fault(rec, id, BC_REC_INVALID, now_ms);
    if (sequence <= rec->snapshot.accepted_frames) {
        ++rec->snapshot.duplicate_frames;
        return BC_REC_DUPLICATE;
    }
    if (rec->snapshot.accepted_frames == UINT32_MAX ||
        sequence != rec->snapshot.accepted_frames + 1U)
        return bc_recording_fault(rec, id, BC_REC_SEQUENCE_GAP, now_ms);
    if (rec->storage_failed)
        return rec->snapshot.error;
    if (rec->snapshot.accepted_bytes > UINT32_MAX - length)
        return bc_recording_fault(rec, id, BC_REC_NO_SPACE, now_ms);
    result = rec->port.append(rec->port.ctx, data, length);
    if (result != BC_REC_OK) {
        rec->storage_failed = true;
        return bc_recording_fault(rec, id, result, now_ms);
    }
    rec->crc32_state = crc_update(rec->crc32_state, data, length);
    rec->snapshot.accepted_bytes += length;
    ++rec->snapshot.accepted_frames;
    if ((rec->config.checkpoint_bytes != 0U &&
         rec->snapshot.accepted_bytes - rec->snapshot.durable_bytes >= rec->config.checkpoint_bytes) ||
        (rec->config.checkpoint_ms != 0U &&
         (uint32_t)(now_ms - rec->checkpoint_ms) >= rec->config.checkpoint_ms)) {
        result = checkpoint(rec, now_ms);
        if (result != BC_REC_OK)
            return result;
    }
    if (rec->link_ready && rec->port.live != NULL) {
        if (rec->port.live(rec->port.ctx, id, sequence, data, length))
            ++rec->snapshot.live_sent_frames;
        else
            ++rec->snapshot.live_dropped_frames;
    }
    return BC_REC_OK;
}

bc_rec_result bc_recording_delivered(bc_recording *rec, uint64_t id,
                                      uint32_t bytes, uint32_t crc32)
{
    if (!valid(rec) || !id || !bytes) return BC_REC_INVALID;
    if (rec->snapshot.start.id != id) return BC_REC_WRONG_SESSION;
    if (bc_recording_active(rec)) return BC_REC_BUSY;
    if (rec->snapshot.file.bytes != bytes || rec->snapshot.file.crc32 != crc32)
        return BC_REC_CUSTODY_REQUIRED;
    if (!rec->snapshot.file.delivered) {
        rec->snapshot.file.delivered = true;
        rec->snapshot.phase = BC_REC_DELIVERED;
        changed(rec);
    }
    return BC_REC_OK;
}

bc_rec_result bc_recording_drained(bc_recording *rec, uint64_t id)
{
    if (!valid(rec) || id == 0U)
        return BC_REC_INVALID;
    if (rec->snapshot.start.id != id)
        return BC_REC_WRONG_SESSION;
    if (rec->snapshot.phase != BC_REC_STOPPING)
        return bc_recording_active(rec) ? BC_REC_BUSY : rec->snapshot.error;
    rec->capture_active = false;
    return finalize(rec);
}

void bc_recording_tick(bc_recording *rec, uint32_t now_ms)
{
    if (!valid(rec))
        return;
    if (rec->snapshot.phase == BC_REC_RECORDING &&
        rec->snapshot.start.duration_limit_ms != 0U &&
        (uint32_t)(now_ms - rec->started_ms) >= rec->snapshot.start.duration_limit_ms)
        (void)bc_recording_stop(rec, rec->snapshot.start.id, now_ms);
    if (rec->snapshot.phase == BC_REC_STOPPING &&
        (uint32_t)(now_ms - rec->stop_ms) >= rec->config.stop_timeout_ms) {
        error_first(rec, BC_REC_STOP_TIMEOUT);
        if (rec->port.capture_abort(rec->port.ctx, rec->snapshot.start.id)) {
            rec->capture_active = false;
            (void)finalize(rec);
        } else {
            /* Back off retries while remaining busy. A failed abort cannot
             * turn an unknown capture state into a completed recording. */
            rec->stop_ms = now_ms;
            if (!rec->timeout_reported) {
                rec->timeout_reported = true;
                changed(rec);
            }
        }
    } else if (bc_recording_active(rec) && rec->opened && !rec->storage_failed &&
               rec->snapshot.accepted_bytes != rec->snapshot.durable_bytes &&
               rec->config.checkpoint_ms != 0U &&
               (uint32_t)(now_ms - rec->checkpoint_ms) >= rec->config.checkpoint_ms) {
        (void)checkpoint(rec, now_ms);
    }
}

void bc_recording_link(bc_recording *rec, bool ready)
{
    if (valid(rec))
        rec->link_ready = ready;
}
