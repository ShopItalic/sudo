#ifndef BC_RECORDING_H
#define BC_RECORDING_H

#include <stdbool.h>
#include <stdint.h>

/* Allocation-free recording owner, shared by the device adapter and emulator.
 * All calls run on one worker. Ports must not reenter the owner; ISR/capture
 * callbacks post session-tagged events to that worker instead. Time is ms. */
#define BC_REC_FRAME_MAX 220U
#define BC_REC_NAME_SIZE 40U
#define BC_REC_MAX_INTERVAL 0x7fffffffUL

typedef enum {
    BC_REC_IDLE = 0, BC_REC_STARTING, BC_REC_RECORDING, BC_REC_STOPPING,
    BC_REC_SAVED, BC_REC_PARTIAL, BC_REC_FAILED, BC_REC_EMPTY, BC_REC_DELIVERED
} bc_rec_phase;

typedef enum {
    BC_REC_OK = 0, BC_REC_INVALID, BC_REC_BUSY, BC_REC_WRONG_SESSION,
    BC_REC_DUPLICATE, BC_REC_NO_SPACE, BC_REC_OPEN_ERROR, BC_REC_WRITE_ERROR,
    BC_REC_SYNC_ERROR, BC_REC_CLOSE_ERROR, BC_REC_CAPTURE_ERROR,
    BC_REC_CAPTURE_OVERFLOW, BC_REC_SEQUENCE_GAP, BC_REC_STOP_TIMEOUT,
    BC_REC_ALREADY_EXISTS, BC_REC_EMPTY_AUDIO, BC_REC_INTERRUPTED,
    BC_REC_TOUCH_ERROR, BC_REC_NOT_FOUND, BC_REC_CUSTODY_REQUIRED,
    BC_REC_UNSUPPORTED, BC_REC_CANCELLED, BC_REC_CRC_ERROR
} bc_rec_result;

typedef enum { BC_REC_PTT = 1, BC_REC_MEMO = 2, BC_REC_APP = 3 } bc_rec_trigger;

typedef struct {
    /* Persistent recording identity, also the idempotency key for Start.
     * The adapter supplies a nonzero, collision-checked ID for gestures and
     * the client supplies a stable ID for a retried app Start. */
    uint64_t id;
    bc_rec_trigger trigger;
    uint32_t duration_limit_ms; /* 0: end only on Stop/fault. */
} bc_rec_start;

typedef struct {
    char name[BC_REC_NAME_SIZE]; /* NUL-terminated legacy raw-audio export. */
    uint32_t bytes;
    uint32_t frames;
    uint32_t crc32; /* CRC-32/ISO-HDLC of raw bytes (same value as zlib crc32). */
    bool complete;
    bool recovered;
    bool delivered; /* Persisted phone receipt; raw file may have been retired. */
} bc_rec_file;

typedef struct {
    bc_rec_start start;
    bc_rec_phase phase;
    bc_rec_result error;
    uint32_t accepted_bytes;
    uint32_t accepted_frames;
    uint32_t durable_bytes;
    uint32_t live_sent_frames;
    uint32_t live_dropped_frames;
    uint32_t duplicate_frames;
    uint32_t revision;
    bc_rec_file file;
} bc_rec_snapshot;

typedef struct {
    uint32_t checkpoint_ms;
    uint32_t checkpoint_bytes;
    uint32_t stop_timeout_ms;
} bc_rec_config;

typedef struct {
    void *ctx;
    /* Open never truncates an existing file. ALREADY_EXISTS returns that
     * identity's recovered/finalized metadata, and must verify its original
     * Start parameters. An active recording owned elsewhere returns BUSY. */
    bc_rec_result (*open)(void *ctx, const bc_rec_start *start, bc_rec_file *file);
    bc_rec_result (*append)(void *ctx, const uint8_t *data, uint16_t length);
    /* A successful checkpoint covers all preceding successful appends. */
    bc_rec_result (*checkpoint)(void *ctx, bc_rec_file *file);
    /* Always release the storage handle, even on error, retaining recoverable
     * originals. Only complete=true may write a verified completion marker.
     * Returned bytes/frames/checksum describe a verified prefix. */
    bc_rec_result (*finish)(void *ctx, bool complete, bc_rec_file *file);
    /* A failed Start must leave the capture hardware quiescent. */
    bc_rec_result (*capture_start)(void *ctx, uint64_t id);
    /* Stop is asynchronous. Deliver all complete frames for this ID before
     * calling bc_recording_drained. Do not clear a capture/encoder queue. */
    bc_rec_result (*capture_stop)(void *ctx, uint64_t id);
    /* Abort returns true only when capture is quiescent. Lost/uncertain audio
     * remains a PARTIAL recording; this is never normal successful Stop. */
    bool (*capture_abort)(void *ctx, uint64_t id);
    /* Live/event delivery must be nonblocking. A failed live write changes
     * diagnostics only; local recording remains the owner of the audio. */
    bool (*live)(void *ctx, uint64_t id, uint32_t sequence,
                 const uint8_t *data, uint16_t length);
    void (*changed)(void *ctx, const bc_rec_snapshot *snapshot);
} bc_rec_port;

typedef struct {
    bc_rec_snapshot snapshot;
    bc_rec_config config;
    bc_rec_port port;
    uint32_t started_ms;
    uint32_t checkpoint_ms;
    uint32_t stop_ms;
    uint32_t crc32_state;
    bool opened;
    bool capture_active;
    bool storage_failed;
    bool link_ready;
    bool timeout_reported;
    bool initialized;
} bc_recording;

bool bc_recording_init(bc_recording *rec, const bc_rec_port *port,
                       const bc_rec_config *config);
bc_rec_result bc_recording_start(bc_recording *rec, const bc_rec_start *start,
                                 uint32_t now_ms);
/* Replayed Start/Stop returns the stored error (or OK) and current snapshot.
 * ALREADY_EXISTS is a storage-port result, never a request to reopen capture. */
bc_rec_result bc_recording_stop(bc_recording *rec, uint64_t id, uint32_t now_ms);
bc_rec_result bc_recording_frame(bc_recording *rec, uint64_t id,
                                 uint32_t sequence, const uint8_t *data,
                                 uint16_t length, uint32_t now_ms);
bc_rec_result bc_recording_drained(bc_recording *rec, uint64_t id);
bc_rec_result bc_recording_fault(bc_recording *rec, uint64_t id,
                                 bc_rec_result error, uint32_t now_ms);
void bc_recording_tick(bc_recording *rec, uint32_t now_ms);
void bc_recording_link(bc_recording *rec, bool ready);
const bc_rec_snapshot *bc_recording_snapshot(const bc_recording *rec);
bool bc_recording_active(const bc_recording *rec);
/* Reflect a successfully persisted exact phone receipt in the current
 * terminal snapshot. The storage adapter must commit custody first. */
bc_rec_result bc_recording_delivered(bc_recording *rec, uint64_t id,
                                      uint32_t bytes, uint32_t crc32);

#endif
