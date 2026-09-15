#ifndef APP_FACTORY_DELETE_P08_H
#define APP_FACTORY_DELETE_P08_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "lfs.h"

#define FACTORY_DELETE_NAME_MAX 58U
#define FACTORY_DELETE_PATH_SIZE (FACTORY_DELETE_NAME_MAX + 2U)

typedef enum {
    FACTORY_DELETE_REMOVED = 0,
    FACTORY_DELETE_ALREADY_ABSENT,
    FACTORY_DELETE_INVALID,
    FACTORY_DELETE_BUSY,
    FACTORY_DELETE_NOT_REGULAR,
    FACTORY_DELETE_STORAGE_ERROR,
    FACTORY_DELETE_NOT_CONFIRMED
} factory_delete_result;

/* Caller exclusively owns this workspace and the mounted filesystem.
 * Kept off the factory command task's small stack. No heap or new task. */
typedef struct {
    char path[FACTORY_DELETE_PATH_SIZE];
    struct lfs_info info;
    int storage_error;
} factory_delete_workspace;

/* A wire name may omit NUL or end in NUL padding. Only a root basename is
 * accepted, never a path, a hidden entry, or a name with trailing garbage. */
bool factory_delete_parse_name(const uint8_t *data, size_t length,
                               factory_delete_workspace *workspace);
factory_delete_result factory_delete_remove(lfs_t *lfs,
                                             factory_delete_workspace *workspace);

/* Platform boundary: reserves idle factory storage before touching workspace
 * or flash, and preserves the legacy one-byte 1=success / 0=failure reply. */
bool app_ppg_file_delete_request(const uint8_t *data, unsigned length);

#endif
