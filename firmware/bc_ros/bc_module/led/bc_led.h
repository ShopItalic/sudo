#ifndef __BC_LED_H__
#define __BC_LED_H__




void bc_led_test_on(void);

void bc_led_test_off(void);

void bc_led_device_find(void);




void bc_led_blue_on(void);

void bc_led_blue_off(void);

void bc_led_red_on(void);

void bc_led_red_off(void);
void bc_led_blue_toggle(void);
void bc_led_red_toggle(void);

void bc_led_low_power_flash_start(void);

void bc_led_low_power_flash_stop(void);

void bc_led_hardware_check_error_hint_flash_start(void);

void bc_led_hardware_check_error_hint_flash_stop(void);

void bc_led_charge_flash_start(void);

void bc_led_charge_flash_stop(void);

void bc_led_ble_connect_hint(void);

void bc_led_ble_disconnect_hint(void);

void bc_led_charge_over_start(void);

void bc_led_charge_over_stop(void);

void bc_led_all_on(void);

void bc_led_all_off(void);

#endif

