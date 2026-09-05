#ifndef _USER_FDS_H
#define _USER_FDS_H

#include <stdint.h>

//存储的用户数据和信息
#define USER_INFO_FILE  (0x1000)
#define USER_INFO_KEY   (0x0001)

#define RECORD_FILE     (0x0000)//不定
#define RECORD_KEY      (0x0002)

void user_fds_init(void);
uint8_t record_is_exist(uint32_t fid, uint32_t key);
void record_write(uint32_t fid, uint32_t key, void const *p_data, uint32_t len);
uint8_t record_read(uint32_t fid, uint32_t key, uint8_t *p_data, uint32_t *len);
void record_update(uint32_t fid, uint32_t key, void const *p_data, uint32_t len);
uint8_t record_delete(uint32_t fid, uint32_t key);
uint8_t record_delete_next(void);
void print_all_cmd(void);
void stat_cmd(void);
void gc_cmd(void);

#endif
