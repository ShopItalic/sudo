#ifndef APP_FACTORY_MOTION_OFF_H
#define APP_FACTORY_MOTION_OFF_H
#include <stdbool.h>
#include <stdint.h>
enum { FACTORY_MOTION_NOT_CHECKED, FACTORY_MOTION_OFF,
       FACTORY_MOTION_IO_FAILED, FACTORY_MOTION_WRONG_DEVICE };
/* Debugger-visible proof of attempted shutdown; not a physical power reading. */
extern volatile unsigned app_factory_motion_status;
void app_factory_motion_off(void);
uint8_t app_factory_motion_id(void);
#endif
