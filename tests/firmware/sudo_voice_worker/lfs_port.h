#ifndef SUDO_VOICE_WORKER_LFS_PORT_H
#define SUDO_VOICE_WORKER_LFS_PORT_H

#include "lfs.h"

extern const struct lfs_config cfg;

int lfs_sfud_init(lfs_t *lfs);
int lfs_sfud_format(lfs_t *lfs);
uint32_t lfs_sfud_jedec_id(void);

#endif
