#ifndef BC_AUDIO_FORMAT_H
#define BC_AUDIO_FORMAT_H

#include <stdbool.h>
#include <stdint.h>

/* Negotiated per-recording audio format descriptor. It travels with catalog,
 * query and state snapshots (16 wire bytes), is persisted in recording
 * metadata, and selects the decoder on the phone. Every stored recording
 * carries its own descriptor; nothing assumes the current firmware format. */
enum bc_audio_codec {
    BC_AUDIO_CODEC_NONE = 0,
    /* Supplier IMA ADPCM: 220 raw bytes per 440 mono samples at 8 kHz. */
    BC_AUDIO_CODEC_ADPCM = 1,
    /* RFC 6716 Opus packets in the Sudo length-prefixed container. */
    BC_AUDIO_CODEC_OPUS = 2
};

typedef struct {
    uint8_t codec;             /* enum bc_audio_codec */
    uint8_t container_version; /* Opus container version; 0 for ADPCM */
    uint16_t sample_rate_hz;
    uint8_t channels;
    uint8_t frame_ms;          /* nominal packet/block duration */
    uint16_t pre_skip;         /* decoder samples to drop, at sample_rate_hz */
    uint32_t sample_count;     /* exact real samples; 0 = unknown/untrimmed */
    uint16_t block_bytes;      /* fixed-size codecs; 0 = length-prefixed */
    uint16_t block_samples;    /* samples per block/frame */
} bc_audio_format;

#define BC_AUDIO_FORMAT_WIRE_SIZE 16U

/* Legacy supplier ADPCM as interpreted by every shipped client. */
#define BC_AUDIO_FORMAT_LEGACY_ADPCM_INIT \
    { BC_AUDIO_CODEC_ADPCM, 0U, 8000U, 1U, 55U, 0U, 0U, 220U, 440U }

/* Wire/metadata layout (little endian): 0 codec, 1 container version,
 * 2-3 sample rate, 4 channels, 5 frame ms, 6-7 pre-skip, 8-11 sample count,
 * 12-13 block bytes, 14-15 block samples. */
void bc_audio_format_encode(uint8_t out[BC_AUDIO_FORMAT_WIRE_SIZE],
                            const bc_audio_format *format);
/* Decode rejects unknown codecs and inconsistent fields; an all-zero
 * descriptor (codec NONE) decodes only as the idle/sentinel value. */
bool bc_audio_format_decode(const uint8_t in[BC_AUDIO_FORMAT_WIRE_SIZE],
                            bc_audio_format *format);
bool bc_audio_format_valid(const bc_audio_format *format);
bool bc_audio_format_equal(const bc_audio_format *a, const bc_audio_format *b);
void bc_audio_format_legacy_adpcm(bc_audio_format *format);

#endif
