/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#define FAL_DEBUG 1
#define FAL_PART_HAS_TABLE_CFG

/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev fml_device_onchip_flash ;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &fml_device_onchip_flash,                                             \
}

/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
#define FAL_PART_TABLE                                                                \
{                                                                                     \
    {FAL_PART_MAGIC_WORD,  "fdb_kvdb1",    "device_flash",   0*4096, 10*4096, 0},  \
	{FAL_PART_MAGIC_WORD,  "fdb_tsdb1",    "device_flash",   10*4096, 10*4096, 0},  \
}
#endif /* FAL_PART_HAS_TABLE_CFG */

//#define FAL_PART_TABLE                                                                \
//{                                                                                     \
//    {FAL_PART_MAGIC_WORD,  "fdb_tsdb1",    "device_flash",   268*1024,  50*1024, 0},  \
//}
//#endif /* FAL_PART_HAS_TABLE_CFG */

#endif /* _FAL_CFG_H_ */
