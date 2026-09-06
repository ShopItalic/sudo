#ifndef __APP_AUTHENTICATION_HANDLER_H__
#define __APP_AUTHENTICATION_HANDLER_H__


#include <stdint.h>


void app_authentcation_states_get(uint8_t *status,uint8_t*code,uint8_t *code_length);


int app_authentication_activate_algorithm_key(uint8_t *key_code,uint8_t key_length);


void app_authentcation_req_code_get(uint8_t*code,uint8_t *code_length);


void app_authentication_info_init(void);


#endif


