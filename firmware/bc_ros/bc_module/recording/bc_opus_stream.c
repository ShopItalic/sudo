#include "bc_opus_stream.h"

#include <string.h>

static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put32(uint8_t *p, uint32_t v)
{
    unsigned i;
    for (i = 0; i < 4U; ++i) p[i] = (uint8_t)(v >> (8U * i));
}
static uint16_t get16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

void bc_opus_stream_encode_header(uint8_t out[BC_OPUS_STREAM_HEADER_SIZE],
                                  const bc_audio_format *f)
{
    memset(out, 0, BC_OPUS_STREAM_HEADER_SIZE);
    memcpy(out, BC_OPUS_STREAM_MAGIC, 4U);
    out[4] = BC_OPUS_STREAM_VERSION;
    out[5] = BC_AUDIO_CODEC_OPUS;
    put16(out + 6, f->sample_rate_hz);
    out[8] = f->channels;
    out[9] = f->frame_ms;
    put16(out + 10, f->pre_skip);
}

bool bc_opus_stream_decode_header(const uint8_t in[BC_OPUS_STREAM_HEADER_SIZE],
                                  bc_audio_format *f)
{
    bc_audio_format value;
    if (in == NULL || f == NULL || memcmp(in, BC_OPUS_STREAM_MAGIC, 4U) != 0 ||
        in[4] != BC_OPUS_STREAM_VERSION || in[5] != BC_AUDIO_CODEC_OPUS ||
        get32(in + 12) != 0U)
        return false;
    memset(&value, 0, sizeof(value));
    value.codec = BC_AUDIO_CODEC_OPUS;
    value.container_version = BC_OPUS_STREAM_VERSION;
    value.sample_rate_hz = get16(in + 6);
    value.channels = in[8];
    value.frame_ms = in[9];
    value.pre_skip = get16(in + 10);
    value.block_bytes = 0U;
    value.block_samples = (uint16_t)((uint32_t)value.sample_rate_hz * value.frame_ms / 1000U);
    if (!bc_audio_format_valid(&value)) return false;
    *f = value;
    return true;
}

/* ---- writer ---- */

void bc_opus_stream_writer_init(bc_opus_stream_writer *w)
{
    if (w != NULL) memset(w, 0, sizeof(*w));
}

static bool push_unit(bc_opus_stream_writer *w, const uint8_t *bytes, size_t length)
{
    uint8_t slot;
    if (length == 0U || length > BC_OPUS_STREAM_WRITER_BYTES ||
        w->unit_count == BC_OPUS_STREAM_WRITER_UNITS ||
        (size_t)w->length + length > BC_OPUS_STREAM_WRITER_BYTES)
        return false;
    memcpy(w->buffer + w->length, bytes, length);
    w->length = (uint16_t)(w->length + length);
    slot = (uint8_t)((w->unit_read + w->unit_count) % BC_OPUS_STREAM_WRITER_UNITS);
    w->units[slot] = w->length;
    ++w->unit_count;
    return true;
}

bool bc_opus_stream_write_header(bc_opus_stream_writer *w, const bc_audio_format *format)
{
    uint8_t header[BC_OPUS_STREAM_HEADER_SIZE];
    if (w == NULL || format == NULL || w->header_written ||
        format->codec != BC_AUDIO_CODEC_OPUS || !bc_audio_format_valid(format))
        return false;
    bc_opus_stream_encode_header(header, format);
    if (!push_unit(w, header, sizeof(header))) return false;
    w->header_written = true;
    return true;
}

bool bc_opus_stream_write_packet(bc_opus_stream_writer *w, const uint8_t *packet, size_t length)
{
    uint8_t record[BC_OPUS_STREAM_RECORD_MAX];
    if (w == NULL || packet == NULL || !w->header_written || w->trailer_written ||
        length == 0U || length > BC_OPUS_PACKET_MAX)
        return false;
    put16(record, (uint16_t)length);
    memcpy(record + 2, packet, length);
    if (!push_unit(w, record, length + 2U)) return false;
    ++w->records_written;
    return true;
}

bool bc_opus_stream_write_trailer(bc_opus_stream_writer *w, uint32_t sample_count)
{
    uint8_t trailer[BC_OPUS_STREAM_TRAILER_SIZE];
    if (w == NULL || !w->header_written || w->trailer_written) return false;
    put16(trailer, 0U);
    put16(trailer + 2, BC_OPUS_STREAM_TRAILER_END);
    put32(trailer + 4, sample_count);
    if (!push_unit(w, trailer, sizeof(trailer))) return false;
    w->trailer_written = true;
    return true;
}

bool bc_opus_stream_writer_empty(const bc_opus_stream_writer *w)
{
    return w == NULL || w->length == 0U;
}

/* Drop a buffered header when no audio ever followed it, so an empty capture
 * reports empty audio instead of a header-only file. */
bool bc_opus_stream_discard_header_only(bc_opus_stream_writer *w)
{
    if (w == NULL || !w->header_written || w->records_written != 0U ||
        w->bytes_written != 0U || w->trailer_written)
        return false;
    bc_opus_stream_writer_init(w);
    return true;
}

static void consume(bc_opus_stream_writer *w, size_t count)
{
    /* Remove count bytes from the head of the buffer and drop whole units. */
    size_t remaining = count;
    while (remaining != 0U && w->unit_count != 0U) {
        uint16_t end = w->units[w->unit_read];
        if ((size_t)end <= remaining) {
            /* This unit is fully emitted. */
            remaining -= end;
            memmove(w->buffer, w->buffer + end, (size_t)w->length - end);
            w->length = (uint16_t)(w->length - end);
            {
                uint8_t i;
                for (i = 0; i < w->unit_count; ++i) {
                    uint8_t slot = (uint8_t)((w->unit_read + i) % BC_OPUS_STREAM_WRITER_UNITS);
                    w->units[slot] = (uint16_t)(w->units[slot] - end);
                }
            }
            w->unit_read = (uint8_t)((w->unit_read + 1U) % BC_OPUS_STREAM_WRITER_UNITS);
            --w->unit_count;
            w->head_partial = 0U;
        } else {
            /* A fragment of the head unit. */
            uint8_t i;
            memmove(w->buffer, w->buffer + remaining, (size_t)w->length - remaining);
            w->length = (uint16_t)(w->length - remaining);
            for (i = 0; i < w->unit_count; ++i) {
                uint8_t slot = (uint8_t)((w->unit_read + i) % BC_OPUS_STREAM_WRITER_UNITS);
                w->units[slot] = (uint16_t)(w->units[slot] - remaining);
            }
            w->head_partial = (uint16_t)(w->head_partial + remaining);
            remaining = 0U;
        }
    }
}

size_t bc_opus_stream_take_chunk(bc_opus_stream_writer *w, uint8_t *out, size_t capacity, bool flush)
{
    size_t boundary = 0U;
    size_t take;
    uint8_t i;
    if (w == NULL || out == NULL || capacity == 0U || w->length == 0U) return 0U;
    /* Largest unit boundary that fits the capacity. */
    for (i = 0; i < w->unit_count; ++i) {
        uint16_t end = w->units[(w->unit_read + i) % BC_OPUS_STREAM_WRITER_UNITS];
        if ((size_t)end > capacity) break;
        boundary = end;
    }
    if (boundary != 0U) {
        if (!flush && (size_t)w->length <= capacity) return 0U; /* wait to fill */
        if (!flush && boundary == (size_t)w->length) return 0U;
        take = boundary;
    } else {
        /* The head unit itself is larger than one chunk: fragment it. */
        take = capacity;
    }
    memcpy(out, w->buffer, take);
    consume(w, take);
    w->bytes_written += (uint32_t)take;
    return take;
}

/* ---- parser ---- */

void bc_opus_stream_parser_init(bc_opus_stream_parser *p)
{
    if (p != NULL) memset(p, 0, sizeof(*p));
}

const uint8_t *bc_opus_stream_parser_packet(const bc_opus_stream_parser *p, size_t *length)
{
    if (p == NULL || length == NULL) return NULL;
    *length = p->packet_length;
    return p->pending;
}

bc_opus_parse_event bc_opus_stream_parse(bc_opus_stream_parser *p, const uint8_t *bytes,
                                         size_t length, size_t *used)
{
    size_t offset = 0U;
    if (used != NULL) *used = 0U;
    if (p == NULL || (length != 0U && bytes == NULL)) return BC_OPUS_PARSE_ERROR_FORMAT;
    if (p->failed) return BC_OPUS_PARSE_ERROR_FORMAT;
    while (offset < length) {
        if (p->ended) {
            p->failed = true;
            if (used != NULL) *used = offset;
            return BC_OPUS_PARSE_ERROR_AFTER_END;
        }
        if (!p->header_done) {
            size_t need = BC_OPUS_STREAM_HEADER_SIZE - p->pending_length;
            size_t copy = length - offset < need ? length - offset : need;
            memcpy(p->pending + p->pending_length, bytes + offset, copy);
            p->pending_length = (uint16_t)(p->pending_length + copy);
            offset += copy;
            p->consumed += copy;
            if (p->pending_length < BC_OPUS_STREAM_HEADER_SIZE) continue;
            if (memcmp(p->pending, BC_OPUS_STREAM_MAGIC, 4U) != 0) {
                p->failed = true;
                if (used != NULL) *used = offset;
                return BC_OPUS_PARSE_ERROR_MAGIC;
            }
            if (!bc_opus_stream_decode_header(p->pending, &p->format)) {
                p->failed = true;
                if (used != NULL) *used = offset;
                return BC_OPUS_PARSE_ERROR_FORMAT;
            }
            p->header_done = true;
            p->pending_length = 0U;
            if (used != NULL) *used = offset;
            return BC_OPUS_PARSE_HEADER;
        }
        if (p->record_length == 0U) {
            /* Reading a two-byte length prefix. */
            p->pending[p->pending_length++] = bytes[offset++];
            ++p->consumed;
            if (p->pending_length < 2U) continue;
            {
                uint16_t value = get16(p->pending);
                p->pending_length = 0U;
                if (value == 0U) {
                    /* Trailer: kind u16 + sample count u32 follow. */
                    p->record_length = 0xffffU;
                    continue;
                }
                if (value > BC_OPUS_PACKET_MAX) {
                    p->failed = true;
                    if (used != NULL) *used = offset;
                    return BC_OPUS_PARSE_ERROR_LENGTH;
                }
                p->record_length = value;
            }
            continue;
        }
        if (p->record_length == 0xffffU) {
            size_t need = 6U - p->pending_length;
            size_t copy = length - offset < need ? length - offset : need;
            memcpy(p->pending + p->pending_length, bytes + offset, copy);
            p->pending_length = (uint16_t)(p->pending_length + copy);
            offset += copy;
            p->consumed += copy;
            if (p->pending_length < 6U) continue;
            if (get16(p->pending) != BC_OPUS_STREAM_TRAILER_END) {
                p->failed = true;
                if (used != NULL) *used = offset;
                return BC_OPUS_PARSE_ERROR_TRAILER;
            }
            p->sample_count = get32(p->pending + 2);
            p->ended = true;
            p->record_length = 0U;
            p->pending_length = 0U;
            if (used != NULL) *used = offset;
            return BC_OPUS_PARSE_TRAILER;
        }
        {
            size_t need = (size_t)p->record_length - p->pending_length;
            size_t copy = length - offset < need ? length - offset : need;
            memcpy(p->pending + p->pending_length, bytes + offset, copy);
            p->pending_length = (uint16_t)(p->pending_length + copy);
            offset += copy;
            p->consumed += copy;
            if (p->pending_length < p->record_length) continue;
            ++p->packets;
            p->packet_bytes += p->record_length;
            p->packet_length = p->record_length;
            p->record_length = 0U;
            p->pending_length = 0U;
            if (used != NULL) *used = offset;
            return BC_OPUS_PARSE_PACKET;
        }
    }
    if (used != NULL) *used = offset;
    return BC_OPUS_PARSE_NEED_MORE;
}
