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
//extern const struct fal_flash_dev fml_device_onchip_flash ;
extern struct fal_flash_dev fml_device_onchip_flash ;
/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &fml_device_onchip_flash,                                             \
}

/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */


#if (defined(HANDWARE_4_4_1) || defined(HANDWARE_4_1_2)  ||defined(HANDWARE_4_1_3) || defined(HANDWARE_4_0_2) || defined(HANDWARE_4_1_1) || defined(HANDWARE_4_5_1)|| defined(HANDWARE_1_19_1))

#define FAL_PART_TABLE                                                                \
{                                                                                     \
	{FAL_PART_MAGIC_WORD,  "fdb_tsdb1",    "device_flash",   2*4096, 10*4096, 0},  \
}

#else   

#define FAL_PART_TABLE                                                                \
{                                                                                     \
	{FAL_PART_MAGIC_WORD,  "fdb_tsdb1",    "device_flash",   2*4096, 16*4096, 0},  \
}

#endif


#endif



#endif /* _FAL_CFG_H_ */
