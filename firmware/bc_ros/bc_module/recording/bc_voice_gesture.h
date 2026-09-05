#ifndef BC_VOICE_GESTURE_H
#define BC_VOICE_GESTURE_H

#include "bc_recording.h"
#include "bc_touch_report.h"

typedef struct {
    uint32_t ptt_limit_ms;  /* 0 means until release. Independent of memo. */
    uint32_t memo_limit_ms;
    uint16_t touch_timeout_ms; /* Missing sensor reports end PTT as partial. */
    uint16_t double_tap_debounce_ms;
    bool memo_enabled;
} bc_voice_gesture_config;

typedef struct {
    bc_recording *recording;
    void *id_ctx;
    bc_rec_result (*new_id)(void *ctx, uint64_t *id);
    bc_voice_gesture_config config;
    uint64_t ptt_id;
    uint32_t last_report_ms;
    uint32_t last_double_ms;
    bool hold_attempted;
    bool have_report;
    bool have_double;
} bc_voice_gesture;

bool bc_voice_gesture_init(bc_voice_gesture *gesture, bc_recording *recording,
                            const bc_voice_gesture_config *config,
                            bc_rec_result (*new_id)(void *, uint64_t *), void *ctx);
bc_rec_result bc_voice_gesture_configure(bc_voice_gesture *gesture,
                                         const bc_voice_gesture_config *config);
/* Called on the same worker as recording. The physical IQS hold threshold is
 * applied by the sensor; a held finger gets only one Start attempt until an
 * explicit release. Link state never changes gesture behavior. */
bc_rec_result bc_voice_gesture_report(bc_voice_gesture *gesture,
                                      const bc_touch_report_t *report,
                                      uint32_t now_ms);
void bc_voice_gesture_tick(bc_voice_gesture *gesture, uint32_t now_ms);

#endif
