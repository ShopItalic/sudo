#ifndef MOCK_FACTORY_FDS_H
#define MOCK_FACTORY_FDS_H
#include <stdint.h>
#include <stdbool.h>
#define NRF_SUCCESS 0U
#define FDS_ERR_NOT_FOUND 94U
#define FDS_ERR_NO_SPACE_IN_QUEUES 92U
#define FDS_ERR_BUSY 98U
typedef uint32_t ret_code_t;
typedef struct { uint32_t record_id; } fds_record_desc_t;
typedef struct { unsigned position; } fds_find_token_t;
typedef struct { uint16_t length_words; } fds_header_t;
typedef struct { const fds_header_t *p_header; const void *p_data; } fds_flash_record_t;
typedef struct { uint16_t file_id, key; struct { const void *p_data; uint32_t length_words; } data; } fds_record_t;
enum { FDS_EVT_INIT, FDS_EVT_WRITE, FDS_EVT_UPDATE };
typedef struct { unsigned id; uint32_t result; struct { uint32_t record_id; uint16_t file_id, record_key; } write; } fds_evt_t;
typedef void (*fds_cb_t)(const fds_evt_t *);
ret_code_t fds_register(fds_cb_t cb);
ret_code_t fds_init(void);
ret_code_t fds_record_find(uint16_t file, uint16_t key, fds_record_desc_t *desc, fds_find_token_t *token);
ret_code_t fds_record_open(fds_record_desc_t *desc, fds_flash_record_t *record);
ret_code_t fds_record_close(fds_record_desc_t *desc);
ret_code_t fds_record_write(fds_record_desc_t *desc, const fds_record_t *record);
ret_code_t fds_record_update(fds_record_desc_t *desc, const fds_record_t *record);
#endif
