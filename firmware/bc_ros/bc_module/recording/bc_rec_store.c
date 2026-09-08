#include "bc_rec_store.h"

#include <stddef.h>
#include <string.h>

#include "bc_audio_format.h"

/*
 * The metadata attribute is intentionally a byte-defined format.  It is
 * never read through a C struct, so the on-disk representation is unchanged
 * on targets with a different ABI or alignment.
 *
 *   0..3   magic "SREC"
 *   4..5   format version (LE16)
 *   6..7   encoded size (LE16)
 *   8..15  recording id (LE64)
 *   16     trigger
 *   17     flags: complete, recovered, tombstone, custody
 *   18..19 reserved (zero)
 *   20..23 duration limit (LE32)
 *   24..27 durable raw bytes (LE32)
 *   28..31 durable frame count (LE32)
 *   32..35 raw CRC-32/ISO-HDLC (LE32)
 *   36..39 persisted custody byte count (LE32)
 *   40..43 persisted custody CRC (LE32)
 *   44..83 NUL-terminated legacy export name (40 bytes)
 *   84..87 version 1: metadata CRC-32/ISO-HDLC of bytes 0..83 (LE32)
 *   84..99 version 2: audio descriptor (bc_audio_format wire layout)
 *   100..103 version 2: metadata CRC-32/ISO-HDLC of bytes 0..99 (LE32)
 *
 * Version 1 records (S01-S04 firmware) carry no descriptor and decode as
 * the supplier ADPCM format. New records are always written as version 2.
 */
enum {
    META_MAGIC = 0,
    META_VERSION = 4,
    META_SIZE = 6,
    META_ID = 8,
    META_TRIGGER = 16,
    META_FLAGS = 17,
    META_RESERVED = 18,
    META_DURATION = 20,
    META_BYTES = 24,
    META_FRAMES = 28,
    META_CRC = 32,
    META_RECEIPT_BYTES = 36,
    META_RECEIPT_CRC = 40,
    META_NAME = 44,
    META_V1_CHECKSUM = 84,
    META_V1_BODY_SIZE = 84,
    META_AUDIO = 84,
    META_CHECKSUM = 100,
    META_BODY_SIZE = 100
};

enum {
    META_FORMAT_VERSION_V1 = 1,
    META_FORMAT_VERSION = 2,
    META_FLAG_COMPLETE = 1U << 0,
    META_FLAG_RECOVERED = 1U << 1,
    META_FLAG_TOMBSTONE = 1U << 2,
    META_FLAG_CUSTODY = 1U << 3,
    META_FLAG_MASK = META_FLAG_COMPLETE | META_FLAG_RECOVERED |
                     META_FLAG_TOMBSTONE | META_FLAG_CUSTODY
};

static bool store_valid(const bc_rec_store *store)
{
    return store != NULL && store->initialized && store->lfs != NULL &&
           store->lfs->cfg != NULL;
}

static bool start_valid(const bc_rec_start *start)
{
    return start != NULL && start->id != 0U &&
           start->trigger >= BC_REC_PTT && start->trigger <= BC_REC_APP &&
           start->duration_limit_ms <= BC_REC_MAX_INTERVAL;
}

static bool name_valid(const char name[BC_REC_NAME_SIZE])
{
    size_t i;

    if (name == NULL || name[0] == '\0')
        return false;

    for (i = 0; i < BC_REC_NAME_SIZE; ++i) {
        unsigned char c = (unsigned char)name[i];
        if (c == '\0')
            break;
        if (c < 0x20U || c == 0x7fU || c == '/' || c == '\\')
            return false;
    }

    if (i == BC_REC_NAME_SIZE ||
        (i == 1U && name[0] == '.') ||
        (i == 2U && name[0] == '.' && name[1] == '.'))
        return false;

    return true;
}

static void put_le16(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value & 0xffU);
    dst[1] = (uint8_t)((value >> 8) & 0xffU);
}

static void put_le32(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value & 0xffU);
    dst[1] = (uint8_t)((value >> 8) & 0xffU);
    dst[2] = (uint8_t)((value >> 16) & 0xffU);
    dst[3] = (uint8_t)((value >> 24) & 0xffU);
}

static void put_le64(uint8_t *dst, uint64_t value)
{
    unsigned i;
    for (i = 0; i < 8U; ++i)
        dst[i] = (uint8_t)((value >> (8U * i)) & 0xffU);
}

static uint16_t get_le16(const uint8_t *src)
{
    return (uint16_t)src[0] | (uint16_t)((uint16_t)src[1] << 8);
}

static uint32_t get_le32(const uint8_t *src)
{
    return (uint32_t)src[0] |
           ((uint32_t)src[1] << 8) |
           ((uint32_t)src[2] << 16) |
           ((uint32_t)src[3] << 24);
}

static uint64_t get_le64(const uint8_t *src)
{
    uint64_t value = 0;
    unsigned i;

    for (i = 0; i < 8U; ++i)
        value |= (uint64_t)src[i] << (8U * i);
    return value;
}

/* lfs_crc is the vendor LittleFS CRC primitive.  ISO-HDLC adds the standard
 * initial and final XORs around it, which also matches zlib.crc32. */
static uint32_t crc32_bytes(const uint8_t *data, size_t size)
{
    return lfs_crc(0xffffffffUL, data, size) ^ 0xffffffffUL;
}

static uint32_t crc32_extend(uint32_t state, const uint8_t *data,
                             size_t size)
{
    return lfs_crc(state, data, size);
}

static bool counts_valid(uint32_t bytes, uint32_t frames)
{
    return (bytes == 0U) == (frames == 0U) && frames <= bytes &&
           (uint64_t)bytes <= (uint64_t)frames * BC_REC_FRAME_MAX;
}

static bool same_start(const bc_rec_start *left, const bc_rec_start *right)
{
    return left != NULL && right != NULL && left->id == right->id &&
           left->trigger == right->trigger &&
           left->duration_limit_ms == right->duration_limit_ms;
}

/* delivered is intentionally omitted: raw records report the receipt while
 * their .done companion is present, whereas the companion always reports it. */
static bool same_file(const bc_rec_file *left, const bc_rec_file *right)
{
    return left != NULL && right != NULL && left->bytes == right->bytes &&
           left->frames == right->frames && left->crc32 == right->crc32 &&
           left->complete == right->complete &&
           left->recovered == right->recovered &&
           memcmp(left->name, right->name, BC_REC_NAME_SIZE) == 0 &&
           bc_audio_format_equal(&left->audio, &right->audio);
}

static void path_for_id(char path[BC_REC_STORE_PATH_SIZE], uint64_t id,
                        const char suffix[])
{
    static const char hex[] = "0123456789abcdef";
    size_t n = 0;
    int i;

    memcpy(path, BC_REC_STORE_DIR, sizeof(BC_REC_STORE_DIR) - 1U);
    n = sizeof(BC_REC_STORE_DIR) - 1U;
    path[n++] = '/';
    for (i = 15; i >= 0; --i)
        path[n++] = hex[(id >> (4U * (unsigned)i)) & 0xfU];
    while (*suffix != '\0')
        path[n++] = *suffix++;
    path[n] = '\0';
}

static bool parse_record_name(const char *name, uint64_t *id,
                              bool *tombstone)
{
    uint64_t value = 0;
    size_t length;
    unsigned i;
    const char *suffix;
    size_t suffix_length;

    if (name == NULL || id == NULL || tombstone == NULL)
        return false;

    length = strlen(name);
    if (length == 20U) {
        suffix = ".raw";
        suffix_length = 4U;
        *tombstone = false;
    } else if (length == 21U) {
        suffix = ".done";
        suffix_length = 5U;
        *tombstone = true;
    } else {
        return false;
    }

    if (memcmp(name + 16U, suffix, suffix_length) != 0)
        return false;

    for (i = 0; i < 16U; ++i) {
        unsigned digit;
        char c = name[i];
        if (c >= '0' && c <= '9')
            digit = (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f')
            digit = (unsigned)(c - 'a') + 10U;
        else
            return false;
        value = (value << 4) | digit;
    }

    if (value == 0U)
        return false;
    *id = value;
    return true;
}

static void encode_metadata(uint8_t bytes[BC_REC_STORE_METADATA_SIZE],
                            const bc_rec_start *start,
                            const bc_rec_file *file, uint8_t flags,
                            uint32_t receipt_bytes, uint32_t receipt_crc)
{
    memset(bytes, 0, BC_REC_STORE_METADATA_SIZE);
    memcpy(bytes + META_MAGIC, "SREC", 4U);
    put_le16(bytes + META_VERSION, META_FORMAT_VERSION);
    put_le16(bytes + META_SIZE, BC_REC_STORE_METADATA_SIZE);
    put_le64(bytes + META_ID, start->id);
    bytes[META_TRIGGER] = (uint8_t)start->trigger;
    bytes[META_FLAGS] = flags;
    put_le32(bytes + META_DURATION, start->duration_limit_ms);
    put_le32(bytes + META_BYTES, file->bytes);
    put_le32(bytes + META_FRAMES, file->frames);
    put_le32(bytes + META_CRC, file->crc32);
    put_le32(bytes + META_RECEIPT_BYTES, receipt_bytes);
    put_le32(bytes + META_RECEIPT_CRC, receipt_crc);
    memcpy(bytes + META_NAME, file->name, BC_REC_NAME_SIZE);
    bc_audio_format_encode(bytes + META_AUDIO, &file->audio);
    put_le32(bytes + META_CHECKSUM, crc32_bytes(bytes, META_BODY_SIZE));
}

/* Accepts a version-1 or version-2 attribute image in a v2-sized buffer.
 * Returns the checksum-verified version, or 0 for a corrupt image. */
static unsigned metadata_version(const uint8_t bytes[BC_REC_STORE_METADATA_SIZE])
{
    uint16_t version;
    uint16_t encoded_size;
    if (memcmp(bytes + META_MAGIC, "SREC", 4U) != 0)
        return 0U;
    version = get_le16(bytes + META_VERSION);
    encoded_size = get_le16(bytes + META_SIZE);
    if (version == META_FORMAT_VERSION_V1) {
        unsigned i;
        if (encoded_size != BC_REC_STORE_METADATA_V1_SIZE ||
            get_le32(bytes + META_V1_CHECKSUM) != crc32_bytes(bytes, META_V1_BODY_SIZE))
            return 0U;
        /* LittleFS zero-fills a shorter stored attribute; anything else in
         * the tail means a foreign or damaged image. */
        for (i = BC_REC_STORE_METADATA_V1_SIZE; i < BC_REC_STORE_METADATA_SIZE; ++i)
            if (bytes[i] != 0U)
                return 0U;
        return META_FORMAT_VERSION_V1;
    }
    if (version == META_FORMAT_VERSION) {
        if (encoded_size != BC_REC_STORE_METADATA_SIZE ||
            get_le32(bytes + META_CHECKSUM) != crc32_bytes(bytes, META_BODY_SIZE))
            return 0U;
        return META_FORMAT_VERSION;
    }
    return 0U;
}

static bc_rec_store_status decode_metadata(
    const uint8_t bytes[BC_REC_STORE_METADATA_SIZE], uint64_t expected_id,
    bool expect_tombstone, bc_rec_start *start, bc_rec_file *file,
    uint32_t *receipt_bytes, uint32_t *receipt_crc)
{
    unsigned version;
    uint8_t flags;
    uint32_t receipt_count;
    uint32_t receipt_value;

    version = metadata_version(bytes);
    if (version == 0U)
        return BC_REC_STORE_CORRUPT;

    if (get_le64(bytes + META_ID) != expected_id ||
        bytes[META_RESERVED] != 0U || bytes[META_RESERVED + 1U] != 0U)
        return BC_REC_STORE_CORRUPT;

    flags = bytes[META_FLAGS];
    if ((flags & (uint8_t)~META_FLAG_MASK) != 0U ||
        ((flags & META_FLAG_TOMBSTONE) != 0U) != expect_tombstone ||
        (!expect_tombstone && (flags & (META_FLAG_RECOVERED |
                                         META_FLAG_CUSTODY)) != 0U))
        return BC_REC_STORE_CORRUPT;

    if (start != NULL) {
        start->id = expected_id;
        start->trigger = (bc_rec_trigger)bytes[META_TRIGGER];
        start->duration_limit_ms = get_le32(bytes + META_DURATION);
        if (!start_valid(start))
            return BC_REC_STORE_CORRUPT;
    } else if (bytes[META_TRIGGER] < BC_REC_PTT ||
               bytes[META_TRIGGER] > BC_REC_APP ||
               get_le32(bytes + META_DURATION) > BC_REC_MAX_INTERVAL) {
        return BC_REC_STORE_CORRUPT;
    }

    if (file == NULL)
        return BC_REC_STORE_INVALID;
    memset(file, 0, sizeof(*file));
    file->bytes = get_le32(bytes + META_BYTES);
    file->frames = get_le32(bytes + META_FRAMES);
    file->crc32 = get_le32(bytes + META_CRC);
    file->complete = (flags & META_FLAG_COMPLETE) != 0U;
    file->recovered = (flags & META_FLAG_RECOVERED) != 0U;
    file->delivered = expect_tombstone;
    memcpy(file->name, bytes + META_NAME, BC_REC_NAME_SIZE);
    if (!name_valid(file->name) || !counts_valid(file->bytes, file->frames) ||
        (!expect_tombstone && file->complete && file->bytes == 0U))
        return BC_REC_STORE_CORRUPT;
    if (version == META_FORMAT_VERSION_V1) {
        bc_audio_format_legacy_adpcm(&file->audio);
    } else if (!bc_audio_format_decode(bytes + META_AUDIO, &file->audio) ||
               file->audio.codec == BC_AUDIO_CODEC_NONE ||
               (file->audio.sample_count != 0U && !file->complete)) {
        /* A descriptor is mandatory in version 2, and an exact sample
         * count exists only once the recording is complete. */
        return BC_REC_STORE_CORRUPT;
    }

    receipt_count = get_le32(bytes + META_RECEIPT_BYTES);
    receipt_value = get_le32(bytes + META_RECEIPT_CRC);
    if (!expect_tombstone && (receipt_count != 0U || receipt_value != 0U))
        return BC_REC_STORE_CORRUPT;

    if (expect_tombstone) {
        if ((flags & META_FLAG_CUSTODY) == 0U ||
            (file->complete == file->recovered) || file->bytes == 0U ||
            receipt_count != file->bytes || receipt_value != file->crc32)
            return BC_REC_STORE_CORRUPT;
        if (receipt_bytes != NULL)
            *receipt_bytes = receipt_count;
        if (receipt_crc != NULL)
            *receipt_crc = receipt_value;
    } else {
        file->recovered = false;
    }

    if (receipt_bytes != NULL && !expect_tombstone)
        *receipt_bytes = 0U;
    if (receipt_crc != NULL && !expect_tombstone)
        *receipt_crc = 0U;
    return BC_REC_STORE_OK;
}

static bc_rec_store_status status_from_lfs(int error,
                                           bc_rec_store_status fallback)
{
    if (error == LFS_ERR_NOSPC)
        return BC_REC_STORE_NO_SPACE;
    if (error == LFS_ERR_CORRUPT)
        return BC_REC_STORE_CORRUPT;
    if (error == LFS_ERR_NOENT)
        return BC_REC_STORE_NOT_FOUND;
    return fallback;
}

static bc_rec_result port_result_from_status(bc_rec_store_status status,
                                             bc_rec_result fallback)
{
    switch (status) {
    case BC_REC_STORE_OK:
        return BC_REC_OK;
    case BC_REC_STORE_INVALID:
        return BC_REC_INVALID;
    case BC_REC_STORE_BUSY:
        return BC_REC_BUSY;
    case BC_REC_STORE_DUPLICATE:
        return BC_REC_DUPLICATE;
    case BC_REC_STORE_NO_SPACE:
        return BC_REC_NO_SPACE;
    case BC_REC_STORE_EMPTY:
        return BC_REC_EMPTY_AUDIO;
    case BC_REC_STORE_SYNC_ERROR:
        return BC_REC_SYNC_ERROR;
    case BC_REC_STORE_CLOSE_ERROR:
        return BC_REC_CLOSE_ERROR;
    case BC_REC_STORE_WRITE_ERROR:
        return BC_REC_WRITE_ERROR;
    case BC_REC_STORE_CORRUPT:
        return BC_REC_CRC_ERROR;
    default:
        return fallback;
    }
}

static void configure_file(bc_rec_store *store)
{
    store->file_attr.type = BC_REC_STORE_META_ATTR;
    store->file_attr.buffer = store->metadata;
    store->file_attr.size = BC_REC_STORE_METADATA_SIZE;
    store->file_config.buffer = store->file_cache;
    store->file_config.attrs = &store->file_attr;
    store->file_config.attr_count = 1U;
}

/* A failed sync may occur during the device sync callback, before LittleFS
 * marks the file errored itself.  Mark it here so the mandatory close cannot
 * retry the dirty metadata and accidentally publish an uncommitted prefix. */
static void prevent_file_retry(lfs_file_t *file)
{
    file->flags |= LFS_F_ERRED;
}

static int open_with_metadata(bc_rec_store *store, const char *path,
                              int flags, bool clear_attribute)
{
    configure_file(store);
    if (clear_attribute)
        memset(store->metadata, 0, sizeof(store->metadata));
    return lfs_file_opencfg(store->lfs, &store->file, path, flags,
                            &store->file_config);
}

static bc_rec_store_status ensure_directory(bc_rec_store *store)
{
    struct lfs_info info;
    int error = lfs_mkdir(store->lfs, BC_REC_STORE_DIR);
    if (error != LFS_ERR_OK && error != LFS_ERR_EXIST)
        return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);

    error = lfs_stat(store->lfs, BC_REC_STORE_DIR, &info);
    if (error != LFS_ERR_OK)
        return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);
    return info.type == LFS_TYPE_DIR ? BC_REC_STORE_OK :
                                         BC_REC_STORE_OPEN_ERROR;
}

static bc_rec_store_status stat_path(bc_rec_store *store, const char *path,
                                     struct lfs_info *info)
{
    int error = lfs_stat(store->lfs, path, info);
    if (error != LFS_ERR_OK)
        return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);
    if (info->type != LFS_TYPE_REG)
        return BC_REC_STORE_CORRUPT;
    return BC_REC_STORE_OK;
}

static bool is_attrless_empty_file(bc_rec_store *store, const char *path,
                                   const struct lfs_info *info)
{
    uint8_t probe;
    lfs_ssize_t result;

    if (info == NULL || info->size != 0U)
        return false;
    result = lfs_getattr(store->lfs, path, BC_REC_STORE_META_ATTR, &probe, 1U);
    return result == LFS_ERR_NOATTR;
}

/* Verify a raw file without modifying it.  An incomplete file may have a
 * physically present tail from an interrupted data commit; only the prefix
 * described by its atomically committed attribute is accepted. */
static bc_rec_store_status inspect_raw_path(
    bc_rec_store *store, const char *path, uint64_t id,
    bc_rec_start *start, bc_rec_file *file)
{
    struct lfs_info info;
    bc_rec_store_status status;
    lfs_soff_t raw_size;
    lfs_size_t remaining;
    uint32_t crc_state = 0xffffffffUL;
    int error;

    status = stat_path(store, path, &info);
    if (status != BC_REC_STORE_OK)
        return status;

    error = open_with_metadata(store, path, LFS_O_RDONLY, true);
    if (error != LFS_ERR_OK)
        return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);

    status = decode_metadata(store->metadata, id, false, start, file,
                             NULL, NULL);
    if (status != BC_REC_STORE_OK)
        goto close_and_return;

    raw_size = lfs_file_size(store->lfs, &store->file);
    if (raw_size < 0 || (uint64_t)raw_size < file->bytes ||
        (file->complete && (uint64_t)raw_size != file->bytes)) {
        status = raw_size < 0
            ? status_from_lfs((int)raw_size, BC_REC_STORE_OPEN_ERROR)
            : BC_REC_STORE_CORRUPT;
        goto close_and_return;
    }

    if (file->bytes == 0U) {
        status = BC_REC_STORE_EMPTY;
        goto close_and_return;
    }

    remaining = file->bytes;
    while (remaining != 0U) {
        lfs_size_t request = remaining > BC_REC_STORE_CACHE_SIZE
                           ? BC_REC_STORE_CACHE_SIZE : remaining;
        lfs_ssize_t got = lfs_file_read(store->lfs, &store->file,
                                        store->read_buffer, request);
        if (got < 0) {
            status = status_from_lfs((int)got, BC_REC_STORE_OPEN_ERROR);
            goto close_and_return;
        }
        if ((lfs_size_t)got != request) {
            status = BC_REC_STORE_CORRUPT;
            goto close_and_return;
        }
        crc_state = crc32_extend(crc_state, store->read_buffer,
                                 (size_t)got);
        remaining -= request;
    }

    if ((crc_state ^ 0xffffffffUL) != file->crc32) {
        status = BC_REC_STORE_CORRUPT;
        goto close_and_return;
    }

    file->recovered = !file->complete;
    status = BC_REC_STORE_OK;

close_and_return:
    error = lfs_file_close(store->lfs, &store->file);
    if (status == BC_REC_STORE_OK && error != LFS_ERR_OK)
        status = status_from_lfs(error, BC_REC_STORE_CLOSE_ERROR);
    return status;
}

static bc_rec_store_status inspect_tombstone_path(
    bc_rec_store *store, const char *path, uint64_t id,
    bc_rec_start *start, bc_rec_file *file)
{
    struct lfs_info info;
    lfs_soff_t size;
    bc_rec_store_status status;
    int error;

    status = stat_path(store, path, &info);
    if (status != BC_REC_STORE_OK)
        return status;

    error = open_with_metadata(store, path, LFS_O_RDONLY, true);
    if (error != LFS_ERR_OK)
        return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);

    size = lfs_file_size(store->lfs, &store->file);
    if (size < 0)
        status = status_from_lfs((int)size, BC_REC_STORE_OPEN_ERROR);
    else if (size != 0)
        status = BC_REC_STORE_CORRUPT;
    else
        status = decode_metadata(store->metadata, id, true, start, file,
                                 NULL, NULL);

    error = lfs_file_close(store->lfs, &store->file);
    if (status == BC_REC_STORE_OK && error != LFS_ERR_OK)
        status = status_from_lfs(error, BC_REC_STORE_CLOSE_ERROR);
    return status;
}

/* Catalogs intentionally validate only the checksummed metadata attribute.
 * Raw length and CRC are deferred to reader_open, which keeps directory scans
 * bounded even when the flash holds many recordings. */
static bc_rec_store_status inspect_metadata_path(
    bc_rec_store *store, const char *path, uint64_t id, bool tombstone,
    bc_rec_start *start, bc_rec_file *file)
{
    struct lfs_info info;
    bc_rec_store_status status;
    lfs_soff_t size;
    int error;

    status = stat_path(store, path, &info);
    if (status != BC_REC_STORE_OK)
        return status;
    error = open_with_metadata(store, path, LFS_O_RDONLY, true);
    if (error != LFS_ERR_OK)
        return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);

    size = lfs_file_size(store->lfs, &store->file);
    if (size < 0)
        status = status_from_lfs((int)size, BC_REC_STORE_OPEN_ERROR);
    else if (tombstone && size != 0)
        status = BC_REC_STORE_CORRUPT;
    else {
        status = decode_metadata(store->metadata, id, tombstone, start, file,
                                 NULL, NULL);
        if (status == BC_REC_STORE_OK && !tombstone &&
            ((uint64_t)size < file->bytes ||
             (file->complete && (uint64_t)size != file->bytes)))
            status = BC_REC_STORE_CORRUPT;
    }

    error = lfs_file_close(store->lfs, &store->file);
    if (status == BC_REC_STORE_OK && error != LFS_ERR_OK)
        status = status_from_lfs(error, BC_REC_STORE_CLOSE_ERROR);
    if (status == BC_REC_STORE_OK && !tombstone)
        file->recovered = !file->complete;
    return status;
}

static bc_rec_store_status inspect_id(bc_rec_store *store, uint64_t id,
                                      bc_rec_start *start, bc_rec_file *file,
                                      bool *was_tombstone)
{
    char raw_path[BC_REC_STORE_PATH_SIZE];
    char tombstone_path[BC_REC_STORE_PATH_SIZE];
    struct lfs_info info;
    bc_rec_store_status status;
    bc_rec_store_status tombstone_status;

    path_for_id(raw_path, id, ".raw");
    status = inspect_raw_path(store, raw_path, id, start, file);
    if (status != BC_REC_STORE_NOT_FOUND) {
        if (status == BC_REC_STORE_OK) {
            /* A persisted receipt may coexist with raw bytes until the
             * separate remove call succeeds.  Surface delivered on the raw
            * record too, while continuing to verify the raw prefix. */
            path_for_id(tombstone_path, id, ".done");
            tombstone_status = stat_path(store, tombstone_path, &info);
            if (tombstone_status == BC_REC_STORE_OK) {
                bc_rec_start receipt_start;
                bc_rec_file receipt_file;
                bc_rec_store_status receipt_status =
                    inspect_tombstone_path(store, tombstone_path, id,
                                           &receipt_start, &receipt_file);
                if (receipt_status == BC_REC_STORE_CORRUPT &&
                    is_attrless_empty_file(store, tombstone_path, &info)) {
                    /* An interrupted first receipt sync left only the
                     * tombstone name.  Keep the valid raw record visible and
                     * let receipt() resume that placeholder. */
                } else if (receipt_status != BC_REC_STORE_OK ||
                           !same_start(&receipt_start, start) ||
                           !same_file(&receipt_file, file))
                    return receipt_status == BC_REC_STORE_OK
                         ? BC_REC_STORE_CORRUPT : receipt_status;
                else
                    file->delivered = true;
            } else if (tombstone_status != BC_REC_STORE_NOT_FOUND) {
                return tombstone_status;
            }
        }
        if (was_tombstone != NULL)
            *was_tombstone = false;
        return status;
    }

    path_for_id(tombstone_path, id, ".done");
    status = inspect_tombstone_path(store, tombstone_path, id, start, file);
    if (was_tombstone != NULL)
        *was_tombstone = status == BC_REC_STORE_OK;
    return status;
}

/* Metadata-only identity lookup used by Start collision checks, stat, and
 * directory/catalog scans.  It checks the committed raw length but defers
 * content CRC work to inspect_id or the staged reader. */
static bc_rec_store_status inspect_id_metadata(
    bc_rec_store *store, uint64_t id, bc_rec_start *start,
    bc_rec_file *file, bool *was_tombstone)
{
    char raw_path[BC_REC_STORE_PATH_SIZE];
    char tombstone_path[BC_REC_STORE_PATH_SIZE];
    struct lfs_info raw_info;
    struct lfs_info tombstone_info;
    bc_rec_store_status status;
    bc_rec_store_status tombstone_status;

    if (was_tombstone != NULL)
        *was_tombstone = false;
    path_for_id(raw_path, id, ".raw");
    status = stat_path(store, raw_path, &raw_info);
    if (status == BC_REC_STORE_OK) {
        status = inspect_metadata_path(store, raw_path, id, false,
                                       start, file);
        if (status != BC_REC_STORE_OK)
            return status;
        if ((uint64_t)raw_info.size < file->bytes ||
            (file->complete && (uint64_t)raw_info.size != file->bytes))
            return BC_REC_STORE_CORRUPT;

        path_for_id(tombstone_path, id, ".done");
        tombstone_status = stat_path(store, tombstone_path, &tombstone_info);
        if (tombstone_status == BC_REC_STORE_OK) {
            bc_rec_start receipt_start;
            bc_rec_file receipt_file;

            tombstone_status = inspect_tombstone_path(
                store, tombstone_path, id, &receipt_start, &receipt_file);
            if (tombstone_status == BC_REC_STORE_CORRUPT &&
                is_attrless_empty_file(store, tombstone_path,
                                       &tombstone_info)) {
                /* An interrupted first receipt sync left no custody data. */
            } else if (tombstone_status != BC_REC_STORE_OK ||
                       !same_start(&receipt_start, start) ||
                       !same_file(&receipt_file, file)) {
                return tombstone_status == BC_REC_STORE_OK
                     ? BC_REC_STORE_CORRUPT : tombstone_status;
            } else {
                file->delivered = true;
            }
        } else if (tombstone_status != BC_REC_STORE_NOT_FOUND) {
            return tombstone_status;
        }
        return file->bytes == 0U ? BC_REC_STORE_EMPTY : BC_REC_STORE_OK;
    }
    if (status != BC_REC_STORE_NOT_FOUND)
        return status;

    path_for_id(tombstone_path, id, ".done");
    status = inspect_tombstone_path(store, tombstone_path, id, start, file);
    if (status == BC_REC_STORE_OK && was_tombstone != NULL)
        *was_tombstone = true;
    return status;
}

static bc_rec_result map_port_open_status(bc_rec_store_status status)
{
    if (status == BC_REC_STORE_NO_SPACE)
        return BC_REC_NO_SPACE;
    if (status == BC_REC_STORE_INVALID)
        return BC_REC_INVALID;
    if (status == BC_REC_STORE_BUSY)
        return BC_REC_BUSY;
    if (status == BC_REC_STORE_DUPLICATE)
        return BC_REC_DUPLICATE;
    if (status == BC_REC_STORE_EMPTY)
        return BC_REC_EMPTY_AUDIO;
    return BC_REC_OPEN_ERROR;
}

bool bc_rec_store_init(bc_rec_store *store, lfs_t *lfs,
                       bc_rec_store_namer namer, void *namer_ctx)
{
    if (store == NULL || lfs == NULL || lfs->cfg == NULL || namer == NULL ||
        lfs->cfg->cache_size == 0U ||
        lfs->cfg->cache_size > BC_REC_STORE_CACHE_SIZE ||
        lfs->cfg->attr_max < BC_REC_STORE_METADATA_SIZE)
        return false;

    memset(store, 0, sizeof(*store));
    store->lfs = lfs;
    store->namer = namer;
    store->namer_ctx = namer_ctx;
    store->format_set = false;
    store->file_attr.type = BC_REC_STORE_META_ATTR;
    store->file_attr.buffer = store->metadata;
    store->file_attr.size = BC_REC_STORE_METADATA_SIZE;
    store->file_config.buffer = store->file_cache;
    store->file_config.attrs = &store->file_attr;
    store->file_config.attr_count = 1U;
    store->initialized = true;
    return true;
}

bool bc_rec_store_set_format(bc_rec_store *store, const bc_audio_format *format)
{
    if (!store_valid(store) || store->active || format == NULL ||
        !bc_audio_format_valid(format) || format->codec == BC_AUDIO_CODEC_NONE ||
        format->sample_count != 0U)
        return false;
    store->format = *format;
    store->format_set = true;
    return true;
}

bool bc_rec_store_set_final_samples(bc_rec_store *store, uint32_t samples)
{
    if (!store_valid(store) || !store->active ||
        store->current.audio.codec != BC_AUDIO_CODEC_OPUS)
        return false;
    store->final_samples = samples;
    return true;
}

bool bc_rec_store_decode_metadata_name(const uint8_t *bytes, size_t size,
                                       char name[BC_REC_NAME_SIZE])
{
    uint8_t image[BC_REC_STORE_METADATA_SIZE];
    if (bytes == NULL || name == NULL ||
        (size != BC_REC_STORE_METADATA_V1_SIZE && size != BC_REC_STORE_METADATA_SIZE))
        return false;
    memset(image, 0, sizeof(image));
    memcpy(image, bytes, size);
    if (metadata_version(image) == 0U)
        return false;
    memcpy(name, image + META_NAME, BC_REC_NAME_SIZE);
    return name_valid(name);
}

static bc_rec_store_status inspect_existing_for_start(
    bc_rec_store *store, const bc_rec_start *start, bc_rec_file *file)
{
    bc_rec_start existing_start;
    bc_rec_store_status status;

    /* Start collision checks inspect only the checksummed attribute and the
     * committed file length.  A full raw CRC belongs to reader_open/verify;
     * retrying Start must stay bounded even for a long old recording. */
    status = inspect_id_metadata(store, start->id, &existing_start, file,
                                 NULL);
    if (status != BC_REC_STORE_OK)
        return status;
    if (!same_start(&existing_start, start))
        return BC_REC_STORE_DUPLICATE;
    return BC_REC_STORE_OK;
}

bc_rec_result bc_rec_store_open(void *ctx, const bc_rec_start *start,
                                bc_rec_file *file)
{
    bc_rec_store *store = (bc_rec_store *)ctx;
    char path[BC_REC_STORE_PATH_SIZE];
    bc_rec_store_status status;
    struct lfs_info info;
    int error;

    if (!store_valid(store) || !start_valid(start) || file == NULL)
        return BC_REC_INVALID;
    if (store->active || store->reader_active)
        return BC_REC_BUSY;
    if (!store->format_set)
        return BC_REC_UNSUPPORTED;

    status = ensure_directory(store);
    if (status != BC_REC_STORE_OK)
        return map_port_open_status(status);

    path_for_id(path, start->id, ".raw");
    status = stat_path(store, path, &info);
    if (status == BC_REC_STORE_OK) {
        status = inspect_existing_for_start(store, start, file);
        if (status == BC_REC_STORE_OK) {
            if (file->bytes == 0U)
                return BC_REC_EMPTY_AUDIO;
            return BC_REC_ALREADY_EXISTS;
        }
        return map_port_open_status(status);
    }
    if (status != BC_REC_STORE_NOT_FOUND)
        return map_port_open_status(status);

    /* A tombstone occupies the id even after raw deletion. */
    path_for_id(path, start->id, ".done");
    status = stat_path(store, path, &info);
    if (status == BC_REC_STORE_OK) {
        status = inspect_existing_for_start(store, start, file);
        if (status == BC_REC_STORE_OK) {
            if (file->bytes == 0U)
                return BC_REC_EMPTY_AUDIO;
            return BC_REC_ALREADY_EXISTS;
        }
        return map_port_open_status(status);
    }
    if (status != BC_REC_STORE_NOT_FOUND)
        return map_port_open_status(status);

    memset(file, 0, sizeof(*file));
    if (!store->namer(store->namer_ctx, start, file->name) ||
        !name_valid(file->name))
        return BC_REC_INVALID;

    file->complete = false;
    file->recovered = false;
    file->audio = store->format;
    file->audio.sample_count = 0U;
    store->final_samples = 0U;
    store->active_start = *start;
    store->active_id = start->id;
    store->current = *file;
    store->durable = *file;
    store->crc_state = 0xffffffffUL;
    store->durable_crc_state = store->crc_state;
    store->io_error = BC_REC_OK;
    encode_metadata(store->metadata, start, file, 0U, 0U, 0U);
    memcpy(store->durable_metadata, store->metadata,
           BC_REC_STORE_METADATA_SIZE);

    path_for_id(store->path, start->id, ".raw");
    error = open_with_metadata(store, store->path,
                               LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL, false);
    if (error == LFS_ERR_EXIST) {
        /* The id won a race or was created between stat and open.  Inspect it
         * read-only and never retry with TRUNC. */
        status = inspect_existing_for_start(store, start, file);
        if (status == BC_REC_STORE_OK) {
            if (file->bytes == 0U)
                return BC_REC_EMPTY_AUDIO;
            return BC_REC_ALREADY_EXISTS;
        }
        return map_port_open_status(status);
    }
    if (error != LFS_ERR_OK)
        return port_result_from_status(
            status_from_lfs(error, BC_REC_STORE_OPEN_ERROR),
            BC_REC_OPEN_ERROR);

    store->active = true;
    store->io_failed = false;
    *file = store->current;
    return BC_REC_OK;
}

bc_rec_result bc_rec_store_append(void *ctx, const uint8_t *data,
                                  uint16_t length)
{
    bc_rec_store *store = (bc_rec_store *)ctx;
    lfs_ssize_t written;

    if (!store_valid(store) || data == NULL || length == 0U)
        return BC_REC_INVALID;
    if (!store->active)
        return BC_REC_BUSY;
    if (store->io_failed)
        return store->io_error == BC_REC_OK ? BC_REC_WRITE_ERROR
                                             : store->io_error;
    if (length > BC_REC_FRAME_MAX)
        return BC_REC_INVALID;
    if (store->current.bytes > UINT32_MAX - length ||
        store->current.frames == UINT32_MAX) {
        store->io_failed = true;
        store->io_error = BC_REC_NO_SPACE;
        return BC_REC_NO_SPACE;
    }

    written = lfs_file_write(store->lfs, &store->file, data, length);
    if (written != (lfs_ssize_t)length) {
        prevent_file_retry(&store->file);
        store->io_failed = true;
        store->io_error = written == LFS_ERR_NOSPC
                        ? BC_REC_NO_SPACE : BC_REC_WRITE_ERROR;
        if (store->io_error == BC_REC_NO_SPACE)
            return BC_REC_NO_SPACE;
        return BC_REC_WRITE_ERROR;
    }

    store->crc_state = crc32_extend(store->crc_state, data, length);
    store->current.bytes += length;
    ++store->current.frames;
    store->current.crc32 = store->crc_state ^ 0xffffffffUL;
    return BC_REC_OK;
}

static void restore_durable(bc_rec_store *store)
{
    store->current = store->durable;
    store->crc_state = store->durable_crc_state;
    memcpy(store->metadata, store->durable_metadata,
           BC_REC_STORE_METADATA_SIZE);
}

bc_rec_result bc_rec_store_checkpoint(void *ctx, bc_rec_file *file)
{
    bc_rec_store *store = (bc_rec_store *)ctx;
    int error;

    if (!store_valid(store) || file == NULL)
        return BC_REC_INVALID;
    if (!store->active)
        return BC_REC_BUSY;
    if (store->io_failed) {
        *file = store->durable;
        return store->io_error == BC_REC_OK ? BC_REC_SYNC_ERROR
                                             : store->io_error;
    }

    encode_metadata(store->metadata, &store->active_start, &store->current,
                    0U, 0U, 0U);
    error = lfs_file_sync(store->lfs, &store->file);
    if (error != LFS_ERR_OK) {
        prevent_file_retry(&store->file);
        store->io_failed = true;
        restore_durable(store);
        *file = store->durable;
        store->io_error = error == LFS_ERR_NOSPC
                        ? BC_REC_NO_SPACE : BC_REC_SYNC_ERROR;
        return store->io_error;
    }

    store->durable = store->current;
    store->durable.recovered = false;
    store->durable_crc_state = store->crc_state;
    memcpy(store->durable_metadata, store->metadata,
           BC_REC_STORE_METADATA_SIZE);
    *file = store->durable;
    return BC_REC_OK;
}

static bc_rec_result close_active(bc_rec_store *store, bc_rec_result prior)
{
    int error = lfs_file_close(store->lfs, &store->file);
    store->active = false;
    if (prior == BC_REC_OK && error != LFS_ERR_OK) {
        if (error == LFS_ERR_NOSPC)
            return BC_REC_NO_SPACE;
        return BC_REC_CLOSE_ERROR;
    }
    return prior;
}

static bc_rec_result mark_complete(bc_rec_store *store,
                                   bc_rec_file *verified)
{
    bc_rec_file complete_file = *verified;
    bc_rec_start marked_start;
    bc_rec_file marked_file;
    bc_rec_store_status status;
    int error;
    int close_error;

    complete_file.complete = true;
    complete_file.recovered = false;
    if (complete_file.audio.codec == BC_AUDIO_CODEC_OPUS)
        complete_file.audio.sample_count = store->final_samples;
    encode_metadata(store->metadata, &store->active_start, &complete_file,
                    META_FLAG_COMPLETE, 0U, 0U);

    error = open_with_metadata(store, store->path, LFS_O_WRONLY, false);
    if (error != LFS_ERR_OK)
        return port_result_from_status(
            status_from_lfs(error, BC_REC_STORE_OPEN_ERROR),
            BC_REC_OPEN_ERROR);

    error = lfs_file_sync(store->lfs, &store->file);
    if (error != LFS_ERR_OK)
        prevent_file_retry(&store->file);
    close_error = lfs_file_close(store->lfs, &store->file);
    if (error != LFS_ERR_OK) {
        if (error == LFS_ERR_NOSPC)
            return BC_REC_NO_SPACE;
        return BC_REC_SYNC_ERROR;
    }
    if (close_error != LFS_ERR_OK)
        return close_error == LFS_ERR_NOSPC ? BC_REC_NO_SPACE
                                            : BC_REC_CLOSE_ERROR;

    /* Confirm the completion attribute itself survived the close. */
    status = inspect_metadata_path(store, store->path, store->active_id, false,
                                   &marked_start, &marked_file);
    if (status != BC_REC_STORE_OK)
        return port_result_from_status(status, BC_REC_CLOSE_ERROR);
    if (!same_start(&marked_start, &store->active_start) ||
        !same_file(&marked_file, &complete_file))
        return BC_REC_CRC_ERROR;

    *verified = complete_file;
    store->current = complete_file;
    store->durable = complete_file;
    store->current.crc32 = complete_file.crc32;
    store->durable_crc_state = complete_file.crc32 ^ 0xffffffffUL;
    store->crc_state = store->durable_crc_state;
    memcpy(store->durable_metadata, store->metadata,
           BC_REC_STORE_METADATA_SIZE);
    return BC_REC_OK;
}

bc_rec_result bc_rec_store_finish(void *ctx, bool complete,
                                  bc_rec_file *file)
{
    bc_rec_store *store = (bc_rec_store *)ctx;
    bc_rec_file verified;
    bc_rec_start verified_start;
    bc_rec_store_status status;
    bc_rec_result result = BC_REC_OK;
    int error;

    if (!store_valid(store))
        return BC_REC_INVALID;
    if (!store->active)
        return BC_REC_BUSY;

    /* A failed append/sync can leave LittleFS with an errored open handle.
     * Keep its last committed attribute and still close it below. */
    if (!store->io_failed) {
        encode_metadata(store->metadata, &store->active_start,
                        &store->current, 0U, 0U, 0U);
        error = lfs_file_sync(store->lfs, &store->file);
        if (error != LFS_ERR_OK) {
            prevent_file_retry(&store->file);
            store->io_failed = true;
            restore_durable(store);
            store->io_error = error == LFS_ERR_NOSPC
                            ? BC_REC_NO_SPACE : BC_REC_SYNC_ERROR;
            result = store->io_error;
        } else {
            store->durable = store->current;
            store->durable.recovered = false;
            store->durable_crc_state = store->crc_state;
            memcpy(store->durable_metadata, store->metadata,
                   BC_REC_STORE_METADATA_SIZE);
        }
    } else {
        restore_durable(store);
        result = store->io_error == BC_REC_OK ? BC_REC_WRITE_ERROR
                                               : store->io_error;
    }

    result = close_active(store, result);
    if (result == BC_REC_OK && complete && store->durable.bytes == 0U)
        result = BC_REC_EMPTY_AUDIO;

    if (result == BC_REC_OK && complete) {
        struct lfs_info info;

        /* The append path has already checked every write and maintained the
         * running CRC.  Completion only needs a metadata/length readback;
         * archive transfer uses reader_verify_step for a fresh raw scan. */
        status = stat_path(store, store->path, &info);
        if (status == BC_REC_STORE_OK)
            status = inspect_metadata_path(store, store->path,
                                           store->active_id, false,
                                           &verified_start, &verified);
        if (status == BC_REC_STORE_OK &&
            ((uint64_t)info.size != store->durable.bytes ||
             !same_start(&verified_start, &store->active_start) ||
             verified.complete || verified.bytes != store->durable.bytes ||
             verified.frames != store->durable.frames ||
             verified.crc32 != store->durable.crc32 ||
             !bc_audio_format_equal(&verified.audio, &store->durable.audio) ||
             memcmp(verified.name, store->durable.name,
                    BC_REC_NAME_SIZE) != 0))
            status = BC_REC_STORE_CORRUPT;
        if (status != BC_REC_STORE_OK) {
            result = port_result_from_status(status,
                                             BC_REC_CLOSE_ERROR);
        } else {
            result = mark_complete(store, &verified);
        }
    }

    if (file != NULL) {
        *file = store->durable;
        file->complete = result == BC_REC_OK && complete;
        file->recovered = false;
        if (result == BC_REC_EMPTY_AUDIO)
            file->complete = false;
    }
    store->final_samples = 0U;
    return result;
}

bc_rec_store_status bc_rec_store_lookup(bc_rec_store *store, uint64_t id,
                                         bc_rec_start *start,
                                         bc_rec_file *file)
{
    if (!store_valid(store) || id == 0U || start == NULL || file == NULL)
        return BC_REC_STORE_INVALID;
    if (store->active || store->reader_active)
        return BC_REC_STORE_BUSY;
    return inspect_id(store, id, start, file, NULL);
}

bc_rec_store_status bc_rec_store_stat(bc_rec_store *store, uint64_t id,
                                       bc_rec_start *start,
                                       bc_rec_file *file)
{
    if (!store_valid(store) || id == 0U || start == NULL || file == NULL)
        return BC_REC_STORE_INVALID;
    if (store->active || store->reader_active)
        return BC_REC_STORE_BUSY;
    return inspect_id_metadata(store, id, start, file, NULL);
}

bc_rec_store_status bc_rec_store_list_begin(bc_rec_store *store,
                                             bc_rec_store_cursor *cursor)
{
    int error;

    if (!store_valid(store) || cursor == NULL)
        return BC_REC_STORE_INVALID;
    if (store->active || store->reader_active)
        return BC_REC_STORE_BUSY;
    memset(cursor, 0, sizeof(*cursor));
    error = lfs_dir_open(store->lfs, &cursor->dir, BC_REC_STORE_DIR);
    if (error != LFS_ERR_OK)
        return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);
    cursor->open = true;
    return BC_REC_STORE_OK;
}

bc_rec_store_status bc_rec_store_list_next(bc_rec_store *store,
                                            bc_rec_store_cursor *cursor,
                                            bc_rec_start *start,
                                            bc_rec_file *file)
{
    struct lfs_info info;
    uint64_t id;
    bool tombstone;
    bc_rec_store_status status;
    char raw_path[BC_REC_STORE_PATH_SIZE];
    struct lfs_info raw_info;
    int error;

    if (!store_valid(store) || cursor == NULL || !cursor->open ||
        start == NULL || file == NULL)
        return BC_REC_STORE_INVALID;
    if (store->active || store->reader_active)
        return BC_REC_STORE_BUSY;

    for (;;) {
        error = lfs_dir_read(store->lfs, &cursor->dir, &info);
        if (error < 0)
            return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);
        if (error == 0)
            return BC_REC_STORE_END;
        if (info.type != LFS_TYPE_REG ||
            !parse_record_name(info.name, &id, &tombstone))
            continue;

        if (tombstone) {
            path_for_id(raw_path, id, ".raw");
            /* A receipt may be persisted before the remove.  Present the
             * live raw record once and defer the tombstone until deletion. */
            if (lfs_stat(store->lfs, raw_path, &raw_info) == LFS_ERR_OK)
                continue;
        }

        status = inspect_id(store, id, start, file, NULL);
        if (status == BC_REC_STORE_NOT_FOUND)
            continue;
        return status;
    }
}

bc_rec_store_status bc_rec_store_metadata_next(
    bc_rec_store *store, bc_rec_store_cursor *cursor, bc_rec_start *start,
    bc_rec_file *file)
{
    struct lfs_info info;
    struct lfs_info companion_info;
    bc_rec_start candidate_start;
    bc_rec_file candidate_file;
    bc_rec_start receipt_start;
    bc_rec_file receipt_file;
    uint64_t id;
    bool tombstone;
    bc_rec_store_status status;
    char path[BC_REC_STORE_PATH_SIZE];
    char companion_path[BC_REC_STORE_PATH_SIZE];
    int error;

    if (!store_valid(store) || cursor == NULL || !cursor->open ||
        cursor->catalog || start == NULL || file == NULL)
        return BC_REC_STORE_INVALID;
    if (store->active || store->reader_active)
        return BC_REC_STORE_BUSY;

    /* One call owns exactly one directory read.  This lets a worker bound
     * both filesystem work and response latency while it scans old files. */
    error = lfs_dir_read(store->lfs, &cursor->dir, &info);
    if (error < 0)
        return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);
    if (error == 0)
        return BC_REC_STORE_END;
    if (info.type != LFS_TYPE_REG ||
        !parse_record_name(info.name, &id, &tombstone))
        return BC_REC_STORE_MORE;

    path_for_id(path, id, tombstone ? ".done" : ".raw");
    if (tombstone) {
        /* A receipt tombstone is only the visible record after raw removal.
         * While both exist, the raw entry below owns the catalog row. */
        path_for_id(companion_path, id, ".raw");
        error = lfs_stat(store->lfs, companion_path, &companion_info);
        if (error == LFS_ERR_OK)
            return BC_REC_STORE_MORE;
        if (error != LFS_ERR_NOENT)
            return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);
    }

    status = inspect_metadata_path(store, path, id, tombstone,
                                   &candidate_start, &candidate_file);
    if (status != BC_REC_STORE_OK)
        return status;

    if (!tombstone) {
        path_for_id(companion_path, id, ".done");
        error = lfs_stat(store->lfs, companion_path, &companion_info);
        if (error == LFS_ERR_OK) {
            status = inspect_metadata_path(store, companion_path, id, true,
                                           &receipt_start, &receipt_file);
            if (status == BC_REC_STORE_CORRUPT &&
                is_attrless_empty_file(store, companion_path,
                                       &companion_info)) {
                /* An interrupted first receipt sync leaves an empty name;
                 * the still-live raw metadata remains the authoritative row. */
            } else if (status != BC_REC_STORE_OK ||
                       !same_start(&receipt_start, &candidate_start) ||
                       !same_file(&receipt_file, &candidate_file)) {
                return status == BC_REC_STORE_OK
                     ? BC_REC_STORE_CORRUPT : status;
            } else {
                candidate_file.delivered = true;
            }
        } else if (error != LFS_ERR_NOENT) {
            return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);
        }
    }

    *start = candidate_start;
    *file = candidate_file;
    return BC_REC_STORE_OK;
}

bc_rec_store_status bc_rec_store_list_end(bc_rec_store *store,
                                          bc_rec_store_cursor *cursor)
{
    int error;

    if (!store_valid(store) || cursor == NULL || !cursor->open)
        return BC_REC_STORE_INVALID;
    error = lfs_dir_close(store->lfs, &cursor->dir);
    cursor->open = false;
    if (error != LFS_ERR_OK)
        return status_from_lfs(error, BC_REC_STORE_CLOSE_ERROR);
    return BC_REC_STORE_OK;
}

bc_rec_store_status bc_rec_store_catalog_begin(bc_rec_store *store,
                                               bc_rec_store_cursor *cursor,
                                               uint64_t after_id)
{
    int error;

    if (!store_valid(store) || cursor == NULL)
        return BC_REC_STORE_INVALID;
    if (store->active || store->reader_active)
        return BC_REC_STORE_BUSY;
    memset(cursor, 0, sizeof(*cursor));
    error = lfs_dir_open(store->lfs, &cursor->dir, BC_REC_STORE_DIR);
    if (error != LFS_ERR_OK)
        return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);
    cursor->open = true;
    cursor->catalog = true;
    cursor->after_id = after_id;
    return BC_REC_STORE_OK;
}

bc_rec_store_status bc_rec_store_catalog_next(bc_rec_store *store,
                                               bc_rec_store_cursor *cursor,
                                               bc_rec_start *start,
                                               bc_rec_file *file,
                                               bool *crc_verified)
{
    struct lfs_info info;
    struct lfs_info raw_info;
    bc_rec_start candidate_start;
    bc_rec_file candidate_file;
    bc_rec_start best_start;
    bc_rec_file best_file;
    uint64_t id;
    uint64_t best_id = 0U;
    bool tombstone;
    bool found = false;
    bc_rec_store_status status;
    char path[BC_REC_STORE_PATH_SIZE];
    char raw_path[BC_REC_STORE_PATH_SIZE];
    char tombstone_path[BC_REC_STORE_PATH_SIZE];
    int error;

    if (!store_valid(store) || cursor == NULL || !cursor->open ||
        !cursor->catalog || start == NULL || file == NULL ||
        crc_verified == NULL)
        return BC_REC_STORE_INVALID;
    if (store->active || store->reader_active)
        return BC_REC_STORE_BUSY;

    error = lfs_dir_rewind(store->lfs, &cursor->dir);
    if (error != LFS_ERR_OK)
        return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);

    for (;;) {
        error = lfs_dir_read(store->lfs, &cursor->dir, &info);
        if (error < 0)
            return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);
        if (error == 0)
            break;
        if (info.type != LFS_TYPE_REG ||
            !parse_record_name(info.name, &id, &tombstone) ||
            id <= cursor->after_id || (found && id >= best_id))
            continue;

        path_for_id(path, id, tombstone ? ".done" : ".raw");
        if (tombstone) {
            path_for_id(raw_path, id, ".raw");
            error = lfs_stat(store->lfs, raw_path, &raw_info);
            if (error == LFS_ERR_OK)
                continue;
            if (error != LFS_ERR_NOENT)
                return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);
        }

        status = inspect_metadata_path(store, path, id, tombstone,
                                       &candidate_start, &candidate_file);
        if (status != BC_REC_STORE_OK)
            return status;

        if (!tombstone) {
            path_for_id(tombstone_path, id, ".done");
            error = lfs_stat(store->lfs, tombstone_path, &raw_info);
            if (error == LFS_ERR_OK) {
                bc_rec_start receipt_start;
                bc_rec_file receipt_file;
                status = inspect_metadata_path(store, tombstone_path, id, true,
                                               &receipt_start, &receipt_file);
                if (status == BC_REC_STORE_CORRUPT &&
                    is_attrless_empty_file(store, tombstone_path, &raw_info)) {
                    /* A failed receipt sync left an uncommitted placeholder. */
                } else if (status != BC_REC_STORE_OK ||
                           !same_start(&receipt_start, &candidate_start) ||
                           !same_file(&receipt_file, &candidate_file))
                    return status == BC_REC_STORE_OK
                         ? BC_REC_STORE_CORRUPT : status;
                else
                    candidate_file.delivered = true;
            } else if (error != LFS_ERR_NOENT) {
                return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);
            }
        }

        found = true;
        best_id = id;
        best_start = candidate_start;
        best_file = candidate_file;
    }

    if (!found)
        return BC_REC_STORE_END;
    cursor->after_id = best_id;
    *start = best_start;
    *file = best_file;
    *crc_verified = false;
    return BC_REC_STORE_OK;
}

bc_rec_store_status bc_rec_store_reader_begin(bc_rec_store *store,
                                              uint64_t id,
                                              bc_rec_store_reader *reader,
                                              bc_rec_start *start,
                                              bc_rec_file *file)
{
    char path[BC_REC_STORE_PATH_SIZE];
    char tombstone_path[BC_REC_STORE_PATH_SIZE];
    struct lfs_info info;
    bc_rec_store_status status;
    lfs_soff_t raw_size;
    int error;

    if (!store_valid(store) || id == 0U || reader == NULL || start == NULL ||
        file == NULL)
        return BC_REC_STORE_INVALID;
    if (store->active || store->reader_active)
        return BC_REC_STORE_BUSY;

    memset(reader, 0, sizeof(*reader));
    reader->owner = store;
    reader->id = id;
    reader->file_attr.type = BC_REC_STORE_META_ATTR;
    reader->file_attr.buffer = reader->metadata;
    reader->file_attr.size = BC_REC_STORE_METADATA_SIZE;
    reader->file_config.buffer = reader->file_cache;
    reader->file_config.attrs = &reader->file_attr;
    reader->file_config.attr_count = 1U;

    path_for_id(path, id, ".raw");
    error = lfs_stat(store->lfs, path, &info);
    if (error == LFS_ERR_NOENT) {
        path_for_id(tombstone_path, id, ".done");
        status = inspect_tombstone_path(store, tombstone_path, id, start, file);
        reader->owner = NULL;
        return status == BC_REC_STORE_OK ? BC_REC_STORE_DELETED : status;
    }
    if (error != LFS_ERR_OK) {
        reader->owner = NULL;
        return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);
    }
    if (info.type != LFS_TYPE_REG) {
        reader->owner = NULL;
        return BC_REC_STORE_CORRUPT;
    }

    memset(reader->metadata, 0, sizeof(reader->metadata));
    error = lfs_file_opencfg(store->lfs, &reader->file, path, LFS_O_RDONLY,
                             &reader->file_config);
    if (error != LFS_ERR_OK) {
        reader->owner = NULL;
        return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);
    }

    status = decode_metadata(reader->metadata, id, false, start, file,
                             NULL, NULL);
    if (status != BC_REC_STORE_OK)
        goto reader_close;
    raw_size = lfs_file_size(store->lfs, &reader->file);
    if (raw_size < 0 || (uint64_t)raw_size < file->bytes ||
        (file->complete && (uint64_t)raw_size != file->bytes)) {
        status = raw_size < 0
            ? status_from_lfs((int)raw_size, BC_REC_STORE_OPEN_ERROR)
            : BC_REC_STORE_CORRUPT;
        goto reader_close;
    }
    if (file->bytes == 0U) {
        status = BC_REC_STORE_EMPTY;
        goto reader_close;
    }

    /* Check a receipt's metadata while the reader is still staged.  This is
     * a bounded operation and does not scan the raw content. */
    path_for_id(tombstone_path, id, ".done");
    error = lfs_stat(store->lfs, tombstone_path, &info);
    if (error == LFS_ERR_OK) {
        bc_rec_start receipt_start;
        bc_rec_file receipt_file;
        status = inspect_metadata_path(store, tombstone_path, id, true,
                                       &receipt_start, &receipt_file);
        if (status == BC_REC_STORE_CORRUPT &&
            is_attrless_empty_file(store, tombstone_path, &info)) {
            /* An interrupted receipt has no custody data yet. */
        } else if (status != BC_REC_STORE_OK ||
                   !same_start(&receipt_start, start) ||
                   !same_file(&receipt_file, file)) {
            status = status == BC_REC_STORE_OK ? BC_REC_STORE_CORRUPT : status;
            goto reader_close;
        } else {
            file->delivered = true;
        }
    } else if (error != LFS_ERR_NOENT) {
        status = status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);
        goto reader_close;
    }

    file->recovered = !file->complete;
    reader->bytes = file->bytes;
    reader->expected_crc32 = file->crc32;
    reader->verify_crc_state = 0xffffffffUL;
    reader->verify_offset = 0U;
    reader->verified = false;
    reader->open = true;
    store->reader_active = true;
    return BC_REC_STORE_OK;

reader_close:
    error = lfs_file_close(store->lfs, &reader->file);
    if (status == BC_REC_STORE_OK && error != LFS_ERR_OK)
        status = status_from_lfs(error, BC_REC_STORE_CLOSE_ERROR);
    reader->owner = NULL;
    return status;
}

bc_rec_store_status bc_rec_store_reader_verify_step(
    bc_rec_store_reader *reader, uint32_t max_bytes)
{
    uint32_t remaining;
    uint32_t budget;

    if (reader == NULL || !reader->open || reader->owner == NULL)
        return BC_REC_STORE_INVALID;
    if (reader->verified)
        return BC_REC_STORE_OK;
    if (max_bytes == 0U)
        return BC_REC_STORE_MORE;
    if (reader->verify_offset > reader->bytes)
        return BC_REC_STORE_CORRUPT;

    remaining = reader->bytes - reader->verify_offset;
    budget = remaining < max_bytes ? remaining : max_bytes;
    while (budget != 0U) {
        lfs_size_t request = budget > BC_REC_STORE_CACHE_SIZE
                           ? BC_REC_STORE_CACHE_SIZE : budget;
        lfs_ssize_t got = lfs_file_read(reader->owner->lfs, &reader->file,
                                        reader->read_buffer, request);
        if (got < 0)
            return status_from_lfs((int)got, BC_REC_STORE_OPEN_ERROR);
        if ((lfs_size_t)got != request)
            return BC_REC_STORE_CORRUPT;
        reader->verify_crc_state = crc32_extend(reader->verify_crc_state,
                                                reader->read_buffer,
                                                (size_t)got);
        reader->verify_offset += request;
        budget -= request;
    }

    if (reader->verify_offset != reader->bytes)
        return BC_REC_STORE_MORE;
    if ((reader->verify_crc_state ^ 0xffffffffUL) !=
        reader->expected_crc32)
        return BC_REC_STORE_CORRUPT;
    reader->verified = true;
    return BC_REC_STORE_OK;
}

bc_rec_store_status bc_rec_store_reader_open(bc_rec_store *store, uint64_t id,
                                             bc_rec_store_reader *reader,
                                             bc_rec_start *start,
                                             bc_rec_file *file)
{
    bc_rec_store_status status;

    status = bc_rec_store_reader_begin(store, id, reader, start, file);
    if (status != BC_REC_STORE_OK)
        return status;
    while (!reader->verified) {
        status = bc_rec_store_reader_verify_step(reader,
                                                  BC_REC_STORE_CACHE_SIZE);
        if (status != BC_REC_STORE_MORE)
            break;
    }
    if (status != BC_REC_STORE_OK) {
        (void)bc_rec_store_reader_close(reader);
        return status;
    }
    return BC_REC_STORE_OK;
}

int32_t bc_rec_store_reader_read(bc_rec_store_reader *reader,
                                 uint32_t offset, uint8_t *data,
                                 uint32_t capacity)
{
    lfs_ssize_t seek_result;
    lfs_ssize_t read_result;
    uint32_t count;

    if (reader == NULL || !reader->open || reader->owner == NULL)
        return BC_REC_STORE_INVALID;
    if (!reader->verified)
        return BC_REC_STORE_UNVERIFIED;
    if (capacity != 0U && data == NULL)
        return BC_REC_STORE_INVALID;
    if (offset >= reader->bytes || capacity == 0U)
        return 0;
    count = reader->bytes - offset;
    if (count > capacity)
        count = capacity;
    seek_result = lfs_file_seek(reader->owner->lfs, &reader->file,
                                (lfs_soff_t)offset, LFS_SEEK_SET);
    if (seek_result < 0)
        return (int32_t)status_from_lfs((int)seek_result,
                                        BC_REC_STORE_OPEN_ERROR);
    read_result = lfs_file_read(reader->owner->lfs, &reader->file, data, count);
    if (read_result < 0)
        return (int32_t)status_from_lfs((int)read_result,
                                        BC_REC_STORE_OPEN_ERROR);
    if ((uint32_t)read_result != count)
        return BC_REC_STORE_CORRUPT;
    return (int32_t)read_result;
}

bc_rec_store_status bc_rec_store_reader_close(bc_rec_store_reader *reader)
{
    bc_rec_store *store;
    int error;

    if (reader == NULL || !reader->open || reader->owner == NULL)
        return BC_REC_STORE_INVALID;
    store = reader->owner;
    error = lfs_file_close(store->lfs, &reader->file);
    reader->open = false;
    reader->owner = NULL;
    store->reader_active = false;
    if (error != LFS_ERR_OK)
        return status_from_lfs(error, BC_REC_STORE_CLOSE_ERROR);
    return BC_REC_STORE_OK;
}

int32_t bc_rec_store_read(bc_rec_store *store, uint64_t id, uint32_t offset,
                          uint8_t *data, uint32_t capacity)
{
    bc_rec_store_reader reader;
    bc_rec_start start;
    bc_rec_file file;
    bc_rec_store_status status;
    bc_rec_store_status close_status;
    int32_t count;

    if (!store_valid(store) || id == 0U ||
        (capacity != 0U && data == NULL))
        return BC_REC_STORE_INVALID;
    if (store->active || store->reader_active)
        return BC_REC_STORE_BUSY;

    status = bc_rec_store_reader_open(store, id, &reader, &start, &file);
    if (status != BC_REC_STORE_OK)
        return (int32_t)status;
    count = bc_rec_store_reader_read(&reader, offset, data, capacity);
    close_status = bc_rec_store_reader_close(&reader);
    if (count < 0)
        return count;
    if (close_status != BC_REC_STORE_OK)
        return (int32_t)close_status;
    return count;
}

static bc_rec_store_status persist_receipt_tombstone(
    bc_rec_store *store, const bc_rec_start *start, const bc_rec_file *file,
    uint32_t bytes, uint32_t crc32)
{
    char path[BC_REC_STORE_PATH_SIZE];
    struct lfs_info info;
    uint8_t flags = META_FLAG_TOMBSTONE | META_FLAG_CUSTODY;
    bool create;
    int error;
    int close_error;
    bc_rec_start existing_start;
    bc_rec_file existing_file;
    bc_rec_store_status status;

    path_for_id(path, start->id, ".done");
    status = stat_path(store, path, &info);
    if (status == BC_REC_STORE_OK) {
        status = inspect_tombstone_path(store, path, start->id,
                                         &existing_start, &existing_file);
        if (status == BC_REC_STORE_OK)
            return same_start(&existing_start, start) &&
                           same_file(&existing_file, file) &&
                           existing_file.bytes == bytes &&
                           existing_file.crc32 == crc32
                 ? BC_REC_STORE_OK : BC_REC_STORE_RECEIPT_MISMATCH;

        /* lfs_file_opencfg creates the directory entry before its first
         * atomic attribute commit.  A failed first sync can therefore leave
         * an empty, attr-less .done entry.  It is safe to resume that exact
         * placeholder from a valid raw receipt, while a present but corrupt
         * attribute remains an honest corruption error. */
        if (status != BC_REC_STORE_CORRUPT || info.size != 0U)
            return status;
        {
            uint8_t probe;
            lfs_ssize_t attr_status = lfs_getattr(
                store->lfs, path, BC_REC_STORE_META_ATTR, &probe, 1U);
            if (attr_status != LFS_ERR_NOATTR)
                return status;
        }
        create = false;
    } else if (status == BC_REC_STORE_NOT_FOUND) {
        create = true;
    } else {
        return status;
    }

    if (file->recovered)
        flags |= META_FLAG_RECOVERED;
    else
        flags |= META_FLAG_COMPLETE;
    encode_metadata(store->metadata, start, file, flags, bytes, crc32);
    error = open_with_metadata(store, path,
                               LFS_O_WRONLY | (create ?
                               (LFS_O_CREAT | LFS_O_EXCL) : 0), false);
    if (error == LFS_ERR_EXIST && create)
        return persist_receipt_tombstone(store, start, file, bytes, crc32);
    if (error != LFS_ERR_OK)
        return status_from_lfs(error, BC_REC_STORE_OPEN_ERROR);

    error = lfs_file_sync(store->lfs, &store->file);
    if (error != LFS_ERR_OK)
        prevent_file_retry(&store->file);
    close_error = lfs_file_close(store->lfs, &store->file);
    if (error != LFS_ERR_OK)
        return status_from_lfs(error, BC_REC_STORE_SYNC_ERROR);
    if (close_error != LFS_ERR_OK)
        return status_from_lfs(close_error, BC_REC_STORE_CLOSE_ERROR);

    /* The receipt is not actionable until the committed attribute can be
     * read back and its checksum/identity matches the request. */
    status = inspect_tombstone_path(store, path, start->id,
                                    &existing_start, &existing_file);
    if (status != BC_REC_STORE_OK)
        return status;
    if (!same_start(&existing_start, start) ||
        !same_file(&existing_file, file) ||
        existing_file.bytes != bytes || existing_file.crc32 != crc32)
        return BC_REC_STORE_CORRUPT;
    return BC_REC_STORE_OK;
}

bc_rec_store_status bc_rec_store_receipt(bc_rec_store *store, uint64_t id,
                                          uint32_t bytes, uint32_t crc32)
{
    char raw_path[BC_REC_STORE_PATH_SIZE];
    char tombstone_path[BC_REC_STORE_PATH_SIZE];
    bc_rec_start start;
    bc_rec_file file;
    bc_rec_store_status status;
    struct lfs_info info;

    if (!store_valid(store) || id == 0U || bytes == 0U)
        return BC_REC_STORE_INVALID;
    if (store->active || store->reader_active)
        return BC_REC_STORE_BUSY;

    /* The phone receipt carries the exact byte count and CRC.  Matching it
     * against the checksum-validated durable metadata is sufficient custody
     * evidence and avoids a full raw scan on the worker. */
    path_for_id(raw_path, id, ".raw");
    status = stat_path(store, raw_path, &info);
    if (status == BC_REC_STORE_OK) {
        status = inspect_metadata_path(store, raw_path, id, false,
                                       &start, &file);
    } else if (status == BC_REC_STORE_NOT_FOUND) {
        path_for_id(tombstone_path, id, ".done");
        status = inspect_tombstone_path(store, tombstone_path, id, &start,
                                        &file);
    }
    if (status != BC_REC_STORE_OK)
        return status;
    if (file.bytes == 0U)
        return BC_REC_STORE_EMPTY;
    if ((!file.complete && !file.recovered) || file.bytes != bytes ||
        file.crc32 != crc32)
        return BC_REC_STORE_RECEIPT_MISMATCH;
    return persist_receipt_tombstone(store, &start, &file, bytes, crc32);
}

bc_rec_store_status bc_rec_store_delete(bc_rec_store *store, uint64_t id,
                                         uint32_t bytes, uint32_t crc32)
{
    char tombstone_path[BC_REC_STORE_PATH_SIZE];
    char raw_path[BC_REC_STORE_PATH_SIZE];
    bc_rec_start start;
    bc_rec_file tombstone;
    bc_rec_store_status status;
    struct lfs_info info;
    int error;

    if (!store_valid(store) || id == 0U || bytes == 0U)
        return BC_REC_STORE_INVALID;
    if (store->active || store->reader_active)
        return BC_REC_STORE_BUSY;

    path_for_id(tombstone_path, id, ".done");
    status = inspect_tombstone_path(store, tombstone_path, id, &start,
                                    &tombstone);
    if (status == BC_REC_STORE_NOT_FOUND)
        return BC_REC_STORE_NOT_TERMINAL;
    if (status != BC_REC_STORE_OK)
        return status;
    if (tombstone.bytes != bytes || tombstone.crc32 != crc32)
        return BC_REC_STORE_RECEIPT_MISMATCH;

    path_for_id(raw_path, id, ".raw");
    error = lfs_stat(store->lfs, raw_path, &info);
    if (error == LFS_ERR_NOENT)
        return BC_REC_STORE_OK;
    if (error != LFS_ERR_OK || info.type != LFS_TYPE_REG)
        return status_from_lfs(error, BC_REC_STORE_DELETE_ERROR);
    /* The checked tombstone is the custody record.  Once it exists, raw
     * bytes may be retired without rescanning them; this also permits cleanup
     * after a raw bit error discovered after the phone accepted the file. */
    error = lfs_remove(store->lfs, raw_path);
    if (error != LFS_ERR_OK)
        return status_from_lfs(error, BC_REC_STORE_DELETE_ERROR);
    error = lfs_stat(store->lfs, raw_path, &info);
    if (error != LFS_ERR_NOENT)
        return BC_REC_STORE_DELETE_ERROR;
    return BC_REC_STORE_OK;
}
