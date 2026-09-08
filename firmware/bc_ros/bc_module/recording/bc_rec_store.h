#ifndef BC_REC_STORE_H
#define BC_REC_STORE_H

#include <stdbool.h>
#include <stdint.h>

#include "../file/LittleFS/lfs.h"
#include "bc_recording.h"

/*
 * Persistent recordings are deliberately kept in a private directory.  The
 * path is derived only from the binary recording id; the legacy export name
 * is metadata and is never used as a path component.
 */
#define BC_REC_STORE_DIR "/.sudo-rec"
#define BC_REC_STORE_META_ATTR 0xa5U
#define BC_REC_STORE_CACHE_SIZE 256U
/* Metadata version 1 (S01-S04) is 88 bytes; version 2 appends the 16-byte
 * audio descriptor and moves the checksum. Buffers hold the larger size and
 * LittleFS zero-fills the tail of an older attribute on read. */
#define BC_REC_STORE_METADATA_V1_SIZE 88U
#define BC_REC_STORE_METADATA_SIZE 104U
#define BC_REC_STORE_PATH_SIZE 64U

/* The callback must write a NUL-terminated legacy export name into name. */
typedef bool (*bc_rec_store_namer)(void *ctx, const bc_rec_start *start,
                                   char name[BC_REC_NAME_SIZE]);

/* Read-only APIs return these statuses.  The negative values are also safe
 * to return from bc_rec_store_read, whose non-negative result is a byte
 * count. */
typedef enum {
    BC_REC_STORE_OK = 0,
    BC_REC_STORE_END = 1,
    BC_REC_STORE_MORE = 2,
    BC_REC_STORE_NOT_FOUND = -2,
    BC_REC_STORE_INVALID = -22,
    BC_REC_STORE_BUSY = -16,
    BC_REC_STORE_DUPLICATE = -17,
    BC_REC_STORE_NO_SPACE = -28,
    BC_REC_STORE_OPEN_ERROR = -100,
    BC_REC_STORE_WRITE_ERROR = -101,
    BC_REC_STORE_SYNC_ERROR = -102,
    BC_REC_STORE_CLOSE_ERROR = -103,
    BC_REC_STORE_CORRUPT = -104,
    BC_REC_STORE_EMPTY = -105,
    BC_REC_STORE_RECEIPT_MISMATCH = -106,
    BC_REC_STORE_NOT_TERMINAL = -107,
    BC_REC_STORE_DELETED = -108,
    BC_REC_STORE_DELETE_ERROR = -109,
    BC_REC_STORE_UNVERIFIED = -110
} bc_rec_store_status;

typedef struct {
    lfs_dir_t dir;
    bool open;
    bool catalog;
    uint64_t after_id;
} bc_rec_store_cursor;

typedef struct bc_rec_store bc_rec_store;

/* A reader owns one read-only LittleFS file until reader_close.  Opening a
 * recording while a reader is live returns BUSY; this keeps the store's
 * single mounted lfs_t and bounded buffers unambiguous for the worker. */
typedef struct {
    bc_rec_store *owner;
    lfs_file_t file;
    uint8_t file_cache[BC_REC_STORE_CACHE_SIZE];
    uint8_t read_buffer[BC_REC_STORE_CACHE_SIZE];
    uint8_t metadata[BC_REC_STORE_METADATA_SIZE];
    struct lfs_file_config file_config;
    struct lfs_attr file_attr;
    uint64_t id;
    uint32_t bytes;
    uint32_t expected_crc32;
    uint32_t verify_crc_state;
    uint32_t verify_offset;
    bool verified;
    bool open;
} bc_rec_store_reader;

/*
 * The caller owns this object and keeps it alive while the store is used.
 * Public calls are expected to be serialized by the recording/archive
 * worker.  The store itself contains no locks, heap pointers, or RTOS types.
 */
struct bc_rec_store {
    lfs_t *lfs;
    bc_rec_store_namer namer;
    void *namer_ctx;
    /* Descriptor written into every new recording. Open fails until the
     * worker configures it, so a file can never carry a guessed codec. */
    bc_audio_format format;
    bool format_set;
    /* Exact real sample count supplied by the encoder before a complete
     * finish; persisted only with a completion marker. */
    uint32_t final_samples;

    bool initialized;
    bool active;
    bool reader_active;
    bool io_failed;
    uint64_t active_id;
    bc_rec_start active_start;
    bc_rec_file current;
    bc_rec_file durable;
    uint32_t crc_state;
    uint32_t durable_crc_state;
    bc_rec_result io_error;
    char path[BC_REC_STORE_PATH_SIZE];

    lfs_file_t file;
    uint8_t file_cache[BC_REC_STORE_CACHE_SIZE];
    uint8_t read_buffer[BC_REC_STORE_CACHE_SIZE];
    uint8_t metadata[BC_REC_STORE_METADATA_SIZE];
    uint8_t durable_metadata[BC_REC_STORE_METADATA_SIZE];
    struct lfs_file_config file_config;
    struct lfs_attr file_attr;
};

bool bc_rec_store_init(bc_rec_store *store, lfs_t *lfs,
                       bc_rec_store_namer namer, void *namer_ctx);
/* The format every new recording is labeled with. Only a valid non-NONE
 * descriptor with sample_count 0 is accepted; it cannot change while a
 * recording is active. */
bool bc_rec_store_set_format(bc_rec_store *store, const bc_audio_format *format);
/* Records the exact real sample count of the active Opus recording so the
 * completion marker can persist it. Ignored for fixed-block codecs. */
bool bc_rec_store_set_final_samples(bc_rec_store *store, uint32_t samples);
/* Decodes a raw metadata attribute of either version (v1 88 bytes, v2 104
 * bytes, or a v2-sized buffer holding a zero-padded v1 record). Used by the
 * export-name collision scan; returns false for a corrupt attribute. */
bool bc_rec_store_decode_metadata_name(const uint8_t *bytes, size_t size,
                                       char name[BC_REC_NAME_SIZE]);

/* These four functions have bc_rec_port-compatible signatures and may be
 * assigned directly to bc_rec_port.open/append/checkpoint/finish. */
bc_rec_result bc_rec_store_open(void *ctx, const bc_rec_start *start,
                                bc_rec_file *file);
bc_rec_result bc_rec_store_append(void *ctx, const uint8_t *data,
                                  uint16_t length);
bc_rec_result bc_rec_store_checkpoint(void *ctx, bc_rec_file *file);
bc_rec_result bc_rec_store_finish(void *ctx, bool complete,
                                  bc_rec_file *file);

/* Read-only inspection.  lookup verifies the committed raw prefix CRC before
 * returning; stat validates only the checksummed metadata and committed
 * length for bounded identity/query checks.  A valid incomplete record is
 * exposed as file->complete=false and file->recovered=true. */
bc_rec_store_status bc_rec_store_lookup(bc_rec_store *store, uint64_t id,
                                         bc_rec_start *start,
                                         bc_rec_file *file);
bc_rec_store_status bc_rec_store_stat(bc_rec_store *store, uint64_t id,
                                       bc_rec_start *start,
                                       bc_rec_file *file);
bc_rec_store_status bc_rec_store_list_begin(bc_rec_store *store,
                                             bc_rec_store_cursor *cursor);
/* list_next performs full raw-prefix verification for each returned record.
 * Use metadata_next below when a worker must advance one directory entry per
 * iteration without scanning audio. */
bc_rec_store_status bc_rec_store_list_next(bc_rec_store *store,
                                            bc_rec_store_cursor *cursor,
                                            bc_rec_start *start,
                                            bc_rec_file *file);
/* Consume at most one directory entry.  Callers use list_begin/list_end
 * around this API while idle; MORE means that the one entry was ignored
 * (unrelated name or a .done shadowed by its live .raw companion).  A
 * returned record has checksummed metadata and length checks only; raw CRC
 * verification remains the reader API's responsibility. */
bc_rec_store_status bc_rec_store_metadata_next(
    bc_rec_store *store, bc_rec_store_cursor *cursor, bc_rec_start *start,
    bc_rec_file *file);
bc_rec_store_status bc_rec_store_list_end(bc_rec_store *store,
                                          bc_rec_store_cursor *cursor);

/* Metadata-only catalog pagination.  Entries are selected in increasing
 * uint64 id order and are returned only when id > the previous after_id.
 * crc_verified is always false here: callers that need raw integrity should
 * use reader_open, which verifies the metadata prefix and CRC once. */
bc_rec_store_status bc_rec_store_catalog_begin(bc_rec_store *store,
                                               bc_rec_store_cursor *cursor,
                                               uint64_t after_id);
bc_rec_store_status bc_rec_store_catalog_next(bc_rec_store *store,
                                               bc_rec_store_cursor *cursor,
                                               bc_rec_start *start,
                                               bc_rec_file *file,
                                               bool *crc_verified);

/* reader_begin opens and checks only the metadata and committed raw length.
 * It leaves the reader owned by the store and returns with verified=false.
 * Call reader_verify_step from bounded worker iterations; each call consumes
 * at most max_bytes of raw content and returns MORE until the expected CRC is
 * confirmed.  Close is safe at any point and cancels verification. */
bc_rec_store_status bc_rec_store_reader_begin(bc_rec_store *store,
                                              uint64_t id,
                                              bc_rec_store_reader *reader,
                                              bc_rec_start *start,
                                              bc_rec_file *file);
bc_rec_store_status bc_rec_store_reader_verify_step(
    bc_rec_store_reader *reader, uint32_t max_bytes);
bc_rec_store_status bc_rec_store_reader_open(bc_rec_store *store, uint64_t id,
                                             bc_rec_store_reader *reader,
                                             bc_rec_start *start,
                                             bc_rec_file *file);
int32_t bc_rec_store_reader_read(bc_rec_store_reader *reader,
                                 uint32_t offset, uint8_t *data,
                                 uint32_t capacity);
bc_rec_store_status bc_rec_store_reader_close(bc_rec_store_reader *reader);

/* Returns a checked byte count, or a negative bc_rec_store_status.  Reads are
 * capped by the verified metadata prefix; a tombstone has no readable raw
 * bytes and returns BC_REC_STORE_DELETED. */
int32_t bc_rec_store_read(bc_rec_store *store, uint64_t id, uint32_t offset,
                          uint8_t *data, uint32_t capacity);

/* Custody is persisted in a tombstone before raw deletion.  receipt() is
 * idempotent and leaves the raw file in place; delete() removes it only after
 * rechecking the persisted exact byte count and CRC. */
bc_rec_store_status bc_rec_store_receipt(bc_rec_store *store, uint64_t id,
                                          uint32_t bytes, uint32_t crc32);
bc_rec_store_status bc_rec_store_delete(bc_rec_store *store, uint64_t id,
                                         uint32_t bytes, uint32_t crc32);

#endif
