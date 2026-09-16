/* Build-only ADPCM control: same P11 DMA/queue/storage ownership, no Opus
 * initialization or scratch allocation. Never publish this as a P11 release.
 * Retains the factory every-other-sample decimator and IMA predictor. */
#ifndef P11_ADPCM_CONTROL
#error "Control codec is not the P11 Opus release"
#endif
#include "app_factory_audio.h"
#include "adpcm_a.h"
#include <string.h>
static adpcm_state predictor;
bool p11_audio_init(p11_audio *a, void *state, size_t state_bytes, p11_audio_sink sink, void *context)
{
    (void)state; (void)state_bytes;
    if (!a || !sink) return false;
    memset(a, 0, sizeof(*a));
    a->sink = sink; a->context = context; a->ready = true;
    a->segment_limit = 8000UL * 120UL; /* 480 KB, below the inherited 1 MiB guard. */
    return true;
}
bool p11_audio_begin(p11_audio *a)
{
    if (!a || !a->ready || a->active || !a->segment_limit || a->segment_limit % 64) return false;
    memset(&predictor, 0, sizeof(predictor));
    a->accounted_samples = 0; a->segments = 1; a->active = true; a->failed = false;
    return true;
}
bool p11_audio_feed(p11_audio *a, const int16_t *pcm, size_t samples)
{
    if (!a || !a->active || a->failed || !pcm || samples % 4) return false;
    while (samples) {
        int16_t downsampled[64];
        uint8_t bytes[32];
        unsigned i, count = samples > 128 ? 64 : (unsigned)samples / 2;
        if (a->accounted_samples == a->segment_limit) {
            if (!a->sink(a->context, P11_AUDIO_ROLLOVER, NULL, 0, 0)) { a->failed = true; return false; }
            memset(&predictor, 0, sizeof(predictor)); a->accounted_samples = 0; ++a->segments;
        }
        if (count > a->segment_limit - a->accounted_samples) count = a->segment_limit - a->accounted_samples;
        for (i = 0; i < count; ++i) downsampled[i] = pcm[2*i];
        adpcm_encoder(downsampled, (char *)bytes, count, &predictor);
        if (!a->sink(a->context, P11_AUDIO_DATA, bytes, count / 2, count)) { a->failed = true; return false; }
        a->accounted_samples += count; pcm += 2*count; samples -= 2*count;
    }
    return true;
}
bool p11_audio_finish(p11_audio *a)
{
    if (!a || !a->active) return false;
    a->active = false; return !a->failed;
}
