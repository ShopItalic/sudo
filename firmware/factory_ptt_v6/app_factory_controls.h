#ifndef APP_FACTORY_CONTROLS_H
#define APP_FACTORY_CONTROLS_H
#include <stdbool.h>
#include <stdint.h>

/* P06 extends the P04/P05 ABI: 0 disabled, 1 hold/release PTT, 2 tap recording toggle.
 * Defaults: hold=1, double=0, triple=0, haptics=1, recording light=1, hold delay=500 ms. Ring is authoritative. */
#define FACTORY_HOLD_MIN_STEPS 1U
#define FACTORY_HOLD_MAX_STEPS 10U
#define FACTORY_HOLD_DEFAULT_STEPS 1U
#define FACTORY_HOLD_STEP_MS 500U
#define FACTORY_CONTROLS_FILE 0x1001U
#define FACTORY_CONTROLS_KEY  0x0001U
void app_factory_controls_init(bool peer_manager_owns_fds); /* before pm_init */
void app_factory_controls_service(void); /* command worker only */
bool app_factory_controls_command(const uint8_t *data, unsigned length);
bool app_factory_controls_ready(void);
bool app_factory_controls_haptics(void);
bool app_factory_controls_recording_light(void);
unsigned app_factory_controls_hold_delay_ms(void); /* 500..5000, 500 ms steps */
unsigned app_factory_controls_action(unsigned gesture); /* 0 hold, 1 double, 2 triple */
void bc_linear_motor_silence(void);
void bc_linear_motor_service(void); /* command worker only */
#endif
