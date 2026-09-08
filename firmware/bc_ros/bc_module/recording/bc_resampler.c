#include "bc_resampler.h"

#include <string.h>

static uint32_t gcd32(uint32_t a, uint32_t b)
{
    while (b != 0U) {
        uint32_t t = a % b;
        a = b;
        b = t;
    }
    return a;
}

bool bc_resampler_init(bc_resampler *r, uint32_t in_rate_hz, uint32_t out_rate_hz)
{
    uint32_t g;
    if (r == NULL || in_rate_hz == 0U || out_rate_hz == 0U ||
        in_rate_hz > 192000U || out_rate_hz > 192000U)
        return false;
    memset(r, 0, sizeof(*r));
    g = gcd32(in_rate_hz, out_rate_hz);
    r->in_rate = in_rate_hz / g;
    r->out_rate = out_rate_hz / g;
    return true;
}

void bc_resampler_reset(bc_resampler *r)
{
    if (r == NULL) return;
    r->phase = 0U;
    memset(r->window, 0, sizeof(r->window));
    r->filled = 0U;
    r->consumed = 0U;
    r->produced = 0U;
}

size_t bc_resampler_output_bound(const bc_resampler *r, size_t in_count)
{
    uint64_t bound;
    if (r == NULL || r->in_rate == 0U) return 0U;
    bound = ((uint64_t)in_count * r->out_rate + r->in_rate - 1U) / r->in_rate;
    return (size_t)(bound + 1U);
}

static int16_t interpolate(const int16_t w[4], uint32_t phase, uint32_t out_rate)
{
    /* t in Q15; coefficients scaled by two to keep integer arithmetic. */
    int64_t t = ((int64_t)phase << 15) / (int64_t)out_rate;
    int64_t w0 = w[0], w1 = w[1], w2 = w[2], w3 = w[3];
    int64_t c1 = w2 - w0;
    int64_t c2 = 2 * w0 - 5 * w1 + 4 * w2 - w3;
    int64_t c3 = w3 - w0 + 3 * (w1 - w2);
    int64_t acc = (c3 * t) >> 15;
    acc = ((c2 + acc) * t) >> 15;
    acc = ((c1 + acc) * t) >> 15;
    acc = w1 + (acc >> 1);
    if (acc > 32767) acc = 32767;
    if (acc < -32768) acc = -32768;
    return (int16_t)acc;
}

size_t bc_resampler_process(bc_resampler *r, const int16_t *in, size_t in_count,
                            int16_t *out, size_t out_capacity, size_t *consumed)
{
    size_t used = 0U;
    size_t made = 0U;
    if (consumed != NULL) *consumed = 0U;
    if (r == NULL || r->in_rate == 0U || (in_count != 0U && in == NULL) ||
        (out_capacity != 0U && out == NULL))
        return 0U;
    while (used < in_count) {
        /* Outputs at fractional positions inside [x[i], x[i+1]) need x[i+2].
         * Shift the window only when its dependent outputs are finished. */
        if (r->filled < 3U) {
            r->window[r->filled + 1U] = in[used++];
            ++r->filled;
            ++r->consumed;
            if (r->filled < 3U) continue;
        } else {
            if (r->phase < r->out_rate) {
                /* Emit every output belonging to the current window. */
                if (made == out_capacity) break;
                out[made++] = interpolate(r->window, r->phase, r->out_rate);
                ++r->produced;
                r->phase += r->in_rate;
                continue;
            }
            r->phase -= r->out_rate;
            r->window[0] = r->window[1];
            r->window[1] = r->window[2];
            r->window[2] = r->window[3];
            r->window[3] = in[used++];
            ++r->consumed;
            continue;
        }
        /* Window primed with x[-1]=0, x[0], x[1], x[2]; emit before the
         * next shift consumes another input. */
        while (r->phase < r->out_rate && made < out_capacity) {
            out[made++] = interpolate(r->window, r->phase, r->out_rate);
            ++r->produced;
            r->phase += r->in_rate;
        }
        if (r->phase < r->out_rate) break;
    }
    /* Drain outputs still owed by the current window after the last input. */
    while (r->filled >= 3U && r->phase < r->out_rate && made < out_capacity) {
        out[made++] = interpolate(r->window, r->phase, r->out_rate);
        ++r->produced;
        r->phase += r->in_rate;
    }
    if (consumed != NULL) *consumed = used;
    return made;
}
