#ifndef sy6103_H
#define sy6103_H

#include <stdint.h>
#include "stdbool.h"

enum sy6103_charge_status
{
	SY6103_CHARGED_NOT = 0,
	SY6103_CHARGED_ING,
	SY6103_CHARGED_OVER,
};


typedef bool (*sy6103_i2c_read_callback)(uint8_t reg_add ,uint8_t *data,uint8_t length); 
typedef bool (*sy6103_i2c_write_callback)(uint8_t reg_add ,uint8_t data,uint8_t length); 
typedef void (*sy6103_i2c_init_callback)(void); 
typedef void (*sy6103_i2c_open_callback)(void); 
typedef void (*sy6103_i2c_close_callback)(void);
typedef void (*sy6103_i2c_int_chg_callback)(void);

struct sy6103_i2c_port
{
	sy6103_i2c_read_callback read_callback;
	sy6103_i2c_write_callback write_callback;
	sy6103_i2c_init_callback  init_callback;
	sy6103_i2c_open_callback  open_callback;
	sy6103_i2c_close_callback close_callback;
	sy6103_i2c_int_chg_callback int_chg_open_callback;
	sy6103_i2c_int_chg_callback int_chg_close_callback;
};


bool sy6103_check_chip_id(void);

void sy6103_feed_dog(void);

enum sy6103_charge_status sy6103_charge_get_status(void);

bool sy6103_get_chip_id(uint8_t *data);

bool sy6103_gte_reg_all(uint8_t *data);

bool sy6103_init(void);

bool sy6103_hardware_error_register_callback(const void *error_callback);

void sy6103_shlp_mode(void);

void sy6103_int_close(void);

void sy6103_find(void);


#endif
