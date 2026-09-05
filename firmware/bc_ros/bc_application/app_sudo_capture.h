#ifndef APP_SUDO_CAPTURE_H
#define APP_SUDO_CAPTURE_H

#include "bc_recording.h"
#include "FreeRTOS.h"
#include "task.h"

/* One recording worker owns this adapter. Its wake notification is also used
 * for queued commands; the worker polls until no capture work remains. */
void app_sudo_capture_init(TaskHandle_t worker);
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
