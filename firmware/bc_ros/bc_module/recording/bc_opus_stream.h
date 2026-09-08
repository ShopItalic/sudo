#ifndef BC_OPUS_STREAM_H
#define BC_OPUS_STREAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bc_audio_format.h"
#include "bc_opus_profile.h"

/* Sudo Opus container v1. The stored raw file and the live preview stream
 * carry exactly these bytes, so one parser serves archive sync and live
 * decoding on the phone.
 *
 *   Header (16 bytes): "SOPU", version u8 = 1, codec u8 = 2 (Opus),
 *     sample rate u16, channels u8, frame ms u8, pre-skip u16, reserved u32.
 *   Record: length u16 (1..1275) followed by one RFC 6716 Opus packet.
 *   Trailer (8 bytes): length u16 = 0, kind u16 = 1, real sample count u32.
 *
 * A recording interrupted before its trailer is a valid prefix: the decoder
 * ignores a trailing partial record and treats the sample count as unknown.
 * All integers are little endian. */
#define BC_OPUS_STREAM_MAGIC "SOPU"
#define BC_OPUS_STREAM_VERSION 1U
#define BC_OPUS_STREAM_HEADER_SIZE 16U
#define BC_OPUS_STREAM_TRAILER_SIZE 8U
#define BC_OPUS_STREAM_TRAILER_END 1U
#define BC_OPUS_STREAM_RECORD_MAX (2U + BC_OPUS_PACKET_MAX)

/* The writer buffers whole units (header, records, trailer) and hands out
 * transport-sized chunks that end on a unit boundary whenever possible.
 * Only a unit larger than the chunk capacity is fragmented, and then only
 * across as many chunks as it needs. */
#define BC_OPUS_STREAM_WRITER_BYTES 1536U
#define BC_OPUS_STREAM_WRITER_UNITS 64U

typedef struct {
    uint8_t buffer[BC_OPUS_STREAM_WRITER_BYTES];
    uint16_t length;                              /* buffered bytes */
    uint16_t units[BC_OPUS_STREAM_WRITER_UNITS];  /* unit end offsets */
    uint8_t unit_read, unit_count;
    uint16_t head_partial;  /* bytes of the head unit already emitted */
    bool header_written, trailer_written;
    uint32_t bytes_written; /* total container bytes emitted as chunks */
    uint32_t records_written;
} bc_opus_stream_writer;

void bc_opus_stream_writer_init(bc_opus_stream_writer *w);
bool bc_opus_stream_write_header(bc_opus_stream_writer *w, const bc_audio_format *format);
bool bc_opus_stream_write_packet(bc_opus_stream_writer *w, const uint8_t *packet, size_t length);
bool bc_opus_stream_write_trailer(bc_opus_stream_writer *w, uint32_t sample_count);
/* Returns the next chunk of at most capacity bytes, or 0 when nothing is due.
 * With flush=false a chunk is due only when buffered units already exceed
 * capacity (or the head unit itself is oversized); flush=true drains
 * everything in boundary-aligned pieces. */
size_t bc_opus_stream_take_chunk(bc_opus_stream_writer *w, uint8_t *out, size_t capacity, bool flush);
bool bc_opus_stream_writer_empty(const bc_opus_stream_writer *w);
bool bc_opus_stream_discard_header_only(bc_opus_stream_writer *w);

/* Incremental parser fed with arbitrary byte chunks. It reconstructs the
 * header, each packet and the trailer, and reports the exact error class. */
typedef enum {
    BC_OPUS_PARSE_NEED_MORE = 0,
    BC_OPUS_PARSE_HEADER,
    BC_OPUS_PARSE_PACKET,
    BC_OPUS_PARSE_TRAILER,
    BC_OPUS_PARSE_ERROR_MAGIC,
    BC_OPUS_PARSE_ERROR_FORMAT,
    BC_OPUS_PARSE_ERROR_LENGTH,
    BC_OPUS_PARSE_ERROR_TRAILER,
    BC_OPUS_PARSE_ERROR_AFTER_END
} bc_opus_parse_event;

typedef struct {
    uint8_t pending[BC_OPUS_STREAM_RECORD_MAX];
    uint16_t pending_length;
    uint16_t record_length;   /* 0 while reading a length prefix */
    uint16_t packet_length;   /* length of the packet exposed by the last PACKET event */
    bool header_done, ended, failed;
    bc_audio_format format;
    uint32_t packets, packet_bytes;
    uint32_t sample_count;    /* from the trailer; 0 until ended */
    uint64_t consumed;
} bc_opus_stream_parser;

void bc_opus_stream_parser_init(bc_opus_stream_parser *p);
/* Consumes bytes until one event is produced; *used reports the bytes
 * accepted. Call again with the remainder until it returns NEED_MORE with
 * all bytes used. PACKET events expose the packet in p->pending until the
 * next parse call overwrites it. */
bc_opus_parse_event bc_opus_stream_parse(bc_opus_stream_parser *p, const uint8_t *bytes,
                                         size_t length, size_t *used);
const uint8_t *bc_opus_stream_parser_packet(const bc_opus_stream_parser *p, size_t *length);

/* Header helpers shared by the writer, parser and host fixtures. */
void bc_opus_stream_encode_header(uint8_t out[BC_OPUS_STREAM_HEADER_SIZE], const bc_audio_format *format);
bool bc_opus_stream_decode_header(const uint8_t in[BC_OPUS_STREAM_HEADER_SIZE], bc_audio_format *format);

#endif
