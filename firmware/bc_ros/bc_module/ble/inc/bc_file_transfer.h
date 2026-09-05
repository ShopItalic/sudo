#ifndef BC_FILE_TRANSFER_H
#define BC_FILE_TRANSFER_H
#include <stdbool.h>
#include <stdint.h>

/* Legacy file-transfer payload: status + four LE32 fields + file bytes.
 * The application prepends its existing four-byte command header. */
#define BC_FILE_HEADER_SIZE 17U
#define BC_FILE_MAX_CHUNK 223U

typedef enum {
    BC_FILE_DONE, BC_FILE_CANCELLED, BC_FILE_INVALID, BC_FILE_READ_ERROR,
    BC_FILE_SEND_ERROR
} bc_file_result;

typedef struct {
    void *ctx;
    int32_t (*seek)(void *ctx, uint32_t offset);
    int32_t (*read)(void *ctx, uint8_t *data, uint32_t length);
    bool (*send)(void *ctx, const uint8_t *data, uint16_t length);
    bool (*current)(void *ctx);
} bc_file_port;

bc_file_result bc_file_transfer(const bc_file_port *port, uint32_t file_size,
                                uint32_t offset, uint16_t chunk_size);
#endif
