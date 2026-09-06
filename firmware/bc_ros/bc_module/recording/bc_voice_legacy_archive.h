#ifndef BC_VOICE_LEGACY_ARCHIVE_H
#define BC_VOICE_LEGACY_ARCHIVE_H

#include <stdbool.h>
#include <stdint.h>

#include "../file/LittleFS/lfs.h"
#include "bc_rec_store.h"

/*
 * The factory file protocol is an unfragmented packet protocol.  Its four
 * byte command header is preserved byte-for-byte in every response.  The
 * largest response is therefore 4 + 17 + 220 = 241 bytes.
 */
#define BC_VOICE_LEGACY_COMMAND             0x36U
#define BC_VOICE_LEGACY_SUB_LIST            0x10U
#define BC_VOICE_LEGACY_SUB_UPLOAD          0x11U
#define BC_VOICE_LEGACY_SUB_CANCEL          0x02U
#define BC_VOICE_LEGACY_SUB_DELETE          0x12U
#define BC_VOICE_LEGACY_SUB_FORMAT          0x13U
#define BC_VOICE_LEGACY_SUB_SPACE           0x14U
#define BC_VOICE_LEGACY_SUB_BATCH           0x1aU
#define BC_VOICE_LEGACY_SUB_RESUME          0x18U

#define BC_VOICE_LEGACY_HEADER_SIZE         4U
#define BC_VOICE_LEGACY_NAME_SIZE           38U
#define BC_VOICE_LEGACY_LIST_FIELDS_SIZE    12U
#define BC_VOICE_LEGACY_LIST_PACKET_MAX \
    (BC_VOICE_LEGACY_HEADER_SIZE + BC_VOICE_LEGACY_LIST_FIELDS_SIZE + \
     BC_VOICE_LEGACY_NAME_SIZE)
#define BC_VOICE_LEGACY_UPLOAD_HEADER_SIZE 17U
#define BC_VOICE_LEGACY_CHUNK_SIZE          220U
#define BC_VOICE_LEGACY_UPLOAD_PACKET_MAX \
    (BC_VOICE_LEGACY_HEADER_SIZE + BC_VOICE_LEGACY_UPLOAD_HEADER_SIZE + \
     BC_VOICE_LEGACY_CHUNK_SIZE)
#define BC_VOICE_LEGACY_LIST_ATT_MIN        BC_VOICE_LEGACY_LIST_PACKET_MAX
#define BC_VOICE_LEGACY_UPLOAD_ATT_MIN      BC_VOICE_LEGACY_UPLOAD_PACKET_MAX
#define BC_VOICE_LEGACY_VERIFY_BUDGET       1024U
#define BC_VOICE_LEGACY_FILE_CACHE_SIZE     256U
#define BC_VOICE_LEGACY_PATH_SIZE \
    (1U + BC_VOICE_LEGACY_NAME_SIZE + 1U)

typedef bool (*bc_voice_legacy_archive_send)(void *ctx,
                                             const uint8_t *packet,
                                             uint16_t length,
                                             uint32_t epoch);

typedef enum {
    BC_VOICE_LEGACY_IDLE = 0,
    BC_VOICE_LEGACY_LIST_COUNT_OLD_BEGIN,
    BC_VOICE_LEGACY_LIST_COUNT_OLD,
    BC_VOICE_LEGACY_LIST_COUNT_NEW_BEGIN,
    BC_VOICE_LEGACY_LIST_COUNT_NEW,
    BC_VOICE_LEGACY_LIST_EMIT_OLD_BEGIN,
    BC_VOICE_LEGACY_LIST_EMIT_OLD,
    BC_VOICE_LEGACY_LIST_EMIT_NEW_BEGIN,
    BC_VOICE_LEGACY_LIST_EMIT_NEW,
    BC_VOICE_LEGACY_LIST_BUILD_EMPTY,
    BC_VOICE_LEGACY_UPLOAD_FIND_OLD_BEGIN,
    BC_VOICE_LEGACY_UPLOAD_FIND_OLD,
    BC_VOICE_LEGACY_UPLOAD_OLD_OPEN,
    BC_VOICE_LEGACY_UPLOAD_FIND_NEW_BEGIN,
    BC_VOICE_LEGACY_UPLOAD_FIND_NEW,
    BC_VOICE_LEGACY_UPLOAD_NEW_BEGIN,
    BC_VOICE_LEGACY_UPLOAD_NEW_VERIFY,
    BC_VOICE_LEGACY_UPLOAD_READ,
    BC_VOICE_LEGACY_UPLOAD_SEND,
    BC_VOICE_LEGACY_UPLOAD_CLOSE,
    BC_VOICE_LEGACY_SEND_ONLY
} bc_voice_legacy_archive_phase;

typedef enum {
    BC_VOICE_LEGACY_SOURCE_NONE = 0,
    BC_VOICE_LEGACY_SOURCE_OLD,
    BC_VOICE_LEGACY_SOURCE_NEW
} bc_voice_legacy_archive_source;

/*
 * This object is intentionally public and allocation-free.  The application
 * owns one instance and calls it only from its serialized worker.  A NULL
 * store is allowed for a legacy-only filesystem; it simply removes the
 * virtual new-store half of list and lookup.
 */
typedef struct {
    lfs_t *lfs;
    bc_rec_store *store;
    bc_voice_legacy_archive_send send;
    void *send_ctx;
    bool initialized;

    bc_voice_legacy_archive_phase phase;
    bc_voice_legacy_archive_phase after_send;
    bc_voice_legacy_archive_source source;
    uint32_t epoch;
    uint16_t att_limit;
    uint8_t request_header[BC_VOICE_LEGACY_HEADER_SIZE];
    uint8_t requested_name[BC_VOICE_LEGACY_NAME_SIZE];
    uint32_t requested_offset;

    /* Two-pass list state.  No file-name array is retained. */
    uint32_t list_total;
    uint32_t list_emitted;
    uint64_t new_id;
    bc_rec_start new_start;
    bc_rec_file new_file;

    lfs_dir_t old_dir;
    bool old_dir_open;
    struct lfs_info old_info;
    lfs_file_t old_file;
    bool old_file_open;
    uint8_t old_file_cache[BC_VOICE_LEGACY_FILE_CACHE_SIZE];
    struct lfs_file_config old_file_config;
    char old_path[BC_VOICE_LEGACY_PATH_SIZE];

    bc_rec_store_cursor new_cursor;
    bool new_cursor_open;
    bc_rec_store_reader new_reader;

    uint32_t file_size;
    uint32_t transfer_offset;
    uint32_t transfer_sequence;
    uint16_t transfer_chunk;

    uint8_t tx_packet[BC_VOICE_LEGACY_UPLOAD_PACKET_MAX];
    uint16_t tx_length;
    bool tx_pending;
} bc_voice_legacy_archive;

bool bc_voice_legacy_archive_init(
    bc_voice_legacy_archive *archive, lfs_t *lfs, bc_rec_store *store,
    bc_voice_legacy_archive_send send, void *send_ctx);

bc_rec_result bc_voice_legacy_archive_request(
    bc_voice_legacy_archive *archive, const uint8_t *packet,
    uint16_t length, uint32_t epoch, uint16_t att_limit);

/* One cooperative step: at most one directory/catalog operation or one
 * bounded (<=1024-byte) verification/read, plus one send attempt. */
bc_rec_result bc_voice_legacy_archive_poll(bc_voice_legacy_archive *archive);

/* Cancel is safe while idle and always resets the operation after attempting
 * to close every owned handle. */
bc_rec_result bc_voice_legacy_archive_cancel(bc_voice_legacy_archive *archive);

bool bc_voice_legacy_archive_active(const bc_voice_legacy_archive *archive);

#endif
