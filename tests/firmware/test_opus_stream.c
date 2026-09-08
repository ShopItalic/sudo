#include "bc_opus_stream.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, failures;
static void check_condition(bool c, const char *e, unsigned line)
{
    ++checks;
    if (!c) { ++failures; fprintf(stderr, "FAIL line %u: %s\n", line, e); }
}
#define CHECK(c) check_condition((c), #c, __LINE__)

static void opus_format(bc_audio_format *f)
{
    memset(f, 0, sizeof(*f));
    f->codec = BC_AUDIO_CODEC_OPUS;
    f->container_version = 1U;
    f->sample_rate_hz = 16000U;
    f->channels = 1U;
    f->frame_ms = 20U;
    f->pre_skip = 104U;
    f->block_samples = 320U;
}

static void test_descriptor_wire_round_trip(void)
{
    bc_audio_format f, g, legacy;
    uint8_t wire[BC_AUDIO_FORMAT_WIRE_SIZE];
    opus_format(&f);
    f.sample_count = 0x01020304U;
    CHECK(bc_audio_format_valid(&f));
    bc_audio_format_encode(wire, &f);
    CHECK(wire[0] == 2U && wire[1] == 1U && wire[2] == 0x80U && wire[3] == 0x3eU);
    CHECK(wire[6] == 104U && wire[7] == 0U && wire[8] == 4U && wire[11] == 1U);
    CHECK(wire[14] == 0x40U && wire[15] == 0x01U);
    CHECK(bc_audio_format_decode(wire, &g) && bc_audio_format_equal(&f, &g));
    bc_audio_format_legacy_adpcm(&legacy);
    CHECK(bc_audio_format_valid(&legacy) && legacy.block_bytes == 220U && legacy.block_samples == 440U);
    bc_audio_format_encode(wire, &legacy);
    CHECK(bc_audio_format_decode(wire, &g) && bc_audio_format_equal(&legacy, &g));
    /* Zero descriptor is the idle sentinel; any other unknown codec fails. */
    memset(wire, 0, sizeof(wire));
    CHECK(bc_audio_format_decode(wire, &g) && g.codec == BC_AUDIO_CODEC_NONE);
    wire[0] = 3U;
    CHECK(!bc_audio_format_decode(wire, &g));
    /* Inconsistent Opus fields are rejected. */
    opus_format(&f); f.block_samples = 321U; CHECK(!bc_audio_format_valid(&f));
    opus_format(&f); f.pre_skip = 320U; CHECK(!bc_audio_format_valid(&f));
    opus_format(&f); f.sample_rate_hz = 44100U; CHECK(!bc_audio_format_valid(&f));
    opus_format(&f); f.channels = 2U; CHECK(!bc_audio_format_valid(&f));
    opus_format(&f); f.block_bytes = 220U; CHECK(!bc_audio_format_valid(&f));
    legacy.sample_count = 1U; CHECK(!bc_audio_format_valid(&legacy));
}

static void test_header_round_trip(void)
{
    bc_audio_format f, g;
    uint8_t header[BC_OPUS_STREAM_HEADER_SIZE];
    opus_format(&f);
    bc_opus_stream_encode_header(header, &f);
    CHECK(memcmp(header, "SOPU", 4) == 0 && header[4] == 1U && header[5] == 2U);
    CHECK(bc_opus_stream_decode_header(header, &g) && bc_audio_format_equal(&f, &g));
    header[12] = 1U; CHECK(!bc_opus_stream_decode_header(header, &g)); header[12] = 0U;
    header[4] = 2U; CHECK(!bc_opus_stream_decode_header(header, &g)); header[4] = 1U;
    header[0] = 'X'; CHECK(!bc_opus_stream_decode_header(header, &g));
}

/* Collects every chunk into one stream and validates it with the parser. */
typedef struct {
    uint8_t stream[65536];
    size_t length;
    size_t chunks;
    size_t max_chunk;
    size_t chunk_lengths[512];
} collector;

static void collect(collector *c, bc_opus_stream_writer *w, bool flush)
{
    uint8_t chunk[220];
    size_t n;
    while ((n = bc_opus_stream_take_chunk(w, chunk, sizeof(chunk), flush)) != 0U) {
        CHECK(n <= 220U);
        memcpy(c->stream + c->length, chunk, n);
        c->length += n;
        if (c->chunks < 512U) c->chunk_lengths[c->chunks] = n;
        ++c->chunks;
        if (n > c->max_chunk) c->max_chunk = n;
    }
}

static void test_writer_aligns_chunks_to_packets(void)
{
    bc_opus_stream_writer w;
    bc_audio_format f;
    collector c;
    uint8_t packet[30];
    unsigned i;
    memset(&c, 0, sizeof(c));
    memset(packet, 0x5a, sizeof(packet));
    opus_format(&f);
    bc_opus_stream_writer_init(&w);
    CHECK(!bc_opus_stream_write_packet(&w, packet, sizeof(packet))); /* header first */
    CHECK(bc_opus_stream_write_header(&w, &f));
    CHECK(!bc_opus_stream_write_header(&w, &f));
    /* Header alone waits: nothing is due until a chunk would overflow. */
    collect(&c, &w, false);
    CHECK(c.chunks == 0U);
    for (i = 0; i < 20U; ++i) {
        CHECK(bc_opus_stream_write_packet(&w, packet, sizeof(packet)));
        collect(&c, &w, false);
    }
    /* 16-byte header + 32-byte records: the first chunk holds header + 6
     * records (208 bytes) because a seventh would exceed 220. */
    CHECK(c.chunks >= 2U);
    CHECK(c.chunk_lengths[0] == 16U + 6U * 32U);
    CHECK(c.chunk_lengths[1] == 6U * 32U);
    CHECK(bc_opus_stream_write_trailer(&w, 6400U));
    CHECK(!bc_opus_stream_write_packet(&w, packet, sizeof(packet))); /* after trailer */
    collect(&c, &w, true);
    CHECK(bc_opus_stream_writer_empty(&w));
    CHECK(c.length == 16U + 20U * 32U + 8U);
    CHECK(w.bytes_written == c.length && w.records_written == 20U);
    /* Every chunk boundary is a unit boundary. */
    {
        size_t offset = 0U, k;
        bc_opus_stream_parser p;
        bc_opus_stream_parser_init(&p);
        for (k = 0; k < c.chunks; ++k) {
            size_t used = 0U, n = c.chunk_lengths[k], pos = 0U;
            bc_opus_parse_event event;
            while (pos < n) {
                event = bc_opus_stream_parse(&p, c.stream + offset + pos, n - pos, &used);
                CHECK(event != BC_OPUS_PARSE_ERROR_MAGIC && event != BC_OPUS_PARSE_ERROR_FORMAT &&
                      event != BC_OPUS_PARSE_ERROR_LENGTH && event != BC_OPUS_PARSE_ERROR_TRAILER &&
                      event != BC_OPUS_PARSE_ERROR_AFTER_END);
                pos += used;
            }
            /* Chunk ends exactly on a unit boundary: no partial record pending. */
            CHECK(p.pending_length == 0U && p.record_length == 0U);
            offset += n;
        }
        CHECK(p.ended && p.packets == 20U && p.sample_count == 6400U);
    }
}

static void test_writer_fragments_only_oversized_units(void)
{
    bc_opus_stream_writer w;
    bc_audio_format f;
    collector c;
    uint8_t big[700], small[30];
    bc_opus_stream_parser p;
    size_t used, pos = 0U;
    unsigned packets = 0U;
    memset(&c, 0, sizeof(c));
    memset(big, 0x11, sizeof(big));
    memset(small, 0x22, sizeof(small));
    opus_format(&f);
    bc_opus_stream_writer_init(&w);
    CHECK(bc_opus_stream_write_header(&w, &f));
    CHECK(bc_opus_stream_write_packet(&w, small, sizeof(small)));
    CHECK(bc_opus_stream_write_packet(&w, big, sizeof(big)));
    collect(&c, &w, false);
    /* header + small record are flushed first (48 bytes), then the 702-byte
     * record is fragmented into 220 + 220 + 220 and a 42-byte remainder that
     * waits for more data or a flush. */
    CHECK(c.chunks == 4U);
    CHECK(c.chunk_lengths[0] == 48U && c.chunk_lengths[1] == 220U &&
          c.chunk_lengths[2] == 220U && c.chunk_lengths[3] == 220U);
    CHECK(bc_opus_stream_write_packet(&w, small, sizeof(small)));
    collect(&c, &w, false);
    CHECK(c.chunks == 4U); /* 42 + 32 = 74 fits: still waiting */
    CHECK(bc_opus_stream_write_trailer(&w, 1U));
    collect(&c, &w, true);
    CHECK(c.chunks == 5U && c.chunk_lengths[4] == 42U + 32U + 8U);
    CHECK(c.length == 16U + 32U + 702U + 32U + 8U);
    bc_opus_stream_parser_init(&p);
    while (pos < c.length) {
        bc_opus_parse_event event = bc_opus_stream_parse(&p, c.stream + pos, c.length - pos, &used);
        if (event == BC_OPUS_PARSE_PACKET) {
            size_t n;
            const uint8_t *packet = bc_opus_stream_parser_packet(&p, &n);
            ++packets;
            CHECK(n == (packets == 2U ? 700U : 30U));
            CHECK(packet[0] == (packets == 2U ? 0x11U : 0x22U));
        }
        CHECK(event != BC_OPUS_PARSE_ERROR_LENGTH);
        pos += used;
    }
    CHECK(packets == 3U && p.ended && p.sample_count == 1U);
    CHECK(!bc_opus_stream_write_packet(&w, big, BC_OPUS_PACKET_MAX + 1U));
}

static void test_parser_random_chunking_and_errors(void)
{
    static uint8_t stream[4096];
    size_t length = 0U, pos, used, i;
    uint8_t header[16];
    bc_audio_format f;
    bc_opus_stream_parser p;
    unsigned trial;
    opus_format(&f);
    bc_opus_stream_encode_header(header, &f);
    memcpy(stream, header, 16U); length = 16U;
    for (i = 0; i < 40U; ++i) {
        size_t n = 1U + (size_t)(rand() % 60);
        stream[length++] = (uint8_t)n; stream[length++] = 0U;
        memset(stream + length, (int)i, n); length += n;
    }
    stream[length++] = 0U; stream[length++] = 0U; stream[length++] = 1U; stream[length++] = 0U;
    stream[length++] = 0x10U; stream[length++] = 0x27U; stream[length++] = 0U; stream[length++] = 0U;
    for (trial = 0; trial < 50U; ++trial) {
        unsigned packets = 0U;
        bool saw_header = false, saw_trailer = false;
        bc_opus_stream_parser_init(&p);
        pos = 0U;
        while (pos < length) {
            size_t chunk = 1U + (size_t)(rand() % 97);
            size_t chunk_pos = 0U;
            if (chunk > length - pos) chunk = length - pos;
            while (chunk_pos < chunk) {
                bc_opus_parse_event e = bc_opus_stream_parse(&p, stream + pos + chunk_pos, chunk - chunk_pos, &used);
                if (e == BC_OPUS_PARSE_HEADER) saw_header = true;
                else if (e == BC_OPUS_PARSE_PACKET) {
                    size_t n; const uint8_t *packet = bc_opus_stream_parser_packet(&p, &n);
                    CHECK(packet[0] == (uint8_t)packets);
                    ++packets;
                } else if (e == BC_OPUS_PARSE_TRAILER) saw_trailer = true;
                else CHECK(e == BC_OPUS_PARSE_NEED_MORE);
                chunk_pos += used;
            }
            pos += chunk;
        }
        CHECK(saw_header && saw_trailer && packets == 40U && p.sample_count == 10000U);
        CHECK(bc_audio_format_equal(&p.format, &f));
    }
    /* Bytes after the trailer are an explicit error. */
    CHECK(bc_opus_stream_parse(&p, stream, 1U, &used) == BC_OPUS_PARSE_ERROR_AFTER_END);
    /* Bad magic. */
    bc_opus_stream_parser_init(&p);
    stream[0] = 'Q';
    CHECK(bc_opus_stream_parse(&p, stream, 16U, &used) == BC_OPUS_PARSE_ERROR_MAGIC);
    CHECK(bc_opus_stream_parse(&p, stream, 16U, &used) == BC_OPUS_PARSE_ERROR_FORMAT); /* sticky */
    stream[0] = 'S';
    /* Bad header field. */
    bc_opus_stream_parser_init(&p);
    stream[8] = 2U;
    CHECK(bc_opus_stream_parse(&p, stream, 16U, &used) == BC_OPUS_PARSE_ERROR_FORMAT);
    stream[8] = 1U;
    /* Length beyond the codec maximum. */
    bc_opus_stream_parser_init(&p);
    CHECK(bc_opus_stream_parse(&p, stream, 16U, &used) == BC_OPUS_PARSE_HEADER);
    {
        uint8_t bad[2] = {0xfcU, 0x04U}; /* 1276 */
        CHECK(bc_opus_stream_parse(&p, bad, 2U, &used) == BC_OPUS_PARSE_ERROR_LENGTH);
    }
    /* A truncated record or trailer is NEED_MORE, never a fake packet. */
    bc_opus_stream_parser_init(&p);
    CHECK(bc_opus_stream_parse(&p, stream, 16U + 2U + 3U, &used) == BC_OPUS_PARSE_HEADER);
    CHECK(bc_opus_stream_parse(&p, stream + 16U, 2U + 3U, &used) == BC_OPUS_PARSE_NEED_MORE);
    CHECK(p.packets == 0U && used == 5U);
    bc_opus_stream_parser_init(&p);
    {
        uint8_t partial_trailer[16 + 5];
        memcpy(partial_trailer, header, 16U);
        memset(partial_trailer + 16U, 0, 5U); partial_trailer[18] = 1U;
        CHECK(bc_opus_stream_parse(&p, partial_trailer, 16U, &used) == BC_OPUS_PARSE_HEADER);
        CHECK(bc_opus_stream_parse(&p, partial_trailer + 16U, 5U, &used) == BC_OPUS_PARSE_NEED_MORE);
        CHECK(!p.ended);
    }
    /* Wrong trailer kind. */
    bc_opus_stream_parser_init(&p);
    {
        uint8_t wrong[16 + 8];
        memcpy(wrong, header, 16U);
        memset(wrong + 16U, 0, 8U); wrong[18] = 2U;
        CHECK(bc_opus_stream_parse(&p, wrong, 16U, &used) == BC_OPUS_PARSE_HEADER);
        CHECK(bc_opus_stream_parse(&p, wrong + 16U, 8U, &used) == BC_OPUS_PARSE_ERROR_TRAILER);
    }
}

static void test_header_only_discard_and_bounds(void)
{
    bc_opus_stream_writer w;
    bc_audio_format f;
    uint8_t chunk[220];
    uint8_t packet[1275];
    unsigned i;
    opus_format(&f);
    bc_opus_stream_writer_init(&w);
    CHECK(!bc_opus_stream_discard_header_only(&w));
    CHECK(bc_opus_stream_write_header(&w, &f));
    CHECK(bc_opus_stream_discard_header_only(&w));
    CHECK(bc_opus_stream_writer_empty(&w) && !w.header_written);
    /* After a record exists the header can no longer be discarded. */
    CHECK(bc_opus_stream_write_header(&w, &f));
    memset(packet, 1, sizeof(packet));
    CHECK(bc_opus_stream_write_packet(&w, packet, 10U));
    CHECK(!bc_opus_stream_discard_header_only(&w));
    /* The writer buffer is bounded: undrained maximum records fail explicitly. */
    CHECK(bc_opus_stream_write_packet(&w, packet, sizeof(packet)));
    CHECK(!bc_opus_stream_write_packet(&w, packet, sizeof(packet)));
    for (i = 0; i < 8U; ++i)
        (void)bc_opus_stream_take_chunk(&w, chunk, sizeof(chunk), true);
    CHECK(bc_opus_stream_writer_empty(&w));
    CHECK(bc_opus_stream_take_chunk(&w, chunk, sizeof(chunk), true) == 0U);
    CHECK(bc_opus_stream_take_chunk(NULL, chunk, sizeof(chunk), true) == 0U);
    f.codec = BC_AUDIO_CODEC_ADPCM;
    bc_opus_stream_writer_init(&w);
    CHECK(!bc_opus_stream_write_header(&w, &f));
}

int main(void)
{
    test_descriptor_wire_round_trip();
    test_header_round_trip();
    test_writer_aligns_chunks_to_packets();
    test_writer_fragments_only_oversized_units();
    test_parser_random_chunking_and_errors();
    test_header_only_discard_and_bounds();
    printf("opus stream: %u checks, %u failures\n", checks, failures);
    return failures == 0U ? 0 : 1;
}
