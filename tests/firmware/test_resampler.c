#include "bc_resampler.h"

#include <math.h>
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

static void test_ratio_is_reduced_and_exact(void)
{
    bc_resampler r;
    CHECK(bc_resampler_init(&r, 16125U, 16000U));
    CHECK(r.in_rate == 129U && r.out_rate == 128U);
    CHECK(!bc_resampler_init(&r, 0U, 16000U));
    CHECK(!bc_resampler_init(&r, 16125U, 0U));
    CHECK(!bc_resampler_init(NULL, 16125U, 16000U));
    CHECK(bc_resampler_init(&r, 16000U, 16000U) && r.in_rate == 1U && r.out_rate == 1U);
}

static void test_unity_ratio_passes_samples_through(void)
{
    bc_resampler r;
    int16_t in[64], out[80];
    size_t consumed, produced, i;
    for (i = 0; i < 64U; ++i) in[i] = (int16_t)(i * 500 - 16000);
    CHECK(bc_resampler_init(&r, 8000U, 8000U));
    produced = bc_resampler_process(&r, in, 64U, out, 80U, &consumed);
    CHECK(consumed == 64U);
    /* Unity ratio: output i equals input i; the four-tap window only holds
     * back the last two inputs until later samples arrive. */
    CHECK(produced == 62U);
    for (i = 0; i < produced; ++i) CHECK(out[i] == in[i]);
}

static void test_16125_to_16000_count_and_chunk_equivalence(void)
{
    enum { N = 16125 * 3 };
    static int16_t in[N], whole[16000 * 3 + 8], chunked[16000 * 3 + 8];
    bc_resampler a, b;
    size_t i, produced_whole = 0U, produced_chunked = 0U, offset = 0U, consumed;
    unsigned seed = 12345U;
    for (i = 0; i < N; ++i) {
        seed = seed * 1103515245U + 12345U;
        in[i] = (int16_t)((seed >> 16) & 0xffffU);
    }
    CHECK(bc_resampler_init(&a, 16125U, 16000U));
    CHECK(bc_resampler_init(&b, 16125U, 16000U));
    produced_whole = bc_resampler_process(&a, in, N, whole, sizeof(whole) / sizeof(whole[0]), &consumed);
    CHECK(consumed == N);
    /* Three seconds in yield three seconds out within the window latency. */
    CHECK(produced_whole >= 16000U * 3U - 4U && produced_whole <= 16000U * 3U);
    while (offset < N) {
        size_t chunk = 1U + (size_t)(rand() % 880);
        if (chunk > N - offset) chunk = N - offset;
        produced_chunked += bc_resampler_process(&b, in + offset, chunk,
            chunked + produced_chunked, sizeof(chunked) / sizeof(chunked[0]) - produced_chunked, &consumed);
        CHECK(consumed == chunk);
        offset += chunk;
    }
    CHECK(produced_chunked == produced_whole);
    CHECK(memcmp(whole, chunked, produced_whole * sizeof(int16_t)) == 0);
    CHECK(a.consumed == N && a.produced == produced_whole);
}

static void test_output_capacity_limits_and_resumes(void)
{
    int16_t in[300], out[400];
    bc_resampler r;
    size_t consumed, produced, total = 0U, i, j;
    for (i = 0; i < 300U; ++i) in[i] = (int16_t)(i * 100);
    CHECK(bc_resampler_init(&r, 16125U, 16000U));
    for (i = 0; i < 300U;) {
        produced = bc_resampler_process(&r, in + i, 300U - i, out + total, 7U, &consumed);
        CHECK(produced <= 7U);
        CHECK(consumed <= 300U - i);
        total += produced;
        i += consumed;
        if (produced == 0U && consumed == 0U) break;
    }
    /* Everything consumed even with a tiny output window. */
    CHECK(i == 300U);
    CHECK(total >= 295U && total <= 298U);
    for (j = 1U; j < total; ++j) CHECK(out[j] >= out[j - 1U]); /* monotone ramp stays monotone */
}

static void test_sine_fidelity_and_rate(void)
{
    enum { IN_RATE = 16125, OUT_RATE = 16000, SECONDS = 2 };
    static int16_t in[IN_RATE * SECONDS];
    static int16_t out[OUT_RATE * SECONDS + 8];
    bc_resampler r;
    size_t i, produced, consumed;
    double freq = 1000.0, signal = 0.0, noise = 0.0;
    for (i = 0; i < (size_t)IN_RATE * SECONDS; ++i)
        in[i] = (int16_t)lrint(20000.0 * sin(2.0 * M_PI * freq * (double)i / IN_RATE));
    CHECK(bc_resampler_init(&r, IN_RATE, OUT_RATE));
    produced = bc_resampler_process(&r, in, (size_t)IN_RATE * SECONDS, out,
                                    sizeof(out) / sizeof(out[0]), &consumed);
    CHECK(consumed == (size_t)IN_RATE * SECONDS);
    /* Output sample k sits exactly at input position k * in/out; the window
     * primes with x[-1] = 0 and introduces no delay (see the unity test). */
    for (i = 200U; i + 200U < produced; ++i) {
        double t = ((double)i * r.in_rate / r.out_rate) / IN_RATE;
        double ideal = 20000.0 * sin(2.0 * M_PI * freq * t);
        double error = (double)out[i] - ideal;
        signal += ideal * ideal;
        noise += error * error;
    }
    CHECK(noise > 0.0);
    CHECK(10.0 * log10(signal / noise) > 40.0);
}

static void test_saturation_and_reset(void)
{
    int16_t in[16], out[32];
    bc_resampler r;
    size_t produced, consumed, i;
    for (i = 0; i < 16U; ++i) in[i] = (i & 1U) ? 32767 : -32768;
    CHECK(bc_resampler_init(&r, 3U, 2U));
    produced = bc_resampler_process(&r, in, 16U, out, 32U, &consumed);
    CHECK(consumed == 16U && produced > 0U);
    for (i = 0; i < produced; ++i) CHECK(out[i] >= -32768 && out[i] <= 32767);
    bc_resampler_reset(&r);
    CHECK(r.phase == 0U && r.filled == 0U && r.consumed == 0U && r.produced == 0U);
    CHECK(bc_resampler_output_bound(&r, 3U) >= 2U);
    CHECK(bc_resampler_init(&r, 16125U, 16000U));
    CHECK(bc_resampler_output_bound(&r, 129U) >= 128U && bc_resampler_output_bound(&r, 129U) <= 130U);
}

int main(void)
{
    test_ratio_is_reduced_and_exact();
    test_unity_ratio_passes_samples_through();
    test_16125_to_16000_count_and_chunk_equivalence();
    test_output_capacity_limits_and_resumes();
    test_sine_fidelity_and_rate();
    test_saturation_and_reset();
    printf("resampler: %u checks, %u failures\n", checks, failures);
    return failures == 0U ? 0 : 1;
}
