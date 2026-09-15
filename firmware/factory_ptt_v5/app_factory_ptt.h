#ifndef APP_FACTORY_PTT_V5_H
#define APP_FACTORY_PTT_V5_H
#include <stdbool.h>
#include <stdint.h>

/* Sensor/task calls only publish intent. No recorder or filesystem calls. */
void app_factory_ptt_report(const uint8_t data[8], bool ok);
void app_factory_ptt_cancel(void);
void app_factory_ptt_double_tap(void);
bool app_factory_ptt_triple_tap(void); /* true when a recording action is assigned */
bool app_factory_ptt_begin_settings(void);
void app_factory_ptt_end_settings(void);

/* These functions are called only by the existing BLE command worker. */
void app_factory_ptt_worker_init(void);
void app_factory_ptt_worker_service(void);
void app_factory_ptt_before_command(const uint8_t *data, unsigned length);
void app_factory_ptt_worker_wait(void);
bool app_factory_ptt_status_command(const uint8_t *data, unsigned length);

typedef struct {
    bool armed, wanted, attempted, owned, sampled;
    bool contact, settings_lock, memo_owned, toggle_pending;
    uint8_t suppress_taps;
    uint32_t last_tick, samples, starts, stops, faults, timeouts;
    uint32_t start_failures, stop_failures;
    uint32_t recording_sequence;
    uint8_t recording_state; /* 0 closed/idle, 1 recording, 2 outcome uncertain */
} app_factory_ptt_state;
extern app_factory_ptt_state factory_ptt_diagnostics;
#endif
