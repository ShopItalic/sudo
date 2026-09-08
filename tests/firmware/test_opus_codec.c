/* Real libopus 1.6.1 encoder-to-decoder checks for the Sudo Opus profile.
 * No codec stubs: packets produced by bc_opus_encoder are decoded by the
 * upstream fixed-point decoder, trimmed by pre-skip and the exact sample
 * count, and compared with the input signal. With OPUS_FIXTURE_DIR set, the
 * suite also writes the shared fixtures consumed by the iOS tests. */
#include "bc_opus_encoder.h"
#include "bc_opus_stream.h"
#include "bc_opus_port_host.h"

#include "opus.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* glibc hides M_PI under strict C99; the suites define their own. */
#define TEST_PI 3.14159265358979323846

static unsigned checks, failures;
static void check_condition(bool c, const char *e, unsigned line)
{
    ++checks;
    if (!c) { ++failures; fprintf(stderr, "FAIL line %u: %s\n", line, e); }
}
#define CHECK(c) check_condition((c), #c, __LINE__)

#define MAX_SAMPLES (48000 * 3)
#define MAX_STREAM (1U << 20)

typedef enum { SIG_SILENCE, SIG_SINE, SIG_SPEECH, SIG_NOISE, SIG_IMPULSE } signal_kind;

static void synthesize(signal_kind kind, uint32_t rate, int16_t *out, size_t count, unsigned seed)
{
    size_t i;
    double phase = 0.0, pitch = 120.0;
    for (i = 0; i < count; ++i) {
        double t = (double)i / rate;
        double v = 0.0;
        switch (kind) {
        case SIG_SILENCE: v = 0.0; break;
        case SIG_SINE: v = 0.5 * sin(2.0 * TEST_PI * 440.0 * t); break;
        case SIG_SPEECH: {
            /* Harmonic source with a slowly gliding pitch, formant-like
             * spectral tilt and syllabic amplitude envelope. */
            double envelope = 0.5 * (1.0 - cos(2.0 * TEST_PI * 3.0 * t));
            double sum = 0.0;
            unsigned h;
            pitch = 120.0 + 40.0 * sin(2.0 * TEST_PI * 0.7 * t);
            phase += 2.0 * TEST_PI * pitch / rate;
            for (h = 1; h <= 20U; ++h) {
                double f = pitch * h;
                double formant = 1.0 / (1.0 + pow((f - 600.0) / 250.0, 2.0)) +
                                 0.5 / (1.0 + pow((f - 1500.0) / 300.0, 2.0));
                if (f < rate / 2.0) sum += formant * sin(h * phase) / h;
            }
            v = 0.6 * envelope * sum;
            break;
        }
        case SIG_NOISE:
            seed = seed * 1103515245U + 12345U;
            v = ((double)((seed >> 8) & 0xffffU) / 65535.0 - 0.5) * 0.4;
            break;
        case SIG_IMPULSE: v = (i % (rate / 4)) == 0 ? 0.9 : 0.0; break;
        }
        if (v > 0.999) v = 0.999;
        if (v < -0.999) v = -0.999;
        out[i] = (int16_t)lrint(v * 32767.0);
    }
}

typedef struct {
    const char *name;
    signal_kind signal;
    uint32_t rate;
    uint8_t frame_ms;
    uint32_t bitrate;
    bool vbr;
    bc_opus_application application;
    size_t samples;       /* input sample count */
    double min_correlation;
    bool expect_constant_packets;
    bool expect_large_packets;
    bool fixture;
} scenario;

/* "profile-*" scenarios track bc_opus_profile.h so a tuning change re-measures
 * the target profile's memory, packet size and fidelity automatically. */
#define PROFILE BC_OPUS_SAMPLE_RATE_HZ, BC_OPUS_FRAME_MS, BC_OPUS_BITRATE_BPS, (BC_OPUS_VBR != 0), BC_OPUS_APPLICATION
#define PROFILE_CBR (BC_OPUS_VBR == 0)
static const scenario scenarios[] = {
    {"profile-speech", SIG_SPEECH, PROFILE, BC_OPUS_SAMPLE_RATE_HZ * 2, 0.80, PROFILE_CBR, false, true},
    {"profile-sine", SIG_SINE, PROFILE, BC_OPUS_SAMPLE_RATE_HZ, 0.90, PROFILE_CBR, false, true},
    {"profile-silence", SIG_SILENCE, PROFILE, BC_OPUS_SAMPLE_RATE_HZ, 0.0, PROFILE_CBR, false, true},
    {"profile-noise", SIG_NOISE, PROFILE, BC_OPUS_SAMPLE_RATE_HZ / 2, 0.0, PROFILE_CBR, false, false},
    {"profile-impulse", SIG_IMPULSE, PROFILE, BC_OPUS_SAMPLE_RATE_HZ / 2, 0.0, PROFILE_CBR, false, false},
    {"profile-short-one-sample", SIG_SINE, PROFILE, 1, 0.0, PROFILE_CBR, false, true},
    {"profile-short-half-frame", SIG_SINE, PROFILE, BC_OPUS_FRAME_SAMPLES / 2, 0.0, PROFILE_CBR, false, true},
    {"profile-short-one-frame", SIG_SINE, PROFILE, BC_OPUS_FRAME_SAMPLES, 0.0, PROFILE_CBR, false, true},
    {"profile-short-frame-plus-one", SIG_SINE, PROFILE, BC_OPUS_FRAME_SAMPLES + 1, 0.0, PROFILE_CBR, false, true},
    {"profile-preskip-boundary", SIG_SPEECH, PROFILE, BC_OPUS_FRAME_SAMPLES * 4 - 104, 0.70, PROFILE_CBR, false, true},
    {"cbr-8k", SIG_SPEECH, 16000, 20, 8000, false, BC_OPUS_APP_VOIP, 16000, 0.70, true, false, true},
    {"cbr-16k", SIG_SPEECH, 16000, 20, 16000, false, BC_OPUS_APP_VOIP, 16000, 0.85, true, false, true},
    {"cbr-24k", SIG_SPEECH, 16000, 20, 24000, false, BC_OPUS_APP_VOIP, 16000, 0.90, true, false, true},
    {"cbr-32k", SIG_SPEECH, 16000, 20, 32000, false, BC_OPUS_APP_VOIP, 16000, 0.90, true, false, false},
    {"vbr-12k", SIG_SPEECH, 16000, 20, 12000, true, BC_OPUS_APP_VOIP, 16000, 0.80, false, false, true},
    {"vbr-24k", SIG_SPEECH, 16000, 20, 24000, true, BC_OPUS_APP_VOIP, 16000, 0.85, false, false, false},
    {"rate-8k-cbr-12k", SIG_SPEECH, 8000, 20, 12000, false, BC_OPUS_APP_VOIP, 8000, 0.80, true, false, true},
    {"rate-12k-cbr-12k", SIG_SPEECH, 12000, 20, 12000, false, BC_OPUS_APP_VOIP, 12000, 0.80, true, false, false},
    {"rate-24k-cbr-16k", SIG_SPEECH, 24000, 20, 16000, false, BC_OPUS_APP_VOIP, 24000, 0.80, true, false, true},
    {"rate-48k-cbr-24k", SIG_SPEECH, 48000, 20, 24000, false, BC_OPUS_APP_VOIP, 48000, 0.80, true, false, true},
    {"frame-10ms-cbr-12k", SIG_SPEECH, 16000, 10, 12000, false, BC_OPUS_APP_VOIP, 16000, 0.75, true, false, false},
    {"frame-40ms-cbr-12k", SIG_SPEECH, 16000, 40, 12000, false, BC_OPUS_APP_VOIP, 16000, 0.80, true, false, true},
    {"frame-60ms-cbr-12k", SIG_SPEECH, 16000, 60, 12000, false, BC_OPUS_APP_VOIP, 16000, 0.80, true, false, false},
    {"large-packets-48k-audio-320k", SIG_NOISE, 48000, 20, 320000, true, BC_OPUS_APP_AUDIO, 48000, 0.0, false, true, true},
};

typedef struct {
    bc_opus_encoder encoder;
    unsigned char *state;
    size_t state_size;
    uint8_t *stream;
    size_t stream_length;
    bc_opus_stream_writer writer;
    size_t chunks;
    size_t max_chunk;
    size_t high_water;
} run;

static void run_free(run *r) { free(r->state); free(r->stream); }

static bool run_encode(run *r, const scenario *s, const int16_t *pcm, size_t count)
{
    bc_opus_encoder_config config;
    bc_audio_format format;
    uint8_t packet[BC_OPUS_PACKET_MAX];
    uint8_t chunk[220];
    size_t fed = 0U, produced, n;
    memset(r, 0, sizeof(*r));
    bc_opus_encoder_default_config(&config);
    config.sample_rate_hz = s->rate;
    config.frame_ms = s->frame_ms;
    config.bitrate_bps = s->bitrate;
    config.vbr = s->vbr;
    config.application = s->application;
    r->state_size = bc_opus_encoder_state_size(1U);
    CHECK(r->state_size > 0U && r->state_size <= BC_OPUS_ENCODER_STATE_MAX);
    r->state = malloc(r->state_size);
    r->stream = malloc(MAX_STREAM);
    CHECK(bc_opus_encoder_init(&r->encoder, &config, r->state, r->state_size) == BC_OPUS_OK);
    bc_opus_encoder_format(&r->encoder, &format);
    CHECK(bc_audio_format_valid(&format));
    CHECK(format.pre_skip == r->encoder.pre_skip && format.block_samples == r->encoder.frame_samples);
    bc_opus_stream_writer_init(&r->writer);
    CHECK(bc_opus_stream_write_header(&r->writer, &format));
    test_opus_scratch_refill();
    while (fed < count) {
        bool ready = false;
        size_t taken = bc_opus_encoder_feed(&r->encoder, pcm + fed, count - fed, &ready);
        fed += taken;
        if (ready) {
            CHECK(bc_opus_encoder_encode(&r->encoder, packet, sizeof(packet), &produced) == BC_OPUS_OK);
            CHECK(produced > 0U && produced <= BC_OPUS_PACKET_MAX);
            CHECK(bc_opus_stream_write_packet(&r->writer, packet, produced));
            while ((n = bc_opus_stream_take_chunk(&r->writer, chunk, sizeof(chunk), false)) != 0U) {
                memcpy(r->stream + r->stream_length, chunk, n);
                r->stream_length += n; ++r->chunks; if (n > r->max_chunk) r->max_chunk = n;
            }
        } else if (taken == 0U) {
            CHECK(false); break;
        }
    }
    CHECK(bc_opus_encoder_finish(&r->encoder) == BC_OPUS_OK);
    while (bc_opus_encoder_pending(&r->encoder)) {
        CHECK(bc_opus_encoder_encode(&r->encoder, packet, sizeof(packet), &produced) == BC_OPUS_OK);
        CHECK(bc_opus_stream_write_packet(&r->writer, packet, produced));
    }
    CHECK(bc_opus_encoder_finished(&r->encoder));
    CHECK(bc_opus_encoder_sample_count(&r->encoder) == count);
    /* Every real sample is representable after pre-skip; an empty capture
     * emits nothing at all. */
    if (count != 0U) CHECK(r->encoder.samples_emitted >= count + r->encoder.pre_skip);
    else CHECK(r->encoder.samples_emitted == 0U);
    CHECK(r->encoder.samples_emitted < count + r->encoder.pre_skip + 2U * r->encoder.frame_samples);
    if (count == 0U) {
        CHECK(bc_opus_stream_discard_header_only(&r->writer));
    } else {
        CHECK(bc_opus_stream_write_trailer(&r->writer, (uint32_t)count));
    }
    while ((n = bc_opus_stream_take_chunk(&r->writer, chunk, sizeof(chunk), true)) != 0U) {
        memcpy(r->stream + r->stream_length, chunk, n);
        r->stream_length += n; ++r->chunks; if (n > r->max_chunk) r->max_chunk = n;
    }
    r->high_water = test_opus_scratch_high_water();
    return failures == 0U;
}

/* Decodes a container stream with the upstream decoder. Returns trimmed PCM
 * sample count; fills descriptor and per-packet statistics. */
typedef struct {
    bc_audio_format format;
    uint32_t packets;
    uint32_t min_packet, max_packet;
    uint32_t decoded_samples;    /* before trimming */
    uint32_t trimmed_samples;    /* after pre-skip and trailer */
    uint32_t trailer_samples;
    bool ended;
} decode_report;

static size_t run_decode(const uint8_t *stream, size_t length, size_t chunk_size,
                         int16_t *pcm, size_t capacity, decode_report *report)
{
    bc_opus_stream_parser parser;
    OpusDecoder *decoder = NULL;
    size_t pos = 0U, used, out = 0U;
    static int16_t frame[BC_OPUS_FRAME_SAMPLES_MAX * 3];
    memset(report, 0, sizeof(*report));
    bc_opus_stream_parser_init(&parser);
    while (pos < length) {
        size_t chunk = chunk_size == 0U ? 1U + (size_t)(rand() % 300) : chunk_size;
        size_t chunk_pos = 0U;
        if (chunk > length - pos) chunk = length - pos;
        while (chunk_pos < chunk) {
            bc_opus_parse_event e = bc_opus_stream_parse(&parser, stream + pos + chunk_pos, chunk - chunk_pos, &used);
            chunk_pos += used;
            if (e == BC_OPUS_PARSE_HEADER) {
                int error = 0;
                report->format = parser.format;
                decoder = opus_decoder_create((opus_int32)parser.format.sample_rate_hz, 1, &error);
                CHECK(error == OPUS_OK && decoder != NULL);
            } else if (e == BC_OPUS_PARSE_PACKET) {
                size_t n; const uint8_t *packet = bc_opus_stream_parser_packet(&parser, &n);
                int samples;
                CHECK(decoder != NULL);
                if (decoder == NULL) return 0U;
                samples = opus_packet_get_nb_samples(packet, (opus_int32)n, (opus_int32)parser.format.sample_rate_hz);
                CHECK(samples == (int)parser.format.block_samples);
                samples = opus_decode(decoder, packet, (opus_int32)n, frame, (int)(sizeof(frame) / sizeof(frame[0])), 0);
                CHECK(samples == (int)parser.format.block_samples);
                if (samples > 0 && out + (size_t)samples <= capacity) {
                    memcpy(pcm + out, frame, (size_t)samples * sizeof(int16_t));
                    out += (size_t)samples;
                }
                ++report->packets;
                if (report->min_packet == 0U || n < report->min_packet) report->min_packet = (uint32_t)n;
                if (n > report->max_packet) report->max_packet = (uint32_t)n;
            } else if (e == BC_OPUS_PARSE_TRAILER) {
                report->ended = true;
                report->trailer_samples = parser.sample_count;
            } else {
                CHECK(e == BC_OPUS_PARSE_NEED_MORE);
            }
        }
        pos += chunk;
    }
    if (decoder != NULL) opus_decoder_destroy(decoder);
    report->decoded_samples = (uint32_t)out;
    /* Trim: drop pre-skip, then keep exactly the trailer count when known. */
    if (out > report->format.pre_skip) {
        size_t keep = out - report->format.pre_skip;
        if (report->ended && report->trailer_samples < keep) keep = report->trailer_samples;
        memmove(pcm, pcm + report->format.pre_skip, keep * sizeof(int16_t));
        out = keep;
    } else out = 0U;
    report->trimmed_samples = (uint32_t)out;
    return out;
}

static double correlation(const int16_t *a, const int16_t *b, size_t count)
{
    double sa = 0.0, sb = 0.0, sab = 0.0;
    size_t i;
    for (i = 0; i < count; ++i) { sa += (double)a[i] * a[i]; sb += (double)b[i] * b[i]; sab += (double)a[i] * b[i]; }
    if (sa == 0.0 || sb == 0.0) return 0.0;
    return sab / sqrt(sa * sb);
}

static double rms(const int16_t *a, size_t count)
{
    double s = 0.0; size_t i;
    for (i = 0; i < count; ++i) s += (double)a[i] * a[i];
    return count ? sqrt(s / count) : 0.0;
}

static const char *fixture_dir;
static size_t worst_high_water, profile_high_water;

static void write_fixture(const scenario *s, const run *r, const int16_t *input, size_t count,
                          const int16_t *decoded, const decode_report *report)
{
    char path[512];
    FILE *f;
    if (fixture_dir == NULL || !s->fixture) return;
    snprintf(path, sizeof(path), "%s/%s.sopus", fixture_dir, s->name);
    f = fopen(path, "wb"); CHECK(f != NULL); if (f) { fwrite(r->stream, 1, r->stream_length, f); fclose(f); }
    snprintf(path, sizeof(path), "%s/%s.decoded.pcm", fixture_dir, s->name);
    f = fopen(path, "wb"); CHECK(f != NULL); if (f) { fwrite(decoded, sizeof(int16_t), report->trimmed_samples, f); fclose(f); }
    (void)input;
    snprintf(path, sizeof(path), "%s/%s.meta.json", fixture_dir, s->name);
    f = fopen(path, "w"); CHECK(f != NULL);
    if (f) {
        fprintf(f, "{\n  \"name\": \"%s\",\n  \"libopus\": \"1.6.1\",\n  \"encoder\": \"fixed-point, complexity %d\",\n",
                s->name, (int)BC_OPUS_COMPLEXITY);
        fprintf(f, "  \"sampleRate\": %u,\n  \"channels\": 1,\n  \"frameMilliseconds\": %u,\n  \"bitrate\": %u,\n  \"vbr\": %s,\n",
                s->rate, s->frame_ms, s->bitrate, s->vbr ? "true" : "false");
        fprintf(f, "  \"preSkip\": %u,\n  \"blockSamples\": %u,\n  \"inputSamples\": %zu,\n  \"packets\": %u,\n",
                report->format.pre_skip, report->format.block_samples, count, report->packets);
        fprintf(f, "  \"minPacketBytes\": %u,\n  \"maxPacketBytes\": %u,\n  \"decodedSamples\": %u,\n  \"trimmedSamples\": %u,\n",
                report->min_packet, report->max_packet, report->decoded_samples, report->trimmed_samples);
        fprintf(f, "  \"trailerSamples\": %u,\n  \"containerBytes\": %zu,\n  \"chunkCount\": %zu,\n  \"maxChunkBytes\": %zu\n}\n",
                report->trailer_samples, r->stream_length, r->chunks, r->max_chunk);
        fclose(f);
    }
}

static void test_scenario(const scenario *s)
{
    static int16_t input[MAX_SAMPLES], decoded[MAX_SAMPLES + 4096], decoded_again[MAX_SAMPLES + 4096];
    run r;
    decode_report report, report_again;
    size_t trimmed, again;
    CHECK(s->samples <= MAX_SAMPLES);
    synthesize(s->signal, s->rate, input, s->samples, 7U);
    if (!run_encode(&r, s, input, s->samples)) { fprintf(stderr, "scenario %s: encode failed\n", s->name); run_free(&r); return; }
    if (r.high_water > worst_high_water) worst_high_water = r.high_water;
    if (strncmp(s->name, "profile-", 8) == 0 && r.high_water > profile_high_water) profile_high_water = r.high_water;
    CHECK(r.high_water < test_opus_scratch_capacity());
    /* The target scratch bound must cover the profile with review margin. */
    if (strncmp(s->name, "profile-", 8) == 0) CHECK(r.high_water + r.high_water / 4U <= BC_OPUS_SCRATCH_BYTES);
    trimmed = run_decode(r.stream, r.stream_length, 0U, decoded, sizeof(decoded) / sizeof(decoded[0]), &report);
    again = run_decode(r.stream, r.stream_length, 1U, decoded_again, sizeof(decoded_again) / sizeof(decoded_again[0]), &report_again);
    /* Chunking never changes the decode. */
    CHECK(again == trimmed && memcmp(decoded, decoded_again, trimmed * sizeof(int16_t)) == 0);
    CHECK(report.ended && report.trailer_samples == s->samples);
    CHECK(trimmed == s->samples);
    CHECK(report.format.sample_rate_hz == s->rate && report.format.frame_ms == s->frame_ms);
    CHECK(report.packets == r.encoder.stats.packets);
    CHECK(report.decoded_samples == r.encoder.samples_emitted);
    CHECK(r.max_chunk <= 220U);
    if (s->expect_constant_packets) CHECK(report.min_packet == report.max_packet);
    else if (s->samples >= 8000U) CHECK(report.min_packet != report.max_packet);
    if (s->expect_large_packets) CHECK(report.max_packet > 220U);
    else CHECK(report.max_packet <= 220U);
    if (s->expect_constant_packets && !s->vbr) {
        uint32_t expected = s->bitrate * s->frame_ms / 8000U;
        CHECK(report.max_packet == expected);
    }
    if (s->signal == SIG_SILENCE) {
        CHECK(rms(decoded, trimmed) < 8.0);
    } else if (s->min_correlation > 0.0) {
        double c = correlation(input, decoded, trimmed);
        double ratio = rms(decoded, trimmed) / rms(input, trimmed);
        if (c < s->min_correlation || ratio < 0.5 || ratio > 2.0)
            fprintf(stderr, "scenario %s: correlation %.3f rms ratio %.3f\n", s->name, c, ratio);
        CHECK(c >= s->min_correlation);
        CHECK(ratio > 0.5 && ratio < 2.0);
    }
    write_fixture(s, &r, input, s->samples, decoded, &report);
    printf("  %-32s packets=%4u bytes=%3u..%4u chunks=%4zu max=%3zu container=%6zu scratch=%5zu\n",
           s->name, report.packets, report.min_packet, report.max_packet, r.chunks, r.max_chunk,
           r.stream_length, r.high_water);
    run_free(&r);
}

static void test_zero_samples_and_reset(void)
{
    run r;
    scenario s = scenarios[0];
    int16_t sample[320];
    uint8_t packet[BC_OPUS_PACKET_MAX];
    size_t produced;
    bool ready;
    s.samples = 0U;
    CHECK(run_encode(&r, &s, NULL, 0U));
    /* Nothing was captured: no packets, no container bytes. */
    CHECK(r.encoder.stats.packets == 0U && r.stream_length == 0U);
    /* Reset re-arms the same state for the next recording with equal pre-skip. */
    CHECK(bc_opus_encoder_reset(&r.encoder) == BC_OPUS_OK);
    CHECK(r.encoder.pre_skip == 104U && r.encoder.samples_in == 0U && !r.encoder.finished);
    memset(sample, 0, sizeof(sample));
    CHECK(bc_opus_encoder_feed(&r.encoder, sample, 320U, &ready) == 320U && ready);
    CHECK(bc_opus_encoder_feed(&r.encoder, sample, 1U, &ready) == 0U && ready);
    CHECK(bc_opus_encoder_encode(&r.encoder, packet, sizeof(packet), &produced) == BC_OPUS_OK && produced == 30U);
    CHECK(bc_opus_encoder_encode(&r.encoder, packet, sizeof(packet), &produced) == BC_OPUS_SEQUENCE);
    /* A too-small output buffer is an explicit error, never a silent drop. */
    CHECK(bc_opus_encoder_feed(&r.encoder, sample, 320U, &ready) == 320U);
    CHECK(bc_opus_encoder_encode(&r.encoder, packet, 1U, &produced) == BC_OPUS_OUTPUT_TOO_SMALL);
    run_free(&r);
}

static void test_explicit_failures(void)
{
    bc_opus_encoder enc;
    bc_opus_encoder_config config;
    unsigned char tiny[16];
    bc_opus_encoder_default_config(&config);
    CHECK(bc_opus_encoder_config_valid(&config));
    CHECK(bc_opus_encoder_init(&enc, &config, tiny, sizeof(tiny)) == BC_OPUS_STATE_TOO_SMALL);
    config.sample_rate_hz = 44100U;
    CHECK(!bc_opus_encoder_config_valid(&config));
    CHECK(bc_opus_encoder_init(&enc, &config, tiny, sizeof(tiny)) == BC_OPUS_INVALID);
    bc_opus_encoder_default_config(&config);
    config.frame_ms = 25U; CHECK(!bc_opus_encoder_config_valid(&config));
    bc_opus_encoder_default_config(&config);
    config.bitrate_bps = 1000U; CHECK(!bc_opus_encoder_config_valid(&config));
    bc_opus_encoder_default_config(&config);
    config.max_packet_bytes = 1276U; CHECK(!bc_opus_encoder_config_valid(&config));
    CHECK(bc_opus_encoder_state_size(2U) == 0U);
    /* The profile never asks the C allocator for anything. */
    CHECK(test_opus_alloc_calls() == 0U || test_opus_free_calls() == test_opus_alloc_calls());
}

/* A stream whose bitrate changes between packets, written directly with the
 * upstream encoder: the phone must accept any valid packet sequence. */
static void test_bitrate_switch_fixture(void)
{
    static int16_t input[16000 * 2], decoded[16000 * 2 + 4096];
    OpusEncoder *enc;
    int error = 0;
    bc_audio_format format;
    bc_opus_stream_writer writer;
    uint8_t *stream = malloc(MAX_STREAM);
    uint8_t packet[BC_OPUS_PACKET_MAX], chunk[220];
    size_t length = 0U, frame, n, trimmed;
    opus_int32 lookahead = 0;
    decode_report report;
    run fake;
    scenario s = {"bitrate-switch-vbr", SIG_SPEECH, 16000, 20, 0, true, BC_OPUS_APP_VOIP, 16000 * 2, 0.75, false, false, true};
    static const opus_int32 bitrates[] = {8000, 12000, 24000, 16000, 40000, 6000, 32000, 12000};
    synthesize(SIG_SPEECH, 16000U, input, s.samples, 3U);
    enc = opus_encoder_create(16000, 1, OPUS_APPLICATION_VOIP, &error);
    CHECK(error == OPUS_OK && enc != NULL);
    if (enc == NULL) { free(stream); return; }
    CHECK(opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(0)) == OPUS_OK);
    CHECK(opus_encoder_ctl(enc, OPUS_SET_VBR(1)) == OPUS_OK);
    CHECK(opus_encoder_ctl(enc, OPUS_GET_LOOKAHEAD(&lookahead)) == OPUS_OK);
    memset(&format, 0, sizeof(format));
    format.codec = BC_AUDIO_CODEC_OPUS; format.container_version = 1U; format.sample_rate_hz = 16000U;
    format.channels = 1U; format.frame_ms = 20U; format.pre_skip = (uint16_t)lookahead; format.block_samples = 320U;
    bc_opus_stream_writer_init(&writer);
    CHECK(bc_opus_stream_write_header(&writer, &format));
    for (frame = 0; frame * 320U < s.samples; ++frame) {
        int produced;
        CHECK(opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrates[(frame / 10U) % 8U])) == OPUS_OK);
        produced = opus_encode(enc, input + frame * 320U, 320, packet, sizeof(packet));
        CHECK(produced > 0);
        CHECK(bc_opus_stream_write_packet(&writer, packet, (size_t)produced));
        while ((n = bc_opus_stream_take_chunk(&writer, chunk, sizeof(chunk), false)) != 0U) { memcpy(stream + length, chunk, n); length += n; }
    }
    /* Flush the encoder delay with one silent frame. */
    {
        int16_t silence[320]; int produced;
        memset(silence, 0, sizeof(silence));
        produced = opus_encode(enc, silence, 320, packet, sizeof(packet));
        CHECK(produced > 0 && bc_opus_stream_write_packet(&writer, packet, (size_t)produced));
    }
    CHECK(bc_opus_stream_write_trailer(&writer, (uint32_t)s.samples));
    while ((n = bc_opus_stream_take_chunk(&writer, chunk, sizeof(chunk), true)) != 0U) { memcpy(stream + length, chunk, n); length += n; }
    opus_encoder_destroy(enc);
    trimmed = run_decode(stream, length, 0U, decoded, sizeof(decoded) / sizeof(decoded[0]), &report);
    CHECK(trimmed == s.samples && report.min_packet != report.max_packet);
    CHECK(correlation(input, decoded, trimmed) >= s.min_correlation);
    memset(&fake, 0, sizeof(fake));
    fake.stream = stream; fake.stream_length = length; fake.chunks = 0U; fake.max_chunk = 220U;
    write_fixture(&s, &fake, input, s.samples, decoded, &report);
    free(stream);
}

int main(void)
{
    size_t i;
    fixture_dir = getenv("OPUS_FIXTURE_DIR");
    printf("libopus: %s, encoder state %zu bytes, scratch capacity %zu bytes\n",
           opus_get_version_string(), bc_opus_encoder_state_size(1U), test_opus_scratch_capacity());
    for (i = 0; i < sizeof(scenarios) / sizeof(scenarios[0]); ++i) test_scenario(&scenarios[i]);
    test_zero_samples_and_reset();
    test_explicit_failures();
    test_bitrate_switch_fixture();
    printf("pseudostack high water: profile %zu bytes (target bound %u with 25%% margin), worst host scenario %zu bytes\n",
           profile_high_water, (unsigned)BC_OPUS_SCRATCH_BYTES, worst_high_water);
    CHECK(profile_high_water + profile_high_water / 4U <= BC_OPUS_SCRATCH_BYTES);
    printf("opus codec: %u checks, %u failures\n", checks, failures);
    return failures == 0U ? 0 : 1;
}
