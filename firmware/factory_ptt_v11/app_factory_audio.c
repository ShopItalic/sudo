#include "app_factory_audio.h"
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>

#if BC_OPUS_PACKET_MAX != 30U || BC_OPUS_FRAME_SAMPLES_MAX != 320U
#error "P11 must use its fixed 16 kHz/12 kbps/20 ms profile"
#endif
#if !defined(ENABLE_HARDENING) || !defined(OVERRIDE_celt_fatal)
#error "P11 requires pre-write libopus pseudostack bounds checks"
#endif
#if defined(VAR_ARRAYS) || defined(USE_ALLOCA)
#error "Do not inherit supplier VLA/alloca codec stacks on P11"
#endif

/* libopus has one NONTHREADSAFE_PSEUDOSTACK. No ISR, decoder, or second
 * worker may call it. On a guard fault reset its pointers before unwinding;
 * never return from celt_fatal into the allocation that would overflow. */
extern char *global_stack;
extern char *scratch_ptr;
static jmp_buf codec_return;
static bool codec_guard_active;
static unsigned guard_failures;
static void *codec_scratch;
static size_t codec_scratch_bytes;

void p11_opus_set_scratch(void *scratch, size_t bytes)
{
    codec_scratch = scratch;
    codec_scratch_bytes = bytes;
    global_stack = scratch_ptr = NULL;
}
unsigned p11_opus_guard_failures(void) { return guard_failures; }
#ifdef P11_AUDIO_TEST
bool p11_audio_test_guard(void (*operation)(void))
{
    if (setjmp(codec_return)) return false;
    codec_guard_active = true;
    operation();
    codec_guard_active = false;
    return true;
}
#endif
void *bc_opus_port_alloc(size_t size) { (void)size; return NULL; }
void bc_opus_port_free(void *ptr) { (void)ptr; }
void *bc_opus_port_alloc_scratch(size_t size)
{
    return size == BC_OPUS_SCRATCH_BYTES && size <= codec_scratch_bytes ? codec_scratch : NULL;
}
void celt_fatal(const char *message, const char *file, int line)
{
    (void)message; (void)file; (void)line;
    ++guard_failures;
    if (codec_guard_active) {
        codec_guard_active = false;
        global_stack = scratch_ptr = NULL;
        longjmp(codec_return, 1);
    }
    /* Unreachable in the reviewed call graph. A call outside the sole owner
     * cannot be recovered by jumping to another task's stack. */
    abort();
}

static bool fail(p11_audio *a) { a->failed = true; return false; }
static bool emit(p11_audio *a, p11_audio_record_kind kind, const uint8_t *data,
                 unsigned length, unsigned samples)
{
    if (a->failed || !a->sink(a->context, kind, data, length, samples)) return fail(a);
    return true;
}
static bool reset_codec(p11_audio *a)
{
    bc_opus_result result;
    if (setjmp(codec_return)) return fail(a);
    codec_guard_active = true;
    result = bc_opus_encoder_reset(&a->encoder);
    codec_guard_active = false;
    if (result != BC_OPUS_OK) return fail(a);
    a->accounted_samples = 0;
    return true;
}
static bool header(p11_audio *a)
{
    uint8_t bytes[16] = {'S','O','P','U',1,2,0x80,0x3e,1,20,0,0,0,0,0,0};
    bytes[10] = (uint8_t)a->encoder.pre_skip;
    bytes[11] = (uint8_t)(a->encoder.pre_skip >> 8);
    return emit(a, P11_AUDIO_DATA, bytes, sizeof(bytes), 0);
}
static bool packet(p11_audio *a)
{
    uint8_t bytes[P11_AUDIO_RECORD_BYTES];
    size_t length = 0;
    bc_opus_result result;
    unsigned samples;
    if (setjmp(codec_return)) return fail(a);
    codec_guard_active = true;
    result = bc_opus_encoder_encode(&a->encoder, bytes + 2, sizeof(bytes) - 2, &length);
    codec_guard_active = false;
    if (result != BC_OPUS_OK || length == 0 || length > 30) return fail(a);
    samples = (unsigned)(a->encoder.samples_in - a->accounted_samples);
    if (samples > 320) return fail(a);
    bytes[0] = (uint8_t)length; bytes[1] = 0;
    if (!emit(a, P11_AUDIO_DATA, bytes, (unsigned)length + 2, samples)) return false;
    a->accounted_samples = a->encoder.samples_in;
    return true;
}
static bool end_segment(p11_audio *a)
{
    uint8_t bytes[8] = {0,0,1,0,0,0,0,0};
    uint32_t samples = a->encoder.samples_in;
    if (bc_opus_encoder_finish(&a->encoder) != BC_OPUS_OK) return fail(a);
    while (bc_opus_encoder_pending(&a->encoder)) if (!packet(a)) return false;
    if (!bc_opus_encoder_finished(&a->encoder)) return fail(a);
    bytes[4] = (uint8_t)samples; bytes[5] = (uint8_t)(samples >> 8);
    bytes[6] = (uint8_t)(samples >> 16); bytes[7] = (uint8_t)(samples >> 24);
    return emit(a, P11_AUDIO_DATA, bytes, sizeof(bytes), 0);
}

bool p11_audio_init(p11_audio *a, void *state, size_t state_bytes,
                    p11_audio_sink sink, void *context)
{
    bc_opus_encoder_config config;
    bc_opus_result result;
    if (!a || !state || !sink || !codec_scratch || codec_scratch_bytes < BC_OPUS_SCRATCH_BYTES)
        return false;
    memset(a, 0, sizeof(*a));
    a->sink = sink; a->context = context;
    a->segment_limit = P11_AUDIO_SEGMENT_SAMPLES;
    bc_opus_encoder_default_config(&config);
    if (!bc_resampler_init(&a->resampler, 16125, 16000)) return fail(a);
    if (setjmp(codec_return)) return fail(a);
    codec_guard_active = true;
    result = bc_opus_encoder_init(&a->encoder, &config, state, state_bytes);
    codec_guard_active = false;
    if (result != BC_OPUS_OK) return fail(a);
    a->ready = true;
    return true;
}
bool p11_audio_begin(p11_audio *a)
{
    if (!a || !a->ready || a->active || a->segment_limit == 0 ||
        a->segment_limit > P11_AUDIO_SEGMENT_SAMPLES || a->segment_limit % 320) return false;
    a->failed = false; a->segments = 1;
    bc_resampler_reset(&a->resampler);
    if (!reset_codec(a)) return false;
    a->active = true;
    return header(a);
}
bool p11_audio_feed(p11_audio *a, const int16_t *pcm, size_t samples)
{
    size_t offset = 0;
    if (!a || !a->active || a->failed || (!pcm && samples)) return false;
    while (offset < samples) {
        int16_t resampled[160];
        size_t used = 0, done = 0;
        size_t count = bc_resampler_process(&a->resampler, pcm + offset, samples - offset,
                                            resampled, 160, &used);
        if (!used && !count) return fail(a);
        offset += used;
        while (done < count) {
            bool frame_ready = false;
            size_t taken;
            /* Only roll when more real samples arrive. Ending exactly at
             * the boundary never creates an empty extra file. Resampler
             * phase is preserved across independently decodable segments. */
            if (a->encoder.samples_in == a->segment_limit) {
                if (!end_segment(a) || !emit(a, P11_AUDIO_ROLLOVER, NULL, 0, 0) ||
                    !reset_codec(a) || !header(a)) return false;
                ++a->segments;
            }
            taken = bc_opus_encoder_feed(&a->encoder, resampled + done, count - done, &frame_ready);
            if (!taken && !frame_ready) return fail(a);
            done += taken;
            if (frame_ready && !packet(a)) return false;
        }
    }
    return true;
}
bool p11_audio_finish(p11_audio *a)
{
    bool ok;
    if (!a || !a->active) return false;
    ok = !a->failed && end_segment(a);
    a->active = false;
    return ok;
}
