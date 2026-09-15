#ifndef APP_FACTORY_CLEANUP_P10_H
#define APP_FACTORY_CLEANUP_P10_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "lfs.h"
#include "sha256.h"

/* The attribute belongs to the file object, not a reusable filename. A new
 * exclusive create has no inherited attribute. All multibyte fields are LE. */
#define FC_ATTR 0xd0U
#define FC_META_BYTES 64U
#define FC_ID_BYTES 16U
#define FC_DESCRIPTOR_BYTES 52U
#define FC_PATH_BYTES 60U
#define FC_HASH_SLICE 512U
#define FC_ATTEMPTS 5U
typedef enum {
    FC_READY=0, FC_WORKING=1, FC_BUSY=2, FC_INVALID=3, FC_STORAGE=4,
    FC_STALE=5, FC_MISMATCH=6, FC_PENDING=7, FC_ABSENT=8, FC_FAILED=9
} factory_cleanup_result;
typedef struct {
    lfs_t *fs;
    char path[FC_PATH_BYTES];
    uint8_t meta[FC_META_BYTES], check[FC_META_BYTES];
    uint8_t descriptor[FC_DESCRIPTOR_BYTES], buffer[FC_HASH_SLICE];
    uint8_t cache[256];
    lfs_file_t file;
    struct lfs_file_config file_cfg;
    lfs_dir_t dir;
    struct lfs_info info;
    sha256_context_t sha;
    uint32_t offset, wake_at, prepared_at;
    lfs_soff_t cursor;
    factory_cleanup_result result;
    int storage_error;
    unsigned mode; /* 0 scan, 1 prepare hash, 2 prepared, 3 cleanup hash */
    bool faulted;
} factory_cleanup;

void factory_cleanup_init(factory_cleanup *c, lfs_t *fs);
/* Caller serializes every invocation with all other filesystem operations.
 * No open handle survives an invocation, and no filesystem work runs in ISR. */
factory_cleanup_result factory_cleanup_prepare(factory_cleanup *c,
    const uint8_t *name, unsigned length, const uint8_t fresh_id[FC_ID_BYTES], uint32_t now);
factory_cleanup_result factory_cleanup_confirm(factory_cleanup *c,
    const uint8_t descriptor[FC_DESCRIPTOR_BYTES], bool explicit_retry, uint32_t now);
void factory_cleanup_step(factory_cleanup *c, uint32_t now);

/* Factory adapter, running on the existing BLE command worker. */
bool app_factory_cleanup_command(const uint8_t *packet, unsigned length);
void app_factory_cleanup_service(void);
void app_factory_cleanup_mount(lfs_t *fs);
#endif
