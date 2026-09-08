#include "bc_audio_format.h"

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

void bc_audio_format_legacy_adpcm(bc_audio_format *format)
{
    static const bc_audio_format legacy = BC_AUDIO_FORMAT_LEGACY_ADPCM_INIT;
    if (format != NULL) *format = legacy;
}

bool bc_audio_format_valid(const bc_audio_format *f)
{
    if (f == NULL) return false;
    switch (f->codec) {
    case BC_AUDIO_CODEC_NONE: {
        static const bc_audio_format zero = {0};
        return memcmp(f, &zero, sizeof(zero)) == 0;
    }
    case BC_AUDIO_CODEC_ADPCM: {
        bc_audio_format legacy;
        bc_audio_format_legacy_adpcm(&legacy);
        /* ADPCM never carries a sample count or pre-skip. */
        return memcmp(f, &legacy, sizeof(legacy)) == 0;
    }
    case BC_AUDIO_CODEC_OPUS:
        if (f->container_version != 1U || f->channels != 1U || f->block_bytes != 0U)
            return false;
        if (f->sample_rate_hz != 8000U && f->sample_rate_hz != 12000U &&
            f->sample_rate_hz != 16000U && f->sample_rate_hz != 24000U &&
            f->sample_rate_hz != 48000U)
            return false;
        if (f->frame_ms != 10U && f->frame_ms != 20U && f->frame_ms != 40U &&
            f->frame_ms != 60U)
            return false;
        if ((uint32_t)f->block_samples != (uint32_t)f->sample_rate_hz * f->frame_ms / 1000U)
            return false;
        /* Pre-skip is bounded by one frame of encoder delay. */
        return f->pre_skip < f->block_samples;
    default:
        return false;
    }
}

bool bc_audio_format_equal(const bc_audio_format *a, const bc_audio_format *b)
{
    return a != NULL && b != NULL && memcmp(a, b, sizeof(*a)) == 0;
}

void bc_audio_format_encode(uint8_t out[BC_AUDIO_FORMAT_WIRE_SIZE],
                            const bc_audio_format *f)
{
    memset(out, 0, BC_AUDIO_FORMAT_WIRE_SIZE);
    if (f == NULL) return;
    out[0] = f->codec;
    out[1] = f->container_version;
    put16(out + 2, f->sample_rate_hz);
    out[4] = f->channels;
    out[5] = f->frame_ms;
    put16(out + 6, f->pre_skip);
    put32(out + 8, f->sample_count);
    put16(out + 12, f->block_bytes);
    put16(out + 14, f->block_samples);
}

bool bc_audio_format_decode(const uint8_t in[BC_AUDIO_FORMAT_WIRE_SIZE],
                            bc_audio_format *f)
{
    bc_audio_format value;
    if (in == NULL || f == NULL) return false;
    memset(&value, 0, sizeof(value));
    value.codec = in[0];
    value.container_version = in[1];
    value.sample_rate_hz = get16(in + 2);
    value.channels = in[4];
    value.frame_ms = in[5];
    value.pre_skip = get16(in + 6);
    value.sample_count = get32(in + 8);
    value.block_bytes = get16(in + 12);
    value.block_samples = get16(in + 14);
    if (!bc_audio_format_valid(&value)) return false;
    *f = value;
    return true;
}
