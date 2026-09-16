#include "app_factory_audio.h"
#include "opus.h"
#include "stack_alloc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static uint8_t scratch_space[BC_OPUS_SCRATCH_BYTES + 64];
static p11_audio audio;
static void *encoder_state;
static OpusDecoder *decoder;
static unsigned units, fail_at, headers, trailers, rolls, packets, written_samples;
static unsigned decoded, preskip, total_samples;
static bool needs_header, ended;
static int16_t decoded_pcm[1920];

static unsigned get32(const uint8_t *p)
{ return (unsigned)p[0] | (unsigned)p[1]<<8 | (unsigned)p[2]<<16 | (unsigned)p[3]<<24; }
static bool write_record(void *context, p11_audio_record_kind kind,
    const uint8_t *p, unsigned length, unsigned samples)
{
    (void)context;
    if (++units == fail_at) return false;
    CHECK(length <= 32);
    if (kind == P11_AUDIO_ROLLOVER) {
        CHECK(ended && length == 0 && samples == 0);
        ++rolls; needs_header = true; ended = false;
    } else if (needs_header) {
        CHECK(length == 16 && !memcmp(p,"SOPU",4));
        CHECK(p[4] == 1 && p[5] == 2 && p[6] == 0x80 && p[7] == 0x3e);
        CHECK(p[8] == 1 && p[9] == 20 && get32(p+12) == 0 && samples == 0);
        preskip = p[10] | (unsigned)p[11]<<8;
        CHECK(preskip < 320);
        CHECK(opus_decoder_init(decoder,16000,1) == OPUS_OK);
        decoded = 0; needs_header = false; ended = false; ++headers;
    } else if (length == 8 && !p[0] && !p[1]) {
        unsigned real = get32(p+4);
        CHECK(!ended && p[2] == 1 && p[3] == 0 && samples == 0);
        CHECK((real == 0 && decoded == 0) || (decoded >= preskip + real && decoded - preskip - real < 320));
        total_samples += real; ++trailers; ended = true;
    } else {
        int count;
        CHECK(!ended && length == 32 && p[0] == 30 && p[1] == 0 && samples <= 320);
        CHECK(opus_packet_get_nb_samples(p+2,(int)length-2,16000) == 320);
        count = opus_decode(decoder,p+2,(int)length-2,decoded_pcm,1920,0);
        CHECK(count == 320);
        decoded += (unsigned)count; written_samples += samples; ++packets;
    }
    return true;
}
static void reset(unsigned fail_unit)
{
    fail_at = fail_unit; units = headers = trailers = rolls = packets = written_samples = total_samples = 0;
    needs_header = true; ended = false;
    memset(scratch_space,0xa5,sizeof(scratch_space));
    p11_opus_set_scratch(scratch_space,BC_OPUS_SCRATCH_BYTES);
    CHECK(p11_audio_init(&audio,encoder_state,bc_opus_encoder_state_size(1),write_record,NULL));
}
static void canary(void)
{
    unsigned i;
    for (i=BC_OPUS_SCRATCH_BYTES;i<sizeof(scratch_space);++i) CHECK(scratch_space[i] == 0xa5);
}
static void recordings(void)
{
    static const unsigned lengths[] = {0,1,2,3,319,320,321,879,880,881,16125,32250,50000};
    unsigned n, offset, i;
    int16_t block[880];
    for (i=0;i<880;i++) block[i] = (int16_t)((i * 29173U) ^ (i<<4));
    for (n=0;n<sizeof(lengths)/sizeof(lengths[0]);++n) {
        reset(0); CHECK(p11_audio_begin(&audio));
        for (offset=0;offset<lengths[n];) {
            unsigned take=lengths[n]-offset; if(take>880) take=880;
            CHECK(p11_audio_feed(&audio,block,take)); offset+=take;
        }
        CHECK(p11_audio_finish(&audio)); CHECK(!p11_audio_finish(&audio));
        CHECK(headers == 1 && trailers == 1 && rolls == 0);
        CHECK(total_samples == audio.resampler.produced && written_samples == total_samples);
        canary();
    }
}
static void rollover(void)
{
    int16_t input[880] = {0};
    unsigned i;
    reset(0); audio.segment_limit=640; CHECK(p11_audio_begin(&audio));
    for (i=0;i<12;i++) CHECK(p11_audio_feed(&audio,input,880));
    CHECK(p11_audio_finish(&audio));
    CHECK(rolls > 10 && headers == rolls+1 && trailers == headers);
    CHECK(total_samples == audio.resampler.produced && total_samples == written_samples);
    canary();
    reset(0); audio.segment_limit=640; CHECK(p11_audio_begin(&audio));
    while (audio.resampler.produced < 640) CHECK(p11_audio_feed(&audio,input,1));
    CHECK(p11_audio_finish(&audio)); CHECK(headers == 1 && rolls == 0 && total_samples == 640);
    canary();
}
static void write_failures(void)
{
    int16_t input[880] = {0};
    unsigned failure;
    for (failure=1;failure<30;failure++) {
        bool ok;
        reset(failure); audio.segment_limit=640;
        ok=p11_audio_begin(&audio);
        for (unsigned i=0;ok && i<15;i++) ok=p11_audio_feed(&audio,input,880);
        CHECK(!ok && audio.failed);
        CHECK(!p11_audio_finish(&audio));
        CHECK(!p11_audio_feed(&audio,input,880));
        canary();
    }
}
extern bool p11_audio_test_guard(void (*operation)(void));
static void overflow(void)
{
    VARDECL(opus_int16, probe);
    global_stack = scratch_ptr = (char *)scratch_space;
    global_stack += BC_OPUS_SCRATCH_BYTES - 4;
    ALLOC(probe,16,opus_int16);
    probe[15]=0; /* Must never execute: PUSH checks before writing. */
}
int main(void)
{
    encoder_state=malloc(bc_opus_encoder_state_size(1));
    decoder=malloc((size_t)opus_decoder_get_size(1)); CHECK(encoder_state && decoder);
    CHECK(sizeof(audio.encoder.frame) == 640);
    recordings(); rollover(); write_failures();
    reset(0); CHECK(!p11_audio_test_guard(overflow)); CHECK(p11_opus_guard_failures() == 1); canary();
    CHECK(p11_audio_begin(&audio)); CHECK(p11_audio_finish(&audio));
    free(decoder); free(encoder_state);
    printf("P11 codec: %u checks, 0 failures; state %zu B; encoder %zu B\n", checks, bc_opus_encoder_state_size(1), sizeof(bc_opus_encoder));
    return 0;
}
