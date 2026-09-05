#ifndef ETA4662_H
#define ETA4662_H

#include <stdint.h>
#include "stdbool.h"

enum eta4662_charge_status
{
	ETA4662_CHARGED_NOT = 0,
	ETA4662_CHARGED_ING,
	ETA4662_CHARGED_OVER,
};


typedef bool (*eta4662_i2c_read_callback)(uint8_t reg_add ,uint8_t *data,uint8_t length); 
typedef bool (*eta4662_i2c_write_callback)(uint8_t reg_add ,uint8_t data,uint8_t length); 
typedef void (*eta4662_i2c_init_callback)(void); 
typedef void (*eta4662_i2c_open_callback)(void); 
typedef void (*eta4662_i2c_close_callback)(void);
typedef void (*eta4662_i2c_int_chg_callback)(void);

struct eta4662_i2c_port
{
	eta4662_i2c_read_callback read_callback;
	eta4662_i2c_write_callback write_callback;
	eta4662_i2c_init_callback  init_callback;
	eta4662_i2c_open_callback  open_callback;
	eta4662_i2c_close_callback close_callback;
	eta4662_i2c_int_chg_callback int_chg_open_callback;
	eta4662_i2c_int_chg_callback int_chg_close_callback;
};


bool eta4662_check_chip_id(void);

void eta4662_feed_dog(void);

enum eta4662_charge_status eta4662_charge_get_status(void);

bool eta4662_get_chip_id(uint8_t *data);

bool eta4662_gte_reg_all(uint8_t *data);

bool eta4662_init(void);

bool eta4662_hardware_error_register_callback(const void *error_callback);

void eta4662_shlp_mode(void);

void eta4662_int_close(void);

void eta4662_find(void);


#endif
