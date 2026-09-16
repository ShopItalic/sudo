#include <stdbool.h>
#include <stdint.h>
bool bc_g_sensor_i2c_read(uint8_t, uint8_t, uint8_t *, uint8_t);
bool bc_g_sensor_i2c_write(uint8_t, uint8_t, uint8_t *, uint8_t);
void bc_g_sensor_i2c_open(void);
void bc_g_sensor_i2c_close(void);
