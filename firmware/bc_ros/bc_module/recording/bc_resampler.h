#ifndef BC_RESAMPLER_H
#define BC_RESAMPLER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Bounded fixed-point rational resampler for mono 16-bit PCM. The position
 * of every output sample is tracked as an exact integer fraction of the
 * input rate, so a 16125 -> 16000 conversion never drifts. Interpolation is
 * a four-tap Catmull-Rom (cubic Hermite) kernel evaluated in Q15 with 64-bit
 * intermediates and saturation; no heap, no floating point. */
typedef struct {
    uint32_t in_rate;   /* reduced ratio numerator */
    uint32_t out_rate;  /* reduced ratio denominator */
    uint32_t phase;     /* fractional input position numerator, < out_rate */
    int16_t window[4];  /* x[i-1], x[i], x[i+1], x[i+2] */
    uint8_t filled;     /* input samples received while priming (0..3) */
    uint64_t consumed;  /* input samples accepted */
    uint64_t produced;  /* output samples emitted */
} bc_resampler;

bool bc_resampler_init(bc_resampler *r, uint32_t in_rate_hz, uint32_t out_rate_hz);
void bc_resampler_reset(bc_resampler *r);
/* Consumes input samples until either all are used or out_capacity is
 * reached. Returns the number of outputs written; *consumed reports how many
 * inputs were accepted so a caller can resume from the remainder. */
size_t bc_resampler_process(bc_resampler *r, const int16_t *in, size_t in_count,
                            int16_t *out, size_t out_capacity, size_t *consumed);
/* Upper bound on outputs that in_count inputs can produce. */
size_t bc_resampler_output_bound(const bc_resampler *r, size_t in_count);

#endif
