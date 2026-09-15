#ifndef APP_FACTORY_SCROLL_H
#define APP_FACTORY_SCROLL_H
#include <stdint.h>

/* P09 only. Settings stay in the controls FDS record; no provisioning writes. */
void app_factory_scroll_init(void);
void app_factory_scroll_input(unsigned direction); /* touch task: bounded intent only */
void app_factory_scroll_service(void); /* BLE command worker: one report per pass */
void app_factory_scroll_reset(void); /* cancel intents when settings change */

#define FACTORY_SCROLL_REPORT_BYTES 5U
#define FACTORY_SCROLL_QUEUE_SIZE 4U
#define FACTORY_SCROLL_MAX_AGE_MS 250U
#define FACTORY_SCROLL_STEP 4
extern volatile uint32_t factory_scroll_init_error, factory_scroll_last_error;
extern volatile uint32_t factory_scroll_sent, factory_scroll_dropped;
#endif
