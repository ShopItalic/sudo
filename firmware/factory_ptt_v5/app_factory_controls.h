#ifndef APP_FACTORY_CONTROLS_H
#define APP_FACTORY_CONTROLS_H
#include <stdbool.h>
#include <stdint.h>

/* P04 ABI: 0 disabled, 1 hold/release PTT, 2 tap recording toggle.
 * Defaults: hold=1, double=0, triple=0, haptics=1. Ring is authoritative. */
#define FACTORY_CONTROLS_FILE 0x1001U
#define FACTORY_CONTROLS_KEY  0x0001U
void app_factory_controls_init(bool peer_manager_owns_fds); /* before pm_init */
void app_factory_controls_service(void); /* command worker only */
bool app_factory_controls_command(const uint8_t *data, unsigned length);
bool app_factory_controls_ready(void);
bool app_factory_controls_haptics(void);
unsigned app_factory_controls_action(unsigned gesture); /* 0 hold, 1 double, 2 triple */
void bc_linear_motor_silence(void);
void bc_linear_motor_service(void); /* command worker only */
#endif
