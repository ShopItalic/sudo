#include "bc_opus_encoder.h"

#include <string.h>

#include "opus.h"

void bc_opus_encoder_default_config(bc_opus_encoder_config *c)
{
    if (c == NULL) return;
    memset(c, 0, sizeof(*c));
    c->sample_rate_hz = BC_OPUS_SAMPLE_RATE_HZ;
    c->channels = BC_OPUS_CHANNELS;
    c->frame_ms = BC_OPUS_FRAME_MS;
    c->bitrate_bps = BC_OPUS_BITRATE_BPS;
    c->complexity = BC_OPUS_COMPLEXITY;
    c->vbr = BC_OPUS_VBR != 0;
    c->constrained_vbr = false;
    c->dtx = BC_OPUS_DTX != 0;
    c->inband_fec = BC_OPUS_INBAND_FEC != 0;
    c->application = BC_OPUS_APPLICATION;
    c->max_packet_bytes = BC_OPUS_PACKET_MAX;
}

bool bc_opus_encoder_config_valid(const bc_opus_encoder_config *c)
{
    if (c == NULL || c->channels != 1U) return false;
    if (c->sample_rate_hz != 8000U && c->sample_rate_hz != 12000U &&
        c->sample_rate_hz != 16000U && c->sample_rate_hz != 24000U &&
        c->sample_rate_hz != 48000U)
        return false;
    if (c->frame_ms != 10U && c->frame_ms != 20U && c->frame_ms != 40U && c->frame_ms != 60U)
        return false;
    if (c->sample_rate_hz * c->frame_ms / 1000U > BC_OPUS_FRAME_SAMPLES_MAX) return false;
    if (c->bitrate_bps < 6000U || c->bitrate_bps > 510000U) return false;
    if (c->complexity < 0 || c->complexity > 10) return false;
    if (c->application > BC_OPUS_APP_RESTRICTED_LOWDELAY) return false;
    if (c->max_packet_bytes == 0U || c->max_packet_bytes > BC_OPUS_PACKET_MAX) return false;
    return true;
}

size_t bc_opus_encoder_state_size(uint8_t channels)
{
    int size;
    if (channels != 1U) return 0U;
    size = opus_encoder_get_size(1);
    return size > 0 ? (size_t)size : 0U;
}

static int application_value(bc_opus_application application)
{
    switch (application) {
    case BC_OPUS_APP_AUDIO: return OPUS_APPLICATION_AUDIO;
    case BC_OPUS_APP_RESTRICTED_LOWDELAY: return OPUS_APPLICATION_RESTRICTED_LOWDELAY;
    case BC_OPUS_APP_VOIP:
    default: return OPUS_APPLICATION_VOIP;
    }
}

static bc_opus_result configure(bc_opus_encoder *enc)
{
    OpusEncoder *st = (OpusEncoder *)enc->state;
    const bc_opus_encoder_config *c = &enc->config;
    opus_int32 lookahead = 0;
    int error;
    error = opus_encoder_init(st, (opus_int32)c->sample_rate_hz, c->channels,
                              application_value(c->application));
    if (error != OPUS_OK) { enc->last_opus_error = error; return BC_OPUS_INIT_FAILED; }
    if (opus_encoder_ctl(st, OPUS_SET_BITRATE((opus_int32)c->bitrate_bps)) != OPUS_OK ||
        opus_encoder_ctl(st, OPUS_SET_COMPLEXITY((opus_int32)c->complexity)) != OPUS_OK ||
        opus_encoder_ctl(st, OPUS_SET_VBR(c->vbr ? 1 : 0)) != OPUS_OK ||
        opus_encoder_ctl(st, OPUS_SET_VBR_CONSTRAINT(c->constrained_vbr ? 1 : 0)) != OPUS_OK ||
        opus_encoder_ctl(st, OPUS_SET_DTX(c->dtx ? 1 : 0)) != OPUS_OK ||
        opus_encoder_ctl(st, OPUS_SET_INBAND_FEC(c->inband_fec ? 1 : 0)) != OPUS_OK ||
        opus_encoder_ctl(st, OPUS_SET_SIGNAL(c->application == BC_OPUS_APP_VOIP ?
                                             OPUS_SIGNAL_VOICE : OPUS_AUTO)) != OPUS_OK ||
        opus_encoder_ctl(st, OPUS_GET_LOOKAHEAD(&lookahead)) != OPUS_OK ||
        lookahead < 0 || lookahead >= (opus_int32)enc->frame_samples) {
        enc->last_opus_error = OPUS_INTERNAL_ERROR;
        return BC_OPUS_INIT_FAILED;
    }
    enc->pre_skip = (uint16_t)lookahead;
    return BC_OPUS_OK;
}

bc_opus_result bc_opus_encoder_init(bc_opus_encoder *enc, const bc_opus_encoder_config *config,
                                    void *state, size_t state_size)
{
    size_t needed;
    bc_opus_result result;
    if (enc == NULL || !bc_opus_encoder_config_valid(config) || state == NULL)
        return BC_OPUS_INVALID;
    memset(enc, 0, sizeof(*enc));
    enc->config = *config;
    needed = bc_opus_encoder_state_size(config->channels);
    if (needed == 0U || needed > state_size) return BC_OPUS_STATE_TOO_SMALL;
    enc->state = state;
    enc->state_size = state_size;
    enc->frame_samples = (uint16_t)(config->sample_rate_hz * config->frame_ms / 1000U);
    result = configure(enc);
    if (result != BC_OPUS_OK) { enc->failed = true; return result; }
    enc->initialized = true;
    return BC_OPUS_OK;
}

void bc_opus_encoder_set_cycle_source(bc_opus_encoder *enc, bc_opus_cycle_source source, void *ctx)
{
    if (enc == NULL) return;
    enc->cycles = source;
    enc->cycles_ctx = ctx;
}

bc_opus_result bc_opus_encoder_reset(bc_opus_encoder *enc)
{
    uint16_t pre_skip;
    bc_opus_result result;
    if (enc == NULL || enc->state == NULL) return BC_OPUS_INVALID;
    pre_skip = enc->pre_skip;
    memset(enc->frame, 0, sizeof(enc->frame));
    enc->frame_fill = 0U;
    enc->samples_in = 0U;
    enc->samples_emitted = 0U;
    enc->finishing = enc->finished = enc->failed = false;
    enc->last_opus_error = 0;
    result = configure(enc);
    if (result != BC_OPUS_OK || (enc->initialized && pre_skip != enc->pre_skip)) {
        enc->failed = true;
        enc->initialized = false;
        return result != BC_OPUS_OK ? result : BC_OPUS_INIT_FAILED;
    }
    enc->initialized = true;
    return BC_OPUS_OK;
}

void bc_opus_encoder_format(const bc_opus_encoder *enc, bc_audio_format *format)
{
    if (format == NULL) return;
    memset(format, 0, sizeof(*format));
    if (enc == NULL || !enc->initialized) return;
    format->codec = BC_AUDIO_CODEC_OPUS;
    format->container_version = 1U;
    format->sample_rate_hz = (uint16_t)enc->config.sample_rate_hz;
    format->channels = enc->config.channels;
    format->frame_ms = enc->config.frame_ms;
    format->pre_skip = enc->pre_skip;
    format->sample_count = 0U;
    format->block_bytes = 0U;
    format->block_samples = enc->frame_samples;
}

size_t bc_opus_encoder_feed(bc_opus_encoder *enc, const int16_t *pcm, size_t count, bool *frame_ready)
{
    size_t room, take;
    if (frame_ready != NULL) *frame_ready = false;
    if (enc == NULL || !enc->initialized || enc->failed || enc->finishing ||
        (count != 0U && pcm == NULL))
        return 0U;
    if (enc->frame_fill >= enc->frame_samples) {
        if (frame_ready != NULL) *frame_ready = true;
        return 0U;
    }
    room = (size_t)enc->frame_samples - enc->frame_fill;
    take = count < room ? count : room;
    if (take != 0U) {
        if (enc->samples_in > UINT32_MAX - take) return 0U;
        memcpy(enc->frame + enc->frame_fill, pcm, take * sizeof(int16_t));
        enc->frame_fill = (uint16_t)(enc->frame_fill + take);
        enc->samples_in += (uint32_t)take;
    }
    if (frame_ready != NULL) *frame_ready = enc->frame_fill >= enc->frame_samples;
    return take;
}

bool bc_opus_encoder_pending(const bc_opus_encoder *enc)
{
    if (enc == NULL || !enc->initialized || enc->failed) return false;
    if (enc->frame_fill >= enc->frame_samples) return true;
    if (!enc->finishing || enc->finished) return false;
    if (enc->frame_fill != 0U) return true;
    /* Nothing captured means nothing to flush: no silent packet is emitted. */
    return enc->samples_in != 0U && enc->samples_emitted < enc->samples_in + enc->pre_skip;
}

bool bc_opus_encoder_finished(const bc_opus_encoder *enc)
{
    return enc != NULL && enc->finished;
}

uint32_t bc_opus_encoder_sample_count(const bc_opus_encoder *enc)
{
    return enc == NULL ? 0U : enc->samples_in;
}

bc_opus_result bc_opus_encoder_finish(bc_opus_encoder *enc)
{
    if (enc == NULL || !enc->initialized || enc->failed) return BC_OPUS_INVALID;
    if (enc->finished) return BC_OPUS_SEQUENCE;
    enc->finishing = true;
    if (!bc_opus_encoder_pending(enc)) enc->finished = true;
    return BC_OPUS_OK;
}

bc_opus_result bc_opus_encoder_encode(bc_opus_encoder *enc, uint8_t *packet, size_t capacity, size_t *length)
{
    opus_int32 max_bytes;
    opus_int32 produced;
    uint32_t start = 0U, elapsed;
    if (length != NULL) *length = 0U;
    if (enc == NULL || packet == NULL || length == NULL || !enc->initialized || enc->failed)
        return BC_OPUS_INVALID;
    if (!bc_opus_encoder_pending(enc)) return BC_OPUS_SEQUENCE;
    if (capacity < 2U) return BC_OPUS_OUTPUT_TOO_SMALL;
    if (enc->frame_fill < enc->frame_samples) {
        /* Finishing: pad the partial (or empty) frame with silence. */
        memset(enc->frame + enc->frame_fill, 0,
               ((size_t)enc->frame_samples - enc->frame_fill) * sizeof(int16_t));
        enc->frame_fill = enc->frame_samples;
    }
    max_bytes = (opus_int32)(capacity < enc->config.max_packet_bytes ? capacity : enc->config.max_packet_bytes);
    if (enc->cycles != NULL) start = enc->cycles(enc->cycles_ctx);
    produced = opus_encode((OpusEncoder *)enc->state, enc->frame, enc->frame_samples,
                           packet, max_bytes);
    if (enc->cycles != NULL) {
        elapsed = enc->cycles(enc->cycles_ctx) - start;
        enc->stats.last_cycles = elapsed;
        if (elapsed > enc->stats.max_cycles) enc->stats.max_cycles = elapsed;
        enc->stats.total_cycles += elapsed;
    }
    if (produced <= 0) {
        enc->failed = true;
        enc->last_opus_error = produced == 0 ? OPUS_INTERNAL_ERROR : (int)produced;
        return produced == OPUS_BUFFER_TOO_SMALL ? BC_OPUS_OUTPUT_TOO_SMALL : BC_OPUS_ENCODE_FAILED;
    }
    enc->frame_fill = 0U;
    enc->samples_emitted += enc->frame_samples;
    ++enc->stats.frames;
    ++enc->stats.packets;
    enc->stats.packet_bytes += (uint32_t)produced;
    if (enc->stats.min_packet_bytes == 0U || (uint32_t)produced < enc->stats.min_packet_bytes)
        enc->stats.min_packet_bytes = (uint32_t)produced;
    if ((uint32_t)produced > enc->stats.max_packet_bytes)
        enc->stats.max_packet_bytes = (uint32_t)produced;
    *length = (size_t)produced;
    if (enc->finishing && !bc_opus_encoder_pending(enc)) enc->finished = true;
    return BC_OPUS_OK;
}
