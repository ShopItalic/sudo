#ifndef APP_SUDO_CAPTURE_H
#define APP_SUDO_CAPTURE_H

#include "bc_recording.h"
#include "bc_voice_service.h"
#include "FreeRTOS.h"
#include "task.h"

/* One recording worker owns this adapter. Its wake notification is also used
 * for queued commands; the worker polls until no capture work remains. */
void app_sudo_capture_init(TaskHandle_t worker);
/* Prepares the Opus encoder once on the worker: allocates the codec state
 * and bounded scratch from the RTOS heap, validates the profile with a
 * self-test frame, and derives the recording descriptor. Returns false when
 * recording is unavailable; Start then fails explicitly instead of labeling
 * audio with a codec that cannot run. */
bool app_sudo_capture_prepare(void);
/* Descriptor every new recording receives; false until prepared. */
bool app_sudo_capture_format(bc_audio_format *format);
/* Audio port for the voice service: descriptor plus encoder instrumentation. */
bool app_sudo_capture_audio_stats(void *ctx, bc_audio_format *format,
                                  bc_voice_audio_stats *stats);
/* Exact real sample count of the most recent recording with this ID, valid
 * once its final frames were delivered to the owner. Zero otherwise. */
uint32_t app_sudo_capture_sample_count(uint64_t id);
/* PTT sensor liveness is enforced at DMA boundaries even while the storage
 * worker is busy. Call arm after successful capture_start; touch from the
 * sensor task renews a live lease or requests stop on release/error. */
void app_sudo_capture_ptt_arm(uint64_t id, uint32_t lease_ms, uint32_t limit_ms);
void app_sudo_capture_ptt_touch(uint64_t id, bool valid, bool contact);
bc_rec_result app_sudo_capture_start(void *ctx, uint64_t id);
bc_rec_result app_sudo_capture_stop(void *ctx, uint64_t id);
bool app_sudo_capture_abort(void *ctx, uint64_t id);
bool app_sudo_capture_poll(bc_recording *owner, uint32_t now_ms);

#endif
