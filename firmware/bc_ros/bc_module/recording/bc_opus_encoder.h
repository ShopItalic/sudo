#ifndef BC_OPUS_ENCODER_H
#define BC_OPUS_ENCODER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bc_audio_format.h"
#include "bc_opus_profile.h"

/* Portable libopus encoder wrapper. The caller owns the state buffer, the
 * libopus scratch (see custom_support.h) and the worker context; nothing
 * here allocates, and every failure is an explicit result code. */
typedef enum {
    BC_OPUS_APP_VOIP = 0,
    BC_OPUS_APP_AUDIO = 1,
    BC_OPUS_APP_RESTRICTED_LOWDELAY = 2
} bc_opus_application;

typedef enum {
    BC_OPUS_OK = 0,
    BC_OPUS_INVALID,        /* bad argument or configuration */
    BC_OPUS_STATE_TOO_SMALL,/* opus_encoder_get_size exceeds the buffer */
    BC_OPUS_INIT_FAILED,    /* libopus init/ctl rejected the profile */
    BC_OPUS_ENCODE_FAILED,  /* libopus returned an error */
    BC_OPUS_OUTPUT_TOO_SMALL,
    BC_OPUS_SEQUENCE        /* call order violated (e.g. encode without a frame) */
} bc_opus_result;

typedef struct {
    uint32_t sample_rate_hz;
    uint8_t channels;
    uint8_t frame_ms;
    uint32_t bitrate_bps;
    int8_t complexity;
    bool vbr;
    bool constrained_vbr;
    bool dtx;
    bool inband_fec;
    bc_opus_application application;
    uint16_t max_packet_bytes;
} bc_opus_encoder_config;

/* Optional cycle/time source for timing instrumentation. */
typedef uint32_t (*bc_opus_cycle_source)(void *ctx);

typedef struct {
    uint32_t frames;
    uint32_t packets;
    uint32_t packet_bytes;
    uint32_t last_cycles;
    uint32_t max_cycles;
    uint64_t total_cycles;
    uint32_t min_packet_bytes;
    uint32_t max_packet_bytes;
} bc_opus_encoder_stats;

typedef struct {
    bc_opus_encoder_config config;
    void *state;              /* OpusEncoder* inside the caller's buffer */
    size_t state_size;
    uint16_t frame_samples;
    uint16_t pre_skip;
    int16_t frame[BC_OPUS_FRAME_SAMPLES_MAX];
    uint16_t frame_fill;
    uint32_t samples_in;      /* real samples accepted */
    uint32_t samples_emitted; /* samples covered by emitted packets */
    bool initialized, finishing, finished, failed;
    int last_opus_error;
    bc_opus_cycle_source cycles;
    void *cycles_ctx;
    bc_opus_encoder_stats stats;
} bc_opus_encoder;

/* The Sudo Voice profile from bc_opus_profile.h. */
void bc_opus_encoder_default_config(bc_opus_encoder_config *config);
bool bc_opus_encoder_config_valid(const bc_opus_encoder_config *config);
/* Bytes required for the state buffer with this channel count, or 0. */
size_t bc_opus_encoder_state_size(uint8_t channels);

bc_opus_result bc_opus_encoder_init(bc_opus_encoder *enc,
                                    const bc_opus_encoder_config *config,
                                    void *state, size_t state_size);
void bc_opus_encoder_set_cycle_source(bc_opus_encoder *enc, bc_opus_cycle_source source, void *ctx);
/* Reinitializes the codec state for a new recording; the configuration,
 * buffers and pre-skip must remain identical or the call fails. */
bc_opus_result bc_opus_encoder_reset(bc_opus_encoder *enc);
/* The descriptor a recording with this encoder carries (sample_count 0). */
void bc_opus_encoder_format(const bc_opus_encoder *enc, bc_audio_format *format);

/* Accepts samples until one frame is complete; returns the count consumed.
 * *frame_ready is true when bc_opus_encoder_encode must be called before
 * more samples can be accepted. */
size_t bc_opus_encoder_feed(bc_opus_encoder *enc, const int16_t *pcm, size_t count,
                            bool *frame_ready);
/* Encodes the pending frame (padding a final partial frame with silence when
 * finishing). Returns OK with *length > 0, or an explicit error. */
bc_opus_result bc_opus_encoder_encode(bc_opus_encoder *enc, uint8_t *packet,
                                      size_t capacity, size_t *length);
/* Begins flushing: pending samples are padded to a frame and enough silent
 * frames follow so the decoder can output every real sample after pre-skip.
 * Call encode while bc_opus_encoder_pending(enc) is true. */
bc_opus_result bc_opus_encoder_finish(bc_opus_encoder *enc);
bool bc_opus_encoder_pending(const bc_opus_encoder *enc);
bool bc_opus_encoder_finished(const bc_opus_encoder *enc);
/* Exact real samples accepted; the container trailer value. */
uint32_t bc_opus_encoder_sample_count(const bc_opus_encoder *enc);

#endif
