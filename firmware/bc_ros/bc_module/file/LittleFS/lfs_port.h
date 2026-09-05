#ifndef __LFS_PORT_H__
#define __LFS_PORT_H__




#include "lfs.h"

extern const struct lfs_config cfg;

int lfs_sfud_init(lfs_t *lfs);

int lfs_sfud_format(lfs_t *lfs);

#if defined(SUDO_VOICE_ONLY)
uint32_t lfs_sfud_jedec_id(void);
#endif

#endif


