#include "bc_voice_legacy_archive.h"

#include <string.h>

static uint32_t get_le32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static void put_le32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static bc_rec_result result_from_store(bc_rec_store_status status)
{
    switch (status) {
    case BC_REC_STORE_OK:
        return BC_REC_OK;
    case BC_REC_STORE_END:
    case BC_REC_STORE_NOT_FOUND:
        return BC_REC_NOT_FOUND;
    case BC_REC_STORE_MORE:
    case BC_REC_STORE_BUSY:
    case BC_REC_STORE_UNVERIFIED:
        return BC_REC_BUSY;
    case BC_REC_STORE_DUPLICATE:
        return BC_REC_DUPLICATE;
    case BC_REC_STORE_NO_SPACE:
        return BC_REC_NO_SPACE;
    case BC_REC_STORE_OPEN_ERROR:
        return BC_REC_OPEN_ERROR;
    case BC_REC_STORE_WRITE_ERROR:
        return BC_REC_WRITE_ERROR;
    case BC_REC_STORE_SYNC_ERROR:
        return BC_REC_SYNC_ERROR;
    case BC_REC_STORE_CLOSE_ERROR:
        return BC_REC_CLOSE_ERROR;
    case BC_REC_STORE_EMPTY:
        return BC_REC_EMPTY_AUDIO;
    case BC_REC_STORE_RECEIPT_MISMATCH:
        return BC_REC_CUSTODY_REQUIRED;
    case BC_REC_STORE_DELETED:
        return BC_REC_NOT_FOUND;
    case BC_REC_STORE_CORRUPT:
        return BC_REC_CRC_ERROR;
    case BC_REC_STORE_INVALID:
    case BC_REC_STORE_DELETE_ERROR:
    default:
        return BC_REC_INVALID;
    }
}

static bool name_bytes_valid(const uint8_t *name)
{
    uint32_t i;

    if (name == NULL || name[0] == '.' ||
        (name[0] == '.' && name[1] == '.'))
        return false;
    if (name[BC_VOICE_LEGACY_NAME_SIZE - 1U] == 0U)
        return false;
    if (name[33U] != (uint8_t)'8' && name[33U] != (uint8_t)'B' &&
        name[33U] != (uint8_t)'D')
        return false;
    for (i = 0; i < BC_VOICE_LEGACY_NAME_SIZE; ++i) {
        if (name[i] == 0U || name[i] == (uint8_t)'/' ||
            name[i] == (uint8_t)'\\')
            return false;
    }
    return true;
}

static bool info_name_valid(const struct lfs_info *info,
                            uint8_t name[BC_VOICE_LEGACY_NAME_SIZE])
{
    size_t length = 0U;

    if (info == NULL || name == NULL || info->type != LFS_TYPE_REG)
        return false;
    while (length < BC_VOICE_LEGACY_NAME_SIZE && info->name[length] != '\0')
        ++length;
    if (length != BC_VOICE_LEGACY_NAME_SIZE ||
        info->name[BC_VOICE_LEGACY_NAME_SIZE] != '\0')
        return false;
    memcpy(name, info->name, BC_VOICE_LEGACY_NAME_SIZE);
    return name_bytes_valid(name);
}

static bool store_file_name_valid(const bc_rec_file *file,
                                  uint8_t name[BC_VOICE_LEGACY_NAME_SIZE])
{
    if (file == NULL || name == NULL || file->name[BC_VOICE_LEGACY_NAME_SIZE] !=
        '\0')
        return false;
    memcpy(name, file->name, BC_VOICE_LEGACY_NAME_SIZE);
    return name_bytes_valid(name);
}

static bool same_name(const uint8_t left[BC_VOICE_LEGACY_NAME_SIZE],
                      const uint8_t right[BC_VOICE_LEGACY_NAME_SIZE])
{
    return left != NULL && right != NULL &&
           memcmp(left, right, BC_VOICE_LEGACY_NAME_SIZE) == 0;
}

static bool archive_valid(const bc_voice_legacy_archive *archive)
{
    return archive != NULL && archive->initialized && archive->lfs != NULL &&
           archive->lfs->cfg != NULL && archive->send != NULL;
}

static bool phase_active(bc_voice_legacy_archive_phase phase)
{
    return phase != BC_VOICE_LEGACY_IDLE;
}

static bc_rec_result lfs_result(int error, bc_rec_result fallback)
{
    if (error == LFS_ERR_NOSPC)
        return BC_REC_NO_SPACE;
    if (error == LFS_ERR_NOENT)
        return BC_REC_NOT_FOUND;
    if (error == LFS_ERR_CORRUPT)
        return BC_REC_CRC_ERROR;
    return fallback;
}

static void reset_operation(bc_voice_legacy_archive *archive)
{
    archive->phase = BC_VOICE_LEGACY_IDLE;
    archive->after_send = BC_VOICE_LEGACY_IDLE;
    archive->source = BC_VOICE_LEGACY_SOURCE_NONE;
    archive->epoch = 0U;
    archive->att_limit = 0U;
    memset(archive->request_header, 0, sizeof(archive->request_header));
    memset(archive->requested_name, 0, sizeof(archive->requested_name));
    archive->requested_offset = 0U;
    archive->list_total = 0U;
    archive->list_emitted = 0U;
    archive->new_id = 0U;
    memset(&archive->new_start, 0, sizeof(archive->new_start));
    memset(&archive->new_file, 0, sizeof(archive->new_file));
    memset(&archive->old_info, 0, sizeof(archive->old_info));
    memset(archive->old_path, 0, sizeof(archive->old_path));
    archive->file_size = 0U;
    archive->transfer_offset = 0U;
    archive->transfer_sequence = 0U;
    archive->transfer_chunk = 0U;
    archive->tx_length = 0U;
    archive->tx_pending = false;
}

static bc_rec_result close_resources(bc_voice_legacy_archive *archive)
{
    bc_rec_result result = BC_REC_OK;
    int error;
    bc_rec_store_status store_status;

    if (archive->new_reader.open) {
        store_status = bc_rec_store_reader_close(&archive->new_reader);
        if (store_status != BC_REC_STORE_OK)
            result = result_from_store(store_status);
    }
    if (archive->old_file_open) {
        error = lfs_file_close(archive->lfs, &archive->old_file);
        archive->old_file_open = false;
        if (error != LFS_ERR_OK && result == BC_REC_OK)
            result = lfs_result(error, BC_REC_CLOSE_ERROR);
    }
    if (archive->old_dir_open) {
        error = lfs_dir_close(archive->lfs, &archive->old_dir);
        archive->old_dir_open = false;
        if (error != LFS_ERR_OK && result == BC_REC_OK)
            result = lfs_result(error, BC_REC_CLOSE_ERROR);
    }
    if (archive->new_cursor_open) {
        store_status = bc_rec_store_list_end(archive->store,
                                             &archive->new_cursor);
        archive->new_cursor_open = false;
        if (store_status != BC_REC_STORE_OK && result == BC_REC_OK)
            result = result_from_store(store_status);
    }
    return result;
}

static bc_rec_result fail_operation(bc_voice_legacy_archive *archive,
                                    bc_rec_result result)
{
    bc_rec_result close_result = close_resources(archive);
    reset_operation(archive);
    return result == BC_REC_OK ? close_result : result;
}

static bool count_one(bc_voice_legacy_archive *archive)
{
    if (archive->list_total == UINT32_MAX)
        return false;
    ++archive->list_total;
    return true;
}

static bool list_new_file_eligible(const bc_rec_file *file,
                                   uint8_t name[BC_VOICE_LEGACY_NAME_SIZE])
{
    if (file == NULL || file->bytes == 0U || file->delivered ||
        (!file->complete && !file->recovered))
        return false;
    return store_file_name_valid(file, name);
}

static bool list_old_info_eligible(const struct lfs_info *info,
                                   uint8_t name[BC_VOICE_LEGACY_NAME_SIZE])
{
    return info_name_valid(info, name) &&
           (uint64_t)info->size <= UINT32_MAX;
}

static void prepare_status_packet(bc_voice_legacy_archive *archive,
                                  uint8_t status)
{
    memcpy(archive->tx_packet, archive->request_header,
           BC_VOICE_LEGACY_HEADER_SIZE);
    archive->tx_packet[BC_VOICE_LEGACY_HEADER_SIZE] = status;
    archive->tx_length = BC_VOICE_LEGACY_HEADER_SIZE + 1U;
    archive->tx_pending = true;
    archive->after_send = BC_VOICE_LEGACY_IDLE;
    archive->phase = BC_VOICE_LEGACY_SEND_ONLY;
}

static void prepare_list_packet(bc_voice_legacy_archive *archive,
                                const uint8_t name[BC_VOICE_LEGACY_NAME_SIZE],
                                uint32_t size)
{
    uint8_t *payload = archive->tx_packet + BC_VOICE_LEGACY_HEADER_SIZE;

    memcpy(archive->tx_packet, archive->request_header,
           BC_VOICE_LEGACY_HEADER_SIZE);
    put_le32(payload, archive->list_total);
    put_le32(payload + 4U, archive->list_emitted + 1U);
    put_le32(payload + 8U, size);
    memcpy(payload + BC_VOICE_LEGACY_LIST_FIELDS_SIZE, name,
           BC_VOICE_LEGACY_NAME_SIZE);
    archive->tx_length = BC_VOICE_LEGACY_LIST_PACKET_MAX;
    archive->tx_pending = true;
    archive->after_send = archive->phase == BC_VOICE_LEGACY_LIST_EMIT_OLD
                        ? BC_VOICE_LEGACY_LIST_EMIT_OLD
                        : BC_VOICE_LEGACY_LIST_EMIT_NEW;
    archive->phase = BC_VOICE_LEGACY_SEND_ONLY;
}

static void prepare_empty_list_packet(bc_voice_legacy_archive *archive)
{
    memcpy(archive->tx_packet, archive->request_header,
           BC_VOICE_LEGACY_HEADER_SIZE);
    memset(archive->tx_packet + BC_VOICE_LEGACY_HEADER_SIZE, 0,
           BC_VOICE_LEGACY_LIST_FIELDS_SIZE);
    archive->tx_length = BC_VOICE_LEGACY_HEADER_SIZE +
                         BC_VOICE_LEGACY_LIST_FIELDS_SIZE;
    archive->tx_pending = true;
    archive->after_send = BC_VOICE_LEGACY_IDLE;
    archive->phase = BC_VOICE_LEGACY_SEND_ONLY;
}

static bc_rec_result build_space_packet(bc_voice_legacy_archive *archive)
{
    uint64_t total;
    uint64_t used;
    lfs_ssize_t used_blocks;
    uint8_t *payload;

    if (archive->lfs->cfg->block_size == 0U ||
        archive->lfs->cfg->block_count == 0U)
        return BC_REC_OPEN_ERROR;
    total = (uint64_t)archive->lfs->cfg->block_size *
            archive->lfs->cfg->block_count;
    if (total > UINT32_MAX)
        return BC_REC_NO_SPACE;
    used_blocks = lfs_fs_size(archive->lfs);
    if (used_blocks < 0 || (uint64_t)used_blocks >
        archive->lfs->cfg->block_count)
        return lfs_result((int)used_blocks, BC_REC_OPEN_ERROR);
    used = (uint64_t)(uint32_t)used_blocks * archive->lfs->cfg->block_size;
    if (used > total || used > UINT32_MAX)
        return BC_REC_CRC_ERROR;

    memcpy(archive->tx_packet, archive->request_header,
           BC_VOICE_LEGACY_HEADER_SIZE);
    payload = archive->tx_packet + BC_VOICE_LEGACY_HEADER_SIZE;
    put_le32(payload, (uint32_t)total);
    put_le32(payload + 4U, (uint32_t)used);
    put_le32(payload + 8U, (uint32_t)(total - used));
    archive->tx_length = BC_VOICE_LEGACY_HEADER_SIZE + 12U;
    archive->tx_pending = true;
    archive->after_send = BC_VOICE_LEGACY_IDLE;
    archive->phase = BC_VOICE_LEGACY_SEND_ONLY;
    return BC_REC_OK;
}

static bc_rec_result open_old_directory(bc_voice_legacy_archive *archive)
{
    int error;

    error = lfs_dir_open(archive->lfs, &archive->old_dir, "/");
    if (error != LFS_ERR_OK)
        return lfs_result(error, BC_REC_OPEN_ERROR);
    archive->old_dir_open = true;
    return BC_REC_OK;
}

static bc_rec_result open_new_catalog(bc_voice_legacy_archive *archive)
{
    bc_rec_store_status status;

    if (archive->store == NULL)
        return BC_REC_NOT_FOUND;
    status = bc_rec_store_catalog_begin(archive->store, &archive->new_cursor,
                                        0U);
    if (status != BC_REC_STORE_OK)
        return result_from_store(status);
    archive->new_cursor_open = true;
    return BC_REC_OK;
}

static bc_rec_result close_old_directory(bc_voice_legacy_archive *archive)
{
    int error;

    if (!archive->old_dir_open)
        return BC_REC_OK;
    error = lfs_dir_close(archive->lfs, &archive->old_dir);
    archive->old_dir_open = false;
    return error == LFS_ERR_OK ? BC_REC_OK :
           lfs_result(error, BC_REC_CLOSE_ERROR);
}

static void configure_old_file(bc_voice_legacy_archive *archive)
{
    memset(&archive->old_file_config, 0, sizeof(archive->old_file_config));
    archive->old_file_config.buffer = archive->old_file_cache;
}

static bc_rec_result open_old_file(bc_voice_legacy_archive *archive)
{
    lfs_soff_t seek_result;
    int error;

    archive->old_path[0] = '/';
    memcpy(archive->old_path + 1U, archive->requested_name,
           BC_VOICE_LEGACY_NAME_SIZE);
    archive->old_path[1U + BC_VOICE_LEGACY_NAME_SIZE] = '\0';
    configure_old_file(archive);
    error = lfs_file_opencfg(archive->lfs, &archive->old_file,
                             archive->old_path, LFS_O_RDONLY,
                             &archive->old_file_config);
    if (error != LFS_ERR_OK)
        return lfs_result(error, BC_REC_OPEN_ERROR);
    archive->old_file_open = true;
    seek_result = lfs_file_seek(archive->lfs, &archive->old_file,
                                (lfs_soff_t)archive->requested_offset,
                                LFS_SEEK_SET);
    if (seek_result != (lfs_soff_t)archive->requested_offset) {
        error = seek_result < 0 ? (int)seek_result : LFS_ERR_CORRUPT;
        (void)lfs_file_close(archive->lfs, &archive->old_file);
        archive->old_file_open = false;
        return lfs_result(error, BC_REC_OPEN_ERROR);
    }
    archive->source = BC_VOICE_LEGACY_SOURCE_OLD;
    archive->file_size = (uint32_t)archive->old_info.size;
    archive->transfer_offset = archive->requested_offset;
    archive->transfer_sequence = 1U;
    archive->phase = BC_VOICE_LEGACY_UPLOAD_READ;
    return BC_REC_OK;
}

static bc_rec_result begin_new_reader(bc_voice_legacy_archive *archive)
{
    bc_rec_store_status status;

    status = bc_rec_store_reader_begin(archive->store, archive->new_id,
                                       &archive->new_reader,
                                       &archive->new_start,
                                       &archive->new_file);
    if (status != BC_REC_STORE_OK)
        return result_from_store(status);
    if (archive->new_file.delivered || archive->new_file.bytes == 0U ||
        (!archive->new_file.complete && !archive->new_file.recovered) ||
        archive->requested_offset >= archive->new_file.bytes ||
        archive->new_file.bytes > (uint32_t)INT32_MAX) {
        (void)bc_rec_store_reader_close(&archive->new_reader);
        return archive->new_file.bytes == 0U ? BC_REC_EMPTY_AUDIO :
               BC_REC_INVALID;
    }
    archive->source = BC_VOICE_LEGACY_SOURCE_NEW;
    archive->file_size = archive->new_file.bytes;
    archive->transfer_offset = archive->requested_offset;
    archive->transfer_sequence = 1U;
    archive->phase = BC_VOICE_LEGACY_UPLOAD_NEW_VERIFY;
    return BC_REC_OK;
}

static bc_rec_result close_upload_source(bc_voice_legacy_archive *archive)
{
    bc_rec_result result = BC_REC_OK;
    int error;
    bc_rec_store_status store_status;

    if (archive->source == BC_VOICE_LEGACY_SOURCE_OLD &&
        archive->old_file_open) {
        error = lfs_file_close(archive->lfs, &archive->old_file);
        archive->old_file_open = false;
        if (error != LFS_ERR_OK)
            result = lfs_result(error, BC_REC_CLOSE_ERROR);
    } else if (archive->source == BC_VOICE_LEGACY_SOURCE_NEW &&
               archive->new_reader.open) {
        store_status = bc_rec_store_reader_close(&archive->new_reader);
        if (store_status != BC_REC_STORE_OK)
            result = result_from_store(store_status);
    }
    archive->source = BC_VOICE_LEGACY_SOURCE_NONE;
    return result;
}

static bc_rec_result prepare_upload_packet(bc_voice_legacy_archive *archive)
{
    uint32_t remaining;
    uint32_t chunk_count;
    lfs_ssize_t old_read;
    int32_t new_read;
    uint8_t *payload;

    if (archive->file_size == 0U || archive->transfer_offset >=
        archive->file_size || archive->transfer_sequence == 0U)
        return BC_REC_INVALID;
    remaining = archive->file_size - archive->transfer_offset;
    chunk_count = remaining / BC_VOICE_LEGACY_CHUNK_SIZE;
    if (remaining % BC_VOICE_LEGACY_CHUNK_SIZE != 0U)
        ++chunk_count;
    archive->transfer_chunk = (uint16_t)(remaining <
                                         BC_VOICE_LEGACY_CHUNK_SIZE
                                         ? remaining
                                         : BC_VOICE_LEGACY_CHUNK_SIZE);
    payload = archive->tx_packet + BC_VOICE_LEGACY_HEADER_SIZE;
    memcpy(archive->tx_packet, archive->request_header,
           BC_VOICE_LEGACY_HEADER_SIZE);
    payload[0] = 1U;
    put_le32(payload + 1U, remaining);
    put_le32(payload + 5U, chunk_count);
    put_le32(payload + 9U, archive->transfer_sequence);
    put_le32(payload + 13U, archive->transfer_chunk);
    if (archive->source == BC_VOICE_LEGACY_SOURCE_OLD) {
        old_read = lfs_file_read(archive->lfs, &archive->old_file,
                                 payload + BC_VOICE_LEGACY_UPLOAD_HEADER_SIZE,
                                 archive->transfer_chunk);
        if (old_read != (lfs_ssize_t)archive->transfer_chunk)
            return old_read < 0 ? lfs_result((int)old_read,
                                             BC_REC_OPEN_ERROR)
                                : BC_REC_OPEN_ERROR;
    } else if (archive->source == BC_VOICE_LEGACY_SOURCE_NEW) {
        new_read = bc_rec_store_reader_read(
            &archive->new_reader, archive->transfer_offset,
            payload + BC_VOICE_LEGACY_UPLOAD_HEADER_SIZE,
            archive->transfer_chunk);
        if (new_read != (int32_t)archive->transfer_chunk)
            return new_read < 0 ? result_from_store((bc_rec_store_status)new_read)
                                : BC_REC_OPEN_ERROR;
    } else {
        return BC_REC_INVALID;
    }
    archive->tx_length = (uint16_t)(BC_VOICE_LEGACY_HEADER_SIZE +
                                    BC_VOICE_LEGACY_UPLOAD_HEADER_SIZE +
                                    archive->transfer_chunk);
    archive->tx_pending = true;
    archive->phase = BC_VOICE_LEGACY_UPLOAD_SEND;
    return BC_REC_OK;
}

static bc_rec_result poll_send(bc_voice_legacy_archive *archive)
{
    if (!archive->tx_pending)
        return BC_REC_INVALID;
    if (!archive->send(archive->send_ctx, archive->tx_packet,
                       archive->tx_length, archive->epoch))
        return BC_REC_BUSY;
    archive->tx_pending = false;
    if (archive->phase == BC_VOICE_LEGACY_UPLOAD_SEND) {
        archive->transfer_offset += archive->transfer_chunk;
        ++archive->transfer_sequence;
        archive->phase = archive->transfer_offset == archive->file_size
                       ? BC_VOICE_LEGACY_UPLOAD_CLOSE
                       : BC_VOICE_LEGACY_UPLOAD_READ;
    } else {
        if (archive->after_send == BC_VOICE_LEGACY_LIST_EMIT_OLD ||
            archive->after_send == BC_VOICE_LEGACY_LIST_EMIT_NEW)
            ++archive->list_emitted;
        archive->phase = archive->after_send;
    }
    archive->after_send = BC_VOICE_LEGACY_IDLE;
    return BC_REC_OK;
}

static bc_rec_result poll_list_count_old(bc_voice_legacy_archive *archive)
{
    int error;
    uint8_t name[BC_VOICE_LEGACY_NAME_SIZE];

    error = lfs_dir_read(archive->lfs, &archive->old_dir,
                         &archive->old_info);
    if (error < 0)
        return lfs_result(error, BC_REC_OPEN_ERROR);
    if (error == 0) {
        bc_rec_result result = close_old_directory(archive);
        if (result != BC_REC_OK)
            return result;
        archive->phase = BC_VOICE_LEGACY_LIST_COUNT_NEW_BEGIN;
    } else if (list_old_info_eligible(&archive->old_info, name) &&
               !count_one(archive)) {
        return BC_REC_NO_SPACE;
    }
    return BC_REC_OK;
}

static bc_rec_result poll_list_count_new(bc_voice_legacy_archive *archive)
{
    bc_rec_store_status status;
    bc_rec_start start;
    bc_rec_file file;
    bool verified;
    uint8_t name[BC_VOICE_LEGACY_NAME_SIZE];

    status = bc_rec_store_catalog_next(archive->store, &archive->new_cursor,
                                       &start, &file, &verified);
    if (status == BC_REC_STORE_END) {
        status = bc_rec_store_list_end(archive->store, &archive->new_cursor);
        archive->new_cursor_open = false;
        if (status != BC_REC_STORE_OK)
            return result_from_store(status);
        archive->phase = archive->list_total == 0U
                       ? BC_VOICE_LEGACY_LIST_BUILD_EMPTY
                       : BC_VOICE_LEGACY_LIST_EMIT_OLD_BEGIN;
    } else if (status != BC_REC_STORE_OK) {
        return result_from_store(status);
    } else if (list_new_file_eligible(&file, name) && !count_one(archive)) {
        return BC_REC_NO_SPACE;
    }
    return BC_REC_OK;
}

static bc_rec_result poll_list_emit_old(bc_voice_legacy_archive *archive)
{
    int error;
    uint8_t name[BC_VOICE_LEGACY_NAME_SIZE];

    error = lfs_dir_read(archive->lfs, &archive->old_dir,
                         &archive->old_info);
    if (error < 0)
        return lfs_result(error, BC_REC_OPEN_ERROR);
    if (error == 0) {
        bc_rec_result result = close_old_directory(archive);
        if (result != BC_REC_OK)
            return result;
        archive->phase = BC_VOICE_LEGACY_LIST_EMIT_NEW_BEGIN;
    } else if (list_old_info_eligible(&archive->old_info, name)) {
        if (archive->list_emitted >= archive->list_total)
            return BC_REC_CRC_ERROR;
        prepare_list_packet(archive, name, (uint32_t)archive->old_info.size);
    }
    return BC_REC_OK;
}

static bc_rec_result poll_list_emit_new(bc_voice_legacy_archive *archive)
{
    bc_rec_store_status status;
    bc_rec_start start;
    bc_rec_file file;
    bool verified;
    uint8_t name[BC_VOICE_LEGACY_NAME_SIZE];

    status = bc_rec_store_catalog_next(archive->store, &archive->new_cursor,
                                       &start, &file, &verified);
    if (status == BC_REC_STORE_END) {
        status = bc_rec_store_list_end(archive->store, &archive->new_cursor);
        archive->new_cursor_open = false;
        if (status != BC_REC_STORE_OK)
            return result_from_store(status);
        if (archive->list_emitted != archive->list_total)
            return BC_REC_CRC_ERROR;
        archive->phase = BC_VOICE_LEGACY_IDLE;
    } else if (status != BC_REC_STORE_OK) {
        return result_from_store(status);
    } else if (list_new_file_eligible(&file, name)) {
        if (archive->list_emitted >= archive->list_total)
            return BC_REC_CRC_ERROR;
        prepare_list_packet(archive, name, file.bytes);
    }
    return BC_REC_OK;
}

static bc_rec_result poll_upload_find_old(bc_voice_legacy_archive *archive)
{
    int error;
    uint8_t name[BC_VOICE_LEGACY_NAME_SIZE];

    error = lfs_dir_read(archive->lfs, &archive->old_dir,
                         &archive->old_info);
    if (error < 0)
        return lfs_result(error, BC_REC_OPEN_ERROR);
    if (error == 0) {
        bc_rec_result result = close_old_directory(archive);
        if (result != BC_REC_OK)
            return result;
        archive->phase = BC_VOICE_LEGACY_UPLOAD_FIND_NEW_BEGIN;
    } else if (info_name_valid(&archive->old_info, name) &&
               same_name(name, archive->requested_name)) {
        if (archive->old_info.size == 0U)
            return BC_REC_EMPTY_AUDIO;
        if ((uint64_t)archive->old_info.size > UINT32_MAX)
            return BC_REC_INVALID;
        if ((uint64_t)archive->old_info.size > (uint64_t)INT32_MAX ||
            archive->requested_offset >= (uint32_t)archive->old_info.size)
            return BC_REC_INVALID;
        memcpy(archive->requested_name, name,
               BC_VOICE_LEGACY_NAME_SIZE);
        {
            bc_rec_result close_result = close_old_directory(archive);
            if (close_result != BC_REC_OK)
                return close_result;
        }
        archive->phase = BC_VOICE_LEGACY_UPLOAD_OLD_OPEN;
    }
    return BC_REC_OK;
}

static bc_rec_result poll_upload_find_new(bc_voice_legacy_archive *archive)
{
    bc_rec_store_status status;
    bc_rec_start start;
    bc_rec_file file;
    bool verified;
    uint8_t name[BC_VOICE_LEGACY_NAME_SIZE];

    status = bc_rec_store_catalog_next(archive->store, &archive->new_cursor,
                                       &start, &file, &verified);
    if (status == BC_REC_STORE_END) {
        status = bc_rec_store_list_end(archive->store, &archive->new_cursor);
        archive->new_cursor_open = false;
        if (status != BC_REC_STORE_OK)
            return result_from_store(status);
        return BC_REC_NOT_FOUND;
    }
    if (status != BC_REC_STORE_OK)
        return result_from_store(status);
    if (store_file_name_valid(&file, name) &&
        same_name(name, archive->requested_name) && !file.delivered &&
        (file.complete || file.recovered)) {
        if (file.bytes == 0U)
            return BC_REC_EMPTY_AUDIO;
        if (archive->requested_offset >= file.bytes)
            return BC_REC_INVALID;
        archive->new_id = start.id;
        archive->new_start = start;
        archive->new_file = file;
        status = bc_rec_store_list_end(archive->store, &archive->new_cursor);
        archive->new_cursor_open = false;
        if (status != BC_REC_STORE_OK)
            return result_from_store(status);
        archive->phase = BC_VOICE_LEGACY_UPLOAD_NEW_BEGIN;
    }
    return BC_REC_OK;
}

bool bc_voice_legacy_archive_init(
    bc_voice_legacy_archive *archive, lfs_t *lfs, bc_rec_store *store,
    bc_voice_legacy_archive_send send, void *send_ctx)
{
    if (archive == NULL || lfs == NULL || lfs->cfg == NULL || send == NULL ||
        lfs->cfg->cache_size == 0U ||
        lfs->cfg->cache_size > BC_VOICE_LEGACY_FILE_CACHE_SIZE ||
        (store != NULL && store->lfs != lfs))
        return false;
    memset(archive, 0, sizeof(*archive));
    archive->lfs = lfs;
    archive->store = store;
    archive->send = send;
    archive->send_ctx = send_ctx;
    archive->initialized = true;
    archive->phase = BC_VOICE_LEGACY_IDLE;
    return true;
}

bc_rec_result bc_voice_legacy_archive_request(
    bc_voice_legacy_archive *archive, const uint8_t *packet,
    uint16_t length, uint32_t epoch, uint16_t att_limit)
{
    uint8_t subcommand;
    const uint8_t *name;

    if (!archive_valid(archive) || packet == NULL ||
        length < BC_VOICE_LEGACY_HEADER_SIZE || packet[2] !=
        BC_VOICE_LEGACY_COMMAND)
        return BC_REC_INVALID;
    subcommand = packet[3];
    if (subcommand == BC_VOICE_LEGACY_SUB_CANCEL) {
        if (length != BC_VOICE_LEGACY_HEADER_SIZE)
            return BC_REC_INVALID;
        return bc_voice_legacy_archive_cancel(archive);
    }
    if (phase_active(archive->phase))
        return BC_REC_BUSY;

    switch (subcommand) {
    case BC_VOICE_LEGACY_SUB_LIST:
        if (length != BC_VOICE_LEGACY_HEADER_SIZE ||
            att_limit < BC_VOICE_LEGACY_LIST_ATT_MIN)
            return BC_REC_INVALID;
        break;
    case BC_VOICE_LEGACY_SUB_UPLOAD:
        if (length != BC_VOICE_LEGACY_HEADER_SIZE +
                         BC_VOICE_LEGACY_NAME_SIZE ||
            att_limit < BC_VOICE_LEGACY_UPLOAD_ATT_MIN)
            return BC_REC_INVALID;
        name = packet + BC_VOICE_LEGACY_HEADER_SIZE;
        if (!name_bytes_valid(name))
            return BC_REC_INVALID;
        break;
    case BC_VOICE_LEGACY_SUB_RESUME:
        if (length != BC_VOICE_LEGACY_HEADER_SIZE + 4U +
                         BC_VOICE_LEGACY_NAME_SIZE ||
            att_limit < BC_VOICE_LEGACY_UPLOAD_ATT_MIN)
            return BC_REC_INVALID;
        name = packet + BC_VOICE_LEGACY_HEADER_SIZE + 4U;
        if (!name_bytes_valid(name))
            return BC_REC_INVALID;
        break;
    case BC_VOICE_LEGACY_SUB_SPACE:
        if (length != BC_VOICE_LEGACY_HEADER_SIZE || att_limit < 16U)
            return BC_REC_INVALID;
        break;
    case BC_VOICE_LEGACY_SUB_DELETE:
        if (length != BC_VOICE_LEGACY_HEADER_SIZE +
                         BC_VOICE_LEGACY_NAME_SIZE ||
            !name_bytes_valid(packet + BC_VOICE_LEGACY_HEADER_SIZE))
            return BC_REC_INVALID;
        break;
    case BC_VOICE_LEGACY_SUB_FORMAT:
        if (length != BC_VOICE_LEGACY_HEADER_SIZE)
            return BC_REC_INVALID;
        break;
    case BC_VOICE_LEGACY_SUB_BATCH:
        /* The supplier one-click request normally includes a file index.
         * It is intentionally unsupported, but must still receive the
         * explicit false response for any safely bounded payload. */
        break;
    default:
        memcpy(archive->request_header, packet, sizeof(archive->request_header));
        archive->epoch = epoch;
        prepare_status_packet(archive, 0U);
        return BC_REC_UNSUPPORTED;
    }

    memcpy(archive->request_header, packet, sizeof(archive->request_header));
    archive->epoch = epoch;
    archive->att_limit = att_limit;
    if (subcommand == BC_VOICE_LEGACY_SUB_LIST) {
        archive->phase = BC_VOICE_LEGACY_LIST_COUNT_OLD_BEGIN;
        return BC_REC_OK;
    }
    if (subcommand == BC_VOICE_LEGACY_SUB_SPACE) {
        archive->phase = BC_VOICE_LEGACY_LIST_BUILD_EMPTY;
        return BC_REC_OK;
    }
    if (subcommand == BC_VOICE_LEGACY_SUB_DELETE ||
        subcommand == BC_VOICE_LEGACY_SUB_FORMAT ||
        subcommand == BC_VOICE_LEGACY_SUB_BATCH) {
        prepare_status_packet(archive, 0U);
        archive->epoch = epoch;
        return BC_REC_UNSUPPORTED;
    }

    if (subcommand == BC_VOICE_LEGACY_SUB_RESUME) {
        archive->requested_offset = get_le32(packet +
                                              BC_VOICE_LEGACY_HEADER_SIZE);
        memcpy(archive->requested_name,
               packet + BC_VOICE_LEGACY_HEADER_SIZE + 4U,
               BC_VOICE_LEGACY_NAME_SIZE);
    } else {
        archive->requested_offset = 0U;
        memcpy(archive->requested_name,
               packet + BC_VOICE_LEGACY_HEADER_SIZE,
               BC_VOICE_LEGACY_NAME_SIZE);
    }
    archive->phase = BC_VOICE_LEGACY_UPLOAD_FIND_OLD_BEGIN;
    return BC_REC_OK;
}

bc_rec_result bc_voice_legacy_archive_poll(bc_voice_legacy_archive *archive)
{
    bc_rec_store_status status;
    bc_rec_result result;

    if (!archive_valid(archive))
        return BC_REC_INVALID;
    switch (archive->phase) {
    case BC_VOICE_LEGACY_IDLE:
        return BC_REC_OK;
    case BC_VOICE_LEGACY_SEND_ONLY:
    case BC_VOICE_LEGACY_UPLOAD_SEND:
        return poll_send(archive);
    case BC_VOICE_LEGACY_LIST_COUNT_OLD_BEGIN:
        result = open_old_directory(archive);
        if (result == BC_REC_OK)
            archive->phase = BC_VOICE_LEGACY_LIST_COUNT_OLD;
        break;
    case BC_VOICE_LEGACY_LIST_COUNT_OLD:
        result = poll_list_count_old(archive);
        break;
    case BC_VOICE_LEGACY_LIST_COUNT_NEW_BEGIN:
        if (archive->store == NULL) {
            archive->phase = archive->list_total == 0U
                           ? BC_VOICE_LEGACY_LIST_BUILD_EMPTY
                           : BC_VOICE_LEGACY_LIST_EMIT_OLD_BEGIN;
            result = BC_REC_OK;
        } else {
            result = open_new_catalog(archive);
            if (result == BC_REC_OK)
                archive->phase = BC_VOICE_LEGACY_LIST_COUNT_NEW;
            else if (result == BC_REC_NOT_FOUND) {
                archive->phase = archive->list_total == 0U
                               ? BC_VOICE_LEGACY_LIST_BUILD_EMPTY
                               : BC_VOICE_LEGACY_LIST_EMIT_OLD_BEGIN;
                result = BC_REC_OK;
            }
        }
        break;
    case BC_VOICE_LEGACY_LIST_COUNT_NEW:
        result = poll_list_count_new(archive);
        break;
    case BC_VOICE_LEGACY_LIST_EMIT_OLD_BEGIN:
        result = open_old_directory(archive);
        if (result == BC_REC_OK)
            archive->phase = BC_VOICE_LEGACY_LIST_EMIT_OLD;
        break;
    case BC_VOICE_LEGACY_LIST_EMIT_OLD:
        result = poll_list_emit_old(archive);
        break;
    case BC_VOICE_LEGACY_LIST_EMIT_NEW_BEGIN:
        if (archive->store == NULL) {
            if (archive->list_emitted != archive->list_total)
                result = BC_REC_CRC_ERROR;
            else
                result = BC_REC_OK;
            archive->phase = BC_VOICE_LEGACY_IDLE;
        } else {
            result = open_new_catalog(archive);
            if (result == BC_REC_OK)
                archive->phase = BC_VOICE_LEGACY_LIST_EMIT_NEW;
            else if (result == BC_REC_NOT_FOUND &&
                     archive->list_emitted == archive->list_total) {
                archive->phase = BC_VOICE_LEGACY_IDLE;
                result = BC_REC_OK;
            }
        }
        break;
    case BC_VOICE_LEGACY_LIST_EMIT_NEW:
        result = poll_list_emit_new(archive);
        break;
    case BC_VOICE_LEGACY_LIST_BUILD_EMPTY:
        if (archive->request_header[2] == BC_VOICE_LEGACY_COMMAND &&
            archive->request_header[3] == BC_VOICE_LEGACY_SUB_SPACE)
            result = build_space_packet(archive);
        else {
            prepare_empty_list_packet(archive);
            result = BC_REC_OK;
        }
        break;
    case BC_VOICE_LEGACY_UPLOAD_FIND_OLD_BEGIN:
        result = open_old_directory(archive);
        if (result == BC_REC_OK)
            archive->phase = BC_VOICE_LEGACY_UPLOAD_FIND_OLD;
        break;
    case BC_VOICE_LEGACY_UPLOAD_FIND_OLD:
        result = poll_upload_find_old(archive);
        break;
    case BC_VOICE_LEGACY_UPLOAD_OLD_OPEN:
        result = open_old_file(archive);
        break;
    case BC_VOICE_LEGACY_UPLOAD_FIND_NEW_BEGIN:
        result = open_new_catalog(archive);
        if (result == BC_REC_OK)
            archive->phase = BC_VOICE_LEGACY_UPLOAD_FIND_NEW;
        break;
    case BC_VOICE_LEGACY_UPLOAD_FIND_NEW:
        result = poll_upload_find_new(archive);
        break;
    case BC_VOICE_LEGACY_UPLOAD_NEW_BEGIN:
        result = begin_new_reader(archive);
        break;
    case BC_VOICE_LEGACY_UPLOAD_NEW_VERIFY:
        status = bc_rec_store_reader_verify_step(&archive->new_reader,
                                                 BC_VOICE_LEGACY_VERIFY_BUDGET);
        if (status == BC_REC_STORE_OK) {
            archive->phase = BC_VOICE_LEGACY_UPLOAD_READ;
            result = BC_REC_OK;
        } else if (status == BC_REC_STORE_MORE) {
            result = BC_REC_OK;
        } else {
            result = result_from_store(status);
        }
        break;
    case BC_VOICE_LEGACY_UPLOAD_READ:
        result = prepare_upload_packet(archive);
        break;
    case BC_VOICE_LEGACY_UPLOAD_CLOSE:
        result = close_upload_source(archive);
        if (result == BC_REC_OK)
            reset_operation(archive);
        break;
    default:
        result = BC_REC_INVALID;
        break;
    }
    if (result != BC_REC_OK && result != BC_REC_BUSY)
        return fail_operation(archive, result);
    return result;
}

bc_rec_result bc_voice_legacy_archive_cancel(bc_voice_legacy_archive *archive)
{
    bc_rec_result result;

    if (!archive_valid(archive))
        return BC_REC_INVALID;
    result = close_resources(archive);
    reset_operation(archive);
    return result;
}

bool bc_voice_legacy_archive_active(const bc_voice_legacy_archive *archive)
{
    return archive_valid(archive) && phase_active(archive->phase);
}
