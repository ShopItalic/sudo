#ifndef APP_FACTORY_AUDIO_P11_H
#define APP_FACTORY_AUDIO_P11_H
#include "bc_opus_encoder.h"
#include "bc_resampler.h"

/* Fixed P11 profile. These are storage records, not BLE packets. The phone
 * container parser remains general; only this encoder is limited to 30 B. */
#define P11_AUDIO_RECORD_BYTES 32U
#define P11_AUDIO_SEGMENT_SAMPLES (16000UL * 600UL)
typedef enum { P11_AUDIO_DATA, P11_AUDIO_ROLLOVER } p11_audio_record_kind;
typedef bool (*p11_audio_sink)(void *context, p11_audio_record_kind kind,
    const uint8_t *bytes, unsigned length, unsigned real_samples);
typedef struct {
    bc_opus_encoder encoder;
    bc_resampler resampler;
    p11_audio_sink sink;
    void *context;
    uint32_t accounted_samples;
    uint32_t segment_limit;
    uint32_t segments;
    bool ready, active, failed;
} p11_audio;

/* All operations, including the sink, belong to one encoder task. Caller
 * owns state/scratch; this core never allocates, logs or touches hardware. */
bool p11_audio_init(p11_audio *, void *state, size_t state_bytes,
    p11_audio_sink, void *context);
bool p11_audio_begin(p11_audio *);
bool p11_audio_feed(p11_audio *, const int16_t *pcm, size_t samples);
bool p11_audio_finish(p11_audio *);

/* P11's ENABLE_HARDENING fatal port unwinds ONLY an active codec call on
 * its owner task. The adapter installs its scratch before init. */
void p11_opus_set_scratch(void *scratch, size_t bytes);
unsigned p11_opus_guard_failures(void);
#endif
