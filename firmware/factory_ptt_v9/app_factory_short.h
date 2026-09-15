#ifndef APP_FACTORY_SHORT_P09_H
#define APP_FACTORY_SHORT_P09_H
#include <stdbool.h>
#include <stdint.h>
#define FACTORY_SHORT_MAX_STEPS 10U
#define FACTORY_SHORT_DEFAULT_STEPS 4U
#define FACTORY_SHORT_SAMPLES_PER_STEP 4000U
/* Duration settings: zero keeps every recording, otherwise 500 ms steps. */
void app_factory_short_init(void);
void app_factory_short_service(void);
bool app_factory_short_command(const uint8_t *data, unsigned length);
unsigned app_factory_short_steps(void); /* zero while settings are unverified */

/* Capture bookkeeping is independent of filenames and persistent settings.
 * Task/ISR callers only update bounded state; no filesystem work here. */
bool factory_capture_begin(unsigned steps);
void factory_capture_enable(void);
void factory_capture_ready(void);
bool factory_capture_producer_enter(void);
void factory_capture_producer_leave(void);
bool factory_capture_queue_begin(uint8_t *header);
void factory_capture_queue_failed(void);
bool factory_capture_sender_enter(const uint8_t *header);
void factory_capture_sender_leave(void);
void factory_capture_written(unsigned bytes, bool success);
void factory_capture_fault(void);
void factory_capture_isr_fault(void);
void factory_capture_stop_accepting(void);
bool factory_capture_quiet(void);
bool factory_capture_should_discard(void);
void factory_capture_end(void);
bool factory_capture_active(void);
bool factory_capture_succeeded(void);
bool app_factory_capture_bind_file(void);
bool app_factory_capture_finish_file(void);
#endif
