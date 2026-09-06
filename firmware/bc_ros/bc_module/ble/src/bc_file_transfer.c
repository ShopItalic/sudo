#include "bc_file_transfer.h"
#include <stddef.h>

static void put_le32(uint8_t *out, uint32_t value)
{
    unsigned int i;
    for (i = 0; i < 4; ++i)
        out[i] = (uint8_t)(value >> (8 * i));
}

bc_file_result bc_file_transfer(const bc_file_port *port, uint32_t file_size,
                                uint32_t offset, uint16_t chunk_size)
{
    uint8_t packet[BC_FILE_HEADER_SIZE + BC_FILE_MAX_CHUNK];
    uint32_t remaining, count, sequence = 1;
    if (!port || !port->seek || !port->read || !port->send || !port->current ||
        !chunk_size || chunk_size > BC_FILE_MAX_CHUNK || offset > file_size ||
        file_size > INT32_MAX)
        return BC_FILE_INVALID;
    if (!port->current(port->ctx))
        return BC_FILE_CANCELLED;
    if (port->seek(port->ctx, offset) != (int32_t)offset)
        return BC_FILE_READ_ERROR;
    remaining = file_size - offset;
    count = remaining / chunk_size + (remaining % chunk_size != 0);
    packet[0] = 1; /* Vendor PPG_FLS_UPLOAD status; applies to audio files too. */
    put_le32(packet + 1, remaining);
    put_le32(packet + 5, count);
    while (remaining) {
        uint32_t filled = 0;
        uint16_t length = remaining < chunk_size ? (uint16_t)remaining : chunk_size;
        if (!port->current(port->ctx))
            return BC_FILE_CANCELLED;
        while (filled < length) {
            int32_t read_count;
            if (!port->current(port->ctx))
                return BC_FILE_CANCELLED;
            read_count = port->read(port->ctx, packet + BC_FILE_HEADER_SIZE + filled,
                                    length - filled);
            if (read_count <= 0 || (uint32_t)read_count > length - filled)
                return BC_FILE_READ_ERROR;
            filled += (uint32_t)read_count;
        }
        put_le32(packet + 9, sequence);
        put_le32(packet + 13, length);
        if (!port->current(port->ctx))
            return BC_FILE_CANCELLED;
        if (!port->send(port->ctx, packet, BC_FILE_HEADER_SIZE + length))
            return port->current(port->ctx) ? BC_FILE_SEND_ERROR : BC_FILE_CANCELLED;
        remaining -= length;
        ++sequence;
    }
    return port->current(port->ctx) ? BC_FILE_DONE : BC_FILE_CANCELLED;
}
