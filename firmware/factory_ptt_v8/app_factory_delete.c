#include "app_factory_delete.h"

static bool name_byte(uint8_t value)
{
    return (value >= '0' && value <= '9') ||
           (value >= 'A' && value <= 'Z') ||
           (value >= 'a' && value <= 'z') ||
           value == '_' || value == '-' || value == ':' || value == '.';
}

bool factory_delete_parse_name(const uint8_t *data, size_t length,
                               factory_delete_workspace *workspace)
{
    size_t i, name_length = 0;
    if (!workspace) return false;
    workspace->path[0] = '\0';
    workspace->storage_error = 0;
    /* The legacy command has at most 250 payload bytes. Validate before
     * reading even the first byte; a terminator never authorizes trailing data. */
    if (!data || length == 0U || length > 250U || data[0] == '.') return false;
    while (name_length < length && data[name_length] != 0U) {
        if (name_length >= FACTORY_DELETE_NAME_MAX || !name_byte(data[name_length]))
            return false;
        ++name_length;
    }
    if (name_length == 0U) return false;
    for (i = name_length; i < length; ++i)
        if (data[i] != 0U) return false;
    workspace->path[0] = '/';
    for (i = 0; i < name_length; ++i) workspace->path[i + 1U] = (char)data[i];
    workspace->path[name_length + 1U] = '\0';
    return true;
}

factory_delete_result factory_delete_remove(lfs_t *lfs,
                                             factory_delete_workspace *workspace)
{
    int error;
    if (!lfs || !workspace || workspace->path[0] != '/' ||
        workspace->path[1] == '\0') return FACTORY_DELETE_INVALID;
    workspace->storage_error = 0;
    error = lfs_stat(lfs, workspace->path, &workspace->info);
    if (error == LFS_ERR_NOENT) return FACTORY_DELETE_ALREADY_ABSENT;
    if (error != LFS_ERR_OK) {
        workspace->storage_error = error;
        return FACTORY_DELETE_STORAGE_ERROR;
    }
    if (workspace->info.type != LFS_TYPE_REG) return FACTORY_DELETE_NOT_REGULAR;
    error = lfs_remove(lfs, workspace->path);
    if (error != LFS_ERR_OK) {
        workspace->storage_error = error;
        return FACTORY_DELETE_STORAGE_ERROR;
    }
    error = lfs_stat(lfs, workspace->path, &workspace->info);
    if (error == LFS_ERR_NOENT) return FACTORY_DELETE_REMOVED;
    workspace->storage_error = error;
    return error == LFS_ERR_OK ? FACTORY_DELETE_NOT_CONFIRMED :
                                FACTORY_DELETE_STORAGE_ERROR;
}
