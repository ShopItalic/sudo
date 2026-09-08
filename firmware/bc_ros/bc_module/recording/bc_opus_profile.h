#ifndef BC_OPUS_PROFILE_H
#define BC_OPUS_PROFILE_H

/* Centralized Sudo Voice Opus recording profile. Every codec tunable lives
 * here so a later battery/quality change is one reviewed edit. Values are
 * engineering defaults; physical CPU, battery and audio measurements remain
 * qualification gates and are not claimed by this header. */

/* The standard 1.23.2 microphone path is untouched: PDM clock 1.032 MHz with
 * the nRF52840 default decimation ratio of 64 yields this nominal PCM rate. */
#define BC_OPUS_PCM_NOMINAL_HZ 16125U
#define BC_OPUS_SAMPLE_RATE_HZ 16000U
#define BC_OPUS_CHANNELS 1U
#define BC_OPUS_FRAME_MS 20U
#define BC_OPUS_FRAME_SAMPLES \
    (BC_OPUS_SAMPLE_RATE_HZ * BC_OPUS_FRAME_MS / 1000U)
#define BC_OPUS_BITRATE_BPS 12000U
#define BC_OPUS_COMPLEXITY 0
#define BC_OPUS_VBR 0
#define BC_OPUS_DTX 0
#define BC_OPUS_INBAND_FEC 0
/* OPUS_APPLICATION_VOIP with OPUS_SIGNAL_VOICE; see bc_opus_encoder.h. */
#define BC_OPUS_APPLICATION BC_OPUS_APP_VOIP

/* Bound of one encoded packet from this profile. The encoder emits one frame
 * per packet, so RFC 6716's 1,275-byte frame limit (section 3.2.1) is the
 * packet limit here and libopus is told the same maximum. It is not a codec
 * packet limit: a multi-frame packet may legitimately exceed 1,275 bytes, and
 * phone readers accept any valid packet up to the container's u16 record
 * length. The stream container and the transport fragment packets larger
 * than one chunk instead of rejecting them. */
#define BC_OPUS_PACKET_MAX 1275U
/* Largest mono frame at any supported Opus rate (60 ms at 48 kHz is 2880;
 * the profile uses 20 ms so 960 covers every rate up to 48 kHz). */
#define BC_OPUS_FRAME_SAMPLES_MAX 960U

/* libopus temporary allocation is a non-thread-safe pseudostack owned by the
 * recording worker. The host codec suite measures its high-water mark for
 * the profile above; this bound includes review margin and is enforced with
 * a canary on the target. */
#define BC_OPUS_SCRATCH_BYTES 20480U
/* opus_encoder_get_size(1) for the fixed-point build is checked at init
 * against the buffer actually provided; this is the review upper bound. */
#define BC_OPUS_ENCODER_STATE_MAX 16384U
/* Canary bytes appended after the pseudostack on the target. */
#define BC_OPUS_SCRATCH_CANARY_BYTES 64U

#endif
